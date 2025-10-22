/**
 * Electron API type definitions
 * Exposes the native PhantomVault core functions to the renderer process
 * 
 * Phase 4 - Step 6: Added comprehensive TypeScript definitions for native C++ API
 */

// ==================== PHASE 4: NATIVE C++ API TYPES ====================

/**
 * Result wrapper for native C++ operations
 * All native methods return this standardized response format
 */
export interface NativeOperationResult<T = any> {
  success: boolean;
  result?: T;
  error?: string;
}

/**
 * File attributes for native file system operations
 */
export interface FileAttributes {
  hidden: boolean;
  readonly: boolean;
  system: boolean;
  archive: boolean;
  compressed: boolean;
  encrypted: boolean;
  temporary: boolean;
  offline: boolean;
  notContentIndexed: boolean;
  sparseFile: boolean;
  reparsePoint: boolean;
}

/**
 * Partial file attributes for setting specific attributes
 */
export type PartialFileAttributes = Partial<FileAttributes>;

/**
 * Native C++ API exposed through N-API addon
 * All methods interact with the native PhantomVault core library
 * 
 * Features:
 * - Hardware-accelerated AES-256-GCM encryption (40-80x faster than JS)
 * - Native filesystem operations with cross-platform support
 * - Process concealment for stealth mode
 * - DOD-compliant secure deletion
 */
export interface PhantomVaultNativeAPI {
  // ==================== INITIALIZATION ====================
  
  /**
   * Initialize the PhantomVault native core
   * Must be called before any other native operations
   */
  initialize(): Promise<NativeOperationResult<boolean>>;

  /**
   * Check if the native core is initialized
   */
  isInitialized(): Promise<NativeOperationResult<boolean>>;

  /**
   * Get the native core version string (e.g., "1.0.0")
   */
  getVersion(): Promise<NativeOperationResult<string>>;

  // ==================== FOLDER ENCRYPTION ====================

  /**
   * Encrypt a folder and all its contents using AES-256-GCM
   * 
   * Performance: 40-80x faster than JavaScript implementation
   * Security: PBKDF2 key derivation with 10,000 iterations
   * 
   * @param folderPath - Absolute path to folder to encrypt
   * @param password - Encryption password (min 8 chars recommended)
   * @throws Error if folder doesn't exist, password is empty, or encryption fails
   */
  encryptFolder(folderPath: string, password: string): Promise<NativeOperationResult<boolean>>;

  /**
   * Decrypt a folder and all its contents using AES-256-GCM
   * 
   * Verifies authentication tags to ensure data integrity
   * 
   * @param folderPath - Absolute path to folder to decrypt
   * @param password - Decryption password (must match encryption password)
   * @throws Error if folder doesn't exist, password is incorrect, or decryption fails
   */
  decryptFolder(folderPath: string, password: string): Promise<NativeOperationResult<boolean>>;

  // ==================== FILE SYSTEM OPERATIONS ====================

  /**
   * Hide a folder by renaming it with a dot prefix (Unix-style hidden)
   * 
   * Example: /home/user/vault → /home/user/.vault
   * On Windows, also sets the HIDDEN file attribute
   */
  hideFolder(folderPath: string): Promise<NativeOperationResult<boolean>>;

  /**
   * Unhide a folder by removing the dot prefix
   * 
   * Example: /home/user/.vault → /home/user/vault
   * On Windows, also removes the HIDDEN file attribute
   */
  unhideFolder(folderPath: string): Promise<NativeOperationResult<boolean>>;

  /**
   * Check if a folder is currently hidden (has dot prefix)
   */
  isHidden(folderPath: string): Promise<NativeOperationResult<boolean>>;

  /**
   * Set file attributes (Windows-specific, no-op on Unix)
   */
  setFileAttributes(filePath: string, attributes: PartialFileAttributes): Promise<NativeOperationResult<boolean>>;

  /**
   * Get file attributes (Windows-specific, returns defaults on Unix)
   */
  getFileAttributes(filePath: string): Promise<NativeOperationResult<FileAttributes>>;

  // ==================== PROCESS CONCEALMENT ====================

  /**
   * Hide the current process by disguising it as a system process
   * 
   * Default disguise: "kworker/0:0" (Linux kernel worker)
   * Affects process name in ps, top, htop, and task managers
   * Original process name is preserved for restoration
   * 
   * @param disguiseName - Optional custom disguise name (default: "kworker/0:0")
   */
  hideProcess(disguiseName?: string): Promise<NativeOperationResult<boolean>>;

  /**
   * Restore the process to its original name
   */
  showProcess(): Promise<NativeOperationResult<boolean>>;

  /**
   * Check if the process is currently hidden (disguised)
   */
  isProcessHidden(): Promise<NativeOperationResult<boolean>>;

  /**
   * Set the process name to a custom value
   * 
   * @param processName - New process name (max 15 chars on Linux)
   */
  setProcessName(processName: string): Promise<NativeOperationResult<boolean>>;

  /**
   * Get the current process name as it appears in task managers
   */
  getCurrentProcessName(): Promise<NativeOperationResult<string>>;

  /**
   * Get the original process name before any disguises were applied
   */
  getOriginalProcessName(): Promise<NativeOperationResult<string>>;
}

// ==================== VAULT PROFILE & FOLDER MANAGER API ====================

/**
 * Vault Profile Manager API for managing user profiles
 */
export interface PhantomVaultProfileAPI {
  /**
   * Get the active profile
   */
  getActive(): Promise<{ success: boolean; profile?: any; error?: string }>;

  /**
   * Get all profiles
   */
  getAll(): Promise<{ success: boolean; profiles?: any[]; error?: string }>;

  /**
   * Create a new profile
   */
  create(name: string, masterPassword: string, recoveryKey: string): 
    Promise<{ success: boolean; profile?: any; error?: string }>;

  /**
   * Set the active profile
   */
  setActive(profileId: string): Promise<{ success: boolean; error?: string }>;

  /**
   * Verify password against profile
   */
  verifyPassword(profileId: string, password: string): 
    Promise<{ success: boolean; isValid?: boolean; error?: string }>;
}

/**
 * Vault Folder Manager API for managing encrypted folders
 */
export interface PhantomVaultFolderAPI {
  /**
   * Get all folders for a profile
   */
  getAll(profileId: string): Promise<{ success: boolean; folders?: any[]; error?: string }>;

  /**
   * Lock a folder
   */
  lock(profileId: string, folderId: string): Promise<{ success: boolean; result?: any; error?: string }>;

  /**
   * Lock a folder with a specific password (auto-encrypt + hide)
   */
  lockWithPassword(profileId: string, folderId: string, password: string): 
    Promise<{ success: boolean; result?: any; error?: string }>;

  /**
   * Unlock a folder
   */
  unlock(profileId: string, folderId: string, password: string, mode?: 'temporary' | 'permanent'): 
    Promise<{ success: boolean; result?: any; error?: string }>;

  /**
   * Unlock all folders with a password
   */
  unlockAll(profileId: string, password: string, mode?: 'temporary' | 'permanent'): 
    Promise<{ success: boolean; result?: any; error?: string }>;

  /**
   * Add a folder to the profile
   */
  add(profileId: string, folderPath: string, folderName?: string): 
    Promise<{ success: boolean; folderId?: string; folder?: any; error?: string }>;

  /**
   * Remove a folder from the profile
   */
  remove(profileId: string, folderId: string): 
    Promise<{ success: boolean; error?: string }>;
}

// ==================== LEGACY API TYPES ====================

export interface EncryptionResult {
  success: boolean;
  data?: Uint8Array;
  error?: string;
}

export interface HotkeyConfig {
  unlockHotkey: string;
  recoveryHotkey: string;
  enabled: boolean;
}

export interface HotkeyAPI {
  getConfig: () => Promise<HotkeyConfig>;
  setUnlockHotkey: (accelerator: string) => Promise<{ success: boolean; error?: string }>;
  setRecoveryHotkey: (accelerator: string) => Promise<{ success: boolean; error?: string }>;
  isAvailable: (accelerator: string) => Promise<boolean>;
  getSuggestions: () => Promise<string[]>;
  setEnabled: (enabled: boolean) => Promise<{ success: boolean; error?: string }>;
}

// Phase 3: Auto-Lock Manager
export interface AutoLockStats {
  isMonitoring: boolean;
  platform: string;
  temporaryUnlocks: number;
  unlocks: Array<{
    folderId: string;
    mode: 'temporary' | 'permanent';
    unlockedAt: Date;
    folderPath: string;
  }>;
}

export interface LockResult {
  success: number;
  failed: number;
  errors: string[];
}

export interface AutoLockAPI {
  registerUnlock: (folderId: string, profileId: string, mode: 'temporary' | 'permanent', folderPath: string) => Promise<{ success: boolean; error?: string }>;
  unregisterUnlock: (folderId: string) => Promise<{ success: boolean; error?: string }>;
  getStats: () => Promise<AutoLockStats>;
  lockAllTemporary: () => Promise<LockResult>;
}

// Phase 3: File System Watcher
export interface WatcherEvent {
  type: 'add' | 'change' | 'unlink';
  path: string;
  timestamp: Date;
}

export interface WatcherStats {
  isWatching: boolean;
  storagePath: string | null;
  totalEvents: number;
  recentEvents: WatcherEvent[];
}

export interface FSWatcherAPI {
  getStats: () => Promise<WatcherStats>;
  getRecentEvents: (count?: number) => Promise<WatcherEvent[]>;
}

export interface PhantomVaultAPI {
  // ==================== PHASE 4: NATIVE C++ API ====================
  
  /**
   * Native C++ operations via N-API addon
   * 
   * High-performance encryption, file system, and process operations
   * All methods are async and return NativeOperationResult wrapper
   * 
   * @example
   * // Encrypt and hide a folder
   * const encryptResult = await window.phantomVault.native.encryptFolder(
   *   '/home/user/secrets',
   *   'MyPassword123!'
   * );
   * if (encryptResult.success) {
   *   await window.phantomVault.native.hideFolder('/home/user/secrets');
   * }
   * 
   * @example
   * // Hide process in task manager
   * await window.phantomVault.native.hideProcess('systemd');
   */
  native: PhantomVaultNativeAPI;
  
  /**
   * Vault profile management (main process)
   */
  profile: PhantomVaultProfileAPI;
  
  /**
   * Vault folder management (main process)
   */
  folder: PhantomVaultFolderAPI;
  
  // ==================== LEGACY API ====================
  
  // Core library
  getVersion: () => Promise<string>;
  
  // Encryption operations
  generateKey: () => Promise<Uint8Array>;
  generateIV: () => Promise<Uint8Array>;
  generateSalt: () => Promise<Uint8Array>;
  encryptData: (data: Uint8Array, key: Uint8Array, iv: Uint8Array) => Promise<EncryptionResult>;
  decryptData: (encryptedData: Uint8Array, key: Uint8Array, iv: Uint8Array) => Promise<EncryptionResult>;
  encryptFile: (sourcePath: string, destPath: string, key: Uint8Array, iv: Uint8Array) => Promise<boolean>;
  decryptFile: (sourcePath: string, destPath: string, key: Uint8Array, iv: Uint8Array) => Promise<boolean>;
  deriveKeyFromPassword: (password: string, salt: Uint8Array) => Promise<Uint8Array>;
  
  // File system operations
  selectFolder: () => Promise<string | null>;
  fileExists: (path: string) => Promise<boolean>;
  hideFolder: (path: string) => Promise<boolean>;
  unhideFolder: (path: string) => Promise<boolean>;
  
  // System integration
  showNotification: (title: string, body: string) => void;
  
  // PhantomVault 2.0 - Hotkey Manager
  hotkey: HotkeyAPI;
  
  // PhantomVault 2.0 Phase 3 - Auto-Lock Manager
  autoLock: AutoLockAPI;
  
  // PhantomVault 2.0 Phase 3 - File System Watcher
  fsWatcher: FSWatcherAPI;
  
  // Window management
  minimizeToTray: () => void;
  showWindow: () => void;
  hideWindow: () => void;
  quitApp: () => void;
  
  // PhantomVault 2.0 - Event Listeners
  onShowUnlockOverlay: (callback: (data: { isRecoveryMode: boolean }) => void) => () => void;
  onAutoLockFolder: (callback: (data: { folderId: string }) => void) => () => void;
  onMetadataChanged: (callback: (data: { filePath: string }) => void) => () => void;
  onSuspiciousActivity: (callback: (data: { activity: string }) => void) => () => void;
}

declare global {
  interface Window {
    phantomVault: PhantomVaultAPI;
  }
}

export {};