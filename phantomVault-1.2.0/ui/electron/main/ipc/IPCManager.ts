/**
 * IPC Manager
 * 
 * Central manager for all IPC handlers and communication
 */

import { BrowserWindow, WebContents } from 'electron';
import { VaultHandlers } from './vaultHandlers';
import { ActivityHandlers } from './activityHandlers';
import { SettingsHandlers } from './settingsHandlers';
import { ServiceCommunicator } from '../services/ServiceCommunicator';
import { Logger } from '../utils/Logger';

export class IPCManager {
  private logger: Logger;
  private serviceCommunicator: ServiceCommunicator;
  private vaultHandlers: VaultHandlers;
  private activityHandlers: ActivityHandlers;
  private settingsHandlers: SettingsHandlers;
  private mainWindow: BrowserWindow | null = null;

  constructor(logger: Logger) {
    this.logger = logger;
    this.serviceCommunicator = new ServiceCommunicator(logger);
    
    // Initialize handlers
    this.vaultHandlers = new VaultHandlers(this.serviceCommunicator, logger);
    this.activityHandlers = new ActivityHandlers(this.serviceCommunicator, logger);
    this.settingsHandlers = new SettingsHandlers(logger);
    
    this.setupServiceListeners();
  }

  /**
   * Set the main window reference
   */
  public setMainWindow(window: BrowserWindow): void {
    this.mainWindow = window;
    this.logger.info('Main window reference set for IPC manager');
  }

  /**
   * Initialize IPC communication
   */
  public async initialize(): Promise<void> {
    try {
      this.logger.info('Initializing IPC manager');
      
      // Connect to service
      await this.serviceCommunicator.connect();
      
      this.logger.info('IPC manager initialized successfully');
    } catch (error) {
      this.logger.error('Failed to initialize IPC manager', error);
      throw error;
    }
  }

  /**
   * Setup service event listeners
   */
  private setupServiceListeners(): void {
    // Service connection events
    this.serviceCommunicator.on('connected', () => {
      this.logger.info('Service connected');
      this.broadcastToRenderers('service:connected', { connected: true });
    });

    this.serviceCommunicator.on('disconnected', () => {
      this.logger.warn('Service disconnected');
      this.broadcastToRenderers('service:disconnected', { connected: false });
    });

    this.serviceCommunicator.on('connection-failed', () => {
      this.logger.error('Service connection failed permanently');
      this.broadcastToRenderers('service:connection-failed', { connected: false });
    });

    // Vault status changes
    this.serviceCommunicator.on('vault-status-changed', (data) => {
      this.logger.debug('Vault status changed', data);
      this.broadcastToRenderers('vault:status-changed', data);
    });

    // Activity log entries are handled by ActivityHandlers
  }

  /**
   * Broadcast message to all renderer processes
   */
  private broadcastToRenderers(channel: string, data: any): void {
    if (this.mainWindow && !this.mainWindow.isDestroyed()) {
      try {
        this.mainWindow.webContents.send(channel, data);
      } catch (error) {
        this.logger.warn(`Failed to send message to main window: ${channel}`, error);
      }
    }

    // Send to all other windows if any
    const allWindows = BrowserWindow.getAllWindows();
    for (const window of allWindows) {
      if (window !== this.mainWindow && !window.isDestroyed()) {
        try {
          window.webContents.send(channel, data);
        } catch (error) {
          this.logger.warn(`Failed to send message to window: ${channel}`, error);
        }
      }
    }
  }

  /**
   * Get service status
   */
  public getServiceStatus(): { connected: boolean; reconnectAttempts: number } {
    return this.serviceCommunicator.getServiceStatus();
  }

  /**
   * Get activity subscriber count
   */
  public getActivitySubscriberCount(): number {
    return this.activityHandlers.getSubscriberCount();
  }

  /**
   * Manually reconnect to service
   */
  public async reconnectToService(): Promise<void> {
    try {
      this.logger.info('Manually reconnecting to service');
      await this.serviceCommunicator.disconnect();
      await this.serviceCommunicator.connect();
      this.logger.info('Manual reconnection successful');
    } catch (error) {
      this.logger.error('Manual reconnection failed', error);
      throw error;
    }
  }

  /**
   * Send notification to renderers
   */
  public sendNotification(notification: {
    type: 'success' | 'error' | 'warning' | 'info';
    title: string;
    message: string;
    duration?: number;
  }): void {
    this.broadcastToRenderers('app:notification', notification);
  }

  /**
   * Handle application shutdown
   */
  public async shutdown(): Promise<void> {
    try {
      this.logger.info('Shutting down IPC manager');
      
      // Disconnect from service
      await this.serviceCommunicator.disconnect();
      
      // Cleanup handlers
      this.vaultHandlers.cleanup();
      this.activityHandlers.cleanup();
      this.settingsHandlers.cleanup();
      
      this.logger.info('IPC manager shutdown complete');
    } catch (error) {
      this.logger.error('Error during IPC manager shutdown', error);
    }
  }

  /**
   * Get diagnostic information
   */
  public getDiagnostics(): {
    serviceStatus: { connected: boolean; reconnectAttempts: number };
    activitySubscribers: number;
    mainWindowExists: boolean;
    totalWindows: number;
  } {
    return {
      serviceStatus: this.getServiceStatus(),
      activitySubscribers: this.getActivitySubscriberCount(),
      mainWindowExists: this.mainWindow !== null && !this.mainWindow.isDestroyed(),
      totalWindows: BrowserWindow.getAllWindows().length,
    };
  }

  /**
   * Force service restart
   */
  public async restartService(): Promise<void> {
    try {
      this.logger.info('Restarting service');
      
      // Notify renderers about restart
      this.broadcastToRenderers('service:restarting', {});
      
      // Disconnect and reconnect
      await this.serviceCommunicator.disconnect();
      
      // Wait a moment before reconnecting
      await new Promise(resolve => setTimeout(resolve, 1000));
      
      await this.serviceCommunicator.connect();
      
      this.logger.info('Service restart complete');
    } catch (error) {
      this.logger.error('Service restart failed', error);
      throw error;
    }
  }
}