/**
 * Keyboard Sequence Input Component
 * 
 * Component for entering and validating keyboard sequences for vault access
 */

import React, { useState, useCallback, useEffect } from 'react';

interface KeyboardSequenceInputProps {
  value: string;
  onChange: (sequence: string) => void;
  placeholder?: string;
  existingSequences?: string[];
  className?: string;
}

/**
 * Keyboard sequence input component
 */
export const KeyboardSequenceInput: React.FC<KeyboardSequenceInputProps> = ({
  value,
  onChange,
  placeholder = 'Enter keyboard sequence',
  existingSequences = [],
  className = '',
}) => {
  const [isRecording, setIsRecording] = useState(false);
  const [recordedKeys, setRecordedKeys] = useState<string[]>([]);
  const [validationMessage, setValidationMessage] = useState<string>('');

  // ==================== SEQUENCE VALIDATION ====================

  const validateSequence = useCallback((sequence: string): { isValid: boolean; message: string } => {
    if (!sequence) {
      return { isValid: true, message: '' }; // Optional field
    }

    // Check minimum length
    if (sequence.length < 2) {
      return { isValid: false, message: 'Sequence must be at least 2 characters' };
    }

    // Check maximum length
    if (sequence.length > 20) {
      return { isValid: false, message: 'Sequence must be 20 characters or less' };
    }

    // Check for valid characters (letters, numbers, and basic symbols)
    if (!/^[a-zA-Z0-9!@#$%^&*()_+\-=\[\]{}|;:,.<>?]+$/.test(sequence)) {
      return { isValid: false, message: 'Sequence contains invalid characters' };
    }

    // Check for uniqueness
    if (existingSequences.includes(sequence)) {
      return { isValid: false, message: 'This sequence is already in use' };
    }

    // Check for common patterns to avoid
    const commonPatterns = ['1234', 'abcd', 'qwerty', 'password', 'admin'];
    if (commonPatterns.some(pattern => sequence.toLowerCase().includes(pattern))) {
      return { isValid: false, message: 'Avoid common patterns for better security' };
    }

    return { isValid: true, message: 'Valid sequence' };
  }, [existingSequences]);

  // ==================== KEYBOARD RECORDING ====================

  const startRecording = useCallback(() => {
    setIsRecording(true);
    setRecordedKeys([]);
    setValidationMessage('Type your sequence...');
  }, []);

  const stopRecording = useCallback(() => {
    setIsRecording(false);
    const sequence = recordedKeys.join('');
    if (sequence) {
      onChange(sequence);
      const validation = validateSequence(sequence);
      setValidationMessage(validation.message);
    }
    setRecordedKeys([]);
  }, [recordedKeys, onChange, validateSequence]);

  const handleKeyDown = useCallback((event: KeyboardEvent) => {
    if (!isRecording) return;

    event.preventDefault();
    event.stopPropagation();

    // Handle special keys
    if (event.key === 'Escape') {
      stopRecording();
      return;
    }

    if (event.key === 'Enter') {
      stopRecording();
      return;
    }

    if (event.key === 'Backspace') {
      setRecordedKeys(prev => prev.slice(0, -1));
      return;
    }

    // Record printable characters
    if (event.key.length === 1) {
      setRecordedKeys(prev => {
        const newKeys = [...prev, event.key];
        // Limit to 20 characters
        return newKeys.slice(0, 20);
      });
    }
  }, [isRecording, stopRecording]);

  // ==================== EFFECTS ====================

  useEffect(() => {
    if (isRecording) {
      document.addEventListener('keydown', handleKeyDown, true);
      return () => {
        document.removeEventListener('keydown', handleKeyDown, true);
      };
    }
  }, [isRecording, handleKeyDown]);

  useEffect(() => {
    if (value) {
      const validation = validateSequence(value);
      setValidationMessage(validation.message);
    } else {
      setValidationMessage('');
    }
  }, [value, validateSequence]);

  // ==================== SEQUENCE FORMATTING ====================

  const formatSequence = (sequence: string): string => {
    if (!sequence) return '';
    
    // Add visual separators for readability
    return sequence.split('').map((char, index) => {
      if (index > 0 && index % 4 === 0) {
        return `-${char}`;
      }
      return char;
    }).join('');
  };

  const getSequencePreview = (sequence: string): { mode: string; password: string } | null => {
    if (!sequence || sequence.length < 2) return null;
    
    const firstChar = sequence[0].toLowerCase();
    if (firstChar === 't') {
      return { mode: 'Temporary', password: sequence.slice(1) };
    } else if (firstChar === 'p') {
      return { mode: 'Permanent', password: sequence.slice(1) };
    } else {
      return { mode: 'Temporary (default)', password: sequence };
    }
  };

  // ==================== RENDER ====================

  const validation = validateSequence(value);
  const sequencePreview = getSequencePreview(value);

  return (
    <div className={`keyboard-sequence-input ${className}`}>
      {/* Input Container */}
      <div className="input-container">
        <div className="sequence-display">
          {isRecording ? (
            <div className="recording-display">
              <span className="recording-indicator">🔴 Recording...</span>
              <span className="recorded-sequence">
                {recordedKeys.join('') || 'Type your sequence'}
              </span>
            </div>
          ) : (
            <input
              type="text"
              value={value}
              onChange={(e) => onChange(e.target.value)}
              placeholder={placeholder}
              className={`sequence-input ${!validation.isValid ? 'error' : ''}`}
              maxLength={20}
            />
          )}
        </div>

        <button
          type="button"
          onClick={isRecording ? stopRecording : startRecording}
          className={`record-button ${isRecording ? 'recording' : ''}`}
          title={isRecording ? 'Stop recording (Enter or Esc)' : 'Record sequence'}
        >
          {isRecording ? '⏹️ Stop' : '⏺️ Record'}
        </button>
      </div>

      {/* Sequence Preview */}
      {sequencePreview && validation.isValid && (
        <div className="sequence-preview">
          <div className="preview-label">Preview:</div>
          <div className="preview-content">
            <span className="preview-mode">{sequencePreview.mode}</span>
            <span className="preview-separator">•</span>
            <span className="preview-password">Password: {sequencePreview.password}</span>
          </div>
        </div>
      )}

      {/* Validation Message */}
      {validationMessage && (
        <div className={`validation-message ${validation.isValid ? 'success' : 'error'}`}>
          <span className="validation-icon">
            {validation.isValid ? '✓' : '⚠️'}
          </span>
          <span className="validation-text">{validationMessage}</span>
        </div>
      )}

      {/* Examples */}
      <div className="sequence-examples">
        <div className="examples-title">Examples:</div>
        <div className="examples-list">
          <div className="example-item">
            <code>T1234</code> - Temporary unlock with password "1234"
          </div>
          <div className="example-item">
            <code>Pmysecret</code> - Permanent unlock with password "mysecret"
          </div>
          <div className="example-item">
            <code>vault123</code> - Default (temporary) unlock with password "vault123"
          </div>
        </div>
      </div>

      {/* Recording Instructions */}
      {isRecording && (
        <div className="recording-instructions">
          <div className="instructions-title">Recording keyboard sequence:</div>
          <ul className="instructions-list">
            <li>Type your desired sequence (letters, numbers, symbols)</li>
            <li>Press <kbd>Enter</kbd> or click Stop when finished</li>
            <li>Press <kbd>Esc</kbd> to cancel</li>
            <li>Press <kbd>Backspace</kbd> to delete last character</li>
          </ul>
        </div>
      )}
    </div>
  );
};