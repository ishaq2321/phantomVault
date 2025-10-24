"use strict";
/**
 * Global Hotkey Manager for PhantomVault 2.0
 * Manages global keyboard shortcuts across the OS
 * Uses Electron's globalShortcut API
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
exports.HotkeyManager = void 0;
exports.initializeHotkeyManager = initializeHotkeyManager;
const electron_1 = require("electron");
const fs = __importStar(require("fs"));
const path = __importStar(require("path"));
const os = __importStar(require("os"));
class HotkeyManager {
    constructor() {
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
    static getInstance() {
        if (!HotkeyManager.instance) {
            HotkeyManager.instance = new HotkeyManager();
        }
        return HotkeyManager.instance;
    }
    /**
     * Get config file path (cross-platform)
     */
    getConfigPath() {
        let configDir;
        if (process.platform === 'win32') {
            configDir = path.join(process.env.APPDATA || path.join(os.homedir(), 'AppData', 'Roaming'), 'PhantomVault');
        }
        else {
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
    loadConfig() {
        try {
            if (fs.existsSync(this.configPath)) {
                const data = fs.readFileSync(this.configPath, 'utf-8');
                const savedConfig = JSON.parse(data);
                this.config = { ...this.config, ...savedConfig };
            }
        }
        catch (error) {
            console.error('Failed to load hotkey config:', error);
        }
    }
    /**
     * Save hotkey configuration to disk
     */
    saveConfig() {
        try {
            fs.writeFileSync(this.configPath, JSON.stringify(this.config, null, 2), { mode: 0o600 });
        }
        catch (error) {
            console.error('Failed to save hotkey config:', error);
        }
    }
    /**
     * Register all hotkeys
     */
    registerHotkeys() {
        if (!this.config.enabled) {
            console.log('Hotkeys disabled in config');
            return false;
        }
        // Unregister existing hotkeys first
        this.unregisterHotkeys();
        try {
            // Register unlock hotkey
            const unlockSuccess = electron_1.globalShortcut.register(this.config.unlockHotkey, () => {
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
            const recoverySuccess = electron_1.globalShortcut.register(this.config.recoveryHotkey, () => {
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
        }
        catch (error) {
            console.error('Error registering hotkeys:', error);
            return false;
        }
    }
    /**
     * Unregister all hotkeys
     */
    unregisterHotkeys() {
        electron_1.globalShortcut.unregisterAll();
        console.log('All hotkeys unregistered');
    }
    /**
     * Update unlock hotkey
     */
    setUnlockHotkey(accelerator) {
        // Test if hotkey is available
        const testSuccess = electron_1.globalShortcut.register(accelerator, () => { });
        if (!testSuccess) {
            return {
                success: false,
                error: 'This hotkey is already in use by another application',
            };
        }
        // Unregister test
        electron_1.globalShortcut.unregister(accelerator);
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
    setRecoveryHotkey(accelerator) {
        const testSuccess = electron_1.globalShortcut.register(accelerator, () => { });
        if (!testSuccess) {
            return {
                success: false,
                error: 'This hotkey is already in use by another application',
            };
        }
        electron_1.globalShortcut.unregister(accelerator);
        this.config.recoveryHotkey = accelerator;
        this.saveConfig();
        this.registerHotkeys();
        return { success: true };
    }
    /**
     * Enable/disable hotkeys
     */
    setEnabled(enabled) {
        this.config.enabled = enabled;
        this.saveConfig();
        if (enabled) {
            this.registerHotkeys();
        }
        else {
            this.unregisterHotkeys();
        }
    }
    /**
     * Set callback for unlock hotkey
     */
    onUnlock(callback) {
        this.onUnlockRequested = callback;
    }
    /**
     * Set callback for recovery hotkey
     */
    onRecovery(callback) {
        this.onRecoveryRequested = callback;
    }
    /**
     * Get current configuration
     */
    getConfig() {
        return { ...this.config };
    }
    /**
     * Check if a hotkey is available (not in use)
     */
    isHotkeyAvailable(accelerator) {
        const success = electron_1.globalShortcut.register(accelerator, () => { });
        if (success) {
            electron_1.globalShortcut.unregister(accelerator);
        }
        return success;
    }
    /**
     * Get suggested hotkeys (safe defaults)
     */
    getSuggestedHotkeys() {
        return [
            'CommandOrControl+Alt+V',
            'CommandOrControl+Alt+P',
            'CommandOrControl+Shift+V',
            'CommandOrControl+Shift+Alt+L',
            'Super+Alt+V', // Windows/Meta key
        ];
    }
}
exports.HotkeyManager = HotkeyManager;
/**
 * Initialize hotkey manager when app is ready
 */
function initializeHotkeyManager() {
    const manager = HotkeyManager.getInstance();
    electron_1.app.on('ready', () => {
        manager.registerHotkeys();
    });
    electron_1.app.on('will-quit', () => {
        manager.unregisterHotkeys();
    });
    return manager;
}
