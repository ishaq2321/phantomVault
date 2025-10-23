/**
 * Vault Management System Type Definitions
 * 
 * Comprehensive TypeScript interfaces for the GUI vault management system
 * Based on the design specification for PhantomVault 2.0
 */

// ==================== CORE VAULT DATA MODELS ====================

/**
 * Vault profile information
 */
export interface VaultProfile {
  id: string;
  name: string;
  createdAt: Date;
  lastModified: Date;
  isActive: boolean;
}

/**
 * Individual vault folder configuration
 */
export interface VaultFolder {
  id: string;
  profileId: string;
  name: string;
  originalPath: string;
  encryptedPath: string;
  isLocked: boolean;
  keyboardSequence: string;
  autoLock: boolean;
  lockTimeout: number; // minutes
  size?: number; // bytes
  lastAccess?: Date;
}

/**
 * Vault information for dashboard display
 */
export interface VaultInfo {
  id: string;
  name: string;
  path: string;
  status: VaultStatus;
  lastAccess: Date;
  size: number;
  folderCount: number;
  profile: VaultProfile;
}

/**
 * Vault operational status
 */
export type VaultStatus = 'mounted' | 'unmounted' | 'error' | 'loading' | 'encrypting' | 'decrypting';

/**
 * Vault configuration for creation/editing
 */
export interface VaultConfig {
  name: string;
  path: string;
  password: string;
  keyboardSequence: string;
  autoMount: boolean;
  encryptionLevel: EncryptionLevel;
  autoLock: boolean;
  lockTimeout: number; // minutes
}

/**
 * Encryption strength levels
 */
export type EncryptionLevel = 'standard' | 'high';

// ==================== VAULT OPERATIONS ====================

/**
 * Vault operation types
 */
export type VaultAction = 
  | 'mount'
  | 'unmount' 
  | 'create'
  | 'edit'
  | 'delete'
  | 'lock'
  | 'unlock'
  | 'encrypt'
  | 'decrypt';

/**
 * Result of vault operations
 */
export interface VaultOperationResult {
  success: boolean;
  message: string;
  vaultId?: string;
  error?: string;
  details?: Record<string, any>;
}

/**
 * Bulk operation results
 */
export interface BulkOperationResult {
  totalCount: number;
  successCount: number;
  failedCount: number;
  results: VaultOperationResult[];
}

/**
 * Unlock operation modes
 */
export type UnlockMode = 'temporary' | 'permanent';

/**
 * Unlock operation result
 */
export interface UnlockResult {
  success_count: number;
  failed_count: number;
  error_messages: string[];
  unlocked_folders: string[];
}

// ==================== APPLICATION STATE ====================

/**
 * Main application state
 */
export interface AppState {
  // Authentication
  isAuthenticated: boolean;
  currentProfile: VaultProfile | null;
  
  // Vault data
  vaults: VaultInfo[];
  selectedVault: string | null;
  
  // UI state
  activeView: AppView;
  isLoading: boolean;
  notifications: Notification[];
  
  // Settings
  settings: AppSettings;
  
  // Activity monitoring
  activityLog: LogEntry[];
  activityFilter: LogFilter;
}

/**
 * Application views/pages
 */
export type AppView = 'dashboard' | 'vaults' | 'activity' | 'settings';

/**
 * Application settings
 */
export interface AppSettings {
  // General
  autoStart: boolean;
  minimizeToTray: boolean;
  closeToTray: boolean;
  
  // Security
  autoLockTimeout: number; // minutes
  requirePasswordConfirmation: boolean;
  clearClipboardTimeout: number; // seconds
  
  // UI
  theme: Theme;
  language: string;
  showNotifications: boolean;
  
  // Monitoring
  enableActivityLogging: boolean;
  maxLogEntries: number;
  logLevel: LogLevel;
}

/**
 * UI theme options
 */
export type Theme = 'light' | 'dark' | 'system';

// ==================== ACTIVITY MONITORING ====================

/**
 * Log entry for activity monitoring
 */
export interface LogEntry {
  id: string;
  timestamp: Date;
  level: LogLevel;
  source: string;
  message: string;
  vaultId?: string;
  details?: Record<string, any>;
}

/**
 * Log severity levels
 */
export type LogLevel = 'debug' | 'info' | 'warning' | 'error' | 'security';

/**
 * Log filtering options
 */
export interface LogFilter {
  level?: LogLevel;
  source?: string;
  vaultId?: string;
  startDate?: Date;
  endDate?: Date;
  searchText?: string;
}

// ==================== NOTIFICATIONS ====================

/**
 * Application notification
 */
export interface Notification {
  id: string;
  type: NotificationType;
  title: string;
  message: string;
  timestamp: Date;
  duration?: number; // milliseconds, undefined for persistent
  actions?: NotificationAction[];
}

/**
 * Notification types
 */
export type NotificationType = 'info' | 'success' | 'warning' | 'error';

/**
 * Notification action button
 */
export interface NotificationAction {
  label: string;
  action: () => void;
  style?: 'primary' | 'secondary' | 'danger';
}

// ==================== IPC COMMUNICATION ====================

/**
 * IPC message types for communication with main process
 */
export enum IPCMessageType {
  // Vault Operations
  GET_VAULT_LIST = 'get-vault-list',
  CREATE_VAULT = 'create-vault',
  MOUNT_VAULT = 'mount-vault',
  UNMOUNT_VAULT = 'unmount-vault',
  DELETE_VAULT = 'delete-vault',
  LOCK_VAULT = 'lock-vault',
  UNLOCK_VAULT = 'unlock-vault',
  
  // Profile Operations
  GET_PROFILES = 'get-profiles',
  CREATE_PROFILE = 'create-profile',
  SET_ACTIVE_PROFILE = 'set-active-profile',
  VERIFY_PASSWORD = 'verify-password',
  
  // Configuration
  GET_SETTINGS = 'get-settings',
  UPDATE_SETTINGS = 'update-settings',
  
  // Monitoring
  GET_ACTIVITY_LOG = 'get-activity-log',
  SUBSCRIBE_ACTIVITY = 'subscribe-activity',
  UNSUBSCRIBE_ACTIVITY = 'unsubscribe-activity',
  
  // Status Updates (from main to renderer)
  VAULT_STATUS_CHANGED = 'vault-status-changed',
  ACTIVITY_LOG_ENTRY = 'activity-log-entry',
  ERROR_NOTIFICATION = 'error-notification',
  PROFILE_UPDATED = 'profile-updated',
  FOLDER_STATUS_UPDATE = 'folder-status-update'
}

/**
 * IPC message structure
 */
export interface IPCMessage {
  type: IPCMessageType;
  payload: any;
  requestId?: string;
}

/**
 * IPC request/response wrapper
 */
export interface IPCRequest<T = any> {
  type: IPCMessageType;
  data: T;
}

export interface IPCResponse<T = any> {
  success: boolean;
  data?: T;
  error?: string;
}

// ==================== FORM VALIDATION ====================

/**
 * Form validation result
 */
export interface ValidationResult {
  isValid: boolean;
  errors: ValidationError[];
}

/**
 * Individual validation error
 */
export interface ValidationError {
  field: string;
  message: string;
  code?: string;
}

/**
 * Password strength assessment
 */
export interface PasswordStrength {
  score: number; // 0-4
  feedback: string[];
  requirements: {
    minLength: boolean;
    hasUppercase: boolean;
    hasLowercase: boolean;
    hasNumbers: boolean;
    hasSpecialChars: boolean;
  };
}

// ==================== ERROR HANDLING ====================

/**
 * Application error types
 */
export type ErrorType = 'network' | 'validation' | 'system' | 'security' | 'unknown';

/**
 * Structured error information
 */
export interface AppError {
  type: ErrorType;
  code: string;
  message: string;
  details?: Record<string, any>;
  timestamp: Date;
  recoverable: boolean;
}

/**
 * IPC communication error
 */
export interface IPCError extends AppError {
  type: 'network';
  requestType: IPCMessageType;
  timeout?: boolean;
}

/**
 * Validation error for forms
 */
export interface FormValidationError extends AppError {
  type: 'validation';
  field: string;
}

/**
 * System-level error
 */
export interface SystemError extends AppError {
  type: 'system';
  systemCode?: number;
  systemMessage?: string;
}

/**
 * Security-related error
 */
export interface SecurityError extends AppError {
  type: 'security';
  severity: 'low' | 'medium' | 'high' | 'critical';
}

// ==================== COMPONENT PROPS ====================

/**
 * Common props for vault-related components
 */
export interface VaultComponentProps {
  vault?: VaultInfo;
  onVaultAction?: (action: VaultAction, vaultId: string) => void;
  loading?: boolean;
  error?: AppError | null;
}

/**
 * Dashboard component props
 */
export interface VaultDashboardProps {
  vaults: VaultInfo[];
  onVaultAction: (action: VaultAction, vaultId: string) => void;
  refreshInterval?: number;
  loading?: boolean;
}

/**
 * Vault creation wizard props
 */
export interface VaultCreationWizardProps {
  onComplete: (config: VaultConfig) => void;
  onCancel: () => void;
  initialData?: Partial<VaultConfig>;
}

/**
 * Vault editor props
 */
export interface VaultEditorProps {
  vault: VaultInfo;
  onSave: (config: VaultConfig) => void;
  onCancel: () => void;
  readOnly?: boolean;
}

/**
 * Activity monitor props
 */
export interface ActivityMonitorProps {
  maxEntries?: number;
  filterLevel?: LogLevel;
  autoScroll?: boolean;
  height?: number;
}

/**
 * Settings panel props
 */
export interface SettingsPanelProps {
  settings: AppSettings;
  onSettingsChange: (settings: Partial<AppSettings>) => void;
  onSave: () => void;
  onReset: () => void;
}

// ==================== HOOKS AND UTILITIES ====================

/**
 * Vault operations hook return type
 */
export interface UseVaultOperations {
  // Operations
  createVault: (config: VaultConfig) => Promise<VaultOperationResult>;
  mountVault: (vaultId: string, password?: string) => Promise<VaultOperationResult>;
  unmountVault: (vaultId: string) => Promise<VaultOperationResult>;
  deleteVault: (vaultId: string) => Promise<VaultOperationResult>;
  lockVault: (vaultId: string) => Promise<VaultOperationResult>;
  unlockVault: (vaultId: string, password: string, mode?: UnlockMode) => Promise<VaultOperationResult>;
  
  // Bulk operations
  mountMultiple: (vaultIds: string[], password?: string) => Promise<BulkOperationResult>;
  unmountMultiple: (vaultIds: string[]) => Promise<BulkOperationResult>;
  
  // State
  loading: boolean;
  error: AppError | null;
}

/**
 * IPC communication hook return type
 */
export interface UseIPCCommunication {
  // Send requests
  sendRequest: <T = any>(type: IPCMessageType, data?: any) => Promise<IPCResponse<T>>;
  
  // Subscribe to events
  subscribe: (type: IPCMessageType, callback: (data: any) => void) => () => void;
  
  // Connection state
  isConnected: boolean;
  connectionError: AppError | null;
}

/**
 * Activity monitor hook return type
 */
export interface UseActivityMonitor {
  // Log data
  logs: LogEntry[];
  filteredLogs: LogEntry[];
  
  // Filtering
  filter: LogFilter;
  setFilter: (filter: Partial<LogFilter>) => void;
  clearFilter: () => void;
  
  // Operations
  refresh: () => Promise<void>;
  exportLogs: (format: 'json' | 'csv') => Promise<void>;
  
  // State
  loading: boolean;
  error: AppError | null;
}

// ==================== CONTEXT TYPES ====================

/**
 * Vault context state
 */
export interface VaultContextState {
  vaults: VaultInfo[];
  selectedVault: VaultInfo | null;
  profiles: VaultProfile[];
  activeProfile: VaultProfile | null;
  loading: boolean;
  error: AppError | null;
}

/**
 * Vault context actions
 */
export interface VaultContextActions {
  // Vault operations
  loadVaults: () => Promise<void>;
  selectVault: (vaultId: string | null) => void;
  createVault: (config: VaultConfig) => Promise<VaultOperationResult>;
  updateVault: (vaultId: string, config: Partial<VaultConfig>) => Promise<VaultOperationResult>;
  deleteVault: (vaultId: string) => Promise<VaultOperationResult>;
  
  // Profile operations
  loadProfiles: () => Promise<void>;
  setActiveProfile: (profileId: string) => Promise<void>;
  createProfile: (name: string, password: string, recoveryKey: string) => Promise<VaultOperationResult>;
  
  // Error handling
  clearError: () => void;
}

/**
 * App context state
 */
export interface AppContextState {
  // UI state
  activeView: AppView;
  notifications: Notification[];
  settings: AppSettings;
  
  // Authentication
  isAuthenticated: boolean;
  
  // Global loading/error state
  globalLoading: boolean;
  globalError: AppError | null;
}

/**
 * App context actions
 */
export interface AppContextActions {
  // Navigation
  setActiveView: (view: AppView) => void;
  
  // Notifications
  addNotification: (notification: Omit<Notification, 'id' | 'timestamp'>) => void;
  removeNotification: (id: string) => void;
  clearNotifications: () => void;
  
  // Settings
  updateSettings: (settings: Partial<AppSettings>) => Promise<void>;
  resetSettings: () => Promise<void>;
  
  // Authentication
  authenticate: (password: string) => Promise<boolean>;
  logout: () => void;
  
  // Global state
  setGlobalLoading: (loading: boolean) => void;
  setGlobalError: (error: AppError | null) => void;
}

// ==================== EXPORT ALL TYPES ====================

export type {
  // Re-export from electron.d.ts for convenience
  PhantomVaultAPI,
  PhantomVaultNativeAPI,
  PhantomVaultProfileAPI,
  PhantomVaultFolderAPI,
  NativeOperationResult,
  FileAttributes,
  PartialFileAttributes
} from './electron';