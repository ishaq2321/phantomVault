/**
 * VaultProfileManager - Cross-platform vault profile management
 * Handles OS user isolation and multi-profile support
 */

import * as os from 'os';
import * as path from 'path';
import * as fs from 'fs';
import * as crypto from 'crypto';

export interface VaultProfile {
  id: string;
  name: string;
  createdAt: number;
  lastAccess: number;
  osUser: string;
  masterPasswordHash: string;
  recoveryKeyHash: string;
  encryptedRecoveryKey: string; // Encrypted with master password
}

export interface VaultMetadata {
  version: string;
  profiles: VaultProfile[];
  activeProfileId: string | null;
}

export class VaultProfileManager {
  private static instance: VaultProfileManager;
  private vaultBasePath: string;
  private currentOsUser: string;
  private metadata: VaultMetadata | null = null;

  private constructor() {
    this.currentOsUser = os.userInfo().username;
    
    // Cross-platform vault storage location
    if (process.platform === 'win32') {
      // Windows: %APPDATA%/PhantomVault
      this.vaultBasePath = path.join(
        process.env.APPDATA || path.join(os.homedir(), 'AppData', 'Roaming'),
        'PhantomVault'
      );
    } else {
      // Linux/macOS/Tails: ~/.phantom_vault_storage
      this.vaultBasePath = path.join(os.homedir(), '.phantom_vault_storage');
    }

    this.ensureVaultDirectoryExists();
  }

  public static getInstance(): VaultProfileManager {
    if (!VaultProfileManager.instance) {
      VaultProfileManager.instance = new VaultProfileManager();
    }
    return VaultProfileManager.instance;
  }

  /**
   * Initialize vault directory structure
   */
  private ensureVaultDirectoryExists(): void {
    if (!fs.existsSync(this.vaultBasePath)) {
      fs.mkdirSync(this.vaultBasePath, { recursive: true, mode: 0o700 });
    }

    // Create OS user-specific directory
    const userVaultPath = this.getUserVaultPath();
    if (!fs.existsSync(userVaultPath)) {
      fs.mkdirSync(userVaultPath, { recursive: true, mode: 0o700 });
    }
  }

  /**
   * Get vault path for current OS user
   */
  private getUserVaultPath(): string {
    return path.join(this.vaultBasePath, this.currentOsUser);
  }

  /**
   * Get metadata file path
   */
  private getMetadataPath(): string {
    return path.join(this.getUserVaultPath(), 'vault_metadata.json');
  }

  /**
   * Load vault metadata
   */
  public loadMetadata(): VaultMetadata {
    const metadataPath = this.getMetadataPath();

    if (!fs.existsSync(metadataPath)) {
      // First time setup - create default metadata
      this.metadata = {
        version: '2.0.0',
        profiles: [],
        activeProfileId: null,
      };
      this.saveMetadata();
    } else {
      const data = fs.readFileSync(metadataPath, 'utf-8');
      this.metadata = JSON.parse(data);
    }

    return this.metadata!;
  }

  /**
   * Save vault metadata
   */
  private saveMetadata(): void {
    if (!this.metadata) return;

    const metadataPath = this.getMetadataPath();
    fs.writeFileSync(
      metadataPath,
      JSON.stringify(this.metadata, null, 2),
      { mode: 0o600 }
    );
  }

  /**
   * Check if this is first-time setup
   */
  public isFirstTimeSetup(): boolean {
    const metadata = this.loadMetadata();
    return metadata.profiles.length === 0;
  }

  /**
   * Generate secure recovery key
   */
  public generateRecoveryKey(): string {
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
  private hashPassword(password: string, salt?: Buffer): { hash: string; salt: string } {
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
  public verifyPassword(password: string, storedHash: string): boolean {
    const [saltHex, hashHex] = storedHash.split(':');
    const salt = Buffer.from(saltHex, 'hex');
    const { hash } = this.hashPassword(password, salt);
    return hash === hashHex;
  }

  /**
   * Encrypt recovery key with master password
   */
  private encryptRecoveryKey(recoveryKey: string, masterPassword: string): string {
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
  public decryptRecoveryKey(encryptedKey: string, masterPassword: string): string {
    try {
      const [ivHex, encryptedHex] = encryptedKey.split(':');
      const key = crypto.scryptSync(masterPassword, 'salt', 32);
      const iv = Buffer.from(ivHex, 'hex');
      const decipher = crypto.createDecipheriv('aes-256-cbc', key, iv);
      
      let decrypted = decipher.update(encryptedHex, 'hex', 'utf8');
      decrypted += decipher.final('utf8');
      
      return decrypted;
    } catch (error) {
      throw new Error('Invalid master password');
    }
  }

  /**
   * Create new vault profile
   */
  public createProfile(
    profileName: string,
    masterPassword: string
  ): { profile: VaultProfile; recoveryKey: string } {
    const recoveryKey = this.generateRecoveryKey();
    const { hash, salt } = this.hashPassword(masterPassword);
    const masterPasswordHash = `${salt}:${hash}`;

    // Hash recovery key for verification
    const { hash: recoveryHash, salt: recoverySalt } = this.hashPassword(recoveryKey);
    const recoveryKeyHash = `${recoverySalt}:${recoveryHash}`;

    // Encrypt recovery key with master password
    const encryptedRecoveryKey = this.encryptRecoveryKey(recoveryKey, masterPassword);

    const profile: VaultProfile = {
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

    this.metadata!.profiles.push(profile);
    this.metadata!.activeProfileId = profile.id;
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
  public getProfilePath(profileId: string): string {
    return path.join(this.getUserVaultPath(), profileId);
  }

  /**
   * Get all profiles for current OS user
   */
  public getProfiles(): VaultProfile[] {
    if (!this.metadata) {
      this.loadMetadata();
    }
    return this.metadata!.profiles;
  }

  /**
   * Get active profile
   */
  public getActiveProfile(): VaultProfile | null {
    if (!this.metadata) {
      this.loadMetadata();
    }

    if (!this.metadata!.activeProfileId) {
      return null;
    }

    return (
      this.metadata!.profiles.find(
        (p) => p.id === this.metadata!.activeProfileId
      ) || null
    );
  }

  /**
   * Set active profile
   */
  public setActiveProfile(profileId: string): void {
    if (!this.metadata) {
      this.loadMetadata();
    }

    const profile = this.metadata!.profiles.find((p) => p.id === profileId);
    if (!profile) {
      throw new Error('Profile not found');
    }

    this.metadata!.activeProfileId = profileId;
    profile.lastAccess = Date.now();
    this.saveMetadata();
  }

  /**
   * Verify master password for profile
   */
  public verifyProfilePassword(profileId: string, password: string): boolean {
    if (!this.metadata) {
      this.loadMetadata();
    }

    const profile = this.metadata!.profiles.find((p) => p.id === profileId);
    if (!profile) {
      return false;
    }

    return this.verifyPassword(password, profile.masterPasswordHash);
  }

  /**
   * Get vault base path (for debugging)
   */
  public getVaultBasePath(): string {
    return this.vaultBasePath;
  }

  /**
   * Get current OS user
   */
  public getCurrentOsUser(): string {
    return this.currentOsUser;
  }
}
