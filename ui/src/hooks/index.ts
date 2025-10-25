/**
 * Hooks Index
 * 
 * Central export point for all custom React hooks
 */

// ==================== HOOK EXPORTS ====================

export { useVaultOperations } from './useVaultOperations';
export { useIPCCommunication } from './useIPCCommunication';
export { useActivityMonitor } from './useActivityMonitor';
export { useServiceConnection } from './useServiceConnection';
export { 
  useVaultValidation, 
  useFieldValidation, 
  usePasswordValidation 
} from './useVaultValidation';
export { 
  useVaultStatusMonitor, 
  useVaultMonitor, 
  useVaultStatusAnimations 
} from './useVaultStatusMonitor';
// Temporarily commented out due to build issues
// export { 
//   useKeyboardSequences, 
//   useVaultKeyboardSequences, 
//   useSequenceValidation 
// } from './useKeyboardSequences';

// ==================== COMBINED HOOKS ====================

/**
 * Combined hook that provides all vault-related functionality
 * Useful for components that need multiple vault operations
 */
export const useVaultManagement = () => {
  const vaultOps = useVaultOperations();
  const ipc = useIPCCommunication();
  const activity = useActivityMonitor();
  
  return {
    vaultOperations: vaultOps,
    ipcCommunication: ipc,
    activityMonitor: activity,
  };
};

// ==================== UTILITY HOOKS ====================

import { useState, useCallback, useEffect, useRef } from 'react';
import { useActivityMonitor } from './useActivityMonitor';
import { useIPCCommunication } from './useIPCCommunication';
import { useVaultOperations } from './useVaultOperations';

/**
 * Hook for debouncing values
 * Useful for search inputs and API calls
 */
export const useDebounce = <T>(value: T, delay: number): T => {
  const [debouncedValue, setDebouncedValue] = useState<T>(value);

  useEffect(() => {
    const handler = setTimeout(() => {
      setDebouncedValue(value);
    }, delay);

    return () => {
      clearTimeout(handler);
    };
  }, [value, delay]);

  return debouncedValue;
};

/**
 * Hook for managing async operations with loading and error states
 */
export const useAsync = <T, Args extends any[]>(
  asyncFunction: (...args: Args) => Promise<T>
) => {
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<Error | null>(null);
  const [data, setData] = useState<T | null>(null);

  const execute = useCallback(async (...args: Args) => {
    try {
      setLoading(true);
      setError(null);
      const result = await asyncFunction(...args);
      setData(result);
      return result;
    } catch (err) {
      const error = err instanceof Error ? err : new Error('Unknown error');
      setError(error);
      throw error;
    } finally {
      setLoading(false);
    }
  }, [asyncFunction]);

  const reset = useCallback(() => {
    setLoading(false);
    setError(null);
    setData(null);
  }, []);

  return {
    execute,
    loading,
    error,
    data,
    reset,
  };
};

/**
 * Hook for managing local storage with type safety
 */
export const useLocalStorage = <T>(
  key: string,
  initialValue: T
): [T, (value: T | ((val: T) => T)) => void] => {
  const [storedValue, setStoredValue] = useState<T>(() => {
    try {
      const item = window.localStorage.getItem(key);
      return item ? JSON.parse(item) : initialValue;
    } catch (error) {
      console.warn(`Error reading localStorage key "${key}":`, error);
      return initialValue;
    }
  });

  const setValue = useCallback((value: T | ((val: T) => T)) => {
    try {
      const valueToStore = value instanceof Function ? value(storedValue) : value;
      setStoredValue(valueToStore);
      window.localStorage.setItem(key, JSON.stringify(valueToStore));
    } catch (error) {
      console.warn(`Error setting localStorage key "${key}":`, error);
    }
  }, [key, storedValue]);

  return [storedValue, setValue];
};

/**
 * Hook for managing previous values
 * Useful for comparing current and previous states
 */
export const usePrevious = <T>(value: T): T | undefined => {
  const ref = useRef<T>();
  
  useEffect(() => {
    ref.current = value;
  });
  
  return ref.current;
};

/**
 * Hook for managing intervals with cleanup
 */
export const useInterval = (callback: () => void, delay: number | null) => {
  const savedCallback = useRef(callback);

  useEffect(() => {
    savedCallback.current = callback;
  }, [callback]);

  useEffect(() => {
    if (delay === null) return;

    const id = setInterval(() => savedCallback.current(), delay);
    return () => clearInterval(id);
  }, [delay]);
};

/**
 * Hook for managing timeouts with cleanup
 */
export const useTimeout = (callback: () => void, delay: number | null) => {
  const savedCallback = useRef(callback);

  useEffect(() => {
    savedCallback.current = callback;
  }, [callback]);

  useEffect(() => {
    if (delay === null) return;

    const id = setTimeout(() => savedCallback.current(), delay);
    return () => clearTimeout(id);
  }, [delay]);
};

/**
 * Hook for managing window size
 */
export const useWindowSize = () => {
  const [windowSize, setWindowSize] = useState({
    width: window.innerWidth,
    height: window.innerHeight,
  });

  useEffect(() => {
    const handleResize = () => {
      setWindowSize({
        width: window.innerWidth,
        height: window.innerHeight,
      });
    };

    window.addEventListener('resize', handleResize);
    return () => window.removeEventListener('resize', handleResize);
  }, []);

  return windowSize;
};

/**
 * Hook for managing keyboard shortcuts
 */
export const useKeyboardShortcut = (
  keys: string[],
  callback: () => void,
  options: { preventDefault?: boolean; stopPropagation?: boolean } = {}
) => {
  const { preventDefault = true, stopPropagation = true } = options;

  useEffect(() => {
    const handleKeyDown = (event: KeyboardEvent) => {
      const pressedKeys = [];
      
      if (event.ctrlKey) pressedKeys.push('ctrl');
      if (event.metaKey) pressedKeys.push('meta');
      if (event.shiftKey) pressedKeys.push('shift');
      if (event.altKey) pressedKeys.push('alt');
      
      pressedKeys.push(event.key.toLowerCase());
      
      const normalizedKeys = keys.map(key => key.toLowerCase());
      const isMatch = normalizedKeys.every(key => pressedKeys.includes(key)) &&
                     pressedKeys.length === normalizedKeys.length;
      
      if (isMatch) {
        if (preventDefault) event.preventDefault();
        if (stopPropagation) event.stopPropagation();
        callback();
      }
    };

    document.addEventListener('keydown', handleKeyDown);
    return () => document.removeEventListener('keydown', handleKeyDown);
  }, [keys, callback, preventDefault, stopPropagation]);
};

// ==================== TYPE EXPORTS ====================

export type {
  UseVaultOperations,
  UseIPCCommunication,
  UseActivityMonitor,
} from '../types';