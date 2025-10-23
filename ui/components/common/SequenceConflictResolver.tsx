/**
 * Sequence Conflict Resolver Component
 * 
 * Component for resolving keyboard sequence conflicts with existing vaults
 */

import React, { useState, useCallback } from 'react';
import { SequenceConflict } from '../../services/KeyboardSequenceManager';

interface SequenceConflictResolverProps {
  conflicts: SequenceConflict[];
  proposedSequence: string;
  onResolve: (resolution: ConflictResolution) => void;
  onCancel: () => void;
}

export type ConflictResolution = 
  | { action: 'use_anyway'; sequence: string }
  | { action: 'modify_sequence'; sequence: string }
  | { action: 'replace_existing'; sequenceId: string; sequence: string }
  | { action: 'cancel' };

/**
 * Sequence conflict resolver component
 */
export const SequenceConflictResolver: React.FC<SequenceConflictResolverProps> = ({
  conflicts,
  proposedSequence,
  onResolve,
  onCancel,
}) => {
  const [selectedResolution, setSelectedResolution] = useState<ConflictResolution['action']>('modify_sequence');
  const [modifiedSequence, setModifiedSequence] = useState(proposedSequence);

  // ==================== CONFLICT ANALYSIS ====================

  const exactConflicts = conflicts.filter(c => c.conflictType === 'exact');
  const similarConflicts = conflicts.filter(c => c.conflictType === 'similar');
  const hasExactConflicts = exactConflicts.length > 0;

  // ==================== RESOLUTION HANDLERS ====================

  const handleResolve = useCallback(() => {
    switch (selectedResolution) {
      case 'use_anyway':
        onResolve({ action: 'use_anyway', sequence: proposedSequence });
        break;
      case 'modify_sequence':
        onResolve({ action: 'modify_sequence', sequence: modifiedSequence });
        break;
      case 'replace_existing':
        if (exactConflicts.length > 0) {
          onResolve({ 
            action: 'replace_existing', 
            sequenceId: exactConflicts[0].existingSequence.id,
            sequence: proposedSequence 
          });
        }
        break;
      case 'cancel':
        onResolve({ action: 'cancel' });
        break;
    }
  }, [selectedResolution, proposedSequence, modifiedSequence, exactConflicts, onResolve]);

  // ==================== SEQUENCE SUGGESTIONS ====================

  const generateAlternatives = useCallback((): string[] => {
    const alternatives: string[] = [];
    const base = proposedSequence.replace(/[TP]/i, '');
    
    // Add numbers
    for (let i = 1; i <= 5; i++) {
      alternatives.push(`T${base}${i}`);
      alternatives.push(`P${base}${i}`);
    }
    
    // Add letters
    const letters = ['a', 'b', 'x', 'z'];
    for (const letter of letters) {
      alternatives.push(`T${base}${letter}`);
      alternatives.push(`P${base}${letter}`);
    }
    
    return alternatives.slice(0, 6);
  }, [proposedSequence]);

  const alternatives = generateAlternatives();

  // ==================== RENDER ====================

  return (
    <div className="sequence-conflict-resolver">
      <div className="resolver-header">
        <h3>Keyboard Sequence Conflict</h3>
        <p>The sequence "{proposedSequence}" conflicts with existing sequences.</p>
      </div>

      {/* Conflict Details */}
      <div className="conflict-details">
        <h4>Conflicts Found:</h4>
        <div className="conflicts-list">
          {exactConflicts.map((conflict, index) => (
            <div key={index} className="conflict-item exact-conflict">
              <span className="conflict-icon">🚫</span>
              <div className="conflict-info">
                <div className="conflict-sequence">{conflict.existingSequence.sequence}</div>
                <div className="conflict-vault">Used by: {conflict.existingSequence.vaultName}</div>
                <div className="conflict-type">Exact match</div>
              </div>
            </div>
          ))}
          
          {similarConflicts.map((conflict, index) => (
            <div key={index} className="conflict-item similar-conflict">
              <span className="conflict-icon">⚠️</span>
              <div className="conflict-info">
                <div className="conflict-sequence">{conflict.existingSequence.sequence}</div>
                <div className="conflict-vault">Used by: {conflict.existingSequence.vaultName}</div>
                <div className="conflict-type">
                  {Math.round(conflict.similarity * 100)}% similar
                </div>
              </div>
            </div>
          ))}
        </div>
      </div>

      {/* Resolution Options */}
      <div className="resolution-options">
        <h4>Choose a resolution:</h4>
        
        {/* Option 1: Modify sequence */}
        <label className="resolution-option">
          <input
            type="radio"
            name="resolution"
            value="modify_sequence"
            checked={selectedResolution === 'modify_sequence'}
            onChange={(e) => setSelectedResolution(e.target.value as ConflictResolution['action'])}
          />
          <div className="option-content">
            <div className="option-title">Modify the sequence</div>
            <div className="option-description">Change the sequence to avoid conflicts</div>
            
            {selectedResolution === 'modify_sequence' && (
              <div className="option-details">
                <input
                  type="text"
                  value={modifiedSequence}
                  onChange={(e) => setModifiedSequence(e.target.value)}
                  placeholder="Enter modified sequence"
                  className="modified-sequence-input"
                />
                
                <div className="alternatives">
                  <span className="alternatives-label">Suggestions:</span>
                  <div className="alternatives-list">
                    {alternatives.map((alt, index) => (
                      <button
                        key={index}
                        onClick={() => setModifiedSequence(alt)}
                        className="alternative-button"
                      >
                        {alt}
                      </button>
                    ))}
                  </div>
                </div>
              </div>
            )}
          </div>
        </label>

        {/* Option 2: Use anyway (only for similar conflicts) */}
        {!hasExactConflicts && (
          <label className="resolution-option">
            <input
              type="radio"
              name="resolution"
              value="use_anyway"
              checked={selectedResolution === 'use_anyway'}
              onChange={(e) => setSelectedResolution(e.target.value as ConflictResolution['action'])}
            />
            <div className="option-content">
              <div className="option-title">Use anyway</div>
              <div className="option-description">
                Keep the sequence despite similarity warnings
              </div>
            </div>
          </label>
        )}

        {/* Option 3: Replace existing (only for exact conflicts) */}
        {hasExactConflicts && (
          <label className="resolution-option">
            <input
              type="radio"
              name="resolution"
              value="replace_existing"
              checked={selectedResolution === 'replace_existing'}
              onChange={(e) => setSelectedResolution(e.target.value as ConflictResolution['action'])}
            />
            <div className="option-content">
              <div className="option-title">Replace existing sequence</div>
              <div className="option-description">
                Remove the sequence from "{exactConflicts[0]?.existingSequence.vaultName}" and use it here
              </div>
              <div className="option-warning">
                ⚠️ This will disable the keyboard shortcut for the other vault
              </div>
            </div>
          </label>
        )}

        {/* Option 4: Cancel */}
        <label className="resolution-option">
          <input
            type="radio"
            name="resolution"
            value="cancel"
            checked={selectedResolution === 'cancel'}
            onChange={(e) => setSelectedResolution(e.target.value as ConflictResolution['action'])}
          />
          <div className="option-content">
            <div className="option-title">Cancel</div>
            <div className="option-description">Go back and choose a different sequence</div>
          </div>
        </label>
      </div>

      {/* Actions */}
      <div className="resolver-actions">
        <button
          onClick={onCancel}
          className="resolver-button resolver-button-secondary"
        >
          Cancel
        </button>
        
        <button
          onClick={handleResolve}
          className="resolver-button resolver-button-primary"
          disabled={selectedResolution === 'modify_sequence' && !modifiedSequence.trim()}
        >
          {selectedResolution === 'replace_existing' ? 'Replace & Continue' :
           selectedResolution === 'use_anyway' ? 'Use Anyway' :
           selectedResolution === 'modify_sequence' ? 'Use Modified' :
           'Cancel'}
        </button>
      </div>
    </div>
  );
};