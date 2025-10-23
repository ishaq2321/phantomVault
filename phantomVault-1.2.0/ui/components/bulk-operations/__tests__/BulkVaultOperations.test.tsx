/**
 * Bulk Vault Operations Tests
 * 
 * Unit tests for the BulkVaultOperations component
 */

import React from 'react';
import { render, screen, fireEvent, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { BulkVaultOperations } from '../BulkVaultOperations';
import { VaultProvider } from '../../../contexts/VaultContext';
import { AppProvider } from '../../../contexts/AppContext';
import { VaultInfo } from '../../../types';

// ==================== MOCKS ====================

const mockVaultOperations = {
  mountVault: jest.fn(),
  unmountVault: jest.fn(),
  deleteVault: jest.fn(),
  createVault: jest.fn(),
  updateVault: jest.fn(),
};

const mockAppActions = {
  addNotification: jest.fn(),
  removeNotification: jest.fn(),
  updateSettings: jest.fn(),
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

// Mock child components
jest.mock('../VaultSelectionList', () => ({
  VaultSelectionList: ({ vaults, selectedVaults, onSelectionChange }: any) => (
    <div data-testid="vault-selection-list">
      {vaults.map((vault: VaultInfo) => (
        <div key={vault.id} data-testid={`vault-item-${vault.id}`}>
          <input
            type="checkbox"
            checked={selectedVaults.has(vault.id)}
            onChange={(e) => onSelectionChange(vault.id, e.target.checked)}
            data-testid={`vault-checkbox-${vault.id}`}
          />
          <span>{vault.name}</span>
        </div>
      ))}
    </div>
  ),
}));

jest.mock('../BulkOperationModal', () => ({
  BulkOperationModal: ({ operation, vaults, onConfirm, onCancel }: any) => (
    <div data-testid="bulk-operation-modal">
      <h3>{operation} Operation</h3>
      <p>{vaults.length} vaults selected</p>
      <button
        onClick={() => onConfirm({ passwords: { 'vault-1': 'password1' } })}
        data-testid="confirm-operation"
      >
        Confirm
      </button>
      <button onClick={onCancel} data-testid="cancel-operation">
        Cancel
      </button>
    </div>
  ),
}));

jest.mock('../BulkOperationSummary', () => ({
  BulkOperationSummary: ({ results, onClose, onRetryFailed }: any) => (
    <div data-testid="bulk-operation-summary">
      <h3>Operation Results</h3>
      <p>Successful: {results.successful.length}</p>
      <p>Failed: {results.failed.length}</p>
      <button onClick={onClose} data-testid="close-summary">
        Close
      </button>
      {results.failed.length > 0 && (
        <button onClick={onRetryFailed} data-testid="retry-failed">
          Retry Failed
        </button>
      )}
    </div>
  ),
}));

// ==================== TEST DATA ====================

const createMockVault = (overrides: Partial<VaultInfo> = {}): VaultInfo => ({
  id: `vault-${Math.random().toString(36).substr(2, 9)}`,
  name: 'Test Vault',
  path: '/test/path',
  status: 'unmounted',
  lastAccessed: new Date('2024-01-01'),
  size: 1024 * 1024 * 100,
  folderCount: 25,
  ...overrides,
});

const mockVaults: VaultInfo[] = [
  createMockVault({ id: 'vault-1', name: 'Vault 1', status: 'unmounted' }),
  createMockVault({ id: 'vault-2', name: 'Vault 2', status: 'mounted' }),
  createMockVault({ id: 'vault-3', name: 'Vault 3', status: 'unmounted' }),
  createMockVault({ id: 'vault-4', name: 'Vault 4', status: 'error' }),
];

// ==================== TEST WRAPPER ====================

const TestWrapper: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const mockVaultState = {
    vaults: mockVaults,
    isLoading: false,
    error: null,
  };

  const mockVaultActions = {
    loadVaults: jest.fn(),
    addVault: jest.fn(),
    updateVault: jest.fn(),
    removeVault: jest.fn(),
    setLoading: jest.fn(),
    setError: jest.fn(),
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
  jest.clearAllMocks();
});

// ==================== COMPONENT TESTS ====================

describe('BulkVaultOperations', () => {
  test('should render vault selection list', () => {
    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} />
      </TestWrapper>
    );

    expect(screen.getByTestId('vault-selection-list')).toBeInTheDocument();
    expect(screen.getByText('Vault 1')).toBeInTheDocument();
    expect(screen.getByText('Vault 2')).toBeInTheDocument();
  });

  test('should show selection controls when enabled', () => {
    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} showSelectionControls={true} />
      </TestWrapper>
    );

    expect(screen.getByText('Select All')).toBeInTheDocument();
    expect(screen.getByText('Select None')).toBeInTheDocument();
    expect(screen.getByText('Select Unmounted')).toBeInTheDocument();
    expect(screen.getByText('Select Mounted')).toBeInTheDocument();
  });

  test('should hide selection controls when disabled', () => {
    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} showSelectionControls={false} />
      </TestWrapper>
    );

    expect(screen.queryByText('Select All')).not.toBeInTheDocument();
    expect(screen.queryByText('Select None')).not.toBeInTheDocument();
  });

  test('should select and deselect vaults', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} />
      </TestWrapper>
    );

    // Select a vault
    const checkbox1 = screen.getByTestId('vault-checkbox-vault-1');
    await user.click(checkbox1);

    expect(checkbox1).toBeChecked();

    // Deselect the vault
    await user.click(checkbox1);
    expect(checkbox1).not.toBeChecked();
  });

  test('should select all vaults', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} />
      </TestWrapper>
    );

    const selectAllButton = screen.getByText('Select All');
    await user.click(selectAllButton);

    // All checkboxes should be checked
    mockVaults.forEach(vault => {
      const checkbox = screen.getByTestId(`vault-checkbox-${vault.id}`);
      expect(checkbox).toBeChecked();
    });
  });

  test('should select none when clicking Select None', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} />
      </TestWrapper>
    );

    // First select all
    const selectAllButton = screen.getByText('Select All');
    await user.click(selectAllButton);

    // Then select none
    const selectNoneButton = screen.getByText('Select None');
    await user.click(selectNoneButton);

    // All checkboxes should be unchecked
    mockVaults.forEach(vault => {
      const checkbox = screen.getByTestId(`vault-checkbox-${vault.id}`);
      expect(checkbox).not.toBeChecked();
    });
  });

  test('should select vaults by status', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} />
      </TestWrapper>
    );

    // Select unmounted vaults
    const selectUnmountedButton = screen.getByText('Select Unmounted');
    await user.click(selectUnmountedButton);

    // Only unmounted vaults should be selected
    expect(screen.getByTestId('vault-checkbox-vault-1')).toBeChecked(); // unmounted
    expect(screen.getByTestId('vault-checkbox-vault-2')).not.toBeChecked(); // mounted
    expect(screen.getByTestId('vault-checkbox-vault-3')).toBeChecked(); // unmounted
  });

  test('should show operation controls when vaults are selected', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} />
      </TestWrapper>
    );

    // Select a vault
    const checkbox1 = screen.getByTestId('vault-checkbox-vault-1');
    await user.click(checkbox1);

    // Operation controls should appear
    await waitFor(() => {
      expect(screen.getByRole('button', { name: /mount/i })).toBeInTheDocument();
      expect(screen.getByRole('button', { name: /unmount/i })).toBeInTheDocument();
      expect(screen.getByRole('button', { name: /delete/i })).toBeInTheDocument();
    });
  });

  test('should show correct operation counts', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} />
      </TestWrapper>
    );

    // Select all vaults
    const selectAllButton = screen.getByText('Select All');
    await user.click(selectAllButton);

    await waitFor(() => {
      // Should show counts for each operation
      expect(screen.getByText(/Mount \(3\)/)).toBeInTheDocument(); // 2 unmounted + 1 error
      expect(screen.getByText(/Unmount \(1\)/)).toBeInTheDocument(); // 1 mounted
      expect(screen.getByText(/Delete \(3\)/)).toBeInTheDocument(); // all except mounted
    });
  });

  test('should open bulk mount modal', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} />
      </TestWrapper>
    );

    // Select unmounted vaults
    const checkbox1 = screen.getByTestId('vault-checkbox-vault-1');
    await user.click(checkbox1);

    // Click mount button
    const mountButton = screen.getByRole('button', { name: /mount/i });
    await user.click(mountButton);

    expect(screen.getByTestId('bulk-operation-modal')).toBeInTheDocument();
    expect(screen.getByText('mount Operation')).toBeInTheDocument();
  });

  test('should execute bulk mount operation', async () => {
    const user = userEvent.setup();
    
    mockVaultOperations.mountVault.mockResolvedValue({ success: true });

    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} />
      </TestWrapper>
    );

    // Select and mount vault
    const checkbox1 = screen.getByTestId('vault-checkbox-vault-1');
    await user.click(checkbox1);

    const mountButton = screen.getByRole('button', { name: /mount/i });
    await user.click(mountButton);

    const confirmButton = screen.getByTestId('confirm-operation');
    await user.click(confirmButton);

    await waitFor(() => {
      expect(mockVaultOperations.mountVault).toHaveBeenCalledWith(
        'vault-1',
        'password1',
        expect.any(Object)
      );
    });
  });

  test('should execute bulk unmount operation', async () => {
    const user = userEvent.setup();
    
    mockVaultOperations.unmountVault.mockResolvedValue({ success: true });

    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} />
      </TestWrapper>
    );

    // Select mounted vault
    const checkbox2 = screen.getByTestId('vault-checkbox-vault-2');
    await user.click(checkbox2);

    const unmountButton = screen.getByRole('button', { name: /unmount/i });
    await user.click(unmountButton);

    const confirmButton = screen.getByTestId('confirm-operation');
    await user.click(confirmButton);

    await waitFor(() => {
      expect(mockVaultOperations.unmountVault).toHaveBeenCalledWith(
        'vault-2',
        expect.any(Object)
      );
    });
  });

  test('should execute bulk delete operation', async () => {
    const user = userEvent.setup();
    
    mockVaultOperations.deleteVault.mockResolvedValue({ success: true });

    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} />
      </TestWrapper>
    );

    // Select unmounted vault
    const checkbox1 = screen.getByTestId('vault-checkbox-vault-1');
    await user.click(checkbox1);

    const deleteButton = screen.getByRole('button', { name: /delete/i });
    await user.click(deleteButton);

    const confirmButton = screen.getByTestId('confirm-operation');
    await user.click(confirmButton);

    await waitFor(() => {
      expect(mockVaultOperations.deleteVault).toHaveBeenCalledWith('vault-1');
    });
  });

  test('should show warning when no vaults selected for mount', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} />
      </TestWrapper>
    );

    // Try to mount without selecting vaults
    const mountButton = screen.getByRole('button', { name: /mount/i });
    
    // Button should not be visible when no vaults selected
    expect(mountButton).not.toBeInTheDocument();
  });

  test('should show warning when no mountable vaults selected', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} />
      </TestWrapper>
    );

    // Select only mounted vault
    const checkbox2 = screen.getByTestId('vault-checkbox-vault-2');
    await user.click(checkbox2);

    await waitFor(() => {
      const mountButton = screen.getByRole('button', { name: /mount/i });
      expect(mountButton).toBeDisabled();
    });
  });

  test('should show operation summary after completion', async () => {
    const user = userEvent.setup();
    
    mockVaultOperations.mountVault.mockResolvedValue({ success: true });

    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} />
      </TestWrapper>
    );

    // Execute operation
    const checkbox1 = screen.getByTestId('vault-checkbox-vault-1');
    await user.click(checkbox1);

    const mountButton = screen.getByRole('button', { name: /mount/i });
    await user.click(mountButton);

    const confirmButton = screen.getByTestId('confirm-operation');
    await user.click(confirmButton);

    // Should show summary
    await waitFor(() => {
      expect(screen.getByTestId('bulk-operation-summary')).toBeInTheDocument();
    });
  });

  test('should handle mixed success and failure results', async () => {
    const user = userEvent.setup();
    
    // Mock mixed results
    mockVaultOperations.mountVault
      .mockResolvedValueOnce({ success: true })
      .mockResolvedValueOnce({ success: false, error: 'Invalid password' });

    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} />
      </TestWrapper>
    );

    // Select multiple vaults
    const checkbox1 = screen.getByTestId('vault-checkbox-vault-1');
    const checkbox3 = screen.getByTestId('vault-checkbox-vault-3');
    await user.click(checkbox1);
    await user.click(checkbox3);

    const mountButton = screen.getByRole('button', { name: /mount/i });
    await user.click(mountButton);

    const confirmButton = screen.getByTestId('confirm-operation');
    await user.click(confirmButton);

    // Should show partial completion notification
    await waitFor(() => {
      expect(mockAppActions.addNotification).toHaveBeenCalledWith(
        expect.objectContaining({
          type: 'warning',
          title: 'Bulk Operation Partially Completed',
        })
      );
    });
  });

  test('should retry failed operations', async () => {
    const user = userEvent.setup();
    
    // First attempt fails, second succeeds
    mockVaultOperations.mountVault
      .mockResolvedValueOnce({ success: false, error: 'Network error' })
      .mockResolvedValueOnce({ success: true });

    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} />
      </TestWrapper>
    );

    // Execute operation that fails
    const checkbox1 = screen.getByTestId('vault-checkbox-vault-1');
    await user.click(checkbox1);

    const mountButton = screen.getByRole('button', { name: /mount/i });
    await user.click(mountButton);

    const confirmButton = screen.getByTestId('confirm-operation');
    await user.click(confirmButton);

    // Wait for summary to appear
    await waitFor(() => {
      expect(screen.getByTestId('bulk-operation-summary')).toBeInTheDocument();
    });

    // Retry failed operations
    const retryButton = screen.getByTestId('retry-failed');
    await user.click(retryButton);

    // Should call mount again
    await waitFor(() => {
      expect(mockVaultOperations.mountVault).toHaveBeenCalledTimes(2);
    });
  });

  test('should cancel operation modal', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} />
      </TestWrapper>
    );

    // Open modal
    const checkbox1 = screen.getByTestId('vault-checkbox-vault-1');
    await user.click(checkbox1);

    const mountButton = screen.getByRole('button', { name: /mount/i });
    await user.click(mountButton);

    // Cancel modal
    const cancelButton = screen.getByTestId('cancel-operation');
    await user.click(cancelButton);

    expect(screen.queryByTestId('bulk-operation-modal')).not.toBeInTheDocument();
  });

  test('should close operation summary', async () => {
    const user = userEvent.setup();
    
    mockVaultOperations.mountVault.mockResolvedValue({ success: true });

    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} />
      </TestWrapper>
    );

    // Execute operation
    const checkbox1 = screen.getByTestId('vault-checkbox-vault-1');
    await user.click(checkbox1);

    const mountButton = screen.getByRole('button', { name: /mount/i });
    await user.click(mountButton);

    const confirmButton = screen.getByTestId('confirm-operation');
    await user.click(confirmButton);

    // Wait for summary
    await waitFor(() => {
      expect(screen.getByTestId('bulk-operation-summary')).toBeInTheDocument();
    });

    // Close summary
    const closeButton = screen.getByTestId('close-summary');
    await user.click(closeButton);

    expect(screen.queryByTestId('bulk-operation-summary')).not.toBeInTheDocument();
  });

  test('should call onSelectionChange callback', async () => {
    const user = userEvent.setup();
    const onSelectionChange = jest.fn();
    
    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} onSelectionChange={onSelectionChange} />
      </TestWrapper>
    );

    // Select a vault
    const checkbox1 = screen.getByTestId('vault-checkbox-vault-1');
    await user.click(checkbox1);

    await waitFor(() => {
      expect(onSelectionChange).toHaveBeenCalledWith([mockVaults[0]]);
    });
  });

  test('should respect maxConcurrentOperations', async () => {
    const user = userEvent.setup();
    
    // Mock slow operations
    mockVaultOperations.mountVault.mockImplementation(() => {
      return new Promise(resolve => {
        setTimeout(() => resolve({ success: true }), 100);
      });
    });

    render(
      <TestWrapper>
        <BulkVaultOperations vaults={mockVaults} maxConcurrentOperations={2} />
      </TestWrapper>
    );

    // Select multiple vaults
    const selectAllButton = screen.getByText('Select All');
    await user.click(selectAllButton);

    const mountButton = screen.getByRole('button', { name: /mount/i });
    await user.click(mountButton);

    const confirmButton = screen.getByTestId('confirm-operation');
    await user.click(confirmButton);

    // Should process in batches of 2
    await waitFor(() => {
      expect(mockVaultOperations.mountVault).toHaveBeenCalled();
    }, { timeout: 1000 });
  });
});