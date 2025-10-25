/**
 * Settings View Component
 */

import React, { useState } from 'react';

type SettingsTab = 'general' | 'security' | 'hotkeys' | 'advanced';

export const Settings: React.FC = () => {
  const [activeTab, setActiveTab] = useState<SettingsTab>('general');
  const [settings, setSettings] = useState({
    theme: 'dark',
    autoStart: true,
    notifications: true,
    autoLock: true,
    lockTimeout: 300,
    hotkeyUnlock: 'Ctrl+Alt+V',
    hotkeyRecovery: 'Ctrl+Alt+R',
    encryptionLevel: 'AES-256-GCM',
    keyDerivation: 'PBKDF2',
    iterations: 100000
  });

  const tabs = [
    { id: 'general' as SettingsTab, label: 'General', icon: '⚙️' },
    { id: 'security' as SettingsTab, label: 'Security', icon: '🔒' },
    { id: 'hotkeys' as SettingsTab, label: 'Hotkeys', icon: '⌨️' },
    { id: 'advanced' as SettingsTab, label: 'Advanced', icon: '🔧' }
  ];

  const handleSettingChange = (key: string, value: any) => {
    setSettings(prev => ({ ...prev, [key]: value }));
  };

  const renderGeneralSettings = () => (
    <div style={{ padding: '1.5rem' }}>
      <h3 style={{ margin: '0 0 1.5rem 0', color: 'var(--color-text-primary)' }}>General Preferences</h3>
      
      <div style={{ display: 'flex', flexDirection: 'column', gap: '1.5rem' }}>
        <div>
          <label style={{ display: 'block', marginBottom: '0.5rem', color: 'var(--color-text-primary)', fontWeight: '500' }}>
            Theme
          </label>
          <select 
            value={settings.theme}
            onChange={(e) => handleSettingChange('theme', e.target.value)}
            style={{
              padding: '0.75rem',
              border: '1px solid var(--color-border)',
              borderRadius: '4px',
              background: 'var(--color-bg-primary)',
              color: 'var(--color-text-primary)',
              width: '200px'
            }}
          >
            <option value="light">Light</option>
            <option value="dark">Dark</option>
            <option value="auto">Auto (System)</option>
          </select>
        </div>

        <div style={{ display: 'flex', alignItems: 'center', gap: '0.75rem' }}>
          <input 
            type="checkbox" 
            id="autoStart"
            checked={settings.autoStart}
            onChange={(e) => handleSettingChange('autoStart', e.target.checked)}
            style={{ width: '16px', height: '16px' }}
          />
          <label htmlFor="autoStart" style={{ color: 'var(--color-text-primary)', fontWeight: '500' }}>
            Start PhantomVault on system startup
          </label>
        </div>

        <div style={{ display: 'flex', alignItems: 'center', gap: '0.75rem' }}>
          <input 
            type="checkbox" 
            id="notifications"
            checked={settings.notifications}
            onChange={(e) => handleSettingChange('notifications', e.target.checked)}
            style={{ width: '16px', height: '16px' }}
          />
          <label htmlFor="notifications" style={{ color: 'var(--color-text-primary)', fontWeight: '500' }}>
            Show desktop notifications
          </label>
        </div>
      </div>
    </div>
  );

  const renderSecuritySettings = () => (
    <div style={{ padding: '1.5rem' }}>
      <h3 style={{ margin: '0 0 1.5rem 0', color: 'var(--color-text-primary)' }}>Security Settings</h3>
      
      <div style={{ display: 'flex', flexDirection: 'column', gap: '1.5rem' }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: '0.75rem' }}>
          <input 
            type="checkbox" 
            id="autoLock"
            checked={settings.autoLock}
            onChange={(e) => handleSettingChange('autoLock', e.target.checked)}
            style={{ width: '16px', height: '16px' }}
          />
          <label htmlFor="autoLock" style={{ color: 'var(--color-text-primary)', fontWeight: '500' }}>
            Auto-lock vaults when system locks
          </label>
        </div>

        <div>
          <label style={{ display: 'block', marginBottom: '0.5rem', color: 'var(--color-text-primary)', fontWeight: '500' }}>
            Lock Timeout (seconds)
          </label>
          <input 
            type="number"
            value={settings.lockTimeout}
            onChange={(e) => handleSettingChange('lockTimeout', parseInt(e.target.value))}
            min="60"
            max="3600"
            style={{
              padding: '0.75rem',
              border: '1px solid var(--color-border)',
              borderRadius: '4px',
              background: 'var(--color-bg-primary)',
              color: 'var(--color-text-primary)',
              width: '200px'
            }}
          />
        </div>

        <div>
          <label style={{ display: 'block', marginBottom: '0.5rem', color: 'var(--color-text-primary)', fontWeight: '500' }}>
            Encryption Algorithm
          </label>
          <select 
            value={settings.encryptionLevel}
            onChange={(e) => handleSettingChange('encryptionLevel', e.target.value)}
            style={{
              padding: '0.75rem',
              border: '1px solid var(--color-border)',
              borderRadius: '4px',
              background: 'var(--color-bg-primary)',
              color: 'var(--color-text-primary)',
              width: '200px'
            }}
          >
            <option value="AES-256-GCM">AES-256-GCM (Recommended)</option>
            <option value="AES-256-CBC">AES-256-CBC</option>
            <option value="ChaCha20-Poly1305">ChaCha20-Poly1305</option>
          </select>
        </div>
      </div>
    </div>
  );

  const renderHotkeySettings = () => (
    <div style={{ padding: '1.5rem' }}>
      <h3 style={{ margin: '0 0 1.5rem 0', color: 'var(--color-text-primary)' }}>Hotkey Configuration</h3>
      
      <div style={{ display: 'flex', flexDirection: 'column', gap: '1.5rem' }}>
        <div>
          <label style={{ display: 'block', marginBottom: '0.5rem', color: 'var(--color-text-primary)', fontWeight: '500' }}>
            Unlock/Lock Hotkey
          </label>
          <input 
            type="text"
            value={settings.hotkeyUnlock}
            onChange={(e) => handleSettingChange('hotkeyUnlock', e.target.value)}
            placeholder="Ctrl+Alt+V"
            style={{
              padding: '0.75rem',
              border: '1px solid var(--color-border)',
              borderRadius: '4px',
              background: 'var(--color-bg-primary)',
              color: 'var(--color-text-primary)',
              width: '200px'
            }}
          />
          <p style={{ margin: '0.5rem 0 0 0', fontSize: '0.75rem', color: 'var(--color-text-secondary)' }}>
            Global hotkey to activate invisible password detection
          </p>
        </div>

        <div>
          <label style={{ display: 'block', marginBottom: '0.5rem', color: 'var(--color-text-primary)', fontWeight: '500' }}>
            Recovery Hotkey
          </label>
          <input 
            type="text"
            value={settings.hotkeyRecovery}
            onChange={(e) => handleSettingChange('hotkeyRecovery', e.target.value)}
            placeholder="Ctrl+Alt+R"
            style={{
              padding: '0.75rem',
              border: '1px solid var(--color-border)',
              borderRadius: '4px',
              background: 'var(--color-bg-primary)',
              color: 'var(--color-text-primary)',
              width: '200px'
            }}
          />
          <p style={{ margin: '0.5rem 0 0 0', fontSize: '0.75rem', color: 'var(--color-text-secondary)' }}>
            Emergency recovery key input hotkey
          </p>
        </div>

        <div style={{
          padding: '1rem',
          backgroundColor: 'rgba(33, 150, 243, 0.1)',
          border: '1px solid var(--color-info)',
          borderRadius: '4px'
        }}>
          <h4 style={{ margin: '0 0 0.5rem 0', color: 'var(--color-info)' }}>💡 How it works</h4>
          <p style={{ margin: 0, fontSize: '0.875rem', color: 'var(--color-text-secondary)' }}>
            Press the unlock hotkey, then type your password anywhere on your system. 
            Use T+password for temporary unlock or P+password for permanent unlock.
          </p>
        </div>
      </div>
    </div>
  );

  const renderAdvancedSettings = () => (
    <div style={{ padding: '1.5rem' }}>
      <h3 style={{ margin: '0 0 1.5rem 0', color: 'var(--color-text-primary)' }}>Advanced Configuration</h3>
      
      <div style={{ display: 'flex', flexDirection: 'column', gap: '1.5rem' }}>
        <div>
          <label style={{ display: 'block', marginBottom: '0.5rem', color: 'var(--color-text-primary)', fontWeight: '500' }}>
            Key Derivation Iterations
          </label>
          <input 
            type="number"
            value={settings.iterations}
            onChange={(e) => handleSettingChange('iterations', parseInt(e.target.value))}
            min="10000"
            max="1000000"
            step="10000"
            style={{
              padding: '0.75rem',
              border: '1px solid var(--color-border)',
              borderRadius: '4px',
              background: 'var(--color-bg-primary)',
              color: 'var(--color-text-primary)',
              width: '200px'
            }}
          />
          <p style={{ margin: '0.5rem 0 0 0', fontSize: '0.75rem', color: 'var(--color-text-secondary)' }}>
            Higher values increase security but slow down unlock operations
          </p>
        </div>

        <div style={{
          padding: '1rem',
          backgroundColor: 'rgba(255, 152, 0, 0.1)',
          border: '1px solid var(--color-warning)',
          borderRadius: '4px'
        }}>
          <h4 style={{ margin: '0 0 0.5rem 0', color: 'var(--color-warning)' }}>⚠️ Warning</h4>
          <p style={{ margin: 0, fontSize: '0.875rem', color: 'var(--color-text-secondary)' }}>
            Changing advanced settings may affect performance and compatibility. 
            Only modify these if you understand the implications.
          </p>
        </div>
      </div>
    </div>
  );

  const renderTabContent = () => {
    switch (activeTab) {
      case 'general': return renderGeneralSettings();
      case 'security': return renderSecuritySettings();
      case 'hotkeys': return renderHotkeySettings();
      case 'advanced': return renderAdvancedSettings();
      default: return renderGeneralSettings();
    }
  };

  return (
    <div style={{ padding: '2rem', height: '100%' }}>
      <h1 style={{ margin: '0 0 2rem 0', color: 'var(--color-text-primary)' }}>⚙️ Settings</h1>
      
      <div style={{ display: 'flex', gap: '2rem', height: 'calc(100% - 100px)' }}>
        {/* Sidebar */}
        <div style={{
          width: '200px',
          border: '1px solid var(--color-border)',
          borderRadius: '8px',
          backgroundColor: 'var(--color-bg-elevated)',
          padding: '1rem 0'
        }}>
          {tabs.map(tab => (
            <button
              key={tab.id}
              onClick={() => setActiveTab(tab.id)}
              style={{
                width: '100%',
                padding: '0.75rem 1rem',
                border: 'none',
                background: activeTab === tab.id ? 'var(--color-primary)' : 'transparent',
                color: activeTab === tab.id ? 'white' : 'var(--color-text-primary)',
                textAlign: 'left',
                cursor: 'pointer',
                display: 'flex',
                alignItems: 'center',
                gap: '0.75rem',
                fontSize: '0.875rem',
                fontWeight: '500',
                transition: 'all 0.2s ease'
              }}
            >
              <span>{tab.icon}</span>
              {tab.label}
            </button>
          ))}
        </div>

        {/* Content */}
        <div style={{
          flex: 1,
          border: '1px solid var(--color-border)',
          borderRadius: '8px',
          backgroundColor: 'var(--color-bg-elevated)',
          overflow: 'auto'
        }}>
          {renderTabContent()}
        </div>
      </div>

      {/* Save Button */}
      <div style={{ 
        marginTop: '2rem', 
        padding: '1rem 0', 
        borderTop: '1px solid var(--color-border)',
        display: 'flex',
        justifyContent: 'flex-end',
        gap: '1rem'
      }}>
        <button style={{
          padding: '0.75rem 1.5rem',
          backgroundColor: 'transparent',
          color: 'var(--color-text-secondary)',
          border: '1px solid var(--color-border)',
          borderRadius: '4px',
          cursor: 'pointer'
        }}>
          Reset to Defaults
        </button>
        <button style={{
          padding: '0.75rem 1.5rem',
          backgroundColor: 'var(--color-primary)',
          color: 'white',
          border: 'none',
          borderRadius: '4px',
          cursor: 'pointer',
          fontWeight: '500'
        }}>
          💾 Save Settings
        </button>
      </div>
    </div>
  );
};