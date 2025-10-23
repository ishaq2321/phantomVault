/**
 * Vault Operation Controls Component
 * 
 * Component for vault mount/unmount operations with password prompts and progress indicators
 */

import React, { useState, useCallback, useEffect } from 'react';
import { VaultInfo, VaultOperationResult, OperationProgress } from '../../types';
import { useVault, useApp } from '../../contexts';
import { useVaultOperations } from '../../hooks';
import { PasswordPromptModal } from './PasswordPromptModal';
import { OperationProgressModal } from './OperationProgressModal';
import { ConfirmationModal } from '../common/ConfirmationModal';

export interface VaultOperationControlsProps {
  vault: VaultInfo;
  showLabels?: boolean;
  size?: 'small' | 'medium' | 'large';
  orientation?: 'horizontal' | 'vertical';
  className?: string;
}

interface OperationState {
  type: 'mount' | 'unmount' | null;
  isActive: boolean;
  progress: OperationProgress | null;
  error: string | null;
}

/**
 * Vault operation controls component
 */
export const VaultOperationControls: React.FC<VaultOperationControlsProps> = ({
  vault,
  showLabels = true,
  size = 'medium',
  orientation = 'horizontal',
  className = '',
}) => {
  const { actions: appActions } = useApp();
  const vaultOps = useVaultOperations();

  // ==================== STATE MANAGEMENT ====================

  const [operation, setOperation] = useState<OperationState>({
    type: null,
    isActive: false,
    progress: null,
    error: null,
  });

  const [showPasswordPrompt, setShowPasswordPrompt] = useState(false);
  const [showProgressModal, setShowProgressModal] = useState(false);
  const [showConfirmation, setShowConfirmation] = useState(false);
  const [confirmationConfig, setConfirmationConfig] = useState<{
    title: string;
    message: string;
    action: () => void;
  } | null>(null);

  // ==================== OPERATION HANDLERS ====================

  const handleMountVault = useCallback(async (password?: string) => {
    try {
      setOperation({
        type: 'mount',
        isActive: true,
        progress: { stage: 'initializing', percentage: 0, message: 'Preparing to mount vault...' },
        error: null,
      });

      setShowProgressModal(true);

      // Start mount operation
      const result = await vaultOps.mountVault(vault.id, password, {
        onProgress: (progress) => {
          setOperation(prev => ({ ...prev, progress }));
        },
      });

      if (result.success) {
        appActions.addNotification({
          type: 'success',
          title: 'Vault Mounted',
          message: `"${vault.name}" has been mounted successfully.`,
          duration: 5000,
        });

        setOperation({
          type: null,
          isActive: false,
          progress: null,
          error: null,
        });
      } else {
        throw new Error(result.error || 'Failed to mount vault');
      }
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Unknown error occurred';
      
      setOperation(prev => ({
        ...prev,
        isActive: false,
        error: errorMessage,
      }));

      appActions.addNotification({
        type: 'error',
        title: 'Mount Failed',
        message: errorMessage,
        duration: 8000,
      });
    } finally {
      setShowProgressModal(false);
      setShowPasswordPrompt(false);
    }
  }, [vault.id, vault.name, vaultOps, appActions]);

  const handleUnmountVault = useCallback(async (force: boolean = false) => {
    try {
      setOperation({
        type: 'unmount',
        isActive: true,
        progress: { stage: 'initializing', percentage: 0, message: 'Preparing to unmount vault...' },
        error: null,
      });

      setShowProgressModal(true);

      // Start unmount operation
      const result = await vaultOps.unmountVault(vault.id, {
        force,
        onProgress: (progress) => {
          setOperation(prev => ({ ...prev, progress }));
        },
      });

      if (result.success) {
        appActions.addNotification({
          type: 'success',
          title: 'Vault Unmounted',
          message: `"${vault.name}" has been unmounted successfully.`,
          duration: 5000,
        });

        setOperation({
          type: null,
          isActive: false,
          progress: null,
          error: null,
        });
      } else {
        throw new Error(result.error || 'Failed to unmount vault');
      }
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Unknown error occurred';
      
      setOperation(prev => ({
        ...prev,
        isActive: false,
        error: errorMessage,
      }));

      appActions.addNotification({
        type: 'error',
        title: 'Unmount Failed',
        message: errorMessage,
        duration: 8000,
      });
    } finally {
      setShowProgressModal(false);
      setShowConfirmation(false);
    }
  }, [vault.id, vault.name, vaultOps, appActions]);

  // ==================== USER INTERACTION HANDLERS ====================

  const initiateMount = useCallback(() => {
    if (vault.status === 'mounted') {
      appActions.addNotification({
        type: 'warning',
        title: 'Already Mounted',
        message: `"${vault.name}" is already mounted.`,
        duration: 3000,
      });
      return;
    }

    if (vault.status === 'error') {
      setConfirmationConfig({
        title: 'Mount Vault with Errors',
        message: `"${vault.name}" has errors. Mounting may not work properly. Continue anyway?`,
        action: () => setShowPasswordPrompt(true),
      });
      setShowConfirmation(true);
    } else {
      setShowPasswordPrompt(true);
    }
  }, [vault.status, vault.name, appActions]);

  const initiateUnmount = useCallback(() => {
    if (vault.status !== 'mounted') {
      appActions.addNotification({
        type: 'warning',
        title: 'Not Mounted',
        message: `"${vault.name}" is not currently mounted.`,
        duration: 3000,
      });
      return;
    }

    setConfirmationConfig({
      title: 'Unmount Vault',
      message: `Are you sure you want to unmount "${vault.name}"? Any open files will be closed.`,
      action: () => handleUnmountVault(false),
    });
    setShowConfirmation(true);
  }, [vault.status, vault.name, appActions, handleUnmountVault]);

  const initiateForceUnmount = useCallback(() => {
    setConfirmationConfig({
      title: 'Force Unmount Vault',
      message: `Force unmount "${vault.name}"? This may cause data loss if files are being written.`,
      action: () => handleUnmountVault(true),
    });
    setShowConfirmation(true);
  }, [vault.name, handleUnmountVault]);

  const handlePasswordSubmit = useCallback((password: string) => {
    handleMountVault(password);
  }, [handleMountVault]);

  const handlePasswordCancel = useCallback(() => {
    setShowPasswordPrompt(false);
  }, []);

  const handleConfirmationCancel = useCallback(() => {
    setShowConfirmation(false);
    setConfirmationConfig(null);
  }, []);

  const handleProgressCancel = useCallback(() => {
    // TODO: Implement operation cancellation
    setShowProgressModal(false);
    setOperation({
      type: null,
      isActive: false,
      progress: null,
      error: null,
    });
  }, []);

  // ==================== RENDER HELPERS ====================

  const getButtonSize = () => {
    switch (size) {
      case 'small': return 'btn-sm';
      case 'large': return 'btn-lg';
      default: return 'btn-md';
    }
  };

  const getIconSize = () => {
    switch (size) {
      case 'small': return '0.875rem';
      case 'large': return '1.25rem';
      default: return '1rem';
    }
  };

  const canMount = vault.status === 'unmounted' || vault.status === 'error';
  const canUnmount = vault.status === 'mounted';
  const isOperating = operation.isActive;

  const renderMountButton = () => (
    <button
      onClick={initiateMount}
      disabled={!canMount || isOperating}
      className={`operation-button mount-button ${getButtonSize()} ${!canMount ? 'disabled' : ''}`}
      title={canMount ? 'Mount vault' : 'Vault is already mounted'}
    >
      <span className="button-icon" style={{ fontSize: getIconSize() }}>
        {isOperating && operation.type === 'mount' ? '⏳' : '🔓'}
      </span>
      {showLabels && (
        <span className="button-label">
          {isOperating && operation.type === 'mount' ? 'Mounting...' : 'Mount'}
        </span>
      )}
    </button>
  );

  const renderUnmountButton = () => (
    <button
      onClick={initiateUnmount}
      disabled={!canUnmount || isOperating}
      className={`operation-button unmount-button ${getButtonSize()} ${!canUnmount ? 'disabled' : ''}`}
      title={canUnmount ? 'Unmount vault' : 'Vault is not mounted'}
    >
      <span className="button-icon" style={{ fontSize: getIconSize() }}>
        {isOperating && operation.type === 'unmount' ? '⏳' : '🔒'}
      </span>
      {showLabels && (
        <span className="button-label">
          {isOperating && operation.type === 'unmount' ? 'Unmounting...' : 'Unmount'}
        </span>
      )}
    </button>
  );

  const renderForceUnmountButton = () => {
    if (vault.status !== 'mounted' || size === 'small') return null;

    return (
      <button
        onClick={initiateForceUnmount}
        disabled={isOperating}
        className={`operation-button force-unmount-button ${getButtonSize()}`}
        title="Force unmount vault (may cause data loss)"
      >
        <span className="button-icon" style={{ fontSize: getIconSize() }}>⚡</span>
        {showLabels && <span className="button-label">Force</span>}
      </button>
    );
  };

  // ==================== MAIN RENDER ====================

  return (
    <div className={`vault-operation-controls ${orientation} ${size} ${className}`}>
      {/* Operation Buttons */}
      <div className="operation-buttons">
        {renderMountButton()}
        {renderUnmountButton()}
        {renderForceUnmountButton()}
      </div>

      {/* Status Indicator */}
      {isOperating && (
        <div className="operation-status">
          <div className="status-indicator">
            <span className="status-icon">⏳</span>
            <span className="status-text">
              {operation.type === 'mount' ? 'Mounting' : 'Unmounting'} vault...
            </span>
          </div>
          {operation.progress && (
            <div className="progress-bar">
              <div 
                className="progress-fill"
                style={{ width: `${operation.progress.percentage}%` }}
              />
            </div>
          )}
        </div>
      )}

      {/* Error Display */}
      {operation.error && (
        <div className="operation-error">
          <span className="error-icon">⚠️</span>
          <span className="error-message">{operation.error}</span>
          <button
            onClick={() => setOperation(prev => ({ ...prev, error: null }))}
            className="error-dismiss"
            title="Dismiss error"
          >
            ✕
          </button>
        </div>
      )}

      {/* Modals */}
      {showPasswordPrompt && (
        <PasswordPromptModal
          vaultName={vault.name}
          onSubmit={handlePasswordSubmit}
          onCancel={handlePasswordCancel}
          isLoading={isOperating}
        />
      )}

      {showProgressModal && operation.progress && (
        <OperationProgressModal
          operation={operation.type!}
          vaultName={vault.name}
          progress={operation.progress}
          onCancel={handleProgressCancel}
          canCancel={false} // TODO: Implement cancellation
        />
      )}

      {showConfirmation && confirmationConfig && (
        <ConfirmationModal
          title={confirmationConfig.title}
          message={confirmationConfig.message}
          confirmText="Continue"
          cancelText="Cancel"
          onConfirm={confirmationConfig.action}
          onCancel={handleConfirmationCancel}
          type="warning"
        />
      )}
    </div>
  );
};