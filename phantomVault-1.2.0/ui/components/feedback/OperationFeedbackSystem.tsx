/**
 * Operation Feedback System Component
 * 
 * System for providing comprehensive feedback on vault operations
 */

import React, { useState, useCallback, useEffect, useMemo } from 'react';
import { VaultOperationResult, OperationHistory, NotificationConfig } from '../../types';
import { useApp } from '../../contexts';
import { ToastNotification } from './ToastNotification';
import { OperationHistoryPanel } from './OperationHistoryPanel';
import { ErrorDetailsModal } from './ErrorDetailsModal';

export interface OperationFeedbackSystemProps {
  className?: string;
  maxHistoryItems?: number;
  autoRetryEnabled?: boolean;
  showHistoryPanel?: boolean;
}

interface FeedbackState {
  notifications: NotificationConfig[];
  history: OperationHistory[];
  showErrorDetails: boolean;
  selectedError: OperationHistory | null;
  retryQueue: OperationHistory[];
}

/**
 * Operation feedback system component
 */
export const OperationFeedbackSystem: React.FC<OperationFeedbackSystemProps> = ({
  className = '',
  maxHistoryItems = 100,
  autoRetryEnabled = true,
  showHistoryPanel = false,
}) => {
  const { state: appState, actions: appActions } = useApp();

  // ==================== STATE MANAGEMENT ====================

  const [feedbackState, setFeedbackState] = useState<FeedbackState>({
    notifications: [],
    history: [],
    showErrorDetails: false,
    selectedError: null,
    retryQueue: [],
  });

  // ==================== NOTIFICATION MANAGEMENT ====================

  const addNotification = useCallback((config: Omit<NotificationConfig, 'id' | 'timestamp'>) => {
    const notification: NotificationConfig = {
      ...config,
      id: `notification-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`,
      timestamp: new Date(),
    };

    setFeedbackState(prev => ({
      ...prev,
      notifications: [...prev.notifications, notification],
    }));

    // Auto-remove notification after duration
    if (config.duration && config.duration > 0) {
      setTimeout(() => {
        removeNotification(notification.id);
      }, config.duration);
    }

    return notification.id;
  }, []);

  const removeNotification = useCallback((id: string) => {
    setFeedbackState(prev => ({
      ...prev,
      notifications: prev.notifications.filter(n => n.id !== id),
    }));
  }, []);

  const clearAllNotifications = useCallback(() => {
    setFeedbackState(prev => ({
      ...prev,
      notifications: [],
    }));
  }, []);

  // ==================== OPERATION HISTORY ====================

  const addToHistory = useCallback((operation: Omit<OperationHistory, 'id' | 'timestamp'>) => {
    const historyItem: OperationHistory = {
      ...operation,
      id: `history-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`,
      timestamp: new Date(),
    };

    setFeedbackState(prev => ({
      ...prev,
      history: [historyItem, ...prev.history].slice(0, maxHistoryItems),
    }));

    return historyItem.id;
  }, [maxHistoryItems]);

  const clearHistory = useCallback(() => {
    setFeedbackState(prev => ({
      ...prev,
      history: [],
    }));
  }, []);

  const retryOperation = useCallback(async (historyItem: OperationHistory) => {
    if (!historyItem.retryable) {
      addNotification({
        type: 'warning',
        title: 'Cannot Retry',
        message: 'This operation cannot be retried.',
        duration: 3000,
      });
      return;
    }

    // Add to retry queue
    setFeedbackState(prev => ({
      ...prev,
      retryQueue: [...prev.retryQueue, historyItem],
    }));

    addNotification({
      type: 'info',
      title: 'Retrying Operation',
      message: `Retrying ${historyItem.operation} for "${historyItem.vaultName}"...`,
      duration: 3000,
    });

    // TODO: Implement actual retry logic
    // This would typically call the appropriate vault operation function
    
    // Simulate retry result
    setTimeout(() => {
      const success = Math.random() > 0.3; // 70% success rate for demo
      
      if (success) {
        addNotification({
          type: 'success',
          title: 'Retry Successful',
          message: `${historyItem.operation} completed successfully.`,
          duration: 5000,
        });
        
        addToHistory({
          operation: historyItem.operation,
          vaultId: historyItem.vaultId,
          vaultName: historyItem.vaultName,
          success: true,
          message: 'Operation completed successfully (retry)',
          retryable: false,
        });
      } else {
        addNotification({
          type: 'error',
          title: 'Retry Failed',
          message: `${historyItem.operation} failed again. Please check the error details.`,
          duration: 8000,
        });
        
        addToHistory({
          operation: historyItem.operation,
          vaultId: historyItem.vaultId,
          vaultName: historyItem.vaultName,
          success: false,
          error: 'Operation failed after retry',
          message: 'Retry attempt failed',
          retryable: true,
        });
      }
      
      // Remove from retry queue
      setFeedbackState(prev => ({
        ...prev,
        retryQueue: prev.retryQueue.filter(item => item.id !== historyItem.id),
      }));
    }, 2000);
  }, [addNotification, addToHistory]);

  // ==================== OPERATION RESULT PROCESSING ====================

  const processOperationResult = useCallback((
    operation: string,
    vaultId: string,
    vaultName: string,
    result: VaultOperationResult
  ) => {
    // Add to history
    const historyItem = addToHistory({
      operation,
      vaultId,
      vaultName,
      success: result.success,
      error: result.error,
      message: result.success ? 'Operation completed successfully' : result.error,
      retryable: !result.success && operation !== 'delete',
      details: result.details,
    });

    // Show notification
    if (result.success) {
      addNotification({
        type: 'success',
        title: `${operation.charAt(0).toUpperCase() + operation.slice(1)} Successful`,
        message: `"${vaultName}" ${operation} completed successfully.`,
        duration: 5000,
        actions: operation === 'mount' ? [
          {
            label: 'Open Folder',
            action: () => {
              // TODO: Implement open folder functionality
              console.log('Opening vault folder...');
            },
          },
        ] : undefined,
      });
    } else {
      addNotification({
        type: 'error',
        title: `${operation.charAt(0).toUpperCase() + operation.slice(1)} Failed`,
        message: result.error || 'An unknown error occurred.',
        duration: 8000,
        actions: [
          {
            label: 'View Details',
            action: () => {
              const historyItemFull = feedbackState.history.find(h => h.id === historyItem);
              if (historyItemFull) {
                setFeedbackState(prev => ({
                  ...prev,
                  showErrorDetails: true,
                  selectedError: historyItemFull,
                }));
              }
            },
          },
          {
            label: 'Retry',
            action: () => {
              const historyItemFull = feedbackState.history.find(h => h.id === historyItem);
              if (historyItemFull) {
                retryOperation(historyItemFull);
              }
            },
          },
        ],
      });
    }

    return historyItem;
  }, [addToHistory, addNotification, feedbackState.history, retryOperation]);

  // ==================== ERROR HANDLING ====================

  const showErrorDetails = useCallback((historyItem: OperationHistory) => {
    setFeedbackState(prev => ({
      ...prev,
      showErrorDetails: true,
      selectedError: historyItem,
    }));
  }, []);

  const hideErrorDetails = useCallback(() => {
    setFeedbackState(prev => ({
      ...prev,
      showErrorDetails: false,
      selectedError: null,
    }));
  }, []);

  // ==================== AUTO-RETRY LOGIC ====================

  useEffect(() => {
    if (!autoRetryEnabled) return;

    // Auto-retry failed operations after a delay
    const autoRetryTimer = setTimeout(() => {
      const failedOperations = feedbackState.history
        .filter(item => !item.success && item.retryable)
        .slice(0, 3); // Limit to 3 auto-retries

      failedOperations.forEach(operation => {
        // Only auto-retry if it hasn't been retried recently
        const recentRetry = feedbackState.history.find(
          item => item.vaultId === operation.vaultId && 
                  item.operation === operation.operation &&
                  item.timestamp > new Date(Date.now() - 60000) // Within last minute
        );

        if (!recentRetry) {
          retryOperation(operation);
        }
      });
    }, 10000); // Auto-retry after 10 seconds

    return () => clearTimeout(autoRetryTimer);
  }, [feedbackState.history, autoRetryEnabled, retryOperation]);

  // ==================== STATISTICS ====================

  const operationStats = useMemo(() => {
    const stats = {
      total: feedbackState.history.length,
      successful: 0,
      failed: 0,
      retried: 0,
      byOperation: {} as Record<string, { success: number; failed: number }>,
    };

    feedbackState.history.forEach(item => {
      if (item.success) {
        stats.successful++;
      } else {
        stats.failed++;
      }

      if (item.message?.includes('retry')) {
        stats.retried++;
      }

      if (!stats.byOperation[item.operation]) {
        stats.byOperation[item.operation] = { success: 0, failed: 0 };
      }

      if (item.success) {
        stats.byOperation[item.operation].success++;
      } else {
        stats.byOperation[item.operation].failed++;
      }
    });

    return stats;
  }, [feedbackState.history]);

  // ==================== RENDER HELPERS ====================

  const renderNotifications = () => {
    return feedbackState.notifications.map(notification => (
      <ToastNotification
        key={notification.id}
        notification={notification}
        onDismiss={() => removeNotification(notification.id)}
      />
    ));
  };

  // ==================== MAIN RENDER ====================

  return (
    <div className={`operation-feedback-system ${className}`}>
      {/* Toast Notifications Container */}
      <div className="notifications-container">
        {renderNotifications()}
      </div>

      {/* History Panel */}
      {showHistoryPanel && (
        <OperationHistoryPanel
          history={feedbackState.history}
          stats={operationStats}
          onRetry={retryOperation}
          onShowDetails={showErrorDetails}
          onClear={clearHistory}
        />
      )}

      {/* Error Details Modal */}
      {feedbackState.showErrorDetails && feedbackState.selectedError && (
        <ErrorDetailsModal
          error={feedbackState.selectedError}
          onClose={hideErrorDetails}
          onRetry={() => {
            if (feedbackState.selectedError) {
              retryOperation(feedbackState.selectedError);
              hideErrorDetails();
            }
          }}
        />
      )}

      {/* Global Actions */}
      <div className="feedback-actions">
        {feedbackState.notifications.length > 0 && (
          <button
            onClick={clearAllNotifications}
            className="clear-notifications-button"
            title="Clear all notifications"
          >
            Clear All
          </button>
        )}
      </div>
    </div>
  );
};

// ==================== HOOK FOR EASY INTEGRATION ====================

/**
 * Hook for easy integration with the operation feedback system
 */
export const useOperationFeedback = () => {
  const { actions: appActions } = useApp();

  const reportOperation = useCallback((
    operation: string,
    vaultId: string,
    vaultName: string,
    result: VaultOperationResult
  ) => {
    // This would typically be handled by the OperationFeedbackSystem component
    // For now, we'll use the app's notification system
    if (result.success) {
      appActions.addNotification({
        type: 'success',
        title: `${operation.charAt(0).toUpperCase() + operation.slice(1)} Successful`,
        message: `"${vaultName}" ${operation} completed successfully.`,
        duration: 5000,
      });
    } else {
      appActions.addNotification({
        type: 'error',
        title: `${operation.charAt(0).toUpperCase() + operation.slice(1)} Failed`,
        message: result.error || 'An unknown error occurred.',
        duration: 8000,
      });
    }
  }, [appActions]);

  return {
    reportOperation,
  };
};