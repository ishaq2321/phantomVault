/**
 * Vault Operation Controls Tests
 * 
 * Unit tests for the VaultOperationControls component
 */

import React from 'react';
import { render, screen, fireEvent, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { vi } from 'vitest';
import { VaultOperationControls } from '../VaultOperationControls';
import { VaultProvider } from '../../../contexts/VaultContext';
import { AppProvider } from '../../../contexts/AppContext';
import { VaultInfo } from '../../../types';

// ==================== MOCKS ====================

const mockVaultOperations = {
  mountVault: vi.fn(),
  unmountVault: vi.fn(),
  createVault: vi.fn(),
  deleteVault: vi.fn(),
  updateVault: vi.fn(),
};

const mockAppActions = {
  addNotification: vi.fn(),
  removeNotification: vi.fn(),
  updateSettings: vi.fn(),
};

// Mock the hooks
jest.mock('../../../hooks/useVaultOperations', () => ({
  useVaultOperations: () => mockVaultOperations,
}));

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

// Mock modal components
jest.mock('../PasswordPromptModal', () => ({
  PasswordPromptModal: ({ vaultName, onSubmit, onCancel }: any) => (
    <div data-testid="password-prompt-modal">
      <h3>Enter Password for {vaultName}</h3>
      <button
        onClick={() => onSubmit('test-password')}
        data-testid="submit-password"
      >
        Submit
      </button>
      <button onClick={onCancel} data-testid="cancel-password">
        Cancel
      </button>
    </div>
  ),
}));

jest.mock('../OperationProgressModal', () => ({
  OperationProgressModal: ({ operation, vaultName, progress, onCancel }: any) => (
    <div data-testid="progress-modal">
      <h3>{operation} Progress for {vaultName}</h3>
      <div data-testid="progress-percentage">{progress?.percentage || 0}%</div>
      <button onClick={onCancel} data-testid="cancel-operation">
        Cancel
      </button>
    </div>
  ),
}));

jest.mock('../../common/ConfirmationModal', () => ({
  ConfirmationModal: ({ title, message, onConfirm, onCancel }: any) => (
    <div data-testid="confirmation-modal">
      <h3>{title}</h3>
      <p>{message}</p>
      <button onClick={onConfirm} data-testid="confirm-action">
        Confirm
      </button>
      <button onClick={onCancel} data-testid="cancel-action">
        Cancel
      </button>
    </div>
  ),
}));

// ==================== TEST DATA ====================

const createMockVault = (overrides: Partial<VaultInfo> = {}): VaultInfo => ({
  id: 'test-vault-id',
  name: 'Test Vault',
  path: '/test/path',
  status: 'unmounted',
  lastAccessed: new Date('2024-01-01'),
  size: 1024 * 1024 * 100,
  folderCount: 25,
  ...overrides,
});

// ==================== TEST WRAPPER ====================

const TestWrapper: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const mockVaultState = {
    vaults: [],
    isLoading: false,
    error: null,
  };

  const mockVaultActions = {
    loadVaults: vi.fn(),
    addVault: vi.fn(),
    updateVault: vi.fn(),
    removeVault: vi.fn(),
    setLoading: vi.fn(),
    setError: vi.fn(),
  };

  return (
    <AppProvider>
      <VaultProvider value={{ state: mockVaultState, actions: mockVaultActions }}>
        {children}
      </VaultProvider>
    </AppProvider>
  );
};

// ==================== SETUP ====================

beforeEach(() => {
  vi.clearAllMocks();
});

// ==================== COMPONENT TESTS ====================

describe('VaultOperationControls', () => {
  test('should render mount and unmount buttons', () => {
    const vault = createMockVault();
    
    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} />
      </TestWrapper>
    );

    expect(screen.getByRole('button', { name: /mount/i })).toBeInTheDocument();
    expect(screen.getByRole('button', { name: /unmount/i })).toBeInTheDocument();
  });

  test('should enable mount button for unmounted vault', () => {
    const vault = createMockVault({ status: 'unmounted' });
    
    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} />
      </TestWrapper>
    );

    const mountButton = screen.getByRole('button', { name: /mount/i });
    const unmountButton = screen.getByRole('button', { name: /unmount/i });

    expect(mountButton).not.toBeDisabled();
    expect(unmountButton).toBeDisabled();
  });

  test('should enable unmount button for mounted vault', () => {
    const vault = createMockVault({ status: 'mounted' });
    
    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} />
      </TestWrapper>
    );

    const mountButton = screen.getByRole('button', { name: /mount/i });
    const unmountButton = screen.getByRole('button', { name: /unmount/i });

    expect(mountButton).toBeDisabled();
    expect(unmountButton).not.toBeDisabled();
  });

  test('should show force unmount button for mounted vault', () => {
    const vault = createMockVault({ status: 'mounted' });
    
    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} size="large" />
      </TestWrapper>
    );

    expect(screen.getByRole('button', { name: /force/i })).toBeInTheDocument();
  });

  test('should not show force unmount button for small size', () => {
    const vault = createMockVault({ status: 'mounted' });
    
    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} size="small" />
      </TestWrapper>
    );

    expect(screen.queryByRole('button', { name: /force/i })).not.toBeInTheDocument();
  });

  test('should show password prompt when mounting vault', async () => {
    const user = userEvent.setup();
    const vault = createMockVault({ status: 'unmounted' });
    
    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} />
      </TestWrapper>
    );

    const mountButton = screen.getByRole('button', { name: /mount/i });
    await user.click(mountButton);

    expect(screen.getByTestId('password-prompt-modal')).toBeInTheDocument();
    expect(screen.getByText(`Enter Password for ${vault.name}`)).toBeInTheDocument();
  });

  test('should show confirmation modal when unmounting vault', async () => {
    const user = userEvent.setup();
    const vault = createMockVault({ status: 'mounted' });
    
    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} />
      </TestWrapper>
    );

    const unmountButton = screen.getByRole('button', { name: /unmount/i });
    await user.click(unmountButton);

    expect(screen.getByTestId('confirmation-modal')).toBeInTheDocument();
    expect(screen.getByText('Unmount Vault')).toBeInTheDocument();
  });

  test('should call mountVault when password is submitted', async () => {
    const user = userEvent.setup();
    const vault = createMockVault({ status: 'unmounted' });
    
    mockVaultOperations.mountVault.mockResolvedValue({
      success: true,
      vault: vault,
    });

    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} />
      </TestWrapper>
    );

    // Click mount button
    const mountButton = screen.getByRole('button', { name: /mount/i });
    await user.click(mountButton);

    // Submit password
    const submitButton = screen.getByTestId('submit-password');
    await user.click(submitButton);

    await waitFor(() => {
      expect(mockVaultOperations.mountVault).toHaveBeenCalledWith(
        vault.id,
        'test-password',
        expect.any(Object)
      );
    });
  });

  test('should call unmountVault when confirmed', async () => {
    const user = userEvent.setup();
    const vault = createMockVault({ status: 'mounted' });
    
    mockVaultOperations.unmountVault.mockResolvedValue({
      success: true,
    });

    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} />
      </TestWrapper>
    );

    // Click unmount button
    const unmountButton = screen.getByRole('button', { name: /unmount/i });
    await user.click(unmountButton);

    // Confirm action
    const confirmButton = screen.getByTestId('confirm-action');
    await user.click(confirmButton);

    await waitFor(() => {
      expect(mockVaultOperations.unmountVault).toHaveBeenCalledWith(
        vault.id,
        expect.any(Object)
      );
    });
  });

  test('should show success notification on successful mount', async () => {
    const user = userEvent.setup();
    const vault = createMockVault({ status: 'unmounted' });
    
    mockVaultOperations.mountVault.mockResolvedValue({
      success: true,
      vault: vault,
    });

    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} />
      </TestWrapper>
    );

    // Mount vault
    const mountButton = screen.getByRole('button', { name: /mount/i });
    await user.click(mountButton);
    
    const submitButton = screen.getByTestId('submit-password');
    await user.click(submitButton);

    await waitFor(() => {
      expect(mockAppActions.addNotification).toHaveBeenCalledWith(
        expect.objectContaining({
          type: 'success',
          title: 'Vault Mounted',
        })
      );
    });
  });

  test('should show error notification on failed mount', async () => {
    const user = userEvent.setup();
    const vault = createMockVault({ status: 'unmounted' });
    
    mockVaultOperations.mountVault.mockResolvedValue({
      success: false,
      error: 'Invalid password',
    });

    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} />
      </TestWrapper>
    );

    // Mount vault
    const mountButton = screen.getByRole('button', { name: /mount/i });
    await user.click(mountButton);
    
    const submitButton = screen.getByTestId('submit-password');
    await user.click(submitButton);

    await waitFor(() => {
      expect(mockAppActions.addNotification).toHaveBeenCalledWith(
        expect.objectContaining({
          type: 'error',
          title: 'Mount Failed',
        })
      );
    });
  });

  test('should show progress modal during operation', async () => {
    const user = userEvent.setup();
    const vault = createMockVault({ status: 'unmounted' });
    
    // Mock a slow operation
    mockVaultOperations.mountVault.mockImplementation(
      (vaultId, password, options) => {
        // Simulate progress updates
        setTimeout(() => {
          options?.onProgress?.({
            stage: 'authenticating',
            percentage: 50,
            message: 'Verifying password...',
          });
        }, 100);
        
        return new Promise(resolve => {
          setTimeout(() => {
            resolve({ success: true, vault });
          }, 500);
        });
      }
    );

    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} />
      </TestWrapper>
    );

    // Mount vault
    const mountButton = screen.getByRole('button', { name: /mount/i });
    await user.click(mountButton);
    
    const submitButton = screen.getByTestId('submit-password');
    await user.click(submitButton);

    // Should show progress modal
    await waitFor(() => {
      expect(screen.getByTestId('progress-modal')).toBeInTheDocument();
    });
  });

  test('should handle force unmount', async () => {
    const user = userEvent.setup();
    const vault = createMockVault({ status: 'mounted' });
    
    mockVaultOperations.unmountVault.mockResolvedValue({
      success: true,
    });

    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} size="large" />
      </TestWrapper>
    );

    // Click force unmount button
    const forceButton = screen.getByRole('button', { name: /force/i });
    await user.click(forceButton);

    // Confirm action
    const confirmButton = screen.getByTestId('confirm-action');
    await user.click(confirmButton);

    await waitFor(() => {
      expect(mockVaultOperations.unmountVault).toHaveBeenCalledWith(
        vault.id,
        expect.objectContaining({ force: true })
      );
    });
  });

  test('should show warning for already mounted vault', async () => {
    const user = userEvent.setup();
    const vault = createMockVault({ status: 'mounted' });
    
    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} />
      </TestWrapper>
    );

    // Try to mount already mounted vault
    const mountButton = screen.getByRole('button', { name: /mount/i });
    await user.click(mountButton);

    expect(mockAppActions.addNotification).toHaveBeenCalledWith(
      expect.objectContaining({
        type: 'warning',
        title: 'Already Mounted',
      })
    );
  });

  test('should show warning for not mounted vault on unmount', async () => {
    const user = userEvent.setup();
    const vault = createMockVault({ status: 'unmounted' });
    
    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} />
      </TestWrapper>
    );

    // Try to unmount unmounted vault
    const unmountButton = screen.getByRole('button', { name: /unmount/i });
    await user.click(unmountButton);

    expect(mockAppActions.addNotification).toHaveBeenCalledWith(
      expect.objectContaining({
        type: 'warning',
        title: 'Not Mounted',
      })
    );
  });

  test('should handle vault with error status', async () => {
    const user = userEvent.setup();
    const vault = createMockVault({ status: 'error' });
    
    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} />
      </TestWrapper>
    );

    // Should show confirmation for error vault
    const mountButton = screen.getByRole('button', { name: /mount/i });
    await user.click(mountButton);

    expect(screen.getByTestId('confirmation-modal')).toBeInTheDocument();
    expect(screen.getByText('Mount Vault with Errors')).toBeInTheDocument();
  });

  test('should cancel password prompt', async () => {
    const user = userEvent.setup();
    const vault = createMockVault({ status: 'unmounted' });
    
    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} />
      </TestWrapper>
    );

    // Open password prompt
    const mountButton = screen.getByRole('button', { name: /mount/i });
    await user.click(mountButton);

    // Cancel password prompt
    const cancelButton = screen.getByTestId('cancel-password');
    await user.click(cancelButton);

    expect(screen.queryByTestId('password-prompt-modal')).not.toBeInTheDocument();
  });

  test('should cancel confirmation modal', async () => {
    const user = userEvent.setup();
    const vault = createMockVault({ status: 'mounted' });
    
    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} />
      </TestWrapper>
    );

    // Open confirmation modal
    const unmountButton = screen.getByRole('button', { name: /unmount/i });
    await user.click(unmountButton);

    // Cancel confirmation
    const cancelButton = screen.getByTestId('cancel-action');
    await user.click(cancelButton);

    expect(screen.queryByTestId('confirmation-modal')).not.toBeInTheDocument();
  });

  test('should disable buttons during operation', async () => {
    const user = userEvent.setup();
    const vault = createMockVault({ status: 'unmounted' });
    
    // Mock a slow operation
    mockVaultOperations.mountVault.mockImplementation(() => {
      return new Promise(resolve => {
        setTimeout(() => resolve({ success: true, vault }), 1000);
      });
    });

    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} />
      </TestWrapper>
    );

    // Start mount operation
    const mountButton = screen.getByRole('button', { name: /mount/i });
    await user.click(mountButton);
    
    const submitButton = screen.getByTestId('submit-password');
    await user.click(submitButton);

    // Buttons should be disabled during operation
    await waitFor(() => {
      expect(mountButton).toBeDisabled();
    });
  });

  test('should render without labels when showLabels is false', () => {
    const vault = createMockVault();
    
    render(
      <TestWrapper>
        <VaultOperationControls vault={vault} showLabels={false} />
      </TestWrapper>
    );

    // Should not show text labels, only icons
    expect(screen.queryByText('Mount')).not.toBeInTheDocument();
    expect(screen.queryByText('Unmount')).not.toBeInTheDocument();
  });

  test('should apply custom className', () => {
    const vault = createMockVault();
    
    const { container } = render(
      <TestWrapper>
        <VaultOperationControls vault={vault} className="custom-class" />
      </TestWrapper>
    );

    expect(container.firstChild).toHaveClass('custom-class');
  });

  test('should handle different orientations', () => {
    const vault = createMockVault();
    
    const { rerender } = render(
      <TestWrapper>
        <VaultOperationControls vault={vault} orientation="horizontal" />
      </TestWrapper>
    );

    expect(screen.getByRole('button', { name: /mount/i }).closest('.vault-operation-controls')).toHaveClass('horizontal');

    rerender(
      <TestWrapper>
        <VaultOperationControls vault={vault} orientation="vertical" />
      </TestWrapper>
    );

    expect(screen.getByRole('button', { name: /mount/i }).closest('.vault-operation-controls')).toHaveClass('vertical');
  });
});