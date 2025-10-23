/**
 * Bulk Operation Modal Component
 * 
 * Modal for confirming and executing bulk vault operations
 */

import React, { useState, useCallback, useEffect } from 'react';
import { VaultInfo, BulkOperationProgress } from '../../types';

export interface BulkOperationModalProps {
  operation: 'mount' | 'unmount' | 'delete';
  vaults: VaultInfo[];
  progress?: BulkOperationProgress | null;
  isExecuting: boolean;
  onConfirm: (options?: { passwords?: Record<string, string>; force?: boolean }) => void;
  onCancel: () => void;
}

interface PasswordState {
  passwords: Record<string, string>;
  showPasswords: Record<string, boolean>;
  useCommonPassword: boolean;
  commonPassword: string;
}

/**
 * Bulk operation modal component
 */
export const BulkOperationModal: React.FC<BulkOperationModalProps> = ({
  operation,
  vaults,
  progress,
  isExecuting,
  onConfirm,
  onCancel,
}) => {
  const [passwordState, setPasswordState] = useState<PasswordState>({
    passwords: {},
    showPasswords: {},
    useCommonPassword: false,
    commonPassword: '',
  });
  const [forceOperation, setForceOperation] = useState(false);
  const [showAdvanced, setShowAdvanced] = useState(false);

  // ==================== EFFECTS ====================

  useEffect(() => {
    // Initialize password state for mount operations
    if (operation === 'mount') {
      const initialPasswords: Record<string, string> = {};
      vaults.forEach(vault => {
        initialPasswords[vault.id] = '';
      });
      setPasswordState(prev => ({
        ...prev,
        passwords: initialPasswords,
      }));
    }
  }, [operation, vaults]);

  // ==================== HANDLERS ====================

  const handlePasswordChange = useCallback((vaultId: string, password: string) => {
    setPasswordState(prev => ({
      ...prev,
      passwords: {
        ...prev.passwords,
        [vaultId]: password,
      },
    }));
  }, []);

  const handleCommonPasswordChange = useCallback((password: string) => {
    setPasswordState(prev => {
      const newPasswords = { ...prev.passwords };
      if (prev.useCommonPassword) {
        // Apply common password to all vaults
        vaults.forEach(vault => {
          newPasswords[vault.id] = password;
        });
      }
      return {
        ...prev,
        commonPassword: password,
        passwords: newPasswords,
      };
    });
  }, [vaults]);

  const handleUseCommonPasswordToggle = useCallback((useCommon: boolean) => {
    setPasswordState(prev => {
      const newPasswords = { ...prev.passwords };
      if (useCommon) {
        // Apply common password to all vaults
        vaults.forEach(vault => {
          newPasswords[vault.id] = prev.commonPassword;
        });
      }
      return {
        ...prev,
        useCommonPassword: useCommon,
        passwords: newPasswords,
      };
    });
  }, [vaults]);

  const togglePasswordVisibility = useCallback((vaultId: string) => {
    setPasswordState(prev => ({
      ...prev,
      showPasswords: {
        ...prev.showPasswords,
        [vaultId]: !prev.showPasswords[vaultId],
      },
    }));
  }, []);

  const handleConfirm = useCallback(() => {
    const options: { passwords?: Record<string, string>; force?: boolean } = {};

    if (operation === 'mount') {
      options.passwords = passwordState.passwords;
    }

    if (operation === 'unmount' && forceOperation) {
      options.force = true;
    }

    onConfirm(options);
  }, [operation, passwordState.passwords, forceOperation, onConfirm]);

  // ==================== VALIDATION ====================

  const canProceed = useCallback(() => {
    if (isExecuting) return false;

    if (operation === 'mount') {
      // Check if all vaults have passwords
      return vaults.every(vault => passwordState.passwords[vault.id]?.trim());
    }

    return true;
  }, [operation, vaults, passwordState.passwords, isExecuting]);

  // ==================== RENDER HELPERS ====================

  const getOperationConfig = () => {
    switch (operation) {
      case 'mount':
        return {
          title: 'Mount Vaults',
          icon: '🔓',
          description: 'Mount the selected vaults to make them accessible.',
          warningText: 'You will need to provide passwords for each vault.',
        };
      case 'unmount':
        return {
          title: 'Unmount Vaults',
          icon: '🔒',
          description: 'Unmount the selected vaults to secure them.',
          warningText: 'Any open files in these vaults will be closed.',
        };
      case 'delete':
        return {
          title: 'Delete Vaults',
          icon: '🗑️',
          description: 'Permanently delete the selected vaults.',
          warningText: 'This action cannot be undone. All vault data will be lost.',
        };
      default:
        return {
          title: 'Bulk Operation',
          icon: '⚙️',
          description: 'Perform operation on selected vaults.',
          warningText: 'Please review the operation details.',
        };
    }
  };

  const formatProgress = () => {
    if (!progress) return null;

    const percentage = Math.round((progress.completed / progress.total) * 100);
    return {
      percentage,
      text: `${progress.completed} of ${progress.total} completed`,
      current: progress.current?.name || 'Processing...',
    };
  };

  const operationConfig = getOperationConfig();
  const progressInfo = formatProgress();

  // ==================== RENDER SECTIONS ====================

  const renderVaultList = () => (
    <div className="vault-list-section">
      <h4 className="section-title">
        Selected Vaults ({vaults.length})
      </h4>
      <div className="vault-list">
        {vaults.map(vault => (
          <div key={vault.id} className="vault-item">
            <div className="vault-info">
              <span className="vault-name">{vault.name}</span>
              <span className="vault-path">{vault.path}</span>
            </div>
            <div className="vault-status">
              <span className={`status-badge ${vault.status}`}>
                {vault.status}
              </span>
            </div>
          </div>
        ))}
      </div>
    </div>
  );

  const renderPasswordSection = () => {
    if (operation !== 'mount') return null;

    return (
      <div className="password-section">
        <h4 className="section-title">Vault Passwords</h4>
        
        {/* Common Password Option */}
        <div className="common-password-option">
          <label className="checkbox-label">
            <input
              type="checkbox"
              checked={passwordState.useCommonPassword}
              onChange={(e) => handleUseCommonPasswordToggle(e.target.checked)}
              disabled={isExecuting}
            />
            <span className="checkbox-text">Use same password for all vaults</span>
          </label>
        </div>

        {passwordState.useCommonPassword ? (
          /* Common Password Input */
          <div className="password-input-group">
            <label className="password-label">Common Password</label>
            <div className="password-input-container">
              <input
                type="password"
                value={passwordState.commonPassword}
                onChange={(e) => handleCommonPasswordChange(e.target.value)}
                placeholder="Enter password for all vaults"
                className="password-input"
                disabled={isExecuting}
              />
            </div>
          </div>
        ) : (
          /* Individual Password Inputs */
          <div className="individual-passwords">
            {vaults.map(vault => (
              <div key={vault.id} className="password-input-group">
                <label className="password-label">{vault.name}</label>
                <div className="password-input-container">
                  <input
                    type={passwordState.showPasswords[vault.id] ? 'text' : 'password'}
                    value={passwordState.passwords[vault.id] || ''}
                    onChange={(e) => handlePasswordChange(vault.id, e.target.value)}
                    placeholder="Enter vault password"
                    className="password-input"
                    disabled={isExecuting}
                  />
                  <button
                    type="button"
                    onClick={() => togglePasswordVisibility(vault.id)}
                    className="password-toggle"
                    disabled={isExecuting}
                  >
                    {passwordState.showPasswords[vault.id] ? '👁️' : '👁️‍🗨️'}
                  </button>
                </div>
              </div>
            ))}
          </div>
        )}
      </div>
    );
  };

  const renderAdvancedOptions = () => (
    <div className="advanced-options">
      <button
        onClick={() => setShowAdvanced(!showAdvanced)}
        className="advanced-toggle"
        disabled={isExecuting}
      >
        <span className="toggle-icon">{showAdvanced ? '▼' : '▶'}</span>
        Advanced Options
      </button>

      {showAdvanced && (
        <div className="advanced-content">
          {operation === 'unmount' && (
            <label className="checkbox-label">
              <input
                type="checkbox"
                checked={forceOperation}
                onChange={(e) => setForceOperation(e.target.checked)}
                disabled={isExecuting}
              />
              <span className="checkbox-text">
                Force unmount (may cause data loss)
              </span>
            </label>
          )}

          {operation === 'delete' && (
            <div className="delete-warning">
              <span className="warning-icon">⚠️</span>
              <span className="warning-text">
                Deletion is permanent and cannot be undone
              </span>
            </div>
          )}
        </div>
      )}
    </div>
  );

  const renderProgress = () => {
    if (!isExecuting || !progressInfo) return null;

    return (
      <div className="progress-section">
        <div className="progress-header">
          <h4 className="progress-title">Operation in Progress</h4>
          <span className="progress-percentage">{progressInfo.percentage}%</span>
        </div>
        
        <div className="progress-bar">
          <div 
            className="progress-fill"
            style={{ width: `${progressInfo.percentage}%` }}
          />
        </div>
        
        <div className="progress-details">
          <span className="progress-text">{progressInfo.text}</span>
          <span className="current-vault">Current: {progressInfo.current}</span>
        </div>
      </div>
    );
  };

  // ==================== MAIN RENDER ====================

  return (
    <div className="bulk-operation-modal-overlay">
      <div className="bulk-operation-modal">
        {/* Header */}
        <div className="modal-header">
          <div className="header-content">
            <span className="operation-icon">{operationConfig.icon}</span>
            <div className="header-text">
              <h2 className="modal-title">{operationConfig.title}</h2>
              <p className="modal-description">{operationConfig.description}</p>
            </div>
          </div>
          {!isExecuting && (
            <button onClick={onCancel} className="modal-close">✕</button>
          )}
        </div>

        {/* Content */}
        <div className="modal-content">
          {/* Warning */}
          <div className={`operation-warning ${operation}`}>
            <span className="warning-icon">⚠️</span>
            <span className="warning-text">{operationConfig.warningText}</span>
          </div>

          {/* Progress (when executing) */}
          {renderProgress()}

          {/* Vault List */}
          {!isExecuting && renderVaultList()}

          {/* Password Section */}
          {!isExecuting && renderPasswordSection()}

          {/* Advanced Options */}
          {!isExecuting && renderAdvancedOptions()}
        </div>

        {/* Footer */}
        <div className="modal-footer">
          <button
            onClick={onCancel}
            className="footer-button secondary"
            disabled={isExecuting}
          >
            {isExecuting ? 'Running...' : 'Cancel'}
          </button>
          
          <button
            onClick={handleConfirm}
            className={`footer-button primary ${operation}`}
            disabled={!canProceed()}
          >
            {isExecuting ? (
              <>
                <span className="loading-spinner">⏳</span>
                {operationConfig.title}...
              </>
            ) : (
              <>
                <span className="button-icon">{operationConfig.icon}</span>
                {operationConfig.title}
              </>
            )}
          </button>
        </div>
      </div>
    </div>
  );
};