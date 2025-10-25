/**
 * Keyboard Sequences Hook
 * 
 * React hook for managing keyboard sequences with validation and conflict detection
 */

import { useState, useEffect, useCallback } from 'react';
import {
  KeyboardSequence,
  SequenceValidationResult,
  SequenceConflict,
  KeyboardSequenceManager,
  getKeyboardSequenceManager
} from '../services/KeyboardSequenceManager';
import { AppError } from '../types';
import { useApp } from '../contexts';

export interface UseKeyboardSequencesOptions {
  autoLoad?: boolean;
  vaultId?: string;
}

export interface UseKeyboardSequencesReturn {
  // Data
  sequences: KeyboardSequence[];
  allSequences: KeyboardSequence[];
  loading: boolean;
  error: AppError | null;
  
  // Operations
  setSequence: (vaultId: string, vaultName: string, sequence: string, mode?: 'temporary' | 'permanent' | 'auto') => Promise<KeyboardSequence>;
  removeSequence: (sequenceId: string) => Promise<boolean>;
  removeSequencesForVault: (vaultId: string) => Promise<number>;
  
  // Validation
  validateSequence: (sequence: string, excludeVaultId?: string) => SequenceValidationResult;
  findConflicts: (sequence: string, excludeVaultId?: string) => SequenceConflict[];
  
  // Utilities
  generateSuggestions: (vaultName: string, count?: number) => string[];
  parseSequence: (sequence: string) => { mode: 'temporary' | 'permanent' | 'default'; password: string; isValid: boolean };
  recordUsage: (sequenceId: string) => Promise<void>;
  
  // Statistics
  getUsageStats: () => {
    totalSequences: number;
    activeSequences: number;
    totalUsage: number;
    mostUsedSequence?: KeyboardSequence;
    recentlyUsedSequences: KeyboardSequence[];
  };
  
  // Control
  refresh: () => Promise<void>;
}

/**
 * Hook for managing keyboard sequences
 */
export const useKeyboardSequences = (
  options: UseKeyboardSequencesOptions = {}
): UseKeyboardSequencesReturn => {
  const { autoLoad = true, vaultId } = options;
  const { actions: appActions } = useApp();
  
  // State
  const [sequences, setSequences] = useState<KeyboardSequence[]>([]);
  const [allSequences, setAllSequences] = useState<KeyboardSequence[]>([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<AppError | null>(null);
  const [manager, setManager] = useState<KeyboardSequenceManager | null>(null);

  // ==================== INITIALIZATION ====================

  const initializeManager = useCallback(async () => {
    try {
      setLoading(true);
      setError(null);
      
      const sequenceManager = getKeyboardSequenceManager();
      await sequenceManager.initialize();
      setManager(sequenceManager);
      
      // Load sequences
      const allSeqs = sequenceManager.getAllSequences();
      setAllSequences(allSeqs);
      
      if (vaultId) {
        const vaultSeqs = sequenceManager.getSequencesForVault(vaultId);
        setSequences(vaultSeqs);
      } else {
        setSequences(allSeqs);
      }
    } catch (err) {
      const appError: AppError = {
        type: 'system',
        code: 'SEQUENCE_MANAGER_INIT_FAILED',
        message: err instanceof Error ? err.message : 'Failed to initialize sequence manager',
        timestamp: new Date(),
        recoverable: true,
      };
      setError(appError);
    } finally {
      setLoading(false);
    }
  }, [vaultId]);

  // ==================== OPERATIONS ====================

  const setSequence = useCallback(async (
    vaultId: string,
    vaultName: string,
    sequence: string,
    mode: 'temporary' | 'permanent' | 'auto' = 'auto'
  ): Promise<KeyboardSequence> => {
    if (!manager) {
      throw new Error('Sequence manager not initialized');
    }

    try {
      setLoading(true);
      setError(null);

      const newSequence = await manager.setSequence(vaultId, vaultName, sequence, mode);
      
      // Refresh sequences
      await refresh();
      
      appActions.addNotification({
        type: 'success',
        title: 'Sequence Updated',
        message: `Keyboard sequence set for "${vaultName}"`,
        duration: 3000,
      });

      return newSequence;
    } catch (err) {
      const appError: AppError = {
        type: 'validation',
        code: 'SET_SEQUENCE_FAILED',
        message: err instanceof Error ? err.message : 'Failed to set sequence',
        timestamp: new Date(),
        recoverable: true,
      };
      setError(appError);
      
      appActions.addNotification({
        type: 'error',
        title: 'Sequence Error',
        message: appError.message,
        duration: 5000,
      });
      
      throw appError;
    } finally {
      setLoading(false);
    }
  }, [manager, appActions]);

  const removeSequence = useCallback(async (sequenceId: string): Promise<boolean> => {
    if (!manager) {
      throw new Error('Sequence manager not initialized');
    }

    try {
      setLoading(true);
      setError(null);

      const sequence = manager.getSequence(sequenceId);
      const result = await manager.removeSequence(sequenceId);
      
      if (result) {
        // Refresh sequences
        await refresh();
        
        appActions.addNotification({
          type: 'success',
          title: 'Sequence Removed',
          message: sequence ? `Removed sequence for "${sequence.vaultName}"` : 'Sequence removed',
          duration: 3000,
        });
      }

      return result;
    } catch (err) {
      const appError: AppError = {
        type: 'system',
        code: 'REMOVE_SEQUENCE_FAILED',
        message: err instanceof Error ? err.message : 'Failed to remove sequence',
        timestamp: new Date(),
        recoverable: true,
      };
      setError(appError);
      
      appActions.addNotification({
        type: 'error',
        title: 'Remove Failed',
        message: appError.message,
        duration: 5000,
      });
      
      throw appError;
    } finally {
      setLoading(false);
    }
  }, [manager, appActions]);

  const removeSequencesForVault = useCallback(async (vaultId: string): Promise<number> => {
    if (!manager) {
      throw new Error('Sequence manager not initialized');
    }

    try {
      setLoading(true);
      setError(null);

      const count = await manager.removeSequencesForVault(vaultId);
      
      if (count > 0) {
        // Refresh sequences
        await refresh();
        
        appActions.addNotification({
          type: 'success',
          title: 'Sequences Removed',
          message: `Removed ${count} sequence(s)`,
          duration: 3000,
        });
      }

      return count;
    } catch (err) {
      const appError: AppError = {
        type: 'system',
        code: 'REMOVE_VAULT_SEQUENCES_FAILED',
        message: err instanceof Error ? err.message : 'Failed to remove vault sequences',
        timestamp: new Date(),
        recoverable: true,
      };
      setError(appError);
      
      appActions.addNotification({
        type: 'error',
        title: 'Remove Failed',
        message: appError.message,
        duration: 5000,
      });
      
      throw appError;
    } finally {
      setLoading(false);
    }
  }, [manager, appActions]);

  // ==================== VALIDATION ====================

  const validateSequence = useCallback((
    sequence: string,
    excludeVaultId?: string
  ): SequenceValidationResult => {
    if (!manager) {
      return {
        isValid: false,
        errors: ['Sequence manager not initialized'],
        warnings: [],
        suggestions: [],
      };
    }

    return manager.validateSequence(sequence, excludeVaultId);
  }, [manager]);

  const findConflicts = useCallback((
    sequence: string,
    excludeVaultId?: string
  ): SequenceConflict[] => {
    if (!manager) {
      return [];
    }

    return manager.findConflicts(sequence, excludeVaultId);
  }, [manager]);

  // ==================== UTILITIES ====================

  const generateSuggestions = useCallback((
    vaultName: string,
    count: number = 5
  ): string[] => {
    if (!manager) {
      return [];
    }

    return manager.generateSuggestions(vaultName, count);
  }, [manager]);

  const parseSequence = useCallback((sequence: string) => {
    if (!manager) {
      return { mode: 'default' as const, password: '', isValid: false };
    }

    return manager.parseSequence(sequence);
  }, [manager]);

  const recordUsage = useCallback(async (sequenceId: string): Promise<void> => {
    if (!manager) {
      return;
    }

    try {
      await manager.recordUsage(sequenceId);
      // Refresh sequences to update usage stats
      await refresh();
    } catch (err) {
      console.error('Failed to record sequence usage:', err);
    }
  }, [manager]);

  const getUsageStats = useCallback(() => {
    if (!manager) {
      return {
        totalSequences: 0,
        activeSequences: 0,
        totalUsage: 0,
        mostUsedSequence: undefined,
        recentlyUsedSequences: [],
      };
    }

    return manager.getUsageStats();
  }, [manager]);

  // ==================== REFRESH ====================

  const refresh = useCallback(async (): Promise<void> => {
    if (!manager) {
      return;
    }

    try {
      const allSeqs = manager.getAllSequences();
      setAllSequences(allSeqs);
      
      if (vaultId) {
        const vaultSeqs = manager.getSequencesForVault(vaultId);
        setSequences(vaultSeqs);
      } else {
        setSequences(allSeqs);
      }
    } catch (err) {
      console.error('Failed to refresh sequences:', err);
    }
  }, [manager, vaultId]);

  // ==================== EFFECTS ====================

  // Initialize manager on mount
  useEffect(() => {
    if (autoLoad) {
      initializeManager();
    }
  }, [autoLoad, initializeManager]);

  // Update sequences when vaultId changes
  useEffect(() => {
    if (manager) {
      refresh();
    }
  }, [manager, vaultId, refresh]);

  // ==================== RETURN HOOK INTERFACE ====================

  return {
    // Data
    sequences,
    allSequences,
    loading,
    error,
    
    // Operations
    setSequence,
    removeSequence,
    removeSequencesForVault,
    
    // Validation
    validateSequence,
    findConflicts,
    
    // Utilities
    generateSuggestions,
    parseSequence,
    recordUsage,
    
    // Statistics
    getUsageStats,
    
    // Control
    refresh,
  };
};

// ==================== SPECIALIZED HOOKS ====================

/**
 * Hook for managing sequences for a specific vault
 */
export const useVaultKeyboardSequences = (vaultId: string, vaultName: string) => {
  const sequences = useKeyboardSequences({ vaultId });
  
  const setVaultSequence = useCallback(async (
    sequence: string,
    mode?: 'temporary' | 'permanent' | 'auto'
  ) => {
    return sequences.setSequence(vaultId, vaultName, sequence, mode);
  }, [sequences.setSequence, vaultId, vaultName]);

  const removeVaultSequences = useCallback(async () => {
    return sequences.removeSequencesForVault(vaultId);
  }, [sequences.removeSequencesForVault, vaultId]);

  const validateVaultSequence = useCallback((sequence: string) => {
    return sequences.validateSequence(sequence, vaultId);
  }, [sequences.validateSequence, vaultId]);

  const findVaultConflicts = useCallback((sequence: string) => {
    return sequences.findConflicts(sequence, vaultId);
  }, [sequences.findConflicts, vaultId]);

  const generateVaultSuggestions = useCallback((count?: number) => {
    return sequences.generateSuggestions(vaultName, count);
  }, [sequences.generateSuggestions, vaultName]);

  return {
    ...sequences,
    setVaultSequence,
    removeVaultSequences,
    validateVaultSequence,
    findVaultConflicts,
    generateVaultSuggestions,
  };
};

/**
 * Hook for sequence validation with real-time feedback
 */
export const useSequenceValidation = (
  sequence: string,
  excludeVaultId?: string
) => {
  const { validateSequence, findConflicts } = useKeyboardSequences();
  
  const validation = validateSequence(sequence, excludeVaultId);
  const conflicts = findConflicts(sequence, excludeVaultId);
  
  return {
    validation,
    conflicts,
    isValid: validation.isValid,
    hasErrors: validation.errors.length > 0,
    hasWarnings: validation.warnings.length > 0,
    hasConflicts: conflicts.length > 0,
    exactConflicts: conflicts.filter(c => c.conflictType === 'exact'),
    similarConflicts: conflicts.filter(c => c.conflictType === 'similar'),
  };
};