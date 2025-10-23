/**
 * Connection Manager
 * 
 * Manages service connection status, monitoring, and automatic reconnection
 */

import { EventEmitter } from 'events';
import { ServiceCommunicator } from './ServiceCommunicator';
import { Logger } from '../utils/Logger';

export interface ConnectionStatus {
  connected: boolean;
  lastConnected: Date | null;
  lastDisconnected: Date | null;
  reconnectAttempts: number;
  maxReconnectAttempts: number;
  reconnectDelay: number;
  isReconnecting: boolean;
  error: string | null;
}

export interface ConnectionHealth {
  status: 'healthy' | 'degraded' | 'unhealthy' | 'disconnected';
  latency: number | null;
  lastHeartbeat: Date | null;
  consecutiveFailures: number;
  uptime: number; // seconds
}

export class ConnectionManager extends EventEmitter {
  private serviceCommunicator: ServiceCommunicator;
  private logger: Logger;
  private status: ConnectionStatus;
  private health: ConnectionHealth;
  private reconnectTimer: NodeJS.Timeout | null = null;
  private healthCheckTimer: NodeJS.Timeout | null = null;
  private heartbeatTimer: NodeJS.Timeout | null = null;
  private connectionStartTime: Date | null = null;
  
  // Configuration
  private readonly healthCheckInterval = 30000; // 30 seconds
  private readonly heartbeatInterval = 10000; // 10 seconds
  private readonly healthyLatencyThreshold = 1000; // 1 second
  private readonly degradedLatencyThreshold = 5000; // 5 seconds

  constructor(serviceCommunicator: ServiceCommunicator, logger: Logger) {
    super();
    this.serviceCommunicator = serviceCommunicator;
    this.logger = logger;
    
    this.status = {
      connected: false,
      lastConnected: null,
      lastDisconnected: null,
      reconnectAttempts: 0,
      maxReconnectAttempts: 5,
      reconnectDelay: 2000,
      isReconnecting: false,
      error: null,
    };
    
    this.health = {
      status: 'disconnected',
      latency: null,
      lastHeartbeat: null,
      consecutiveFailures: 0,
      uptime: 0,
    };
    
    this.setupServiceListeners();
  }

  /**
   * Setup service event listeners
   */
  private setupServiceListeners(): void {
    this.serviceCommunicator.on('connected', () => {
      this.handleServiceConnected();
    });

    this.serviceCommunicator.on('disconnected', () => {
      this.handleServiceDisconnected();
    });

    this.serviceCommunicator.on('connection-failed', () => {
      this.handleConnectionFailed();
    });
  }

  /**
   * Start connection management
   */
  public async start(): Promise<void> {
    this.logger.info('Starting connection manager');
    
    try {
      await this.connect();
      this.startHealthMonitoring();
      this.logger.info('Connection manager started successfully');
    } catch (error) {
      this.logger.error('Failed to start connection manager', error);
      throw error;
    }
  }

  /**
   * Stop connection management
   */
  public async stop(): Promise<void> {
    this.logger.info('Stopping connection manager');
    
    this.stopHealthMonitoring();
    this.stopReconnectTimer();
    
    if (this.status.connected) {
      await this.disconnect();
    }
    
    this.logger.info('Connection manager stopped');
  }

  /**
   * Connect to service
   */
  public async connect(): Promise<void> {
    if (this.status.connected || this.status.isReconnecting) {
      return;
    }

    try {
      this.logger.info('Connecting to service');
      this.status.isReconnecting = true;
      this.status.error = null;
      
      await this.serviceCommunicator.connect();
      
    } catch (error) {
      this.status.isReconnecting = false;
      const errorMessage = error instanceof Error ? error.message : 'Unknown connection error';
      this.status.error = errorMessage;
      this.logger.error('Failed to connect to service', error);
      throw error;
    }
  }

  /**
   * Disconnect from service
   */
  public async disconnect(): Promise<void> {
    this.logger.info('Disconnecting from service');
    
    this.stopHealthMonitoring();
    this.stopReconnectTimer();
    
    try {
      await this.serviceCommunicator.disconnect();
    } catch (error) {
      this.logger.error('Error during service disconnection', error);
    }
  }

  /**
   * Handle service connected event
   */
  private handleServiceConnected(): void {
    this.logger.info('Service connection established');
    
    this.status.connected = true;
    this.status.lastConnected = new Date();
    this.status.reconnectAttempts = 0;
    this.status.isReconnecting = false;
    this.status.error = null;
    this.connectionStartTime = new Date();
    
    this.health.status = 'healthy';
    this.health.consecutiveFailures = 0;
    this.health.lastHeartbeat = new Date();
    
    this.startHeartbeat();
    this.emit('connected', this.getConnectionInfo());
  }

  /**
   * Handle service disconnected event
   */
  private handleServiceDisconnected(): void {
    this.logger.warn('Service connection lost');
    
    this.status.connected = false;
    this.status.lastDisconnected = new Date();
    this.connectionStartTime = null;
    
    this.health.status = 'disconnected';
    this.health.latency = null;
    this.health.lastHeartbeat = null;
    this.health.uptime = 0;
    
    this.stopHeartbeat();
    this.emit('disconnected', this.getConnectionInfo());
    
    // Start reconnection process
    this.startReconnection();
  }

  /**
   * Handle connection failed event
   */
  private handleConnectionFailed(): void {
    this.logger.error('Service connection failed permanently');
    
    this.status.isReconnecting = false;
    this.status.error = 'Connection failed permanently';
    
    this.health.status = 'disconnected';
    this.health.consecutiveFailures++;
    
    this.emit('connection-failed', this.getConnectionInfo());
  }

  /**
   * Start reconnection process
   */
  private startReconnection(): void {
    if (this.status.isReconnecting || this.reconnectTimer) {
      return;
    }

    if (this.status.reconnectAttempts >= this.status.maxReconnectAttempts) {
      this.logger.error('Max reconnection attempts reached');
      this.handleConnectionFailed();
      return;
    }

    this.status.reconnectAttempts++;
    this.status.isReconnecting = true;
    
    const delay = this.calculateReconnectDelay();
    this.logger.info(`Scheduling reconnection attempt ${this.status.reconnectAttempts}/${this.status.maxReconnectAttempts} in ${delay}ms`);
    
    this.reconnectTimer = setTimeout(async () => {
      this.reconnectTimer = null;
      
      try {
        await this.connect();
      } catch (error) {
        this.logger.error(`Reconnection attempt ${this.status.reconnectAttempts} failed`, error);
        this.status.isReconnecting = false;
        
        // Schedule next attempt
        this.startReconnection();
      }
    }, delay);
  }

  /**
   * Calculate reconnection delay with exponential backoff
   */
  private calculateReconnectDelay(): number {
    const baseDelay = this.status.reconnectDelay;
    const exponentialDelay = baseDelay * Math.pow(2, this.status.reconnectAttempts - 1);
    const maxDelay = 30000; // 30 seconds max
    
    return Math.min(exponentialDelay, maxDelay);
  }

  /**
   * Stop reconnection timer
   */
  private stopReconnectTimer(): void {
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }
    this.status.isReconnecting = false;
  }

  /**
   * Start health monitoring
   */
  private startHealthMonitoring(): void {
    this.stopHealthMonitoring();
    
    this.healthCheckTimer = setInterval(() => {
      this.performHealthCheck();
    }, this.healthCheckInterval);
    
    this.logger.debug('Health monitoring started');
  }

  /**
   * Stop health monitoring
   */
  private stopHealthMonitoring(): void {
    if (this.healthCheckTimer) {
      clearInterval(this.healthCheckTimer);
      this.healthCheckTimer = null;
    }
    
    this.stopHeartbeat();
    this.logger.debug('Health monitoring stopped');
  }

  /**
   * Start heartbeat monitoring
   */
  private startHeartbeat(): void {
    this.stopHeartbeat();
    
    this.heartbeatTimer = setInterval(() => {
      this.sendHeartbeat();
    }, this.heartbeatInterval);
    
    this.logger.debug('Heartbeat monitoring started');
  }

  /**
   * Stop heartbeat monitoring
   */
  private stopHeartbeat(): void {
    if (this.heartbeatTimer) {
      clearInterval(this.heartbeatTimer);
      this.heartbeatTimer = null;
    }
  }

  /**
   * Send heartbeat to service
   */
  private async sendHeartbeat(): Promise<void> {
    if (!this.status.connected) {
      return;
    }

    try {
      const startTime = Date.now();
      
      // Send a simple ping to the service
      await this.serviceCommunicator.ping();
      
      const latency = Date.now() - startTime;
      this.health.latency = latency;
      this.health.lastHeartbeat = new Date();
      this.health.consecutiveFailures = 0;
      
      // Update health status based on latency
      if (latency <= this.healthyLatencyThreshold) {
        this.health.status = 'healthy';
      } else if (latency <= this.degradedLatencyThreshold) {
        this.health.status = 'degraded';
      } else {
        this.health.status = 'unhealthy';
      }
      
      this.logger.debug(`Heartbeat successful: ${latency}ms`);
      
    } catch (error) {
      this.health.consecutiveFailures++;
      this.health.status = 'unhealthy';
      
      this.logger.warn(`Heartbeat failed (${this.health.consecutiveFailures} consecutive failures)`, error);
      
      // If too many consecutive failures, consider connection lost
      if (this.health.consecutiveFailures >= 3) {
        this.logger.error('Too many heartbeat failures, considering connection lost');
        this.handleServiceDisconnected();
      }
    }
  }

  /**
   * Perform comprehensive health check
   */
  private async performHealthCheck(): Promise<void> {
    if (!this.status.connected) {
      return;
    }

    try {
      // Update uptime
      if (this.connectionStartTime) {
        this.health.uptime = Math.floor((Date.now() - this.connectionStartTime.getTime()) / 1000);
      }
      
      // Emit health status
      this.emit('health-check', this.getHealthInfo());
      
      this.logger.debug('Health check completed', this.getHealthInfo());
      
    } catch (error) {
      this.logger.error('Health check failed', error);
    }
  }

  /**
   * Force reconnection
   */
  public async forceReconnect(): Promise<void> {
    this.logger.info('Forcing reconnection to service');
    
    this.stopReconnectTimer();
    this.status.reconnectAttempts = 0;
    
    if (this.status.connected) {
      await this.disconnect();
    }
    
    await this.connect();
  }

  /**
   * Get connection information
   */
  public getConnectionInfo(): ConnectionStatus {
    return { ...this.status };
  }

  /**
   * Get health information
   */
  public getHealthInfo(): ConnectionHealth {
    return { ...this.health };
  }

  /**
   * Get comprehensive status
   */
  public getStatus(): {
    connection: ConnectionStatus;
    health: ConnectionHealth;
    diagnostics: {
      hasReconnectTimer: boolean;
      hasHealthCheckTimer: boolean;
      hasHeartbeatTimer: boolean;
    };
  } {
    return {
      connection: this.getConnectionInfo(),
      health: this.getHealthInfo(),
      diagnostics: {
        hasReconnectTimer: this.reconnectTimer !== null,
        hasHealthCheckTimer: this.healthCheckTimer !== null,
        hasHeartbeatTimer: this.heartbeatTimer !== null,
      },
    };
  }

  /**
   * Update connection configuration
   */
  public updateConfig(config: {
    maxReconnectAttempts?: number;
    reconnectDelay?: number;
  }): void {
    if (config.maxReconnectAttempts !== undefined) {
      this.status.maxReconnectAttempts = config.maxReconnectAttempts;
    }
    
    if (config.reconnectDelay !== undefined) {
      this.status.reconnectDelay = config.reconnectDelay;
    }
    
    this.logger.info('Connection configuration updated', config);
  }

  /**
   * Check if service is healthy
   */
  public isHealthy(): boolean {
    return this.status.connected && this.health.status === 'healthy';
  }

  /**
   * Check if service is available (connected but may be degraded)
   */
  public isAvailable(): boolean {
    return this.status.connected && this.health.status !== 'disconnected';
  }
}