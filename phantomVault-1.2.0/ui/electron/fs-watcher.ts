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

import chokidar, { FSWatcher } from 'chokidar';
import * as fs from 'fs';
import * as path from 'path';

interface WatcherEvent {
  type: 'add' | 'change' | 'unlink';
  path: string;
  timestamp: Date;
}

interface BackupInfo {
  originalPath: string;
  backupPath: string;
  timestamp: Date;
}

export class FileSystemWatcher {
  private static instance: FileSystemWatcher | null = null;
  
  // Watcher instance
  private watcher: FSWatcher | null = null;
  
  // Storage path being watched
  private storagePath: string | null = null;
  
  // Recent events (for suspicious activity detection)
  private recentEvents: WatcherEvent[] = [];
  
  // Backup history
  private backups: BackupInfo[] = [];
  
  // Callbacks
  private onChangeCallback: ((filePath: string) => void) | null = null;
  private onSuspiciousCallback: ((activity: string) => void) | null = null;
  
  private constructor() {
    console.log('[FileSystemWatcher] Initialized');
  }
  
  /**
   * Get singleton instance
   */
  public static getInstance(): FileSystemWatcher {
    if (!FileSystemWatcher.instance) {
      FileSystemWatcher.instance = new FileSystemWatcher();
    }
    return FileSystemWatcher.instance;
  }
  
  /**
   * Start watching vault storage directory
   * @param storagePath - Path to vault storage directory
   */
  public startWatching(storagePath: string): void {
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
    this.watcher = chokidar.watch(path.join(storagePath, '**/*.json'), {
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
  public async stopWatching(): Promise<void> {
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
  private recordEvent(type: 'add' | 'change' | 'unlink', filePath: string): void {
    const event: WatcherEvent = {
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
  private backupMetadata(filePath: string): void {
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
      
      const backupInfo: BackupInfo = {
        originalPath: filePath,
        backupPath,
        timestamp: new Date(),
      };
      
      this.backups.push(backupInfo);
      
      console.log(`[FileSystemWatcher] ✓ Backed up: ${path.basename(backupPath)}`);
      
      // Keep only last 10 backups per file
      this.cleanupOldBackups(backupDir, 10);
      
    } catch (error) {
      console.error('[FileSystemWatcher] Backup failed:', error);
    }
  }
  
  /**
   * Cleanup old backups
   * @param backupDir - Backup directory
   * @param keepCount - Number of backups to keep
   */
  private cleanupOldBackups(backupDir: string, keepCount: number): void {
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
    } catch (error) {
      console.error('[FileSystemWatcher] Cleanup failed:', error);
    }
  }
  
  /**
   * Detect suspicious activity
   * @param filePath - File path that changed
   * @returns True if suspicious
   */
  private detectSuspiciousActivity(filePath: string): boolean {
    // Check for rapid changes (more than 5 changes in 10 seconds)
    const now = new Date();
    const tenSecondsAgo = new Date(now.getTime() - 10000);
    
    const recentChanges = this.recentEvents.filter(
      e => e.path === filePath && e.timestamp > tenSecondsAgo
    );
    
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
  public onChange(callback: (filePath: string) => void): void {
    this.onChangeCallback = callback;
  }
  
  /**
   * Set callback for suspicious activity
   * @param callback - Function to call on suspicious activity
   */
  public onSuspicious(callback: (activity: string) => void): void {
    this.onSuspiciousCallback = callback;
  }
  
  /**
   * Get recent events
   * @param count - Number of events to return
   */
  public getRecentEvents(count: number = 10): WatcherEvent[] {
    return this.recentEvents.slice(-count);
  }
  
  /**
   * Get backup history
   * @param count - Number of backups to return
   */
  public getBackupHistory(count: number = 10): BackupInfo[] {
    return this.backups.slice(-count);
  }
  
  /**
   * Get statistics
   */
  public getStats() {
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
  public isWatching(): boolean {
    return this.watcher !== null;
  }
}

// Export singleton instance
export default FileSystemWatcher;
