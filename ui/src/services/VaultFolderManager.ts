/**
 * VaultFolderManager - Manages encrypted folder vaults
 * Handles folder encryption, locking, and metadata
 * 
 * Phase 4 - Step 6: Added TypeScript types for native C++ operations
 */

import * as fs from 'fs';
import * as path from 'path';
import * as crypto from 'crypto';
import { VaultProfileManager } from './VaultProfileManager';
import type { NativeOperationResult } from '../types/electron';

export interface FolderVault {
  id: string;
  folderPath: string;
  folderName: string;
  isLocked: boolean;
  usesMasterPassword: boolean;
  customPasswordHash?: string;
  customRecoveryKeyHash?: string;
  encryptedCustomRecoveryKey?: string;
  createdAt: number;
  lastUnlocked?: number;
  unlockMode?: 'temporary' | 'permanent' | null;
}

export interface VaultFoldersMetadata {
  profileId: string;
  folders: FolderVault[];
  lastModified: number;
}

export class VaultFolderManager {
  private static instance: VaultFolderManager;
  private profileManager: VaultProfileManager;
  private foldersMetadata: Map<string, VaultFoldersMetadata> = new Map();
  
  // Track unlock states in memory (not persisted)
  private unlockStates: Map<string, {
    mode: 'temporary' | 'permanent';
    unlockedAt: Date;
    profileId: string;
    folderPath: string;
  }> = new Map();
  
  // Callback for unlock events (used by AutoLockManager)
  private onUnlockCallback: ((folderId: string, profileId: string, mode: 'temporary' | 'permanent', folderPath: string) => void) | null = null;
  private onLockCallback: ((folderId: string) => void) | null = null;

  private constructor() {
    this.profileManager = VaultProfileManager.getInstance();
  }

  public static getInstance(): VaultFolderManager {
    if (!VaultFolderManager.instance) {
      VaultFolderManager.instance = new VaultFolderManager();
    }
    return VaultFolderManager.instance;
  }

  /**
   * Get folders metadata file path for a profile
   */
  private getFoldersMetadataPath(profileId: string): string {
    const profilePath = this.profileManager.getProfilePath(profileId);
    return path.join(profilePath, 'folders_metadata.json');
  }

  /**
   * Load folders metadata for a profile
   */
  public loadFoldersMetadata(profileId: string): VaultFoldersMetadata {
    const metadataPath = this.getFoldersMetadataPath(profileId);

    if (!fs.existsSync(metadataPath)) {
      const metadata: VaultFoldersMetadata = {
        profileId,
        folders: [],
        lastModified: Date.now(),
      };
      this.foldersMetadata.set(profileId, metadata);
      this.saveFoldersMetadata(profileId);
      return metadata;
    }

    const data = fs.readFileSync(metadataPath, 'utf-8');
    const metadata = JSON.parse(data);
    this.foldersMetadata.set(profileId, metadata);
    return metadata;
  }

  /**
   * Save folders metadata for a profile
   */
  private saveFoldersMetadata(profileId: string): void {
    const metadata = this.foldersMetadata.get(profileId);
    if (!metadata) return;

    metadata.lastModified = Date.now();
    const metadataPath = this.getFoldersMetadataPath(profileId);
    fs.writeFileSync(
      metadataPath,
      JSON.stringify(metadata, null, 2),
      { mode: 0o600 }
    );
  }

  /**
   * Add folder to vault
   */
  public addFolder(
    profileId: string,
    folderPath: string,
    customPassword?: string
  ): FolderVault {
    let metadata = this.foldersMetadata.get(profileId);
    if (!metadata) {
      metadata = this.loadFoldersMetadata(profileId);
    }

    // Check if folder already exists
    const existing = metadata.folders.find((f) => f.folderPath === folderPath);
    if (existing) {
      throw new Error('Folder already in vault');
    }

    // Verify folder exists
    if (!fs.existsSync(folderPath)) {
      throw new Error('Folder does not exist');
    }

    const folderName = path.basename(folderPath);
    const vault: FolderVault = {
      id: `vault_${Date.now()}_${crypto.randomBytes(4).toString('hex')}`,
      folderPath,
      folderName,
      isLocked: false,
      usesMasterPassword: !customPassword,
      createdAt: Date.now(),
      unlockMode: null,
    };

    // If custom password provided, set it up
    if (customPassword) {
      const { hash, salt } = this.hashPassword(customPassword);
      vault.customPasswordHash = `${salt}:${hash}`;

      // Generate custom recovery key
      const recoveryKey = this.profileManager.generateRecoveryKey();
      const { hash: recoveryHash, salt: recoverySalt } = this.hashPassword(recoveryKey);
      vault.customRecoveryKeyHash = `${recoverySalt}:${recoveryHash}`;

      // Encrypt custom recovery key
      vault.encryptedCustomRecoveryKey = this.encryptRecoveryKey(
        recoveryKey,
        customPassword
      );
    }

    metadata.folders.push(vault);
    this.saveFoldersMetadata(profileId);

    return vault;
  }

  /**
   * Hash password (same as VaultProfileManager)
   */
  private hashPassword(password: string, salt?: Buffer): { hash: string; salt: string } {
    const passwordSalt = salt || crypto.randomBytes(32);
    const hash = crypto.pbkdf2Sync(password, passwordSalt, 100000, 64, 'sha512');
    return {
      hash: hash.toString('hex'),
      salt: passwordSalt.toString('hex'),
    };
  }

  /**
   * Encrypt recovery key with password
   */
  private encryptRecoveryKey(recoveryKey: string, password: string): string {
    const key = crypto.scryptSync(password, 'salt', 32);
    const iv = crypto.randomBytes(16);
    const cipher = crypto.createCipheriv('aes-256-cbc', key, iv);

    let encrypted = cipher.update(recoveryKey, 'utf8', 'hex');
    encrypted += cipher.final('hex');

    return iv.toString('hex') + ':' + encrypted;
  }

  /**
   * Get all folders for a profile
   */
  public getFolders(profileId: string): FolderVault[] {
    let metadata = this.foldersMetadata.get(profileId);
    if (!metadata) {
      metadata = this.loadFoldersMetadata(profileId);
    }
    return metadata.folders;
  }

  /**
   * Get folder by ID
   */
  public getFolder(profileId: string, vaultId: string): FolderVault | null {
    const folders = this.getFolders(profileId);
    return folders.find((f) => f.id === vaultId) || null;
  }

  /**
   * Lock folder (encrypt + hide it)
   * Phase 4: Uses native C++ encryption and filesystem operations
   */
  public async lockFolder(profileId: string, vaultId: string): Promise<void> {
    const metadata = this.foldersMetadata.get(profileId);
    if (!metadata) {
      throw new Error('Profile metadata not found');
    }

    const folder = metadata.folders.find((f) => f.id === vaultId);
    if (!folder) {
      throw new Error('Folder not found');
    }
    
    if (folder.isLocked) {
      return; // Already locked
    }

    try {
      // Get password for encryption
      const profile = this.profileManager.getActiveProfile();
      if (!profile) {
        throw new Error('No active profile');
      }
      
      let password: string;
      
      if (folder.usesMasterPassword) {
        // Use master password (need to get it from profile manager)
        // For now, we'll need to pass it in or store it temporarily
        // This is a limitation we'll address in the UI integration
        throw new Error('Master password must be provided for locking');
      } else if (folder.customPasswordHash) {
        // For custom password folders, we need the password
        // This should be handled by the UI prompting the user
        throw new Error('Custom password must be provided for locking');
      } else {
        throw new Error('No password configured for folder');
      }
      
      // Note: In production, the UI will call lockFolderWithPassword instead
      
    } catch (error) {
      console.error('Lock folder error:', error);
      throw error;
    }
  }
  
  /**
   * Lock folder with password (Phase 4: Native C++ implementation)
   */
  public async lockFolderWithPassword(
    profileId: string,
    vaultId: string,
    password: string
  ): Promise<void> {
    const metadata = this.foldersMetadata.get(profileId);
    if (!metadata) {
      throw new Error('Profile metadata not found');
    }

    const folder = metadata.folders.find((f) => f.id === vaultId);
    if (!folder) {
      throw new Error('Folder not found');
    }
    
    if (folder.isLocked) {
      return; // Already locked
    }

    try {
      // Phase 4: Use native C++ encryption
      if ((window as any).phantomVault?.native) {
        console.log(`[Phase 4] Encrypting folder with C++ core: ${folder.folderPath}`);
        
        // Step 1: Encrypt folder using C++ (AES-256-GCM)
        const encryptResult = await (window as any).phantomVault.native.encryptFolder(
          folder.folderPath,
          password
        );
        
        if (!encryptResult.success) {
          throw new Error(`Encryption failed: ${encryptResult.error}`);
        }
        
        console.log(`[Phase 4] ✅ Folder encrypted: ${folder.folderPath}`);
        
        // Step 2: Hide folder using C++ (dot prefix)
        const hideResult = await (window as any).phantomVault.native.hideFolder(
          folder.folderPath
        );
        
        if (!hideResult.success) {
          throw new Error(`Hide failed: ${hideResult.error}`);
        }
        
        // Update folder path to hidden path
        folder.folderPath = hideResult.newPath;
        console.log(`[Phase 4] ✅ Folder hidden: ${hideResult.newPath}`);
        
      } else {
        // Fallback: Use JavaScript implementation
        console.warn('[Phase 4] Native addon not available, using fallback');
        this.lockFolderFallback(folder);
      }

      folder.isLocked = true;
      folder.unlockMode = null;
      
      // Clear unlock state from memory
      this.unlockStates.delete(folder.id);
      
      // Notify callback (for AutoLockManager)
      if (this.onLockCallback) {
        this.onLockCallback(folder.id);
      }
      
      this.saveFoldersMetadata(profileId);
      
    } catch (error) {
      console.error('Lock folder error:', error);
      throw error;
    }
  }
  
  /**
   * Fallback lock implementation (JavaScript)
   */
  private lockFolderFallback(folder: FolderVault): void {
    // Hide folder by adding dot prefix
    const folderDir = path.dirname(folder.folderPath);
    const folderName = path.basename(folder.folderPath);
    const hiddenPath = path.join(folderDir, `.${folderName}`);

    if (fs.existsSync(folder.folderPath)) {
      fs.renameSync(folder.folderPath, hiddenPath);
      folder.folderPath = hiddenPath;
    }
  }

  /**
   * Unlock folder with password and mode (T/P)
   * Phase 4: Uses native C++ decryption and filesystem operations
   */
  public async unlockFolder(
    profileId: string,
    vaultId: string,
    password: string,
    mode: 'temporary' | 'permanent'
  ): Promise<boolean> {
    const metadata = this.foldersMetadata.get(profileId);
    if (!metadata) return false;

    const folder = metadata.folders.find((f) => f.id === vaultId);
    if (!folder) return false;
    
    if (!folder.isLocked) {
      return true; // Already unlocked
    }

    // Verify password
    const profile = this.profileManager.getActiveProfile();
    if (!profile) return false;

    let passwordValid = false;

    if (folder.usesMasterPassword) {
      // Use master password
      passwordValid = this.profileManager.verifyProfilePassword(profileId, password);
    } else if (folder.customPasswordHash) {
      // Use custom password
      const [saltHex, hashHex] = folder.customPasswordHash.split(':');
      const salt = Buffer.from(saltHex, 'hex');
      const { hash } = this.hashPassword(password, salt);
      passwordValid = hash === hashHex;
    }

    if (!passwordValid) {
      return false;
    }

    try {
      // Phase 4: Use native C++ decryption
      if ((window as any).phantomVault?.native) {
        console.log(`[Phase 4] Unhiding folder with C++ core: ${folder.folderPath}`);
        
        // Step 1: Unhide folder using C++ (remove dot prefix)
        const unhideResult = await (window as any).phantomVault.native.unhideFolder(
          folder.folderPath
        );
        
        if (!unhideResult.success) {
          throw new Error(`Unhide failed: ${unhideResult.error}`);
        }
        
        // Update folder path to visible path
        const visiblePath = unhideResult.newPath;
        console.log(`[Phase 4] ✅ Folder unhidden: ${visiblePath}`);
        
        // Step 2: Decrypt folder using C++ (AES-256-GCM)
        const decryptResult = await (window as any).phantomVault.native.decryptFolder(
          visiblePath,
          password
        );
        
        if (!decryptResult.success) {
          // Decryption failed - re-hide the folder
          console.error(`[Phase 4] ❌ Decryption failed, re-hiding folder`);
          await (window as any).phantomVault.native.hideFolder(visiblePath);
          throw new Error(`Decryption failed: ${decryptResult.error}`);
        }
        
        folder.folderPath = visiblePath;
        console.log(`[Phase 4] ✅ Folder decrypted: ${visiblePath}`);
        
      } else {
        // Fallback: Use JavaScript implementation
        console.warn('[Phase 4] Native addon not available, using fallback');
        this.unlockFolderFallback(folder);
      }

      folder.isLocked = false;
      folder.lastUnlocked = Date.now();
      folder.unlockMode = mode;
      
      // Track unlock state in memory
      this.unlockStates.set(folder.id, {
        mode,
        unlockedAt: new Date(),
        profileId,
        folderPath: folder.folderPath,
      });
      
      // Notify callback (for AutoLockManager)
      if (this.onUnlockCallback) {
        this.onUnlockCallback(folder.id, profileId, mode, folder.folderPath);
      }

      // If permanent unlock, remove from vault
      if (mode === 'permanent') {
        const index = metadata.folders.indexOf(folder);
        if (index > -1) {
          metadata.folders.splice(index, 1);
        }
      }

      this.saveFoldersMetadata(profileId);
      return true;
      
    } catch (error) {
      console.error('Unlock folder error:', error);
      return false;
    }
  }
  
  /**
   * Fallback unlock implementation (JavaScript)
   */
  private unlockFolderFallback(folder: FolderVault): void {
    // Unhide folder
    const folderDir = path.dirname(folder.folderPath);
    const folderName = path.basename(folder.folderPath).replace(/^\./, '');
    const visiblePath = path.join(folderDir, folderName);

    if (fs.existsSync(folder.folderPath)) {
      fs.renameSync(folder.folderPath, visiblePath);
      folder.folderPath = visiblePath;
    }
  }

  /**
   * Unlock all folders with master password
   * Phase 4: Async to support native C++ operations
   */
  public async unlockAllFolders(
    profileId: string,
    password: string,
    mode: 'temporary' | 'permanent'
  ): Promise<{ success: number; failed: number }> {
    const folders = this.getFolders(profileId);
    let success = 0;
    let failed = 0;

    for (const folder of folders) {
      if (folder.isLocked && folder.usesMasterPassword) {
        const unlocked = await this.unlockFolder(profileId, folder.id, password, mode);
        if (unlocked) {
          success++;
        } else {
          failed++;
        }
      }
    }

    return { success, failed };
  }

  /**
   * Remove folder from vault (permanent unlock)
   */
  public removeFolder(profileId: string, vaultId: string): void {
    const metadata = this.foldersMetadata.get(profileId);
    if (!metadata) return;

    const index = metadata.folders.findIndex((f) => f.id === vaultId);
    if (index > -1) {
      metadata.folders.splice(index, 1);
      this.saveFoldersMetadata(profileId);
    }
  }

  /**
   * Lock all temporary folders (for auto-lock)
   */
  public lockTemporaryFolders(profileId: string): void {
    const metadata = this.foldersMetadata.get(profileId);
    if (!metadata) return;

    for (const folder of metadata.folders) {
      if (!folder.isLocked && folder.unlockMode === 'temporary') {
        this.lockFolder(profileId, folder.id);
      }
    }
  }
  
  /**
   * Get all temporarily unlocked folders
   */
  public getTemporaryUnlockedFolders(profileId: string): FolderVault[] {
    const metadata = this.foldersMetadata.get(profileId);
    if (!metadata) return [];
    
    return metadata.folders.filter(
      f => !f.isLocked && f.unlockMode === 'temporary'
    );
  }
  
  /**
   * Register callback for unlock events
   */
  public onUnlock(callback: (folderId: string, profileId: string, mode: 'temporary' | 'permanent', folderPath: string) => void): void {
    this.onUnlockCallback = callback;
  }
  
  /**
   * Register callback for lock events
   */
  public onLock(callback: (folderId: string) => void): void {
    this.onLockCallback = callback;
  }
  
  /**
   * Get unlock state for a folder
   */
  public getUnlockState(folderId: string) {
    return this.unlockStates.get(folderId);
  }
  
  /**
   * Get all unlock states
   */
  public getAllUnlockStates() {
    return Array.from(this.unlockStates.entries()).map(([id, state]) => ({
      folderId: id,
      ...state,
    }));
  }
  
  /**
   * Update folder metadata after lock/unlock from main process
   * This is called from IPC handlers that directly manipulate folders
   */
  public updateFolderLockState(
    profileId: string,
    folderId: string,
    isLocked: boolean,
    newFolderPath?: string
  ): void {
    const metadata = this.foldersMetadata.get(profileId);
    if (!metadata) {
      throw new Error('Profile metadata not found');
    }

    const folder = metadata.folders.find((f) => f.id === folderId);
    if (!folder) {
      throw new Error('Folder not found');
    }

    folder.isLocked = isLocked;
    if (newFolderPath) {
      folder.folderPath = newFolderPath;
    }
    
    if (!isLocked) {
      folder.lastUnlocked = Date.now();
    } else {
      folder.unlockMode = null;
      this.unlockStates.delete(folderId);
    }

    this.saveFoldersMetadata(profileId);
  }
}
