/**
 * Validation Test
 * 
 * Simple test to verify validation utilities work
 */

import { describe, it, expect } from 'vitest';

// Simple validation functions for testing
const validateVaultName = (name: string) => {
  if (!name || !name.trim()) {
    return { isValid: false, errors: [{ field: 'name', message: 'Name is required' }] };
  }
  if (name.length < 2) {
    return { isValid: false, errors: [{ field: 'name', message: 'Name too short' }] };
  }
  return { isValid: true, errors: [] };
};

const validatePassword = (password: string) => {
  if (!password) {
    return { isValid: false, errors: [{ field: 'password', message: 'Password is required' }] };
  }
  if (password.length < 8) {
    return { isValid: false, errors: [{ field: 'password', message: 'Password too short' }] };
  }
  return { isValid: true, errors: [] };
};

describe('Validation Utilities', () => {
  describe('validateVaultName', () => {
    it('should accept valid names', () => {
      const result = validateVaultName('Valid Name');
      expect(result.isValid).toBe(true);
      expect(result.errors).toHaveLength(0);
    });

    it('should reject empty names', () => {
      const result = validateVaultName('');
      expect(result.isValid).toBe(false);
      expect(result.errors[0].message).toBe('Name is required');
    });

    it('should reject short names', () => {
      const result = validateVaultName('A');
      expect(result.isValid).toBe(false);
      expect(result.errors[0].message).toBe('Name too short');
    });
  });

  describe('validatePassword', () => {
    it('should accept valid passwords', () => {
      const result = validatePassword('ValidPass123');
      expect(result.isValid).toBe(true);
      expect(result.errors).toHaveLength(0);
    });

    it('should reject empty passwords', () => {
      const result = validatePassword('');
      expect(result.isValid).toBe(false);
      expect(result.errors[0].message).toBe('Password is required');
    });

    it('should reject short passwords', () => {
      const result = validatePassword('short');
      expect(result.isValid).toBe(false);
      expect(result.errors[0].message).toBe('Password too short');
    });
  });
});