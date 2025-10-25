/**
 * Main Application Component
 * 
 * Root component that provides the main application layout with navigation and routing
 */

import React, { useState, useEffect, useCallback } from 'react';
import { VaultProvider } from './contexts/VaultContext';
import { AppProvider } from './contexts/AppContext';
import { Sidebar } from './components/layout/Sidebar';
import { Header } from './components/layout/Header';
import { MainContent } from './components/layout/MainContent';
import { NotificationContainer } from './components/common/NotificationContainer';
import { useServiceConnection } from './hooks/useServiceConnection';
import './App.css';

export type AppView = 'dashboard' | 'vaults' | 'activity' | 'settings';

export interface AppState {
  currentView: AppView;
  sidebarCollapsed: boolean;
  isFullscreen: boolean;
  theme: 'light' | 'dark';
}

/**
 * Main application component
 */
export const App: React.FC = () => {
  const [appState, setAppState] = useState<AppState>({
    currentView: 'dashboard',
    sidebarCollapsed: false,
    isFullscreen: false,
    theme: 'dark',
  });

  const { connectionState, isConnected } = useServiceConnection();

  // Handle view navigation
  const handleViewChange = useCallback((view: AppView) => {
    setAppState(prev => ({ ...prev, currentView: view }));
  }, []);

  // Handle sidebar toggle
  const handleSidebarToggle = useCallback(() => {
    setAppState(prev => ({ ...prev, sidebarCollapsed: !prev.sidebarCollapsed }));
  }, []);

  // Handle theme change
  const handleThemeChange = useCallback((theme: 'light' | 'dark') => {
    setAppState(prev => ({ ...prev, theme }));
    
    // Apply theme to document
    document.documentElement.setAttribute('data-theme', theme);
  }, []);

  // Handle fullscreen toggle
  const handleFullscreenToggle = useCallback(() => {
    setAppState(prev => ({ ...prev, isFullscreen: !prev.isFullscreen }));
  }, []);

  // Initialize theme
  useEffect(() => {
    document.documentElement.setAttribute('data-theme', appState.theme);
  }, [appState.theme]);

  // Handle keyboard shortcuts
  useEffect(() => {
    const handleKeyDown = (event: KeyboardEvent) => {
      // Ctrl/Cmd + number keys for view navigation
      if ((event.ctrlKey || event.metaKey) && !event.shiftKey && !event.altKey) {
        switch (event.key) {
          case '1':
            event.preventDefault();
            handleViewChange('dashboard');
            break;
          case '2':
            event.preventDefault();
            handleViewChange('vaults');
            break;
          case '3':
            event.preventDefault();
            handleViewChange('activity');
            break;
          case '4':
            event.preventDefault();
            handleViewChange('settings');
            break;
          case 'b':
            event.preventDefault();
            handleSidebarToggle();
            break;
          case 'f':
            event.preventDefault();
            handleFullscreenToggle();
            break;
        }
      }
    };

    document.addEventListener('keydown', handleKeyDown);
    return () => document.removeEventListener('keydown', handleKeyDown);
  }, [handleViewChange, handleSidebarToggle, handleFullscreenToggle]);

  return (
    <AppProvider>
      <VaultProvider>
        <div className={`app ${appState.theme} ${appState.isFullscreen ? 'fullscreen' : ''}`}>
          {/* Main Layout */}
          <div className="app-layout">
            {/* Sidebar */}
            <Sidebar
              currentView={appState.currentView}
              collapsed={appState.sidebarCollapsed}
              onViewChange={handleViewChange}
              onToggle={handleSidebarToggle}
              className={appState.sidebarCollapsed ? 'collapsed' : ''}
            />

            {/* Main Content Area */}
            <div className="app-main">
              {/* Header */}
              <Header
                currentView={appState.currentView}
                theme={appState.theme}
                isFullscreen={appState.isFullscreen}
                serviceConnected={isConnected}
                onThemeChange={handleThemeChange}
                onFullscreenToggle={handleFullscreenToggle}
                onSidebarToggle={handleSidebarToggle}
              />

              {/* Content */}
              <MainContent
                currentView={appState.currentView}
                className="main-content"
              />
            </div>
          </div>

          {/* Notifications */}
          <NotificationContainer />

          {/* Connection Lost Overlay */}
          {!isConnected && (
            <div className="connection-overlay">
              <div className="connection-message">
                <div className="connection-icon">🔌</div>
                <h3>Connection Lost</h3>
                <p>Unable to connect to PhantomVault service</p>
                <div className="connection-actions">
                  <button 
                    className="reconnect-button\"
                    onClick={() => window.location.reload()}
                  >
                    Restart Application
                  </button>
                </div>
              </div>
            </div>
          )}
        </div>
      </VaultProvider>
    </AppProvider>
  );
};