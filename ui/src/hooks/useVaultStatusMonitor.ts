/**
 * Vault Status Monitor Hook
 * 
 * React hook for integrating vault status monitoring into components
 * Provides real-time status updates and automatic refresh functionality
 */

import { useState, useEffect, useCallback, useRef } from 'react';
import {
  VaultInfo,
  VaultStatus,
  AppError
} from '../types';
import {
  VaultStatusMonitor,
  VaultStatusChange,
  StatusMonitorConfig,
  createVaultStatusMonitor
} from '../services/VaultStatusMonitor';
import { useApp } from '../contexts';

export interface UseVaultStatusMonitorOptions {
  config?: Partial<StatusMonitorConfig>;
  autoStart?: boolean;
  onStatusChange?: (change: VaultStatusChange) => void;
  onError?: (error: AppError) => void;
}

export interface UseVaultStatusMonitorReturn {
  // Status data
  vaults: VaultInfo[];
  isMonitoring: boolean;
  isConnected: boolean;
  lastUpdate: Date | null;
  error: AppError | null;
  
  // Control methods
  start: () => void;
  stop: () => void;
  refresh: () => Promise<VaultInfo[]>;
  
  // Configuration
  updateConfig: (config: Partial<StatusMonitorConfig>) => void;
  
  // Status information
  getMonitorStatus: () => {
    isMonitoring: boolean;
    isConnected: boolean;
    lastUpdate: Date | null;
    vaultCount: number;
    retryCount: number;
  };
}

/**
 * Hook for vault status monitoring
 */
export const useVaultStatusMonitor = (
  options: UseVaultStatusMonitorOptions = {}
): UseVaultStatusMonitorReturn => {
  const {
    config = {},
    autoStart = true,
    onStatusChange,
    onError
  } = options;

  const { actions: appActions } = useApp();
  
  // State
  const [vaults, setVaults] = useState<VaultInfo[]>([]);
  const [isMonitoring, setIsMonitoring] = useState(false);
  const [isConnected, setIsConnected] = useState(true);
  const [lastUpdate, setLastUpdate] = useState<Date | null>(null);
  const [error, setError] = useState<AppError | null>(null);
  
  // Monitor instance ref
  const monitorRef = useRef<VaultStatusMonitor | null>(null);

  // ==================== MONITOR SETUP ====================

  const createMonitor = useCallback(() => {
    if (monitorRef.current) {
      monitorRef.current.stop();
    }

    const monitor = createVaultStatusMonitor(config, {
      onVaultsUpdated: (updatedVaults) => {
        setVaults(updatedVaults);
        setLastUpdate(new Date());
        setError(null);
      },
      
      onStatusChange: (change) => {
        console.log('Vault status changed:', change);
        
        // Show notification for status changes
        const statusMessages = {
          mounted: 'Vault mounted successfully',
          unmounted: 'Vault unmounted',
          error: 'Vault encountered an error',
          loading: 'Vault operation in progress',
          encrypting: 'Vault is being encrypted',
          decrypting: 'Vault is being decrypted',
        };

        const message = statusMessages[change.newStatus] || `Status changed to ${change.newStatus}`;
        
        appActions.addNotification({
          type: change.newStatus === 'error' ? 'error' : 'info',
          title: `${change.vault.name}`,
          message,
          duration: 3000,
        });

        // Call external callback if provided
        if (onStatusChange) {
          onStatusChange(change);
        }
      },
      
      onError: (monitorError) => {
        console.error('Vault status monitor error:', monitorError);
        setError(monitorError);
        
        // Show error notification
        appActions.addNotification({
          type: 'error',
          title: 'Monitoring Error',
          message: monitorError.message,
          duration: 5000,
        });

        // Call external callback if provided
        if (onError) {
          onError(monitorError);
        }
      },
      
      onConnectionChange: (connected) => {
        setIsConnected(connected);
        
        // Show connection status notification
        appActions.addNotification({
          type: connected ? 'success' : 'warning',
          title: connected ? 'Connection Restored' : 'Connection Lost',
          message: connected 
            ? 'Vault monitoring reconnected'
            : 'Lost connection to vault service',
          duration: connected ? 3000 : 0, // Persistent for disconnection
        });
      },
    });

    monitorRef.current = monitor;
    return monitor;
  }, [config, onStatusChange, onError, appActions]);

  // ==================== CONTROL METHODS ====================

  const start = useCallback(() => {
    if (!monitorRef.current) {
      createMonitor();
    }
    
    if (monitorRef.current) {
      monitorRef.current.start();
      setIsMonitoring(true);
      setError(null);
    }
  }, [createMonitor]);

  const stop = useCallback(() => {
    if (monitorRef.current) {
      monitorRef.current.stop();
      setIsMonitoring(false);
    }
  }, []);

  const refresh = useCallback(async (): Promise<VaultInfo[]> => {
    if (!monitorRef.current) {
      return [];
    }

    try {
      setError(null);
      const updatedVaults = await monitorRef.current.refresh();
      return updatedVaults;
    } catch (error) {
      const appError: AppError = {
        type: 'system',
        code: 'MANUAL_REFRESH_FAILED',
        message: error instanceof Error ? error.message : 'Failed to refresh vault status',
        timestamp: new Date(),
        recoverable: true,
      };
      
      setError(appError);
      throw appError;
    }
  }, []);

  const updateConfig = useCallback((newConfig: Partial<StatusMonitorConfig>) => {
    if (monitorRef.current) {
      monitorRef.current.updateConfig(newConfig);
    }
  }, []);

  const getMonitorStatus = useCallback(() => {
    if (monitorRef.current) {
      return monitorRef.current.getStatus();
    }
    
    return {
      isMonitoring: false,
      isConnected: false,
      lastUpdate: null,
      vaultCount: 0,
      retryCount: 0,
    };
  }, []);

  // ==================== EFFECTS ====================

  // Initialize monitor on mount
  useEffect(() => {
    createMonitor();
    
    return () => {
      if (monitorRef.current) {
        monitorRef.current.stop();
      }
    };
  }, []); // Only run on mount/unmount

  // Auto-start if enabled
  useEffect(() => {
    if (autoStart && monitorRef.current && !isMonitoring) {
      start();
    }
  }, [autoStart, start, isMonitoring]);

  // Update monitor callbacks when they change
  useEffect(() => {
    if (monitorRef.current) {
      monitorRef.current.updateCallbacks({
        onStatusChange: (change) => {
          // Update local state
          setVaults(prevVaults => 
            prevVaults.map(vault => 
              vault.id === change.vaultId 
                ? { ...vault, status: change.newStatus }
                : vault
            )
          );
          
          // Call external callback
          if (onStatusChange) {
            onStatusChange(change);
          }
        },
        onError,
      });
    }
  }, [onStatusChange, onError]);

  // ==================== RETURN HOOK INTERFACE ====================

  return {
    // Status data
    vaults,
    isMonitoring,
    isConnected,
    lastUpdate,
    error,
    
    // Control methods
    start,
    stop,
    refresh,
    
    // Configuration
    updateConfig,
    
    // Status information
    getMonitorStatus,
  };
};

// ==================== SPECIALIZED HOOKS ====================

/**
 * Hook for monitoring a specific vault
 */
export const useVaultMonitor = (vaultId: string) => {
  const monitor = useVaultStatusMonitor();
  
  const vault = monitor.vaults.find(v => v.id === vaultId);
  
  return {
    vault,
    status: vault?.status,
    isMonitoring: monitor.isMonitoring,
    refresh: monitor.refresh,
  };
};

/**
 * Hook for monitoring vault status changes with animations
 */
export const useVaultStatusAnimations = () => {
  const [statusChanges, setStatusChanges] = useState<Map<string, VaultStatusChange>>(new Map());
  
  const monitor = useVaultStatusMonitor({
    config: { enableStatusAnimations: true },
    onStatusChange: (change) => {
      setStatusChanges(prev => {
        const newMap = new Map(prev);
        newMap.set(change.vaultId, change);
        
        // Clear the animation after 3 seconds
        setTimeout(() => {
          setStatusChanges(current => {
            const updatedMap = new Map(current);
            updatedMap.delete(change.vaultId);
            return updatedMap;
          });
        }, 3000);
        
        return newMap;
      });
    },
  });
  
  const getVaultAnimation = useCallback((vaultId: string) => {
    return statusChanges.get(vaultId);
  }, [statusChanges]);
  
  return {
    ...monitor,
    getVaultAnimation,
    hasActiveAnimations: statusChanges.size > 0,
  };
};