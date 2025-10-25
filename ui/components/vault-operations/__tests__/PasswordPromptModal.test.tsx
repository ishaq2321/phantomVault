/**
 * Password Prompt Modal Tests
 * 
 * Unit tests for the PasswordPromptModal component
 */

import React from 'react';
import { render, screen, fireEvent, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { PasswordPromptModal } from '../PasswordPromptModal';

// ==================== MOCKS ====================

// Mock password strength indicator
jest.mock('../../common/PasswordStrengthIndicator', () => ({
  PasswordStrengthIndicator: ({ strength }: any) => (
    <div data-testid="password-strength-indicator">
      <span data-testid="strength-score">{strength.score}</span>
      <span data-testid="strength-feedback">{strength.feedback.join(', ')}</span>
    </div>
  ),
}));

// Mock validation hook
jest.mock('../../../hooks/useVaultValidation', () => ({
  usePasswordValidation: (password: string) => {
    const strength = {
      score: password.length >= 8 ? 4 : 2,
      feedback: password.length >= 8 ? [] : ['Use at least 8 characters'],
      requirements: {
        minLength: password.length >= 8,
        hasUppercase: /[A-Z]/.test(password),
        hasLowercase: /[a-z]/.test(password),
        hasNumbers: /\d/.test(password),
        hasSpecialChars: /[!@#$%^&*(),.?":{}|<>]/.test(password),
      },
    };

    return {
      strength,
      isPasswordValid: password.length >= 8,
      passwordError: password.length >= 8 ? undefined : 'Password too short',
    };
  },
}));

// ==================== SETUP ====================

const defaultProps = {
  vaultName: 'Test Vault',
  onSubmit: jest.fn(),
  onCancel: jest.fn(),
};

beforeEach(() => {
  jest.clearAllMocks();
});

// ==================== COMPONENT TESTS ====================

describe('PasswordPromptModal', () => {
  test('should render with vault name', () => {
    render(<PasswordPromptModal {...defaultProps} />);

    expect(screen.getByText('Enter Password for "Test Vault"')).toBeInTheDocument();
    expect(screen.getByPlaceholderText('Enter vault password')).toBeInTheDocument();
  });

  test('should render custom title and message', () => {
    render(
      <PasswordPromptModal
        {...defaultProps}
        title="Custom Title"
        message="Custom message for the user"
      />
    );

    expect(screen.getByText('Custom Title')).toBeInTheDocument();
    expect(screen.getByText('Custom message for the user')).toBeInTheDocument();
  });

  test('should focus password input on mount', () => {
    render(<PasswordPromptModal {...defaultProps} />);

    const passwordInput = screen.getByPlaceholderText('Enter vault password');
    expect(passwordInput).toHaveFocus();
  });

  test('should handle password input changes', async () => {
    const user = userEvent.setup();
    
    render(<PasswordPromptModal {...defaultProps} />);

    const passwordInput = screen.getByPlaceholderText('Enter vault password');
    await user.type(passwordInput, 'test-password');

    expect(passwordInput).toHaveValue('test-password');
  });

  test('should toggle password visibility', async () => {
    const user = userEvent.setup();
    
    render(<PasswordPromptModal {...defaultProps} />);

    const passwordInput = screen.getByPlaceholderText('Enter vault password');
    const toggleButton = screen.getByRole('button', { name: /show password/i });

    // Initially should be password type
    expect(passwordInput).toHaveAttribute('type', 'password');

    // Click toggle
    await user.click(toggleButton);
    expect(passwordInput).toHaveAttribute('type', 'text');

    // Click toggle again
    await user.click(toggleButton);
    expect(passwordInput).toHaveAttribute('type', 'password');
  });

  test('should submit password on form submit', async () => {
    const user = userEvent.setup();
    const onSubmit = jest.fn();
    
    render(<PasswordPromptModal {...defaultProps} onSubmit={onSubmit} />);

    const passwordInput = screen.getByPlaceholderText('Enter vault password');
    const submitButton = screen.getByRole('button', { name: /mount vault/i });

    await user.type(passwordInput, 'test-password');
    await user.click(submitButton);

    expect(onSubmit).toHaveBeenCalledWith('test-password');
  });

  test('should submit password on Enter key', async () => {
    const user = userEvent.setup();
    const onSubmit = jest.fn();
    
    render(<PasswordPromptModal {...defaultProps} onSubmit={onSubmit} />);

    const passwordInput = screen.getByPlaceholderText('Enter vault password');
    await user.type(passwordInput, 'test-password');
    await user.keyboard('{Enter}');

    expect(onSubmit).toHaveBeenCalledWith('test-password');
  });

  test('should call onCancel when cancel button clicked', async () => {
    const user = userEvent.setup();
    const onCancel = jest.fn();
    
    render(<PasswordPromptModal {...defaultProps} onCancel={onCancel} />);

    const cancelButton = screen.getByRole('button', { name: /cancel/i });
    await user.click(cancelButton);

    expect(onCancel).toHaveBeenCalled();
  });

  test('should call onCancel on Escape key', async () => {
    const user = userEvent.setup();
    const onCancel = jest.fn();
    
    render(<PasswordPromptModal {...defaultProps} onCancel={onCancel} />);

    await user.keyboard('{Escape}');

    expect(onCancel).toHaveBeenCalled();
  });

  test('should disable inputs when loading', () => {
    render(<PasswordPromptModal {...defaultProps} isLoading={true} />);

    const passwordInput = screen.getByPlaceholderText('Enter vault password');
    const toggleButton = screen.getByRole('button', { name: /show password/i });
    const submitButton = screen.getByRole('button', { name: /mounting/i });

    expect(passwordInput).toBeDisabled();
    expect(toggleButton).toBeDisabled();
    expect(submitButton).toBeDisabled();
  });

  test('should show loading state', () => {
    render(<PasswordPromptModal {...defaultProps} isLoading={true} />);

    expect(screen.getByText('Mounting...')).toBeInTheDocument();
    expect(screen.getByText('Please wait while we unlock your vault')).toBeInTheDocument();
  });

  test('should prevent cancel when loading', async () => {
    const user = userEvent.setup();
    const onCancel = jest.fn();
    
    render(<PasswordPromptModal {...defaultProps} isLoading={true} onCancel={onCancel} />);

    // Close button should not be visible when loading
    expect(screen.queryByRole('button', { name: /close/i })).not.toBeInTheDocument();

    // Escape key should not work when loading
    await user.keyboard('{Escape}');
    expect(onCancel).not.toHaveBeenCalled();
  });

  test('should show password strength indicator when enabled', async () => {
    const user = userEvent.setup();
    
    render(<PasswordPromptModal {...defaultProps} showStrengthIndicator={true} />);

    const passwordInput = screen.getByPlaceholderText('Enter vault password');
    await user.type(passwordInput, 'weak');

    expect(screen.getByTestId('password-strength-indicator')).toBeInTheDocument();
    expect(screen.getByTestId('strength-score')).toHaveTextContent('2');
  });

  test('should hide password strength indicator when disabled', async () => {
    const user = userEvent.setup();
    
    render(<PasswordPromptModal {...defaultProps} showStrengthIndicator={false} />);

    const passwordInput = screen.getByPlaceholderText('Enter vault password');
    await user.type(passwordInput, 'weak');

    expect(screen.queryByTestId('password-strength-indicator')).not.toBeInTheDocument();
  });

  test('should prevent submission with invalid password when strength checking enabled', async () => {
    const user = userEvent.setup();
    const onSubmit = jest.fn();
    
    render(
      <PasswordPromptModal
        {...defaultProps}
        onSubmit={onSubmit}
        showStrengthIndicator={true}
      />
    );

    const passwordInput = screen.getByPlaceholderText('Enter vault password');
    const submitButton = screen.getByRole('button', { name: /mount vault/i });

    await user.type(passwordInput, 'weak'); // Too short
    await user.click(submitButton);

    expect(onSubmit).not.toHaveBeenCalled();
    expect(submitButton).toBeDisabled();
  });

  test('should allow submission with valid password when strength checking enabled', async () => {
    const user = userEvent.setup();
    const onSubmit = jest.fn();
    
    render(
      <PasswordPromptModal
        {...defaultProps}
        onSubmit={onSubmit}
        showStrengthIndicator={true}
      />
    );

    const passwordInput = screen.getByPlaceholderText('Enter vault password');
    const submitButton = screen.getByRole('button', { name: /mount vault/i });

    await user.type(passwordInput, 'strongpassword'); // Valid length
    await user.click(submitButton);

    expect(onSubmit).toHaveBeenCalledWith('strongpassword');
  });

  test('should allow empty password when allowEmptyPassword is true', async () => {
    const user = userEvent.setup();
    const onSubmit = jest.fn();
    
    render(
      <PasswordPromptModal
        {...defaultProps}
        onSubmit={onSubmit}
        allowEmptyPassword={true}
      />
    );

    const submitButton = screen.getByRole('button', { name: /mount vault/i });
    await user.click(submitButton);

    expect(onSubmit).toHaveBeenCalledWith('');
  });

  test('should prevent submission with empty password by default', async () => {
    const user = userEvent.setup();
    const onSubmit = jest.fn();
    
    render(<PasswordPromptModal {...defaultProps} onSubmit={onSubmit} />);

    const submitButton = screen.getByRole('button', { name: /mount vault/i });
    await user.click(submitButton);

    expect(onSubmit).not.toHaveBeenCalled();
    expect(submitButton).toBeDisabled();
  });

  test('should show attempt warning after failed attempts', () => {
    render(<PasswordPromptModal {...defaultProps} />);

    // Simulate failed attempt by re-rendering with attempts
    // In real implementation, this would be managed by parent component
    const { rerender } = render(<PasswordPromptModal {...defaultProps} />);
    
    // This would require the component to track attempts internally
    // or receive attempts as a prop
  });

  test('should show security notice', () => {
    render(<PasswordPromptModal {...defaultProps} />);

    expect(screen.getByText('Your password is encrypted and never stored in plain text.')).toBeInTheDocument();
  });

  test('should handle password validation errors', async () => {
    const user = userEvent.setup();
    
    render(<PasswordPromptModal {...defaultProps} showStrengthIndicator={true} />);

    const passwordInput = screen.getByPlaceholderText('Enter vault password');
    await user.type(passwordInput, 'weak');

    expect(screen.getByText('Password too short')).toBeInTheDocument();
  });

  test('should not prevent submission when not loading and password is valid', async () => {
    const user = userEvent.setup();
    const onSubmit = jest.fn();
    
    render(<PasswordPromptModal {...defaultProps} onSubmit={onSubmit} />);

    const passwordInput = screen.getByPlaceholderText('Enter vault password');
    const submitButton = screen.getByRole('button', { name: /mount vault/i });

    await user.type(passwordInput, 'validpassword');
    
    expect(submitButton).not.toBeDisabled();
    
    await user.click(submitButton);
    expect(onSubmit).toHaveBeenCalledWith('validpassword');
  });

  test('should handle multiple failed attempts', () => {
    // This test would verify the component shows appropriate warnings
    // for multiple failed password attempts
    
    render(<PasswordPromptModal {...defaultProps} />);

    // In a real implementation, you might pass an attempts prop
    // or the component might track attempts internally
    
    // Verify appropriate warning messages are shown
    // based on the number of attempts
  });

  test('should apply custom className', () => {
    const { container } = render(
      <PasswordPromptModal {...defaultProps} className="custom-modal-class" />
    );

    expect(container.querySelector('.password-prompt-modal')).toHaveClass('custom-modal-class');
  });
});