"use strict";
/**
 * Global Hotkey Manager for PhantomVault 2.0
 * Manages global keyboard shortcuts across the OS
 * Uses Electron's globalShortcut API
 */
var __assign = (this && this.__assign) || function () {
    __assign = Object.assign || function(t) {
        for (var s, i = 1, n = arguments.length; i < n; i++) {
            s = arguments[i];
            for (var p in s) if (Object.prototype.hasOwnProperty.call(s, p))
                t[p] = s[p];
        }
        return t;
    };
    return __assign.apply(this, arguments);
};
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
var electron_1 = require("electron");
var fs = __importStar(require("fs"));
var path = __importStar(require("path"));
var os = __importStar(require("os"));
var HotkeyManager = /** @class */ (function () {
    function HotkeyManager() {
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
    HotkeyManager.getInstance = function () {
        if (!HotkeyManager.instance) {
            HotkeyManager.instance = new HotkeyManager();
        }
        return HotkeyManager.instance;
    };
    /**
     * Get config file path (cross-platform)
     */
    HotkeyManager.prototype.getConfigPath = function () {
        var configDir;
        if (process.platform === 'win32') {
            configDir = path.join(process.env.APPDATA || path.join(os.homedir(), 'AppData', 'Roaming'), 'PhantomVault');
        }
        else {
            configDir = path.join(os.homedir(), '.phantom_vault_storage');
        }
        if (!fs.existsSync(configDir)) {
            fs.mkdirSync(configDir, { recursive: true, mode: 448 });
        }
        return path.join(configDir, 'hotkey_config.json');
    };
    /**
     * Load hotkey configuration from disk
     */
    HotkeyManager.prototype.loadConfig = function () {
        try {
            if (fs.existsSync(this.configPath)) {
                var data = fs.readFileSync(this.configPath, 'utf-8');
                var savedConfig = JSON.parse(data);
                this.config = __assign(__assign({}, this.config), savedConfig);
            }
        }
        catch (error) {
            console.error('Failed to load hotkey config:', error);
        }
    };
    /**
     * Save hotkey configuration to disk
     */
    HotkeyManager.prototype.saveConfig = function () {
        try {
            fs.writeFileSync(this.configPath, JSON.stringify(this.config, null, 2), { mode: 384 });
        }
        catch (error) {
            console.error('Failed to save hotkey config:', error);
        }
    };
    /**
     * Register all hotkeys
     */
    HotkeyManager.prototype.registerHotkeys = function () {
        var _this = this;
        if (!this.config.enabled) {
            console.log('Hotkeys disabled in config');
            return false;
        }
        // Unregister existing hotkeys first
        this.unregisterHotkeys();
        try {
            // Register unlock hotkey
            var unlockSuccess = electron_1.globalShortcut.register(this.config.unlockHotkey, function () {
                console.log("Unlock hotkey pressed: ".concat(_this.config.unlockHotkey));
                if (_this.onUnlockRequested) {
                    _this.onUnlockRequested();
                }
            });
            if (!unlockSuccess) {
                console.error("Failed to register unlock hotkey: ".concat(this.config.unlockHotkey));
                return false;
            }
            // Register recovery hotkey
            var recoverySuccess = electron_1.globalShortcut.register(this.config.recoveryHotkey, function () {
                console.log("Recovery hotkey pressed: ".concat(_this.config.recoveryHotkey));
                if (_this.onRecoveryRequested) {
                    _this.onRecoveryRequested();
                }
            });
            if (!recoverySuccess) {
                console.error("Failed to register recovery hotkey: ".concat(this.config.recoveryHotkey));
                // Unlock hotkey registered, but recovery failed
                // Continue anyway
            }
            console.log('✅ Global hotkeys registered successfully');
            console.log("   Unlock: ".concat(this.config.unlockHotkey));
            console.log("   Recovery: ".concat(this.config.recoveryHotkey));
            return true;
        }
        catch (error) {
            console.error('Error registering hotkeys:', error);
            return false;
        }
    };
    /**
     * Unregister all hotkeys
     */
    HotkeyManager.prototype.unregisterHotkeys = function () {
        electron_1.globalShortcut.unregisterAll();
        console.log('All hotkeys unregistered');
    };
    /**
     * Update unlock hotkey
     */
    HotkeyManager.prototype.setUnlockHotkey = function (accelerator) {
        // Test if hotkey is available
        var testSuccess = electron_1.globalShortcut.register(accelerator, function () { });
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
    };
    /**
     * Update recovery hotkey
     */
    HotkeyManager.prototype.setRecoveryHotkey = function (accelerator) {
        var testSuccess = electron_1.globalShortcut.register(accelerator, function () { });
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
    };
    /**
     * Enable/disable hotkeys
     */
    HotkeyManager.prototype.setEnabled = function (enabled) {
        this.config.enabled = enabled;
        this.saveConfig();
        if (enabled) {
            this.registerHotkeys();
        }
        else {
            this.unregisterHotkeys();
        }
    };
    /**
     * Set callback for unlock hotkey
     */
    HotkeyManager.prototype.onUnlock = function (callback) {
        this.onUnlockRequested = callback;
    };
    /**
     * Set callback for recovery hotkey
     */
    HotkeyManager.prototype.onRecovery = function (callback) {
        this.onRecoveryRequested = callback;
    };
    /**
     * Get current configuration
     */
    HotkeyManager.prototype.getConfig = function () {
        return __assign({}, this.config);
    };
    /**
     * Check if a hotkey is available (not in use)
     */
    HotkeyManager.prototype.isHotkeyAvailable = function (accelerator) {
        var success = electron_1.globalShortcut.register(accelerator, function () { });
        if (success) {
            electron_1.globalShortcut.unregister(accelerator);
        }
        return success;
    };
    /**
     * Get suggested hotkeys (safe defaults)
     */
    HotkeyManager.prototype.getSuggestedHotkeys = function () {
        return [
            'CommandOrControl+Alt+V',
            'CommandOrControl+Alt+P',
            'CommandOrControl+Shift+V',
            'CommandOrControl+Shift+Alt+L',
            'Super+Alt+V', // Windows/Meta key
        ];
    };
    return HotkeyManager;
}());
exports.HotkeyManager = HotkeyManager;
/**
 * Initialize hotkey manager when app is ready
 */
function initializeHotkeyManager() {
    var manager = HotkeyManager.getInstance();
    electron_1.app.on('ready', function () {
        manager.registerHotkeys();
    });
    electron_1.app.on('will-quit', function () {
        manager.unregisterHotkeys();
    });
    return manager;
}
