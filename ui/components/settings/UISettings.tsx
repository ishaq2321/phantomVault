/**
 * UI Settings Component
 * 
 * Theme, layout, and visual preference settings
 */

import React, { useCallback, useState } from 'react';
import { AppSettings } from '../../types';

export interface UISettingsProps {
  settings: AppSettings;
  onSettingsChange: (changes: Partial<AppSettings>) => void;
}

/**
 * UI settings component
 */
export const UISettings: React.FC<UISettingsProps> = ({
  settings,
  onSettingsChange,
}) => {
  const [previewTheme, setPreviewTheme] = useState<string | null>(null);

  // Handle setting change
  const handleChange = useCallback((key: keyof AppSettings, value: any) => {
    onSettingsChange({ [key]: value });
  }, [onSettingsChange]);

  // Handle theme preview
  const handleThemePreview = useCallback((theme: string) => {
    setPreviewTheme(theme);
    // In a real implementation, this would apply the theme temporarily
    setTimeout(() => setPreviewTheme(null), 3000);
  }, []);

  return (
    <div className="ui-settings">
      <div className="settings-section">
        <h2 className="section-title">Appearance</h2>
        <p className="section-description">
          Customize the visual appearance and theme of the application.
        </p>
        
        <div className="setting-group">
          {/* Theme Selection */}
          <div className="setting-item">
            <div className="setting-info">
              <label className="setting-label">Theme</label>
              <p className="setting-description">
                Choose between light, dark, or system-based theme.
              </p>
            </div>
            <div className="setting-control">
              <div className="theme-selector">
                {[
                  { id: 'light', name: 'Light', icon: '☀️', preview: '#ffffff' },
                  { id: 'dark', name: 'Dark', icon: '🌙', preview: '#1a1a1a' },
                  { id: 'system', name: 'System', icon: '🖥️', preview: 'linear-gradient(45deg, #ffffff 50%, #1a1a1a 50%)' },
                ].map(theme => (
                  <button
                    key={theme.id}
                    onClick={() => handleChange('theme', theme.id)}
                    onMouseEnter={() => handleThemePreview(theme.id)}
                    className={`theme-option ${settings.theme === theme.id ? 'active' : ''}`}
                  >
                    <div 
                      className="theme-preview\"
                      style={{ background: theme.preview }}
                    />
                    <span className="theme-icon">{theme.icon}</span>
                    <span className="theme-name">{theme.name}</span>
                  </button>
                ))}
              </div>
              {previewTheme && (
                <p className="preview-notice">
                  Previewing {previewTheme} theme...
                </p>
              )}
            </div>
          </div>

          {/* Color Accent */}
          <div className="setting-item">
            <div className="setting-info">
              <label className="setting-label">Accent color</label>
              <p className="setting-description">
                Choose an accent color for highlights and interactive elements.
              </p>
            </div>
            <div className="setting-control">
              <div className="color-selector">
                {[
                  { name: 'Blue', value: '#2196F3' },
                  { name: 'Purple', value: '#9C27B0' },
                  { name: 'Green', value: '#4CAF50' },
                  { name: 'Orange', value: '#FF9800' },
                  { name: 'Red', value: '#F44336' },
                  { name: 'Teal', value: '#009688' },
                ].map(color => (
                  <button
                    key={color.value}
                    className="color-option\"
                    style={{ backgroundColor: color.value }}
                    title={color.name}
                    onClick={() => {
                      console.log('Accent color:', color.value);
                    }}
                  />
                ))}
              </div>
            </div>
          </div>

          {/* Font Size */}
          <div className="setting-item">
            <div className="setting-info">
              <label className="setting-label">Font size</label>
              <p className="setting-description">
                Adjust the base font size for better readability.
              </p>
            </div>
            <div className="setting-control">
              <div className="font-size-selector">
                {[
                  { name: 'Small', value: 'small', size: '12px' },
                  { name: 'Medium', value: 'medium', size: '14px' },
                  { name: 'Large', value: 'large', size: '16px' },
                  { name: 'Extra Large', value: 'xl', size: '18px' },
                ].map(size => (
                  <button
                    key={size.value}
                    className="font-size-option\"
                    onClick={() => {
                      console.log('Font size:', size.value);
                    }}
                  >
                    <span style={{ fontSize: size.size }}>Aa</span>
                    <span className="size-name">{size.name}</span>
                  </button>
                ))}
              </div>
            </div>
          </div>
        </div>
      </div>

      <div className="settings-section">
        <h2 className="section-title">Layout & Navigation</h2>
        <p className="section-description">
          Configure the layout and navigation behavior of the interface.
        </p>
        
        <div className="setting-group">
          {/* Sidebar Position */}
          <div className="setting-item">
            <div className="setting-info">
              <label className="setting-label">Sidebar position</label>
              <p className="setting-description">
                Choose whether the navigation sidebar appears on the left or right.
              </p>
            </div>
            <div className="setting-control">
              <select 
                className="select-input\"
                defaultValue="left\"
                onChange={(e) => {
                  console.log('Sidebar position:', e.target.value);
                }}
              >
                <option value="left">Left</option>
                <option value="right">Right</option>
              </select>
            </div>
          </div>

          {/* Compact Mode */}
          <div className="setting-item">
            <div className="setting-info">
              <label className="setting-label">Compact mode</label>
              <p className="setting-description">
                Use a more compact layout with reduced spacing and smaller elements.
              </p>
            </div>
            <div className="setting-control">
              <label className="toggle-switch">
                <input
                  type="checkbox\"
                  defaultChecked={false}
                  onChange={(e) => {
                    console.log('Compact mode:', e.target.checked);
                  }}
                />
                <span className="toggle-slider"></span>
              </label>
            </div>
          </div>

          {/* Show Tooltips */}
          <div className="setting-item">
            <div className="setting-info">
              <label className="setting-label">Show tooltips</label>
              <p className="setting-description">
                Display helpful tooltips when hovering over interface elements.
              </p>
            </div>
            <div className="setting-control">
              <label className="toggle-switch">
                <input
                  type="checkbox\"
                  defaultChecked={true}
                  onChange={(e) => {
                    console.log('Show tooltips:', e.target.checked);
                  }}
                />
                <span className="toggle-slider"></span>
              </label>
            </div>
          </div>

          {/* Animation Speed */}
          <div className="setting-item">
            <div className="setting-info">
              <label className="setting-label">Animation speed</label>
              <p className="setting-description">
                Control the speed of interface animations and transitions.
              </p>
            </div>
            <div className="setting-control">
              <select 
                className="select-input\"
                defaultValue="normal\"
                onChange={(e) => {
                  console.log('Animation speed:', e.target.value);
                }}
              >
                <option value="disabled">Disabled</option>
                <option value="slow">Slow</option>
                <option value="normal">Normal</option>
                <option value="fast">Fast</option>
              </select>
            </div>
          </div>
        </div>
      </div>

      <div className="settings-section">
        <h2 className="section-title">Notifications & Feedback</h2>
        <p className="section-description">
          Configure how the application provides visual and audio feedback.
        </p>
        
        <div className="setting-group">
          {/* Notification Position */}
          <div className="setting-item">
            <div className="setting-info">
              <label className="setting-label">Notification position</label>
              <p className="setting-description">
                Choose where in-app notifications appear on screen.
              </p>
            </div>
            <div className="setting-control">
              <select 
                className="select-input\"
                defaultValue="top-right\"
                onChange={(e) => {
                  console.log('Notification position:', e.target.value);
                }}
              >
                <option value="top-left">Top Left</option>
                <option value="top-right">Top Right</option>
                <option value="bottom-left">Bottom Left</option>
                <option value="bottom-right">Bottom Right</option>
                <option value="center">Center</option>
              </select>
            </div>
          </div>

          {/* Notification Duration */}
          <div className="setting-item">
            <div className="setting-info">
              <label className="setting-label">Notification duration</label>
              <p className="setting-description">
                How long notifications remain visible before auto-dismissing.
              </p>
            </div>
            <div className="setting-control">
              <div className="number-input-group">
                <input
                  type="number\"
                  min="1\"
                  max="30\"
                  defaultValue={5}
                  className="number-input\"
                  onChange={(e) => {
                    console.log('Notification duration:', e.target.value);
                  }}
                />
                <span className="input-suffix">seconds</span>
              </div>
            </div>
          </div>

          {/* Sound Effects */}
          <div className="setting-item">
            <div className="setting-info">
              <label className="setting-label">Sound effects</label>
              <p className="setting-description">
                Play sound effects for notifications and important events.
              </p>
            </div>
            <div className="setting-control">
              <label className="toggle-switch">
                <input
                  type="checkbox\"
                  defaultChecked={true}
                  onChange={(e) => {
                    console.log('Sound effects:', e.target.checked);
                  }}
                />
                <span className="toggle-slider"></span>
              </label>
            </div>
          </div>

          {/* Visual Effects */}
          <div className="setting-item">
            <div className="setting-info">
              <label className="setting-label">Visual effects</label>
              <p className="setting-description">
                Enable visual effects like glows, shadows, and particle effects.
              </p>
            </div>
            <div className="setting-control">
              <label className="toggle-switch">
                <input
                  type="checkbox\"
                  defaultChecked={true}
                  onChange={(e) => {
                    console.log('Visual effects:', e.target.checked);
                  }}
                />
                <span className="toggle-slider"></span>
              </label>
            </div>
          </div>
        </div>
      </div>

      <div className="settings-section">
        <h2 className="section-title">Accessibility</h2>
        <p className="section-description">
          Configure accessibility features for better usability.
        </p>
        
        <div className="setting-group">
          {/* High Contrast */}
          <div className="setting-item">
            <div className="setting-info">
              <label className="setting-label">High contrast mode</label>
              <p className="setting-description">
                Use high contrast colors for better visibility.
              </p>
            </div>
            <div className="setting-control">
              <label className="toggle-switch">
                <input
                  type="checkbox\"
                  defaultChecked={false}
                  onChange={(e) => {
                    console.log('High contrast:', e.target.checked);
                  }}
                />
                <span className="toggle-slider"></span>
              </label>
            </div>
          </div>

          {/* Reduce Motion */}
          <div className="setting-item">
            <div className="setting-info">
              <label className="setting-label">Reduce motion</label>
              <p className="setting-description">
                Minimize animations and motion effects for users sensitive to movement.
              </p>
            </div>
            <div className="setting-control">
              <label className="toggle-switch">
                <input
                  type="checkbox\"
                  defaultChecked={false}
                  onChange={(e) => {
                    console.log('Reduce motion:', e.target.checked);
                  }}
                />
                <span className="toggle-slider"></span>
              </label>
            </div>
          </div>

          {/* Screen Reader Support */}
          <div className="setting-item">
            <div className="setting-info">
              <label className="setting-label">Enhanced screen reader support</label>
              <p className="setting-description">
                Provide additional context and descriptions for screen readers.
              </p>
            </div>
            <div className="setting-control">
              <label className="toggle-switch">
                <input
                  type="checkbox\"
                  defaultChecked={false}
                  onChange={(e) => {
                    console.log('Screen reader support:', e.target.checked);
                  }}
                />
                <span className="toggle-slider"></span>
              </label>
            </div>
          </div>

          {/* Keyboard Navigation */}
          <div className="setting-item">
            <div className="setting-info">
              <label className="setting-label">Enhanced keyboard navigation</label>
              <p className="setting-description">
                Improve keyboard navigation with visible focus indicators and shortcuts.
              </p>
            </div>
            <div className="setting-control">
              <label className="toggle-switch">
                <input
                  type="checkbox\"
                  defaultChecked={true}
                  onChange={(e) => {
                    console.log('Enhanced keyboard navigation:', e.target.checked);
                  }}
                />
                <span className="toggle-slider"></span>
              </label>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};