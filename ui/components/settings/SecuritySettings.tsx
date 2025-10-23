/**
 * Security Settings Component
 * 
 * Security-related preferences and authentication settings
 */

import React, { useState, useCallback } from 'react';
import { AppSettings } from '../../types';

export interface SecuritySettingsProps {
  settings: AppSettings;
  onSettingsChange: (changes: Partial<AppSettings>) => void;
}

/**
 * Security settings component
 */
export const SecuritySettings: React.FC<SecuritySettingsProps> = ({
  settings,
  onSettingsChange,
}) => {
  const [showPasswordDialog, setShowPasswordDialog] = useState(false);
  const [currentPassword, setCurrentPassword] = useState('');
  const [newPassword, setNewPassword] = useState('');
  const [confirmPassword, setConfirmPassword] = useState('');

  // Handle setting change
  const handleChange = useCallback((key: keyof AppSettings, value: any) => {
    onSettingsChange({ [key]: value });
  }, [onSettingsChange]);

  // Handle master password change
  const handlePasswordChange = useCallback(() => {
    if (newPassword !== confirmPassword) {
      alert('New passwords do not match');
      return;
    }
    
    if (newPassword.length < 8) {
      alert('Password must be at least 8 characters long');
      return;
    }
    
    // In a real implementation, this would update the master password
    console.log('Changing master password...');
    setShowPasswordDialog(false);
    setCurrentPassword('');
    setNewPassword('');
    setConfirmPassword('');
  }, [newPassword, confirmPassword]);

  return (
    <div className=\"security-settings\">
      <div className=\"settings-section\">
        <h2 className=\"section-title\">Authentication</h2>
        <p className=\"section-description\">
          Configure authentication and access control settings.
        </p>
        
        <div className=\"setting-group\">
          {/* Master Password */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Master password</label>
              <p className=\"setting-description\">
                Change the master password used to protect your vault configurations.
              </p>
            </div>
            <div className=\"setting-control\">
              <button 
                className=\"action-button secondary\"
                onClick={() => setShowPasswordDialog(true)}
              >
                Change Password
              </button>
            </div>
          </div>

          {/* Auto Lock Timeout (Security Context) */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Auto-lock timeout</label>
              <p className=\"setting-description\">
                Automatically lock the application after inactivity for security.
              </p>
            </div>
            <div className=\"setting-control\">
              <div className=\"number-input-group\">
                <input
                  type=\"number\"
                  min=\"1\"
                  max=\"120\"
                  value={settings.lockTimeout}
                  onChange={(e) => handleChange('lockTimeout', parseInt(e.target.value) || 15)}
                  className=\"number-input\"
                />
                <span className=\"input-suffix\">minutes</span>
              </div>
            </div>
          </div>

          {/* Require Password for Operations */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Require password confirmation</label>
              <p className=\"setting-description\">
                Require password confirmation for sensitive operations like vault deletion.
              </p>
            </div>
            <div className=\"setting-control\">
              <label className=\"toggle-switch\">
                <input
                  type=\"checkbox\"
                  defaultChecked={true}
                  onChange={(e) => {
                    // Handle password confirmation requirement
                    console.log('Password confirmation required:', e.target.checked);
                  }}
                />
                <span className=\"toggle-slider\"></span>
              </label>
            </div>
          </div>
        </div>
      </div>

      <div className=\"settings-section\">
        <h2 className=\"section-title\">Vault Security</h2>
        <p className=\"section-description\">
          Configure security settings for vault operations and data protection.
        </p>
        
        <div className=\"setting-group\">
          {/* Default Encryption Level */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Default encryption level</label>
              <p className=\"setting-description\">
                Choose the default encryption strength for new vaults.
              </p>
            </div>
            <div className=\"setting-control\">
              <select 
                className=\"select-input\"
                defaultValue=\"high\"
                onChange={(e) => {
                  console.log('Default encryption level:', e.target.value);
                }}
              >
                <option value=\"standard\">Standard (AES-256)</option>
                <option value=\"high\">High (AES-256 + Additional Layers)</option>
              </select>
            </div>
          </div>

          {/* Auto-unmount Timeout */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Auto-unmount timeout</label>
              <p className=\"setting-description\">
                Automatically unmount vaults after a period of inactivity.
              </p>
            </div>
            <div className=\"setting-control\">
              <div className=\"number-input-group\">
                <input
                  type=\"number\"
                  min=\"5\"
                  max=\"480\"
                  defaultValue={30}
                  className=\"number-input\"
                  onChange={(e) => {
                    console.log('Auto-unmount timeout:', e.target.value);
                  }}
                />
                <span className=\"input-suffix\">minutes</span>
              </div>
            </div>
          </div>

          {/* Clear Clipboard */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Clear clipboard timeout</label>
              <p className=\"setting-description\">
                Automatically clear clipboard contents after copying sensitive data.
              </p>
            </div>
            <div className=\"setting-control\">
              <div className=\"number-input-group\">
                <input
                  type=\"number\"
                  min=\"10\"
                  max=\"300\"
                  defaultValue={30}
                  className=\"number-input\"
                  onChange={(e) => {
                    console.log('Clipboard timeout:', e.target.value);
                  }}
                />
                <span className=\"input-suffix\">seconds</span>
              </div>
            </div>
          </div>
        </div>
      </div>

      <div className=\"settings-section\">
        <h2 className=\"section-title\">Keyboard Sequences</h2>
        <p className=\"section-description\">
          Configure global keyboard sequence detection and security.
        </p>
        
        <div className=\"setting-group\">
          {/* Global Sequence Detection */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Enable global sequence detection</label>
              <p className=\"setting-description\">
                Allow PhantomVault to detect keyboard sequences system-wide.
              </p>
            </div>
            <div className=\"setting-control\">
              <label className=\"toggle-switch\">
                <input
                  type=\"checkbox\"
                  defaultChecked={true}
                  onChange={(e) => {
                    console.log('Global sequence detection:', e.target.checked);
                  }}
                />
                <span className=\"toggle-slider\"></span>
              </label>
            </div>
          </div>

          {/* Sequence Timeout */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Sequence input timeout</label>
              <p className=\"setting-description\">
                Maximum time allowed between keystrokes in a sequence.
              </p>
            </div>
            <div className=\"setting-control\">
              <div className=\"number-input-group\">
                <input
                  type=\"number\"
                  min=\"1\"
                  max=\"10\"
                  defaultValue={3}
                  className=\"number-input\"
                  onChange={(e) => {
                    console.log('Sequence timeout:', e.target.value);
                  }}
                />
                <span className=\"input-suffix\">seconds</span>
              </div>
            </div>
          </div>

          {/* Sequence Privacy */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Hide sequences in logs</label>
              <p className=\"setting-description\">
                Prevent keyboard sequences from appearing in activity logs.
              </p>
            </div>
            <div className=\"setting-control\">
              <label className=\"toggle-switch\">
                <input
                  type=\"checkbox\"
                  defaultChecked={true}
                  onChange={(e) => {
                    console.log('Hide sequences in logs:', e.target.checked);
                  }}
                />
                <span className=\"toggle-slider\"></span>
              </label>
            </div>
          </div>
        </div>
      </div>

      <div className=\"settings-section\">
        <h2 className=\"section-title\">Security Monitoring</h2>
        <p className=\"section-description\">
          Configure security monitoring and alert preferences.
        </p>
        
        <div className=\"setting-group\">
          {/* Failed Attempt Monitoring */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Monitor failed access attempts</label>
              <p className=\"setting-description\">
                Track and alert on repeated failed authentication attempts.
              </p>
            </div>
            <div className=\"setting-control\">
              <label className=\"toggle-switch\">
                <input
                  type=\"checkbox\"
                  defaultChecked={true}
                  onChange={(e) => {
                    console.log('Monitor failed attempts:', e.target.checked);
                  }}
                />
                <span className=\"toggle-slider\"></span>
              </label>
            </div>
          </div>

          {/* Security Alerts */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Security alert notifications</label>
              <p className=\"setting-description\">
                Show notifications for security-related events and warnings.
              </p>
            </div>
            <div className=\"setting-control\">
              <label className=\"toggle-switch\">
                <input
                  type=\"checkbox\"
                  defaultChecked={true}
                  onChange={(e) => {
                    console.log('Security alerts:', e.target.checked);
                  }}
                />
                <span className=\"toggle-slider\"></span>
              </label>
            </div>
          </div>

          {/* Audit Log */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Enable security audit log</label>
              <p className=\"setting-description\">
                Maintain a detailed log of all security-related operations.
              </p>
            </div>
            <div className=\"setting-control\">
              <label className=\"toggle-switch\">
                <input
                  type=\"checkbox\"
                  defaultChecked={true}
                  onChange={(e) => {
                    console.log('Security audit log:', e.target.checked);
                  }}
                />
                <span className=\"toggle-slider\"></span>
              </label>
            </div>
          </div>
        </div>
      </div>

      {/* Password Change Dialog */}
      {showPasswordDialog && (
        <div className=\"modal-overlay\">
          <div className=\"password-dialog\">
            <div className=\"dialog-header\">
              <h3>Change Master Password</h3>
              <button 
                onClick={() => setShowPasswordDialog(false)}
                className=\"close-button\"
              >
                ✕
              </button>
            </div>
            
            <div className=\"dialog-content\">
              <div className=\"input-group\">
                <label>Current Password</label>
                <input
                  type=\"password\"
                  value={currentPassword}
                  onChange={(e) => setCurrentPassword(e.target.value)}
                  className=\"password-input\"
                />
              </div>
              
              <div className=\"input-group\">
                <label>New Password</label>
                <input
                  type=\"password\"
                  value={newPassword}
                  onChange={(e) => setNewPassword(e.target.value)}
                  className=\"password-input\"
                />
              </div>
              
              <div className=\"input-group\">
                <label>Confirm New Password</label>
                <input
                  type=\"password\"
                  value={confirmPassword}
                  onChange={(e) => setConfirmPassword(e.target.value)}
                  className=\"password-input\"
                />
              </div>
            </div>
            
            <div className=\"dialog-actions\">
              <button 
                onClick={() => setShowPasswordDialog(false)}
                className=\"action-button secondary\"
              >
                Cancel
              </button>
              <button 
                onClick={handlePasswordChange}
                className=\"action-button primary\"
                disabled={!currentPassword || !newPassword || !confirmPassword}
              >
                Change Password
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};