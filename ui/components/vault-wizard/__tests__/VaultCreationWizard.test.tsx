/**
 * Vault Creation Wizard Tests
 * 
 * Unit tests for the VaultCreationWizard component
 */

import React from 'react';
import { render, screen, fireEvent, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { VaultCreationWizard } from '../VaultCreationWizard';
import { VaultProvider } from '../../../contexts/VaultContext';
import { AppProvider } from '../../../contexts/AppContext';

// ==================== MOCKS ====================

const mockVaultOperations = {
  createVault: jest.fn(),
  mountVault: jest.fn(),
  unmountVault: jest.fn(),
  deleteVault: jest.fn(),
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

// Mock folder selector
jest.mock('../../common/FolderSelector', () => ({
  FolderSelector: ({ value, onChange, error }: any) => (
    <div data-testid="folder-selector">
      <input
        data-testid="folder-input\"
        value={value}
        onChange={(e) => onChange(e.target.value)}
        className={error ? 'error' : ''}
      />
      <button
        data-testid="folder-browse\"
        onClick={() => onChange('/selected/folder')}
      >
        Browse
      </button>
    </div>
  ),
}));

// Mock keyboard sequence input
jest.mock('../../common/KeyboardSequenceInput', () => ({
  KeyboardSequenceInput: ({ value, onChange }: any) => (
    <input
      data-testid="keyboard-sequence-input\"
      value={value}
      onChange={(e) => onChange(e.target.value)}
    />
  ),
}));

// ==================== TEST WRAPPER ====================

const TestWrapper: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const mockVaultState = {
    vaults: [],
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

const defaultProps = {
  onComplete: jest.fn(),
  onCancel: jest.fn(),
};

beforeEach(() => {
  jest.clearAllMocks();
  mockVaultOperations.createVault.mockResolvedValue({
    success: true,
    vault: {
      id: 'new-vault-id',
      name: 'Test Vault',
      path: '/test/path',
      status: 'unmounted',
      lastAccessed: new Date(),
      size: 0,
      folderCount: 0,
    },
  });
});

// ==================== COMPONENT TESTS ====================

describe('VaultCreationWizard', () => {
  test('should render initial step with basic info form', () => {
    render(
      <TestWrapper>
        <VaultCreationWizard {...defaultProps} />
      </TestWrapper>
    );

    expect(screen.getByText('Create New Vault')).toBeInTheDocument();
    expect(screen.getByText('Step 1 of 4')).toBeInTheDocument();
    expect(screen.getByLabelText(/vault name/i)).toBeInTheDocument();
    expect(screen.getByTestId('folder-selector')).toBeInTheDocument();
  });

  test('should validate required fields on first step', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <VaultCreationWizard {...defaultProps} />
      </TestWrapper>
    );

    const nextButton = screen.getByText('Next');
    await user.click(nextButton);

    await waitFor(() => {
      expect(screen.getByText(/vault name is required/i)).toBeInTheDocument();
      expect(screen.getByText(/folder path is required/i)).toBeInTheDocument();
    });
  });

  test('should proceed to next step when basic info is valid', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <VaultCreationWizard {...defaultProps} />
      </TestWrapper>
    );

    // Fill in basic info
    const nameInput = screen.getByLabelText(/vault name/i);
    await user.type(nameInput, 'Test Vault');

    const browseButton = screen.getByTestId('folder-browse');
    await user.click(browseButton);

    // Proceed to next step
    const nextButton = screen.getByText('Next');
    await user.click(nextButton);

    await waitFor(() => {
      expect(screen.getByText('Step 2 of 4')).toBeInTheDocument();
      expect(screen.getByText('Security Settings')).toBeInTheDocument();
    });
  });

  test('should validate password requirements on security step', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <VaultCreationWizard {...defaultProps} />
      </TestWrapper>
    );

    // Navigate to security step
    await user.type(screen.getByLabelText(/vault name/i), 'Test Vault');
    await user.click(screen.getByTestId('folder-browse'));
    await user.click(screen.getByText('Next'));

    await waitFor(() => {
      expect(screen.getByText('Security Settings')).toBeInTheDocument();
    });

    // Try to proceed without password
    const nextButton = screen.getByText('Next');
    await user.click(nextButton);

    await waitFor(() => {
      expect(screen.getByText(/password is required/i)).toBeInTheDocument();
    });
  });

  test('should validate password confirmation match', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <VaultCreationWizard {...defaultProps} />
      </TestWrapper>
    );

    // Navigate to security step
    await user.type(screen.getByLabelText(/vault name/i), 'Test Vault');
    await user.click(screen.getByTestId('folder-browse'));
    await user.click(screen.getByText('Next'));

    await waitFor(() => {
      expect(screen.getByText('Security Settings')).toBeInTheDocument();
    });

    // Enter mismatched passwords
    const passwordInput = screen.getByLabelText(/^password/i);
    const confirmInput = screen.getByLabelText(/confirm password/i);
    
    await user.type(passwordInput, 'StrongPass123!');
    await user.type(confirmInput, 'DifferentPass123!');

    const nextButton = screen.getByText('Next');
    await user.click(nextButton);

    await waitFor(() => {
      expect(screen.getByText(/passwords do not match/i)).toBeInTheDocument();
    });
  });

  test('should proceed to advanced settings with valid security info', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <VaultCreationWizard {...defaultProps} />
      </TestWrapper>
    );

    // Navigate through steps
    await user.type(screen.getByLabelText(/vault name/i), 'Test Vault');
    await user.click(screen.getByTestId('folder-browse'));
    await user.click(screen.getByText('Next'));

    await waitFor(() => {
      expect(screen.getByText('Security Settings')).toBeInTheDocument();
    });

    // Fill security info
    const passwordInput = screen.getByLabelText(/^password/i);
    const confirmInput = screen.getByLabelText(/confirm password/i);
    
    await user.type(passwordInput, 'StrongPass123!');
    await user.type(confirmInput, 'StrongPass123!');

    await user.click(screen.getByText('Next'));

    await waitFor(() => {
      expect(screen.getByText('Step 3 of 4')).toBeInTheDocument();
      expect(screen.getByText('Advanced Settings')).toBeInTheDocument();
    });
  });

  test('should handle keyboard sequence configuration', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <VaultCreationWizard {...defaultProps} />
      </TestWrapper>
    );

    // Navigate to advanced settings
    await user.type(screen.getByLabelText(/vault name/i), 'Test Vault');
    await user.click(screen.getByTestId('folder-browse'));
    await user.click(screen.getByText('Next'));

    await waitFor(() => {
      expect(screen.getByText('Security Settings')).toBeInTheDocument();
    });

    const passwordInput = screen.getByLabelText(/^password/i);
    const confirmInput = screen.getByLabelText(/confirm password/i);
    
    await user.type(passwordInput, 'StrongPass123!');
    await user.type(confirmInput, 'StrongPass123!');
    await user.click(screen.getByText('Next'));

    await waitFor(() => {
      expect(screen.getByText('Advanced Settings')).toBeInTheDocument();
    });

    // Configure keyboard sequence
    const sequenceInput = screen.getByTestId('keyboard-sequence-input');
    await user.type(sequenceInput, 'testvault');

    expect(sequenceInput).toHaveValue('testvault');
  });

  test('should show summary on final step', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <VaultCreationWizard {...defaultProps} />
      </TestWrapper>
    );

    // Navigate through all steps
    await user.type(screen.getByLabelText(/vault name/i), 'Test Vault');
    await user.click(screen.getByTestId('folder-browse'));
    await user.click(screen.getByText('Next'));

    await waitFor(() => {
      expect(screen.getByText('Security Settings')).toBeInTheDocument();
    });

    const passwordInput = screen.getByLabelText(/^password/i);
    const confirmInput = screen.getByLabelText(/confirm password/i);
    
    await user.type(passwordInput, 'StrongPass123!');
    await user.type(confirmInput, 'StrongPass123!');
    await user.click(screen.getByText('Next'));

    await waitFor(() => {
      expect(screen.getByText('Advanced Settings')).toBeInTheDocument();
    });

    await user.click(screen.getByText('Next'));

    await waitFor(() => {
      expect(screen.getByText('Step 4 of 4')).toBeInTheDocument();
      expect(screen.getByText('Review & Create')).toBeInTheDocument();
      expect(screen.getByText('Test Vault')).toBeInTheDocument();
      expect(screen.getByText('/selected/folder')).toBeInTheDocument();
    });
  });

  test('should create vault when confirmed', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <VaultCreationWizard {...defaultProps} />
      </TestWrapper>
    );

    // Navigate through all steps
    await user.type(screen.getByLabelText(/vault name/i), 'Test Vault');
    await user.click(screen.getByTestId('folder-browse'));
    await user.click(screen.getByText('Next'));

    await waitFor(() => {
      expect(screen.getByText('Security Settings')).toBeInTheDocument();
    });

    const passwordInput = screen.getByLabelText(/^password/i);
    const confirmInput = screen.getByLabelText(/confirm password/i);
    
    await user.type(passwordInput, 'StrongPass123!');
    await user.type(confirmInput, 'StrongPass123!');
    await user.click(screen.getByText('Next'));

    await waitFor(() => {
      expect(screen.getByText('Advanced Settings')).toBeInTheDocument();
    });

    await user.click(screen.getByText('Next'));

    await waitFor(() => {
      expect(screen.getByText('Review & Create')).toBeInTheDocument();
    });

    // Create vault
    const createButton = screen.getByText('Create Vault');
    await user.click(createButton);

    await waitFor(() => {
      expect(mockVaultOperations.createVault).toHaveBeenCalledWith({
        name: 'Test Vault',
        path: '/selected/folder',
        password: 'StrongPass123!',
        keyboardSequence: '',
        autoMount: false,
        encryptionLevel: 'standard',
        autoLock: true,
        lockTimeout: 15,
      });
    });

    expect(defaultProps.onComplete).toHaveBeenCalled();
  });

  test('should handle vault creation failure', async () => {
    const user = userEvent.setup();
    
    mockVaultOperations.createVault.mockResolvedValue({
      success: false,
      error: 'Failed to create vault',
    });

    render(
      <TestWrapper>
        <VaultCreationWizard {...defaultProps} />
      </TestWrapper>
    );

    // Navigate to final step and create
    await user.type(screen.getByLabelText(/vault name/i), 'Test Vault');
    await user.click(screen.getByTestId('folder-browse'));
    await user.click(screen.getByText('Next'));

    await waitFor(() => {
      expect(screen.getByText('Security Settings')).toBeInTheDocument();
    });

    const passwordInput = screen.getByLabelText(/^password/i);
    const confirmInput = screen.getByLabelText(/confirm password/i);
    
    await user.type(passwordInput, 'StrongPass123!');
    await user.type(confirmInput, 'StrongPass123!');
    await user.click(screen.getByText('Next'));
    await user.click(screen.getByText('Next'));

    await waitFor(() => {
      expect(screen.getByText('Review & Create')).toBeInTheDocument();
    });

    const createButton = screen.getByText('Create Vault');
    await user.click(createButton);

    await waitFor(() => {
      expect(mockAppActions.addNotification).toHaveBeenCalledWith(
        expect.objectContaining({
          type: 'error',
          title: 'Creation Failed',
        })
      );
    });
  });

  test('should allow navigation between steps', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <VaultCreationWizard {...defaultProps} />
      </TestWrapper>
    );

    // Navigate forward
    await user.type(screen.getByLabelText(/vault name/i), 'Test Vault');
    await user.click(screen.getByTestId('folder-browse'));
    await user.click(screen.getByText('Next'));

    await waitFor(() => {
      expect(screen.getByText('Step 2 of 4')).toBeInTheDocument();
    });

    // Navigate back
    const backButton = screen.getByText('Back');
    await user.click(backButton);

    await waitFor(() => {
      expect(screen.getByText('Step 1 of 4')).toBeInTheDocument();
      expect(screen.getByDisplayValue('Test Vault')).toBeInTheDocument();
    });
  });

  test('should handle cancel action', async () => {
    const user = userEvent.setup();
    
    render(
      <TestWrapper>
        <VaultCreationWizard {...defaultProps} />
      </TestWrapper>
    );

    const cancelButton = screen.getByText('Cancel');
    await user.click(cancelButton);

    expect(defaultProps.onCancel).toHaveBeenCalled();
  });

  test('should show loading state during creation', async () => {
    const user = userEvent.setup();
    
    // Make createVault return a pending promise
    let resolveCreate: (value: any) => void;
    const createPromise = new Promise((resolve) => {
      resolveCreate = resolve;
    });
    mockVaultOperations.createVault.mockReturnValue(createPromise);

    render(
      <TestWrapper>
        <VaultCreationWizard {...defaultProps} />
      </TestWrapper>
    );

    // Navigate to final step
    await user.type(screen.getByLabelText(/vault name/i), 'Test Vault');
    await user.click(screen.getByTestId('folder-browse'));
    await user.click(screen.getByText('Next'));

    await waitFor(() => {
      expect(screen.getByText('Security Settings')).toBeInTheDocument();
    });

    const passwordInput = screen.getByLabelText(/^password/i);
    const confirmInput = screen.getByLabelText(/confirm password/i);
    
    await user.type(passwordInput, 'StrongPass123!');
    await user.type(confirmInput, 'StrongPass123!');
    await user.click(screen.getByText('Next'));
    await user.click(screen.getByText('Next'));

    await waitFor(() => {
      expect(screen.getByText('Review & Create')).toBeInTheDocument();
    });

    // Start creation
    const createButton = screen.getByText('Create Vault');
    await user.click(createButton);

    // Should show loading state
    await waitFor(() => {
      expect(screen.getByText(/creating/i)).toBeInTheDocument();
    });

    // Resolve the promise
    resolveCreate!({ success: true, vault: { id: 'test' } });

    await waitFor(() => {
      expect(defaultProps.onComplete).toHaveBeenCalled();
    });
  });
});