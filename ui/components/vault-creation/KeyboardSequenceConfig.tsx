/**
 * Keyboard Sequence Configuration Component
 * 
 * Advanced keyboard sequence configuration with validation, conflict detection,
 * and visual feedback for vault creation and editing
 */

import React, { useState, useCallback, useEffect } from 'react';
import { VaultInfo } from '../../types';
import { useKeyboardSequences, useSequenceValidation } from '../../hooks/useKeyboardSequences';
import { KeyboardSequenceInput } from '../common/KeyboardSequenceInput';

interface KeyboardSequenceConfigProps {
  vault?: VaultInfo;
  initialSequence?: string;
  onSequenceChange: (sequence: string, isValid: boolean) => void;
  onSequenceSet?: (sequence: string) => Promise<void>;
  showAdvanced?: boolean;
  className?: string;
}

/**
 * Keyboard sequence configuration component
 */
export const KeyboardSequenceConfig: React.FC<KeyboardSequenceConfigProps> = ({
  vault,
  initialSequence = '',
  onSequenceChange,
  onSequenceSet,
  showAdvanced = true,
  className = '',
}) => {
  const [sequence, setSequence] = useState(initialSequence);
  const [showSuggestions, setShowSuggestions] = useState(false);
  const [isSettingSequence, setIsSettingSequence] = useState(false);

  const {
    generateSuggestions,
    parseSequence,
    getUsageStats,
    allSequences,
  } = useKeyboardSequences();

  const {
    validation,
    conflicts,
    isValid,
    hasErrors,
    hasWarnings,
    hasConflicts,
    exactConflicts,
    similarConflicts,
  } = useSequenceValidation(sequence, vault?.id);

  // ==================== SEQUENCE MANAGEMENT ====================

  const handleSequenceChange = useCallback((newSequence: string) => {
    setSequence(newSequence);
    onSequenceChange(newSequence, isValid);
  }, [onSequenceChange, isValid]);

  const handleSetSequence = useCallback(async () => {
    if (!onSequenceSet || !isValid) {
      return;
    }

    try {
      setIsSettingSequence(true);
      await onSequenceSet(sequence);
    } catch (error) {
      console.error('Failed to set sequence:', error);
    } finally {
      setIsSettingSequence(false);
    }
  }, [onSequenceSet, sequence, isValid]);

  // ==================== SUGGESTIONS ====================

  const suggestions = vault ? generateSuggestions(vault.name, 8) : [];
  const usageStats = getUsageStats();

  const handleSuggestionClick = useCallback((suggestion: string) => {
    handleSequenceChange(suggestion);
    setShowSuggestions(false);
  }, [handleSequenceChange]);

  // ==================== SEQUENCE ANALYSIS ====================

  const sequenceAnalysis = parseSequence(sequence);
  
  const getSequenceStrength = (): 'weak' | 'fair' | 'good' | 'strong' => {
    if (!sequence || sequence.length < 2) return 'weak';
    if (hasErrors || exactConflicts.length > 0) return 'weak';
    if (hasWarnings || similarConflicts.length > 0) return 'fair';
    if (sequence.length >= 6 && /[A-Z]/.test(sequence) && /\d/.test(sequence)) return 'strong';
    return 'good';
  };

  const getStrengthColor = (strength: string): string => {
    switch (strength) {
      case 'weak': return '#F44336';
      case 'fair': return '#FF9800';
      case 'good': return '#8BC34A';
      case 'strong': return '#4CAF50';
      default: return '#9E9E9E';
    }
  };

  // ==================== EFFECTS ====================

  useEffect(() => {
    setSequence(initialSequence);
  }, [initialSequence]);

  // ==================== RENDER ====================

  const sequenceStrength = getSequenceStrength();
  const strengthColor = getStrengthColor(sequenceStrength);

  return (
    <div className={`keyboard-sequence-config ${className}`}>
      {/* Main Input */}
      <div className="sequence-input-section">
        <KeyboardSequenceInput
          value={sequence}
          onChange={handleSequenceChange}
          placeholder="Enter keyboard sequence (e.g., T1234, Pmysecret)"
          existingSequences={allSequences.map(s => s.sequence)}
        />
        
        {onSequenceSet && (
          <button
            onClick={handleSetSequence}
            disabled={!isValid || isSettingSequence}
            className="set-sequence-button"
          >
            {isSettingSequence ? (
              <>
                <span className="loading-spinner">⏳</span>
                Setting...
              </>
            ) : (
              <>
                <span className="button-icon">💾</span>
                Set Sequence
              </>
            )}
          </button>
        )}
      </div>

      {/* Sequence Analysis */}
      {sequence && (
        <div className="sequence-analysis">
          <div className="analysis-header">
            <h4>Sequence Analysis</h4>
            <div className="strength-indicator" style={{ color: strengthColor }}>
              <span className="strength-icon">
                {sequenceStrength === 'strong' ? '🔒' : 
                 sequenceStrength === 'good' ? '🔐' : 
                 sequenceStrength === 'fair' ? '🔓' : '⚠️'}
              </span>
              <span className="strength-text">{sequenceStrength.toUpperCase()}</span>
            </div>
          </div>

          <div className="analysis-content">
            {/* Mode and Password */}
            <div className="sequence-breakdown">
              <div className="breakdown-item">
                <span className="breakdown-label">Mode:</span>
                <span className={`breakdown-value mode-${sequenceAnalysis.mode}`}>
                  {sequenceAnalysis.mode === 'temporary' ? 'Temporary (T)' :
                   sequenceAnalysis.mode === 'permanent' ? 'Permanent (P)' :
                   'Default (Temporary)'}
                </span>
              </div>
              <div className="breakdown-item">
                <span className="breakdown-label">Password:</span>
                <span className="breakdown-value password-value">
                  {sequenceAnalysis.password || sequence}
                </span>
              </div>
            </div>

            {/* Validation Results */}
            {hasErrors && (
              <div className="validation-section errors">
                <h5>❌ Errors</h5>
                <ul>
                  {validation.errors.map((error, index) => (
                    <li key={index}>{error}</li>
                  ))}
                </ul>
              </div>
            )}

            {hasWarnings && (
              <div className="validation-section warnings">
                <h5>⚠️ Warnings</h5>
                <ul>
                  {validation.warnings.map((warning, index) => (
                    <li key={index}>{warning}</li>
                  ))}
                </ul>
              </div>
            )}

            {validation.suggestions.length > 0 && (
              <div className="validation-section suggestions">
                <h5>💡 Suggestions</h5>
                <ul>
                  {validation.suggestions.map((suggestion, index) => (
                    <li key={index}>{suggestion}</li>
                  ))}
                </ul>
              </div>
            )}

            {/* Conflicts */}
            {hasConflicts && (
              <div className="conflicts-section">
                <h5>🔄 Conflicts</h5>
                {exactConflicts.map((conflict, index) => (
                  <div key={index} className="conflict-item exact">
                    <span className="conflict-icon">🚫</span>
                    <span className="conflict-text">
                      Exact match with "{conflict.existingSequence.vaultName}"
                    </span>
                  </div>
                ))}
                {similarConflicts.map((conflict, index) => (
                  <div key={index} className="conflict-item similar">
                    <span className="conflict-icon">⚠️</span>
                    <span className="conflict-text">
                      Similar to "{conflict.existingSequence.vaultName}" 
                      ({Math.round(conflict.similarity * 100)}% similar)
                    </span>
                  </div>
                ))}
              </div>
            )}
          </div>
        </div>
      )}

      {/* Suggestions */}
      {showAdvanced && (
        <div className="suggestions-section">
          <div className="suggestions-header">
            <h4>Suggestions</h4>
            <button
              onClick={() => setShowSuggestions(!showSuggestions)}
              className="toggle-suggestions-button"
            >
              {showSuggestions ? '▼' : '▶'} 
              {suggestions.length} available
            </button>
          </div>

          {showSuggestions && (
            <div className="suggestions-grid">
              {suggestions.map((suggestion, index) => {
                const suggestionAnalysis = parseSequence(suggestion);
                return (
                  <button
                    key={index}
                    onClick={() => handleSuggestionClick(suggestion)}
                    className="suggestion-item"
                  >
                    <div className="suggestion-sequence">{suggestion}</div>
                    <div className="suggestion-details">
                      <span className="suggestion-mode">
                        {suggestionAnalysis.mode === 'temporary' ? 'T' :
                         suggestionAnalysis.mode === 'permanent' ? 'P' : 'D'}
                      </span>
                      <span className="suggestion-password">
                        {suggestionAnalysis.password}
                      </span>
                    </div>
                  </button>
                );
              })}
            </div>
          )}
        </div>
      )}

      {/* Usage Statistics */}
      {showAdvanced && usageStats.totalSequences > 0 && (
        <div className="usage-stats">
          <h4>Usage Statistics</h4>
          <div className="stats-grid">
            <div className="stat-item">
              <div className="stat-value">{usageStats.totalSequences}</div>
              <div className="stat-label">Total Sequences</div>
            </div>
            <div className="stat-item">
              <div className="stat-value">{usageStats.activeSequences}</div>
              <div className="stat-label">Active</div>
            </div>
            <div className="stat-item">
              <div className="stat-value">{usageStats.totalUsage}</div>
              <div className="stat-label">Total Uses</div>
            </div>
          </div>

          {usageStats.mostUsedSequence && (
            <div className="most-used">
              <span className="most-used-label">Most used:</span>
              <span className="most-used-sequence">
                {usageStats.mostUsedSequence.sequence}
              </span>
              <span className="most-used-vault">
                ({usageStats.mostUsedSequence.vaultName})
              </span>
              <span className="most-used-count">
                {usageStats.mostUsedSequence.useCount} uses
              </span>
            </div>
          )}
        </div>
      )}

      {/* Help Information */}
      <div className="sequence-help">
        <div className="help-title">💡 How keyboard sequences work</div>
        <div className="help-content">
          <div className="help-item">
            <strong>T + password:</strong> Temporary unlock (auto-locks when idle)
          </div>
          <div className="help-item">
            <strong>P + password:</strong> Permanent unlock (until manually locked)
          </div>
          <div className="help-item">
            <strong>password only:</strong> Default temporary unlock
          </div>
          <div className="help-item">
            <strong>Usage:</strong> Press global hotkey, then type your sequence anywhere
          </div>
        </div>
      </div>
    </div>
  );
};