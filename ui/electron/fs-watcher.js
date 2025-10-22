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
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
exports.FileSystemWatcher = void 0;
var chokidar_1 = __importDefault(require("chokidar"));
var fs = __importStar(require("fs"));
var path = __importStar(require("path"));
var FileSystemWatcher = /** @class */ (function () {
    function FileSystemWatcher() {
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
    FileSystemWatcher.getInstance = function () {
        if (!FileSystemWatcher.instance) {
            FileSystemWatcher.instance = new FileSystemWatcher();
        }
        return FileSystemWatcher.instance;
    };
    /**
     * Start watching vault storage directory
     * @param storagePath - Path to vault storage directory
     */
    FileSystemWatcher.prototype.startWatching = function (storagePath) {
        var _this = this;
        if (this.watcher) {
            console.log('[FileSystemWatcher] Already watching');
            return;
        }
        if (!fs.existsSync(storagePath)) {
            console.error("[FileSystemWatcher] Storage path does not exist: ".concat(storagePath));
            return;
        }
        this.storagePath = storagePath;
        console.log("[FileSystemWatcher] Starting to watch: ".concat(storagePath));
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
        this.watcher.on('add', function (filePath) {
            _this.recordEvent('add', filePath);
            console.log("[FileSystemWatcher] File added: ".concat(filePath));
        });
        // File changed
        this.watcher.on('change', function (filePath) {
            _this.recordEvent('change', filePath);
            console.log("[FileSystemWatcher] File changed: ".concat(filePath));
            // Backup metadata file
            _this.backupMetadata(filePath);
            // Check for suspicious activity
            if (_this.detectSuspiciousActivity(filePath)) {
                var warning = "Suspicious modification detected: ".concat(filePath);
                console.warn("[FileSystemWatcher] \u26A0\uFE0F ".concat(warning));
                if (_this.onSuspiciousCallback) {
                    _this.onSuspiciousCallback(warning);
                }
            }
            // Notify callback
            if (_this.onChangeCallback) {
                _this.onChangeCallback(filePath);
            }
        });
        // File deleted
        this.watcher.on('unlink', function (filePath) {
            _this.recordEvent('unlink', filePath);
            console.warn("[FileSystemWatcher] \u26A0\uFE0F File deleted: ".concat(filePath));
            // Deletion is always suspicious
            var warning = "Critical: Metadata file deleted: ".concat(filePath);
            console.error("[FileSystemWatcher] \uD83D\uDEA8 ".concat(warning));
            if (_this.onSuspiciousCallback) {
                _this.onSuspiciousCallback(warning);
            }
        });
        // Watcher ready
        this.watcher.on('ready', function () {
            console.log('[FileSystemWatcher] ✓ Watcher ready');
        });
        // Watcher error
        this.watcher.on('error', function (error) {
            console.error('[FileSystemWatcher] Error:', error);
        });
    };
    /**
     * Stop watching
     */
    FileSystemWatcher.prototype.stopWatching = function () {
        return __awaiter(this, void 0, void 0, function () {
            return __generator(this, function (_a) {
                switch (_a.label) {
                    case 0:
                        if (!this.watcher) {
                            return [2 /*return*/];
                        }
                        console.log('[FileSystemWatcher] Stopping watcher...');
                        return [4 /*yield*/, this.watcher.close()];
                    case 1:
                        _a.sent();
                        this.watcher = null;
                        this.storagePath = null;
                        console.log('[FileSystemWatcher] Watcher stopped');
                        return [2 /*return*/];
                }
            });
        });
    };
    /**
     * Record an event
     * @param type - Event type
     * @param filePath - File path
     */
    FileSystemWatcher.prototype.recordEvent = function (type, filePath) {
        var event = {
            type: type,
            path: filePath,
            timestamp: new Date(),
        };
        this.recentEvents.push(event);
        // Keep only last 100 events
        if (this.recentEvents.length > 100) {
            this.recentEvents.shift();
        }
    };
    /**
     * Backup metadata file
     * @param filePath - Path to metadata file
     */
    FileSystemWatcher.prototype.backupMetadata = function (filePath) {
        try {
            if (!fs.existsSync(filePath)) {
                console.warn("[FileSystemWatcher] Cannot backup - file not found: ".concat(filePath));
                return;
            }
            var timestamp = new Date().toISOString().replace(/[:.]/g, '-');
            var backupDir = path.join(path.dirname(filePath), '.backups');
            var filename = path.basename(filePath, '.json');
            var backupPath = path.join(backupDir, "".concat(filename, "_").concat(timestamp, ".json"));
            // Create backup directory
            if (!fs.existsSync(backupDir)) {
                fs.mkdirSync(backupDir, { recursive: true });
            }
            // Copy file
            fs.copyFileSync(filePath, backupPath);
            var backupInfo = {
                originalPath: filePath,
                backupPath: backupPath,
                timestamp: new Date(),
            };
            this.backups.push(backupInfo);
            console.log("[FileSystemWatcher] \u2713 Backed up: ".concat(path.basename(backupPath)));
            // Keep only last 10 backups per file
            this.cleanupOldBackups(backupDir, 10);
        }
        catch (error) {
            console.error('[FileSystemWatcher] Backup failed:', error);
        }
    };
    /**
     * Cleanup old backups
     * @param backupDir - Backup directory
     * @param keepCount - Number of backups to keep
     */
    FileSystemWatcher.prototype.cleanupOldBackups = function (backupDir, keepCount) {
        try {
            if (!fs.existsSync(backupDir)) {
                return;
            }
            var files = fs.readdirSync(backupDir)
                .filter(function (f) { return f.endsWith('.json'); })
                .map(function (f) { return ({
                name: f,
                path: path.join(backupDir, f),
                mtime: fs.statSync(path.join(backupDir, f)).mtime,
            }); })
                .sort(function (a, b) { return b.mtime.getTime() - a.mtime.getTime(); }); // Newest first
            // Delete old backups
            if (files.length > keepCount) {
                var toDelete = files.slice(keepCount);
                for (var _i = 0, toDelete_1 = toDelete; _i < toDelete_1.length; _i++) {
                    var file = toDelete_1[_i];
                    fs.unlinkSync(file.path);
                    console.log("[FileSystemWatcher] Deleted old backup: ".concat(file.name));
                }
            }
        }
        catch (error) {
            console.error('[FileSystemWatcher] Cleanup failed:', error);
        }
    };
    /**
     * Detect suspicious activity
     * @param filePath - File path that changed
     * @returns True if suspicious
     */
    FileSystemWatcher.prototype.detectSuspiciousActivity = function (filePath) {
        // Check for rapid changes (more than 5 changes in 10 seconds)
        var now = new Date();
        var tenSecondsAgo = new Date(now.getTime() - 10000);
        var recentChanges = this.recentEvents.filter(function (e) { return e.path === filePath && e.timestamp > tenSecondsAgo; });
        if (recentChanges.length > 5) {
            console.warn("[FileSystemWatcher] Rapid changes detected: ".concat(recentChanges.length, " changes in 10s"));
            return true;
        }
        // Check for changes outside app (when app is running)
        // This would require tracking app-initiated changes vs external changes
        // For now, we'll assume all changes could be suspicious
        return false;
    };
    /**
     * Set callback for file changes
     * @param callback - Function to call on file change
     */
    FileSystemWatcher.prototype.onChange = function (callback) {
        this.onChangeCallback = callback;
    };
    /**
     * Set callback for suspicious activity
     * @param callback - Function to call on suspicious activity
     */
    FileSystemWatcher.prototype.onSuspicious = function (callback) {
        this.onSuspiciousCallback = callback;
    };
    /**
     * Get recent events
     * @param count - Number of events to return
     */
    FileSystemWatcher.prototype.getRecentEvents = function (count) {
        if (count === void 0) { count = 10; }
        return this.recentEvents.slice(-count);
    };
    /**
     * Get backup history
     * @param count - Number of backups to return
     */
    FileSystemWatcher.prototype.getBackupHistory = function (count) {
        if (count === void 0) { count = 10; }
        return this.backups.slice(-count);
    };
    /**
     * Get statistics
     */
    FileSystemWatcher.prototype.getStats = function () {
        return {
            isWatching: this.watcher !== null,
            storagePath: this.storagePath,
            totalEvents: this.recentEvents.length,
            totalBackups: this.backups.length,
            recentEvents: this.recentEvents.slice(-5),
        };
    };
    /**
     * Check if watching
     */
    FileSystemWatcher.prototype.isWatching = function () {
        return this.watcher !== null;
    };
    FileSystemWatcher.instance = null;
    return FileSystemWatcher;
}());
exports.FileSystemWatcher = FileSystemWatcher;
// Export singleton instance
exports.default = FileSystemWatcher;
