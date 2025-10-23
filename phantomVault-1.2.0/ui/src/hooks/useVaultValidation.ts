/**
 * Vault Validation Hook
 * 
 * React hook for real-time vault configuration validation
 */

import { useState, useCallback, useEffect, useMemo } from 'react';
import {
  VaultConfig,
  ValidationResult,
  ValidationError,
  PasswordStrength,
  VaultInfo
} from '../types';
import {
  validateVaultConfig,
  validateVaultName,
  validateVaultPassword,
  validateFolderPath,
  validateKeyboardSequence,
  validateLockTimeout,
  validatePasswordConfirmation,
  calculatePasswordStrength
} from '../utils/vaultValidation';

export interface UseVaultValidationOptions {
  existingVaults?: VaultInfo[];
  currentVaultId?: string;
  validateOnChange?: boolean;
  debounceMs?: number;
}

export interface UseVaultValidationReturn {
  // Validation state
  validation: ValidationResult;
  isValidating: boolean;
  
  // Field-specific validation
  validateField: (field: keyof VaultConfig, value: any) => ValidationResult;
  getFieldError: (field: string) => string | undefined;
  hasFieldError: (field: string) => boolean;
  
  // Full config validation
  validateConfig: (config: Partial<VaultConfig>) => ValidationResult;
  
  // Password utilities
  calculatePasswordStrength: (password: string) => PasswordStrength;
  validatePasswordMatch: (password: string, confirmPassword: string) => ValidationResult;
  
  // Real-time validation
  startValidation: (config: Partial<VaultConfig>) => void;
  clearValidation: () => void;
}

/**
 * Hook for vault configuration validation with real-time feedback
 */
export function useVaultValidation(options: UseVaultValidationOptions = {}): UseVaultValidationReturn {
  const {
    existingVaults = [],
    currentVaultId,
    validateOnChange = true,
    debounceMs = 300
  } = options;

  // ==================== STATE ====================

  const [validation, setValidation] = useState<ValidationResult>({
    isValid: true,
    errors: []
  });
  const [isValidating, setIsValidating] = useState(false);
  const [debounceTimer, setDebounceTimer] = useState<NodeJS.Timeout | null>(null);

  // ==================== VALIDATION FUNCTIONS ====================

  const validateField = useCallback((field: keyof VaultConfig, value: any): ValidationResult => {
    switch (field) {
      case 'name':
        return validateVaultName(value, existingVaults, currentVaultId);
      
      case 'password':
        return validateVaultPassword(value);
      
      case 'path':
        return validateFolderPath(value);
      
      case 'keyboardSequence':
        const existingSequences = existingVaults
          .filter(v => v.id !== currentVaultId)
          .map(v => v.name); // Simplified - would use actual sequences
        return validateKeyboardSequence(value, existingSequences, currentVaultId);
      
      case 'lockTimeout':
        return validateLockTimeout(value);
      
      default:
        return { isValid: true, errors: [] };
    }
  }, [existingVaults, currentVaultId]);

  const validateConfigFull = useCallback((config: Partial<VaultConfig>): ValidationResult => {
    return validateVaultConfig(config, existingVaults, currentVaultId);
  }, [existingVaults, currentVaultId]);

  const getFieldError = useCallback((fieldName: string): string | undefined => {
    return validation.errors.find(error => error.field === fieldName)?.message;
  }, [validation.errors]);

  const hasFieldError = useCallback((fieldName: string): boolean => {
    return validation.errors.some(error => error.field === fieldName);
  }, [validation.errors]);

  const validatePasswordMatch = useCallback((password: string, confirmPassword: string): ValidationResult => {
    return validatePasswordConfirmation(password, confirmPassword);
  }, []);

  // ==================== REAL-TIME VALIDATION ====================

  const startValidation = useCallback((config: Partial<VaultConfig>) => {
    if (!validateOnChange) return;

    // Clear existing timer
    if (debounceTimer) {
      clearTimeout(debounceTimer);
    }

    setIsValidating(true);

    // Debounce validation
    const timer = setTimeout(() => {
      const result = validateConfigFull(config);
      setValidation(result);
      setIsValidating(false);
    }, debounceMs);

    setDebounceTimer(timer);
  }, [validateOnChange, debounceTimer, debounceMs, validateConfigFull]);

  const clearValidation = useCallback(() => {
    setValidation({ isValid: true, errors: [] });
    setIsValidating(false);
    
    if (debounceTimer) {
      clearTimeout(debounceTimer);
      setDebounceTimer(null);
    }
  }, [debounceTimer]);

  // ==================== CLEANUP ====================

  useEffect(() => {
    return () => {
      if (debounceTimer) {
        clearTimeout(debounceTimer);
      }
    };
  }, [debounceTimer]);

  // ==================== RETURN ====================

  return {
    validation,
    isValidating,
    validateField,
    getFieldError,
    hasFieldError,
    validateConfig: validateConfigFull,
    calculatePasswordStrength,
    validatePasswordMatch,
    startValidation,
    clearValidation
  };
}

/**
 * Hook for validating a single field with real-time feedback
 */
export function useFieldValidation<T>(
  field: keyof VaultConfig,
  value: T,
  options: UseVaultValidationOptions = {}
) {
  const { validateField, getFieldError, hasFieldError } = useVaultValidation(options);
  
  const fieldValidation = useMemo(() => {
    return validateField(field, value);
  }, [validateField, field, value]);

  return {
    isValid: fieldValidation.isValid,
    error: fieldValidation.errors[0]?.message,
    hasError: fieldValidation.errors.length > 0,
    validation: fieldValidation
  };
}

/**
 * Hook for password strength validation with real-time feedback
 */
export function usePasswordValidation(password: string, confirmPassword?: string) {
  const strength = useMemo(() => {
    return password ? calculatePasswordStrength(password) : null;
  }, [password]);

  const passwordValidation = useMemo(() => {
    return validateVaultPassword(password);
  }, [password]);

  const confirmValidation = useMemo(() => {
    if (confirmPassword !== undefined) {
      return validatePasswordConfirmation(password, confirmPassword);
    }
    return { isValid: true, errors: [] };
  }, [password, confirmPassword]);

  return {
    strength,
    passwordValidation,
    confirmValidation,
    isPasswordValid: passwordValidation.isValid,
    isConfirmValid: confirmValidation.isValid,
    passwordError: passwordValidation.errors[0]?.message,
    confirmError: confirmValidation.errors[0]?.message
  };
}