/**
 * Service Communicator
 * 
 * Handles communication with the PhantomVault C++ service backend
 */

import { spawn, ChildProcess } from 'child_process';
import { EventEmitter } from 'events';
import * as path from 'path';
import * as fs from 'fs';
import { VaultConfig, VaultInfo, ActivityLogEntry } from '../../../src/types';
import { Logger } from '../utils/Logger';

export interface ServiceMessage {
  id: string;
  type: string;
  data?: any;
  timestamp: number;
}

export interface ServiceResponse {
  id: string;
  success: boolean;
  data?: any;
  error?: string;
  timestamp: number;
}

export class ServiceCommunicator extends EventEmitter {
  private serviceProcess: ChildProcess | null = null;
  private logger: Logger;
  private isConnected: boolean = false;
  private reconnectAttempts: number = 0;
  private maxReconnectAttempts: number = 5;
  private reconnectDelay: number = 2000;
  private pendingRequests: Map<string, { resolve: Function; reject: Function; timeout: NodeJS.Timeout }> = new Map();
  private requestTimeout: number = 10000; // 10 seconds
  private servicePath: string;

  constructor(logger: Logger, servicePath?: string) {
    super();
    this.logger = logger;
    this.servicePath = servicePath || this.getDefaultServicePath();
  }

  /**
   * Get the default service executable path
   */
  private getDefaultServicePath(): string {
    const platform = process.platform;
    const arch = process.arch;
    
    let executableName = 'phantomvault-service';
    if (platform === 'win32') {
      executableName += '.exe';
    }
    
    // Look for service executable in various locations
    const possiblePaths = [
      path.join(__dirname, '..', '..', '..', 'core', 'build', executableName),
      path.join(__dirname, '..', '..', '..', 'service', executableName),
      path.join(process.cwd(), 'service', executableName),
      executableName, // Assume it's in PATH
    ];
    
    for (const servicePath of possiblePaths) {
      if (fs.existsSync(servicePath)) {
        return servicePath;
      }
    }
    
    // Default to the first path if none found
    return possiblePaths[0];
  }

  /**
   * Start the service and establish connection
   */
  public async connect(): Promise<void> {
    if (this.isConnected) {
      return;
    }

    try {
      this.logger.info(`Starting PhantomVault service: ${this.servicePath}`);
      
      // Spawn the service process
      this.serviceProcess = spawn(this.servicePath, ['--ipc-mode'], {
        stdio: ['pipe', 'pipe', 'pipe'],
        detached: false,
      });

      // Set up event handlers
      this.setupProcessHandlers();
      
      // Wait for initial connection
      await this.waitForConnection();
      
      this.isConnected = true;
      this.reconnectAttempts = 0;
      
      this.logger.info('Successfully connected to PhantomVault service');
      this.emit('connected');
      
    } catch (error) {
      this.logger.error('Failed to connect to PhantomVault service', error);
      throw error;
    }
  }

  /**
   * Disconnect from the service
   */
  public async disconnect(): Promise<void> {
    if (!this.isConnected) {
      return;
    }

    this.logger.info('Disconnecting from PhantomVault service');
    
    this.isConnected = false;
    
    // Clear pending requests
    for (const [id, request] of this.pendingRequests) {
      clearTimeout(request.timeout);
      request.reject(new Error('Service disconnected'));
    }
    this.pendingRequests.clear();
    
    // Terminate service process
    if (this.serviceProcess) {
      this.serviceProcess.kill('SIGTERM');
      
      // Force kill after 5 seconds if not terminated
      setTimeout(() => {
        if (this.serviceProcess && !this.serviceProcess.killed) {
          this.serviceProcess.kill('SIGKILL');
        }
      }, 5000);
      
      this.serviceProcess = null;
    }
    
    this.emit('disconnected');
    this.logger.info('Disconnected from PhantomVault service');
  }

  /**
   * Set up process event handlers
   */
  private setupProcessHandlers(): void {
    if (!this.serviceProcess) return;

    // Handle stdout messages
    this.serviceProcess.stdout?.on('data', (data) => {
      const messages = data.toString().trim().split('\n');
      for (const messageStr of messages) {
        if (messageStr.trim()) {
          try {
            const message = JSON.parse(messageStr) as ServiceResponse;
            this.handleServiceMessage(message);
          } catch (error) {
            this.logger.warn(`Failed to parse service message: ${messageStr}`);
          }
        }
      }
    });

    // Handle stderr
    this.serviceProcess.stderr?.on('data', (data) => {
      this.logger.warn(`Service stderr: ${data.toString()}`);
    });

    // Handle process exit
    this.serviceProcess.on('exit', (code, signal) => {
      this.logger.info(`Service process exited with code ${code}, signal ${signal}`);
      this.handleServiceDisconnection();
    });

    // Handle process error
    this.serviceProcess.on('error', (error) => {
      this.logger.error('Service process error', error);
      this.handleServiceDisconnection();
    });
  }

  /**
   * Handle service message
   */
  private handleServiceMessage(message: ServiceResponse): void {
    // Handle response to pending request
    if (this.pendingRequests.has(message.id)) {
      const request = this.pendingRequests.get(message.id)!;
      clearTimeout(request.timeout);
      this.pendingRequests.delete(message.id);
      
      if (message.success) {
        request.resolve(message.data);
      } else {
        request.reject(new Error(message.error || 'Service request failed'));
      }
      return;
    }

    // Handle event messages
    if (message.type === 'vault-status-changed') {
      this.emit('vault-status-changed', message.data);
    } else if (message.type === 'activity-log-entry') {
      this.emit('activity-log-entry', message.data);
    } else if (message.type === 'service-ready') {
      this.emit('service-ready');
    }
  }

  /**
   * Handle service disconnection
   */
  private handleServiceDisconnection(): void {
    if (!this.isConnected) return;
    
    this.isConnected = false;
    this.emit('disconnected');
    
    // Clear pending requests
    for (const [id, request] of this.pendingRequests) {
      clearTimeout(request.timeout);
      request.reject(new Error('Service disconnected'));
    }
    this.pendingRequests.clear();
    
    // Attempt reconnection
    if (this.reconnectAttempts < this.maxReconnectAttempts) {
      this.reconnectAttempts++;
      this.logger.info(`Attempting to reconnect to service (attempt ${this.reconnectAttempts}/${this.maxReconnectAttempts})`);
      
      setTimeout(() => {
        this.connect().catch((error) => {
          this.logger.error(`Reconnection attempt ${this.reconnectAttempts} failed`, error);
        });
      }, this.reconnectDelay);
    } else {
      this.logger.error('Max reconnection attempts reached, giving up');
      this.emit('connection-failed');
    }
  }

  /**
   * Wait for initial connection
   */
  private waitForConnection(): Promise<void> {
    return new Promise((resolve, reject) => {
      const timeout = setTimeout(() => {
        reject(new Error('Service connection timeout'));
      }, 10000);

      const onReady = () => {
        clearTimeout(timeout);
        this.removeListener('service-ready', onReady);
        resolve();
      };

      this.once('service-ready', onReady);
    });
  }

  /**
   * Send request to service
   */
  private async sendRequest<T>(type: string, data?: any): Promise<T> {
    if (!this.isConnected || !this.serviceProcess) {
      throw new Error('Service not connected');
    }

    const id = `req_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
    const message: ServiceMessage = {
      id,
      type,
      data,
      timestamp: Date.now(),
    };

    return new Promise((resolve, reject) => {
      // Set up timeout
      const timeout = setTimeout(() => {
        this.pendingRequests.delete(id);
        reject(new Error(`Request timeout: ${type}`));
      }, this.requestTimeout);

      // Store request
      this.pendingRequests.set(id, { resolve, reject, timeout });

      // Send message
      try {
        this.serviceProcess!.stdin?.write(JSON.stringify(message) + '\n');
      } catch (error) {
        clearTimeout(timeout);
        this.pendingRequests.delete(id);
        reject(error);
      }
    });
  }

  // ==================== VAULT OPERATIONS ====================

  /**
   * List all vaults
   */
  public async listVaults(): Promise<VaultInfo[]> {
    return this.sendRequest<VaultInfo[]>('list-vaults');
  }

  /**
   * Create a new vault
   */
  public async createVault(config: VaultConfig): Promise<VaultInfo> {
    return this.sendRequest<VaultInfo>('create-vault', config);
  }

  /**
   * Update an existing vault
   */
  public async updateVault(vaultId: string, config: VaultConfig): Promise<VaultInfo> {
    return this.sendRequest<VaultInfo>('update-vault', { vaultId, config });
  }

  /**
   * Delete a vault
   */
  public async deleteVault(vaultId: string): Promise<void> {
    return this.sendRequest<void>('delete-vault', { vaultId });
  }

  /**
   * Mount a vault
   */
  public async mountVault(vaultId: string, password: string): Promise<VaultInfo> {
    return this.sendRequest<VaultInfo>('mount-vault', { vaultId, password });
  }

  /**
   * Unmount a vault
   */
  public async unmountVault(vaultId: string, force: boolean = false): Promise<void> {
    return this.sendRequest<void>('unmount-vault', { vaultId, force });
  }

  /**
   * Get vault status
   */
  public async getVaultStatus(vaultId: string): Promise<VaultInfo> {
    return this.sendRequest<VaultInfo>('get-vault-status', { vaultId });
  }

  /**
   * Validate a path
   */
  public async validatePath(path: string): Promise<boolean> {
    return this.sendRequest<boolean>('validate-path', { path });
  }

  /**
   * Validate a keyboard sequence
   */
  public async validateSequence(sequence: string, excludeVaultId?: string): Promise<boolean> {
    return this.sendRequest<boolean>('validate-sequence', { sequence, excludeVaultId });
  }

  /**
   * Get activity log
   */
  public async getActivityLog(filters?: any): Promise<ActivityLogEntry[]> {
    return this.sendRequest<ActivityLogEntry[]>('get-activity-log', filters);
  }

  /**
   * Clear activity log
   */
  public async clearActivityLog(): Promise<void> {
    return this.sendRequest<void>('clear-activity-log');
  }

  /**
   * Send ping to service
   */
  public async ping(): Promise<{ timestamp: number; pong: boolean }> {
    return this.sendRequest<{ timestamp: number; pong: boolean }>('ping');
  }

  // ==================== STATUS METHODS ====================

  /**
   * Check if service is connected
   */
  public isServiceConnected(): boolean {
    return this.isConnected;
  }

  /**
   * Get service status
   */
  public getServiceStatus(): { connected: boolean; reconnectAttempts: number } {
    return {
      connected: this.isConnected,
      reconnectAttempts: this.reconnectAttempts,
    };
  }

  /**
   * Send request to service (exposed for external use)
   */
  public async sendServiceRequest<T>(type: string, data?: any): Promise<T> {
    return this.sendRequest<T>(type, data);
  }
}