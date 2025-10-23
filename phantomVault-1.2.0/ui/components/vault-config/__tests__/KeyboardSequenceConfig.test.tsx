/**
 * Keyboard Sequence Configuration Tests
 * 
 * Unit tests for the KeyboardSequenceConfig component
 */

import React from 'react';
import { render, screen, fireEvent, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { KeyboardSequenceConfig } from '../KeyboardSequenceConfig';
import { VaultProvider } from '../../../contexts/VaultContext';
import { VaultInfo } from '../../../types';

// ==================== MOCKS ====================

const mockVaults: VaultInfo[] = [
  {
    id: 'vault-1',
    name: 'Personal Documents',
    path: '/home/user/personal',
    status: 'unmounted',
    lastAccessed: new Date('2024-01-01'),
    size: 1024 * 1024 * 100,
    folderCount: 25
  },
  {
    id: 'vault-2',
    name: 'Work Files',
    path: '/home/user/work',
    status: 'mounted',
    lastAccessed: new Date('2024-01-02'),
    size: 1024 * 1024 * 500,
    folderCount: 50
  }
];

// Mock KeyboardSequenceInput component
jest.mock('../../common/KeyboardSequenceInput', () => ({
  KeyboardSequenceInput: ({ value, onChange, existingSequences }: any) => (
    <div data-testid=\"keyboard-sequence-input\">
      <input
        data-testid=\"sequence-input\"
        value={value}
        onChange={(e) => onChange(e.target.value)}
      />
      <div data-testid=\"existing-sequences\">
        {existingSequences?.join(', ')}
      </div>
    </div>
  ),
}));

// Mock validation hook
jest.mock('../../../hooks/useVaultValidation', () => ({
  useVaultValidation: ({ existingVaults, currentVaultId }: any) => ({
    validateField: (field: string, value: any) => {
      if (field === 'keyboardSequence') {
        if (!value) return { isValid: true, errors: [] };
        if (value.length < 2) {
          return { 
            isValid: false, 
            errors: [{ field, message: 'Sequence must be at least 2 characters', code: 'MIN_LENGTH' }] 
          };
        }
        if (existingVaults?.some((v: VaultInfo) => v.name === value && v.id !== currentVaultId)) {
          return { 
            isValid: false, 
            errors: [{ field, message: 'This sequence is already in use', code: 'DUPLICATE_SEQUENCE' }] 
          };
        }
      }
      return { isValid: true, errors: [] };
    },
    getFieldError: () => undefined,
    hasFieldError: () => false,
  }),
}));

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
    <VaultProvider value={{ state: mockVaultState, actions: mockVaultActions }}>
      {children}
    </VaultProvider>
  );
};

// ==================== SETUP ====================

const defaultProps = {
  value: '',
  onChange: jest.fn(),
};

beforeEach(() => {
  jest.clearAllMocks();
});

// ==================== COMPONENT TESTS ====================

describe('KeyboardSequenceConfig', () => {
  test('should render keyboard sequence input', () => {
    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} />
      </TestWrapper>
    );

    expect(screen.getByTestId('keyboard-sequence-input')).toBeInTheDocument();
    expect(screen.getByTestId('sequence-input')).toBeInTheDocument();
  });

  test('should pass existing sequences to input component', () => {
    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} />
      </TestWrapper>
    );

    const existingSequences = screen.getByTestId('existing-sequences');
    expect(existingSequences).toHaveTextContent('Personal Documents, Work Files');
  });

  test('should exclude current vault from existing sequences', () => {
    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} currentVaultId=\"vault-1\" />
      </TestWrapper>
    );

    const existingSequences = screen.getByTestId('existing-sequences');
    expect(existingSequences).toHaveTextContent('Work Files');
    expect(existingSequences).not.toHaveTextContent('Personal Documents');
  });

  test('should handle sequence changes', async () => {
    const user = userEvent.setup();
    const onChange = jest.fn();

    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} onChange={onChange} />
      </TestWrapper>
    );

    const input = screen.getByTestId('sequence-input');
    await user.type(input, 'testseq');

    expect(onChange).toHaveBeenCalledWith('testseq');
  });

  test('should show sequence analysis for valid sequences', () => {
    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} value=\"TestSeq123!\" />
      </TestWrapper>
    );

    expect(screen.getByText('Sequence Analysis')).toBeInTheDocument();
    expect(screen.getByText('12 characters')).toBeInTheDocument();
  });

  test('should calculate sequence strength correctly', () => {
    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} value=\"ComplexSeq123!\" />
      </TestWrapper>
    );

    expect(screen.getByText('STRONG')).toBeInTheDocument();
    expect(screen.getByText('Numbers')).toBeInTheDocument();
    expect(screen.getByText('Letters')).toBeInTheDocument();
    expect(screen.getByText('Symbols')).toBeInTheDocument();
    expect(screen.getByText('Mixed Case')).toBeInTheDocument();
  });

  test('should show weak strength for simple sequences', () => {
    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} value=\"weak\" />
      </TestWrapper>
    );

    expect(screen.getByText('WEAK')).toBeInTheDocument();
  });

  test('should detect and show conflicts', () => {
    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} value=\"Personal Documents\" />
      </TestWrapper>
    );

    expect(screen.getByText(/Potential Conflicts/)).toBeInTheDocument();
    expect(screen.getByText('Personal Documents')).toBeInTheDocument();
    expect(screen.getByText('100% similar')).toBeInTheDocument();
  });

  test('should allow toggling conflict visibility', async () => {
    const user = userEvent.setup();

    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} value=\"Personal Documents\" />
      </TestWrapper>
    );

    const toggleButton = screen.getByRole('button', { name: /▼/ });
    await user.click(toggleButton);

    expect(screen.queryByText('Personal Documents')).not.toBeInTheDocument();
    expect(screen.getByRole('button', { name: /▶/ })).toBeInTheDocument();
  });

  test('should show advanced options when enabled', () => {
    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} showAdvancedOptions={true} />
      </TestWrapper>
    );

    expect(screen.getByText('Advanced Options')).toBeInTheDocument();
    expect(screen.getByText('Simple Mode')).toBeInTheDocument();
    expect(screen.getByText('Advanced Mode')).toBeInTheDocument();
  });

  test('should hide advanced options when disabled', () => {
    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} showAdvancedOptions={false} />
      </TestWrapper>
    );

    expect(screen.queryByText('Advanced Options')).not.toBeInTheDocument();
  });

  test('should handle mode selection', async () => {
    const user = userEvent.setup();

    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} showAdvancedOptions={true} />
      </TestWrapper>
    );

    const advancedRadio = screen.getByLabelText('Advanced Mode');
    await user.click(advancedRadio);

    expect(advancedRadio).toBeChecked();
  });

  test('should generate random sequence', async () => {
    const user = userEvent.setup();
    const onChange = jest.fn();

    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} onChange={onChange} showAdvancedOptions={true} />
      </TestWrapper>
    );

    const generateButton = screen.getByText('🎲 Generate Random');
    await user.click(generateButton);

    expect(onChange).toHaveBeenCalledWith(expect.any(String));
    const generatedSequence = onChange.mock.calls[0][0];
    expect(generatedSequence.length).toBeGreaterThanOrEqual(6);
    expect(generatedSequence.length).toBeLessThanOrEqual(8);
  });

  test('should clear sequence', async () => {
    const user = userEvent.setup();
    const onChange = jest.fn();

    render(
      <TestWrapper>
        <KeyboardSequenceConfig 
          {...defaultProps} 
          value=\"existing\" 
          onChange={onChange} 
          showAdvancedOptions={true} 
        />
      </TestWrapper>
    );

    const clearButton = screen.getByText('🗑️ Clear');
    await user.click(clearButton);

    expect(onChange).toHaveBeenCalledWith('');
  });

  test('should disable clear button when no value', () => {
    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} showAdvancedOptions={true} />
      </TestWrapper>
    );

    const clearButton = screen.getByText('🗑️ Clear');
    expect(clearButton).toBeDisabled();
  });

  test('should handle disabled state', () => {
    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} disabled={true} />
      </TestWrapper>
    );

    const container = screen.getByTestId('keyboard-sequence-input').parentElement;
    expect(container).toHaveClass('disabled');
  });

  test('should show validation errors', () => {
    // Mock validation to return error
    jest.doMock('../../../hooks/useVaultValidation', () => ({
      useVaultValidation: () => ({
        validateField: () => ({
          isValid: false,
          errors: [{ field: 'keyboardSequence', message: 'Sequence is too short', code: 'MIN_LENGTH' }]
        }),
        getFieldError: () => undefined,
        hasFieldError: () => false,
      }),
    }));

    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} value=\"a\" />
      </TestWrapper>
    );

    expect(screen.getByText('Sequence is too short')).toBeInTheDocument();
  });

  test('should calculate entropy correctly', () => {
    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} value=\"abcdefgh\" />
      </TestWrapper>
    );

    // Should show entropy value
    expect(screen.getByText(/bits/)).toBeInTheDocument();
  });

  test('should detect similar sequences', () => {
    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} value=\"PersonalDocs\" />
      </TestWrapper>
    );

    // Should detect similarity to \"Personal Documents\"
    expect(screen.getByText(/Potential Conflicts/)).toBeInTheDocument();
  });

  test('should handle empty sequence gracefully', () => {
    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} value=\"\" />
      </TestWrapper>
    );

    // Should not show analysis for empty sequence
    expect(screen.queryByText('Sequence Analysis')).not.toBeInTheDocument();
    expect(screen.queryByText('Potential Conflicts')).not.toBeInTheDocument();
  });

  test('should prevent conflicts in generated sequences', async () => {
    const user = userEvent.setup();
    const onChange = jest.fn();

    // Mock Math.random to return predictable values that would create conflicts
    const originalRandom = Math.random;
    let callCount = 0;
    Math.random = jest.fn(() => {
      callCount++;
      // First call creates a conflict, second call creates a unique sequence
      return callCount === 1 ? 0 : 0.5;
    });

    render(
      <TestWrapper>
        <KeyboardSequenceConfig {...defaultProps} onChange={onChange} showAdvancedOptions={true} />
      </TestWrapper>
    );

    const generateButton = screen.getByText('🎲 Generate Random');
    await user.click(generateButton);

    expect(onChange).toHaveBeenCalled();
    
    // Restore original Math.random
    Math.random = originalRandom;
  });
});"