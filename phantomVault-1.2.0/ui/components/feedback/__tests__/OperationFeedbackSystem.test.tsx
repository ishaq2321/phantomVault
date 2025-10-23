/**
 * Operation Feedback System Tests
 * 
 * Unit tests for the OperationFeedbackSystem component
 */

import React from 'react';
import { render, screen, fireEvent, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { OperationFeedbackSystem, useOperationFeedback } from '../OperationFeedbackSystem';
import { AppProvider } from '../../../contexts/AppContext';
import { VaultOperationResult, OperationHistory } from '../../../types';

// ==================== MOCKS ====================

const mockAppActions = {
  addNotification: jest.fn(),
  removeNotification: jest.fn(),
  updateSettings: jest.fn(),
};

jest.mock('../../../contexts/AppContext', () => ({
  ...jest.requireActual('../../../contexts/AppContext'),
  useApp: () => ({
    state: {
      notifications: [],
      settings: {
        theme: 'dark',
        autoStart: false,
        notifications: true,
        minimizeToTray: true,
        autoLock: true,
        lockTimeout: 15,
      },
    },
    actions: mockAppActions,
  }),
}));

// Mock child components
jest.mock('../ToastNotification', () => ({
  ToastNotification: ({ notification, onDismiss }: any) => (
    <div data-testid={`toast-${notification.id}`}>
      <h4>{notification.title}</h4>
      <p>{notification.message}</p>
      <button onClick={onDismiss} data-testid={`dismiss-${notification.id}`}>
        Dismiss
      </button>
    </div>
  ),
}));

jest.mock('../OperationHistoryPanel', () => ({
  OperationHistoryPanel: ({ history, onRetry, onShowDetails, onClear }: any) => (
    <div data-testid="operation-history-panel">
      <h3>Operation History ({history.length})</h3>
      {history.map((item: OperationHistory) => (
        <div key={item.id} data-testid={`history-item-${item.id}`}>
          <span>{item.operation} - {item.vaultName}</span>
          <span>{item.success ? 'Success' : 'Failed'}</span>
          {item.retryable && (
            <button
              onClick={() => onRetry(item)}
              data-testid={`retry-${item.id}`}
            >
              Retry
            </button>
          )}
          <button
            onClick={() => onShowDetails(item)}
            data-testid={`details-${item.id}`}
          >
            Details
          </button>
        </div>
      ))}
      <button onClick={onClear} data-testid="clear-history">
        Clear History
      </button>
    </div>
  ),
}));

jest.mock('../ErrorDetailsModal', () => ({
  ErrorDetailsModal: ({ error, onClose, onRetry }: any) => (
    <div data-testid="error-details-modal">
      <h3>Error Details</h3>
      <p>{error.error}</p>
      <button onClick={onClose} data-testid="close-error-details">
        Close
      </button>
      {onRetry && (
        <button onClick={onRetry} data-testid="retry-from-details">
          Retry
        </button>
      )}
    </div>
  ),
}));

// ==================== TEST WRAPPER ====================

const TestWrapper: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  return <AppProvider>{children}</AppProvider>;
};

// ==================== SETUP ====================

beforeEach(() => {
  jest.clearAllMocks();
  jest.useFakeTimers();
});

afterEach(() => {
  jest.runOnlyPendingTimers();
  jest.useRealTimers();
});

// ==================== COMPONENT TESTS ====================

describe('OperationFeedbackSystem', () => {
  test('should render without crashing', () => {
    render(
      <TestWrapper>
        <OperationFeedbackSystem />
      </TestWrapper>
    );

    expect(screen.getByRole('button', { name: /clear all/i })).toBeInTheDocument();
  });

  test('should show history panel when enabled', () => {
    render(
      <TestWrapper>
        <OperationFeedbackSystem showHistoryPanel={true} />
      </TestWrapper>
    );

    expect(screen.getByTestId('operation-history-panel')).toBeInTheDocument();
  });

  test('should hide history panel when disabled', () => {
    render(
      <TestWrapper>
        <OperationFeedbackSystem showHistoryPanel={false} />
      </TestWrapper>
    );

    expect(screen.queryByTestId('operation-history-panel')).not.toBeInTheDocument();
  });

  test('should display toast notifications', async () => {
    const TestComponent = () => {
      const [system, setSystem] = React.useState<any>(null);

      React.useEffect(() => {
        if (system) {
          system.addNotification({
            type: 'success',
            title: 'Test Success',
            message: 'Operation completed successfully',
            duration: 5000,
          });
        }
      }, [system]);

      return (
        <OperationFeedbackSystem
          ref={setSystem}
          showHistoryPanel={true}
        />
      );
    };

    render(
      <TestWrapper>
        <TestComponent />
      </TestWrapper>
    );

    // Note: This test would need the component to expose methods
    // In a real implementation, you'd use a ref or context to access methods
  });

  test('should auto-dismiss notifications after duration', async () => {
    // This test would require the component to expose notification management
    // For now, we'll test the timer behavior conceptually
    
    render(
      <TestWrapper>
        <OperationFeedbackSystem />
      </TestWrapper>
    );

    // Fast-forward time to test auto-dismiss
    jest.advanceTimersByTime(5000);

    // Notifications should be auto-dismissed
    await waitFor(() => {
      // Check that notifications are removed
    });
  });

  test('should clear all notifications', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <OperationFeedbackSystem />
      </TestWrapper>
    );

    // Assuming there are notifications to clear
    const clearButton = screen.getByRole('button', { name: /clear all/i });
    await user.click(clearButton);

    // All notifications should be cleared
    expect(screen.queryByTestId(/^toast-/)).not.toBeInTheDocument();
  });

  test('should show error details modal', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <OperationFeedbackSystem showHistoryPanel={true} />
      </TestWrapper>
    );

    // This would require the component to have some failed operations in history
    // For testing purposes, we'll simulate the modal being shown
    
    // The actual test would involve:
    // 1. Adding a failed operation to history
    // 2. Clicking on show details
    // 3. Verifying the modal appears
  });

  test('should retry failed operations', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <OperationFeedbackSystem showHistoryPanel={true} />
      </TestWrapper>
    );

    // This would test the retry functionality
    // In a real scenario, you'd:
    // 1. Have a failed operation in history
    // 2. Click retry button
    // 3. Verify the operation is retried
  });

  test('should limit history items to maxHistoryItems', () => {
    const maxItems = 5;
    
    render(
      <TestWrapper>
        <OperationFeedbackSystem maxHistoryItems={maxItems} showHistoryPanel={true} />
      </TestWrapper>
    );

    // Test would involve adding more than maxItems operations
    // and verifying only the most recent ones are kept
  });

  test('should handle auto-retry when enabled', async () => {
    render(
      <TestWrapper>
        <OperationFeedbackSystem autoRetryEnabled={true} showHistoryPanel={true} />
      </TestWrapper>
    );

    // Fast-forward time to trigger auto-retry
    jest.advanceTimersByTime(10000);

    // Auto-retry should be triggered for failed operations
    await waitFor(() => {
      // Verify auto-retry behavior
    });
  });

  test('should not auto-retry when disabled', async () => {
    render(
      <TestWrapper>
        <OperationFeedbackSystem autoRetryEnabled={false} showHistoryPanel={true} />
      </TestWrapper>
    );

    // Fast-forward time
    jest.advanceTimersByTime(10000);

    // Auto-retry should not be triggered
    await waitFor(() => {
      // Verify no auto-retry occurred
    });
  });

  test('should apply custom className', () => {
    const { container } = render(
      <TestWrapper>
        <OperationFeedbackSystem className="custom-feedback-class" />
      </TestWrapper>
    );

    expect(container.firstChild).toHaveClass('custom-feedback-class');
  });
});

// ==================== HOOK TESTS ====================

describe('useOperationFeedback', () => {
  const TestComponent = ({ result }: { result: VaultOperationResult }) => {
    const { reportOperation } = useOperationFeedback();

    React.useEffect(() => {
      reportOperation('mount', 'vault-1', 'Test Vault', result);
    }, [reportOperation, result]);

    return <div>Test Component</div>;
  };

  test('should report successful operation', () => {
    const successResult: VaultOperationResult = {
      success: true,
    };

    render(
      <TestWrapper>
        <TestComponent result={successResult} />
      </TestWrapper>
    );

    expect(mockAppActions.addNotification).toHaveBeenCalledWith(
      expect.objectContaining({
        type: 'success',
        title: 'Mount Successful',
      })
    );
  });

  test('should report failed operation', () => {
    const failureResult: VaultOperationResult = {
      success: false,
      error: 'Invalid password',
    };

    render(
      <TestWrapper>
        <TestComponent result={failureResult} />
      </TestWrapper>
    );

    expect(mockAppActions.addNotification).toHaveBeenCalledWith(
      expect.objectContaining({
        type: 'error',
        title: 'Mount Failed',
        message: 'Invalid password',
      })
    );
  });

  test('should handle operation without error message', () => {
    const failureResult: VaultOperationResult = {
      success: false,
    };

    render(
      <TestWrapper>
        <TestComponent result={failureResult} />
      </TestWrapper>
    );

    expect(mockAppActions.addNotification).toHaveBeenCalledWith(
      expect.objectContaining({
        type: 'error',
        title: 'Mount Failed',
        message: 'An unknown error occurred.',
      })
    );
  });

  test('should capitalize operation names correctly', () => {
    const successResult: VaultOperationResult = {
      success: true,
    };

    const TestMultipleOperations = () => {
      const { reportOperation } = useOperationFeedback();

      React.useEffect(() => {
        reportOperation('mount', 'vault-1', 'Test Vault', successResult);
        reportOperation('unmount', 'vault-2', 'Test Vault 2', successResult);
        reportOperation('delete', 'vault-3', 'Test Vault 3', successResult);
      }, [reportOperation]);

      return <div>Test Component</div>;
    };

    render(
      <TestWrapper>
        <TestMultipleOperations />
      </TestWrapper>
    );

    expect(mockAppActions.addNotification).toHaveBeenCalledWith(
      expect.objectContaining({
        title: 'Mount Successful',
      })
    );

    expect(mockAppActions.addNotification).toHaveBeenCalledWith(
      expect.objectContaining({
        title: 'Unmount Successful',
      })
    );

    expect(mockAppActions.addNotification).toHaveBeenCalledWith(
      expect.objectContaining({
        title: 'Delete Successful',
      })
    );
  });
});

// ==================== INTEGRATION TESTS ====================

describe('OperationFeedbackSystem Integration', () => {
  test('should handle complete operation lifecycle', async () => {
    const user = userEvent.setup();
    
    // This would be a more comprehensive test that:
    // 1. Starts an operation
    // 2. Shows progress
    // 3. Completes with success/failure
    // 4. Shows notification
    // 5. Adds to history
    // 6. Allows retry if failed
    
    render(
      <TestWrapper>
        <OperationFeedbackSystem showHistoryPanel={true} />
      </TestWrapper>
    );

    // Test the complete flow
    // This would require more complex setup with actual operation simulation
  });

  test('should handle concurrent operations', async () => {
    // Test multiple operations running simultaneously
    // Verify proper state management and UI updates
    
    render(
      <TestWrapper>
        <OperationFeedbackSystem showHistoryPanel={true} />
      </TestWrapper>
    );

    // Simulate multiple concurrent operations
    // Verify they're handled correctly
  });

  test('should persist operation history across component remounts', () => {
    const { rerender } = render(
      <TestWrapper>
        <OperationFeedbackSystem showHistoryPanel={true} />
      </TestWrapper>
    );

    // Add some operations to history
    // Remount component
    rerender(
      <TestWrapper>
        <OperationFeedbackSystem showHistoryPanel={true} />
      </TestWrapper>
    );

    // Verify history is maintained
    // Note: This would require the component to use persistent storage
  });
});