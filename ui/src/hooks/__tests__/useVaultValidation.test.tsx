/**
 * Vault Validation Hook Tests
 * 
 * Unit tests for the useVaultValidation React hook
 */

import React from 'react';
import { renderHook, act } from '@testing-library/react';
import { useVaultValidation, useFieldValidation, usePasswordValidation } from '../useVaultValidation';
import { VaultInfo, VaultConfig } from '../../types';

// ==================== TEST DATA ====================

const mockVaults: VaultInfo[] = [
  {
    id: 'vault-1',
    name: 'Personal Documents',
    path: '/home/user/personal',
    status: 'unmounted',
    lastAccessed: new Date('2024-01-01'),
    size: 1024 * 1024 * 100,
    folderCount: 25
  },
  {
    id: 'vault-2',
    name: 'Work Files',
    path: '/home/user/work',
    status: 'mounted',
    lastAccessed: new Date('2024-01-02'),
    size: 1024 * 1024 * 500,
    folderCount: 50
  }
];

// ==================== MOCK TIMERS ====================

import { vi } from 'vitest';

beforeEach(() => {
  vi.useFakeTimers();
});

afterEach(() => {
  vi.runOnlyPendingTimers();
  vi.useRealTimers();
});

// ==================== useVaultValidation TESTS ====================

describe('useVaultValidation', () => {
  test('should initialize with valid state', () => {
    const { result } = renderHook(() => useVaultValidation());
    
    expect(result.current.validation.isValid).toBe(true);
    expect(result.current.validation.errors).toHaveLength(0);
    expect(result.current.isValidating).toBe(false);
  });

  test('should validate individual fields', () => {
    const { result } = renderHook(() => useVaultValidation({
      existingVaults: mockVaults
    }));

    // Test valid field
    const validResult = result.current.validateField('name', 'Valid Vault Name');
    expect(validResult.isValid).toBe(true);

    // Test invalid field
    const invalidResult = result.current.validateField('name', '');
    expect(invalidResult.isValid).toBe(false);
    expect(invalidResult.errors[0].field).toBe('name');
  });

  test('should validate complete configuration', () => {
    const { result } = renderHook(() => useVaultValidation({
      existingVaults: mockVaults
    }));

    const validConfig: Partial<VaultConfig> = {
      name: 'Test Vault',
      path: '/home/user/test',
      password: 'StrongPass123!',
      keyboardSequence: 'testseq',
      lockTimeout: 15
    };

    const validResult = result.current.validateConfig(validConfig);
    expect(validResult.isValid).toBe(true);

    const invalidConfig: Partial<VaultConfig> = {
      name: '', // Invalid
      password: 'weak' // Invalid
    };

    const invalidResult = result.current.validateConfig(invalidConfig);
    expect(invalidResult.isValid).toBe(false);
    expect(invalidResult.errors.length).toBeGreaterThan(0);
  });

  test('should provide field error helpers', () => {
    const { result } = renderHook(() => useVaultValidation());

    // Start with invalid config to set validation state
    act(() => {
      const config: Partial<VaultConfig> = { name: '' };
      result.current.startValidation(config);
    });

    // Fast-forward timers to complete debounced validation
    act(() => {
      vi.advanceTimersByTime(300);
    });

    expect(result.current.hasFieldError('name')).toBe(true);
    expect(result.current.getFieldError('name')).toBeTruthy();
    expect(result.current.hasFieldError('nonexistent')).toBe(false);
    expect(result.current.getFieldError('nonexistent')).toBeUndefined();
  });

  test('should validate password confirmation', () => {
    const { result } = renderHook(() => useVaultValidation());

    const matchingResult = result.current.validatePasswordMatch('password123', 'password123');
    expect(matchingResult.isValid).toBe(true);

    const nonMatchingResult = result.current.validatePasswordMatch('password123', 'different');
    expect(nonMatchingResult.isValid).toBe(false);
  });

  test('should handle real-time validation with debouncing', () => {
    const { result } = renderHook(() => useVaultValidation({
      validateOnChange: true,
      debounceMs: 200
    }));

    const config: Partial<VaultConfig> = { name: '' };

    act(() => {
      result.current.startValidation(config);
    });

    // Should be validating immediately
    expect(result.current.isValidating).toBe(true);

    // Fast-forward past debounce time
    act(() => {
      vi.advanceTimersByTime(200);
    });

    // Should complete validation
    expect(result.current.isValidating).toBe(false);
    expect(result.current.validation.isValid).toBe(false);
  });

  test('should clear validation state', () => {
    const { result } = renderHook(() => useVaultValidation());

    // Set invalid state first
    act(() => {
      const config: Partial<VaultConfig> = { name: '' };
      result.current.startValidation(config);
    });

    act(() => {
      vi.advanceTimersByTime(300);
    });

    expect(result.current.validation.isValid).toBe(false);

    // Clear validation
    act(() => {
      result.current.clearValidation();
    });

    expect(result.current.validation.isValid).toBe(true);
    expect(result.current.validation.errors).toHaveLength(0);
    expect(result.current.isValidating).toBe(false);
  });

  test('should handle disabled validation on change', () => {
    const { result } = renderHook(() => useVaultValidation({
      validateOnChange: false
    }));

    const config: Partial<VaultConfig> = { name: '' };

    act(() => {
      result.current.startValidation(config);
    });

    // Should not start validating when disabled
    expect(result.current.isValidating).toBe(false);
  });

  test('should consider existing vaults for uniqueness validation', () => {
    const { result } = renderHook(() => useVaultValidation({
      existingVaults: mockVaults
    }));

    // Test duplicate name
    const duplicateResult = result.current.validateField('name', 'Personal Documents');
    expect(duplicateResult.isValid).toBe(false);

    // Test unique name
    const uniqueResult = result.current.validateField('name', 'Unique Vault Name');
    expect(uniqueResult.isValid).toBe(true);
  });

  test('should allow editing current vault without uniqueness conflict', () => {
    const { result } = renderHook(() => useVaultValidation({
      existingVaults: mockVaults,
      currentVaultId: 'vault-1'
    }));

    // Should allow keeping the same name when editing
    const result1 = result.current.validateField('name', 'Personal Documents');
    expect(result1.isValid).toBe(true);
  });
});

// ==================== useFieldValidation TESTS ====================

describe('useFieldValidation', () => {
  test('should validate single field', () => {
    const { result } = renderHook(() => 
      useFieldValidation('name', 'Valid Name', { existingVaults: mockVaults })
    );

    expect(result.current.isValid).toBe(true);
    expect(result.current.hasError).toBe(false);
    expect(result.current.error).toBeUndefined();
  });

  test('should detect field errors', () => {
    const { result } = renderHook(() => 
      useFieldValidation('name', '', { existingVaults: mockVaults })
    );

    expect(result.current.isValid).toBe(false);
    expect(result.current.hasError).toBe(true);
    expect(result.current.error).toBeTruthy();
  });

  test('should update validation when value changes', () => {
    const { result, rerender } = renderHook(
      ({ value }) => useFieldValidation('name', value, { existingVaults: mockVaults }),
      { initialProps: { value: '' } }
    );

    // Initially invalid
    expect(result.current.isValid).toBe(false);

    // Update to valid value
    rerender({ value: 'Valid Name' });
    expect(result.current.isValid).toBe(true);
  });
});

// ==================== usePasswordValidation TESTS ====================

describe('usePasswordValidation', () => {
  test('should calculate password strength', () => {
    const { result } = renderHook(() => 
      usePasswordValidation('StrongPass123!')
    );

    expect(result.current.strength).toBeTruthy();
    expect(result.current.strength!.score).toBeGreaterThan(0);
    expect(result.current.isPasswordValid).toBe(true);
  });

  test('should validate weak passwords', () => {
    const { result } = renderHook(() => 
      usePasswordValidation('weak')
    );

    expect(result.current.strength).toBeTruthy();
    expect(result.current.strength!.score).toBeLessThan(3);
    expect(result.current.isPasswordValid).toBe(false);
    expect(result.current.passwordError).toBeTruthy();
  });

  test('should validate password confirmation', () => {
    const { result } = renderHook(() => 
      usePasswordValidation('password123', 'password123')
    );

    expect(result.current.isConfirmValid).toBe(true);
    expect(result.current.confirmError).toBeUndefined();
  });

  test('should detect password confirmation mismatch', () => {
    const { result } = renderHook(() => 
      usePasswordValidation('password123', 'different')
    );

    expect(result.current.isConfirmValid).toBe(false);
    expect(result.current.confirmError).toBeTruthy();
  });

  test('should handle empty password', () => {
    const { result } = renderHook(() => 
      usePasswordValidation('')
    );

    expect(result.current.strength).toBeNull();
    expect(result.current.isPasswordValid).toBe(false);
  });

  test('should update when password changes', () => {
    const { result, rerender } = renderHook(
      ({ password }) => usePasswordValidation(password),
      { initialProps: { password: 'weak' } }
    );

    // Initially weak
    expect(result.current.isPasswordValid).toBe(false);

    // Update to strong password
    rerender({ password: 'StrongPass123!' });
    expect(result.current.isPasswordValid).toBe(true);
  });

  test('should provide detailed strength analysis', () => {
    const { result } = renderHook(() => 
      usePasswordValidation('TestPass123!')
    );

    const strength = result.current.strength!;
    expect(strength.requirements.minLength).toBe(true);
    expect(strength.requirements.hasUppercase).toBe(true);
    expect(strength.requirements.hasLowercase).toBe(true);
    expect(strength.requirements.hasNumbers).toBe(true);
    expect(strength.requirements.hasSpecialChars).toBe(true);
    expect(strength.feedback).toHaveLength(0); // No feedback for strong password
  });

  test('should provide helpful feedback for weak passwords', () => {
    const { result } = renderHook(() => 
      usePasswordValidation('abc')
    );

    const strength = result.current.strength!;
    expect(strength.feedback.length).toBeGreaterThan(0);
    expect(strength.feedback).toContain('Use at least 8 characters');
  });
});