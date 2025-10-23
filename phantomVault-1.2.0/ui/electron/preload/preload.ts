/**
 * Preload Script
 * 
 * Exposes secure IPC API to the renderer process
 */

import { contextBridge, ipcRenderer } from 'electron';
import { 
  VaultConfig, 
  VaultInfo, 
  AppSettings, 
  ActivityLogEntry, 
  ActivityFilter,
  ApiResponse,
  ElectronAPI 
} from '../../src/types';

// ==================== IPC API IMPLEMENTATION ====================

const electronAPI: ElectronAPI = {
  // ==================== GENERAL IPC METHODS ====================
  
  invoke: (channel: string, ...args: any[]) => {
    return ipcRenderer.invoke(channel, ...args);
  },

  on: (channel: string, callback: (...args: any[]) => void) => {
    ipcRenderer.on(channel, (_event, ...args) => callback(...args));
  },

  removeAllListeners: (channel: string) => {
    ipcRenderer.removeAllListeners(channel);
  },

  // ==================== VAULT OPERATIONS ====================

  createVault: async (config: VaultConfig): Promise<ApiResponse<VaultInfo>> => {
    return ipcRenderer.invoke('vault:create', config);
  },

  mountVault: async (id: string, password: string): Promise<ApiResponse<VaultInfo>> => {
    return ipcRenderer.invoke('vault:mount', id, password);
  },

  unmountVault: async (id: string, force?: boolean): Promise<ApiResponse> => {
    return ipcRenderer.invoke('vault:unmount', id, force);
  },

  deleteVault: async (id: string): Promise<ApiResponse> => {
    return ipcRenderer.invoke('vault:delete', id);
  },

  updateVault: async (id: string, config: VaultConfig): Promise<ApiResponse<VaultInfo>> => {
    return ipcRenderer.invoke('vault:update', id, config);
  },

  listVaults: async (): Promise<ApiResponse<VaultInfo[]>> => {
    return ipcRenderer.invoke('vault:list');
  },

  // ==================== FILE SYSTEM OPERATIONS ====================

  selectFolder: async (): Promise<ApiResponse<string>> => {
    return ipcRenderer.invoke('fs:select-folder');
  },

  checkPath: async (path: string): Promise<ApiResponse<boolean>> => {
    return ipcRenderer.invoke('fs:check-path', path);
  },

  // ==================== SETTINGS ====================

  getSettings: async (): Promise<ApiResponse<AppSettings>> => {
    return ipcRenderer.invoke('settings:get');
  },

  updateSettings: async (settings: Partial<AppSettings>): Promise<ApiResponse> => {
    return ipcRenderer.invoke('settings:update', settings);
  },

  // ==================== ACTIVITY MONITORING ====================

  getActivityLog: async (filters?: Partial<ActivityFilter>): Promise<ApiResponse<ActivityLogEntry[]>> => {
    return ipcRenderer.invoke('activity:get-log', filters);
  },

  subscribeToActivity: (callback: (entry: ActivityLogEntry) => void): (() => void) => {
    // Subscribe to activity updates
    ipcRenderer.invoke('activity:subscribe');
    
    // Set up listener for new entries
    const listener = (_event: any, entry: ActivityLogEntry) => {
      callback(entry);
    };
    
    ipcRenderer.on('activity:new-entry', listener);
    
    // Return unsubscribe function
    return () => {
      ipcRenderer.removeListener('activity:new-entry', listener);
      ipcRenderer.invoke('activity:unsubscribe');
    };
  },

  clearActivityLog: async (): Promise<ApiResponse> => {
    return ipcRenderer.invoke('activity:clear-log');
  },

  // ==================== SYSTEM INTEGRATION ====================

  showNotification: (notification: {
    type: 'success' | 'error' | 'warning' | 'info';
    title: string;
    message: string;
    duration?: number;
  }) => {
    // Send notification to main process for system notifications
    ipcRenderer.send('app:show-notification', notification);
  },

  setTrayMenu: (items: any[]) => {
    ipcRenderer.send('app:set-tray-menu', items);
  },
};

// ==================== CONTEXT BRIDGE ====================

// Expose the API to the renderer process
contextBridge.exposeInMainWorld('electronAPI', electronAPI);

// ==================== ADDITIONAL EVENT LISTENERS ====================

// Listen for service status changes
ipcRenderer.on('service:connected', (_event, data) => {
  console.log('Service connected:', data);
  // Dispatch custom event for React components
  window.dispatchEvent(new CustomEvent('service-connected', { detail: data }));
});

ipcRenderer.on('service:disconnected', (_event, data) => {
  console.log('Service disconnected:', data);
  window.dispatchEvent(new CustomEvent('service-disconnected', { detail: data }));
});

ipcRenderer.on('service:connection-failed', (_event, data) => {
  console.error('Service connection failed:', data);
  window.dispatchEvent(new CustomEvent('service-connection-failed', { detail: data }));
});

ipcRenderer.on('service:restarting', (_event, data) => {
  console.log('Service restarting:', data);
  window.dispatchEvent(new CustomEvent('service-restarting', { detail: data }));
});

// Listen for vault status changes
ipcRenderer.on('vault:status-changed', (_event, data) => {
  console.log('Vault status changed:', data);
  window.dispatchEvent(new CustomEvent('vault-status-changed', { detail: data }));
});

// Listen for app notifications
ipcRenderer.on('app:notification', (_event, notification) => {
  console.log('App notification:', notification);
  window.dispatchEvent(new CustomEvent('app-notification', { detail: notification }));
});

// ==================== DEVELOPMENT HELPERS ====================

if (process.env.NODE_ENV === 'development') {
  // Expose additional debugging tools in development
  contextBridge.exposeInMainWorld('electronDev', {
    getServiceStatus: () => ipcRenderer.invoke('dev:get-service-status'),
    restartService: () => ipcRenderer.invoke('dev:restart-service'),
    getDiagnostics: () => ipcRenderer.invoke('dev:get-diagnostics'),
  });
}

// ==================== ERROR HANDLING ====================

// Handle uncaught errors in preload
process.on('uncaughtException', (error) => {
  console.error('Preload uncaught exception:', error);
});

process.on('unhandledRejection', (reason, promise) => {
  console.error('Preload unhandled rejection at:', promise, 'reason:', reason);
});

// ==================== INITIALIZATION ====================

console.log('PhantomVault preload script loaded');

// Notify main process that preload is ready
ipcRenderer.send('preload:ready');

// Export types for TypeScript (this won't be executed, just for type checking)
export type { ElectronAPI };