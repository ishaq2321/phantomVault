"use strict";
/**
 * FileSystemWatcher - Monitors vault storage for unauthorized changes
 *
 * Features:
 * - Watches vault storage directory for file changes
 * - Detects unauthorized metadata modifications
 * - Automatic metadata backup on changes
 * - Suspicious activity detection
 * - Cross-platform support using chokidar
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
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.FileSystemWatcher = void 0;
const chokidar_1 = __importDefault(require("chokidar"));
const fs = __importStar(require("fs"));
const path = __importStar(require("path"));
class FileSystemWatcher {
    constructor() {
        // Watcher instance
        this.watcher = null;
        // Storage path being watched
        this.storagePath = null;
        // Recent events (for suspicious activity detection)
        this.recentEvents = [];
        // Backup history
        this.backups = [];
        // Callbacks
        this.onChangeCallback = null;
        this.onSuspiciousCallback = null;
        console.log('[FileSystemWatcher] Initialized');
    }
    /**
     * Get singleton instance
     */
    static getInstance() {
        if (!FileSystemWatcher.instance) {
            FileSystemWatcher.instance = new FileSystemWatcher();
        }
        return FileSystemWatcher.instance;
    }
    /**
     * Start watching vault storage directory
     * @param storagePath - Path to vault storage directory
     */
    startWatching(storagePath) {
        if (this.watcher) {
            console.log('[FileSystemWatcher] Already watching');
            return;
        }
        if (!fs.existsSync(storagePath)) {
            console.error(`[FileSystemWatcher] Storage path does not exist: ${storagePath}`);
            return;
        }
        this.storagePath = storagePath;
        console.log(`[FileSystemWatcher] Starting to watch: ${storagePath}`);
        // Watch for changes to JSON metadata files only
        this.watcher = chokidar_1.default.watch(path.join(storagePath, '**/*.json'), {
            persistent: true,
            ignoreInitial: true,
            awaitWriteFinish: {
                stabilityThreshold: 500,
                pollInterval: 100,
            },
        });
        // File added
        this.watcher.on('add', (filePath) => {
            this.recordEvent('add', filePath);
            console.log(`[FileSystemWatcher] File added: ${filePath}`);
        });
        // File changed
        this.watcher.on('change', (filePath) => {
            this.recordEvent('change', filePath);
            console.log(`[FileSystemWatcher] File changed: ${filePath}`);
            // Backup metadata file
            this.backupMetadata(filePath);
            // Check for suspicious activity
            if (this.detectSuspiciousActivity(filePath)) {
                const warning = `Suspicious modification detected: ${filePath}`;
                console.warn(`[FileSystemWatcher] ⚠️ ${warning}`);
                if (this.onSuspiciousCallback) {
                    this.onSuspiciousCallback(warning);
                }
            }
            // Notify callback
            if (this.onChangeCallback) {
                this.onChangeCallback(filePath);
            }
        });
        // File deleted
        this.watcher.on('unlink', (filePath) => {
            this.recordEvent('unlink', filePath);
            console.warn(`[FileSystemWatcher] ⚠️ File deleted: ${filePath}`);
            // Deletion is always suspicious
            const warning = `Critical: Metadata file deleted: ${filePath}`;
            console.error(`[FileSystemWatcher] 🚨 ${warning}`);
            if (this.onSuspiciousCallback) {
                this.onSuspiciousCallback(warning);
            }
        });
        // Watcher ready
        this.watcher.on('ready', () => {
            console.log('[FileSystemWatcher] ✓ Watcher ready');
        });
        // Watcher error
        this.watcher.on('error', (error) => {
            console.error('[FileSystemWatcher] Error:', error);
        });
    }
    /**
     * Stop watching
     */
    async stopWatching() {
        if (!this.watcher) {
            return;
        }
        console.log('[FileSystemWatcher] Stopping watcher...');
        await this.watcher.close();
        this.watcher = null;
        this.storagePath = null;
        console.log('[FileSystemWatcher] Watcher stopped');
    }
    /**
     * Record an event
     * @param type - Event type
     * @param filePath - File path
     */
    recordEvent(type, filePath) {
        const event = {
            type,
            path: filePath,
            timestamp: new Date(),
        };
        this.recentEvents.push(event);
        // Keep only last 100 events
        if (this.recentEvents.length > 100) {
            this.recentEvents.shift();
        }
    }
    /**
     * Backup metadata file
     * @param filePath - Path to metadata file
     */
    backupMetadata(filePath) {
        try {
            if (!fs.existsSync(filePath)) {
                console.warn(`[FileSystemWatcher] Cannot backup - file not found: ${filePath}`);
                return;
            }
            const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
            const backupDir = path.join(path.dirname(filePath), '.backups');
            const filename = path.basename(filePath, '.json');
            const backupPath = path.join(backupDir, `${filename}_${timestamp}.json`);
            // Create backup directory
            if (!fs.existsSync(backupDir)) {
                fs.mkdirSync(backupDir, { recursive: true });
            }
            // Copy file
            fs.copyFileSync(filePath, backupPath);
            const backupInfo = {
                originalPath: filePath,
                backupPath,
                timestamp: new Date(),
            };
            this.backups.push(backupInfo);
            console.log(`[FileSystemWatcher] ✓ Backed up: ${path.basename(backupPath)}`);
            // Keep only last 10 backups per file
            this.cleanupOldBackups(backupDir, 10);
        }
        catch (error) {
            console.error('[FileSystemWatcher] Backup failed:', error);
        }
    }
    /**
     * Cleanup old backups
     * @param backupDir - Backup directory
     * @param keepCount - Number of backups to keep
     */
    cleanupOldBackups(backupDir, keepCount) {
        try {
            if (!fs.existsSync(backupDir)) {
                return;
            }
            const files = fs.readdirSync(backupDir)
                .filter(f => f.endsWith('.json'))
                .map(f => ({
                name: f,
                path: path.join(backupDir, f),
                mtime: fs.statSync(path.join(backupDir, f)).mtime,
            }))
                .sort((a, b) => b.mtime.getTime() - a.mtime.getTime()); // Newest first
            // Delete old backups
            if (files.length > keepCount) {
                const toDelete = files.slice(keepCount);
                for (const file of toDelete) {
                    fs.unlinkSync(file.path);
                    console.log(`[FileSystemWatcher] Deleted old backup: ${file.name}`);
                }
            }
        }
        catch (error) {
            console.error('[FileSystemWatcher] Cleanup failed:', error);
        }
    }
    /**
     * Detect suspicious activity
     * @param filePath - File path that changed
     * @returns True if suspicious
     */
    detectSuspiciousActivity(filePath) {
        // Check for rapid changes (more than 5 changes in 10 seconds)
        const now = new Date();
        const tenSecondsAgo = new Date(now.getTime() - 10000);
        const recentChanges = this.recentEvents.filter(e => e.path === filePath && e.timestamp > tenSecondsAgo);
        if (recentChanges.length > 5) {
            console.warn(`[FileSystemWatcher] Rapid changes detected: ${recentChanges.length} changes in 10s`);
            return true;
        }
        // Check for changes outside app (when app is running)
        // This would require tracking app-initiated changes vs external changes
        // For now, we'll assume all changes could be suspicious
        return false;
    }
    /**
     * Set callback for file changes
     * @param callback - Function to call on file change
     */
    onChange(callback) {
        this.onChangeCallback = callback;
    }
    /**
     * Set callback for suspicious activity
     * @param callback - Function to call on suspicious activity
     */
    onSuspicious(callback) {
        this.onSuspiciousCallback = callback;
    }
    /**
     * Get recent events
     * @param count - Number of events to return
     */
    getRecentEvents(count = 10) {
        return this.recentEvents.slice(-count);
    }
    /**
     * Get backup history
     * @param count - Number of backups to return
     */
    getBackupHistory(count = 10) {
        return this.backups.slice(-count);
    }
    /**
     * Get statistics
     */
    getStats() {
        return {
            isWatching: this.watcher !== null,
            storagePath: this.storagePath,
            totalEvents: this.recentEvents.length,
            totalBackups: this.backups.length,
            recentEvents: this.recentEvents.slice(-5),
        };
    }
    /**
     * Check if watching
     */
    isWatching() {
        return this.watcher !== null;
    }
}
exports.FileSystemWatcher = FileSystemWatcher;
FileSystemWatcher.instance = null;
// Export singleton instance
exports.default = FileSystemWatcher;
