const { contextBridge, ipcRenderer } = require('electron');

/**
 * Preload script - Exposes safe IPC methods to renderer process
 * This script runs in an isolated context with access to both Node.js and DOM APIs
 */

// Expose protected methods that renderer can use
contextBridge.exposeInMainWorld('phantomVault', {
  // Core library
  getVersion: () => ipcRenderer.invoke('get-version'),

  // Encryption operations (will be implemented via IPC)
  generateKey: () => ipcRenderer.invoke('generate-key'),
  generateIV: () => ipcRenderer.invoke('generate-iv'),
  generateSalt: () => ipcRenderer.invoke('generate-salt'),
  encryptData: (data, key, iv) => ipcRenderer.invoke('encrypt-data', data, key, iv),
  decryptData: (encryptedData, key, iv) => ipcRenderer.invoke('decrypt-data', encryptedData, key, iv),
  encryptFile: (sourcePath, destPath, key, iv) => ipcRenderer.invoke('encrypt-file', sourcePath, destPath, key, iv),
  decryptFile: (sourcePath, destPath, key, iv) => ipcRenderer.invoke('decrypt-file', sourcePath, destPath, key, iv),
  deriveKeyFromPassword: (password, salt) => ipcRenderer.invoke('derive-key-from-password', password, salt),

  // File system operations
  selectFolder: () => ipcRenderer.invoke('select-folder'),
  fileExists: (path) => ipcRenderer.invoke('file-exists', path),
  hideFolder: (path) => ipcRenderer.invoke('hide-folder', path),
  unhideFolder: (path) => ipcRenderer.invoke('unhide-folder', path),

  // Configuration
  saveConfig: (vaultId, config) => ipcRenderer.invoke('save-config', vaultId, config),
  loadConfig: (vaultId) => ipcRenderer.invoke('load-config', vaultId),

  // Password recovery
  setupRecovery: (vaultId, questions, answers) => ipcRenderer.invoke('setup-recovery', vaultId, questions, answers),
  getRecoveryQuestions: (vaultId) => ipcRenderer.invoke('get-recovery-questions', vaultId),
  verifyRecoveryAnswers: (vaultId, answers) => ipcRenderer.invoke('verify-recovery-answers', vaultId, answers),

  // System integration
  registerGlobalHotkey: (hotkey) => ipcRenderer.invoke('register-global-hotkey', hotkey),
  getCurrentHotkey: () => ipcRenderer.invoke('get-current-hotkey'),
  showNotification: (title, body) => ipcRenderer.invoke('show-notification', title, body),
  closeOverlayWindow: () => ipcRenderer.invoke('close-overlay-window'),

  // PhantomVault 2.0 - Hotkey Manager APIs
  hotkey: {
    getConfig: () => ipcRenderer.invoke('hotkey:get-config'),
    setUnlockHotkey: (accelerator) => ipcRenderer.invoke('hotkey:set-unlock', accelerator),
    setRecoveryHotkey: (accelerator) => ipcRenderer.invoke('hotkey:set-recovery', accelerator),
    isAvailable: (accelerator) => ipcRenderer.invoke('hotkey:is-available', accelerator),
    getSuggestions: () => ipcRenderer.invoke('hotkey:get-suggestions'),
    setEnabled: (enabled) => ipcRenderer.invoke('hotkey:set-enabled', enabled),
  },
  
  // PhantomVault 2.0 Phase 3 - AutoLock Manager APIs
  autoLock: {
    registerUnlock: (folderId, profileId, mode, folderPath) => 
      ipcRenderer.invoke('autolock:register-unlock', { folderId, profileId, mode, folderPath }),
    unregisterUnlock: (folderId) => 
      ipcRenderer.invoke('autolock:unregister-unlock', { folderId }),
    getStats: () => ipcRenderer.invoke('autolock:get-stats'),
    lockAllTemporary: () => ipcRenderer.invoke('autolock:lock-all-temporary'),
  },
  
  // PhantomVault 2.0 Phase 3 - FileSystemWatcher APIs
  fsWatcher: {
    getStats: () => ipcRenderer.invoke('fswatcher:get-stats'),
    getRecentEvents: (count) => ipcRenderer.invoke('fswatcher:get-recent-events', count),
  },
  
  // PhantomVault 2.0 Phase 4 - Native C++ Core Integration
  native: {
    // Encryption operations
    encryptFolder: (folderPath, password) => 
      ipcRenderer.invoke('native:encrypt-folder', folderPath, password),
    decryptFolder: (folderPath, password) => 
      ipcRenderer.invoke('native:decrypt-folder', folderPath, password),
    
    // File system operations
    hideFolder: (folderPath) => 
      ipcRenderer.invoke('native:hide-folder', folderPath),
    unhideFolder: (folderPath) => 
      ipcRenderer.invoke('native:unhide-folder', folderPath),
    isHidden: (folderPath) => 
      ipcRenderer.invoke('native:is-hidden', folderPath),
    setFileAttributes: (filePath, attributes) => 
      ipcRenderer.invoke('native:set-file-attributes', filePath, attributes),
    getFileAttributes: (filePath) => 
      ipcRenderer.invoke('native:get-file-attributes', filePath),
    
    // Process concealment
    hideProcess: () => 
      ipcRenderer.invoke('native:hide-process'),
    showProcess: () => 
      ipcRenderer.invoke('native:show-process'),
    isProcessHidden: () => 
      ipcRenderer.invoke('native:is-process-hidden'),
    setProcessName: (name) => 
      ipcRenderer.invoke('native:set-process-name', name),
    getCurrentProcessName: () => 
      ipcRenderer.invoke('native:get-current-process-name'),
    getOriginalProcessName: () => 
      ipcRenderer.invoke('native:get-original-process-name'),
    
    // Core operations
    initialize: () => 
      ipcRenderer.invoke('native:initialize'),
    getVersion: () => 
      ipcRenderer.invoke('native:get-version'),
    isInitialized: () => 
      ipcRenderer.invoke('native:is-initialized'),
  },
  
  // PhantomVault 2.0 Phase 4 - Vault Profile Manager
  profile: {
    getActive: () => ipcRenderer.invoke('profile:get-active'),
    getAll: () => ipcRenderer.invoke('profile:get-all'),
    create: (name, masterPassword, recoveryKey) => 
      ipcRenderer.invoke('profile:create', name, masterPassword, recoveryKey),
    setActive: (profileId) => ipcRenderer.invoke('profile:set-active', profileId),
    verifyPassword: (profileId, password) => 
      ipcRenderer.invoke('profile:verify-password', profileId, password),
  },
  
  // PhantomVault 2.0 Phase 4 - Vault Folder Manager
  folder: {
    getAll: (profileId) => ipcRenderer.invoke('folder:get-all', profileId),
    lock: (profileId, folderId) => ipcRenderer.invoke('folder:lock', profileId, folderId),
    lockWithPassword: (profileId, folderId, password) => 
      ipcRenderer.invoke('folder:lock-with-password', profileId, folderId, password),
    unlock: (profileId, folderId, password, mode) => 
      ipcRenderer.invoke('folder:unlock', profileId, folderId, password, mode),
    unlockAll: (profileId, password, mode) => 
      ipcRenderer.invoke('folder:unlock-all', profileId, password, mode),
    hasTemporaryUnlocked: (profileId) => 
      ipcRenderer.invoke('folder:has-temporary-unlocked', profileId),
    lockAllTemporary: (profileId, password) => 
      ipcRenderer.invoke('folder:lock-all-temporary', profileId, password),
    add: (profileId, folderPath, folderName) => 
      ipcRenderer.invoke('folder:add', profileId, folderPath, folderName),
    remove: (profileId, folderId) => 
      ipcRenderer.invoke('folder:remove', profileId, folderId),
  },

  // Window management
  minimizeToTray: () => ipcRenderer.invoke('minimize-to-tray'),
  showWindow: () => ipcRenderer.invoke('show-window'),
  hideWindow: () => ipcRenderer.invoke('hide-window'),
  quitApp: () => ipcRenderer.invoke('quit-app'),

  // Event listeners
  onLockAllVaults: (callback) => {
    ipcRenderer.on('lock-all-vaults', callback);
    return () => ipcRenderer.removeListener('lock-all-vaults', callback);
  },
  onShowSettings: (callback) => {
    ipcRenderer.on('show-settings', callback);
    return () => ipcRenderer.removeListener('show-settings', callback);
  },
  
  // PhantomVault 2.0 - Invisible Overlay Events
  onShowUnlockOverlay: (callback) => {
    ipcRenderer.on('show-unlock-overlay', (event, data) => {
      console.log('🔔 PRELOAD: Received show-unlock-overlay event', data);
      callback(data);
    });
    return () => ipcRenderer.removeListener('show-unlock-overlay', callback);
  },
  
  // PhantomVault 2.0 Phase 3 - Auto-Lock Events
  onAutoLockFolder: (callback) => {
    ipcRenderer.on('auto-lock-folder', (event, data) => callback(data));
    return () => ipcRenderer.removeListener('auto-lock-folder', callback);
  },
  
  onMetadataChanged: (callback) => {
    ipcRenderer.on('metadata-changed', (event, data) => callback(data));
    return () => ipcRenderer.removeListener('metadata-changed', callback);
  },
  
  onSuspiciousActivity: (callback) => {
    ipcRenderer.on('suspicious-activity', (event, data) => callback(data));
    return () => ipcRenderer.removeListener('suspicious-activity', callback);
  },
});

// Log that preload is ready
console.log('🔒 PhantomVault preload script loaded');
console.log('📋 Exposed APIs:', Object.keys(window.phantomVault || {}));
