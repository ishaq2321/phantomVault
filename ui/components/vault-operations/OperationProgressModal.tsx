/**
 * Operation Progress Modal Component
 * 
 * Modal for showing progress of vault operations
 */

import React, { useState, useEffect, useCallback } from 'react';
import { OperationProgress } from '../../types';

export interface OperationProgressModalProps {
  operation: 'mount' | 'unmount' | 'create' | 'delete';
  vaultName: string;
  progress: OperationProgress;
  onCancel?: () => void;
  canCancel?: boolean;
  showDetails?: boolean;
}

interface OperationStageInfo {
  icon: string;
  title: string;
  description: string;
}

/**
 * Operation progress modal component
 */
export const OperationProgressModal: React.FC<OperationProgressModalProps> = ({
  operation,
  vaultName,
  progress,
  onCancel,
  canCancel = false,
  showDetails = true,
}) => {
  const [showDetailedLog, setShowDetailedLog] = useState(false);
  const [elapsedTime, setElapsedTime] = useState(0);

  // ==================== EFFECTS ====================

  useEffect(() => {
    // Track elapsed time
    const startTime = Date.now();
    const timer = setInterval(() => {
      setElapsedTime(Date.now() - startTime);
    }, 100);

    return () => clearInterval(timer);
  }, []);

  useEffect(() => {
    // Handle escape key for cancellation
    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key === 'Escape' && canCancel && onCancel) {
        onCancel();
      }
    };

    document.addEventListener('keydown', handleKeyDown);
    return () => document.removeEventListener('keydown', handleKeyDown);
  }, [canCancel, onCancel]);

  // ==================== OPERATION CONFIGURATION ====================

  const getOperationConfig = (): { title: string; verb: string; icon: string } => {
    switch (operation) {
      case 'mount':
        return { title: 'Mounting Vault', verb: 'mounting', icon: '🔓' };
      case 'unmount':
        return { title: 'Unmounting Vault', verb: 'unmounting', icon: '🔒' };
      case 'create':
        return { title: 'Creating Vault', verb: 'creating', icon: '🆕' };
      case 'delete':
        return { title: 'Deleting Vault', verb: 'deleting', icon: '🗑️' };
      default:
        return { title: 'Processing', verb: 'processing', icon: '⚙️' };
    }
  };

  const getStageInfo = (stage: string): OperationStageInfo => {
    const stageMap: Record<string, OperationStageInfo> = {
      initializing: {
        icon: '🔄',
        title: 'Initializing',
        description: 'Preparing operation...',
      },
      validating: {
        icon: '🔍',
        title: 'Validating',
        description: 'Checking vault integrity...',
      },
      authenticating: {
        icon: '🔐',
        title: 'Authenticating',
        description: 'Verifying credentials...',
      },
      mounting: {
        icon: '📁',
        title: 'Mounting',
        description: 'Connecting vault to filesystem...',
      },
      unmounting: {
        icon: '📤',
        title: 'Unmounting',
        description: 'Disconnecting vault from filesystem...',
      },
      encrypting: {
        icon: '🔒',
        title: 'Encrypting',
        description: 'Securing vault data...',
      },
      decrypting: {
        icon: '🔓',
        title: 'Decrypting',
        description: 'Unlocking vault data...',
      },
      syncing: {
        icon: '🔄',
        title: 'Syncing',
        description: 'Synchronizing changes...',
      },
      finalizing: {
        icon: '✅',
        title: 'Finalizing',
        description: 'Completing operation...',
      },
      cleanup: {
        icon: '🧹',
        title: 'Cleanup',
        description: 'Cleaning up temporary files...',
      },
    };

    return stageMap[stage] || {
      icon: '⚙️',
      title: 'Processing',
      description: 'Working...',
    };
  };

  // ==================== HANDLERS ====================

  const handleCancel = useCallback(() => {
    if (canCancel && onCancel) {
      onCancel();
    }
  }, [canCancel, onCancel]);

  const toggleDetailedLog = useCallback(() => {
    setShowDetailedLog(prev => !prev);
  }, []);

  // ==================== RENDER HELPERS ====================

  const formatElapsedTime = (ms: number): string => {
    const seconds = Math.floor(ms / 1000);
    const minutes = Math.floor(seconds / 60);
    
    if (minutes > 0) {
      return `${minutes}m ${seconds % 60}s`;
    }
    return `${seconds}s`;
  };

  const getProgressColor = (percentage: number): string => {
    if (percentage < 30) return '#FF9800'; // Orange
    if (percentage < 70) return '#2196F3'; // Blue
    return '#4CAF50'; // Green
  };

  const operationConfig = getOperationConfig();
  const stageInfo = getStageInfo(progress.stage);

  // ==================== MAIN RENDER ====================

  return (
    <div className="operation-progress-modal-overlay">
      <div className="operation-progress-modal">
        {/* Header */}
        <div className="modal-header">
          <div className="header-content">
            <span className="operation-icon">{operationConfig.icon}</span>
            <div className="header-text">
              <h2 className="modal-title">{operationConfig.title}</h2>
              <p className="vault-name">"{vaultName}"</p>
            </div>
          </div>
          <div className="header-info">
            <span className="elapsed-time">{formatElapsedTime(elapsedTime)}</span>
          </div>
        </div>

        {/* Progress Content */}
        <div className="modal-content">
          {/* Current Stage */}
          <div className="current-stage">
            <div className="stage-header">
              <span className="stage-icon">{stageInfo.icon}</span>
              <div className="stage-info">
                <h3 className="stage-title">{stageInfo.title}</h3>
                <p className="stage-description">{stageInfo.description}</p>
              </div>
            </div>
          </div>

          {/* Progress Bar */}
          <div className="progress-section">
            <div className="progress-header">
              <span className="progress-label">Progress</span>
              <span className="progress-percentage">{Math.round(progress.percentage)}%</span>
            </div>
            <div className="progress-bar">
              <div 
                className="progress-fill"
                style={{ 
                  width: `${progress.percentage}%`,
                  backgroundColor: getProgressColor(progress.percentage)
                }}
              />
            </div>
          </div>

          {/* Current Message */}
          {progress.message && (
            <div className="progress-message">
              <span className="message-icon">💬</span>
              <span className="message-text">{progress.message}</span>
            </div>
          )}

          {/* Detailed Information */}
          {showDetails && (
            <div className="progress-details">
              <button
                onClick={toggleDetailedLog}
                className="details-toggle"
              >
                <span className="toggle-icon">{showDetailedLog ? '▼' : '▶'}</span>
                <span className="toggle-text">
                  {showDetailedLog ? 'Hide Details' : 'Show Details'}
                </span>
              </button>

              {showDetailedLog && (
                <div className="detailed-log">
                  <div className="log-header">
                    <span className="log-title">Operation Log</span>
                  </div>
                  <div className="log-content">
                    <div className="log-entry">
                      <span className="log-time">{formatElapsedTime(elapsedTime)}</span>
                      <span className="log-stage">[{stageInfo.title}]</span>
                      <span className="log-message">{progress.message || stageInfo.description}</span>
                    </div>
                    {/* Additional log entries would be added here */}
                  </div>
                </div>
              )}
            </div>
          )}

          {/* Operation Stats */}
          <div className="operation-stats">
            <div className="stat-item">
              <span className="stat-label">Stage:</span>
              <span className="stat-value">{stageInfo.title}</span>
            </div>
            <div className="stat-item">
              <span className="stat-label">Progress:</span>
              <span className="stat-value">{Math.round(progress.percentage)}%</span>
            </div>
            <div className="stat-item">
              <span className="stat-label">Elapsed:</span>
              <span className="stat-value">{formatElapsedTime(elapsedTime)}</span>
            </div>
          </div>
        </div>

        {/* Footer */}
        <div className="modal-footer">
          <div className="footer-info">
            <span className="info-text">
              {operationConfig.verb.charAt(0).toUpperCase() + operationConfig.verb.slice(1)} vault...
            </span>
          </div>
          {canCancel && (
            <button
              onClick={handleCancel}
              className="footer-button secondary"
            >
              <span className="button-icon">⏹️</span>
              Cancel
            </button>
          )}
        </div>

        {/* Animated Background */}
        <div className="progress-animation">
          <div className="animation-dots">
            <span className="dot dot-1">●</span>
            <span className="dot dot-2">●</span>
            <span className="dot dot-3">●</span>
          </div>
        </div>
      </div>
    </div>
  );
};