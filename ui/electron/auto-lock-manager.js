"use strict";
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
exports.AutoLockManager = void 0;
const electron_1 = require("electron");
const os = __importStar(require("os"));
class AutoLockManager {
    constructor() {
        // Callbacks
        this.onLockCallback = null;
        this.temporaryUnlocks = new Map();
        this.isMonitoring = false;
        this.platform = os.platform();
        console.log(`[AutoLockManager] Initialized for platform: ${this.platform}`);
    }
    /**
     * Get singleton instance
     */
    static getInstance() {
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
    registerUnlock(folderId, profileId, mode, folderPath) {
        const unlockInfo = {
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
        }
        else {
            // Remove from tracking if switching to permanent
            this.temporaryUnlocks.delete(folderId);
            console.log(`[AutoLockManager] Permanent unlock - not tracking: ${folderId}`);
        }
    }
    /**
     * Unregister a folder (when manually locked)
     * @param folderId - Folder ID
     */
    unregisterUnlock(folderId) {
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
    getTemporaryUnlocks() {
        return Array.from(this.temporaryUnlocks.values());
    }
    /**
     * Get count of temporary unlocked folders
     */
    getTemporaryUnlockCount() {
        return this.temporaryUnlocks.size;
    }
    /**
     * Set callback for locking a folder
     * @param callback - Function to lock a folder
     */
    onLock(callback) {
        this.onLockCallback = callback;
    }
    /**
     * Lock all temporary folders
     * @returns Result of lock operations
     */
    async lockAllTemporaryFolders() {
        const result = {
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
                }
                else {
                    console.warn('[AutoLockManager] No lock callback registered!');
                    result.failed++;
                    result.errors.push(`No lock callback for ${unlock.folderId}`);
                }
            }
            catch (error) {
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
    startMonitoring() {
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
    stopMonitoring() {
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
    setupPowerMonitorListeners() {
        console.log('[AutoLockManager] Setting up PowerMonitor listeners...');
        // Lock screen event (Windows, macOS, some Linux)
        electron_1.powerMonitor.on('lock-screen', async () => {
            console.log('[AutoLockManager] 🔒 Screen locked - auto-locking temporary folders');
            const result = await this.lockAllTemporaryFolders();
            if (result.success > 0) {
                this.sendNotification('Auto-Lock', `${result.success} temporary folder(s) auto-locked`);
            }
        });
        // Suspend/sleep event
        electron_1.powerMonitor.on('suspend', async () => {
            console.log('[AutoLockManager] 💤 System suspending - auto-locking temporary folders');
            const result = await this.lockAllTemporaryFolders();
            if (result.success > 0) {
                this.sendNotification('Auto-Lock', `${result.success} temporary folder(s) locked before suspend`);
            }
        });
        // Shutdown event (Windows)
        electron_1.powerMonitor.on('shutdown', async (e) => {
            console.log('[AutoLockManager] ⚡ System shutting down - auto-locking temporary folders');
            // Prevent shutdown until folders are locked
            e.preventDefault();
            const result = await this.lockAllTemporaryFolders();
            console.log('[AutoLockManager] Folders locked, allowing shutdown');
            // Allow shutdown to continue
            electron_1.app.quit();
        });
        // Optional: Log unlock events
        electron_1.powerMonitor.on('unlock-screen', () => {
            console.log('[AutoLockManager] 🔓 Screen unlocked');
            // Note: Folders remain locked until user manually unlocks via hotkey
        });
        electron_1.powerMonitor.on('resume', () => {
            console.log('[AutoLockManager] ⏰ System resumed from suspend');
        });
        console.log('[AutoLockManager] PowerMonitor listeners registered');
    }
    /**
     * Setup app lifecycle listeners
     */
    setupAppLifecycleListeners() {
        console.log('[AutoLockManager] Setting up app lifecycle listeners...');
        // App quit - lock all temporary folders
        electron_1.app.on('before-quit', async (e) => {
            if (this.temporaryUnlocks.size > 0) {
                console.log('[AutoLockManager] 🚪 App quitting - auto-locking temporary folders');
                // Prevent quit until folders are locked
                e.preventDefault();
                const result = await this.lockAllTemporaryFolders();
                console.log(`[AutoLockManager] Locked ${result.success} folder(s) before quit`);
                // Clear tracking
                this.temporaryUnlocks.clear();
                // Now allow quit
                electron_1.app.quit();
            }
        });
        console.log('[AutoLockManager] App lifecycle listeners registered');
    }
    /**
     * Send notification to user
     * @param title - Notification title
     * @param message - Notification message
     */
    sendNotification(title, message) {
        // This will be sent via IPC to renderer process for display
        // For now, just log
        console.log(`[Notification] ${title}: ${message}`);
    }
    /**
     * Get monitoring status
     */
    isActive() {
        return this.isMonitoring;
    }
    /**
     * Get platform info
     */
    getPlatform() {
        return this.platform;
    }
    /**
     * Get statistics
     */
    getStats() {
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
exports.AutoLockManager = AutoLockManager;
AutoLockManager.instance = null;
// Export singleton instance
exports.default = AutoLockManager;
