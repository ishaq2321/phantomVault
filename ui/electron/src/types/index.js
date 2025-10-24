"use strict";
/**
 * Type Definitions Index
 *
 * Central export point for all TypeScript type definitions
 * used throughout the PhantomVault GUI application
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.IPCMessageType = exports.DEFAULT_IPC_CONFIG = exports.DEFAULT_IPC_RETRY = exports.DEFAULT_IPC_TIMEOUTS = void 0;
exports.isIPCError = isIPCError;
exports.isValidationError = isValidationError;
exports.isSystemError = isSystemError;
exports.isSecurityError = isSecurityError;
exports.isVaultMounted = isVaultMounted;
exports.isVaultLocked = isVaultLocked;
exports.isVaultOperationInProgress = isVaultOperationInProgress;
// ==================== CONSTANTS ====================
var ipc_1 = require("./ipc");
Object.defineProperty(exports, "DEFAULT_IPC_TIMEOUTS", { enumerable: true, get: function () { return ipc_1.DEFAULT_IPC_TIMEOUTS; } });
Object.defineProperty(exports, "DEFAULT_IPC_RETRY", { enumerable: true, get: function () { return ipc_1.DEFAULT_IPC_RETRY; } });
Object.defineProperty(exports, "DEFAULT_IPC_CONFIG", { enumerable: true, get: function () { return ipc_1.DEFAULT_IPC_CONFIG; } });
// ==================== TYPE GUARDS ====================
/**
 * Type guard to check if an error is an IPC error
 */
function isIPCError(error) {
    return error.type === 'network' && 'requestType' in error;
}
/**
 * Type guard to check if an error is a validation error
 */
function isValidationError(error) {
    return error.type === 'validation' && 'field' in error;
}
/**
 * Type guard to check if an error is a system error
 */
function isSystemError(error) {
    return error.type === 'system';
}
/**
 * Type guard to check if an error is a security error
 */
function isSecurityError(error) {
    return error.type === 'security' && 'severity' in error;
}
/**
 * Type guard to check if a vault is mounted
 */
function isVaultMounted(vault) {
    return vault.status === 'mounted';
}
/**
 * Type guard to check if a vault is locked
 */
function isVaultLocked(vault) {
    return vault.status === 'unmounted';
}
/**
 * Type guard to check if a vault operation is in progress
 */
function isVaultOperationInProgress(vault) {
    return ['loading', 'encrypting', 'decrypting'].includes(vault.status);
}
// ==================== ENUM EXPORTS ====================
/**
 * Re-export enums for convenience
 */
var vault_1 = require("./vault");
Object.defineProperty(exports, "IPCMessageType", { enumerable: true, get: function () { return vault_1.IPCMessageType; } });
