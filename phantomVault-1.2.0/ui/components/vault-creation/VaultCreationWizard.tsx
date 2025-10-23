/**
 * Vault Creation Wizard Component
 * 
 * Multi-step wizard for creating new vaults with validation and progress tracking
 */

import React, { useState, useCallback, useEffect } from 'react';
import {
  VaultConfig,
  VaultCreationWizardProps,
  ValidationResult,
  ValidationError,
  PasswordStrength,
  EncryptionLevel,
  UnlockMode
} from '../../types';
import { useVault, useApp } from '../../contexts';
import { useVaultOperations } from '../../hooks';
import { PasswordStrengthIndicator } from '../common/PasswordStrengthIndicator';
import { FolderSelector } from '../common/FolderSelector';
import { KeyboardSequenceInput } from '../common/KeyboardSequenceInput';
import { KeyboardSequenceConfig } from './KeyboardSequenceConfig';

interface WizardStep {
  id: string;
  title: string;
  description: string;
  component: React.ComponentType<any>;
  isValid: boolean;
  isOptional?: boolean;
}

/**
 * Vault creation wizard component
 */
export const VaultCreationWizard: React.FC<VaultCreationWizardProps> = ({
  onComplete,
  onCancel,
  initialData = {},
}) => {
  const { state: vaultState } = useVault();
  const { actions: appActions } = useApp();
  const vaultOps = useVaultOperations();

  // ==================== STATE MANAGEMENT ====================

  const [currentStep, setCurrentStep] = useState(0);
  const [config, setConfig] = useState<Partial<VaultConfig>>({
    name: '',
    path: '',
    password: '',
    keyboardSequence: '',
    autoMount: false,
    encryptionLevel: 'standard',
    autoLock: true,
    lockTimeout: 15,
    ...initialData,
  });
  const [validation, setValidation] = useState<Record<string, ValidationResult>>({});
  const [isCreating, setIsCreating] = useState(false);

  // ==================== WIZARD STEPS CONFIGURATION ====================

  const steps: WizardStep[] = [
    {
      id: 'basic-info',
      title: 'Basic Information',
      description: 'Enter the vault name and select the folder to protect',
      component: BasicInfoStep,
      isValid: validateBasicInfo(),
    },
    {
      id: 'security',
      title: 'Security Settings',
      description: 'Set up password and encryption options',
      component: SecurityStep,
      isValid: validateSecurity(),
    },
    {
      id: 'keyboard-shortcut',
      title: 'Keyboard Shortcut',
      description: 'Configure keyboard sequence for quick access',
      component: KeyboardShortcutStep,
      isValid: validateKeyboardShortcut(),
      isOptional: true,
    },
    {
      id: 'advanced',
      title: 'Advanced Options',
      description: 'Configure auto-mount and locking behavior',
      component: AdvancedStep,
      isValid: true, // Advanced options are always valid
      isOptional: true,
    },
    {
      id: 'review',
      title: 'Review & Create',
      description: 'Review your settings and create the vault',
      component: ReviewStep,
      isValid: true,
    },
  ];

  // ==================== VALIDATION FUNCTIONS ====================

  function validateBasicInfo(): boolean {
    return !!(config.name?.trim() && config.path?.trim());
  }

  function validateSecurity(): boolean {
    return !!(config.password && config.password.length >= 8);
  }

  function validateKeyboardShortcut(): boolean {
    // Optional step - always valid
    return true;
  }

  const validateCurrentStep = useCallback((): ValidationResult => {
    const step = steps[currentStep];
    const errors: ValidationError[] = [];

    switch (step.id) {
      case 'basic-info':
        if (!config.name?.trim()) {
          errors.push({ field: 'name', message: 'Vault name is required' });
        }
        if (!config.path?.trim()) {
          errors.push({ field: 'path', message: 'Folder path is required' });
        }
        break;

      case 'security':
        if (!config.password) {
          errors.push({ field: 'password', message: 'Password is required' });
        } else if (config.password.length < 8) {
          errors.push({ field: 'password', message: 'Password must be at least 8 characters' });
        }
        break;

      case 'keyboard-shortcut':
        if (config.keyboardSequence && !isValidKeyboardSequence(config.keyboardSequence)) {
          errors.push({ field: 'keyboardSequence', message: 'Invalid keyboard sequence' });
        }
        break;
    }

    return {
      isValid: errors.length === 0,
      errors,
    };
  }, [currentStep, config, steps]);

  const isValidKeyboardSequence = (sequence: string): boolean => {
    // Basic validation - should contain at least one letter/number
    return /[a-zA-Z0-9]/.test(sequence) && sequence.length >= 2;
  };

  // ==================== NAVIGATION FUNCTIONS ====================

  const canGoNext = useCallback((): boolean => {
    const step = steps[currentStep];
    return step.isValid || step.isOptional || false;
  }, [currentStep, steps]);

  const canGoPrevious = useCallback((): boolean => {
    return currentStep > 0;
  }, [currentStep]);

  const handleNext = useCallback(() => {
    const validation = validateCurrentStep();
    setValidation(prev => ({ ...prev, [steps[currentStep].id]: validation }));

    if (validation.isValid || steps[currentStep].isOptional) {
      if (currentStep < steps.length - 1) {
        setCurrentStep(prev => prev + 1);
      } else {
        handleCreate();
      }
    }
  }, [currentStep, steps, validateCurrentStep]);

  const handlePrevious = useCallback(() => {
    if (canGoPrevious()) {
      setCurrentStep(prev => prev - 1);
    }
  }, [canGoPrevious]);

  const handleStepClick = useCallback((stepIndex: number) => {
    // Allow clicking on previous steps or current step
    if (stepIndex <= currentStep) {
      setCurrentStep(stepIndex);
    }
  }, [currentStep]);

  // ==================== VAULT CREATION ====================

  const handleCreate = useCallback(async () => {
    try {
      setIsCreating(true);

      // Final validation
      const finalValidation = validateCurrentStep();
      if (!finalValidation.isValid) {
        setValidation(prev => ({ ...prev, [steps[currentStep].id]: finalValidation }));
        return;
      }

      // Create the vault
      const vaultConfig: VaultConfig = {
        name: config.name!,
        path: config.path!,
        password: config.password!,
        keyboardSequence: config.keyboardSequence || '',
        autoMount: config.autoMount || false,
        encryptionLevel: config.encryptionLevel || 'standard',
        autoLock: config.autoLock !== false,
        lockTimeout: config.lockTimeout || 15,
      };

      const result = await vaultOps.createVault(vaultConfig);

      if (result.success) {
        appActions.addNotification({
          type: 'success',
          title: 'Vault Created',
          message: `"${vaultConfig.name}" has been created successfully.`,
          duration: 5000,
        });

        onComplete(vaultConfig);
      } else {
        throw new Error(result.error || 'Failed to create vault');
      }
    } catch (error) {
      console.error('Vault creation failed:', error);
      appActions.addNotification({
        type: 'error',
        title: 'Creation Failed',
        message: error instanceof Error ? error.message : 'Failed to create vault',
        duration: 5000,
      });
    } finally {
      setIsCreating(false);
    }
  }, [config, validateCurrentStep, steps, currentStep, vaultOps, appActions, onComplete]);

  // ==================== CONFIG UPDATES ====================

  const updateConfig = useCallback((updates: Partial<VaultConfig>) => {
    setConfig(prev => ({ ...prev, ...updates }));
  }, []);

  // ==================== STEP COMPONENTS ====================

  function BasicInfoStep({ config, updateConfig, validation }: any) {
    return (
      <div className="wizard-step basic-info-step">
        <div className="form-group">
          <label htmlFor="vault-name" className="form-label">
            Vault Name *
          </label>
          <input
            id="vault-name"
            type="text"
            value={config.name || ''}
            onChange={(e) => updateConfig({ name: e.target.value })}
            placeholder="Enter a name for your vault"
            className={`form-input ${validation?.errors?.some((e: ValidationError) => e.field === 'name') ? 'error' : ''}`}
            autoFocus
          />
          {validation?.errors?.find((e: ValidationError) => e.field === 'name') && (
            <span className="error-message">
              {validation.errors.find((e: ValidationError) => e.field === 'name')?.message}
            </span>
          )}
        </div>

        <div className="form-group">
          <label htmlFor="vault-path" className="form-label">
            Folder to Protect *
          </label>
          <FolderSelector
            value={config.path || ''}
            onChange={(path) => updateConfig({ path })}
            placeholder="Select folder to encrypt and protect"
            error={validation?.errors?.some((e: ValidationError) => e.field === 'path')}
          />
          {validation?.errors?.find((e: ValidationError) => e.field === 'path') && (
            <span className="error-message">
              {validation.errors.find((e: ValidationError) => e.field === 'path')?.message}
            </span>
          )}
        </div>

        <div className="info-box">
          <div className="info-icon">💡</div>
          <div className="info-content">
            <h4>What happens next?</h4>
            <p>
              The selected folder will be encrypted and hidden when the vault is locked. 
              You'll be able to access it by unlocking the vault with your password.
            </p>
          </div>
        </div>
      </div>
    );
  }

  function SecurityStep({ config, updateConfig, validation }: any) {
    const [passwordStrength, setPasswordStrength] = useState<PasswordStrength | null>(null);
    const [confirmPassword, setConfirmPassword] = useState('');

    const calculatePasswordStrength = (password: string): PasswordStrength => {
      const requirements = {
        minLength: password.length >= 8,
        hasUppercase: /[A-Z]/.test(password),
        hasLowercase: /[a-z]/.test(password),
        hasNumbers: /\d/.test(password),
        hasSpecialChars: /[!@#$%^&*(),.?":{}|<>]/.test(password),
      };

      const score = Object.values(requirements).filter(Boolean).length;
      const feedback = [];

      if (!requirements.minLength) feedback.push('Use at least 8 characters');
      if (!requirements.hasUppercase) feedback.push('Add uppercase letters');
      if (!requirements.hasLowercase) feedback.push('Add lowercase letters');
      if (!requirements.hasNumbers) feedback.push('Add numbers');
      if (!requirements.hasSpecialChars) feedback.push('Add special characters');

      return { score, feedback, requirements };
    };

    useEffect(() => {
      if (config.password) {
        setPasswordStrength(calculatePasswordStrength(config.password));
      } else {
        setPasswordStrength(null);
      }
    }, [config.password]);

    return (
      <div className="wizard-step security-step">
        <div className="form-group">
          <label htmlFor="vault-password" className="form-label">
            Vault Password *
          </label>
          <input
            id="vault-password"
            type="password"
            value={config.password || ''}
            onChange={(e) => updateConfig({ password: e.target.value })}
            placeholder="Enter a strong password"
            className={`form-input ${validation?.errors?.some((e: ValidationError) => e.field === 'password') ? 'error' : ''}`}
          />
          {passwordStrength && (
            <PasswordStrengthIndicator strength={passwordStrength} />
          )}
          {validation?.errors?.find((e: ValidationError) => e.field === 'password') && (
            <span className="error-message">
              {validation.errors.find((e: ValidationError) => e.field === 'password')?.message}
            </span>
          )}
        </div>

        <div className="form-group">
          <label htmlFor="confirm-password" className="form-label">
            Confirm Password *
          </label>
          <input
            id="confirm-password"
            type="password"
            value={confirmPassword}
            onChange={(e) => setConfirmPassword(e.target.value)}
            placeholder="Confirm your password"
            className={`form-input ${confirmPassword && confirmPassword !== config.password ? 'error' : ''}`}
          />
          {confirmPassword && confirmPassword !== config.password && (
            <span className="error-message">Passwords do not match</span>
          )}
        </div>

        <div className="form-group">
          <label htmlFor="encryption-level" className="form-label">
            Encryption Level
          </label>
          <select
            id="encryption-level"
            value={config.encryptionLevel || 'standard'}
            onChange={(e) => updateConfig({ encryptionLevel: e.target.value as EncryptionLevel })}
            className="form-select"
          >
            <option value="standard">Standard (AES-256)</option>
            <option value="high">High Security (AES-256 + Additional Layers)</option>
          </select>
          <span className="form-help">
            Standard encryption is suitable for most use cases. High security adds additional protection layers.
          </span>
        </div>
      </div>
    );
  }

  function KeyboardShortcutStep({ config, updateConfig, validation }: any) {
    const [sequenceValid, setSequenceValid] = useState(true);

    return (
      <div className="wizard-step keyboard-step">
        <KeyboardSequenceConfig
          initialSequence={config.keyboardSequence || ''}
          onSequenceChange={(sequence, isValid) => {
            updateConfig({ keyboardSequence: sequence });
            setSequenceValid(isValid);
          }}
          showAdvanced={true}
          className="wizard-sequence-config"
        />
        
        {!sequenceValid && (
          <div className="sequence-validation-error">
            <span className="error-icon">⚠️</span>
            <span className="error-text">
              Please fix the sequence validation errors above before proceeding.
            </span>
          </div>
        )}
      </div>
    );
  }

  function AdvancedStep({ config, updateConfig }: any) {
    return (
      <div className="wizard-step advanced-step">
        <div className="form-group">
          <label className="checkbox-label">
            <input
              type="checkbox"
              checked={config.autoMount || false}
              onChange={(e) => updateConfig({ autoMount: e.target.checked })}
            />
            <span className="checkbox-text">Auto-mount on startup</span>
          </label>
          <span className="form-help">
            Automatically unlock this vault when PhantomVault starts (requires saved password)
          </span>
        </div>

        <div className="form-group">
          <label className="checkbox-label">
            <input
              type="checkbox"
              checked={config.autoLock !== false}
              onChange={(e) => updateConfig({ autoLock: e.target.checked })}
            />
            <span className="checkbox-text">Enable auto-lock</span>
          </label>
          <span className="form-help">
            Automatically lock the vault when the system is idle or on hotkey press
          </span>
        </div>

        {config.autoLock !== false && (
          <div className="form-group">
            <label htmlFor="lock-timeout" className="form-label">
              Auto-lock timeout (minutes)
            </label>
            <input
              id="lock-timeout"
              type="number"
              min="1"
              max="120"
              value={config.lockTimeout || 15}
              onChange={(e) => updateConfig({ lockTimeout: parseInt(e.target.value) || 15 })}
              className="form-input"
            />
            <span className="form-help">
              Vault will auto-lock after this many minutes of system inactivity
            </span>
          </div>
        )}
      </div>
    );
  }

  function ReviewStep({ config }: any) {
    return (
      <div className="wizard-step review-step">
        <h3>Review Your Vault Configuration</h3>
        
        <div className="review-sections">
          <div className="review-section">
            <h4>Basic Information</h4>
            <div className="review-item">
              <span className="review-label">Name:</span>
              <span className="review-value">{config.name}</span>
            </div>
            <div className="review-item">
              <span className="review-label">Folder:</span>
              <span className="review-value">{config.path}</span>
            </div>
          </div>

          <div className="review-section">
            <h4>Security Settings</h4>
            <div className="review-item">
              <span className="review-label">Encryption:</span>
              <span className="review-value">
                {config.encryptionLevel === 'high' ? 'High Security (AES-256+)' : 'Standard (AES-256)'}
              </span>
            </div>
            <div className="review-item">
              <span className="review-label">Password:</span>
              <span className="review-value">••••••••</span>
            </div>
          </div>

          {config.keyboardSequence && (
            <div className="review-section">
              <h4>Keyboard Shortcut</h4>
              <div className="review-item">
                <span className="review-label">Sequence:</span>
                <span className="review-value">{config.keyboardSequence}</span>
              </div>
            </div>
          )}

          <div className="review-section">
            <h4>Advanced Options</h4>
            <div className="review-item">
              <span className="review-label">Auto-mount:</span>
              <span className="review-value">{config.autoMount ? 'Enabled' : 'Disabled'}</span>
            </div>
            <div className="review-item">
              <span className="review-label">Auto-lock:</span>
              <span className="review-value">
                {config.autoLock !== false ? `Enabled (${config.lockTimeout || 15} minutes)` : 'Disabled'}
              </span>
            </div>
          </div>
        </div>

        <div className="warning-box">
          <div className="warning-icon">⚠️</div>
          <div className="warning-content">
            <h4>Important</h4>
            <p>
              Make sure to remember your password and keep it secure. 
              Without it, you won't be able to access your encrypted files.
            </p>
          </div>
        </div>
      </div>
    );
  }

  // ==================== RENDER ====================

  const CurrentStepComponent = steps[currentStep].component;
  const currentValidation = validation[steps[currentStep].id];

  return (
    <div className="vault-creation-wizard">
      {/* Wizard Header */}
      <div className="wizard-header">
        <h1 className="wizard-title">Create New Vault</h1>
        <button
          onClick={onCancel}
          className="wizard-close"
          disabled={isCreating}
          title="Cancel"
        >
          ✕
        </button>
      </div>

      {/* Progress Indicator */}
      <div className="wizard-progress">
        <div className="progress-steps">
          {steps.map((step, index) => (
            <div
              key={step.id}
              className={`progress-step ${
                index === currentStep ? 'active' : 
                index < currentStep ? 'completed' : 'pending'
              } ${step.isOptional ? 'optional' : ''}`}
              onClick={() => handleStepClick(index)}
            >
              <div className="step-number">
                {index < currentStep ? '✓' : index + 1}
              </div>
              <div className="step-info">
                <div className="step-title">{step.title}</div>
                <div className="step-description">{step.description}</div>
              </div>
            </div>
          ))}
        </div>
        <div className="progress-bar">
          <div 
            className="progress-fill"
            style={{ width: `${((currentStep + 1) / steps.length) * 100}%` }}
          />
        </div>
      </div>

      {/* Step Content */}
      <div className="wizard-content">
        <CurrentStepComponent
          config={config}
          updateConfig={updateConfig}
          validation={currentValidation}
        />
      </div>

      {/* Navigation */}
      <div className="wizard-navigation">
        <button
          onClick={handlePrevious}
          disabled={!canGoPrevious() || isCreating}
          className="nav-button nav-button-secondary"
        >
          ← Previous
        </button>

        <div className="nav-info">
          Step {currentStep + 1} of {steps.length}
          {steps[currentStep].isOptional && (
            <span className="optional-badge">Optional</span>
          )}
        </div>

        <button
          onClick={handleNext}
          disabled={(!canGoNext() && !steps[currentStep].isOptional) || isCreating}
          className="nav-button nav-button-primary"
        >
          {isCreating ? (
            <>
              <span className="loading-spinner">⏳</span>
              Creating...
            </>
          ) : currentStep === steps.length - 1 ? (
            'Create Vault'
          ) : (
            'Next →'
          )}
        </button>
      </div>
    </div>
  );
};