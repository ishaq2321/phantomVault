"use strict";
/**
 * VaultFolderManager - Manages encrypted folder vaults
 * Handles folder encryption, locking, and metadata
 *
 * Phase 4 - Step 6: Added TypeScript types for native C++ operations
 */
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
exports.VaultFolderManager = void 0;
const fs = __importStar(require("fs"));
const path = __importStar(require("path"));
const crypto = __importStar(require("crypto"));
const VaultProfileManager_1 = require("./VaultProfileManager");
class VaultFolderManager {
    constructor(vaultAddon = null) {
        this.foldersMetadata = new Map();
        // Track unlock states in memory (not persisted)
        this.unlockStates = new Map();
        // Callback for unlock events (used by AutoLockManager)
        this.onUnlockCallback = null;
        this.onLockCallback = null;
        this.profileManager = VaultProfileManager_1.VaultProfileManager.getInstance();
        this.vaultAddon = vaultAddon; // Store the C++ addon instance
    }
    static getInstance(vaultAddon = null) {
        if (!VaultFolderManager.instance) {
            VaultFolderManager.instance = new VaultFolderManager(vaultAddon);
        } else if (vaultAddon && !VaultFolderManager.instance.vaultAddon) {
            // Set the addon if it wasn't set during construction
            VaultFolderManager.instance.vaultAddon = vaultAddon;
        }
        return VaultFolderManager.instance;
    }
    /**
     * Get folders metadata file path for a profile
     */
    getFoldersMetadataPath(profileId) {
        const profilePath = this.profileManager.getProfilePath(profileId);
        return path.join(profilePath, 'folders_metadata.json');
    }
    /**
     * Get HMAC key for folders metadata
     */
    getHMACKey(profileId) {
        const os = require('os');
        // Use profile ID + machine info as key material
        const keyMaterial = `${profileId}-${os.hostname()}-${os.userInfo().username}`;
        return crypto.createHash('sha256').update(keyMaterial).digest();
    }
    
    /**
     * Load folders metadata for a profile with HMAC verification
     * PRIORITY 2 SECURITY: Detects tampering
     */
    loadFoldersMetadata(profileId) {
        const metadataPath = this.getFoldersMetadataPath(profileId);
        if (!fs.existsSync(metadataPath)) {
            const metadata = {
                profileId,
                folders: [],
                lastModified: Date.now(),
            };
            this.foldersMetadata.set(profileId, metadata);
            this.saveFoldersMetadata(profileId);
            return metadata;
        }
        
        const data = fs.readFileSync(metadataPath, 'utf-8');
        const parsed = JSON.parse(data);
        
        // PRIORITY 2: Verify HMAC if present
        if (parsed.hmac) {
            const { hmac, ...metadataWithoutHmac } = parsed;
            const hmacKey = this.getHMACKey(profileId);
            const dataToVerify = JSON.stringify(metadataWithoutHmac);
            const expectedHmac = crypto.createHmac('sha256', hmacKey)
                .update(dataToVerify)
                .digest('hex');
            
            if (hmac !== expectedHmac) {
                console.error('❌ [SECURITY] Folders metadata HMAC verification failed!');
                throw new Error('Folders metadata integrity check failed - file may have been tampered with');
            }
            console.log('✅ [SECURITY] Folders metadata HMAC verified');
        }
        
        this.foldersMetadata.set(profileId, parsed);
        return parsed;
    }
    
    /**
     * Save folders metadata with HMAC protection
     * PRIORITY 2 SECURITY: Prevents tampering
     */
    saveFoldersMetadata(profileId) {
        const metadata = this.foldersMetadata.get(profileId);
        if (!metadata)
            return;
        
        metadata.lastModified = Date.now();
        
        // PRIORITY 2: Add HMAC for integrity
        const { hmac, ...metadataWithoutHmac } = metadata;
        const hmacKey = this.getHMACKey(profileId);
        const dataToSign = JSON.stringify(metadataWithoutHmac);
        const hmacValue = crypto.createHmac('sha256', hmacKey)
            .update(dataToSign)
            .digest('hex');
        
        const protectedMetadata = {
            ...metadata,
            hmac: hmacValue
        };
        
        const metadataPath = this.getFoldersMetadataPath(profileId);
        fs.writeFileSync(metadataPath, JSON.stringify(protectedMetadata, null, 2), { mode: 0o600 });
    }
    /**
     * Add folder to vault
     */
    addFolder(profileId, folderPath, customPassword) {
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
        const vaultId = `vault_${Date.now()}_${crypto.randomBytes(4).toString('hex')}`;
        const vault = {
            id: vaultId,
            folderPath,
            folderName,
            isLocked: false,
            usesMasterPassword: !customPassword,
            createdAt: Date.now(),
            unlockMode: null,
            // PHASE 4.2: Vault storage tracking
            originalPath: folderPath,           // Where folder came from
            vaultPath: null,                    // Where it's stored when locked (set during lock)
            backups: [],                        // Array of {timestamp, path} for backups
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
            vault.encryptedCustomRecoveryKey = this.encryptRecoveryKey(recoveryKey, customPassword);
        }
        metadata.folders.push(vault);
        this.saveFoldersMetadata(profileId);
        return vault;
    }
    /**
     * Hash password (same as VaultProfileManager)
     */
    hashPassword(password, salt) {
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
    encryptRecoveryKey(recoveryKey, password) {
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
    getFolders(profileId) {
        let metadata = this.foldersMetadata.get(profileId);
        if (!metadata) {
            metadata = this.loadFoldersMetadata(profileId);
        }
        return metadata.folders;
    }
    /**
     * Get folder by ID
     */
    getFolder(profileId, vaultId) {
        const folders = this.getFolders(profileId);
        return folders.find((f) => f.id === vaultId) || null;
    }
    /**
     * Lock folder (encrypt + hide it)
     * Phase 4: Uses native C++ encryption and filesystem operations
     */
    async lockFolder(profileId, vaultId) {
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
            let password;
            if (folder.usesMasterPassword) {
                // Use master password (need to get it from profile manager)
                // For now, we'll need to pass it in or store it temporarily
                // This is a limitation we'll address in the UI integration
                throw new Error('Master password must be provided for locking');
            }
            else if (folder.customPasswordHash) {
                // For custom password folders, we need the password
                // This should be handled by the UI prompting the user
                throw new Error('Custom password must be provided for locking');
            }
            else {
                throw new Error('No password configured for folder');
            }
            // Note: In production, the UI will call lockFolderWithPassword instead
        }
        catch (error) {
            console.error('Lock folder error:', error);
            throw error;
        }
    }
    /**
     * Lock folder with password (Phase 4: Native C++ implementation)
     */
    async lockFolderWithPassword(profileId, vaultId, password) {
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
            if (this.vaultAddon) {
                console.log(`[Phase 4] Encrypting folder with C++ core: ${folder.folderPath}`);
                // Step 1: Encrypt folder using C++ (AES-256-GCM)
                const encryptResult = this.vaultAddon.encryptFolder(folder.folderPath, password);
                if (!encryptResult.success) {
                    throw new Error(`Encryption failed: ${encryptResult.error}`);
                }
                console.log(`[Phase 4] ✅ Folder encrypted: ${folder.folderPath}`);
                // Step 2: Hide folder using C++ (dot prefix)
                const hideResult = this.vaultAddon.hideFolder(folder.folderPath);
                if (!hideResult.success) {
                    throw new Error(`Hide failed: ${hideResult.error}`);
                }
                // Update folder path to hidden path
                folder.folderPath = hideResult.newPath;
                console.log(`[Phase 4] ✅ Folder hidden: ${hideResult.newPath}`);
            }
            else {
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
        }
        catch (error) {
            console.error('Lock folder error:', error);
            throw error;
        }
    }
    /**
     * Fallback lock implementation (JavaScript)
     */
    lockFolderFallback(folder) {
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
     * Phase 4.2: Restores from vault to original location
     */
    async unlockFolder(profileId, vaultId, password, mode) {
        const metadata = this.foldersMetadata.get(profileId);
        if (!metadata)
            return false;
        const folder = metadata.folders.find((f) => f.id === vaultId);
        if (!folder)
            return false;
        if (!folder.isLocked) {
            return true; // Already unlocked
        }
        
        // Verify password
        const profile = this.profileManager.getActiveProfile();
        if (!profile)
            return false;
        let passwordValid = false;
        if (folder.usesMasterPassword) {
            // Use master password
            passwordValid = this.profileManager.verifyProfilePassword(profileId, password);
        }
        else if (folder.customPasswordHash) {
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
            // PHASE 4.2: ONLY works with new vault system
            // Skip folders without vault metadata (old system)
            if (!folder.vaultPath || !folder.originalPath) {
                console.log(`[Phase 4.2] ⚠️  Skipping folder without vault metadata (old system): ${folder.folderName}`);
                console.log(`   This folder needs to be unlocked manually and re-added to use the new vault system.`);
                return false;
            }
            
            const vaultPath = folder.vaultPath;
            const originalPath = folder.originalPath;
            
            console.log(`[Phase 4.2] Unlocking folder from vault`);
            console.log(`   Vault path: ${vaultPath}`);
            console.log(`   Original path: ${originalPath}`);
            
            // Verify vault folder exists
            if (!fs.existsSync(vaultPath)) {
                console.error(`   ❌ Vault folder not found: ${vaultPath}`);
                throw new Error(`Vault folder not found: ${vaultPath}`);
            }
            
            // Step 1: Create backup before unlock
            try {
                console.log(`   [STEP 1] Creating pre-unlock backup...`);
                const backupPath = this.getBackupPath(profileId, vaultId, folder.folderName);
                this.createBackup(vaultPath, backupPath);
                
                // Track backup in metadata
                if (!folder.backups) folder.backups = [];
                folder.backups.push({
                    timestamp: Date.now(),
                    path: backupPath,
                    operation: 'pre-unlock'
                });
                this.saveFoldersMetadata(profileId);
            } catch (backupError) {
                console.error(`   ❌ Pre-unlock backup failed: ${backupError.message}`);
                throw new Error(`Cannot unlock without backup: ${backupError.message}`);
            }
            
            // Step 2: Move from vault to original location
            try {
                console.log(`   [STEP 2] Moving from vault to original location...`);
                this.moveFromVault(vaultPath, originalPath);
            } catch (moveError) {
                console.error(`   ❌ Failed to restore from vault: ${moveError.message}`);
                
                // Rollback: Remove backup since we didn't proceed
                try {
                    const lastBackup = folder.backups[folder.backups.length - 1];
                    if (lastBackup && fs.existsSync(lastBackup.path)) {
                        fs.rmSync(lastBackup.path, { recursive: true, force: true });
                    }
                    folder.backups.pop();
                    this.saveFoldersMetadata(profileId);
                } catch (rollbackError) {
                    console.error(`   ⚠️  Rollback warning: ${rollbackError.message}`);
                }
                
                throw new Error(`Failed to restore from vault: ${moveError.message}`);
            }
            
            // Step 3: Decrypt folder using C++ addon
            if (this.vaultAddon) {
                try {
                    console.log(`   [STEP 3] Decrypting folder contents...`);
                    // C++ decryptFolder returns boolean or throws exception
                    const decryptSuccess = this.vaultAddon.decryptFolder(originalPath, password);
                    if (!decryptSuccess) {
                        throw new Error('Decryption returned false');
                    }
                    console.log(`   ✅ Folder decrypted successfully`);
                } catch (decryptError) {
                    console.error(`   ❌ Decryption failed: ${decryptError.message}`);
                    
                    // Rollback: Move back to vault
                    console.log(`   🔄 Rolling back: moving folder back to vault...`);
                    try {
                        this.moveToVault(originalPath, vaultPath);
                    } catch (rollbackMoveError) {
                        console.error(`   ⚠️  Rollback move failed: ${rollbackMoveError.message}`);
                    }
                    
                    throw new Error(`Decryption failed: ${decryptError.message}`);
                }
            } else {
                console.warn(`   ⚠️  [STEP 3] Native addon not available, skipping decryption`);
            }
            
            // Step 4: Update metadata
            folder.isLocked = false;
            folder.folderPath = originalPath;
            folder.lastUnlocked = Date.now();
            folder.unlockMode = mode;
            folder.vaultPath = null; // Clear vault path when unlocked
            
            // Track unlock state in memory
            this.unlockStates.set(folder.id, {
                mode,
                unlockedAt: new Date(),
                profileId,
                folderPath: originalPath,
            });
            
            // Notify callback (for AutoLockManager)
            if (this.onUnlockCallback) {
                this.onUnlockCallback(folder.id, profileId, mode, originalPath);
            }
            
            // If permanent unlock, remove from vault
            if (mode === 'permanent') {
                const index = metadata.folders.indexOf(folder);
                if (index > -1) {
                    metadata.folders.splice(index, 1);
                }
            }
            
            this.saveFoldersMetadata(profileId);
            
            // Step 5: Clean old backups
            try {
                console.log(`   [STEP 4] Cleaning old backups...`);
                this.cleanOldBackups(profileId, vaultId, folder.folderName, 3);
            } catch (cleanupError) {
                console.warn(`   ⚠️  Backup cleanup warning: ${cleanupError.message}`);
            }
            
            console.log(`   ✅ [Phase 4.2] Folder unlocked and restored successfully`);
            return true;
        }
        catch (error) {
            console.error('Unlock folder error:', error);
            return false;
        }
    }
    /**
     * Fallback unlock implementation (JavaScript)
     */
    unlockFolderFallback(folder) {
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
    async unlockAllFolders(profileId, password, mode) {
        const folders = this.getFolders(profileId);
        let success = 0;
        let failed = 0;
        for (const folder of folders) {
            if (folder.isLocked && folder.usesMasterPassword) {
                const unlocked = await this.unlockFolder(profileId, folder.id, password, mode);
                if (unlocked) {
                    success++;
                }
                else {
                    failed++;
                }
            }
        }
        return { success, failed };
    }
    /**
     * Remove folder from vault (permanent unlock)
     */
    removeFolder(profileId, vaultId) {
        const metadata = this.foldersMetadata.get(profileId);
        if (!metadata)
            return;
        const index = metadata.folders.findIndex((f) => f.id === vaultId);
        if (index > -1) {
            metadata.folders.splice(index, 1);
            this.saveFoldersMetadata(profileId);
        }
    }
    /**
     * Lock all temporary folders (for auto-lock)
     */
    lockTemporaryFolders(profileId) {
        const metadata = this.foldersMetadata.get(profileId);
        if (!metadata)
            return;
        for (const folder of metadata.folders) {
            if (!folder.isLocked && folder.unlockMode === 'temporary') {
                this.lockFolder(profileId, folder.id);
            }
        }
    }
    /**
     * Get all temporarily unlocked folders
     */
    getTemporaryUnlockedFolders(profileId) {
        const metadata = this.foldersMetadata.get(profileId);
        if (!metadata)
            return [];
        return metadata.folders.filter(f => !f.isLocked && f.unlockMode === 'temporary');
    }
    /**
     * Register callback for unlock events
     */
    onUnlock(callback) {
        this.onUnlockCallback = callback;
    }
    /**
     * Register callback for lock events
     */
    onLock(callback) {
        this.onLockCallback = callback;
    }
    /**
     * Get unlock state for a folder
     */
    getUnlockState(folderId) {
        return this.unlockStates.get(folderId);
    }
    /**
     * Get all unlock states
     */
    getAllUnlockStates() {
        return Array.from(this.unlockStates.entries()).map(([id, state]) => ({
            folderId: id,
            ...state,
        }));
    }
    /**
     * Update folder metadata after lock/unlock from main process
     * This is called from IPC handlers that directly manipulate folders
     * 
     * PHASE 4.2: Now handles vault path tracking
     */
    updateFolderLockState(profileId, folderId, isLocked, newFolderPath, isInVault = false) {
        const metadata = this.foldersMetadata.get(profileId);
        if (!metadata) {
            throw new Error('Profile metadata not found');
        }
        const folder = metadata.folders.find((f) => f.id === folderId);
        if (!folder) {
            throw new Error('Folder not found');
        }
        
        folder.isLocked = isLocked;
        
        if (isLocked) {
            // Folder is being locked - it should be in vault
            folder.vaultPath = newFolderPath;
            folder.folderPath = newFolderPath; // Keep backward compatibility
            folder.unlockMode = null;
            this.unlockStates.delete(folderId);
        } else {
            // Folder is being unlocked - it should be at original location
            folder.folderPath = newFolderPath || folder.originalPath;
            folder.lastUnlocked = Date.now();
        }
        
        this.saveFoldersMetadata(profileId);
    }
    
    // ============================================================================
    // PHASE 4.2: SECURE VAULT STORAGE
    // ============================================================================
    
    /**
     * Get vault path for a folder (where encrypted folder is stored)
     */
    getVaultPath(profileId, folderId, folderName) {
        const userVaultPath = this.profileManager.getUserVaultPath();
        const vaultFolderName = `${folderName.replace(/[^a-zA-Z0-9_-]/g, '_')}_vault_${folderId}`;
        return path.join(userVaultPath, 'vaults', vaultFolderName);
    }
    
    /**
     * Get backup path for a folder
     */
    getBackupPath(profileId, folderId, folderName) {
        const userVaultPath = this.profileManager.getUserVaultPath();
        const timestamp = Date.now();
        const backupFolderName = `${folderName.replace(/[^a-zA-Z0-9_-]/g, '_')}_backup_${timestamp}`;
        return path.join(userVaultPath, 'backups', backupFolderName);
    }
    
    /**
     * Move folder to vault (recursively)
     */
    moveToVault(sourcePath, vaultPath) {
        try {
            console.log(`   [VAULT] Moving to vault: ${sourcePath} → ${vaultPath}`);
            
            // Create vault parent directory if needed
            const vaultParent = path.dirname(vaultPath);
            if (!fs.existsSync(vaultParent)) {
                fs.mkdirSync(vaultParent, { recursive: true, mode: 0o755 });
            }
            
            // Move folder to vault
            fs.renameSync(sourcePath, vaultPath);
            
            // Verify move succeeded
            if (!fs.existsSync(vaultPath)) {
                throw new Error('Vault folder not found after move');
            }
            if (fs.existsSync(sourcePath)) {
                throw new Error('Source folder still exists after move');
            }
            
            console.log(`   ✅ [VAULT] Folder moved to vault successfully`);
        } catch (error) {
            console.error(`   ❌ [VAULT] Failed to move to vault:`, error);
            throw error;
        }
    }
    
    /**
     * Move folder from vault back to original location
     */
    moveFromVault(vaultPath, targetPath) {
        try {
            console.log(`   [VAULT] Restoring from vault: ${vaultPath} → ${targetPath}`);
            
            // Verify vault folder exists
            if (!fs.existsSync(vaultPath)) {
                throw new Error(`Vault folder not found: ${vaultPath}`);
            }
            
            // Create target parent directory if needed
            const targetParent = path.dirname(targetPath);
            if (!fs.existsSync(targetParent)) {
                fs.mkdirSync(targetParent, { recursive: true });
            }
            
            // Move folder from vault to original location
            fs.renameSync(vaultPath, targetPath);
            
            // Verify move succeeded
            if (!fs.existsSync(targetPath)) {
                throw new Error('Target folder not found after restore');
            }
            if (fs.existsSync(vaultPath)) {
                throw new Error('Vault folder still exists after restore');
            }
            
            console.log(`   ✅ [VAULT] Folder restored from vault successfully`);
        } catch (error) {
            console.error(`   ❌ [VAULT] Failed to restore from vault:`, error);
            throw error;
        }
    }
    
    /**
     * Create backup of folder
     */
    createBackup(sourcePath, backupPath) {
        try {
            console.log(`   [BACKUP] Creating backup: ${sourcePath} → ${backupPath}`);
            
            // Create backup parent directory if needed
            const backupParent = path.dirname(backupPath);
            if (!fs.existsSync(backupParent)) {
                fs.mkdirSync(backupParent, { recursive: true, mode: 0o700 });
            }
            
            // Copy folder recursively
            this.copyRecursive(sourcePath, backupPath);
            
            console.log(`   ✅ [BACKUP] Backup created successfully`);
        } catch (error) {
            console.error(`   ❌ [BACKUP] Failed to create backup:`, error);
            throw error;
        }
    }
    
    /**
     * Helper: Copy folder recursively
     */
    copyRecursive(src, dest) {
        const stats = fs.statSync(src);
        
        if (stats.isDirectory()) {
            // Create directory
            if (!fs.existsSync(dest)) {
                fs.mkdirSync(dest, { recursive: true });
            }
            
            // Copy all contents
            const entries = fs.readdirSync(src);
            for (const entry of entries) {
                const srcPath = path.join(src, entry);
                const destPath = path.join(dest, entry);
                this.copyRecursive(srcPath, destPath);
            }
        } else {
            // Copy file
            fs.copyFileSync(src, dest);
        }
    }
    
    /**
     * Clean old backups (keep only last N backups)
     */
    cleanOldBackups(profileId, folderId, folderName, keepLast = 3) {
        try {
            const userVaultPath = this.profileManager.getUserVaultPath();
            const backupsPath = path.join(userVaultPath, 'backups');
            
            if (!fs.existsSync(backupsPath)) {
                return;
            }
            
            // Find all backups for this folder
            const prefix = `${folderName.replace(/[^a-zA-Z0-9_-]/g, '_')}_backup_`;
            const backups = fs.readdirSync(backupsPath)
                .filter(name => name.startsWith(prefix))
                .map(name => ({
                    name,
                    path: path.join(backupsPath, name),
                    timestamp: parseInt(name.split('_backup_')[1]) || 0
                }))
                .sort((a, b) => b.timestamp - a.timestamp); // Newest first
            
            // Delete old backups
            const toDelete = backups.slice(keepLast);
            for (const backup of toDelete) {
                console.log(`   [BACKUP] Cleaning old backup: ${backup.name}`);
                fs.rmSync(backup.path, { recursive: true, force: true });
            }
            
            if (toDelete.length > 0) {
                console.log(`   ✅ [BACKUP] Cleaned ${toDelete.length} old backup(s), kept ${keepLast} recent`);
            }
        } catch (error) {
            console.error(`   ⚠️  [BACKUP] Failed to clean old backups:`, error.message);
            // Don't throw - backup cleanup is not critical
        }
    }
}
exports.VaultFolderManager = VaultFolderManager;
