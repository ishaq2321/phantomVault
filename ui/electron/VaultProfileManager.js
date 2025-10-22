"use strict";
/**
 * VaultProfileManager - Cross-platform vault profile management
 * Handles OS user isolation and multi-profile support
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
exports.VaultProfileManager = void 0;
const os = __importStar(require("os"));
const path = __importStar(require("path"));
const fs = __importStar(require("fs"));
const crypto = __importStar(require("crypto"));
class VaultProfileManager {
    constructor() {
        this.metadata = null;
        this.currentOsUser = os.userInfo().username;
        // Cross-platform vault storage location
        if (process.platform === 'win32') {
            // Windows: %APPDATA%/PhantomVault
            this.vaultBasePath = path.join(process.env.APPDATA || path.join(os.homedir(), 'AppData', 'Roaming'), 'PhantomVault');
        }
        else {
            // Linux/macOS/Tails: ~/.phantom_vault_storage
            this.vaultBasePath = path.join(os.homedir(), '.phantom_vault_storage');
        }
        this.ensureVaultDirectoryExists();
    }
    static getInstance() {
        if (!VaultProfileManager.instance) {
            VaultProfileManager.instance = new VaultProfileManager();
        }
        return VaultProfileManager.instance;
    }
    /**
     * Initialize vault directory structure
     */
    ensureVaultDirectoryExists() {
        if (!fs.existsSync(this.vaultBasePath)) {
            fs.mkdirSync(this.vaultBasePath, { recursive: true, mode: 0o700 });
        }
        // Create OS user-specific directory
        const userVaultPath = this.getUserVaultPath();
        if (!fs.existsSync(userVaultPath)) {
            fs.mkdirSync(userVaultPath, { recursive: true, mode: 0o700 });
        }
        
        // PHASE 4.2: Create vaults and backups directories
        const vaultsPath = path.join(userVaultPath, 'vaults');
        const backupsPath = path.join(userVaultPath, 'backups');
        
        if (!fs.existsSync(vaultsPath)) {
            fs.mkdirSync(vaultsPath, { recursive: true, mode: 0o700 });
            console.log(`✅ Created vaults directory: ${vaultsPath} (mode: 0o700 - owner only)`);
        }
        
        if (!fs.existsSync(backupsPath)) {
            fs.mkdirSync(backupsPath, { recursive: true, mode: 0o700 });
            console.log(`✅ Created backups directory: ${backupsPath}`);
        }
    }
    /**
     * Get vault path for current OS user
     */
    getUserVaultPath() {
        return path.join(this.vaultBasePath, this.currentOsUser);
    }
    /**
     * Get metadata file path
     */
    getMetadataPath() {
        return path.join(this.getUserVaultPath(), 'vault_metadata.json');
    }
    /**
     * Load vault metadata with encryption and HMAC integrity check
     * PRIORITY 2 SECURITY: Encrypted + tamper-proof metadata
     */
    loadMetadata() {
        const metadataPath = this.getMetadataPath();
        if (!fs.existsSync(metadataPath)) {
            // First time setup - create default metadata
            this.metadata = {
                version: '2.0.0',
                profiles: [],
                activeProfileId: null,
            };
            this.saveMetadata();
        }
        else {
            const rawData = fs.readFileSync(metadataPath, 'utf-8');
            const parsed = JSON.parse(rawData);
            
            // PRIORITY 2: Check if encrypted
            let metadata;
            if (parsed.encrypted && parsed.iv && parsed.authTag && parsed.data) {
                console.log('🔐 [SECURITY] Loading encrypted metadata');
                try {
                    metadata = this.decryptMetadata(parsed);
                } catch (error) {
                    console.error('❌ [SECURITY] Failed to decrypt metadata:', error.message);
                    throw new Error('Failed to decrypt metadata - encryption key may have changed');
                }
            } else {
                // Legacy unencrypted metadata
                console.warn('⚠️  [SECURITY] Loading unencrypted metadata (will upgrade on save)');
                metadata = parsed;
            }
            
            // PRIORITY 2: Verify HMAC if present
            if (metadata.hmac) {
                const { hmac, ...metadataWithoutHmac } = metadata;
                const hmacKey = this.getHMACKey();
                const dataToVerify = JSON.stringify(metadataWithoutHmac);
                const expectedHmac = crypto.createHmac('sha256', hmacKey)
                    .update(dataToVerify)
                    .digest('hex');
                
                if (hmac !== expectedHmac) {
                    throw new Error('Metadata integrity check failed - file may have been tampered with');
                }
                console.log('✅ [SECURITY] Metadata HMAC verified');
            }
            
            this.metadata = metadata;
        }
        return this.metadata;
    }
    
    /**
     * Get HMAC key for metadata integrity
     * Uses machine-specific identifier
     */
    getHMACKey() {
        const os = require('os');
        // Use machine hostname + username as HMAC key material
        const keyMaterial = `${os.hostname()}-${os.userInfo().username}`;
        return crypto.createHash('sha256').update(keyMaterial).digest();
    }
    
    /**
     * Get encryption key for metadata
     * PRIORITY 2 SECURITY: Derives key from machine-specific data
     */
    getMetadataEncryptionKey() {
        const os = require('os');
        // Derive key from multiple machine-specific identifiers
        const keyMaterial = [
            os.hostname(),
            os.userInfo().username,
            os.platform(),
            os.arch(),
            'phantom_vault_metadata_v2'
        ].join('::');
        
        // Use PBKDF2 to derive a strong key
        return crypto.pbkdf2Sync(keyMaterial, 'metadata_salt_v2', 10000, 32, 'sha256');
    }
    
    /**
     * Encrypt metadata
     * PRIORITY 2 SECURITY: AES-256-GCM encryption
     */
    encryptMetadata(data) {
        const key = this.getMetadataEncryptionKey();
        const iv = crypto.randomBytes(12); // 96-bit IV for GCM
        const cipher = crypto.createCipheriv('aes-256-gcm', key, iv);
        
        let encrypted = cipher.update(JSON.stringify(data), 'utf8', 'hex');
        encrypted += cipher.final('hex');
        
        const authTag = cipher.getAuthTag();
        
        return {
            iv: iv.toString('hex'),
            authTag: authTag.toString('hex'),
            data: encrypted
        };
    }
    
    /**
     * Decrypt metadata
     * PRIORITY 2 SECURITY: AES-256-GCM decryption
     */
    decryptMetadata(encrypted) {
        const key = this.getMetadataEncryptionKey();
        const decipher = crypto.createDecipheriv(
            'aes-256-gcm',
            key,
            Buffer.from(encrypted.iv, 'hex')
        );
        
        decipher.setAuthTag(Buffer.from(encrypted.authTag, 'hex'));
        
        let decrypted = decipher.update(encrypted.data, 'hex', 'utf8');
        decrypted += decipher.final('utf8');
        
        return JSON.parse(decrypted);
    }
    
    /**
     * Save vault metadata with encryption and HMAC integrity protection
     * PRIORITY 2 SECURITY: Encrypted + tamper-proof metadata
     */
    saveMetadata() {
        if (!this.metadata)
            return;
        
        const metadataPath = this.getMetadataPath();
        
        // PRIORITY 2: Add HMAC for integrity
        const { hmac, ...metadataWithoutHmac } = this.metadata;
        const hmacKey = this.getHMACKey();
        const dataToSign = JSON.stringify(metadataWithoutHmac);
        const hmacValue = crypto.createHmac('sha256', hmacKey)
            .update(dataToSign)
            .digest('hex');
        
        const protectedMetadata = {
            ...this.metadata,
            hmac: hmacValue
        };
        
        // PRIORITY 2: Encrypt metadata
        console.log('🔐 [SECURITY] Encrypting metadata before save');
        const encrypted = this.encryptMetadata(protectedMetadata);
        const finalData = {
            encrypted: true,
            version: '2.0.0',
            ...encrypted
        };
        
        fs.writeFileSync(metadataPath, JSON.stringify(finalData, null, 2), { mode: 0o600 });
    }
    /**
     * Check if this is first-time setup
     */
    isFirstTimeSetup() {
        const metadata = this.loadMetadata();
        return metadata.profiles.length === 0;
    }
    /**
     * Generate secure recovery key
     */
    generateRecoveryKey() {
        const segments = [];
        for (let i = 0; i < 4; i++) {
            const bytes = crypto.randomBytes(2);
            const segment = bytes.toString('hex').toUpperCase();
            segments.push(segment);
        }
        return segments.join('-'); // Format: XXXX-XXXX-XXXX-XXXX
    }
    /**
     * Hash password with PBKDF2
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
     * Verify password against hash
     */
    verifyPassword(password, storedHash) {
        const [saltHex, hashHex] = storedHash.split(':');
        const salt = Buffer.from(saltHex, 'hex');
        const { hash } = this.hashPassword(password, salt);
        return hash === hashHex;
    }
    /**
     * Encrypt recovery key with master password
     */
    encryptRecoveryKey(recoveryKey, masterPassword) {
        const key = crypto.scryptSync(masterPassword, 'salt', 32);
        const iv = crypto.randomBytes(16);
        const cipher = crypto.createCipheriv('aes-256-cbc', key, iv);
        let encrypted = cipher.update(recoveryKey, 'utf8', 'hex');
        encrypted += cipher.final('hex');
        return iv.toString('hex') + ':' + encrypted;
    }
    /**
     * Decrypt recovery key with master password
     */
    decryptRecoveryKey(encryptedKey, masterPassword) {
        try {
            const [ivHex, encryptedHex] = encryptedKey.split(':');
            const key = crypto.scryptSync(masterPassword, 'salt', 32);
            const iv = Buffer.from(ivHex, 'hex');
            const decipher = crypto.createDecipheriv('aes-256-cbc', key, iv);
            let decrypted = decipher.update(encryptedHex, 'hex', 'utf8');
            decrypted += decipher.final('utf8');
            return decrypted;
        }
        catch (error) {
            throw new Error('Invalid master password');
        }
    }
    /**
     * Create new vault profile
     */
    createProfile(profileName, masterPassword) {
        const recoveryKey = this.generateRecoveryKey();
        const { hash, salt } = this.hashPassword(masterPassword);
        const masterPasswordHash = `${salt}:${hash}`;
        // Hash recovery key for verification
        const { hash: recoveryHash, salt: recoverySalt } = this.hashPassword(recoveryKey);
        const recoveryKeyHash = `${recoverySalt}:${recoveryHash}`;
        // Encrypt recovery key with master password
        const encryptedRecoveryKey = this.encryptRecoveryKey(recoveryKey, masterPassword);
        const profile = {
            id: `profile_${Date.now()}_${crypto.randomBytes(4).toString('hex')}`,
            name: profileName,
            createdAt: Date.now(),
            lastAccess: Date.now(),
            osUser: this.currentOsUser,
            masterPasswordHash,
            recoveryKeyHash,
            encryptedRecoveryKey,
        };
        // Update metadata
        if (!this.metadata) {
            this.loadMetadata();
        }
        this.metadata.profiles.push(profile);
        this.metadata.activeProfileId = profile.id;
        this.saveMetadata();
        // Create profile directory
        const profilePath = this.getProfilePath(profile.id);
        if (!fs.existsSync(profilePath)) {
            fs.mkdirSync(profilePath, { recursive: true, mode: 0o700 });
        }
        return { profile, recoveryKey };
    }
    /**
     * Get profile directory path
     */
    getProfilePath(profileId) {
        return path.join(this.getUserVaultPath(), profileId);
    }
    /**
     * Get all profiles for current OS user
     */
    getProfiles() {
        if (!this.metadata) {
            this.loadMetadata();
        }
        return this.metadata.profiles;
    }
    /**
     * Get active profile
     */
    getActiveProfile() {
        if (!this.metadata) {
            this.loadMetadata();
        }
        if (!this.metadata.activeProfileId) {
            return null;
        }
        return (this.metadata.profiles.find((p) => p.id === this.metadata.activeProfileId) || null);
    }
    /**
     * Set active profile
     */
    setActiveProfile(profileId) {
        if (!this.metadata) {
            this.loadMetadata();
        }
        const profile = this.metadata.profiles.find((p) => p.id === profileId);
        if (!profile) {
            throw new Error('Profile not found');
        }
        this.metadata.activeProfileId = profileId;
        profile.lastAccess = Date.now();
        this.saveMetadata();
    }
    /**
     * Verify master password for profile
     */
    verifyProfilePassword(profileId, password) {
        if (!this.metadata) {
            this.loadMetadata();
        }
        const profile = this.metadata.profiles.find((p) => p.id === profileId);
        if (!profile) {
            return false;
        }
        return this.verifyPassword(password, profile.masterPasswordHash);
    }
    /**
     * Get vault base path (for debugging)
     */
    getVaultBasePath() {
        return this.vaultBasePath;
    }
    /**
     * Get current OS user
     */
    getCurrentOsUser() {
        return this.currentOsUser;
    }
}
exports.VaultProfileManager = VaultProfileManager;
