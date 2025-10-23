/**
 * IPC Communication Hook
 * 
 * Custom React hook for managing IPC communication with the Electron main process
 * Provides methods for sending requests and subscribing to events
 */

import { useState, useCallback, useEffect, useRef } from 'react';
import {
  IPCMessageType,
  IPCRequest,
  IPCResponse,
  AppError,
  UseIPCCommunication,
  DEFAULT_IPC_CONFIG,
  IPCClientConfig
} from '../types';

/**
 * IPC communication hook
 * Manages communication with the Electron main process
 */
export const useIPCCommunication = (config: Partial<IPCClientConfig> = {}): UseIPCCommunication => {
  const [isConnected, setIsConnected] = useState(true); // Assume connected initially
  const [connectionError, setConnectionError] = useState<AppError | null>(null);
  
  // Merge with default config
  const ipcConfig = { ...DEFAULT_IPC_CONFIG, ...config };
  
  // Track active requests for timeout handling
  const activeRequests = useRef<Map<string, NodeJS.Timeout>>(new Map());
  const requestCounter = useRef(0);
  
  // Track event subscriptions
  const subscriptions = useRef<Map<string, () => void>>(new Map());

  // ==================== HELPER FUNCTIONS ====================

  const generateRequestId = useCallback((): string => {
    requestCounter.current += 1;
    return `req-${Date.now()}-${requestCounter.current}`;
  }, []);

  const getTimeoutForMessageType = useCallback((type: IPCMessageType): number => {
    const timeouts = ipcConfig.timeout;
    
    // Map message types to appropriate timeouts
    if (type.includes('VAULT') || type.includes('FOLDER')) {
      return timeouts.vault_operations;
    }
    if (type.includes('FILE') || type.includes('ENCRYPT') || type.includes('DECRYPT')) {
      return timeouts.file_operations;
    }
    if (type.includes('AUTH') || type.includes('PASSWORD') || type.includes('VERIFY')) {
      return timeouts.authentication;
    }
    
    return timeouts.default;
  }, [ipcConfig.timeout]);

  const logMessage = useCallback((level: string, message: string, data?: any) => {
    if (!ipcConfig.enableLogging) return;
    
    const logLevels = ['debug', 'info', 'warn', 'error'];
    const configLevel = logLevels.indexOf(ipcConfig.logLevel);
    const messageLevel = logLevels.indexOf(level);
    
    if (messageLevel >= configLevel) {
      console[level as keyof Console](`[IPC] ${message}`, data || '');
    }
  }, [ipcConfig.enableLogging, ipcConfig.logLevel]);

  const createIPCError = useCallback((
    message: string,
    requestType: IPCMessageType,
    timeout = false
  ): AppError => {
    return {
      type: 'network',
      code: timeout ? 'IPC_TIMEOUT' : 'IPC_ERROR',
      message,
      timestamp: new Date(),
      recoverable: true,
      requestType,
      timeout,
    };
  }, []);

  // ==================== CONNECTION MANAGEMENT ====================

  const checkConnection = useCallback(async (): Promise<boolean> => {
    try {
      // Try a simple IPC call to check if the main process is responsive
      const testResponse = await window.phantomVault.getVersion();
      
      if (testResponse) {
        setIsConnected(true);
        setConnectionError(null);
        return true;
      } else {
        throw new Error('No response from main process');
      }
    } catch (error) {
      const appError = createIPCError(
        'Lost connection to main process',
        IPCMessageType.GET_VAULT_LIST
      );
      
      setIsConnected(false);
      setConnectionError(appError);
      logMessage('error', 'Connection check failed', error);
      
      return false;
    }
  }, [createIPCError, logMessage]);

  // ==================== REQUEST HANDLING ====================

  const sendRequest = useCallback(async <T = any>(
    type: IPCMessageType,
    data?: any
  ): Promise<IPCResponse<T>> => {
    const requestId = generateRequestId();
    const timeout = getTimeoutForMessageType(type);
    
    logMessage('debug', `Sending IPC request: ${type}`, { requestId, data });

    try {
      // Check connection before sending
      if (!isConnected) {
        const connected = await checkConnection();
        if (!connected) {
          throw createIPCError('Not connected to main process', type);
        }
      }

      // Create timeout promise
      const timeoutPromise = new Promise<never>((_, reject) => {
        const timer = setTimeout(() => {
          activeRequests.current.delete(requestId);
          reject(createIPCError(`Request timeout after ${timeout}ms`, type, true));
        }, timeout);
        
        activeRequests.current.set(requestId, timer);
      });

      // Create the actual request promise
      const requestPromise = (async (): Promise<IPCResponse<T>> => {
        try {
          let result: any;

          // Route to appropriate PhantomVault API method based on message type
          switch (type) {
            case IPCMessageType.GET_VAULT_LIST:
              if (!data?.profileId) {
                throw new Error('Profile ID required for vault list');
              }
              result = await window.phantomVault.folder.getAll(data.profileId);
              break;

            case IPCMessageType.CREATE_VAULT:
              if (!data?.config || !data?.profileId) {
                throw new Error('Config and profile ID required for vault creation');
              }
              result = await window.phantomVault.folder.add(
                data.profileId,
                data.config.path,
                data.config.name
              );
              break;

            case IPCMessageType.MOUNT_VAULT:
              if (!data?.vaultId || !data?.profileId) {
                throw new Error('Vault ID and profile ID required for mounting');
              }
              result = await window.phantomVault.folder.unlock(
                data.profileId,
                data.vaultId,
                data.password || '',
                data.mode || 'temporary'
              );
              break;

            case IPCMessageType.UNMOUNT_VAULT:
              if (!data?.vaultId || !data?.profileId) {
                throw new Error('Vault ID and profile ID required for unmounting');
              }
              result = await window.phantomVault.folder.lock(
                data.profileId,
                data.vaultId
              );
              break;

            case IPCMessageType.DELETE_VAULT:
              if (!data?.vaultId || !data?.profileId) {
                throw new Error('Vault ID and profile ID required for deletion');
              }
              result = await window.phantomVault.folder.remove(
                data.profileId,
                data.vaultId
              );
              break;

            case IPCMessageType.GET_PROFILES:
              result = await window.phantomVault.profile.getAll();
              break;

            case IPCMessageType.CREATE_PROFILE:
              if (!data?.name || !data?.masterPassword || !data?.recoveryKey) {
                throw new Error('Name, password, and recovery key required for profile creation');
              }
              result = await window.phantomVault.profile.create(
                data.name,
                data.masterPassword,
                data.recoveryKey
              );
              break;

            case IPCMessageType.SET_ACTIVE_PROFILE:
              if (!data?.profileId) {
                throw new Error('Profile ID required for setting active profile');
              }
              result = await window.phantomVault.profile.setActive(data.profileId);
              break;

            case IPCMessageType.VERIFY_PASSWORD:
              if (!data?.profileId || !data?.password) {
                throw new Error('Profile ID and password required for verification');
              }
              result = await window.phantomVault.profile.verifyPassword(
                data.profileId,
                data.password
              );
              break;

            default:
              throw new Error(`Unsupported IPC message type: ${type}`);
          }

          // Clear timeout
          const timer = activeRequests.current.get(requestId);
          if (timer) {
            clearTimeout(timer);
            activeRequests.current.delete(requestId);
          }

          logMessage('debug', `IPC request completed: ${type}`, { requestId, result });

          return {
            success: result.success || true,
            data: result,
            requestId,
            timestamp: Date.now(),
          };
        } catch (error) {
          // Clear timeout
          const timer = activeRequests.current.get(requestId);
          if (timer) {
            clearTimeout(timer);
            activeRequests.current.delete(requestId);
          }

          throw error;
        }
      })();

      // Race between request and timeout
      return await Promise.race([requestPromise, timeoutPromise]);
    } catch (error) {
      logMessage('error', `IPC request failed: ${type}`, { requestId, error });
      
      if (error instanceof Error && error.message.includes('timeout')) {
        throw error; // Re-throw timeout errors as-is
      }
      
      throw createIPCError(
        error instanceof Error ? error.message : 'Unknown IPC error',
        type
      );
    }
  }, [
    generateRequestId,
    getTimeoutForMessageType,
    logMessage,
    isConnected,
    checkConnection,
    createIPCError
  ]);

  // ==================== EVENT SUBSCRIPTION ====================

  const subscribe = useCallback((
    type: IPCMessageType,
    callback: (data: any) => void
  ): (() => void) => {
    const subscriptionId = `${type}-${Date.now()}`;
    
    logMessage('debug', `Subscribing to IPC event: ${type}`, { subscriptionId });

    // Set up event listener based on type
    let unsubscribe: (() => void) | null = null;

    switch (type) {
      case IPCMessageType.VAULT_STATUS_CHANGED:
        // This would be implemented when the main process supports it
        unsubscribe = () => {
          logMessage('debug', 'Unsubscribed from vault status changes');
        };
        break;

      case IPCMessageType.ACTIVITY_LOG_ENTRY:
        // This would be implemented when the main process supports it
        unsubscribe = () => {
          logMessage('debug', 'Unsubscribed from activity log entries');
        };
        break;

      case IPCMessageType.ERROR_NOTIFICATION:
        // This would be implemented when the main process supports it
        unsubscribe = () => {
          logMessage('debug', 'Unsubscribed from error notifications');
        };
        break;

      default:
        logMessage('warn', `Unsupported subscription type: ${type}`);
        unsubscribe = () => {};
    }

    // Store subscription for cleanup
    if (unsubscribe) {
      subscriptions.current.set(subscriptionId, unsubscribe);
    }

    // Return unsubscribe function
    return () => {
      const storedUnsubscribe = subscriptions.current.get(subscriptionId);
      if (storedUnsubscribe) {
        storedUnsubscribe();
        subscriptions.current.delete(subscriptionId);
      }
    };
  }, [logMessage]);

  // ==================== EFFECTS ====================

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      // Clear all active request timeouts
      activeRequests.current.forEach(timer => clearTimeout(timer));
      activeRequests.current.clear();
      
      // Unsubscribe from all events
      subscriptions.current.forEach(unsubscribe => unsubscribe());
      subscriptions.current.clear();
    };
  }, []);

  // Periodic connection check
  useEffect(() => {
    const interval = setInterval(checkConnection, 30000); // Check every 30 seconds
    
    return () => clearInterval(interval);
  }, [checkConnection]);

  // ==================== RETURN HOOK INTERFACE ====================

  return {
    sendRequest,
    subscribe,
    isConnected,
    connectionError,
  };
};