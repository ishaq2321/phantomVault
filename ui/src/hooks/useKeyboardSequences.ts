// Temporarily disabled for build compatibility
import { useState, useCallback } from 'react';
import { AppError } from '../types';
import { useApp } from '../contexts';

export interface UseKeyboardSequencesOptions {
  autoLoad?: boolean;
  vaultId?: string;
}

export interface UseKeyboardSequencesReturn {
  // Data
  sequences: any[];
  allSequences: any[];
  loading: boolean;
  error: AppError | null;
  
  // Actions
  loadSequences: () => Promise<void>;
  createSequence: (data: any) => Promise<any>;
  updateSequence: (id: string, data: any) => Promise<any>;
  deleteSequence: (id: string) => Promise<void>;
  validateSequence: (sequence: string, excludeVaultId?: string) => any;
  findConflicts: (sequence: string, excludeVaultId?: string) => any[];
  
  // Utilities
  refresh: () => Promise<void>;
  clearError: () => void;
}

/**
 * Hook for managing keyboard sequences (STUB VERSION)
 */
export const useKeyboardSequences = (
  options: UseKeyboardSequencesOptions = {}
): UseKeyboardSequencesReturn => {
  const [sequences, setSequences] = useState<any[]>([]);
  const [allSequences, setAllSequences] = useState<any[]>([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<AppError | null>(null);

  const loadSequences = useCallback(async () => {
    console.log('[useKeyboardSequences] loadSequences (stub)');
  }, []);

  const createSequence = useCallback(async (data: any) => {
    console.log('[useKeyboardSequences] createSequence (stub)', data);
    return { success: true, id: Date.now().toString() };
  }, []);

  const updateSequence = useCallback(async (id: string, data: any) => {
    console.log('[useKeyboardSequences] updateSequence (stub)', id, data);
    return { success: true };
  }, []);

  const deleteSequence = useCallback(async (id: string) => {
    console.log('[useKeyboardSequences] deleteSequence (stub)', id);
  }, []);

  const validateSequence = useCallback((sequence: string, excludeVaultId?: string) => {
    return {
      isValid: sequence && sequence.length >= 2,
      errors: sequence && sequence.length >= 2 ? [] : ['Sequence too short']
    };
  }, []);

  const findConflicts = useCallback((sequence: string, excludeVaultId?: string) => {
    return [];
  }, []);

  const refresh = useCallback(async () => {
    await loadSequences();
  }, [loadSequences]);

  const clearError = useCallback(() => {
    setError(null);
  }, []);

  return {
    sequences,
    allSequences,
    loading,
    error,
    loadSequences,
    createSequence,
    updateSequence,
    deleteSequence,
    validateSequence,
    findConflicts,
    refresh,
    clearError
  };
};

/**
 * Hook for managing sequences for a specific vault (STUB VERSION)
 */
export const useVaultKeyboardSequences = (vaultId: string, vaultName: string) => {
  const sequences = useKeyboardSequences({ vaultId });
  
  const setVaultSequence = useCallback(async (sequence: string) => {
    console.log('[useVaultKeyboardSequences] setVaultSequence (stub)', vaultId, sequence);
    return { success: true };
  }, [vaultId]);

  return {
    ...sequences,
    vaultId,
    vaultName,
    setVaultSequence
  };
};

/**
 * Hook for sequence validation with real-time feedback (STUB VERSION)
 */
export const useSequenceValidation = (
  sequence: string,
  excludeVaultId?: string
) => {
  const { validateSequence, findConflicts } = useKeyboardSequences();
  
  const validation = validateSequence(sequence, excludeVaultId);
  const conflicts = findConflicts(sequence, excludeVaultId);

  return {
    ...validation,
    conflicts,
    hasConflicts: conflicts.length > 0
  };
};