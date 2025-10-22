/**
 * PhantomVault 2.0 - First-Time Setup Wizard
 * Guides user through creating their first vault profile with P/T mode education
 */

import React, { useState } from 'react';

type WizardStep = 'welcome' | 'profile' | 'password' | 'recovery' | 'tutorial';

interface SetupWizardProps {
  onComplete: () => void;
}

export const SetupWizard: React.FC<SetupWizardProps> = ({ onComplete }) => {
  const [step, setStep] = useState<WizardStep>('welcome');
  const [profileName, setProfileName] = useState('');
  const [masterPassword, setMasterPassword] = useState('');
  const [confirmPassword, setConfirmPassword] = useState('');
  const [recoveryKey, setRecoveryKey] = useState('');
  const [recoverySaved, setRecoverySaved] = useState(false);
  const [copiedMessage, setCopiedMessage] = useState(false);

  // Generate recovery key when moving to recovery step
  const generateRecoveryKey = () => {
    const segments = [];
    for (let i = 0; i < 4; i++) {
      const segment = Math.random().toString(36).substring(2, 6).toUpperCase();
      segments.push(segment);
    }
    return segments.join('-');
  };

  const handleNext = () => {
    if (step === 'welcome') {
      setStep('profile');
    } else if (step === 'profile') {
      if (profileName.trim()) {
        setStep('password');
      }
    } else if (step === 'password') {
      if (masterPassword && masterPassword === confirmPassword && masterPassword.length >= 4) {
        // Generate recovery key
        const key = generateRecoveryKey();
        setRecoveryKey(key);
        setStep('recovery');
      }
    } else if (step === 'recovery') {
      if (recoverySaved) {
        setStep('tutorial');
      }
    } else if (step === 'tutorial') {
      // Create the profile via IPC
      handleCreateProfile();
    }
  };

  const handleBack = () => {
    if (step === 'profile') setStep('welcome');
    else if (step === 'password') setStep('profile');
    else if (step === 'recovery') setStep('password');
    else if (step === 'tutorial') setStep('recovery');
  };

  const handleCreateProfile = async () => {
    try {
      console.log('Creating profile:', profileName);
      const response = await window.phantomVault.profile.create(
        profileName,
        masterPassword,
        recoveryKey
      );
      
      if (response.success && response.profile) {
        console.log('Profile created:', response.profile.id);
        // Set as active profile
        await window.phantomVault.profile.setActive(response.profile.id);
        // Complete setup
        onComplete();
      } else {
        throw new Error(response.error || 'Failed to create profile');
      }
    } catch (error: any) {
      console.error('Failed to create profile:', error);
      window.phantomVault.showNotification(
        'Setup Error',
        error.message || 'Failed to create vault profile'
      );
    }
  };

  const copyRecoveryKey = async () => {
    if (recoveryKey) {
      try {
        await navigator.clipboard.writeText(recoveryKey);
        setCopiedMessage(true);
        setTimeout(() => setCopiedMessage(false), 2000);
      } catch (err) {
        console.error('Failed to copy:', err);
        // Fallback for older browsers
        const textArea = document.createElement('textarea');
        textArea.value = recoveryKey;
        document.body.appendChild(textArea);
        textArea.select();
        document.execCommand('copy');
        document.body.removeChild(textArea);
        setCopiedMessage(true);
        setTimeout(() => setCopiedMessage(false), 2000);
      }
    }
  };

  const getStepNumber = (): number => {
    const steps = ['welcome', 'profile', 'password', 'recovery', 'tutorial'];
    return steps.indexOf(step) + 1;
  };

  const renderStep = () => {
    const stepStyles = {
      container: {
        minHeight: '400px',
        display: 'flex',
        flexDirection: 'column' as const,
        justifyContent: 'center',
      },
      icon: {
        fontSize: '4rem',
        textAlign: 'center' as const,
        marginBottom: '1.5rem',
      },
      title: {
        fontSize: '2rem',
        fontWeight: '700' as const,
        marginBottom: '1rem',
        textAlign: 'center' as const,
        color: '#F6F6F6',
      },
      description: {
        fontSize: '1rem',
        color: '#B4B4B4',
        textAlign: 'center' as const,
        marginBottom: '2rem',
        lineHeight: '1.6',
      },
      input: {
        width: '100%',
        padding: '0.75rem',
        backgroundColor: '#1B1F3B',
        border: '1px solid #424769',
        borderRadius: '8px',
        color: '#F6F6F6',
        fontSize: '1rem',
        outline: 'none',
        boxSizing: 'border-box' as const,
        marginBottom: '1rem',
      },
      button: {
        padding: '0.75rem 2rem',
        backgroundColor: '#7077A1',
        color: '#F6F6F6',
        border: 'none',
        borderRadius: '8px',
        fontSize: '1rem',
        fontWeight: '500' as const,
        cursor: 'pointer',
        transition: 'all 0.2s',
      },
    };

    switch (step) {
      case 'welcome':
        return (
          <div style={stepStyles.container}>
            <div style={stepStyles.icon}>🔐</div>
            <h1 style={stepStyles.title}>Welcome to PhantomVault 2.0</h1>
            <p style={stepStyles.description}>
              The next-generation secure folder management system with invisible unlock technology.
            </p>
            <ul style={{ listStyle: 'none', padding: 0, margin: '0 0 2rem', textAlign: 'left' }}>
              <li style={{ padding: '0.5rem 0', fontSize: '1rem', color: '#F6F6F6' }}>✅ Military-grade AES-256 encryption</li>
              <li style={{ padding: '0.5rem 0', fontSize: '1rem', color: '#F6F6F6' }}>✅ Invisible unlock mechanism</li>
              <li style={{ padding: '0.5rem 0', fontSize: '1rem', color: '#F6F6F6' }}>✅ Cross-platform (Linux, Windows, Tails OS)</li>
              <li style={{ padding: '0.5rem 0', fontSize: '1rem', color: '#F6F6F6' }}>✅ Recovery key backup system</li>
              <li style={{ padding: '0.5rem 0', fontSize: '1rem', color: '#F6F6F6' }}>✅ Multi-profile support</li>
            </ul>
          </div>
        );

      case 'profile':
        return (
          <div style={stepStyles.container}>
            <div style={stepStyles.icon}>👤</div>
            <h2 style={stepStyles.title}>Create Your Profile</h2>
            <p style={stepStyles.description}>
              Choose a name for your vault profile.<br />
              <small>You can create multiple profiles on shared computers.</small>
            </p>
            <div style={{ marginTop: '2rem' }}>
              <label style={{ display: 'block', marginBottom: '0.5rem', fontSize: '0.875rem', color: '#B4B4B4' }}>
                Profile Name
              </label>
              <input
                type="text"
                value={profileName}
                onChange={(e) => setProfileName(e.target.value)}
                placeholder="e.g., My Personal Vault"
                style={stepStyles.input}
                autoFocus
              />
            </div>
          </div>
        );

      case 'password':
        return (
          <div style={stepStyles.container}>
            <div style={stepStyles.icon}>🔑</div>
            <h2 style={stepStyles.title}>Set Master Password</h2>
            <p style={stepStyles.description}>
              Create a strong master password for your vault.<br />
              <small style={{ color: '#F44336' }}>⚠️ You'll need this to unlock your folders.</small>
            </p>
            <div style={{ marginTop: '2rem' }}>
              <label style={{ display: 'block', marginBottom: '0.5rem', fontSize: '0.875rem', color: '#B4B4B4' }}>
                Master Password
              </label>
              <input
                type="password"
                value={masterPassword}
                onChange={(e) => setMasterPassword(e.target.value)}
                placeholder="Enter strong password"
                style={stepStyles.input}
                autoFocus
              />
              {masterPassword && (
                <div style={{ fontSize: '0.875rem', marginBottom: '1rem' }}>
                  {masterPassword.length >= 4 ? (
                    <span style={{ color: '#4CAF50' }}>✓ Password accepted</span>
                  ) : (
                    <span style={{ color: '#F44336' }}>⚠ At least 4 characters required</span>
                  )}
                </div>
              )}
              
              <label style={{ display: 'block', marginBottom: '0.5rem', fontSize: '0.875rem', color: '#B4B4B4' }}>
                Confirm Password
              </label>
              <input
                type="password"
                value={confirmPassword}
                onChange={(e) => setConfirmPassword(e.target.value)}
                placeholder="Re-enter password"
                style={stepStyles.input}
              />
              {confirmPassword && (
                <div style={{ fontSize: '0.875rem' }}>
                  {masterPassword === confirmPassword ? (
                    <span style={{ color: '#4CAF50' }}>✓ Passwords match</span>
                  ) : (
                    <span style={{ color: '#F44336' }}>✗ Passwords don't match</span>
                  )}
                </div>
              )}
            </div>
          </div>
        );

      case 'recovery':
        return (
          <div style={stepStyles.container}>
            <div style={stepStyles.icon}>🛟</div>
            <h2 style={stepStyles.title}>Your Recovery Key</h2>
            <p style={stepStyles.description}>
              Save this recovery key in a secure location.<br />
              <small style={{ color: '#F44336' }}>
                ⚠️ You'll need this if you forget your master password!
              </small>
            </p>
            <div style={{
              backgroundColor: '#1B1F3B',
              border: '2px solid #7077A1',
              borderRadius: '8px',
              padding: '1.5rem',
              textAlign: 'center',
              marginTop: '2rem',
            }}>
              <div style={{
                fontSize: '1.5rem',
                fontWeight: '700',
                color: '#7077A1',
                letterSpacing: '0.1em',
                marginBottom: '1rem',
              }}>
                {recoveryKey}
              </div>
              <button
                onClick={copyRecoveryKey}
                style={{
                  ...stepStyles.button,
                  backgroundColor: copiedMessage ? '#4CAF50' : '#424769',
                }}
              >
                {copiedMessage ? '✓ Copied!' : '📋 Copy to Clipboard'}
              </button>
            </div>
            <div style={{ marginTop: '2rem' }}>
              <label style={{ display: 'flex', alignItems: 'center', cursor: 'pointer' }}>
                <input
                  type="checkbox"
                  checked={recoverySaved}
                  onChange={(e) => setRecoverySaved(e.target.checked)}
                  style={{ marginRight: '0.5rem', width: '20px', height: '20px' }}
                />
                <span style={{ fontSize: '0.875rem' }}>
                  I have saved my recovery key in a secure location
                </span>
              </label>
            </div>
          </div>
        );

      case 'tutorial':
        return (
          <div style={stepStyles.container}>
            <div style={stepStyles.icon}>🎓</div>
            <h2 style={stepStyles.title}>Unlock Modes Tutorial</h2>
            <p style={stepStyles.description}>
              When you unlock folders, you choose how long they stay unlocked:
            </p>
            <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '1rem', marginTop: '2rem' }}>
              <div style={{
                backgroundColor: '#1B1F3B',
                border: '1px solid #424769',
                borderRadius: '8px',
                padding: '1.5rem',
              }}>
                <div style={{ fontSize: '2rem', marginBottom: '1rem' }}>⏱️</div>
                <h3 style={{ fontSize: '1.25rem', marginBottom: '0.5rem' }}>Temporary (T)</h3>
                <p style={{ fontSize: '0.875rem', color: '#B4B4B4', marginBottom: '1rem' }}>
                  Add <strong>T</strong> before password:
                </p>
                <code style={{
                  display: 'block',
                  backgroundColor: '#424769',
                  padding: '0.5rem',
                  borderRadius: '4px',
                  marginBottom: '1rem',
                }}>
                  T1234
                </code>
                <ul style={{ fontSize: '0.75rem', color: '#B4B4B4', margin: 0, paddingLeft: '1.2rem' }}>
                  <li>Locks when device locks</li>
                  <li>Locks when app closes</li>
                  <li>Best for quick access</li>
                </ul>
              </div>
              
              <div style={{
                backgroundColor: '#1B1F3B',
                border: '1px solid #424769',
                borderRadius: '8px',
                padding: '1.5rem',
              }}>
                <div style={{ fontSize: '2rem', marginBottom: '1rem' }}>🔓</div>
                <h3 style={{ fontSize: '1.25rem', marginBottom: '0.5rem' }}>Permanent (P)</h3>
                <p style={{ fontSize: '0.875rem', color: '#B4B4B4', marginBottom: '1rem' }}>
                  Add <strong>P</strong> before password:
                </p>
                <code style={{
                  display: 'block',
                  backgroundColor: '#424769',
                  padding: '0.5rem',
                  borderRadius: '4px',
                  marginBottom: '1rem',
                }}>
                  P1234
                </code>
                <ul style={{ fontSize: '0.75rem', color: '#B4B4B4', margin: 0, paddingLeft: '1.2rem' }}>
                  <li>Stays unlocked forever</li>
                  <li>Removed from vault</li>
                  <li>Re-add to lock again</li>
                </ul>
              </div>
            </div>
            <div style={{
              marginTop: '1.5rem',
              padding: '1rem',
              backgroundColor: '#1B1F3B',
              border: '1px solid #7077A1',
              borderRadius: '8px',
              fontSize: '0.875rem',
            }}>
              <strong>💡 Pro Tip:</strong> If you forget to add T or P, it defaults to Temporary (T) for safety.
            </div>
          </div>
        );

      default:
        return null;
    }
  };

  const canProceed = () => {
    if (step === 'profile') return profileName.trim().length > 0;
    if (step === 'password') return masterPassword.length >= 4 && masterPassword === confirmPassword;
    if (step === 'recovery') return recoverySaved;
    return true;
  };

  return (
    <div style={{
      minHeight: '100vh',
      backgroundColor: '#1B1F3B',
      color: '#F6F6F6',
      display: 'flex',
      flexDirection: 'column',
      alignItems: 'center',
      justifyContent: 'center',
      padding: '2rem',
      fontFamily: 'Inter, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif',
    }}>
      <header style={{ textAlign: 'center', marginBottom: '3rem' }}>
        <div style={{
          width: '80px',
          height: '80px',
          backgroundColor: '#7077A1',
          borderRadius: '50%',
          margin: '0 auto 1rem',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          fontSize: '2rem',
        }}>
          🔐
        </div>
        <h1 style={{ fontSize: '2.5rem', margin: '0 0 0.5rem', fontWeight: '700' }}>
          PhantomVault 2.0
        </h1>
        <p style={{ fontSize: '1.125rem', color: '#B4B4B4', margin: 0 }}>
          Invisible Folder Security
        </p>
      </header>

      <main style={{ width: '100%', maxWidth: '700px' }}>
        {/* Progress dots */}
        <div style={{
          display: 'flex',
          gap: '0.5rem',
          justifyContent: 'center',
          marginBottom: '2rem',
        }}>
          {['welcome', 'profile', 'password', 'recovery', 'tutorial'].map((s, index) => (
            <div
              key={s}
              style={{
                width: '12px',
                height: '12px',
                borderRadius: '50%',
                backgroundColor: getStepNumber() === index + 1 ? '#7077A1' : 
                  getStepNumber() > index + 1 ? '#4CAF50' : '#424769',
                transition: 'all 0.3s',
              }}
            />
          ))}
        </div>

        {/* Step content */}
        <div style={{
          backgroundColor: '#2D3250',
          borderRadius: '12px',
          padding: '2.5rem',
          boxShadow: '0 10px 15px -3px rgba(0, 0, 0, 0.1)',
          minHeight: '500px',
        }}>
          {renderStep()}
        </div>
      </main>

      {/* Navigation buttons */}
      <nav style={{
        display: 'flex',
        gap: '1rem',
        marginTop: '2rem',
        width: '100%',
        maxWidth: '700px',
        justifyContent: step !== 'welcome' ? 'space-between' : 'flex-end',
      }}>
        {step !== 'welcome' && (
          <button
            onClick={handleBack}
            style={{
              padding: '0.75rem 2rem',
              backgroundColor: '#424769',
              color: '#F6F6F6',
              border: 'none',
              borderRadius: '8px',
              fontSize: '1rem',
              fontWeight: '500',
              cursor: 'pointer',
              transition: 'all 0.2s',
            }}
          >
            Back
          </button>
        )}
        <button
          onClick={handleNext}
          disabled={!canProceed()}
          style={{
            padding: '0.75rem 2rem',
            backgroundColor: canProceed() ? '#7077A1' : '#424769',
            color: '#F6F6F6',
            border: 'none',
            borderRadius: '8px',
            fontSize: '1rem',
            fontWeight: '500',
            cursor: canProceed() ? 'pointer' : 'not-allowed',
            transition: 'all 0.2s',
            opacity: canProceed() ? 1 : 0.5,
          }}
        >
          {step === 'tutorial' ? 'Complete Setup' : 'Next'}
        </button>
      </nav>
    </div>
  );
};