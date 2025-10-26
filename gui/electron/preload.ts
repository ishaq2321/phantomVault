/**
 * PhantomVault - Electron Preload Script
 * 
 * Secure bridge between the main process and renderer process.
 * Exposes safe APIs to the React application.
 */

import { contextBridge, ipcRenderer } from 'electron';

// Define the API interface
export interface PhantomVaultAPI {
  // App information
  app: {
    getVersion(): Promise<string>;
    isAdmin(): Promise<boolean>;
  };
  
  // Service management
  service: {
    getStatus(): Promise<{ running: boolean; pid: number | null }>;
    restart(): Promise<boolean>;
  };
  
  // Dialog operations
  dialog: {
    showMessage(options: Electron.MessageBoxOptions): Promise<Electron.MessageBoxReturnValue>;
    showOpenDialog(options: Electron.OpenDialogOptions): Promise<Electron.OpenDialogReturnValue>;
    showSaveDialog(options: Electron.SaveDialogOptions): Promise<Electron.SaveDialogReturnValue>;
  };
  
  // IPC communication with service
  ipc: {
    // Profile operations
    createProfile(name: string, masterKey: string): Promise<any>;
    getAllProfiles(): Promise<any>;
    authenticateProfile(profileId: string, masterKey: string): Promise<any>;
    changeProfilePassword(profileId: string, oldKey: string, newKey: string): Promise<any>;
    
    // Folder operations
    addFolder(profileId: string, folderPath: string): Promise<any>;
    getProfileFolders(profileId: string): Promise<any>;
    unlockFolderTemporary(profileId: string, folderId: string): Promise<any>;
    unlockFolderPermanent(profileId: string, folderId: string): Promise<any>;
    lockTemporaryFolders(profileId: string): Promise<any>;
    
    // Analytics operations
    getProfileAnalytics(profileId: string, timeRange: string): Promise<any>;
    getSystemAnalytics(timeRange: string): Promise<any>;
    
    // Recovery operations
    recoverWithKey(recoveryKey: string): Promise<any>;
    
    // Platform operations
    getPlatformInfo(): Promise<any>;
    getUnlockMethods(): Promise<any>;
    setPreferredUnlockMethod(method: string): Promise<any>;
  };
  
  // Event listeners
  on: {
    serviceStatusChanged(callback: (status: any) => void): void;
    folderStatusChanged(callback: (status: any) => void): void;
    securityEvent(callback: (event: any) => void): void;
  };
  
  // Remove event listeners
  off: {
    serviceStatusChanged(callback: (status: any) => void): void;
    folderStatusChanged(callback: (status: any) => void): void;
    securityEvent(callback: (event: any) => void): void;
  };
}

// Create the API object
const phantomVaultAPI: PhantomVaultAPI = {
  // App information
  app: {
    getVersion: () => ipcRenderer.invoke('app:getVersion'),
    isAdmin: () => ipcRenderer.invoke('app:isAdmin'),
  },
  
  // Service management
  service: {
    getStatus: () => ipcRenderer.invoke('service:getStatus'),
    restart: () => ipcRenderer.invoke('service:restart'),
  },
  
  // Dialog operations
  dialog: {
    showMessage: (options) => ipcRenderer.invoke('dialog:showMessage', options),
    showOpenDialog: (options) => ipcRenderer.invoke('dialog:showOpenDialog', options),
    showSaveDialog: (options) => ipcRenderer.invoke('dialog:showSaveDialog', options),
  },
  
  // IPC communication with service
  ipc: {
    // Profile operations
    createProfile: (name, masterKey) => 
      ipcRenderer.invoke('ipc:createProfile', { name, masterKey }),
    getAllProfiles: () => 
      ipcRenderer.invoke('ipc:getAllProfiles'),
    authenticateProfile: (profileId, masterKey) => 
      ipcRenderer.invoke('ipc:authenticateProfile', { profileId, masterKey }),
    changeProfilePassword: (profileId, oldKey, newKey) => 
      ipcRenderer.invoke('ipc:changeProfilePassword', { profileId, oldKey, newKey }),
    
    // Folder operations
    addFolder: (profileId, folderPath) => 
      ipcRenderer.invoke('ipc:addFolder', { profileId, folderPath }),
    getProfileFolders: (profileId) => 
      ipcRenderer.invoke('ipc:getProfileFolders', { profileId }),
    unlockFolderTemporary: (profileId, folderId) => 
      ipcRenderer.invoke('ipc:unlockFolderTemporary', { profileId, folderId }),
    unlockFolderPermanent: (profileId, folderId) => 
      ipcRenderer.invoke('ipc:unlockFolderPermanent', { profileId, folderId }),
    lockTemporaryFolders: (profileId) => 
      ipcRenderer.invoke('ipc:lockTemporaryFolders', { profileId }),
    
    // Analytics operations
    getProfileAnalytics: (profileId, timeRange) => 
      ipcRenderer.invoke('ipc:getProfileAnalytics', { profileId, timeRange }),
    getSystemAnalytics: (timeRange) => 
      ipcRenderer.invoke('ipc:getSystemAnalytics', { timeRange }),
    
    // Recovery operations
    recoverWithKey: (recoveryKey) => 
      ipcRenderer.invoke('ipc:recoverWithKey', { recoveryKey }),
    
    // Platform operations
    getPlatformInfo: () => 
      ipcRenderer.invoke('ipc:getPlatformInfo'),
    getUnlockMethods: () => 
      ipcRenderer.invoke('ipc:getUnlockMethods'),
    setPreferredUnlockMethod: (method) => 
      ipcRenderer.invoke('ipc:setPreferredUnlockMethod', { method }),
  },
  
  // Event listeners
  on: {
    serviceStatusChanged: (callback) => {
      ipcRenderer.on('service:statusChanged', (_, status) => callback(status));
    },
    folderStatusChanged: (callback) => {
      ipcRenderer.on('folder:statusChanged', (_, status) => callback(status));
    },
    securityEvent: (callback) => {
      ipcRenderer.on('security:event', (_, event) => callback(event));
    },
  },
  
  // Remove event listeners
  off: {
    serviceStatusChanged: (callback) => {
      ipcRenderer.removeListener('service:statusChanged', callback);
    },
    folderStatusChanged: (callback) => {
      ipcRenderer.removeListener('folder:statusChanged', callback);
    },
    securityEvent: (callback) => {
      ipcRenderer.removeListener('security:event', callback);
    },
  },
};

// Expose the API to the renderer process
contextBridge.exposeInMainWorld('phantomVault', phantomVaultAPI);

// Type declaration for TypeScript
declare global {
  interface Window {
    phantomVault: PhantomVaultAPI;
  }
}

console.log('[Preload] PhantomVault API exposed to renderer process');