/**
 * App Context
 * 
 * React Context for managing global application state
 * Handles UI state, notifications, settings, and authentication
 */

import React, { createContext, useContext, useReducer, useCallback, useEffect } from 'react';
import {
  AppView,
  Notification,
  AppSettings,
  AppError,
  AppContextState,
  AppContextActions,
  Theme,
  LogLevel,
  NotificationId
} from '../types';

// ==================== CONTEXT STATE ====================

interface AppState extends AppContextState {
  // Additional internal state
  notificationCounter: number;
  settingsLoaded: boolean;
}

// ==================== ACTIONS ====================

type AppAction =
  | { type: 'SET_ACTIVE_VIEW'; payload: AppView }
  | { type: 'ADD_NOTIFICATION'; payload: Omit<Notification, 'id' | 'timestamp'> }
  | { type: 'REMOVE_NOTIFICATION'; payload: string }
  | { type: 'CLEAR_NOTIFICATIONS' }
  | { type: 'SET_SETTINGS'; payload: AppSettings }
  | { type: 'UPDATE_SETTINGS'; payload: Partial<AppSettings> }
  | { type: 'SET_AUTHENTICATED'; payload: boolean }
  | { type: 'SET_GLOBAL_LOADING'; payload: boolean }
  | { type: 'SET_GLOBAL_ERROR'; payload: AppError | null }
  | { type: 'SET_SETTINGS_LOADED'; payload: boolean };

// ==================== REDUCER ====================

const defaultSettings: AppSettings = {
  // General
  autoStart: false,
  minimizeToTray: true,
  closeToTray: true,
  
  // Security
  autoLockTimeout: 15, // 15 minutes
  requirePasswordConfirmation: true,
  clearClipboardTimeout: 30, // 30 seconds
  
  // UI
  theme: 'system',
  language: 'en',
  showNotifications: true,
  
  // Monitoring
  enableActivityLogging: true,
  maxLogEntries: 1000,
  logLevel: 'info',
};

const initialState: AppState = {
  // UI state
  activeView: 'dashboard',
  notifications: [],
  settings: defaultSettings,
  
  // Authentication
  isAuthenticated: false,
  
  // Global loading/error state
  globalLoading: false,
  globalError: null,
  
  // Internal state
  notificationCounter: 0,
  settingsLoaded: false,
};

function appReducer(state: AppState, action: AppAction): AppState {
  switch (action.type) {
    case 'SET_ACTIVE_VIEW':
      return {
        ...state,
        activeView: action.payload,
      };

    case 'ADD_NOTIFICATION': {
      const id = `notification-${state.notificationCounter + 1}` as NotificationId;
      const notification: Notification = {
        ...action.payload,
        id,
        timestamp: new Date(),
      };

      return {
        ...state,
        notifications: [...state.notifications, notification],
        notificationCounter: state.notificationCounter + 1,
      };
    }

    case 'REMOVE_NOTIFICATION':
      return {
        ...state,
        notifications: state.notifications.filter(n => n.id !== action.payload),
      };

    case 'CLEAR_NOTIFICATIONS':
      return {
        ...state,
        notifications: [],
      };

    case 'SET_SETTINGS':
      return {
        ...state,
        settings: action.payload,
        settingsLoaded: true,
      };

    case 'UPDATE_SETTINGS':
      return {
        ...state,
        settings: {
          ...state.settings,
          ...action.payload,
        },
      };

    case 'SET_AUTHENTICATED':
      return {
        ...state,
        isAuthenticated: action.payload,
      };

    case 'SET_GLOBAL_LOADING':
      return {
        ...state,
        globalLoading: action.payload,
      };

    case 'SET_GLOBAL_ERROR':
      return {
        ...state,
        globalError: action.payload,
        globalLoading: false,
      };

    case 'SET_SETTINGS_LOADED':
      return {
        ...state,
        settingsLoaded: action.payload,
      };

    default:
      return state;
  }
}

// ==================== CONTEXT CREATION ====================

interface AppContextValue {
  state: AppContextState;
  actions: AppContextActions;
}

const AppContext = createContext<AppContextValue | undefined>(undefined);

// ==================== PROVIDER COMPONENT ====================

interface AppProviderProps {
  children: React.ReactNode;
}

export const AppProvider: React.FC<AppProviderProps> = ({ children }) => {
  const [state, dispatch] = useReducer(appReducer, initialState);

  // ==================== ACTION IMPLEMENTATIONS ====================

  const setActiveView = useCallback((view: AppView): void => {
    dispatch({ type: 'SET_ACTIVE_VIEW', payload: view });
  }, []);

  const addNotification = useCallback((
    notification: Omit<Notification, 'id' | 'timestamp'>
  ): void => {
    dispatch({ type: 'ADD_NOTIFICATION', payload: notification });

    // Auto-remove notification after duration (if specified)
    if (notification.duration) {
      setTimeout(() => {
        // Note: We can't access the generated ID here, so we'll need to implement
        // auto-removal differently in a real implementation
      }, notification.duration);
    }
  }, []);

  const removeNotification = useCallback((id: string): void => {
    dispatch({ type: 'REMOVE_NOTIFICATION', payload: id });
  }, []);

  const clearNotifications = useCallback((): void => {
    dispatch({ type: 'CLEAR_NOTIFICATIONS' });
  }, []);

  const updateSettings = useCallback(async (
    settings: Partial<AppSettings>
  ): Promise<void> => {
    try {
      dispatch({ type: 'SET_GLOBAL_LOADING', payload: true });
      dispatch({ type: 'SET_GLOBAL_ERROR', payload: null });

      // Update settings in the backend
      // Note: This would use IPC to communicate with the main process
      // For now, we'll just update the local state
      dispatch({ type: 'UPDATE_SETTINGS', payload: settings });

      // Show success notification
      addNotification({
        type: 'success',
        title: 'Settings Updated',
        message: 'Your settings have been saved successfully.',
        duration: 3000,
      });
    } catch (error) {
      const appError: AppError = {
        type: 'system',
        code: 'UPDATE_SETTINGS_FAILED',
        message: error instanceof Error ? error.message : 'Failed to update settings',
        timestamp: new Date(),
        recoverable: true,
      };
      dispatch({ type: 'SET_GLOBAL_ERROR', payload: appError });

      addNotification({
        type: 'error',
        title: 'Settings Update Failed',
        message: appError.message,
        duration: 5000,
      });
    } finally {
      dispatch({ type: 'SET_GLOBAL_LOADING', payload: false });
    }
  }, [addNotification]);

  const resetSettings = useCallback(async (): Promise<void> => {
    try {
      dispatch({ type: 'SET_GLOBAL_LOADING', payload: true });
      dispatch({ type: 'SET_GLOBAL_ERROR', payload: null });

      // Reset to default settings
      dispatch({ type: 'SET_SETTINGS', payload: defaultSettings });

      addNotification({
        type: 'info',
        title: 'Settings Reset',
        message: 'All settings have been reset to their default values.',
        duration: 3000,
      });
    } catch (error) {
      const appError: AppError = {
        type: 'system',
        code: 'RESET_SETTINGS_FAILED',
        message: error instanceof Error ? error.message : 'Failed to reset settings',
        timestamp: new Date(),
        recoverable: true,
      };
      dispatch({ type: 'SET_GLOBAL_ERROR', payload: appError });

      addNotification({
        type: 'error',
        title: 'Settings Reset Failed',
        message: appError.message,
        duration: 5000,
      });
    } finally {
      dispatch({ type: 'SET_GLOBAL_LOADING', payload: false });
    }
  }, [addNotification]);

  const authenticate = useCallback(async (password: string): Promise<boolean> => {
    try {
      dispatch({ type: 'SET_GLOBAL_LOADING', payload: true });
      dispatch({ type: 'SET_GLOBAL_ERROR', payload: null });

      // Verify password with the active profile
      // Note: This would use the vault context to get the active profile
      // For now, we'll simulate authentication
      
      // Simulate API call delay
      await new Promise(resolve => setTimeout(resolve, 1000));

      // For demo purposes, accept any non-empty password
      if (password.trim().length > 0) {
        dispatch({ type: 'SET_AUTHENTICATED', payload: true });
        
        addNotification({
          type: 'success',
          title: 'Authentication Successful',
          message: 'You have been successfully authenticated.',
          duration: 3000,
        });

        return true;
      } else {
        throw new Error('Invalid password');
      }
    } catch (error) {
      const appError: AppError = {
        type: 'security',
        code: 'AUTHENTICATION_FAILED',
        message: error instanceof Error ? error.message : 'Authentication failed',
        timestamp: new Date(),
        recoverable: true,
        severity: 'medium',
      };
      dispatch({ type: 'SET_GLOBAL_ERROR', payload: appError });

      addNotification({
        type: 'error',
        title: 'Authentication Failed',
        message: appError.message,
        duration: 5000,
      });

      return false;
    } finally {
      dispatch({ type: 'SET_GLOBAL_LOADING', payload: false });
    }
  }, [addNotification]);

  const logout = useCallback((): void => {
    dispatch({ type: 'SET_AUTHENTICATED', payload: false });
    dispatch({ type: 'SET_ACTIVE_VIEW', payload: 'dashboard' });
    
    addNotification({
      type: 'info',
      title: 'Logged Out',
      message: 'You have been successfully logged out.',
      duration: 3000,
    });
  }, [addNotification]);

  const setGlobalLoading = useCallback((loading: boolean): void => {
    dispatch({ type: 'SET_GLOBAL_LOADING', payload: loading });
  }, []);

  const setGlobalError = useCallback((error: AppError | null): void => {
    dispatch({ type: 'SET_GLOBAL_ERROR', payload: error });
  }, []);

  // ==================== EFFECTS ====================

  // Load settings on mount
  useEffect(() => {
    const loadSettings = async () => {
      try {
        // In a real implementation, this would load settings from the main process
        // For now, we'll use the default settings
        dispatch({ type: 'SET_SETTINGS', payload: defaultSettings });
      } catch (error) {
        console.error('Failed to load settings:', error);
        // Use default settings if loading fails
        dispatch({ type: 'SET_SETTINGS', payload: defaultSettings });
      }
    };

    loadSettings();
  }, []);

  // Apply theme changes
  useEffect(() => {
    const applyTheme = (theme: Theme) => {
      const root = document.documentElement;
      
      if (theme === 'system') {
        // Use system preference
        const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
        root.setAttribute('data-theme', prefersDark ? 'dark' : 'light');
      } else {
        root.setAttribute('data-theme', theme);
      }
    };

    applyTheme(state.settings.theme);

    // Listen for system theme changes if using system theme
    if (state.settings.theme === 'system') {
      const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');
      const handleChange = () => applyTheme('system');
      
      mediaQuery.addEventListener('change', handleChange);
      return () => mediaQuery.removeEventListener('change', handleChange);
    }
  }, [state.settings.theme]);

  // Auto-remove notifications with duration
  useEffect(() => {
    const timers: NodeJS.Timeout[] = [];

    state.notifications.forEach(notification => {
      if (notification.duration) {
        const timer = setTimeout(() => {
          removeNotification(notification.id);
        }, notification.duration);
        timers.push(timer);
      }
    });

    return () => {
      timers.forEach(timer => clearTimeout(timer));
    };
  }, [state.notifications, removeNotification]);

  // ==================== CONTEXT VALUE ====================

  const contextValue: AppContextValue = {
    state: {
      activeView: state.activeView,
      notifications: state.notifications,
      settings: state.settings,
      isAuthenticated: state.isAuthenticated,
      globalLoading: state.globalLoading,
      globalError: state.globalError,
    },
    actions: {
      setActiveView,
      addNotification,
      removeNotification,
      clearNotifications,
      updateSettings,
      resetSettings,
      authenticate,
      logout,
      setGlobalLoading,
      setGlobalError,
    },
  };

  return (
    <AppContext.Provider value={contextValue}>
      {children}
    </AppContext.Provider>
  );
};

// ==================== HOOK ====================

export const useApp = (): AppContextValue => {
  const context = useContext(AppContext);
  if (context === undefined) {
    throw new Error('useApp must be used within an AppProvider');
  }
  return context;
};

// ==================== EXPORTS ====================

export default AppContext;