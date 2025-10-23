/**
 * Type Definitions Index
 * 
 * Central export point for all TypeScript type definitions
 * used throughout the PhantomVault GUI application
 */

// ==================== CORE VAULT TYPES ====================
export type {
  // Vault data models
  VaultProfile,
  VaultFolder,
  VaultInfo,
  VaultConfig,
  VaultStatus,
  EncryptionLevel,
  
  // Vault operations
  VaultAction,
  VaultOperationResult,
  BulkOperationResult,
  UnlockMode,
  UnlockResult,
  
  // Application state
  AppState,
  AppView,
  AppSettings,
  Theme,
  
  // Activity monitoring
  LogEntry,
  LogLevel,
  LogFilter,
  
  // Notifications
  Notification,
  NotificationType,
  NotificationAction,
  
  // Form validation
  ValidationResult,
  ValidationError,
  PasswordStrength,
  
  // Error handling
  ErrorType,
  AppError,
  IPCError,
  FormValidationError,
  SystemError,
  SecurityError,
  
  // Component props
  VaultComponentProps,
  VaultDashboardProps,
  VaultCreationWizardProps,
  VaultEditorProps,
  ActivityMonitorProps,
  SettingsPanelProps,
  
  // Hooks
  UseVaultOperations,
  UseIPCCommunication,
  UseActivityMonitor,
  
  // Context types
  VaultContextState,
  VaultContextActions,
  AppContextState,
  AppContextActions,
  
  // IPC message types
  IPCMessageType,
  IPCMessage,
  IPCRequest as VaultIPCRequest,
  IPCResponse as VaultIPCResponse,
} from './vault';

// ==================== IPC COMMUNICATION TYPES ====================
export type {
  // Base IPC types
  BaseIPCRequest,
  BaseIPCResponse,
  
  // Vault operation requests/responses
  GetVaultListRequest,
  GetVaultListResponse,
  CreateVaultRequest,
  CreateVaultResponse,
  MountVaultRequest,
  MountVaultResponse,
  UnmountVaultRequest,
  UnmountVaultResponse,
  DeleteVaultRequest,
  DeleteVaultResponse,
  LockVaultRequest,
  LockVaultResponse,
  UnlockVaultRequest,
  UnlockVaultResponse,
  
  // Profile operation requests/responses
  GetProfilesRequest,
  GetProfilesResponse,
  CreateProfileRequest,
  CreateProfileResponse,
  SetActiveProfileRequest,
  SetActiveProfileResponse,
  VerifyPasswordRequest,
  VerifyPasswordResponse,
  
  // Settings requests/responses
  GetSettingsRequest,
  GetSettingsResponse,
  UpdateSettingsRequest,
  UpdateSettingsResponse,
  
  // Activity monitoring requests/responses
  GetActivityLogRequest,
  GetActivityLogResponse,
  SubscribeActivityRequest,
  SubscribeActivityResponse,
  UnsubscribeActivityRequest,
  UnsubscribeActivityResponse,
  
  // Event notifications
  VaultStatusChangedEvent,
  ActivityLogEntryEvent,
  ErrorNotificationEvent,
  ProfileUpdatedEvent,
  FolderStatusUpdateEvent,
  
  // Union types
  IPCRequest,
  IPCResponse,
  IPCEvent,
  
  // Handler interfaces
  IPCHandlers,
  IPCEventEmitters,
  
  // Utility types
  ExtractRequestData,
  ExtractResponseData,
  
  // Configuration types
  IPCTimeoutConfig,
  IPCRetryConfig,
  IPCClientConfig,
} from './ipc';

// ==================== ELECTRON API TYPES ====================
export type {
  // Native C++ API
  PhantomVaultNativeAPI,
  NativeOperationResult,
  FileAttributes,
  PartialFileAttributes,
  
  // Vault management APIs
  PhantomVaultProfileAPI,
  PhantomVaultFolderAPI,
  
  // Legacy APIs
  PhantomVaultAPI,
  EncryptionResult,
  HotkeyConfig,
  HotkeyAPI,
  AutoLockStats,
  LockResult,
  AutoLockAPI,
  WatcherEvent,
  WatcherStats,
  FSWatcherAPI,
} from './electron';

// ==================== CONSTANTS ====================
export {
  DEFAULT_IPC_TIMEOUTS,
  DEFAULT_IPC_RETRY,
  DEFAULT_IPC_CONFIG,
} from './ipc';

// ==================== TYPE GUARDS ====================

/**
 * Type guard to check if an error is an IPC error
 */
export function isIPCError(error: AppError): error is IPCError {
  return error.type === 'network' && 'requestType' in error;
}

/**
 * Type guard to check if an error is a validation error
 */
export function isValidationError(error: AppError): error is FormValidationError {
  return error.type === 'validation' && 'field' in error;
}

/**
 * Type guard to check if an error is a system error
 */
export function isSystemError(error: AppError): error is SystemError {
  return error.type === 'system';
}

/**
 * Type guard to check if an error is a security error
 */
export function isSecurityError(error: AppError): error is SecurityError {
  return error.type === 'security' && 'severity' in error;
}

/**
 * Type guard to check if a vault is mounted
 */
export function isVaultMounted(vault: VaultInfo): boolean {
  return vault.status === 'mounted';
}

/**
 * Type guard to check if a vault is locked
 */
export function isVaultLocked(vault: VaultInfo): boolean {
  return vault.status === 'unmounted';
}

/**
 * Type guard to check if a vault operation is in progress
 */
export function isVaultOperationInProgress(vault: VaultInfo): boolean {
  return ['loading', 'encrypting', 'decrypting'].includes(vault.status);
}

// ==================== UTILITY TYPES ====================

/**
 * Make all properties of T optional recursively
 */
export type DeepPartial<T> = {
  [P in keyof T]?: T[P] extends object ? DeepPartial<T[P]> : T[P];
};

/**
 * Make specific properties of T required
 */
export type RequireFields<T, K extends keyof T> = T & Required<Pick<T, K>>;

/**
 * Omit properties from T and make remaining properties optional
 */
export type PartialExcept<T, K extends keyof T> = Partial<T> & Pick<T, K>;

/**
 * Extract the payload type from an IPC message
 */
export type MessagePayload<T> = T extends { data: infer P } ? P : never;

/**
 * Create a branded type for IDs
 */
export type Brand<T, B> = T & { __brand: B };

/**
 * Branded types for different ID types
 */
export type VaultId = Brand<string, 'VaultId'>;
export type ProfileId = Brand<string, 'ProfileId'>;
export type FolderId = Brand<string, 'FolderId'>;
export type NotificationId = Brand<string, 'NotificationId'>;
export type LogEntryId = Brand<string, 'LogEntryId'>;

// ==================== ENUM EXPORTS ====================

/**
 * Re-export enums for convenience
 */
export { IPCMessageType } from './vault';

// ==================== GLOBAL AUGMENTATIONS ====================

declare global {
  /**
   * Extend the Window interface with PhantomVault API
   */
  interface Window {
    phantomVault: PhantomVaultAPI;
  }
  
  /**
   * Environment variables available in the renderer process
   */
  interface ImportMetaEnv {
    readonly VITE_APP_VERSION: string;
    readonly VITE_APP_NAME: string;
    readonly VITE_DEV_MODE: boolean;
  }
  
  interface ImportMeta {
    readonly env: ImportMetaEnv;
  }
}

// ==================== MODULE DECLARATIONS ====================

/**
 * Declare modules for assets that don't have TypeScript definitions
 */
declare module '*.svg' {
  const content: string;
  export default content;
}

declare module '*.png' {
  const content: string;
  export default content;
}

declare module '*.jpg' {
  const content: string;
  export default content;
}

declare module '*.jpeg' {
  const content: string;
  export default content;
}

declare module '*.gif' {
  const content: string;
  export default content;
}

declare module '*.webp' {
  const content: string;
  export default content;
}

declare module '*.ico' {
  const content: string;
  export default content;
}

declare module '*.css' {
  const classes: { [key: string]: string };
  export default classes;
}

declare module '*.scss' {
  const classes: { [key: string]: string };
  export default classes;
}

declare module '*.sass' {
  const classes: { [key: string]: string };
  export default classes;
}