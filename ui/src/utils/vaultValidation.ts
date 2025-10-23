/**
 * Vault Validation Utilities
 * 
 * Comprehensive validation functions for vault configuration and security
 */

import { VaultInfo, VaultConfig, ValidationResult, ValidationError } from '../types';

// ==================== CONSTANTS ====================

const VAULT_NAME_MIN_LENGTH = 3;
const VAULT_NAME_MAX_LENGTH = 50;
const PASSWORD_MIN_LENGTH = 8;
const SEQUENCE_MIN_LENGTH = 2;
const SEQUENCE_MAX_LENGTH = 20;
const TIMEOUT_MIN_VALUE = 1;
const TIMEOUT_MAX_VALUE = 1440; // 24 hours in minutes

const FORBIDDEN_NAMES = [
  'system', 'root', 'admin', 'administrator', 'temp', 'tmp', 'cache',
  'config', 'settings', 'phantom', 'vault', 'default'
];

const SYSTEM_PATHS = [
  '/system', '/root', '/boot', '/dev', '/proc', '/sys', '/tmp',
  '/var/log', '/var/cache', '/usr/bin', '/usr/sbin', '/bin', '/sbin'
];

const COMMON_PASSWORDS = [
  'password', '123456', 'password123', 'admin', 'qwerty', 'letmein',
  'welcome', 'monkey', '1234567890', 'password1'
];

const COMMON_SEQUENCES = [
  'qwerty', 'asdf', '1234', 'abcd', '123456', 'qwertyuiop'
];

// ==================== VALIDATION FUNCTIONS ====================

/**
 * Validate vault name
 */
export function validateVaultName(
  name: string,
  existingVaults: VaultInfo[] = [],
  currentVaultId?: string
): ValidationResult {
  const errors: ValidationError[] = [];

  // Required check
  if (!name || name.trim().length === 0) {
    errors.push({
      field: 'name',
      message: 'Vault name is required',
      code: 'REQUIRED'
    });
    return { isValid: false, errors };
  }

  const trimmedName = name.trim();

  // Length checks
  if (trimmedName.length < VAULT_NAME_MIN_LENGTH) {
    errors.push({
      field: 'name',
      message: `Vault name must be at least ${VAULT_NAME_MIN_LENGTH} characters`,
      code: 'MIN_LENGTH'
    });
  }

  if (trimmedName.length > VAULT_NAME_MAX_LENGTH) {
    errors.push({
      field: 'name',
      message: `Vault name must be no more than ${VAULT_NAME_MAX_LENGTH} characters`,
      code: 'MAX_LENGTH'
    });
  }

  // Pattern check
  if (!/^[a-zA-Z0-9\s\-_\.]+$/.test(trimmedName)) {
    errors.push({
      field: 'name',
      message: 'Vault name contains invalid characters',
      code: 'INVALID_PATTERN'
    });
  }

  // Forbidden names check
  if (FORBIDDEN_NAMES.includes(trimmedName.toLowerCase())) {
    errors.push({
      field: 'name',
      message: 'This vault name is not allowed',
      code: 'FORBIDDEN_NAME'
    });
  }

  // Duplicate check
  const isDuplicate = existingVaults.some(vault => 
    vault.name.toLowerCase() === trimmedName.toLowerCase() && 
    vault.id !== currentVaultId
  );

  if (isDuplicate) {
    errors.push({
      field: 'name',
      message: 'A vault with this name already exists',
      code: 'DUPLICATE_NAME'
    });
  }

  return { isValid: errors.length === 0, errors };
}

/**
 * Validate vault password
 */
export function validateVaultPassword(password: string): ValidationResult {
  const errors: ValidationError[] = [];

  // Required check
  if (!password) {
    errors.push({
      field: 'password',
      message: 'Password is required',
      code: 'REQUIRED'
    });
    return { isValid: false, errors };
  }

  // Length check
  if (password.length < PASSWORD_MIN_LENGTH) {
    errors.push({
      field: 'password',
      message: `Password must be at least ${PASSWORD_MIN_LENGTH} characters`,
      code: 'MIN_LENGTH'
    });
  }

  // Character requirements
  if (!/[A-Z]/.test(password)) {
    errors.push({
      field: 'password',
      message: 'Password must contain at least one uppercase letter',
      code: 'MISSING_UPPERCASE'
    });
  }

  if (!/[a-z]/.test(password)) {
    errors.push({
      field: 'password',
      message: 'Password must contain at least one lowercase letter',
      code: 'MISSING_LOWERCASE'
    });
  }

  if (!/\d/.test(password)) {
    errors.push({
      field: 'password',
      message: 'Password must contain at least one number',
      code: 'MISSING_NUMBER'
    });
  }

  // Common password check
  if (COMMON_PASSWORDS.includes(password.toLowerCase())) {
    errors.push({
      field: 'password',
      message: 'This password is too common',
      code: 'WEAK_PASSWORD'
    });
  }

  // Sequential characters check
  if (hasSequentialChars(password)) {
    errors.push({
      field: 'password',
      message: 'Password contains sequential characters',
      code: 'SEQUENTIAL_CHARS'
    });
  }

  // Repeated characters check
  if (hasTooManyRepeatedChars(password)) {
    errors.push({
      field: 'password',
      message: 'Password has too many repeated characters',
      code: 'REPEATED_CHARS'
    });
  }

  return { isValid: errors.length === 0, errors };
}

/**
 * Calculate password strength
 */
export function calculatePasswordStrength(password: string) {
  let score = 0;
  const feedback: string[] = [];
  
  const requirements = {
    minLength: password.length >= 8,
    hasUppercase: /[A-Z]/.test(password),
    hasLowercase: /[a-z]/.test(password),
    hasNumbers: /\d/.test(password),
    hasSpecialChars: /[^a-zA-Z0-9]/.test(password)
  };

  // Length scoring
  if (password.length >= 8) score += 1;
  else feedback.push('Use at least 8 characters');

  if (password.length >= 12) score += 1;

  // Character variety scoring
  if (requirements.hasUppercase) score += 1;
  else feedback.push('Add uppercase letters (A-Z)');

  if (requirements.hasLowercase) score += 1;
  else feedback.push('Add lowercase letters (a-z)');

  if (requirements.hasNumbers) score += 1;
  else feedback.push('Add numbers (0-9)');

  if (requirements.hasSpecialChars) score += 1;
  else feedback.push('Add special characters (!@#$%^&*)');

  // Bonus points for complexity
  if (password.length >= 16) score += 1;
  if (!hasSequentialChars(password)) score += 1;
  if (!hasTooManyRepeatedChars(password)) score += 1;

  return {
    score: Math.min(score, 5),
    requirements,
    feedback
  };
}

/**
 * Validate folder path
 */
export function validateFolderPath(path: string): ValidationResult {
  const errors: ValidationError[] = [];

  // Required check
  if (!path || path.trim().length === 0) {
    errors.push({
      field: 'path',
      message: 'Folder path is required',
      code: 'REQUIRED'
    });
    return { isValid: false, errors };
  }

  const trimmedPath = path.trim();

  // System path check
  const isSystemPath = SYSTEM_PATHS.some(sysPath => 
    trimmedPath.startsWith(sysPath)
  );

  if (isSystemPath) {
    errors.push({
      field: 'path',
      message: 'Cannot create vault in system directory',
      code: 'SYSTEM_PATH'
    });
  }

  // Format check
  if (!/^[a-zA-Z0-9\/\-_\.\s]+$/.test(trimmedPath)) {
    errors.push({
      field: 'path',
      message: 'Path contains invalid characters',
      code: 'INVALID_FORMAT'
    });
  }

  return { isValid: errors.length === 0, errors };
}

/**
 * Validate keyboard sequence
 */
export function validateKeyboardSequence(
  sequence: string,
  existingSequences: string[] = []
): ValidationResult {
  const errors: ValidationError[] = [];

  // Empty is allowed (optional field)
  if (!sequence || sequence.trim().length === 0) {
    return { isValid: true, errors: [] };
  }

  const trimmedSequence = sequence.trim();

  // Length checks
  if (trimmedSequence.length < SEQUENCE_MIN_LENGTH) {
    errors.push({
      field: 'keyboardSequence',
      message: `Sequence must be at least ${SEQUENCE_MIN_LENGTH} characters`,
      code: 'MIN_LENGTH'
    });
  }

  if (trimmedSequence.length > SEQUENCE_MAX_LENGTH) {
    errors.push({
      field: 'keyboardSequence',
      message: `Sequence must be no more than ${SEQUENCE_MAX_LENGTH} characters`,
      code: 'MAX_LENGTH'
    });
  }

  // Duplicate check
  if (existingSequences.includes(trimmedSequence)) {
    errors.push({
      field: 'keyboardSequence',
      message: 'This sequence is already in use',
      code: 'DUPLICATE_SEQUENCE'
    });
  }

  // Common pattern check
  if (COMMON_SEQUENCES.some(common => 
    trimmedSequence.toLowerCase().includes(common)
  )) {
    errors.push({
      field: 'keyboardSequence',
      message: 'Sequence contains common patterns',
      code: 'COMMON_PATTERN'
    });
  }

  return { isValid: errors.length === 0, errors };
}

/**
 * Validate lock timeout
 */
export function validateLockTimeout(timeout: number): ValidationResult {
  const errors: ValidationError[] = [];

  if (timeout < TIMEOUT_MIN_VALUE) {
    errors.push({
      field: 'lockTimeout',
      message: `Timeout must be at least ${TIMEOUT_MIN_VALUE} minute`,
      code: 'MIN_VALUE'
    });
  }

  if (timeout > TIMEOUT_MAX_VALUE) {
    errors.push({
      field: 'lockTimeout',
      message: `Timeout must be no more than ${TIMEOUT_MAX_VALUE} minutes`,
      code: 'MAX_VALUE'
    });
  }

  return { isValid: errors.length === 0, errors };
}

/**
 * Validate password confirmation
 */
export function validatePasswordConfirmation(
  password: string,
  confirmation: string
): ValidationResult {
  const errors: ValidationError[] = [];

  if (password !== confirmation) {
    errors.push({
      field: 'passwordConfirmation',
      message: 'Passwords do not match',
      code: 'PASSWORD_MISMATCH'
    });
  }

  return { isValid: errors.length === 0, errors };
}

/**
 * Validate complete vault configuration
 */
export function validateVaultConfig(
  config: Partial<VaultConfig>,
  existingVaults: VaultInfo[] = [],
  currentVaultId?: string
): ValidationResult {
  const allErrors: ValidationError[] = [];

  // Validate each field if provided
  if (config.name !== undefined) {
    const nameResult = validateVaultName(config.name, existingVaults, currentVaultId);
    allErrors.push(...nameResult.errors);
  }

  if (config.path !== undefined) {
    const pathResult = validateFolderPath(config.path);
    allErrors.push(...pathResult.errors);
  }

  if (config.password !== undefined) {
    const passwordResult = validateVaultPassword(config.password);
    allErrors.push(...passwordResult.errors);
  }

  if (config.keyboardSequence !== undefined) {
    const existingSequences = existingVaults
      .filter(v => v.id !== currentVaultId)
      .map(v => v.name); // Using name as sequence for demo
    const sequenceResult = validateKeyboardSequence(config.keyboardSequence, existingSequences);
    allErrors.push(...sequenceResult.errors);
  }

  if (config.lockTimeout !== undefined) {
    const timeoutResult = validateLockTimeout(config.lockTimeout);
    allErrors.push(...timeoutResult.errors);
  }

  return { isValid: allErrors.length === 0, errors: allErrors };
}

// ==================== HELPER FUNCTIONS ====================

/**
 * Check for sequential characters
 */
function hasSequentialChars(str: string): boolean {
  for (let i = 0; i < str.length - 2; i++) {
    const char1 = str.charCodeAt(i);
    const char2 = str.charCodeAt(i + 1);
    const char3 = str.charCodeAt(i + 2);
    
    if (char2 === char1 + 1 && char3 === char2 + 1) {
      return true;
    }
  }
  return false;
}

/**
 * Check for too many repeated characters
 */
function hasTooManyRepeatedChars(str: string): boolean {
  const charCount: { [key: string]: number } = {};
  
  for (const char of str) {
    charCount[char] = (charCount[char] || 0) + 1;
    if (charCount[char] > str.length / 2) {
      return true;
    }
  }
  
  return false;
}