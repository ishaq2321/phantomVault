"use strict";
/**
 * Vault Management System Type Definitions
 *
 * Comprehensive TypeScript interfaces for the GUI vault management system
 * Based on the design specification for PhantomVault 2.0
 */
Object.defineProperty(exports, "__esModule", { value: true });
exports.IPCMessageType = void 0;
// ==================== IPC COMMUNICATION ====================
/**
 * IPC message types for communication with main process
 */
var IPCMessageType;
(function (IPCMessageType) {
    // Vault Operations
    IPCMessageType["GET_VAULT_LIST"] = "get-vault-list";
    IPCMessageType["CREATE_VAULT"] = "create-vault";
    IPCMessageType["MOUNT_VAULT"] = "mount-vault";
    IPCMessageType["UNMOUNT_VAULT"] = "unmount-vault";
    IPCMessageType["DELETE_VAULT"] = "delete-vault";
    IPCMessageType["LOCK_VAULT"] = "lock-vault";
    IPCMessageType["UNLOCK_VAULT"] = "unlock-vault";
    // Profile Operations
    IPCMessageType["GET_PROFILES"] = "get-profiles";
    IPCMessageType["CREATE_PROFILE"] = "create-profile";
    IPCMessageType["SET_ACTIVE_PROFILE"] = "set-active-profile";
    IPCMessageType["VERIFY_PASSWORD"] = "verify-password";
    // Configuration
    IPCMessageType["GET_SETTINGS"] = "get-settings";
    IPCMessageType["UPDATE_SETTINGS"] = "update-settings";
    // Monitoring
    IPCMessageType["GET_ACTIVITY_LOG"] = "get-activity-log";
    IPCMessageType["SUBSCRIBE_ACTIVITY"] = "subscribe-activity";
    IPCMessageType["UNSUBSCRIBE_ACTIVITY"] = "unsubscribe-activity";
    // Status Updates (from main to renderer)
    IPCMessageType["VAULT_STATUS_CHANGED"] = "vault-status-changed";
    IPCMessageType["ACTIVITY_LOG_ENTRY"] = "activity-log-entry";
    IPCMessageType["ERROR_NOTIFICATION"] = "error-notification";
    IPCMessageType["PROFILE_UPDATED"] = "profile-updated";
    IPCMessageType["FOLDER_STATUS_UPDATE"] = "folder-status-update";
})(IPCMessageType || (exports.IPCMessageType = IPCMessageType = {}));
