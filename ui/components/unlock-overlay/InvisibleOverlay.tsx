/**
 * Invisible Unlock Overlay - PhantomVault 2.0
 * A transparent full-screen window for password input with no visual feedback
 */

import React, { useState, useEffect, useRef } from 'react';
import './InvisibleOverlay.scss';

export type UnlockMode = 'temporary' | 'permanent';

export interface PasswordInput {
  password: string;
  mode: UnlockMode;
  isRecoveryKey: boolean;
  isRelock?: boolean; // NEW: Flag to indicate re-lock operation
}

interface InvisibleOverlayProps {
  isVisible: boolean;
  isRecoveryMode?: boolean;
  isRelockMode?: boolean; // NEW: Flag for re-lock mode (skip T/P selection)
  temporaryCount?: number; // NEW: Number of temporary folders to re-lock
  onSubmit: (input: PasswordInput) => void;
  onCancel: () => void;
}

export const InvisibleOverlay: React.FC<InvisibleOverlayProps> = ({
  isVisible,
  isRecoveryMode = false,
  isRelockMode = false, // NEW: Default to false (normal unlock mode)
  temporaryCount = 0, // NEW: Default to 0
  onSubmit,
  onCancel,
}) => {
  const [passwordInput, setPasswordInput] = useState('');
  const passwordRef = useRef(''); // Keep ref in sync with state

  // Update ref whenever state changes
  useEffect(() => {
    passwordRef.current = passwordInput;
  }, [passwordInput]);

  // Window-level keydown listener for capturing keystrokes
  useEffect(() => {
    if (!isVisible) {
      console.log('👁️  InvisibleOverlay: Now HIDDEN');
      setPasswordInput(''); // Clear input when hidden
      return;
    }

    console.log('👁️  InvisibleOverlay: Now VISIBLE, setting up window keydown listener...', { 
      isRecoveryMode, 
      isRelockMode, 
      temporaryCount 
    });

    // Ignore inputs for the first 300ms to let user release the hotkey
    let isReady = false;
    const readyTimeout = setTimeout(() => {
      isReady = true;
      if (isRelockMode) {
        console.log(`✨ InvisibleOverlay: Ready to accept password for re-locking ${temporaryCount} folder(s)!`);
        console.log(`   (No T/P suffix needed in re-lock mode)`);
      } else {
        console.log('✨ InvisibleOverlay: Ready to accept input!');
        console.log(`   Format: T+password (temporary) or P+password (permanent)`);
      }
    }, 300);

    // Parse password input to extract mode (T/P PREFIX)
    // UNIFIED ARCHITECTURE: T/P + password at the START (e.g., "T1234" or "P1234")
    const parsePasswordInput = (input: string): PasswordInput => {
      // In re-lock mode, skip T/P parsing - just use the password as-is
      if (isRelockMode) {
        return {
          password: input,
          mode: 'temporary', // Not used in re-lock, but required by type
          isRecoveryKey: false,
          isRelock: true, // Flag to indicate this is a re-lock operation
        };
      }

      // Normal unlock mode: parse T/P PREFIX (first character)
      const firstChar = input.charAt(0).toUpperCase();
      
      if (firstChar === 'T' && input.length > 1) {
        return {
          password: input.slice(1), // Remove the 'T' prefix
          mode: 'temporary',
          isRecoveryKey: isRecoveryMode,
          isRelock: false,
        };
      } else if (firstChar === 'P' && input.length > 1) {
        return {
          password: input.slice(1), // Remove the 'P' prefix
          mode: 'permanent',
          isRecoveryKey: isRecoveryMode,
          isRelock: false,
        };
      } else {
        // No T/P prefix - default to temporary mode
        return {
          password: input,
          mode: 'temporary',
          isRecoveryKey: isRecoveryMode,
          isRelock: false,
        };
      }
    };

    const handleCancel = () => {
      console.log('❌ InvisibleOverlay: Cancel triggered');
      setPasswordInput('');
      passwordRef.current = '';
      onCancel();
    };

    const handleSubmit = () => {
      const currentPassword = passwordRef.current;
      console.log('📥 InvisibleOverlay: Submit triggered, input length:', currentPassword.length);
      
      if (!currentPassword.trim()) {
        console.log('❌ InvisibleOverlay: Empty input, canceling');
        handleCancel();
        return;
      }

      const parsedInput = parsePasswordInput(currentPassword);
      console.log('✅ InvisibleOverlay: Parsed input:', { 
        passwordLength: parsedInput.password.length, 
        mode: parsedInput.mode,
        isRecoveryKey: parsedInput.isRecoveryKey,
        isRelock: parsedInput.isRelock 
      });
      
      if (isRelockMode) {
        console.log(`🔒 Submitting re-lock request for ${temporaryCount} folder(s)`);
      }
      
      setPasswordInput('');
      passwordRef.current = '';
      onSubmit(parsedInput);
    };

    // Auto-timeout after 10 seconds
    const timeout = setTimeout(() => {
      console.log('⏰ InvisibleOverlay: Timeout reached, auto-canceling');
      handleCancel();
    }, 10000);

    // Capture keystrokes at window level
    const handleWindowKeyDown = (e: KeyboardEvent) => {
      // Ignore inputs until ready (300ms grace period)
      if (!isReady) {
        console.log('⏳ InvisibleOverlay: Not ready yet, ignoring key:', e.key);
        return;
      }

      const currentLength = passwordRef.current.length;
      console.log('⌨️  InvisibleOverlay: Window key pressed:', e.key, 'Code:', e.code, 'Ctrl:', e.ctrlKey, 'Alt:', e.altKey, 'Current length:', currentLength);
      
      // Ignore the hotkey combination itself
      if ((e.ctrlKey || e.metaKey) && e.altKey && (e.key === 'v' || e.key === 'V')) {
        console.log('🚫 Ignoring hotkey combination Ctrl+Alt+V');
        return;
      }
      
      if (e.key === 'Enter') {
        e.preventDefault();
        e.stopPropagation();
        console.log('↩️  InvisibleOverlay: Enter pressed, submitting...');
        handleSubmit();
      } else if (e.key === 'Escape') {
        e.preventDefault();
        e.stopPropagation();
        console.log('⎋  InvisibleOverlay: Escape pressed, canceling...');
        handleCancel();
      } else if (e.key === 'Backspace') {
        e.preventDefault();
        e.stopPropagation();
        setPasswordInput(prev => {
          const newValue = prev.slice(0, -1);
          console.log('⌫  InvisibleOverlay: Backspace, new length:', newValue.length);
          return newValue;
        });
      } else if (e.key.length === 1 && !e.ctrlKey && !e.metaKey && !e.altKey) {
        // Regular character (not a modifier key, not a shortcut)
        e.preventDefault();
        e.stopPropagation();
        setPasswordInput(prev => {
          const newValue = prev + e.key;
          console.log('✍️  InvisibleOverlay: Character added:', e.key, 'new length:', newValue.length);
          return newValue;
        });
      } else {
        console.log('🔇 Ignoring special key:', e.key, 'with modifiers');
      }
    };

    console.log('🎯 InvisibleOverlay: Adding window keydown listener...');
    window.addEventListener('keydown', handleWindowKeyDown, { capture: true });
    console.log('✅ InvisibleOverlay: Window keydown listener added successfully');

    return () => {
      console.log('🧹 InvisibleOverlay: Cleaning up, removing window keydown listener...');
      window.removeEventListener('keydown', handleWindowKeyDown, { capture: true });
      clearTimeout(timeout);
      clearTimeout(readyTimeout);
    };
  }, [isVisible, isRecoveryMode, onSubmit, onCancel]); // Removed passwordInput from deps to prevent re-registration on every keystroke

  if (!isVisible) {
    return null;
  }

  return (
    <div className="invisible-overlay">
      {/* Transparent backdrop - no visual feedback */}
      <div className="invisible-overlay__backdrop" onClick={() => onCancel()} />

      {/* Minimal hint (optional - can be removed for total invisibility) */}
      {process.env.NODE_ENV === 'development' && (
        <div className="invisible-overlay__dev-hint">
          <p>🔐 Password Input Active (Length: {passwordInput.length})</p>
          <p>
            {isRecoveryMode 
              ? 'Recovery Mode' 
              : isRelockMode 
                ? `Re-Lock Mode (${temporaryCount} folder${temporaryCount !== 1 ? 's' : ''})` 
                : 'Unlock Mode'}
          </p>
          {isRelockMode && <p>⚠️ No T/P suffix needed - password only</p>}
          {!isRelockMode && <p>💡 Add T or P prefix: T+password / P+password</p>}
          <p>Press ESC to cancel • Enter to submit</p>
          <p>Auto-close in 10 seconds</p>
        </div>
      )}
    </div>
  );
};
