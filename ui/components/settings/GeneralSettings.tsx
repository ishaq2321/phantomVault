/**
 * General Settings Component
 * 
 * Basic application preferences and behavior settings
 */

import React, { useCallback } from 'react';
import { AppSettings } from '../../types';

export interface GeneralSettingsProps {
  settings: AppSettings;
  onSettingsChange: (changes: Partial<AppSettings>) => void;
}

/**
 * General settings component
 */
export const GeneralSettings: React.FC<GeneralSettingsProps> = ({
  settings,
  onSettingsChange,
}) => {
  // Handle setting change
  const handleChange = useCallback((key: keyof AppSettings, value: any) => {
    onSettingsChange({ [key]: value });
  }, [onSettingsChange]);

  return (
    <div className=\"general-settings\">
      <div className=\"settings-section\">
        <h2 className=\"section-title\">Startup & System</h2>
        <p className=\"section-description\">
          Configure how PhantomVault behaves when starting up and interacting with your system.
        </p>
        
        <div className=\"setting-group\">
          {/* Auto Start */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Auto-start with system</label>
              <p className=\"setting-description\">
                Automatically launch PhantomVault when your computer starts up.
              </p>
            </div>
            <div className=\"setting-control\">
              <label className=\"toggle-switch\">
                <input
                  type=\"checkbox\"
                  checked={settings.autoStart}
                  onChange={(e) => handleChange('autoStart', e.target.checked)}
                />
                <span className=\"toggle-slider\"></span>
              </label>
            </div>
          </div>

          {/* Minimize to Tray */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Minimize to system tray</label>
              <p className=\"setting-description\">
                Keep PhantomVault running in the background when minimized.
              </p>
            </div>
            <div className=\"setting-control\">
              <label className=\"toggle-switch\">
                <input
                  type=\"checkbox\"
                  checked={settings.minimizeToTray}
                  onChange={(e) => handleChange('minimizeToTray', e.target.checked)}
                />
                <span className=\"toggle-slider\"></span>
              </label>
            </div>
          </div>

          {/* Notifications */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Show notifications</label>
              <p className=\"setting-description\">
                Display system notifications for vault operations and important events.
              </p>
            </div>
            <div className=\"setting-control\">
              <label className=\"toggle-switch\">
                <input
                  type=\"checkbox\"
                  checked={settings.notifications}
                  onChange={(e) => handleChange('notifications', e.target.checked)}
                />
                <span className=\"toggle-slider\"></span>
              </label>
            </div>
          </div>
        </div>
      </div>

      <div className=\"settings-section\">
        <h2 className=\"section-title\">Application Behavior</h2>
        <p className=\"section-description\">
          Customize how the application behaves during normal operation.
        </p>
        
        <div className=\"setting-group\">
          {/* Auto Lock */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Auto-lock application</label>
              <p className=\"setting-description\">
                Automatically lock the application after a period of inactivity.
              </p>
            </div>
            <div className=\"setting-control\">
              <label className=\"toggle-switch\">
                <input
                  type=\"checkbox\"
                  checked={settings.autoLock}
                  onChange={(e) => handleChange('autoLock', e.target.checked)}
                />
                <span className=\"toggle-slider\"></span>
              </label>
            </div>
          </div>

          {/* Lock Timeout */}
          {settings.autoLock && (
            <div className=\"setting-item nested\">
              <div className=\"setting-info\">
                <label className=\"setting-label\">Lock timeout</label>
                <p className=\"setting-description\">
                  Time in minutes before the application automatically locks.
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
                <div className=\"setting-range\">
                  <input
                    type=\"range\"
                    min=\"1\"
                    max=\"120\"
                    value={settings.lockTimeout}
                    onChange={(e) => handleChange('lockTimeout', parseInt(e.target.value))}
                    className=\"range-input\"
                  />
                  <div className=\"range-labels\">
                    <span>1 min</span>
                    <span>2 hours</span>
                  </div>
                </div>
              </div>
            </div>
          )}
        </div>
      </div>

      <div className=\"settings-section\">
        <h2 className=\"section-title\">Performance & Resources</h2>
        <p className=\"section-description\">
          Optimize application performance and resource usage.
        </p>
        
        <div className=\"setting-group\">
          {/* Memory Usage Info */}
          <div className=\"setting-item info-only\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Memory usage</label>
              <p className=\"setting-description\">
                Current application memory usage and optimization status.
              </p>
            </div>
            <div className=\"setting-control\">
              <div className=\"info-display\">
                <span className=\"info-value\">~45 MB</span>
                <span className=\"info-status good\">Optimized</span>
              </div>
            </div>
          </div>

          {/* Cache Management */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Clear application cache</label>
              <p className=\"setting-description\">
                Clear temporary files and cached data to free up space.
              </p>
            </div>
            <div className=\"setting-control\">
              <button 
                className=\"action-button secondary\"
                onClick={() => {
                  // In a real implementation, this would clear the cache
                  console.log('Clearing cache...');
                }}
              >
                Clear Cache
              </button>
            </div>
          </div>
        </div>
      </div>

      <div className=\"settings-section\">
        <h2 className=\"section-title\">Data & Privacy</h2>
        <p className=\"section-description\">
          Manage your data and privacy preferences.
        </p>
        
        <div className=\"setting-group\">
          {/* Usage Analytics */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Anonymous usage analytics</label>
              <p className=\"setting-description\">
                Help improve PhantomVault by sharing anonymous usage statistics.
              </p>
            </div>
            <div className=\"setting-control\">
              <label className=\"toggle-switch\">
                <input
                  type=\"checkbox\"
                  defaultChecked={false}
                  onChange={(e) => {
                    // Handle analytics preference
                    console.log('Analytics:', e.target.checked);
                  }}
                />
                <span className=\"toggle-slider\"></span>
              </label>
            </div>
          </div>

          {/* Error Reporting */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Automatic error reporting</label>
              <p className=\"setting-description\">
                Automatically send error reports to help diagnose and fix issues.
              </p>
            </div>
            <div className=\"setting-control\">
              <label className=\"toggle-switch\">
                <input
                  type=\"checkbox\"
                  defaultChecked={true}
                  onChange={(e) => {
                    // Handle error reporting preference
                    console.log('Error reporting:', e.target.checked);
                  }}
                />
                <span className=\"toggle-slider\"></span>
              </label>
            </div>
          </div>

          {/* Data Export */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Export application data</label>
              <p className=\"setting-description\">
                Export your vault configurations and settings for backup or migration.
              </p>
            </div>
            <div className=\"setting-control\">
              <button 
                className=\"action-button secondary\"
                onClick={() => {
                  // In a real implementation, this would export data
                  console.log('Exporting data...');
                }}
              >
                Export Data
              </button>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};