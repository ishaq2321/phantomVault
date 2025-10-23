/**
 * Vault Status Monitor Service
 * 
 * Manages real-time vault status monitoring and updates
 * Provides automatic refresh, status change detection, and event notifications
 */

import {
  VaultInfo,
  VaultStatus,
  IPCMessageType,
  LogEntry,
  LogLevel,
  AppError
} from '../types';

export interface VaultStatusChange {
  vaultId: string;
  oldStatus: VaultStatus;
  newStatus: VaultStatus;
  timestamp: Date;
  vault: VaultInfo;
}

export interface StatusMonitorConfig {
  refreshInterval: number; // milliseconds
  enableAutoRefresh: boolean;
  enableStatusChangeEvents: boolean;
  enableStatusAnimations: boolean;
  maxRetries: number;
  retryDelay: number; // milliseconds
}

export interface StatusMonitorCallbacks {
  onStatusChange?: (change: VaultStatusChange) => void;
  onVaultsUpdated?: (vaults: VaultInfo[]) => void;
  onError?: (error: AppError) => void;
  onConnectionChange?: (connected: boolean) => void;
}

/**
 * Vault status monitoring service
 */
export class VaultStatusMonitor {
  private config: StatusMonitorConfig;
  private callbacks: StatusMonitorCallbacks;
  private isMonitoring: boolean = false;
  private refreshTimer: NodeJS.Timeout | null = null;
  private lastKnownVaults: Map<string, VaultInfo> = new Map();
  private retryCount: number = 0;
  private isConnected: boolean = true;

  constructor(
    config: Partial<StatusMonitorConfig> = {},
    callbacks: StatusMonitorCallbacks = {}
  ) {
    this.config = {
      refreshInterval: 5000, // 5 seconds
      enableAutoRefresh: true,
      enableStatusChangeEvents: true,
      enableStatusAnimations: true,
      maxRetries: 3,
      retryDelay: 2000, // 2 seconds
      ...config,
    };
    
    this.callbacks = callbacks;
  }

  // ==================== PUBLIC METHODS ====================

  /**
   * Start monitoring vault status
   */
  public start(): void {
    if (this.isMonitoring) {
      console.warn('VaultStatusMonitor is already running');
      return;
    }

    console.log('Starting vault status monitoring...');
    this.isMonitoring = true;
    this.retryCount = 0;

    // Initial load
    this.refreshVaultStatus();

    // Set up auto-refresh if enabled
    if (this.config.enableAutoRefresh) {
      this.startAutoRefresh();
    }

    // Set up IPC event listeners if available
    if (this.config.enableStatusChangeEvents) {
      this.setupEventListeners();
    }
  }

  /**
   * Stop monitoring vault status
   */
  public stop(): void {
    if (!this.isMonitoring) {
      return;
    }

    console.log('Stopping vault status monitoring...');
    this.isMonitoring = false;

    // Clear refresh timer
    if (this.refreshTimer) {
      clearInterval(this.refreshTimer);
      this.refreshTimer = null;
    }

    // Clean up event listeners
    this.cleanupEventListeners();
  }

  /**
   * Manually refresh vault status
   */
  public async refresh(): Promise<VaultInfo[]> {
    return this.refreshVaultStatus();
  }

  /**
   * Update monitoring configuration
   */
  public updateConfig(newConfig: Partial<StatusMonitorConfig>): void {
    const wasMonitoring = this.isMonitoring;
    
    if (wasMonitoring) {
      this.stop();
    }

    this.config = { ...this.config, ...newConfig };

    if (wasMonitoring) {
      this.start();
    }
  }

  /**
   * Update callbacks
   */
  public updateCallbacks(newCallbacks: Partial<StatusMonitorCallbacks>): void {
    this.callbacks = { ...this.callbacks, ...newCallbacks };
  }

  /**
   * Get current monitoring status
   */
  public getStatus(): {
    isMonitoring: boolean;
    isConnected: boolean;
    lastUpdate: Date | null;
    vaultCount: number;
    retryCount: number;
  } {
    return {
      isMonitoring: this.isMonitoring,
      isConnected: this.isConnected,
      lastUpdate: this.lastKnownVaults.size > 0 ? new Date() : null,
      vaultCount: this.lastKnownVaults.size,
      retryCount: this.retryCount,
    };
  }

  // ==================== PRIVATE METHODS ====================

  /**
   * Start auto-refresh timer
   */
  private startAutoRefresh(): void {
    if (this.refreshTimer) {
      clearInterval(this.refreshTimer);
    }

    this.refreshTimer = setInterval(() => {
      if (this.isMonitoring) {
        this.refreshVaultStatus();
      }
    }, this.config.refreshInterval);
  }

  /**
   * Refresh vault status from the backend
   */
  private async refreshVaultStatus(): Promise<VaultInfo[]> {
    try {
      // Get active profile first
      const profileResponse = await window.phantomVault.profile.getActive();
      
      if (!profileResponse.success || !profileResponse.profile) {
        throw new Error('No active profile found');
      }

      // Get all folders for the active profile
      const foldersResponse = await window.phantomVault.folder.getAll(
        profileResponse.profile.id
      );

      if (!foldersResponse.success) {
        throw new Error(foldersResponse.error || 'Failed to fetch vault status');
      }

      // Transform folder data to VaultInfo format
      const vaults: VaultInfo[] = (foldersResponse.folders || []).map(folder => ({
        id: folder.id,
        name: folder.folder_name || folder.name || 'Unnamed Vault',
        path: folder.original_path || '',
        status: this.mapFolderStatusToVaultStatus(folder),
        lastAccess: new Date(), // TODO: Get actual last access time
        size: 0, // TODO: Calculate folder size
        folderCount: 1,
        profile: {
          id: profileResponse.profile.id,
          name: profileResponse.profile.name,
          createdAt: new Date(profileResponse.profile.created_at * 1000),
          lastModified: new Date(profileResponse.profile.created_at * 1000),
          isActive: true,
        },
      }));

      // Check for status changes
      this.detectStatusChanges(vaults);

      // Update last known vaults
      this.updateLastKnownVaults(vaults);

      // Reset retry count on success
      this.retryCount = 0;
      this.setConnectionStatus(true);

      // Notify callbacks
      if (this.callbacks.onVaultsUpdated) {
        this.callbacks.onVaultsUpdated(vaults);
      }

      return vaults;
    } catch (error) {
      console.error('Failed to refresh vault status:', error);
      
      this.retryCount++;
      this.setConnectionStatus(false);

      const appError: AppError = {
        type: 'network',
        code: 'VAULT_STATUS_REFRESH_FAILED',
        message: error instanceof Error ? error.message : 'Failed to refresh vault status',
        timestamp: new Date(),
        recoverable: true,
      };

      if (this.callbacks.onError) {
        this.callbacks.onError(appError);
      }

      // Retry if under max retries
      if (this.retryCount <= this.config.maxRetries) {
        console.log(`Retrying vault status refresh in ${this.config.retryDelay}ms (attempt ${this.retryCount}/${this.config.maxRetries})`);
        
        setTimeout(() => {
          if (this.isMonitoring) {
            this.refreshVaultStatus();
          }
        }, this.config.retryDelay);
      }

      return [];
    }
  }

  /**
   * Map folder status to vault status
   */
  private mapFolderStatusToVaultStatus(folder: any): VaultStatus {
    if (folder.is_locked === undefined) {
      return 'error';
    }
    
    return folder.is_locked ? 'unmounted' : 'mounted';
  }

  /**
   * Detect status changes between current and previous vault states
   */
  private detectStatusChanges(currentVaults: VaultInfo[]): void {
    if (!this.config.enableStatusChangeEvents) {
      return;
    }

    for (const vault of currentVaults) {
      const previousVault = this.lastKnownVaults.get(vault.id);
      
      if (previousVault && previousVault.status !== vault.status) {
        const statusChange: VaultStatusChange = {
          vaultId: vault.id,
          oldStatus: previousVault.status,
          newStatus: vault.status,
          timestamp: new Date(),
          vault,
        };

        console.log(`Vault status changed: ${vault.name} (${previousVault.status} → ${vault.status})`);

        // Create log entry for status change
        this.logStatusChange(statusChange);

        // Notify callback
        if (this.callbacks.onStatusChange) {
          this.callbacks.onStatusChange(statusChange);
        }
      }
    }

    // Check for removed vaults
    for (const [vaultId, previousVault] of this.lastKnownVaults) {
      const currentVault = currentVaults.find(v => v.id === vaultId);
      
      if (!currentVault) {
        console.log(`Vault removed: ${previousVault.name}`);
        
        // Could emit a vault removed event here if needed
      }
    }
  }

  /**
   * Update the last known vaults map
   */
  private updateLastKnownVaults(vaults: VaultInfo[]): void {
    this.lastKnownVaults.clear();
    
    for (const vault of vaults) {
      this.lastKnownVaults.set(vault.id, { ...vault });
    }
  }

  /**
   * Set connection status and notify callback
   */
  private setConnectionStatus(connected: boolean): void {
    if (this.isConnected !== connected) {
      this.isConnected = connected;
      
      if (this.callbacks.onConnectionChange) {
        this.callbacks.onConnectionChange(connected);
      }
    }
  }

  /**
   * Log status change event
   */
  private logStatusChange(change: VaultStatusChange): void {
    const logEntry: LogEntry = {
      id: `status-change-${Date.now()}-${Math.random()}`,
      timestamp: change.timestamp,
      level: 'info' as LogLevel,
      source: 'VaultStatusMonitor',
      message: `Vault "${change.vault.name}" status changed from ${change.oldStatus} to ${change.newStatus}`,
      vaultId: change.vaultId,
      details: {
        oldStatus: change.oldStatus,
        newStatus: change.newStatus,
        vaultPath: change.vault.path,
      },
    };

    // In a real implementation, this would send the log entry to the activity monitor
    console.log('Status change logged:', logEntry);
  }

  /**
   * Set up IPC event listeners for real-time updates
   */
  private setupEventListeners(): void {
    // In a real implementation, this would set up IPC event listeners
    // for vault status changes from the main process
    
    // Example:
    // window.phantomVault.onVaultStatusChanged?.((data) => {
    //   this.handleVaultStatusEvent(data);
    // });
    
    console.log('IPC event listeners set up (placeholder)');
  }

  /**
   * Clean up IPC event listeners
   */
  private cleanupEventListeners(): void {
    // In a real implementation, this would clean up IPC event listeners
    console.log('IPC event listeners cleaned up (placeholder)');
  }

  /**
   * Handle vault status event from IPC
   */
  private handleVaultStatusEvent(data: any): void {
    console.log('Received vault status event:', data);
    
    // Trigger immediate refresh to get updated status
    if (this.isMonitoring) {
      this.refreshVaultStatus();
    }
  }
}

// ==================== FACTORY FUNCTION ====================

/**
 * Create a new vault status monitor instance
 */
export function createVaultStatusMonitor(
  config?: Partial<StatusMonitorConfig>,
  callbacks?: StatusMonitorCallbacks
): VaultStatusMonitor {
  return new VaultStatusMonitor(config, callbacks);
}

// ==================== SINGLETON INSTANCE ====================

let globalMonitorInstance: VaultStatusMonitor | null = null;

/**
 * Get the global vault status monitor instance
 */
export function getGlobalVaultStatusMonitor(): VaultStatusMonitor {
  if (!globalMonitorInstance) {
    globalMonitorInstance = new VaultStatusMonitor();
  }
  
  return globalMonitorInstance;
}

/**
 * Set the global vault status monitor instance
 */
export function setGlobalVaultStatusMonitor(monitor: VaultStatusMonitor): void {
  if (globalMonitorInstance) {
    globalMonitorInstance.stop();
  }
  
  globalMonitorInstance = monitor;
}