/**
 * Global Hotkey Manager for PhantomVault 2.0
 * Manages global keyboard shortcuts across the OS
 * Uses Electron's globalShortcut API
 */

import { app, globalShortcut, BrowserWindow } from 'electron';
import * as fs from 'fs';
import * as path from 'path';
import * as os from 'os';

export interface HotkeyConfig {
  unlockHotkey: string;
  recoveryHotkey: string;
  enabled: boolean;
}

export class HotkeyManager {
  private static instance: HotkeyManager;
  private config: HotkeyConfig;
  private configPath: string;
  private onUnlockRequested?: () => void;
  private onRecoveryRequested?: () => void;

  private constructor() {
    // Default hotkeys
    this.config = {
      unlockHotkey: 'CommandOrControl+Alt+V', // Ctrl+Alt+V on Linux/Windows, Cmd+Alt+V on macOS
      recoveryHotkey: 'CommandOrControl+Alt+R', // For recovery key input
      enabled: true,
    };

    // Config file location
    this.configPath = this.getConfigPath();
    this.loadConfig();
  }

  public static getInstance(): HotkeyManager {
    if (!HotkeyManager.instance) {
      HotkeyManager.instance = new HotkeyManager();
    }
    return HotkeyManager.instance;
  }

  /**
   * Get config file path (cross-platform)
   */
  private getConfigPath(): string {
    let configDir: string;

    if (process.platform === 'win32') {
      configDir = path.join(process.env.APPDATA || path.join(os.homedir(), 'AppData', 'Roaming'), 'PhantomVault');
    } else {
      configDir = path.join(os.homedir(), '.phantom_vault_storage');
    }

    if (!fs.existsSync(configDir)) {
      fs.mkdirSync(configDir, { recursive: true, mode: 0o700 });
    }

    return path.join(configDir, 'hotkey_config.json');
  }

  /**
   * Load hotkey configuration from disk
   */
  private loadConfig(): void {
    try {
      if (fs.existsSync(this.configPath)) {
        const data = fs.readFileSync(this.configPath, 'utf-8');
        const savedConfig = JSON.parse(data);
        this.config = { ...this.config, ...savedConfig };
      }
    } catch (error) {
      console.error('Failed to load hotkey config:', error);
    }
  }

  /**
   * Save hotkey configuration to disk
   */
  private saveConfig(): void {
    try {
      fs.writeFileSync(
        this.configPath,
        JSON.stringify(this.config, null, 2),
        { mode: 0o600 }
      );
    } catch (error) {
      console.error('Failed to save hotkey config:', error);
    }
  }

  /**
   * Register all hotkeys
   */
  public registerHotkeys(): boolean {
    if (!this.config.enabled) {
      console.log('Hotkeys disabled in config');
      return false;
    }

    // Unregister existing hotkeys first
    this.unregisterHotkeys();

    try {
      // Register unlock hotkey
      const unlockSuccess = globalShortcut.register(this.config.unlockHotkey, () => {
        console.log(`Unlock hotkey pressed: ${this.config.unlockHotkey}`);
        if (this.onUnlockRequested) {
          this.onUnlockRequested();
        }
      });

      if (!unlockSuccess) {
        console.error(`Failed to register unlock hotkey: ${this.config.unlockHotkey}`);
        return false;
      }

      // Register recovery hotkey
      const recoverySuccess = globalShortcut.register(this.config.recoveryHotkey, () => {
        console.log(`Recovery hotkey pressed: ${this.config.recoveryHotkey}`);
        if (this.onRecoveryRequested) {
          this.onRecoveryRequested();
        }
      });

      if (!recoverySuccess) {
        console.error(`Failed to register recovery hotkey: ${this.config.recoveryHotkey}`);
        // Unlock hotkey registered, but recovery failed
        // Continue anyway
      }

      console.log('✅ Global hotkeys registered successfully');
      console.log(`   Unlock: ${this.config.unlockHotkey}`);
      console.log(`   Recovery: ${this.config.recoveryHotkey}`);
      return true;
    } catch (error) {
      console.error('Error registering hotkeys:', error);
      return false;
    }
  }

  /**
   * Unregister all hotkeys
   */
  public unregisterHotkeys(): void {
    globalShortcut.unregisterAll();
    console.log('All hotkeys unregistered');
  }

  /**
   * Update unlock hotkey
   */
  public setUnlockHotkey(accelerator: string): { success: boolean; error?: string } {
    // Test if hotkey is available
    const testSuccess = globalShortcut.register(accelerator, () => {});
    
    if (!testSuccess) {
      return {
        success: false,
        error: 'This hotkey is already in use by another application',
      };
    }

    // Unregister test
    globalShortcut.unregister(accelerator);

    // Update config
    this.config.unlockHotkey = accelerator;
    this.saveConfig();

    // Re-register all hotkeys
    this.registerHotkeys();

    return { success: true };
  }

  /**
   * Update recovery hotkey
   */
  public setRecoveryHotkey(accelerator: string): { success: boolean; error?: string } {
    const testSuccess = globalShortcut.register(accelerator, () => {});
    
    if (!testSuccess) {
      return {
        success: false,
        error: 'This hotkey is already in use by another application',
      };
    }

    globalShortcut.unregister(accelerator);

    this.config.recoveryHotkey = accelerator;
    this.saveConfig();
    this.registerHotkeys();

    return { success: true };
  }

  /**
   * Enable/disable hotkeys
   */
  public setEnabled(enabled: boolean): void {
    this.config.enabled = enabled;
    this.saveConfig();

    if (enabled) {
      this.registerHotkeys();
    } else {
      this.unregisterHotkeys();
    }
  }

  /**
   * Set callback for unlock hotkey
   */
  public onUnlock(callback: () => void): void {
    this.onUnlockRequested = callback;
  }

  /**
   * Set callback for recovery hotkey
   */
  public onRecovery(callback: () => void): void {
    this.onRecoveryRequested = callback;
  }

  /**
   * Get current configuration
   */
  public getConfig(): HotkeyConfig {
    return { ...this.config };
  }

  /**
   * Check if a hotkey is available (not in use)
   */
  public isHotkeyAvailable(accelerator: string): boolean {
    const success = globalShortcut.register(accelerator, () => {});
    if (success) {
      globalShortcut.unregister(accelerator);
    }
    return success;
  }

  /**
   * Get suggested hotkeys (safe defaults)
   */
  public getSuggestedHotkeys(): string[] {
    return [
      'CommandOrControl+Alt+V',
      'CommandOrControl+Alt+P',
      'CommandOrControl+Shift+V',
      'CommandOrControl+Shift+Alt+L',
      'Super+Alt+V', // Windows/Meta key
    ];
  }
}

/**
 * Initialize hotkey manager when app is ready
 */
export function initializeHotkeyManager(): HotkeyManager {
  const manager = HotkeyManager.getInstance();

  app.on('ready', () => {
    manager.registerHotkeys();
  });

  app.on('will-quit', () => {
    manager.unregisterHotkeys();
  });

  return manager;
}
