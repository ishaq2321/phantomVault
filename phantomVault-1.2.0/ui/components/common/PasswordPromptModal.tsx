/**
 * Password Prompt Modal Component
 * 
 * Modal dialog for prompting user password input with optional mode selection
 */

import React, { useState, useCallback, useEffect, useRef } from 'react';
import { UnlockMode } from '../../types';

interface PasswordPromptModalProps {
  title: string;
  message: string;
  showModeSelection?: boolean;
  onSubmit: (password: string, mode?: UnlockMode) => void;
  onCancel: () => void;
}

/**
 * Password prompt modal component
 */
export const PasswordPromptModal: React.FC<PasswordPromptModalProps> = ({
  title,
  message,
  showModeSelection = false,
  onSubmit,
  onCancel,
}) => {
  const [password, setPassword] = useState('');
  const [mode, setMode] = useState<UnlockMode>('temporary');
  const [showPassword, setShowPassword] = useState(false);
  const [isSubmitting, setIsSubmitting] = useState(false);
  
  const passwordInputRef = useRef<HTMLInputElement>(null);

  // Focus password input when modal opens
  useEffect(() => {
    const timer = setTimeout(() => {
      passwordInputRef.current?.focus();
    }, 100);
    
    return () => clearTimeout(timer);
  }, []);

  // Handle keyboard shortcuts
  useEffect(() => {
    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key === 'Escape') {
        onCancel();
      } else if (event.key === 'Enter' && !event.shiftKey) {
        event.preventDefault();
        handleSubmit();
      }
    };

    document.addEventListener('keydown', handleKeyDown);
    return () => document.removeEventListener('keydown', handleKeyDown);
  }, [password, mode, onCancel]);

  // Handle form submission
  const handleSubmit = useCallback(async () => {
    if (!password.trim()) {
      return;
    }

    setIsSubmitting(true);
    
    try {
      await onSubmit(password, showModeSelection ? mode : undefined);
    } catch (error) {
      console.error('Password submission failed:', error);
    } finally {
      setIsSubmitting(false);
    }
  }, [password, mode, showModeSelection, onSubmit]);

  // Handle backdrop click
  const handleBackdropClick = useCallback((event: React.MouseEvent) => {
    if (event.target === event.currentTarget) {
      onCancel();
    }
  }, [onCancel]);

  return (
    <div className="modal-backdrop" onClick={handleBackdropClick}>
      <div className="modal-container">
        <div className="modal-header">
          <h2 className="modal-title">{title}</h2>
          <button
            onClick={onCancel}
            className="modal-close-button"
            disabled={isSubmitting}
            title="Close"
          >
            ✕
          </button>
        </div>
        
        <div className="modal-content">
          <p className="modal-message">{message}</p>
          
          <form onSubmit={(e) => { e.preventDefault(); handleSubmit(); }}>
            <div className="form-group">
              <label htmlFor="password-input" className="form-label">
                Password
              </label>
              <div className="password-input-container">
                <input
                  ref={passwordInputRef}
                  id="password-input"
                  type={showPassword ? 'text' : 'password'}
                  value={password}
                  onChange={(e) => setPassword(e.target.value)}
                  placeholder="Enter your password"
                  className="form-input password-input"
                  disabled={isSubmitting}
                  autoComplete="current-password"
                />
                <button
                  type="button"
                  onClick={() => setShowPassword(!showPassword)}
                  className="password-toggle-button"
                  disabled={isSubmitting}
                  title={showPassword ? 'Hide password' : 'Show password'}
                >
                  {showPassword ? '👁️' : '👁️‍🗨️'}
                </button>
              </div>
            </div>
            
            {showModeSelection && (
              <div className="form-group">
                <label className="form-label">Unlock Mode</label>
                <div className="mode-selection">
                  <label className="mode-option">
                    <input
                      type="radio"
                      name="unlock-mode"
                      value="temporary"
                      checked={mode === 'temporary'}
                      onChange={(e) => setMode(e.target.value as UnlockMode)}
                      disabled={isSubmitting}
                    />
                    <div className="mode-content">
                      <span className="mode-title">Temporary</span>
                      <span className="mode-description">
                        Auto-lock when system is idle or on hotkey press
                      </span>
                    </div>
                  </label>
                  
                  <label className="mode-option">
                    <input
                      type="radio"
                      name="unlock-mode"
                      value="permanent"
                      checked={mode === 'permanent'}
                      onChange={(e) => setMode(e.target.value as UnlockMode)}
                      disabled={isSubmitting}
                    />
                    <div className="mode-content">
                      <span className="mode-title">Permanent</span>
                      <span className="mode-description">
                        Stay unlocked until manually locked
                      </span>
                    </div>
                  </label>
                </div>
              </div>
            )}
          </form>
        </div>
        
        <div className="modal-footer">
          <button
            onClick={onCancel}
            className="modal-button modal-button-secondary"
            disabled={isSubmitting}
          >
            Cancel
          </button>
          
          <button
            onClick={handleSubmit}
            className="modal-button modal-button-primary"
            disabled={!password.trim() || isSubmitting}
          >
            {isSubmitting ? (
              <>
                <span className="button-spinner">⏳</span>
                <span>Processing...</span>
              </>
            ) : (
              <>
                <span className="button-icon">🔓</span>
                <span>Submit</span>
              </>
            )}
          </button>
        </div>
      </div>
    </div>
  );
};