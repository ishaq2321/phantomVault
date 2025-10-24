"use strict";
/**
 * Preload Script
 *
 * Exposes secure IPC API to the renderer process
 */
Object.defineProperty(exports, "__esModule", { value: true });
const electron_1 = require("electron");
// ==================== IPC API IMPLEMENTATION ====================
const electronAPI = {
    // ==================== GENERAL IPC METHODS ====================
    invoke: (channel, ...args) => {
        return electron_1.ipcRenderer.invoke(channel, ...args);
    },
    on: (channel, callback) => {
        electron_1.ipcRenderer.on(channel, (_event, ...args) => callback(...args));
    },
    removeAllListeners: (channel) => {
        electron_1.ipcRenderer.removeAllListeners(channel);
    },
    // ==================== VAULT OPERATIONS ====================
    createVault: async (config) => {
        return electron_1.ipcRenderer.invoke('vault:create', config);
    },
    mountVault: async (id, password) => {
        return electron_1.ipcRenderer.invoke('vault:mount', id, password);
    },
    unmountVault: async (id, force) => {
        return electron_1.ipcRenderer.invoke('vault:unmount', id, force);
    },
    deleteVault: async (id) => {
        return electron_1.ipcRenderer.invoke('vault:delete', id);
    },
    updateVault: async (id, config) => {
        return electron_1.ipcRenderer.invoke('vault:update', id, config);
    },
    listVaults: async () => {
        return electron_1.ipcRenderer.invoke('vault:list');
    },
    // ==================== FILE SYSTEM OPERATIONS ====================
    selectFolder: async () => {
        return electron_1.ipcRenderer.invoke('fs:select-folder');
    },
    checkPath: async (path) => {
        return electron_1.ipcRenderer.invoke('fs:check-path', path);
    },
    // ==================== SETTINGS ====================
    getSettings: async () => {
        return electron_1.ipcRenderer.invoke('settings:get');
    },
    updateSettings: async (settings) => {
        return electron_1.ipcRenderer.invoke('settings:update', settings);
    },
    // ==================== ACTIVITY MONITORING ====================
    getActivityLog: async (filters) => {
        return electron_1.ipcRenderer.invoke('activity:get-log', filters);
    },
    subscribeToActivity: (callback) => {
        // Subscribe to activity updates
        electron_1.ipcRenderer.invoke('activity:subscribe');
        // Set up listener for new entries
        const listener = (_event, entry) => {
            callback(entry);
        };
        electron_1.ipcRenderer.on('activity:new-entry', listener);
        // Return unsubscribe function
        return () => {
            electron_1.ipcRenderer.removeListener('activity:new-entry', listener);
            electron_1.ipcRenderer.invoke('activity:unsubscribe');
        };
    },
    clearActivityLog: async () => {
        return electron_1.ipcRenderer.invoke('activity:clear-log');
    },
    // ==================== SYSTEM INTEGRATION ====================
    showNotification: (notification) => {
        // Send notification to main process for system notifications
        electron_1.ipcRenderer.send('app:show-notification', notification);
    },
    setTrayMenu: (items) => {
        electron_1.ipcRenderer.send('app:set-tray-menu', items);
    },
};
// ==================== CONTEXT BRIDGE ====================
// Expose the API to the renderer process
electron_1.contextBridge.exposeInMainWorld('electronAPI', electronAPI);
// ==================== ADDITIONAL EVENT LISTENERS ====================
// Listen for service status changes
electron_1.ipcRenderer.on('service:connected', (_event, data) => {
    console.log('Service connected:', data);
    // Dispatch custom event for React components
    window.dispatchEvent(new CustomEvent('service-connected', { detail: data }));
});
electron_1.ipcRenderer.on('service:disconnected', (_event, data) => {
    console.log('Service disconnected:', data);
    window.dispatchEvent(new CustomEvent('service-disconnected', { detail: data }));
});
electron_1.ipcRenderer.on('service:connection-failed', (_event, data) => {
    console.error('Service connection failed:', data);
    window.dispatchEvent(new CustomEvent('service-connection-failed', { detail: data }));
});
electron_1.ipcRenderer.on('service:restarting', (_event, data) => {
    console.log('Service restarting:', data);
    window.dispatchEvent(new CustomEvent('service-restarting', { detail: data }));
});
// Listen for vault status changes
electron_1.ipcRenderer.on('vault:status-changed', (_event, data) => {
    console.log('Vault status changed:', data);
    window.dispatchEvent(new CustomEvent('vault-status-changed', { detail: data }));
});
// Listen for app notifications
electron_1.ipcRenderer.on('app:notification', (_event, notification) => {
    console.log('App notification:', notification);
    window.dispatchEvent(new CustomEvent('app-notification', { detail: notification }));
});
// ==================== DEVELOPMENT HELPERS ====================
if (process.env.NODE_ENV === 'development') {
    // Expose additional debugging tools in development
    electron_1.contextBridge.exposeInMainWorld('electronDev', {
        getServiceStatus: () => electron_1.ipcRenderer.invoke('dev:get-service-status'),
        restartService: () => electron_1.ipcRenderer.invoke('dev:restart-service'),
        getDiagnostics: () => electron_1.ipcRenderer.invoke('dev:get-diagnostics'),
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
electron_1.ipcRenderer.send('preload:ready');
