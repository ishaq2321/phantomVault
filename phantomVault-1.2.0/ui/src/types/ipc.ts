/**
 * IPC Communication Type Definitions
 * 
 * Defines the communication protocol between the Electron renderer process
 * and the main process for vault management operations
 */

import { 
  VaultInfo, 
  VaultConfig, 
  VaultOperationResult, 
  BulkOperationResult,
  VaultProfile,
  AppSettings,
  LogEntry,
  LogFilter,
  UnlockMode,
  IPCMessageType 
} from './vault';

// ==================== IPC REQUEST/RESPONSE TYPES ====================

/**
 * Base IPC request structure
 */
export interface BaseIPCRequest {
  type: IPCMessageType;
  requestId?: string;
  timestamp?: number;
}

/**
 * Base IPC response structure
 */
export interface BaseIPCResponse<T = any> {
  success: boolean;
  data?: T;
  error?: string;
  requestId?: string;
  timestamp?: number;
}

// ==================== VAULT OPERATION REQUESTS ====================

/**
 * Get vault list request
 */
export interface GetVaultListRequest extends BaseIPCRequest {
  type: IPCMessageType.GET_VAULT_LIST;
}

export interface GetVaultListResponse extends BaseIPCResponse<VaultInfo[]> {}

/**
 * Create vault request
 */
export interface CreateVaultRequest extends BaseIPCRequest {
  type: IPCMessageType.CREATE_VAULT;
  data: {
    config: VaultConfig;
    profileId?: string;
  };
}

export interface CreateVaultResponse extends BaseIPCResponse<{
  vaultId: string;
  vault: VaultInfo;
}> {}

/**
 * Mount vault request
 */
export interface MountVaultRequest extends BaseIPCRequest {
  type: IPCMessageType.MOUNT_VAULT;
  data: {
    vaultId: string;
    password?: string;
    mode?: UnlockMode;
  };
}

export interface MountVaultResponse extends BaseIPCResponse<VaultOperationResult> {}

/**
 * Unmount vault request
 */
export interface UnmountVaultRequest extends BaseIPCRequest {
  type: IPCMessageType.UNMOUNT_VAULT;
  data: {
    vaultId: string;
    force?: boolean;
  };
}

export interface UnmountVaultResponse extends BaseIPCResponse<VaultOperationResult> {}

/**
 * Delete vault request
 */
export interface DeleteVaultRequest extends BaseIPCRequest {
  type: IPCMessageType.DELETE_VAULT;
  data: {
    vaultId: string;
    deleteFiles?: boolean;
    confirmPassword?: string;
  };
}

export interface DeleteVaultResponse extends BaseIPCResponse<VaultOperationResult> {}

/**
 * Lock vault request
 */
export interface LockVaultRequest extends BaseIPCRequest {
  type: IPCMessageType.LOCK_VAULT;
  data: {
    vaultId: string;
    password?: string;
  };
}

export interface LockVaultResponse extends BaseIPCResponse<VaultOperationResult> {}

/**
 * Unlock vault request
 */
export interface UnlockVaultRequest extends BaseIPCRequest {
  type: IPCMessageType.UNLOCK_VAULT;
  data: {
    vaultId: string;
    password: string;
    mode?: UnlockMode;
  };
}

export interface UnlockVaultResponse extends BaseIPCResponse<VaultOperationResult> {}

// ==================== PROFILE OPERATION REQUESTS ====================

/**
 * Get profiles request
 */
export interface GetProfilesRequest extends BaseIPCRequest {
  type: IPCMessageType.GET_PROFILES;
}

export interface GetProfilesResponse extends BaseIPCResponse<{
  profiles: VaultProfile[];
  activeProfile: VaultProfile | null;
}> {}

/**
 * Create profile request
 */
export interface CreateProfileRequest extends BaseIPCRequest {
  type: IPCMessageType.CREATE_PROFILE;
  data: {
    name: string;
    masterPassword: string;
    recoveryKey: string;
  };
}

export interface CreateProfileResponse extends BaseIPCResponse<{
  profile: VaultProfile;
}> {}

/**
 * Set active profile request
 */
export interface SetActiveProfileRequest extends BaseIPCRequest {
  type: IPCMessageType.SET_ACTIVE_PROFILE;
  data: {
    profileId: string;
  };
}

export interface SetActiveProfileResponse extends BaseIPCResponse<{
  profile: VaultProfile;
}> {}

/**
 * Verify password request
 */
export interface VerifyPasswordRequest extends BaseIPCRequest {
  type: IPCMessageType.VERIFY_PASSWORD;
  data: {
    profileId: string;
    password: string;
  };
}

export interface VerifyPasswordResponse extends BaseIPCResponse<{
  isValid: boolean;
}> {}

// ==================== SETTINGS REQUESTS ====================

/**
 * Get settings request
 */
export interface GetSettingsRequest extends BaseIPCRequest {
  type: IPCMessageType.GET_SETTINGS;
}

export interface GetSettingsResponse extends BaseIPCResponse<AppSettings> {}

/**
 * Update settings request
 */
export interface UpdateSettingsRequest extends BaseIPCRequest {
  type: IPCMessageType.UPDATE_SETTINGS;
  data: {
    settings: Partial<AppSettings>;
  };
}

export interface UpdateSettingsResponse extends BaseIPCResponse<AppSettings> {}

// ==================== ACTIVITY MONITORING REQUESTS ====================

/**
 * Get activity log request
 */
export interface GetActivityLogRequest extends BaseIPCRequest {
  type: IPCMessageType.GET_ACTIVITY_LOG;
  data?: {
    filter?: LogFilter;
    limit?: number;
    offset?: number;
  };
}

export interface GetActivityLogResponse extends BaseIPCResponse<{
  logs: LogEntry[];
  totalCount: number;
  hasMore: boolean;
}> {}

/**
 * Subscribe to activity updates request
 */
export interface SubscribeActivityRequest extends BaseIPCRequest {
  type: IPCMessageType.SUBSCRIBE_ACTIVITY;
  data?: {
    filter?: LogFilter;
  };
}

export interface SubscribeActivityResponse extends BaseIPCResponse<{
  subscriptionId: string;
}> {}

/**
 * Unsubscribe from activity updates request
 */
export interface UnsubscribeActivityRequest extends BaseIPCRequest {
  type: IPCMessageType.UNSUBSCRIBE_ACTIVITY;
  data: {
    subscriptionId: string;
  };
}

export interface UnsubscribeActivityResponse extends BaseIPCResponse<{}> {}

// ==================== EVENT NOTIFICATIONS ====================

/**
 * Vault status changed notification
 */
export interface VaultStatusChangedEvent {
  type: IPCMessageType.VAULT_STATUS_CHANGED;
  data: {
    vaultId: string;
    oldStatus: string;
    newStatus: string;
    vault: VaultInfo;
  };
}

/**
 * Activity log entry notification
 */
export interface ActivityLogEntryEvent {
  type: IPCMessageType.ACTIVITY_LOG_ENTRY;
  data: {
    entry: LogEntry;
  };
}

/**
 * Error notification
 */
export interface ErrorNotificationEvent {
  type: IPCMessageType.ERROR_NOTIFICATION;
  data: {
    error: string;
    code?: string;
    details?: Record<string, any>;
  };
}

/**
 * Profile updated notification
 */
export interface ProfileUpdatedEvent {
  type: IPCMessageType.PROFILE_UPDATED;
  data: {
    profile: VaultProfile;
    isActive: boolean;
  };
}

/**
 * Folder status update notification
 */
export interface FolderStatusUpdateEvent {
  type: IPCMessageType.FOLDER_STATUS_UPDATE;
  data: {
    profileId: string;
    folders: Array<{
      id: string;
      name: string;
      isLocked: boolean;
      originalPath: string;
    }>;
  };
}

// ==================== UNION TYPES ====================

/**
 * All possible IPC requests
 */
export type IPCRequest = 
  | GetVaultListRequest
  | CreateVaultRequest
  | MountVaultRequest
  | UnmountVaultRequest
  | DeleteVaultRequest
  | LockVaultRequest
  | UnlockVaultRequest
  | GetProfilesRequest
  | CreateProfileRequest
  | SetActiveProfileRequest
  | VerifyPasswordRequest
  | GetSettingsRequest
  | UpdateSettingsRequest
  | GetActivityLogRequest
  | SubscribeActivityRequest
  | UnsubscribeActivityRequest;

/**
 * All possible IPC responses
 */
export type IPCResponse = 
  | GetVaultListResponse
  | CreateVaultResponse
  | MountVaultResponse
  | UnmountVaultResponse
  | DeleteVaultResponse
  | LockVaultResponse
  | UnlockVaultResponse
  | GetProfilesResponse
  | CreateProfileResponse
  | SetActiveProfileResponse
  | VerifyPasswordResponse
  | GetSettingsResponse
  | UpdateSettingsResponse
  | GetActivityLogResponse
  | SubscribeActivityResponse
  | UnsubscribeActivityResponse;

/**
 * All possible IPC events
 */
export type IPCEvent = 
  | VaultStatusChangedEvent
  | ActivityLogEntryEvent
  | ErrorNotificationEvent
  | ProfileUpdatedEvent
  | FolderStatusUpdateEvent;

// ==================== IPC HANDLER INTERFACES ====================

/**
 * IPC handlers interface for main process
 */
export interface IPCHandlers {
  // Vault management
  [IPCMessageType.GET_VAULT_LIST]: () => Promise<GetVaultListResponse>;
  [IPCMessageType.CREATE_VAULT]: (request: CreateVaultRequest) => Promise<CreateVaultResponse>;
  [IPCMessageType.MOUNT_VAULT]: (request: MountVaultRequest) => Promise<MountVaultResponse>;
  [IPCMessageType.UNMOUNT_VAULT]: (request: UnmountVaultRequest) => Promise<UnmountVaultResponse>;
  [IPCMessageType.DELETE_VAULT]: (request: DeleteVaultRequest) => Promise<DeleteVaultResponse>;
  [IPCMessageType.LOCK_VAULT]: (request: LockVaultRequest) => Promise<LockVaultResponse>;
  [IPCMessageType.UNLOCK_VAULT]: (request: UnlockVaultRequest) => Promise<UnlockVaultResponse>;
  
  // Profile management
  [IPCMessageType.GET_PROFILES]: () => Promise<GetProfilesResponse>;
  [IPCMessageType.CREATE_PROFILE]: (request: CreateProfileRequest) => Promise<CreateProfileResponse>;
  [IPCMessageType.SET_ACTIVE_PROFILE]: (request: SetActiveProfileRequest) => Promise<SetActiveProfileResponse>;
  [IPCMessageType.VERIFY_PASSWORD]: (request: VerifyPasswordRequest) => Promise<VerifyPasswordResponse>;
  
  // Settings management
  [IPCMessageType.GET_SETTINGS]: () => Promise<GetSettingsResponse>;
  [IPCMessageType.UPDATE_SETTINGS]: (request: UpdateSettingsRequest) => Promise<UpdateSettingsResponse>;
  
  // Activity monitoring
  [IPCMessageType.GET_ACTIVITY_LOG]: (request: GetActivityLogRequest) => Promise<GetActivityLogResponse>;
  [IPCMessageType.SUBSCRIBE_ACTIVITY]: (request: SubscribeActivityRequest) => Promise<SubscribeActivityResponse>;
  [IPCMessageType.UNSUBSCRIBE_ACTIVITY]: (request: UnsubscribeActivityRequest) => Promise<UnsubscribeActivityResponse>;
}

/**
 * IPC event emitters interface for main process
 */
export interface IPCEventEmitters {
  // Emit vault status changes
  emitVaultStatusChanged: (data: VaultStatusChangedEvent['data']) => void;
  
  // Emit activity log entries
  emitActivityLogEntry: (data: ActivityLogEntryEvent['data']) => void;
  
  // Emit error notifications
  emitErrorNotification: (data: ErrorNotificationEvent['data']) => void;
  
  // Emit profile updates
  emitProfileUpdated: (data: ProfileUpdatedEvent['data']) => void;
  
  // Emit folder status updates
  emitFolderStatusUpdate: (data: FolderStatusUpdateEvent['data']) => void;
}

// ==================== UTILITY TYPES ====================

/**
 * Extract request data type from IPC request
 */
export type ExtractRequestData<T extends IPCRequest> = T extends { data: infer D } ? D : never;

/**
 * Extract response data type from IPC response
 */
export type ExtractResponseData<T extends IPCResponse> = T extends BaseIPCResponse<infer D> ? D : never;

/**
 * IPC timeout configuration
 */
export interface IPCTimeoutConfig {
  default: number; // Default timeout in milliseconds
  vault_operations: number; // Timeout for vault operations
  file_operations: number; // Timeout for file operations
  authentication: number; // Timeout for authentication
}

/**
 * IPC retry configuration
 */
export interface IPCRetryConfig {
  maxRetries: number;
  retryDelay: number; // milliseconds
  backoffMultiplier: number;
  retryableErrors: string[]; // Error codes that should trigger retry
}

/**
 * IPC client configuration
 */
export interface IPCClientConfig {
  timeout: IPCTimeoutConfig;
  retry: IPCRetryConfig;
  enableLogging: boolean;
  logLevel: 'debug' | 'info' | 'warn' | 'error';
}

// ==================== DEFAULT CONFIGURATIONS ====================

/**
 * Default IPC timeout configuration
 */
export const DEFAULT_IPC_TIMEOUTS: IPCTimeoutConfig = {
  default: 10000, // 10 seconds
  vault_operations: 30000, // 30 seconds
  file_operations: 60000, // 60 seconds
  authentication: 15000, // 15 seconds
};

/**
 * Default IPC retry configuration
 */
export const DEFAULT_IPC_RETRY: IPCRetryConfig = {
  maxRetries: 3,
  retryDelay: 1000, // 1 second
  backoffMultiplier: 2,
  retryableErrors: ['NETWORK_ERROR', 'TIMEOUT', 'SERVICE_UNAVAILABLE'],
};

/**
 * Default IPC client configuration
 */
export const DEFAULT_IPC_CONFIG: IPCClientConfig = {
  timeout: DEFAULT_IPC_TIMEOUTS,
  retry: DEFAULT_IPC_RETRY,
  enableLogging: true,
  logLevel: 'info',
};