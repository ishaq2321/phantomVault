/**
 * Settings Store
 * 
 * Manages persistent storage of application settings
 */

import * as fs from 'fs';
import * as path from 'path';
import { app } from 'electron';
import { AppSettings } from '../../../src/types';
import { Logger } from './Logger';

export class SettingsStore {
  private logger: Logger;
  private settingsFile: string;
  private settings: AppSettings;

  constructor(logger: Logger) {
    this.logger = logger;
    this.settingsFile = this.getSettingsFilePath();
    this.settings = this.getDefaultSettings();
    this.loadSettings();
  }

  /**
   * Get settings file path
   */
  private getSettingsFilePath(): string {
    const userDataPath = app.getPath('userData');
    return path.join(userDataPath, 'settings.json');
  }

  /**
   * Get default settings
   */
  private getDefaultSettings(): AppSettings {
    return {
      theme: 'dark',
      autoStart: false,
      notifications: true,
      minimizeToTray: true,
      autoLock: true,
      lockTimeout: 15,
    };
  }

  /**
   * Load settings from file
   */
  private loadSettings(): void {
    try {
      if (fs.existsSync(this.settingsFile)) {
        const settingsData = fs.readFileSync(this.settingsFile, 'utf8');
        const loadedSettings = JSON.parse(settingsData);
        
        // Merge with defaults to ensure all properties exist
        this.settings = { ...this.getDefaultSettings(), ...loadedSettings };
        
        this.logger.info('Settings loaded successfully');
      } else {
        this.logger.info('No settings file found, using defaults');
        this.saveSettings();
      }
    } catch (error) {
      this.logger.error('Failed to load settings, using defaults', error);
      this.settings = this.getDefaultSettings();
    }
  }

  /**
   * Save settings to file
   */
  private saveSettings(): void {
    try {
      const settingsDir = path.dirname(this.settingsFile);
      if (!fs.existsSync(settingsDir)) {
        fs.mkdirSync(settingsDir, { recursive: true });
      }
      
      const settingsData = JSON.stringify(this.settings, null, 2);
      fs.writeFileSync(this.settingsFile, settingsData, 'utf8');
      
      this.logger.debug('Settings saved successfully');
    } catch (error) {
      this.logger.error('Failed to save settings', error);
      throw error;
    }
  }

  /**
   * Get current settings
   */
  public async getSettings(): Promise<AppSettings> {
    return { ...this.settings };
  }

  /**
   * Update settings
   */
  public async updateSettings(updates: Partial<AppSettings>): Promise<AppSettings> {
    // Validate settings
    this.validateSettings(updates);
    
    // Update settings
    this.settings = { ...this.settings, ...updates };
    
    // Save to file
    this.saveSettings();
    
    this.logger.info('Settings updated', updates);
    return { ...this.settings };
  }

  /**
   * Reset settings to defaults
   */
  public async resetToDefaults(): Promise<AppSettings> {
    this.settings = this.getDefaultSettings();
    this.saveSettings();
    
    this.logger.info('Settings reset to defaults');
    return { ...this.settings };
  }

  /**
   * Export settings to file
   */
  public async exportSettings(filePath: string): Promise<string> {
    try {
      const exportData = {
        version: '1.0.0',
        timestamp: new Date().toISOString(),
        settings: this.settings,
      };
      
      const exportJson = JSON.stringify(exportData, null, 2);
      fs.writeFileSync(filePath, exportJson, 'utf8');
      
      this.logger.info(`Settings exported to: ${filePath}`);
      return filePath;
    } catch (error) {
      this.logger.error(`Failed to export settings to: ${filePath}`, error);
      throw error;
    }
  }

  /**
   * Import settings from file
   */
  public async importSettings(filePath: string): Promise<AppSettings> {
    try {
      if (!fs.existsSync(filePath)) {
        throw new Error('Settings file does not exist');
      }
      
      const importData = fs.readFileSync(filePath, 'utf8');
      const parsedData = JSON.parse(importData);
      
      // Extract settings from export format or use direct format
      const importedSettings = parsedData.settings || parsedData;
      
      // Validate imported settings
      this.validateSettings(importedSettings);
      
      // Merge with defaults to ensure all properties exist
      this.settings = { ...this.getDefaultSettings(), ...importedSettings };
      
      // Save imported settings
      this.saveSettings();
      
      this.logger.info(`Settings imported from: ${filePath}`);
      return { ...this.settings };
    } catch (error) {
      this.logger.error(`Failed to import settings from: ${filePath}`, error);
      throw error;
    }
  }

  /**
   * Validate settings
   */
  private validateSettings(settings: Partial<AppSettings>): void {
    const errors: string[] = [];

    // Validate theme
    if (settings.theme !== undefined) {
      const validThemes = ['light', 'dark'];
      if (!validThemes.includes(settings.theme)) {
        errors.push(`Invalid theme: ${settings.theme}. Must be one of: ${validThemes.join(', ')}`);
      }
    }

    // Validate lock timeout
    if (settings.lockTimeout !== undefined) {
      if (typeof settings.lockTimeout !== 'number' || settings.lockTimeout < 1 || settings.lockTimeout > 120) {
        errors.push('Lock timeout must be a number between 1 and 120 minutes');
      }
    }

    // Validate boolean settings
    const booleanSettings: (keyof AppSettings)[] = ['autoStart', 'notifications', 'minimizeToTray', 'autoLock'];
    for (const key of booleanSettings) {
      if (settings[key] !== undefined && typeof settings[key] !== 'boolean') {
        errors.push(`${key} must be a boolean value`);
      }
    }

    if (errors.length > 0) {
      throw new Error(`Invalid settings: ${errors.join(', ')}`);
    }
  }

  /**
   * Check if a path is accessible
   */
  public async checkPathAccessibility(targetPath: string): Promise<boolean> {
    try {
      // Check if path exists
      if (!fs.existsSync(targetPath)) {
        return false;
      }
      
      // Check if it's a directory
      const stats = fs.statSync(targetPath);
      if (!stats.isDirectory()) {
        return false;
      }
      
      // Check read/write permissions by trying to create a temporary file
      const testFile = path.join(targetPath, '.phantomvault-test');
      try {
        fs.writeFileSync(testFile, 'test', 'utf8');
        fs.unlinkSync(testFile);
        return true;
      } catch (error) {
        return false;
      }
    } catch (error) {
      this.logger.debug(`Path accessibility check failed for: ${targetPath}`, error);
      return false;
    }
  }

  /**
   * Get settings file path
   */
  public getSettingsFilePath(): string {
    return this.settingsFile;
  }

  /**
   * Backup current settings
   */
  public async backupSettings(): Promise<string> {
    const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
    const backupFile = this.settingsFile.replace('.json', `-backup-${timestamp}.json`);
    
    try {
      fs.copyFileSync(this.settingsFile, backupFile);
      this.logger.info(`Settings backed up to: ${backupFile}`);
      return backupFile;
    } catch (error) {
      this.logger.error('Failed to backup settings', error);
      throw error;
    }
  }

  /**
   * Clean up old backup files
   */
  public async cleanupBackups(maxBackups: number = 5): Promise<void> {
    try {
      const settingsDir = path.dirname(this.settingsFile);
      const files = fs.readdirSync(settingsDir);
      
      const backupFiles = files
        .filter(file => file.includes('-backup-') && file.endsWith('.json'))
        .map(file => ({
          name: file,
          path: path.join(settingsDir, file),
          stats: fs.statSync(path.join(settingsDir, file)),
        }))
        .sort((a, b) => b.stats.mtime.getTime() - a.stats.mtime.getTime());
      
      // Remove old backups
      const filesToDelete = backupFiles.slice(maxBackups);
      for (const file of filesToDelete) {
        fs.unlinkSync(file.path);
        this.logger.debug(`Deleted old backup: ${file.name}`);
      }
      
      if (filesToDelete.length > 0) {
        this.logger.info(`Cleaned up ${filesToDelete.length} old backup files`);
      }
    } catch (error) {
      this.logger.error('Failed to cleanup backup files', error);
    }
  }
}