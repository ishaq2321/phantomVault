/**
 * Settings Component
 * 
 * Main settings interface with tabbed categories for different preferences
 */

import React, { useState, useCallback, useEffect } from 'react';
import { AppSettings } from '../../types';
import { useApp } from '../../src/contexts';
import { GeneralSettings } from './GeneralSettings';
import { SecuritySettings } from './SecuritySettings';
import { UISettings } from './UISettings';
import { AdvancedSettings } from './AdvancedSettings';

export interface SettingsProps {
  className?: string;
  onClose?: () => void;
}

type SettingsTab = 'general' | 'security' | 'ui' | 'advanced';

interface SettingsTabInfo {
  id: SettingsTab;
  label: string;
  icon: string;
  description: string;
}

const SETTINGS_TABS: SettingsTabInfo[] = [
  {
    id: 'general',
    label: 'General',
    icon: '⚙️',
    description: 'Basic application preferences and behavior',
  },
  {
    id: 'security',
    label: 'Security',
    icon: '🔒',
    description: 'Security settings and authentication preferences',
  },
  {
    id: 'ui',
    label: 'Interface',
    icon: '🎨',
    description: 'Theme, layout, and visual preferences',
  },
  {
    id: 'advanced',
    label: 'Advanced',
    icon: '🔧',
    description: 'Advanced configuration and debugging options',
  },
];

/**
 * Main settings component
 */
export const Settings: React.FC<SettingsProps> = ({
  className = '',
  onClose,
}) => {
  const { state: appState, actions: appActions } = useApp();
  const [activeTab, setActiveTab] = useState<SettingsTab>('general');
  const [localSettings, setLocalSettings] = useState<AppSettings>(appState.settings);
  const [hasUnsavedChanges, setHasUnsavedChanges] = useState(false);
  const [isSaving, setIsSaving] = useState(false);
  const [saveError, setSaveError] = useState<string | null>(null);

  // Check for unsaved changes
  useEffect(() => {
    const hasChanges = JSON.stringify(localSettings) !== JSON.stringify(appState.settings);
    setHasUnsavedChanges(hasChanges);
  }, [localSettings, appState.settings]);

  // Handle settings change
  const handleSettingsChange = useCallback((changes: Partial<AppSettings>) => {
    setLocalSettings(prev => ({ ...prev, ...changes }));
    setSaveError(null);
  }, []);

  // Save settings
  const saveSettings = useCallback(async () => {
    setIsSaving(true);
    setSaveError(null);
    
    try {
      await appActions.updateSettings(localSettings);
      setHasUnsavedChanges(false);
      
      // Show success notification
      appActions.addNotification({
        type: 'success',
        title: 'Settings Saved',
        message: 'Your preferences have been saved successfully.',
        duration: 3000,
      });
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to save settings';
      setSaveError(errorMessage);
      
      appActions.addNotification({
        type: 'error',
        title: 'Save Failed',
        message: errorMessage,
        duration: 5000,
      });
    } finally {
      setIsSaving(false);
    }
  }, [localSettings, appActions]);

  // Reset settings to defaults
  const resetToDefaults = useCallback(() => {
    const defaultSettings: AppSettings = {
      theme: 'dark',
      autoStart: false,
      notifications: true,
      minimizeToTray: true,
      autoLock: true,
      lockTimeout: 15,
    };
    
    setLocalSettings(defaultSettings);
    setSaveError(null);
  }, []);

  // Discard changes
  const discardChanges = useCallback(() => {
    setLocalSettings(appState.settings);
    setSaveError(null);
  }, [appState.settings]);

  // Handle tab change
  const handleTabChange = useCallback((tab: SettingsTab) => {
    setActiveTab(tab);
  }, []);

  // Handle close with unsaved changes check
  const handleClose = useCallback(() => {
    if (hasUnsavedChanges) {
      const shouldDiscard = window.confirm(
        'You have unsaved changes. Are you sure you want to close without saving?'
      );
      
      if (!shouldDiscard) {
        return;
      }
    }
    
    onClose?.();
  }, [hasUnsavedChanges, onClose]);

  // Render tab content
  const renderTabContent = () => {
    const commonProps = {
      settings: localSettings,
      onSettingsChange: handleSettingsChange,
    };

    switch (activeTab) {
      case 'general':
        return <GeneralSettings {...commonProps} />;
      case 'security':
        return <SecuritySettings {...commonProps} />;
      case 'ui':
        return <UISettings {...commonProps} />;
      case 'advanced':
        return <AdvancedSettings {...commonProps} />;
      default:
        return null;
    }
  };

  return (
    <div className={`settings-container ${className}`}>
      {/* Header */}
      <div className="settings-header">
        <div className="header-content">
          <h1 className="settings-title">Settings</h1>
          <p className="settings-subtitle">
            Configure your PhantomVault preferences and behavior
          </p>
        </div>
        
        {onClose && (
          <button 
            onClick={handleClose}
            className="close-button\"
            title="Close settings\"
          >
            ✕
          </button>
        )}
      </div>

      <div className="settings-content">
        {/* Sidebar with tabs */}
        <div className="settings-sidebar">
          <nav className="settings-nav">
            {SETTINGS_TABS.map(tab => (
              <button
                key={tab.id}
                onClick={() => handleTabChange(tab.id)}
                className={`nav-item ${activeTab === tab.id ? 'active' : ''}`}
                title={tab.description}
              >
                <span className="nav-icon">{tab.icon}</span>
                <span className="nav-label">{tab.label}</span>
              </button>
            ))}
          </nav>
          
          {/* Tab description */}
          <div className="tab-description">
            <p>{SETTINGS_TABS.find(tab => tab.id === activeTab)?.description}</p>
          </div>
        </div>

        {/* Main content area */}
        <div className="settings-main">
          {/* Tab content */}
          <div className="tab-content">
            {renderTabContent()}
          </div>

          {/* Save error */}
          {saveError && (
            <div className="save-error">
              <span className="error-icon">❌</span>
              <span className="error-message">{saveError}</span>
            </div>
          )}

          {/* Action buttons */}
          <div className="settings-actions">
            <div className="action-group">
              <button 
                onClick={resetToDefaults}
                className="action-button secondary\"
                disabled={isSaving}
              >
                Reset to Defaults
              </button>
              
              {hasUnsavedChanges && (
                <button 
                  onClick={discardChanges}
                  className="action-button secondary\"
                  disabled={isSaving}
                >
                  Discard Changes
                </button>
              )}
            </div>
            
            <div className="action-group">
              <button 
                onClick={saveSettings}
                className="action-button primary\"
                disabled={!hasUnsavedChanges || isSaving}
              >
                {isSaving ? (
                  <>
                    <span className="loading-spinner">⏳</span>
                    Saving...
                  </>
                ) : (
                  'Save Settings'
                )}
              </button>
            </div>
          </div>

          {/* Unsaved changes indicator */}
          {hasUnsavedChanges && (
            <div className="unsaved-changes-indicator">
              <span className="indicator-icon">⚠️</span>
              <span className="indicator-text">You have unsaved changes</span>
            </div>
          )}
        </div>
      </div>
    </div>
  );
};