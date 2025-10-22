/**
 * AutoLockManager - Automatically locks temporary folders on system lock/suspend
 * 
 * Features:
 * - Cross-platform system lock detection
 * - Tracks temporary vs permanent unlock states
 * - Auto-locks temporary folders on system lock/suspend/app quit
 * - Preserves permanent unlock state
 * 
 * Platform Support:
 * - Linux: Electron PowerMonitor + systemd-logind DBus
 * - Windows: Electron PowerMonitor
 * - macOS: Electron PowerMonitor
 */

import { powerMonitor, app } from 'electron';
import * as path from 'path';
import * as fs from 'fs';
import * as os from 'os';

interface UnlockInfo {
  folderId: string;
  profileId: string;
  mode: 'temporary' | 'permanent';
  unlockedAt: Date;
  folderPath: string;
}

interface LockResult {
  success: number;
  failed: number;
  errors: string[];
}

export class AutoLockManager {
  private static instance: AutoLockManager | null = null;
  
  // Track temporary unlocks in memory (not persisted)
  private temporaryUnlocks: Map<string, UnlockInfo>;
  
  // Monitoring state
  private isMonitoring: boolean;
  
  // Platform
  private platform: string;
  
  // Callbacks
  private onLockCallback: ((folderId: string) => Promise<void>) | null = null;
  
  private constructor() {
    this.temporaryUnlocks = new Map();
    this.isMonitoring = false;
    this.platform = os.platform();
    
    console.log(`[AutoLockManager] Initialized for platform: ${this.platform}`);
  }
  
  /**
   * Get singleton instance
   */
  public static getInstance(): AutoLockManager {
    if (!AutoLockManager.instance) {
      AutoLockManager.instance = new AutoLockManager();
    }
    return AutoLockManager.instance;
  }
  
  /**
   * Register a folder unlock to track
   * @param folderId - Folder ID
   * @param profileId - Profile ID
   * @param mode - Unlock mode (temporary or permanent)
   * @param folderPath - Physical folder path
   */
  public registerUnlock(
    folderId: string,
    profileId: string,
    mode: 'temporary' | 'permanent',
    folderPath: string
  ): void {
    const unlockInfo: UnlockInfo = {
      folderId,
      profileId,
      mode,
      unlockedAt: new Date(),
      folderPath,
    };
    
    // Only track temporary unlocks
    if (mode === 'temporary') {
      this.temporaryUnlocks.set(folderId, unlockInfo);
      console.log(`[AutoLockManager] Tracking temporary unlock: ${folderId} (${folderPath})`);
      console.log(`[AutoLockManager] Total temporary unlocks: ${this.temporaryUnlocks.size}`);
    } else {
      // Remove from tracking if switching to permanent
      this.temporaryUnlocks.delete(folderId);
      console.log(`[AutoLockManager] Permanent unlock - not tracking: ${folderId}`);
    }
  }
  
  /**
   * Unregister a folder (when manually locked)
   * @param folderId - Folder ID
   */
  public unregisterUnlock(folderId: string): void {
    const wasTracked = this.temporaryUnlocks.has(folderId);
    this.temporaryUnlocks.delete(folderId);
    
    if (wasTracked) {
      console.log(`[AutoLockManager] Unregistered unlock: ${folderId}`);
      console.log(`[AutoLockManager] Remaining temporary unlocks: ${this.temporaryUnlocks.size}`);
    }
  }
  
  /**
   * Get all temporary unlocked folders
   */
  public getTemporaryUnlocks(): UnlockInfo[] {
    return Array.from(this.temporaryUnlocks.values());
  }
  
  /**
   * Get count of temporary unlocked folders
   */
  public getTemporaryUnlockCount(): number {
    return this.temporaryUnlocks.size;
  }
  
  /**
   * Set callback for locking a folder
   * @param callback - Function to lock a folder
   */
  public onLock(callback: (folderId: string) => Promise<void>): void {
    this.onLockCallback = callback;
  }
  
  /**
   * Lock all temporary folders
   * @returns Result of lock operations
   */
  public async lockAllTemporaryFolders(): Promise<LockResult> {
    const result: LockResult = {
      success: 0,
      failed: 0,
      errors: [],
    };
    
    if (this.temporaryUnlocks.size === 0) {
      console.log('[AutoLockManager] No temporary folders to lock');
      return result;
    }
    
    console.log(`[AutoLockManager] Locking ${this.temporaryUnlocks.size} temporary folder(s)...`);
    
    const unlocksCopy = Array.from(this.temporaryUnlocks.values());
    
    for (const unlock of unlocksCopy) {
      try {
        if (this.onLockCallback) {
          await this.onLockCallback(unlock.folderId);
          result.success++;
          console.log(`[AutoLockManager] ✓ Locked: ${unlock.folderPath}`);
        } else {
          console.warn('[AutoLockManager] No lock callback registered!');
          result.failed++;
          result.errors.push(`No lock callback for ${unlock.folderId}`);
        }
      } catch (error) {
        result.failed++;
        const errorMsg = error instanceof Error ? error.message : String(error);
        result.errors.push(`Failed to lock ${unlock.folderId}: ${errorMsg}`);
        console.error(`[AutoLockManager] ✗ Failed to lock: ${unlock.folderPath}`, error);
      }
    }
    
    console.log(`[AutoLockManager] Lock complete: ${result.success} success, ${result.failed} failed`);
    
    return result;
  }
  
  /**
   * Start monitoring system events
   */
  public startMonitoring(): void {
    if (this.isMonitoring) {
      console.log('[AutoLockManager] Already monitoring');
      return;
    }
    
    console.log('[AutoLockManager] Starting system event monitoring...');
    
    // Setup platform-specific listeners
    this.setupPowerMonitorListeners();
    
    // Setup app lifecycle listeners
    this.setupAppLifecycleListeners();
    
    this.isMonitoring = true;
    console.log('[AutoLockManager] Monitoring started');
  }
  
  /**
   * Stop monitoring system events
   */
  public stopMonitoring(): void {
    if (!this.isMonitoring) {
      return;
    }
    
    console.log('[AutoLockManager] Stopping system event monitoring...');
    
    // PowerMonitor listeners are automatically cleaned up
    this.isMonitoring = false;
    console.log('[AutoLockManager] Monitoring stopped');
  }
  
  /**
   * Setup Electron PowerMonitor listeners (cross-platform)
   * Supports: Windows, macOS, Linux
   */
  private setupPowerMonitorListeners(): void {
    console.log('[AutoLockManager] Setting up PowerMonitor listeners...');
    
    // Lock screen event (Windows, macOS, some Linux)
    powerMonitor.on('lock-screen', async () => {
      console.log('[AutoLockManager] 🔒 Screen locked - auto-locking temporary folders');
      const result = await this.lockAllTemporaryFolders();
      
      if (result.success > 0) {
        this.sendNotification(
          'Auto-Lock',
          `${result.success} temporary folder(s) auto-locked`
        );
      }
    });
    
    // Suspend/sleep event
    powerMonitor.on('suspend', async () => {
      console.log('[AutoLockManager] 💤 System suspending - auto-locking temporary folders');
      const result = await this.lockAllTemporaryFolders();
      
      if (result.success > 0) {
        this.sendNotification(
          'Auto-Lock',
          `${result.success} temporary folder(s) locked before suspend`
        );
      }
    });
    
    // Shutdown event (Windows)
    powerMonitor.on('shutdown', async (e) => {
      console.log('[AutoLockManager] ⚡ System shutting down - auto-locking temporary folders');
      
      // Prevent shutdown until folders are locked
      e.preventDefault();
      
      const result = await this.lockAllTemporaryFolders();
      console.log('[AutoLockManager] Folders locked, allowing shutdown');
      
      // Allow shutdown to continue
      app.quit();
    });
    
    // Optional: Log unlock events
    powerMonitor.on('unlock-screen', () => {
      console.log('[AutoLockManager] 🔓 Screen unlocked');
      // Note: Folders remain locked until user manually unlocks via hotkey
    });
    
    powerMonitor.on('resume', () => {
      console.log('[AutoLockManager] ⏰ System resumed from suspend');
    });
    
    console.log('[AutoLockManager] PowerMonitor listeners registered');
  }
  
  /**
   * Setup app lifecycle listeners
   */
  private setupAppLifecycleListeners(): void {
    console.log('[AutoLockManager] Setting up app lifecycle listeners...');
    
    // App quit - lock all temporary folders
    app.on('before-quit', async (e) => {
      if (this.temporaryUnlocks.size > 0) {
        console.log('[AutoLockManager] 🚪 App quitting - auto-locking temporary folders');
        
        // Prevent quit until folders are locked
        e.preventDefault();
        
        const result = await this.lockAllTemporaryFolders();
        console.log(`[AutoLockManager] Locked ${result.success} folder(s) before quit`);
        
        // Clear tracking
        this.temporaryUnlocks.clear();
        
        // Now allow quit
        app.quit();
      }
    });
    
    console.log('[AutoLockManager] App lifecycle listeners registered');
  }
  
  /**
   * Send notification to user
   * @param title - Notification title
   * @param message - Notification message
   */
  private sendNotification(title: string, message: string): void {
    // This will be sent via IPC to renderer process for display
    // For now, just log
    console.log(`[Notification] ${title}: ${message}`);
  }
  
  /**
   * Get monitoring status
   */
  public isActive(): boolean {
    return this.isMonitoring;
  }
  
  /**
   * Get platform info
   */
  public getPlatform(): string {
    return this.platform;
  }
  
  /**
   * Get statistics
   */
  public getStats() {
    return {
      isMonitoring: this.isMonitoring,
      platform: this.platform,
      temporaryUnlocks: this.temporaryUnlocks.size,
      unlocks: Array.from(this.temporaryUnlocks.values()).map(u => ({
        folderId: u.folderId,
        mode: u.mode,
        unlockedAt: u.unlockedAt,
        folderPath: u.folderPath,
      })),
    };
  }
}

// Export singleton instance
export default AutoLockManager;
