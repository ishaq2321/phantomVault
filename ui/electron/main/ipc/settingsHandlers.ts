/**
 * Settings IPC Handlers
 * 
 * IPC handlers for application settings management
 */

import { ipcMain, IpcMainInvokeEvent, app, dialog } from 'electron';
import { AppSettings, ApiResponse } from '../../../src/types';
import { Logger } from '../utils/Logger';
import { SettingsStore } from '../utils/SettingsStore';

export class SettingsHandlers {
  private logger: Logger;
  private settingsStore: SettingsStore;

  constructor(logger: Logger) {
    this.logger = logger;
    this.settingsStore = new SettingsStore(logger);
    this.registerHandlers();
  }

  /**
   * Register all settings-related IPC handlers
   */
  private registerHandlers(): void {
    ipcMain.handle('settings:get', this.handleGetSettings.bind(this));
    ipcMain.handle('settings:update', this.handleUpdateSettings.bind(this));
    ipcMain.handle('settings:reset', this.handleResetSettings.bind(this));
    ipcMain.handle('settings:export', this.handleExportSettings.bind(this));
    ipcMain.handle('settings:import', this.handleImportSettings.bind(this));
    
    // File system operations
    ipcMain.handle('fs:select-folder', this.handleSelectFolder.bind(this));
    ipcMain.handle('fs:check-path', this.handleCheckPath.bind(this));
  }

  /**
   * Handle get settings request
   */
  private async handleGetSettings(event: IpcMainInvokeEvent): Promise<ApiResponse<AppSettings>> {
    try {
      this.logger.debug('IPC: Getting application settings');
      
      const settings = await this.settingsStore.getSettings();
      
      this.logger.debug('IPC: Successfully retrieved settings');
      return {
        success: true,
        data: settings,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to get settings';
      this.logger.error('IPC: Failed to get settings', error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Handle update settings request
   */
  private async handleUpdateSettings(
    event: IpcMainInvokeEvent,
    settings: Partial<AppSettings>
  ): Promise<ApiResponse> {
    try {
      this.logger.info('IPC: Updating application settings', settings);
      
      await this.settingsStore.updateSettings(settings);
      
      // Apply settings that require immediate action
      await this.applySettings(settings);
      
      this.logger.info('IPC: Successfully updated settings');
      return {
        success: true,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to update settings';
      this.logger.error('IPC: Failed to update settings', error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Handle reset settings request
   */
  private async handleResetSettings(event: IpcMainInvokeEvent): Promise<ApiResponse<AppSettings>> {
    try {
      this.logger.info('IPC: Resetting application settings to defaults');
      
      const defaultSettings = await this.settingsStore.resetToDefaults();
      
      // Apply default settings
      await this.applySettings(defaultSettings);
      
      this.logger.info('IPC: Successfully reset settings to defaults');
      return {
        success: true,
        data: defaultSettings,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to reset settings';
      this.logger.error('IPC: Failed to reset settings', error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Handle export settings request
   */
  private async handleExportSettings(event: IpcMainInvokeEvent): Promise<ApiResponse<string>> {
    try {
      this.logger.info('IPC: Exporting application settings');
      
      const result = await dialog.showSaveDialog({
        title: 'Export Settings',
        defaultPath: `phantomvault-settings-${new Date().toISOString().split('T')[0]}.json`,
        filters: [
          { name: 'JSON Files', extensions: ['json'] },
          { name: 'All Files', extensions: ['*'] },
        ],
      });
      
      if (result.canceled || !result.filePath) {
        return {
          success: false,
          error: 'Export cancelled',
          timestamp: new Date(),
        };
      }
      
      const exportPath = await this.settingsStore.exportSettings(result.filePath);
      
      this.logger.info(`IPC: Successfully exported settings to ${exportPath}`);
      return {
        success: true,
        data: exportPath,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to export settings';
      this.logger.error('IPC: Failed to export settings', error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Handle import settings request
   */
  private async handleImportSettings(event: IpcMainInvokeEvent): Promise<ApiResponse<AppSettings>> {
    try {
      this.logger.info('IPC: Importing application settings');
      
      const result = await dialog.showOpenDialog({
        title: 'Import Settings',
        filters: [
          { name: 'JSON Files', extensions: ['json'] },
          { name: 'All Files', extensions: ['*'] },
        ],
        properties: ['openFile'],
      });
      
      if (result.canceled || result.filePaths.length === 0) {
        return {
          success: false,
          error: 'Import cancelled',
          timestamp: new Date(),
        };
      }
      
      const settings = await this.settingsStore.importSettings(result.filePaths[0]);
      
      // Apply imported settings
      await this.applySettings(settings);
      
      this.logger.info(`IPC: Successfully imported settings from ${result.filePaths[0]}`);
      return {
        success: true,
        data: settings,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to import settings';
      this.logger.error('IPC: Failed to import settings', error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Handle select folder request
   */
  private async handleSelectFolder(event: IpcMainInvokeEvent): Promise<ApiResponse<string>> {
    try {
      this.logger.debug('IPC: Opening folder selection dialog');
      
      const result = await dialog.showOpenDialog({
        title: 'Select Folder',
        properties: ['openDirectory', 'createDirectory'],
      });
      
      if (result.canceled || result.filePaths.length === 0) {
        return {
          success: false,
          error: 'Folder selection cancelled',
          timestamp: new Date(),
        };
      }
      
      const selectedPath = result.filePaths[0];
      this.logger.debug(`IPC: Folder selected: ${selectedPath}`);
      
      return {
        success: true,
        data: selectedPath,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to select folder';
      this.logger.error('IPC: Failed to select folder', error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Handle check path request
   */
  private async handleCheckPath(
    event: IpcMainInvokeEvent,
    path: string
  ): Promise<ApiResponse<boolean>> {
    try {
      this.logger.debug(`IPC: Checking path accessibility: ${path}`);
      
      const isAccessible = await this.settingsStore.checkPathAccessibility(path);
      
      return {
        success: true,
        data: isAccessible,
        timestamp: new Date(),
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to check path';
      this.logger.error(`IPC: Failed to check path: ${path}`, error);
      
      return {
        success: false,
        error: errorMessage,
        timestamp: new Date(),
      };
    }
  }

  /**
   * Apply settings that require immediate action
   */
  private async applySettings(settings: Partial<AppSettings>): Promise<void> {
    // Handle auto-start setting
    if (settings.autoStart !== undefined) {
      try {
        if (settings.autoStart) {
          app.setLoginItemSettings({
            openAtLogin: true,
            name: 'PhantomVault',
          });
        } else {
          app.setLoginItemSettings({
            openAtLogin: false,
          });
        }
        this.logger.info(`Auto-start ${settings.autoStart ? 'enabled' : 'disabled'}`);
      } catch (error) {
        this.logger.error('Failed to update auto-start setting', error);
      }
    }

    // Handle theme changes
    if (settings.theme !== undefined) {
      // In a real implementation, this would update the app theme
      this.logger.info(`Theme changed to: ${settings.theme}`);
    }

    // Handle other settings that need immediate application
    // Add more settings handling as needed
  }

  /**
   * Cleanup handlers
   */
  public cleanup(): void {
    ipcMain.removeHandler('settings:get');
    ipcMain.removeHandler('settings:update');
    ipcMain.removeHandler('settings:reset');
    ipcMain.removeHandler('settings:export');
    ipcMain.removeHandler('settings:import');
    ipcMain.removeHandler('fs:select-folder');
    ipcMain.removeHandler('fs:check-path');
  }
}