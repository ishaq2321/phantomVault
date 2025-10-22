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
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
var __generator = (this && this.__generator) || function (thisArg, body) {
    var _ = { label: 0, sent: function() { if (t[0] & 1) throw t[1]; return t[1]; }, trys: [], ops: [] }, f, y, t, g = Object.create((typeof Iterator === "function" ? Iterator : Object).prototype);
    return g.next = verb(0), g["throw"] = verb(1), g["return"] = verb(2), typeof Symbol === "function" && (g[Symbol.iterator] = function() { return this; }), g;
    function verb(n) { return function (v) { return step([n, v]); }; }
    function step(op) {
        if (f) throw new TypeError("Generator is already executing.");
        while (g && (g = 0, op[0] && (_ = 0)), _) try {
            if (f = 1, y && (t = op[0] & 2 ? y["return"] : op[0] ? y["throw"] || ((t = y["return"]) && t.call(y), 0) : y.next) && !(t = t.call(y, op[1])).done) return t;
            if (y = 0, t) op = [op[0] & 2, t.value];
            switch (op[0]) {
                case 0: case 1: t = op; break;
                case 4: _.label++; return { value: op[1], done: false };
                case 5: _.label++; y = op[1]; op = [0]; continue;
                case 7: op = _.ops.pop(); _.trys.pop(); continue;
                default:
                    if (!(t = _.trys, t = t.length > 0 && t[t.length - 1]) && (op[0] === 6 || op[0] === 2)) { _ = 0; continue; }
                    if (op[0] === 3 && (!t || (op[1] > t[0] && op[1] < t[3]))) { _.label = op[1]; break; }
                    if (op[0] === 6 && _.label < t[1]) { _.label = t[1]; t = op; break; }
                    if (t && _.label < t[2]) { _.label = t[2]; _.ops.push(op); break; }
                    if (t[2]) _.ops.pop();
                    _.trys.pop(); continue;
            }
            op = body.call(thisArg, _);
        } catch (e) { op = [6, e]; y = 0; } finally { f = t = 0; }
        if (op[0] & 5) throw op[1]; return { value: op[0] ? op[1] : void 0, done: true };
    }
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.AutoLockManager = void 0;
var electron_1 = require("electron");
var os = __importStar(require("os"));
var AutoLockManager = /** @class */ (function () {
    function AutoLockManager() {
        // Callbacks
        this.onLockCallback = null;
        this.temporaryUnlocks = new Map();
        this.isMonitoring = false;
        this.platform = os.platform();
        console.log("[AutoLockManager] Initialized for platform: ".concat(this.platform));
    }
    /**
     * Get singleton instance
     */
    AutoLockManager.getInstance = function () {
        if (!AutoLockManager.instance) {
            AutoLockManager.instance = new AutoLockManager();
        }
        return AutoLockManager.instance;
    };
    /**
     * Register a folder unlock to track
     * @param folderId - Folder ID
     * @param profileId - Profile ID
     * @param mode - Unlock mode (temporary or permanent)
     * @param folderPath - Physical folder path
     */
    AutoLockManager.prototype.registerUnlock = function (folderId, profileId, mode, folderPath) {
        var unlockInfo = {
            folderId: folderId,
            profileId: profileId,
            mode: mode,
            unlockedAt: new Date(),
            folderPath: folderPath,
        };
        // Only track temporary unlocks
        if (mode === 'temporary') {
            this.temporaryUnlocks.set(folderId, unlockInfo);
            console.log("[AutoLockManager] Tracking temporary unlock: ".concat(folderId, " (").concat(folderPath, ")"));
            console.log("[AutoLockManager] Total temporary unlocks: ".concat(this.temporaryUnlocks.size));
        }
        else {
            // Remove from tracking if switching to permanent
            this.temporaryUnlocks.delete(folderId);
            console.log("[AutoLockManager] Permanent unlock - not tracking: ".concat(folderId));
        }
    };
    /**
     * Unregister a folder (when manually locked)
     * @param folderId - Folder ID
     */
    AutoLockManager.prototype.unregisterUnlock = function (folderId) {
        var wasTracked = this.temporaryUnlocks.has(folderId);
        this.temporaryUnlocks.delete(folderId);
        if (wasTracked) {
            console.log("[AutoLockManager] Unregistered unlock: ".concat(folderId));
            console.log("[AutoLockManager] Remaining temporary unlocks: ".concat(this.temporaryUnlocks.size));
        }
    };
    /**
     * Get all temporary unlocked folders
     */
    AutoLockManager.prototype.getTemporaryUnlocks = function () {
        return Array.from(this.temporaryUnlocks.values());
    };
    /**
     * Get count of temporary unlocked folders
     */
    AutoLockManager.prototype.getTemporaryUnlockCount = function () {
        return this.temporaryUnlocks.size;
    };
    /**
     * Set callback for locking a folder
     * @param callback - Function to lock a folder
     */
    AutoLockManager.prototype.onLock = function (callback) {
        this.onLockCallback = callback;
    };
    /**
     * Lock all temporary folders
     * @returns Result of lock operations
     */
    AutoLockManager.prototype.lockAllTemporaryFolders = function () {
        return __awaiter(this, void 0, void 0, function () {
            var result, unlocksCopy, _i, unlocksCopy_1, unlock, error_1, errorMsg;
            return __generator(this, function (_a) {
                switch (_a.label) {
                    case 0:
                        result = {
                            success: 0,
                            failed: 0,
                            errors: [],
                        };
                        if (this.temporaryUnlocks.size === 0) {
                            console.log('[AutoLockManager] No temporary folders to lock');
                            return [2 /*return*/, result];
                        }
                        console.log("[AutoLockManager] Locking ".concat(this.temporaryUnlocks.size, " temporary folder(s)..."));
                        unlocksCopy = Array.from(this.temporaryUnlocks.values());
                        _i = 0, unlocksCopy_1 = unlocksCopy;
                        _a.label = 1;
                    case 1:
                        if (!(_i < unlocksCopy_1.length)) return [3 /*break*/, 8];
                        unlock = unlocksCopy_1[_i];
                        _a.label = 2;
                    case 2:
                        _a.trys.push([2, 6, , 7]);
                        if (!this.onLockCallback) return [3 /*break*/, 4];
                        return [4 /*yield*/, this.onLockCallback(unlock.folderId)];
                    case 3:
                        _a.sent();
                        result.success++;
                        console.log("[AutoLockManager] \u2713 Locked: ".concat(unlock.folderPath));
                        return [3 /*break*/, 5];
                    case 4:
                        console.warn('[AutoLockManager] No lock callback registered!');
                        result.failed++;
                        result.errors.push("No lock callback for ".concat(unlock.folderId));
                        _a.label = 5;
                    case 5: return [3 /*break*/, 7];
                    case 6:
                        error_1 = _a.sent();
                        result.failed++;
                        errorMsg = error_1 instanceof Error ? error_1.message : String(error_1);
                        result.errors.push("Failed to lock ".concat(unlock.folderId, ": ").concat(errorMsg));
                        console.error("[AutoLockManager] \u2717 Failed to lock: ".concat(unlock.folderPath), error_1);
                        return [3 /*break*/, 7];
                    case 7:
                        _i++;
                        return [3 /*break*/, 1];
                    case 8:
                        console.log("[AutoLockManager] Lock complete: ".concat(result.success, " success, ").concat(result.failed, " failed"));
                        return [2 /*return*/, result];
                }
            });
        });
    };
    /**
     * Start monitoring system events
     */
    AutoLockManager.prototype.startMonitoring = function () {
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
    };
    /**
     * Stop monitoring system events
     */
    AutoLockManager.prototype.stopMonitoring = function () {
        if (!this.isMonitoring) {
            return;
        }
        console.log('[AutoLockManager] Stopping system event monitoring...');
        // PowerMonitor listeners are automatically cleaned up
        this.isMonitoring = false;
        console.log('[AutoLockManager] Monitoring stopped');
    };
    /**
     * Setup Electron PowerMonitor listeners (cross-platform)
     * Supports: Windows, macOS, Linux
     */
    AutoLockManager.prototype.setupPowerMonitorListeners = function () {
        var _this = this;
        console.log('[AutoLockManager] Setting up PowerMonitor listeners...');
        // Lock screen event (Windows, macOS, some Linux)
        electron_1.powerMonitor.on('lock-screen', function () { return __awaiter(_this, void 0, void 0, function () {
            var result;
            return __generator(this, function (_a) {
                switch (_a.label) {
                    case 0:
                        console.log('[AutoLockManager] 🔒 Screen locked - auto-locking temporary folders');
                        return [4 /*yield*/, this.lockAllTemporaryFolders()];
                    case 1:
                        result = _a.sent();
                        if (result.success > 0) {
                            this.sendNotification('Auto-Lock', "".concat(result.success, " temporary folder(s) auto-locked"));
                        }
                        return [2 /*return*/];
                }
            });
        }); });
        // Suspend/sleep event
        electron_1.powerMonitor.on('suspend', function () { return __awaiter(_this, void 0, void 0, function () {
            var result;
            return __generator(this, function (_a) {
                switch (_a.label) {
                    case 0:
                        console.log('[AutoLockManager] 💤 System suspending - auto-locking temporary folders');
                        return [4 /*yield*/, this.lockAllTemporaryFolders()];
                    case 1:
                        result = _a.sent();
                        if (result.success > 0) {
                            this.sendNotification('Auto-Lock', "".concat(result.success, " temporary folder(s) locked before suspend"));
                        }
                        return [2 /*return*/];
                }
            });
        }); });
        // Shutdown event (Windows)
        electron_1.powerMonitor.on('shutdown', function (e) { return __awaiter(_this, void 0, void 0, function () {
            var result;
            return __generator(this, function (_a) {
                switch (_a.label) {
                    case 0:
                        console.log('[AutoLockManager] ⚡ System shutting down - auto-locking temporary folders');
                        // Prevent shutdown until folders are locked
                        e.preventDefault();
                        return [4 /*yield*/, this.lockAllTemporaryFolders()];
                    case 1:
                        result = _a.sent();
                        console.log('[AutoLockManager] Folders locked, allowing shutdown');
                        // Allow shutdown to continue
                        electron_1.app.quit();
                        return [2 /*return*/];
                }
            });
        }); });
        // Optional: Log unlock events
        electron_1.powerMonitor.on('unlock-screen', function () {
            console.log('[AutoLockManager] 🔓 Screen unlocked');
            // Note: Folders remain locked until user manually unlocks via hotkey
        });
        electron_1.powerMonitor.on('resume', function () {
            console.log('[AutoLockManager] ⏰ System resumed from suspend');
        });
        console.log('[AutoLockManager] PowerMonitor listeners registered');
    };
    /**
     * Setup app lifecycle listeners
     */
    AutoLockManager.prototype.setupAppLifecycleListeners = function () {
        var _this = this;
        console.log('[AutoLockManager] Setting up app lifecycle listeners...');
        // App quit - lock all temporary folders
        electron_1.app.on('before-quit', function (e) { return __awaiter(_this, void 0, void 0, function () {
            var result;
            return __generator(this, function (_a) {
                switch (_a.label) {
                    case 0:
                        if (!(this.temporaryUnlocks.size > 0)) return [3 /*break*/, 2];
                        console.log('[AutoLockManager] 🚪 App quitting - auto-locking temporary folders');
                        // Prevent quit until folders are locked
                        e.preventDefault();
                        return [4 /*yield*/, this.lockAllTemporaryFolders()];
                    case 1:
                        result = _a.sent();
                        console.log("[AutoLockManager] Locked ".concat(result.success, " folder(s) before quit"));
                        // Clear tracking
                        this.temporaryUnlocks.clear();
                        // Now allow quit
                        electron_1.app.quit();
                        _a.label = 2;
                    case 2: return [2 /*return*/];
                }
            });
        }); });
        console.log('[AutoLockManager] App lifecycle listeners registered');
    };
    /**
     * Send notification to user
     * @param title - Notification title
     * @param message - Notification message
     */
    AutoLockManager.prototype.sendNotification = function (title, message) {
        // This will be sent via IPC to renderer process for display
        // For now, just log
        console.log("[Notification] ".concat(title, ": ").concat(message));
    };
    /**
     * Get monitoring status
     */
    AutoLockManager.prototype.isActive = function () {
        return this.isMonitoring;
    };
    /**
     * Get platform info
     */
    AutoLockManager.prototype.getPlatform = function () {
        return this.platform;
    };
    /**
     * Get statistics
     */
    AutoLockManager.prototype.getStats = function () {
        return {
            isMonitoring: this.isMonitoring,
            platform: this.platform,
            temporaryUnlocks: this.temporaryUnlocks.size,
            unlocks: Array.from(this.temporaryUnlocks.values()).map(function (u) { return ({
                folderId: u.folderId,
                mode: u.mode,
                unlockedAt: u.unlockedAt,
                folderPath: u.folderPath,
            }); }),
        };
    };
    AutoLockManager.instance = null;
    return AutoLockManager;
}());
exports.AutoLockManager = AutoLockManager;
// Export singleton instance
exports.default = AutoLockManager;
