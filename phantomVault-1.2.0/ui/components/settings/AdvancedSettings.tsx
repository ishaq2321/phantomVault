/**
 * Advanced Settings Component
 * 
 * Advanced configuration and debugging options
 */

import React, { useState, useCallback } from 'react';
import { AppSettings } from '../../types';

export interface AdvancedSettingsProps {
  settings: AppSettings;
  onSettingsChange: (changes: Partial<AppSettings>) => void;
}

/**
 * Advanced settings component
 */
export const AdvancedSettings: React.FC<AdvancedSettingsProps> = ({
  settings,
  onSettingsChange,
}) => {
  const [showDangerZone, setShowDangerZone] = useState(false);
  const [debugInfo, setDebugInfo] = useState<any>(null);

  // Handle setting change
  const handleChange = useCallback((key: keyof AppSettings, value: any) => {
    onSettingsChange({ [key]: value });
  }, [onSettingsChange]);

  // Collect debug information
  const collectDebugInfo = useCallback(() => {
    const info = {
      timestamp: new Date().toISOString(),
      version: '1.0.0', // In real app, get from package.json
      platform: navigator.platform,
      userAgent: navigator.userAgent,
      memory: (performance as any).memory ? {
        used: Math.round((performance as any).memory.usedJSHeapSize / 1024 / 1024),
        total: Math.round((performance as any).memory.totalJSHeapSize / 1024 / 1024),
        limit: Math.round((performance as any).memory.jsHeapSizeLimit / 1024 / 1024),
      } : 'Not available',
      settings: settings,
      localStorage: Object.keys(localStorage).length,
    };
    
    setDebugInfo(info);
  }, [settings]);

  // Export debug info
  const exportDebugInfo = useCallback(() => {
    if (!debugInfo) return;
    
    const blob = new Blob([JSON.stringify(debugInfo, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `phantomvault-debug-${new Date().toISOString().split('T')[0]}.json`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  }, [debugInfo]);

  return (
    <div className=\"advanced-settings\">
      <div className=\"settings-section\">
        <h2 className=\"section-title\">Performance & Debugging</h2>
        <p className=\"section-description\">
          Advanced performance tuning and debugging options.
        </p>
        
        <div className=\"setting-group\">
          {/* Debug Mode */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Debug mode</label>
              <p className=\"setting-description\">
                Enable detailed logging and debugging features for troubleshooting.
              </p>
            </div>
            <div className=\"setting-control\">
              <label className=\"toggle-switch\">
                <input
                  type=\"checkbox\"
                  defaultChecked={false}
                  onChange={(e) => {
                    console.log('Debug mode:', e.target.checked);
                  }}
                />
                <span className=\"toggle-slider\"></span>
              </label>
            </div>
          </div>

          {/* Verbose Logging */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Verbose logging</label>
              <p className=\"setting-description\">
                Include detailed information in log entries for debugging purposes.
              </p>
            </div>
            <div className=\"setting-control\">
              <label className=\"toggle-switch\">
                <input
                  type=\"checkbox\"
                  defaultChecked={false}
                  onChange={(e) => {
                    console.log('Verbose logging:', e.target.checked);
                  }}
                />
                <span className=\"toggle-slider\"></span>
              </label>
            </div>
          </div>

          {/* Performance Monitoring */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Performance monitoring</label>
              <p className=\"setting-description\">
                Monitor and log performance metrics for optimization.
              </p>
            </div>
            <div className=\"setting-control\">
              <label className=\"toggle-switch\">
                <input
                  type=\"checkbox\"
                  defaultChecked={false}
                  onChange={(e) => {
                    console.log('Performance monitoring:', e.target.checked);
                  }}
                />
                <span className=\"toggle-slider\"></span>
              </label>
            </div>
          </div>

          {/* Memory Usage Display */}
          <div className=\"setting-item info-only\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Memory usage</label>
              <p className=\"setting-description\">
                Current memory usage and performance statistics.
              </p>
            </div>
            <div className=\"setting-control\">
              <div className=\"info-display\">
                <div className=\"memory-stats\">
                  <div className=\"stat-item\">
                    <span className=\"stat-label\">Used:</span>
                    <span className=\"stat-value\">45 MB</span>
                  </div>
                  <div className=\"stat-item\">
                    <span className=\"stat-label\">Available:</span>
                    <span className=\"stat-value\">128 MB</span>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>

      <div className=\"settings-section\">
        <h2 className=\"section-title\">Service Integration</h2>
        <p className=\"section-description\">
          Configure integration with the PhantomVault service backend.
        </p>
        
        <div className=\"setting-group\">
          {/* Service Connection Timeout */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Service connection timeout</label>
              <p className=\"setting-description\">
                Maximum time to wait for service responses before timing out.
              </p>
            </div>
            <div className=\"setting-control\">
              <div className=\"number-input-group\">
                <input
                  type=\"number\"
                  min=\"5\"
                  max=\"60\"
                  defaultValue={10}
                  className=\"number-input\"
                  onChange={(e) => {
                    console.log('Service timeout:', e.target.value);
                  }}
                />
                <span className=\"input-suffix\">seconds</span>
              </div>
            </div>
          </div>

          {/* Retry Attempts */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Connection retry attempts</label>
              <p className=\"setting-description\">
                Number of times to retry failed service connections.
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
                    console.log('Retry attempts:', e.target.value);
                  }}
                />
                <span className=\"input-suffix\">attempts</span>
              </div>
            </div>
          </div>

          {/* Service Status */}
          <div className=\"setting-item info-only\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Service status</label>
              <p className=\"setting-description\">
                Current status of the PhantomVault service connection.
              </p>
            </div>
            <div className=\"setting-control\">
              <div className=\"service-status\">
                <span className=\"status-indicator connected\">🟢</span>
                <span className=\"status-text\">Connected</span>
                <button 
                  className=\"test-connection-button\"
                  onClick={() => {
                    console.log('Testing service connection...');
                  }}
                >
                  Test Connection
                </button>
              </div>
            </div>
          </div>
        </div>
      </div>

      <div className=\"settings-section\">
        <h2 className=\"section-title\">Data Management</h2>
        <p className=\"section-description\">
          Manage application data, logs, and configuration files.
        </p>
        
        <div className=\"setting-group\">
          {/* Log Retention */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Log retention period</label>
              <p className=\"setting-description\">
                How long to keep activity logs before automatic cleanup.
              </p>
            </div>
            <div className=\"setting-control\">
              <select 
                className=\"select-input\"
                defaultValue=\"30\"
                onChange={(e) => {
                  console.log('Log retention:', e.target.value);
                }}
              >
                <option value=\"7\">7 days</option>
                <option value=\"30\">30 days</option>
                <option value=\"90\">90 days</option>
                <option value=\"365\">1 year</option>
                <option value=\"0\">Never delete</option>
              </select>
            </div>
          </div>

          {/* Max Log Size */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Maximum log file size</label>
              <p className=\"setting-description\">
                Maximum size for individual log files before rotation.
              </p>
            </div>
            <div className=\"setting-control\">
              <div className=\"number-input-group\">
                <input
                  type=\"number\"
                  min=\"1\"
                  max=\"100\"
                  defaultValue={10}
                  className=\"number-input\"
                  onChange={(e) => {
                    console.log('Max log size:', e.target.value);
                  }}
                />
                <span className=\"input-suffix\">MB</span>
              </div>
            </div>
          </div>

          {/* Configuration Backup */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Automatic configuration backup</label>
              <p className=\"setting-description\">
                Automatically backup configuration files before making changes.
              </p>
            </div>
            <div className=\"setting-control\">
              <label className=\"toggle-switch\">
                <input
                  type=\"checkbox\"
                  defaultChecked={true}
                  onChange={(e) => {
                    console.log('Auto backup:', e.target.checked);
                  }}
                />
                <span className=\"toggle-slider\"></span>
              </label>
            </div>
          </div>
        </div>
      </div>

      <div className=\"settings-section\">
        <h2 className=\"section-title\">Diagnostics</h2>
        <p className=\"section-description\">
          Diagnostic tools and system information for troubleshooting.
        </p>
        
        <div className=\"setting-group\">
          {/* System Information */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">System information</label>
              <p className=\"setting-description\">
                Collect and display system information for debugging.
              </p>
            </div>
            <div className=\"setting-control\">
              <button 
                className=\"action-button secondary\"
                onClick={collectDebugInfo}
              >
                Collect Debug Info
              </button>
            </div>
          </div>

          {/* Debug Info Display */}
          {debugInfo && (
            <div className=\"setting-item info-only\">
              <div className=\"setting-info\">
                <label className=\"setting-label\">Debug information</label>
                <p className=\"setting-description\">
                  Collected system and application information.
                </p>
              </div>
              <div className=\"setting-control\">
                <div className=\"debug-info-display\">
                  <pre className=\"debug-info-content\">
                    {JSON.stringify(debugInfo, null, 2)}
                  </pre>
                  <button 
                    className=\"action-button secondary\"
                    onClick={exportDebugInfo}
                  >
                    Export Debug Info
                  </button>
                </div>
              </div>
            </div>
          )}

          {/* Connection Test */}
          <div className=\"setting-item\">
            <div className=\"setting-info\">
              <label className=\"setting-label\">Connection diagnostics</label>
              <p className=\"setting-description\">
                Test connectivity to the PhantomVault service and external resources.
              </p>
            </div>
            <div className=\"setting-control\">
              <button 
                className=\"action-button secondary\"
                onClick={() => {
                  console.log('Running connection diagnostics...');
                }}
              >
                Run Diagnostics
              </button>
            </div>
          </div>
        </div>
      </div>

      {/* Danger Zone */}
      <div className=\"settings-section danger-zone\">
        <div className=\"danger-zone-header\">
          <h2 className=\"section-title\">Danger Zone</h2>
          <button 
            onClick={() => setShowDangerZone(!showDangerZone)}
            className=\"toggle-danger-zone\"
          >
            {showDangerZone ? 'Hide' : 'Show'} Danger Zone
          </button>
        </div>
        
        {showDangerZone && (
          <>
            <p className=\"section-description danger\">
              ⚠️ These actions are irreversible and may cause data loss. Proceed with caution.
            </p>
            
            <div className=\"setting-group\">
              {/* Reset All Settings */}
              <div className=\"setting-item danger\">
                <div className=\"setting-info\">
                  <label className=\"setting-label\">Reset all settings</label>
                  <p className=\"setting-description\">
                    Reset all application settings to their default values.
                  </p>
                </div>
                <div className=\"setting-control\">
                  <button 
                    className=\"action-button danger\"
                    onClick={() => {
                      if (window.confirm('Are you sure you want to reset all settings? This cannot be undone.')) {
                        console.log('Resetting all settings...');
                      }
                    }}
                  >
                    Reset Settings
                  </button>
                </div>
              </div>

              {/* Clear All Data */}
              <div className=\"setting-item danger\">
                <div className=\"setting-info\">
                  <label className=\"setting-label\">Clear all application data</label>
                  <p className=\"setting-description\">
                    Remove all application data including logs, cache, and configurations.
                  </p>
                </div>
                <div className=\"setting-control\">
                  <button 
                    className=\"action-button danger\"
                    onClick={() => {
                      if (window.confirm('Are you sure you want to clear all data? This will remove everything and cannot be undone.')) {
                        console.log('Clearing all data...');
                      }
                    }}
                  >
                    Clear All Data
                  </button>
                </div>
              </div>

              {/* Factory Reset */}
              <div className=\"setting-item danger\">
                <div className=\"setting-info\">
                  <label className=\"setting-label\">Factory reset</label>
                  <p className=\"setting-description\">
                    Completely reset the application to its initial state.
                  </p>
                </div>
                <div className=\"setting-control\">
                  <button 
                    className=\"action-button danger\"
                    onClick={() => {
                      if (window.confirm('Are you sure you want to perform a factory reset? This will remove ALL data and settings.')) {
                        console.log('Performing factory reset...');
                      }
                    }}
                  >
                    Factory Reset
                  </button>
                </div>
              </div>
            </div>
          </>
        )}
      </div>
    </div>
  );
};