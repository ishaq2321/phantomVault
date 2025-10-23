/**
 * Vault Validation Tests
 * 
 * Unit tests for vault configuration validation utilities
 */

import {
  validateVaultName,
  validateVaultPassword,
  validateFolderPath,
  validateKeyboardSequence,
  validateLockTimeout,
  validateVaultConfig,
  calculatePasswordStrength,
  validatePasswordConfirmation
} from '../vaultValidation';
import { VaultInfo, VaultConfig } from '../../types';

// ==================== TEST DATA ====================

const mockVaults: VaultInfo[] = [
  {
    id: 'vault-1',
    name: 'Personal Documents',
    path: '/home/user/personal',
    status: 'unmounted',
    lastAccess: new Date('2024-01-01'),
    size: 1024 * 1024 * 100, // 100MB
    folderCount: 25
  },
  {
    id: 'vault-2',
    name: 'Work Files',
    path: '/home/user/work',
    status: 'mounted',
    lastAccess: new Date('2024-01-02'),
    size: 1024 * 1024 * 500, // 500MB
    folderCount: 50
  }
];

// ==================== VAULT NAME VALIDATION TESTS ====================

describe('validateVaultName', () => {
  test('should accept valid vault names', () => {
    const validNames = [
      'My Vault',
      'Personal-Documents',
      'Work_Files_2024',
      'Vault.1',
      'Test123'
    ];

    validNames.forEach(name => {
      const result = validateVaultName(name, mockVaults);
      expect(result.isValid).toBe(true);
      expect(result.errors).toHaveLength(0);
    });
  });

  test('should reject empty or whitespace-only names', () => {
    const invalidNames = ['', '   ', '\\t\\n'];

    invalidNames.forEach(name => {
      const result = validateVaultName(name, mockVaults);
      expect(result.isValid).toBe(false);
      expect(result.errors[0].code).toBe('REQUIRED');
    });
  });

  test('should reject names that are too short', () => {
    const result = validateVaultName('A', mockVaults);
    expect(result.isValid).toBe(false);
    expect(result.errors[0].code).toBe('MIN_LENGTH');
  });

  test('should reject names that are too long', () => {
    const longName = 'A'.repeat(101);
    const result = validateVaultName(longName, mockVaults);
    expect(result.isValid).toBe(false);
    expect(result.errors[0].code).toBe('MAX_LENGTH');
  });

  test('should reject names with invalid characters', () => {
    const invalidNames = [
      'Vault<>Name',
      'Vault|Name',
      'Vault\"Name',
      'Vault*Name'
    ];

    invalidNames.forEach(name => {
      const result = validateVaultName(name, mockVaults);
      expect(result.isValid).toBe(false);
      expect(result.errors[0].code).toBe('INVALID_PATTERN');
    });
  });

  test('should reject forbidden names', () => {
    const forbiddenNames = ['con', 'prn', 'aux', 'nul', 'system', 'root'];

    forbiddenNames.forEach(name => {
      const result = validateVaultName(name, mockVaults);
      expect(result.isValid).toBe(false);
      expect(result.errors[0].code).toBe('FORBIDDEN_NAME');
    });
  });

  test('should reject duplicate names', () => {
    const result = validateVaultName('Personal Documents', mockVaults);
    expect(result.isValid).toBe(false);
    expect(result.errors[0].code).toBe('DUPLICATE_NAME');
  });

  test('should allow duplicate names when editing the same vault', () => {
    const result = validateVaultName('Personal Documents', mockVaults, 'vault-1');
    expect(result.isValid).toBe(true);
  });
});

// ==================== PASSWORD VALIDATION TESTS ====================

describe('validateVaultPassword', () => {
  test('should accept strong passwords', () => {
    const strongPasswords = [
      'MyStr0ng!Password',
      'C0mplex#Pass123',
      'Secure$Vault2024'
    ];

    strongPasswords.forEach(password => {
      const result = validateVaultPassword(password);
      expect(result.isValid).toBe(true);
    });
  });

  test('should reject empty passwords', () => {
    const result = validateVaultPassword('');
    expect(result.isValid).toBe(false);
    expect(result.errors[0].code).toBe('REQUIRED');
  });

  test('should reject passwords that are too short', () => {
    const result = validateVaultPassword('short');
    expect(result.isValid).toBe(false);
    expect(result.errors[0].code).toBe('MIN_LENGTH');
  });

  test('should reject passwords without uppercase letters', () => {
    const result = validateVaultPassword('lowercase123');
    expect(result.isValid).toBe(false);
    expect(result.errors.some(e => e.code === 'MISSING_UPPERCASE')).toBe(true);
  });

  test('should reject passwords without lowercase letters', () => {
    const result = validateVaultPassword('UPPERCASE123');
    expect(result.isValid).toBe(false);
    expect(result.errors.some(e => e.code === 'MISSING_LOWERCASE')).toBe(true);
  });

  test('should reject passwords without numbers', () => {
    const result = validateVaultPassword('NoNumbers');
    expect(result.isValid).toBe(false);
    expect(result.errors.some(e => e.code === 'MISSING_NUMBER')).toBe(true);
  });

  test('should reject weak/common passwords', () => {
    const weakPasswords = ['password', '123456', 'qwerty', 'Password123'];

    weakPasswords.forEach(password => {
      const result = validateVaultPassword(password);
      expect(result.isValid).toBe(false);
      expect(result.errors.some(e => e.code === 'WEAK_PASSWORD')).toBe(true);
    });
  });

  test('should reject passwords with sequential characters', () => {
    const sequentialPasswords = ['Abc123def', 'Password1234'];

    sequentialPasswords.forEach(password => {
      const result = validateVaultPassword(password);
      expect(result.isValid).toBe(false);
      expect(result.errors.some(e => e.code === 'SEQUENTIAL_CHARS')).toBe(true);
    });
  });

  test('should reject passwords with too many repeated characters', () => {
    const result = validateVaultPassword('Passworddddd1');
    expect(result.isValid).toBe(false);
    expect(result.errors.some(e => e.code === 'REPEATED_CHARS')).toBe(true);
  });
});

// ==================== PASSWORD STRENGTH TESTS ====================

describe('calculatePasswordStrength', () => {
  test('should calculate weak password strength', () => {
    const strength = calculatePasswordStrength('weak');
    expect(strength.score).toBeLessThan(2);
    expect(strength.feedback.length).toBeGreaterThan(0);
  });

  test('should calculate medium password strength', () => {
    const strength = calculatePasswordStrength('Medium123');
    expect(strength.score).toBeGreaterThanOrEqual(2);
    expect(strength.score).toBeLessThan(4);
  });

  test('should calculate strong password strength', () => {
    const strength = calculatePasswordStrength('VeryStr0ng!Password');
    expect(strength.score).toBeGreaterThanOrEqual(4);
    expect(strength.requirements.minLength).toBe(true);
    expect(strength.requirements.hasUppercase).toBe(true);
    expect(strength.requirements.hasLowercase).toBe(true);
    expect(strength.requirements.hasNumbers).toBe(true);
    expect(strength.requirements.hasSpecialChars).toBe(true);
  });

  test('should provide helpful feedback for weak passwords', () => {
    const strength = calculatePasswordStrength('weak');
    expect(strength.feedback).toContain('Use at least 8 characters');
    expect(strength.feedback).toContain('Add uppercase letters (A-Z)');
    expect(strength.feedback).toContain('Add numbers (0-9)');
  });
});

// ==================== FOLDER PATH VALIDATION TESTS ====================

describe('validateFolderPath', () => {
  test('should accept valid folder paths', () => {
    const validPaths = [
      '/home/user/documents',
      '/Users/john/Desktop/vault',
      'C:\\\\Users\\\\John\\\\Documents',
      '/opt/vaults/personal'
    ];

    validPaths.forEach(path => {
      const result = validateFolderPath(path);
      expect(result.isValid).toBe(true);
    });
  });

  test('should reject empty paths', () => {
    const result = validateFolderPath('');
    expect(result.isValid).toBe(false);
    expect(result.errors[0].code).toBe('REQUIRED');
  });

  test('should reject system paths', () => {
    const systemPaths = [
      '/bin',
      '/etc',
      'C:\\\\Windows',
      'C:\\\\System32'
    ];

    systemPaths.forEach(path => {
      const result = validateFolderPath(path);
      expect(result.isValid).toBe(false);
      expect(result.errors[0].code).toBe('SYSTEM_PATH');
    });
  });

  test('should reject paths with invalid format', () => {
    const invalidPaths = [
      'path\\0with\\0null',
      'path<>with|invalid*chars',
      '../../../etc/passwd'
    ];

    invalidPaths.forEach(path => {
      const result = validateFolderPath(path);
      expect(result.isValid).toBe(false);
      expect(result.errors[0].code).toBe('INVALID_FORMAT');
    });
  });
});

// ==================== KEYBOARD SEQUENCE VALIDATION TESTS ====================

describe('validateKeyboardSequence', () => {
  const existingSequences = ['vault1', 'personal', 'work123'];

  test('should accept valid keyboard sequences', () => {
    const validSequences = [
      'myseq123',
      'Vault!2024',
      'quick_access'
    ];

    validSequences.forEach(sequence => {
      const result = validateKeyboardSequence(sequence, existingSequences);
      expect(result.isValid).toBe(true);
    });
  });

  test('should accept empty sequences (optional field)', () => {
    const result = validateKeyboardSequence('', existingSequences);
    expect(result.isValid).toBe(true);
  });

  test('should reject sequences that are too short', () => {
    const result = validateKeyboardSequence('a', existingSequences);
    expect(result.isValid).toBe(false);
    expect(result.errors[0].code).toBe('MIN_LENGTH');
  });

  test('should reject sequences that are too long', () => {
    const longSequence = 'a'.repeat(21);
    const result = validateKeyboardSequence(longSequence, existingSequences);
    expect(result.isValid).toBe(false);
    expect(result.errors[0].code).toBe('MAX_LENGTH');
  });

  test('should reject duplicate sequences', () => {
    const result = validateKeyboardSequence('vault1', existingSequences);
    expect(result.isValid).toBe(false);
    expect(result.errors[0].code).toBe('DUPLICATE_SEQUENCE');
  });

  test('should reject sequences with common patterns', () => {
    const commonPatterns = ['1234test', 'qwertyabc', 'asdfgh'];

    commonPatterns.forEach(sequence => {
      const result = validateKeyboardSequence(sequence, existingSequences);
      expect(result.isValid).toBe(false);
      expect(result.errors[0].code).toBe('COMMON_PATTERN');
    });
  });
});

// ==================== LOCK TIMEOUT VALIDATION TESTS ====================

describe('validateLockTimeout', () => {
  test('should accept valid timeout values', () => {
    const validTimeouts = [1, 5, 15, 30, 60, 120];

    validTimeouts.forEach(timeout => {
      const result = validateLockTimeout(timeout);
      expect(result.isValid).toBe(true);
    });
  });

  test('should reject timeout values that are too small', () => {
    const result = validateLockTimeout(0);
    expect(result.isValid).toBe(false);
    expect(result.errors[0].code).toBe('MIN_VALUE');
  });

  test('should reject timeout values that are too large', () => {
    const result = validateLockTimeout(1500);
    expect(result.isValid).toBe(false);
    expect(result.errors[0].code).toBe('MAX_VALUE');
  });
});

// ==================== PASSWORD CONFIRMATION TESTS ====================

describe('validatePasswordConfirmation', () => {
  test('should accept matching passwords', () => {
    const result = validatePasswordConfirmation('password123', 'password123');
    expect(result.isValid).toBe(true);
  });

  test('should reject non-matching passwords', () => {
    const result = validatePasswordConfirmation('password123', 'different123');
    expect(result.isValid).toBe(false);
    expect(result.errors[0].code).toBe('PASSWORD_MISMATCH');
  });
});

// ==================== COMPLETE CONFIG VALIDATION TESTS ====================

describe('validateVaultConfig', () => {
  test('should validate complete valid configuration', () => {
    const config: Partial<VaultConfig> = {
      name: 'Test Vault',
      path: '/home/user/test-vault',
      password: 'StrongPass123!',
      keyboardSequence: 'testvault',
      lockTimeout: 15
    };

    const result = validateVaultConfig(config, mockVaults);
    expect(result.isValid).toBe(true);
  });

  test('should collect all validation errors', () => {
    const config: Partial<VaultConfig> = {
      name: '', // Invalid: empty
      path: '/bin', // Invalid: system path
      password: 'weak', // Invalid: too short, no uppercase, no numbers
      keyboardSequence: 'a', // Invalid: too short
      lockTimeout: 0 // Invalid: too small
    };

    const result = validateVaultConfig(config, mockVaults);
    expect(result.isValid).toBe(false);
    expect(result.errors.length).toBeGreaterThan(4);
    
    // Check that errors from different fields are included
    const errorFields = result.errors.map(e => e.field);
    expect(errorFields).toContain('name');
    expect(errorFields).toContain('path');
    expect(errorFields).toContain('password');
    expect(errorFields).toContain('keyboardSequence');
    expect(errorFields).toContain('lockTimeout');
  });

  test('should handle partial configurations', () => {
    const config: Partial<VaultConfig> = {
      name: 'Partial Vault'
      // Other fields undefined
    };

    const result = validateVaultConfig(config, mockVaults);
    expect(result.isValid).toBe(true); // Only validates provided fields
  });
});