/**
 * Header Component
 * 
 * Application header with title, controls, and status indicators
 */

import React, { useCallback, useState } from 'react';
import { AppView } from '../../App';
import './Header.css';

export interface HeaderProps {
  currentView: AppView;
  theme: 'light' | 'dark';
  isFullscreen: boolean;
  serviceConnected: boolean;
  onThemeChange: (theme: 'light' | 'dark') => void;
  onFullscreenToggle: () => void;
  onSidebarToggle: () => void;
  className?: string;
}

interface ViewInfo {
  title: string;
  subtitle: string;
  icon: string;
}

const VIEW_INFO: Record<AppView, ViewInfo> = {
  dashboard: {
    title: 'Dashboard',
    subtitle: 'Overview of your vaults and recent activity',
    icon: '🏠',
  },
  vaults: {
    title: 'Vault Management',
    subtitle: 'Create, configure, and manage your encrypted vaults',
    icon: '🗄️',
  },
  activity: {
    title: 'Activity Monitor',
    subtitle: 'View logs and monitor vault operations',
    icon: '📊',
  },
  settings: {
    title: 'Settings',
    subtitle: 'Configure application preferences and security',
    icon: '⚙️',
  },
};

/**
 * Application header component
 */
export const Header: React.FC<HeaderProps> = ({
  currentView,
  theme,
  isFullscreen,
  serviceConnected,
  onThemeChange,
  onFullscreenToggle,
  onSidebarToggle,
  className = '',
}) => {
  const [showUserMenu, setShowUserMenu] = useState(false);
  
  const viewInfo = VIEW_INFO[currentView];

  // Handle theme toggle
  const handleThemeToggle = useCallback(() => {
    onThemeChange(theme === 'light' ? 'dark' : 'light');
  }, [theme, onThemeChange]);

  // Handle user menu toggle
  const handleUserMenuToggle = useCallback(() => {
    setShowUserMenu(prev => !prev);
  }, []);

  // Get current time
  const getCurrentTime = () => {
    return new Date().toLocaleTimeString([], { 
      hour: '2-digit', 
      minute: '2-digit' 
    });
  };

  return (
    <header className={`app-header ${className}`}>
      {/* Left Section */}
      <div className="header-left">
        <button 
          onClick={onSidebarToggle}
          className="sidebar-toggle-button"
          title="Toggle sidebar (Ctrl+B)"
        >
          ☰
        </button>
        
        <div className="view-info">
          <div className="view-title">
            <span className="view-icon">{viewInfo.icon}</span>
            <h1>{viewInfo.title}</h1>
          </div>
          <p className="view-subtitle">{viewInfo.subtitle}</p>
        </div>
      </div>

      {/* Center Section */}
      <div className="header-center">
        {/* Search Bar (for future implementation) */}
        <div className="search-container">
          <input
            type="text"
            placeholder="Search vaults, logs, settings..."
            className="search-input"
            disabled
          />
          <button className="search-button" disabled>
            🔍
          </button>
        </div>
      </div>

      {/* Right Section */}
      <div className="header-right">
        {/* Service Status Indicator */}
        <div className="service-indicator">
          <span 
            className={`status-dot ${serviceConnected ? 'connected' : 'disconnected'}`}
            title={serviceConnected ? 'Service connected' : 'Service disconnected'}
          />
          <span className="status-text">
            {serviceConnected ? 'Connected' : 'Disconnected'}
          </span>
        </div>

        {/* Current Time */}
        <div className="current-time">
          <span className="time-icon">🕐</span>
          <span className="time-text">{getCurrentTime()}</span>
        </div>

        {/* Control Buttons */}
        <div className="header-controls">
          {/* Theme Toggle */}
          <button
            onClick={handleThemeToggle}
            className="control-button"
            title={`Switch to ${theme === 'light' ? 'dark' : 'light'} theme`}
          >
            {theme === 'light' ? '🌙' : '☀️'}
          </button>

          {/* Fullscreen Toggle */}
          <button
            onClick={onFullscreenToggle}
            className="control-button\"
            title={`${isFullscreen ? 'Exit' : 'Enter'} fullscreen (Ctrl+F)`}
          >
            {isFullscreen ? '🗗' : '🗖'}
          </button>

          {/* Notifications */}
          <button
            className="control-button\"
            title="Notifications\"
            onClick={() => {
              // In a real implementation, this would show notifications
              console.log('Opening notifications...');
            }}
          >
            🔔
          </button>

          {/* User Menu */}
          <div className="user-menu-container">
            <button
              onClick={handleUserMenuToggle}
              className="user-menu-button\"
              title="User menu\"
            >
              <div className="user-avatar">
                <span className="avatar-icon">👤</span>
              </div>
              <span className="user-name">User</span>
              <span className="dropdown-arrow">{showUserMenu ? '▲' : '▼'}</span>
            </button>

            {showUserMenu && (
              <div className="user-menu-dropdown">
                <div className="menu-header">
                  <div className="user-info">
                    <div className="user-avatar large">
                      <span className="avatar-icon">👤</span>
                    </div>
                    <div className="user-details">
                      <span className="user-name">PhantomVault User</span>
                      <span className="user-email">user@example.com</span>
                    </div>
                  </div>
                </div>
                
                <div className="menu-items">
                  <button className="menu-item">
                    <span className="menu-icon">👤</span>
                    <span className="menu-text">Profile</span>
                  </button>
                  
                  <button className="menu-item">
                    <span className="menu-icon">🔒</span>
                    <span className="menu-text">Security</span>
                  </button>
                  
                  <button className="menu-item">
                    <span className="menu-icon">⚙️</span>
                    <span className="menu-text">Preferences</span>
                  </button>
                  
                  <div className="menu-separator\" />
                  
                  <button className="menu-item">
                    <span className="menu-icon">❓</span>
                    <span className="menu-text">Help & Support</span>
                  </button>
                  
                  <button className="menu-item">
                    <span className="menu-icon">ℹ️</span>
                    <span className="menu-text">About</span>
                  </button>
                  
                  <div className="menu-separator\" />
                  
                  <button className="menu-item danger">
                    <span className="menu-icon">🚪</span>
                    <span className="menu-text">Exit</span>
                  </button>
                </div>
              </div>
            )}
          </div>
        </div>
      </div>

      {/* Click outside to close user menu */}
      {showUserMenu && (
        <div 
          className="menu-overlay\"
          onClick={() => setShowUserMenu(false)}
        />
      )}
    </header>
  );
};