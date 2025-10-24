"use strict";
/**
 * IPC Communication Type Definitions
 *
 * Defines the communication protocol between the Electron renderer process
 * and the main process for vault management operations
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.DEFAULT_IPC_CONFIG = exports.DEFAULT_IPC_RETRY = exports.DEFAULT_IPC_TIMEOUTS = void 0;
const vault_1 = require("./vault");
// ==================== DEFAULT CONFIGURATIONS ====================
/**
 * Default IPC timeout configuration
 */
exports.DEFAULT_IPC_TIMEOUTS = {
    default: 10000, // 10 seconds
    vault_operations: 30000, // 30 seconds
    file_operations: 60000, // 60 seconds
    authentication: 15000, // 15 seconds
};
/**
 * Default IPC retry configuration
 */
exports.DEFAULT_IPC_RETRY = {
    maxRetries: 3,
    retryDelay: 1000, // 1 second
    backoffMultiplier: 2,
    retryableErrors: ['NETWORK_ERROR', 'TIMEOUT', 'SERVICE_UNAVAILABLE'],
};
/**
 * Default IPC client configuration
 */
exports.DEFAULT_IPC_CONFIG = {
    timeout: exports.DEFAULT_IPC_TIMEOUTS,
    retry: exports.DEFAULT_IPC_RETRY,
    enableLogging: true,
    logLevel: 'info',
};
