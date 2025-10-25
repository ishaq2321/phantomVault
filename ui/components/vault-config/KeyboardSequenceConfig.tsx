/**
 * Keyboard Sequence Configuration Component
 * 
 * Advanced keyboard sequence configuration with analysis, conflict detection,
 * and generation capabilities for vault access sequences.
 */

import React, { useState, useMemo, useCallback } from 'react';
import { KeyboardSequenceInput } from '../common/KeyboardSequenceInput';
import { useVaultContext } from '../../contexts/VaultContext';
import { useVaultValidation } from '../../hooks/useVaultValidation';
import './KeyboardSequenceConfig.css';

// ==================== TYPES ====================

interface KeyboardSequenceConfigProps {
  value: string;
  onChange: (value: string) => void;
  currentVaultId?: string;
  disabled?: boolean;
  showAdvancedOptions?: boolean;
}

interface SequenceAnalysis {
  length: number;
  entropy: number;
  strength: 'WEAK' | 'MODERATE' | 'STRONG' | 'VERY_STRONG';
  features: string[];
}

interface ConflictInfo {
  vaultId: string;
  vaultName: string;
  sequence: string;
  similarity: number;
}

type SequenceMode = 'simple' | 'advanced';

// ==================== CONSTANTS ====================

const CHARSET_SIZES = {
  lowercase: 26,
  uppercase: 26,
  numbers: 10,
  symbols: 32,
};

const STRENGTH_THRESHOLDS = {
  WEAK: 30,
  MODERATE: 50,
  STRONG: 70,
  VERY_STRONG: 90,
};

// ==================== UTILITY FUNCTIONS ====================

const calculateEntropy = (sequence: string): number => {
  if (!sequence) return 0;
  
  const hasLowercase = /[a-z]/.test(sequence);
  const hasUppercase = /[A-Z]/.test(sequence);
  const hasNumbers = /\d/.test(sequence);
  const hasSymbols = /[^a-zA-Z0-9]/.test(sequence);
  
  let charsetSize = 0;
  if (hasLowercase) charsetSize += CHARSET_SIZES.lowercase;
  if (hasUppercase) charsetSize += CHARSET_SIZES.uppercase;
  if (hasNumbers) charsetSize += CHARSET_SIZES.numbers;
  if (hasSymbols) charsetSize += CHARSET_SIZES.symbols;
  
  return sequence.length * Math.log2(charsetSize || 1);
};

const analyzeSequence = (sequence: string): SequenceAnalysis => {
  if (!sequence) {
    return {
      length: 0,
      entropy: 0,
      strength: 'WEAK',
      features: [],
    };
  }
  
  const entropy = calculateEntropy(sequence);
  const features: string[] = [];
  
  if (/[a-z]/.test(sequence)) features.push('Letters');
  if (/[A-Z]/.test(sequence)) features.push('Mixed Case');
  if (/\d/.test(sequence)) features.push('Numbers');
  if (/[^a-zA-Z0-9]/.test(sequence)) features.push('Symbols');
  if (sequence.length >= 8) features.push('Long');
  if (!/(.)\1{2,}/.test(sequence)) features.push('No Repeats');
  
  let strength: SequenceAnalysis['strength'] = 'WEAK';
  if (entropy >= STRENGTH_THRESHOLDS.VERY_STRONG) strength = 'VERY_STRONG';
  else if (entropy >= STRENGTH_THRESHOLDS.STRONG) strength = 'STRONG';
  else if (entropy >= STRENGTH_THRESHOLDS.MODERATE) strength = 'MODERATE';
  
  return {
    length: sequence.length,
    entropy,
    strength,
    features,
  };
};

const calculateSimilarity = (str1: string, str2: string): number => {
  if (!str1 || !str2) return 0;
  if (str1 === str2) return 100;
  
  const longer = str1.length > str2.length ? str1 : str2;
  const shorter = str1.length > str2.length ? str2 : str1;
  
  if (longer.length === 0) return 100;
  
  const editDistance = levenshteinDistance(longer, shorter);
  return Math.round(((longer.length - editDistance) / longer.length) * 100);
};

const levenshteinDistance = (str1: string, str2: string): number => {
  const matrix = Array(str2.length + 1).fill(null).map(() => Array(str1.length + 1).fill(null));
  
  for (let i = 0; i <= str1.length; i++) matrix[0][i] = i;
  for (let j = 0; j <= str2.length; j++) matrix[j][0] = j;
  
  for (let j = 1; j <= str2.length; j++) {
    for (let i = 1; i <= str1.length; i++) {
      const indicator = str1[i - 1] === str2[j - 1] ? 0 : 1;
      matrix[j][i] = Math.min(
        matrix[j][i - 1] + 1,
        matrix[j - 1][i] + 1,
        matrix[j - 1][i - 1] + indicator
      );
    }
  }
  
  return matrix[str2.length][str1.length];
};

const generateRandomSequence = (length: number = 8): string => {
  const lowercase = 'abcdefghijklmnopqrstuvwxyz';
  const uppercase = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
  const numbers = '0123456789';
  const symbols = '!@#$%^&*()_+-=[]{}|;:,.<>?';
  
  const allChars = lowercase + uppercase + numbers + symbols;
  let result = '';
  
  // Ensure at least one character from each category
  result += lowercase[Math.floor(Math.random() * lowercase.length)];
  result += uppercase[Math.floor(Math.random() * uppercase.length)];
  result += numbers[Math.floor(Math.random() * numbers.length)];
  result += symbols[Math.floor(Math.random() * symbols.length)];
  
  // Fill the rest randomly
  for (let i = 4; i < length; i++) {
    result += allChars[Math.floor(Math.random() * allChars.length)];
  }
  
  // Shuffle the result
  return result.split('').sort(() => Math.random() - 0.5).join('');
};

// ==================== MAIN COMPONENT ====================

export const KeyboardSequenceConfig: React.FC<KeyboardSequenceConfigProps> = ({
  value,
  onChange,
  currentVaultId,
  disabled = false,
  showAdvancedOptions = false,
}) => {
  const { state } = useVaultContext();
  const { validateField } = useVaultValidation({
    existingVaults: state.vaults,
    currentVaultId,
  });
  
  const [showConflicts, setShowConflicts] = useState(true);
  const [sequenceMode, setSequenceMode] = useState<SequenceMode>('simple');
  
  // ==================== COMPUTED VALUES ====================
  
  const existingSequences = useMemo(() => {
    return state.vaults
      .filter(vault => vault.id !== currentVaultId)
      .map(vault => vault.name);
  }, [state.vaults, currentVaultId]);
  
  const analysis = useMemo(() => analyzeSequence(value), [value]);
  
  const conflicts = useMemo((): ConflictInfo[] => {
    if (!value) return [];
    
    return state.vaults
      .filter(vault => vault.id !== currentVaultId)
      .map(vault => ({
        vaultId: vault.id,
        vaultName: vault.name,
        sequence: vault.name, // Using name as sequence for demo
        similarity: calculateSimilarity(value, vault.name),
      }))
      .filter(conflict => conflict.similarity > 50)
      .sort((a, b) => b.similarity - a.similarity);
  }, [value, state.vaults, currentVaultId]);
  
  const validation = useMemo(() => {
    return validateField('keyboardSequence', value);
  }, [validateField, value]);
  
  // ==================== EVENT HANDLERS ====================
  
  const handleGenerateRandom = useCallback(() => {
    let attempts = 0;
    let newSequence: string;
    
    do {
      newSequence = generateRandomSequence(Math.floor(Math.random() * 3) + 6); // 6-8 chars
      attempts++;
    } while (
      attempts < 10 && 
      existingSequences.some(seq => calculateSimilarity(newSequence, seq) > 80)
    );
    
    onChange(newSequence);
  }, [onChange, existingSequences]);
  
  const handleClear = useCallback(() => {
    onChange('');
  }, [onChange]);
  
  const handleModeChange = useCallback((mode: SequenceMode) => {
    setSequenceMode(mode);
  }, []);
  
  // ==================== RENDER HELPERS ====================
  
  const renderValidationError = () => {
    if (validation.isValid || !validation.errors.length) return null;
    
    return (
      <div className="validation-error">
        <span className="error-icon">⚠️</span>
        <span className="error-message">{validation.errors[0].message}</span>
      </div>
    );
  };
  
  const renderSequenceAnalysis = () => {
    if (!value) return null;
    
    return (
      <div className="sequence-analysis">
        <div className="analysis-header">
          <span className="analysis-title">Sequence Analysis</span>
          <span className={`strength-indicator strength-${analysis.strength.toLowerCase()}`}>
            {analysis.strength}
          </span>
        </div>
        <div className="analysis-details">
          <div className="analysis-row">
            <span className="analysis-label">Length:</span>
            <span className="analysis-value">{analysis.length} characters</span>
          </div>
          <div className="analysis-row">
            <span className="analysis-label">Entropy:</span>
            <span className="analysis-value">{analysis.entropy.toFixed(1)} bits</span>
          </div>
          {analysis.features.length > 0 && (
            <div className="analysis-features">
              {analysis.features.map(feature => (
                <span key={feature} className="feature-tag">{feature}</span>
              ))}
            </div>
          )}
        </div>
      </div>
    );
  };
  
  const renderConflicts = () => {
    if (!conflicts.length) return null;
    
    return (
      <div className="sequence-conflicts">
        <div className="conflicts-header">
          <span className="conflicts-title">
            ⚠️ Potential Conflicts ({conflicts.length})
          </span>
          <button
            className="conflicts-toggle"
            onClick={() => setShowConflicts(!showConflicts)}
            aria-label={showConflicts ? 'Hide conflicts' : 'Show conflicts'}
          >
            {showConflicts ? '▼' : '▶'}
          </button>
        </div>
        {showConflicts && (
          <div className="conflicts-list">
            {conflicts.map(conflict => (
              <div key={conflict.vaultId} className="conflict-item">
                <div className="conflict-info">
                  <div className="conflict-vault">{conflict.vaultName}</div>
                  <div className="conflict-sequence">
                    Sequence: <code>{conflict.sequence}</code>
                  </div>
                </div>
                <div className="conflict-similarity">
                  {conflict.similarity}% similar
                </div>
              </div>
            ))}
          </div>
        )}
      </div>
    );
  };
  
  const renderAdvancedOptions = () => {
    if (!showAdvancedOptions) return null;
    
    return (
      <div className="advanced-options">
        <div className="options-header">
          <span className="options-title">Advanced Options</span>
        </div>
        <div className="options-content">
          <div className="option-group">
            <label className="option-label">
              <input
                type="radio"
                name="sequenceMode"
                value="simple"
                checked={sequenceMode === 'simple'}
                onChange={() => handleModeChange('simple')}
              />
              <span className="option-text">Simple Mode</span>
            </label>
            <div className="option-description">
              Basic sequence input with standard validation
            </div>
          </div>
          <div className="option-group">
            <label className="option-label">
              <input
                type="radio"
                name="sequenceMode"
                value="advanced"
                checked={sequenceMode === 'advanced'}
                onChange={() => handleModeChange('advanced')}
              />
              <span className="option-text">Advanced Mode</span>
            </label>
            <div className="option-description">
              Enhanced security analysis and conflict detection
            </div>
          </div>
        </div>
        <div className="options-actions">
          <button
            className="action-button secondary"
            onClick={handleGenerateRandom}
            disabled={disabled}
          >
            🎲 Generate Random
          </button>
          <button
            className="action-button secondary"
            onClick={handleClear}
            disabled={disabled || !value}
          >
            🗑️ Clear
          </button>
        </div>
      </div>
    );
  };
  
  // ==================== MAIN RENDER ====================
  
  return (
    <div className={`keyboard-sequence-config ${disabled ? 'disabled' : ''}`}>
      {renderValidationError()}
      
      <KeyboardSequenceInput
        value={value}
        onChange={onChange}
        existingSequences={existingSequences}
        disabled={disabled}
      />
      
      {renderSequenceAnalysis()}
      {renderConflicts()}
      {renderAdvancedOptions()}
    </div>
  );
};