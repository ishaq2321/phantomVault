/**
 * Password Prompt Modal Component
 * 
 * Modal for prompting user password during vault operations
 */

import React, { useState, useCallback, useEffect, useRef } from 'react';
import { PasswordStrength } from '../../types';
import { usePasswordValidation } from '../../hooks/useVaultValidation';
import { PasswordStrengthIndicator } from '../common/PasswordStrengthIndicator';

export interface PasswordPromptModalProps {
  vaultName: string;
  onSubmit: (password: string) => void;
  onCancel: () => void;
  isLoading?: boolean;
  showStrengthIndicator?: boolean;
  allowEmptyPassword?: boolean;
  title?: string;
  message?: string;
}

/**
 * Password prompt modal component
 */
export const PasswordPromptModal: React.FC<PasswordPromptModalProps> = ({
  vaultName,
  onSubmit,
  onCancel,
  isLoading = false,
  showStrengthIndicator = false,
  allowEmptyPassword = false,
  title,
  message,
}) => {
  const [password, setPassword] = useState('');
  const [showPassword, setShowPassword] = useState(false);
  const [attempts, setAttempts] = useState(0);
  const passwordInputRef = useRef<HTMLInputElement>(null);

  // ==================== VALIDATION ====================

  const { strength, isPasswordValid, passwordError } = usePasswordValidation(password);

  // ==================== EFFECTS ====================

  useEffect(() => {
    // Focus password input when modal opens
    if (passwordInputRef.current) {
      passwordInputRef.current.focus();
    }
  }, []);

  useEffect(() => {
    // Handle escape key
    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key === 'Escape' && !isLoading) {
        onCancel();
      }
    };

    document.addEventListener('keydown', handleKeyDown);
    return () => document.removeEventListener('keydown', handleKeyDown);
  }, [onCancel, isLoading]);

  // ==================== HANDLERS ====================

  const handleSubmit = useCallback((event: React.FormEvent) => {
    event.preventDefault();

    if (isLoading) return;

    if (!allowEmptyPassword && !password.trim()) {
      return;
    }

    if (showStrengthIndicator && !isPasswordValid) {
      return;
    }

    setAttempts(prev => prev + 1);
    onSubmit(password);
  }, [password, isLoading, allowEmptyPassword, showStrengthIndicator, isPasswordValid, onSubmit]);

  const handleCancel = useCallback(() => {
    if (!isLoading) {
      onCancel();
    }
  }, [isLoading, onCancel]);

  const togglePasswordVisibility = useCallback(() => {
    setShowPassword(prev => !prev);
  }, []);

  const handlePasswordChange = useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setPassword(event.target.value);
  }, []);

  // ==================== RENDER HELPERS ====================

  const getModalTitle = () => {
    if (title) return title;
    return `Enter Password for "${vaultName}"`;
  };

  const getModalMessage = () => {
    if (message) return message;
    if (attempts > 0) {
      return `Previous attempt failed. Please verify your password and try again. (Attempt ${attempts + 1})`;
    }
    return 'Please enter the vault password to continue.';
  };

  const canSubmit = () => {
    if (isLoading) return false;
    if (!allowEmptyPassword && !password.trim()) return false;
    if (showStrengthIndicator && !isPasswordValid) return false;
    return true;
  };

  // ==================== MAIN RENDER ====================

  return (
    <div className="password-prompt-modal-overlay">
      <div className="password-prompt-modal">
        {/* Header */}
        <div className="modal-header">
          <h2 className="modal-title">{getModalTitle()}</h2>
          {!isLoading && (
            <button
              onClick={handleCancel}
              className="modal-close"
              title="Cancel"
            >
              ✕
            </button>
          )}
        </div>

        {/* Content */}
        <div className="modal-content">
          <div className="modal-message">
            <span className="message-icon">🔐</span>
            <p className="message-text">{getModalMessage()}</p>
          </div>

          <form onSubmit={handleSubmit} className="password-form">
            <div className="password-input-group">
              <div className="input-container">
                <input
                  ref={passwordInputRef}
                  type={showPassword ? 'text' : 'password'}
                  value={password}
                  onChange={handlePasswordChange}
                  placeholder="Enter vault password"
                  className={`password-input ${attempts > 0 ? 'error' : ''}`}
                  disabled={isLoading}
                  autoComplete="current-password"
                />
                <button
                  type="button"
                  onClick={togglePasswordVisibility}
                  className="password-toggle"
                  disabled={isLoading}
                  title={showPassword ? 'Hide password' : 'Show password'}
                >
                  {showPassword ? '👁️' : '👁️‍🗨️'}
                </button>
              </div>

              {/* Password Strength Indicator */}
              {showStrengthIndicator && password && strength && (
                <PasswordStrengthIndicator strength={strength} />
              )}

              {/* Validation Error */}
              {showStrengthIndicator && passwordError && (
                <div className="password-error">
                  <span className="error-icon">⚠️</span>
                  <span className="error-text">{passwordError}</span>
                </div>
              )}

              {/* Attempt Warning */}
              {attempts > 0 && (
                <div className="attempt-warning">
                  <span className="warning-icon">⚠️</span>
                  <span className="warning-text">
                    {attempts === 1 ? 'Incorrect password. Please try again.' : 
                     attempts === 2 ? 'Still incorrect. Please check your password carefully.' :
                     'Multiple failed attempts. Please verify you have the correct password.'}
                  </span>
                </div>
              )}
            </div>

            {/* Security Notice */}
            <div className="security-notice">
              <span className="notice-icon">🛡️</span>
              <span className="notice-text">
                Your password is encrypted and never stored in plain text.
              </span>
            </div>
          </form>
        </div>

        {/* Footer */}
        <div className="modal-footer">
          <button
            type="button"
            onClick={handleCancel}
            className="footer-button secondary"
            disabled={isLoading}
          >
            Cancel
          </button>
          <button
            onClick={handleSubmit}
            className="footer-button primary"
            disabled={!canSubmit()}
          >
            {isLoading ? (
              <>
                <span className="loading-spinner">⏳</span>
                Mounting...
              </>
            ) : (
              <>
                <span className="button-icon">🔓</span>
                Mount Vault
              </>
            )}
          </button>
        </div>

        {/* Loading Overlay */}
        {isLoading && (
          <div className="loading-overlay">
            <div className="loading-content">
              <div className="loading-spinner-large">⏳</div>
              <div className="loading-text">Mounting vault...</div>
              <div className="loading-subtext">Please wait while we unlock your vault</div>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};