/**
 * PhantomVault GUI - Main Application Component
 * 
 * Root component that provides the main application layout and routing.
 */

import React, { useState, useEffect } from 'react';
import { Routes, Route, Navigate } from 'react-router-dom';
import { Box, Container, Alert, Snackbar } from '@mui/material';

// Components
import Dashboard from '@components/Dashboard';
import Analytics from '@components/Analytics';
import Settings from '@components/Settings';
import LoadingScreen from '@components/LoadingScreen';
import ErrorBoundary from '@components/ErrorBoundary';

// Hooks and services
import { useServiceStatus } from '@hooks/useServiceStatus';
import { useAppTheme } from '@hooks/useAppTheme';

// Types
interface AppState {
  isLoading: boolean;
  error: string | null;
  isAdmin: boolean;
}

/**
 * Main Application Component
 */
const App: React.FC = () => {
  const [appState, setAppState] = useState<AppState>({
    isLoading: true,
    error: null,
    isAdmin: false,
  });

  const { serviceStatus, isConnected } = useServiceStatus();
  const { theme, toggleTheme } = useAppTheme();

  // Initialize application
  useEffect(() => {
    const initializeApp = async () => {
      try {
        // Initialize PhantomVault GUI
        const isAdmin = await window.phantomVault.app.isAdmin();
        const status = await window.phantomVault.service.getStatus();

        setAppState({
          isLoading: false,
          error: null,
          isAdmin,
        });

        // Application initialized successfully
      } catch (error) {
        console.error('[App] Failed to initialize application:', error);
        setAppState({
          isLoading: false,
          error: error instanceof Error ? error.message : 'Failed to initialize application',
          isAdmin: false,
        });
      }
    };

    initializeApp();
  }, []);

  // Handle service connection errors
  useEffect(() => {
    if (!isConnected && !appState.isLoading) {
      setAppState(prev => ({
        ...prev,
        error: 'Lost connection to PhantomVault service',
      }));
    }
  }, [isConnected, appState.isLoading]);

  // Listen for navigation events from main process
  useEffect(() => {
    const handleNavigate = (path: string) => {
      // This would be handled by React Router in a real implementation
      console.log('[App] Navigation requested:', path);
    };

    // Listen for navigation events
    if (window.phantomVault?.on) {
      // Note: This would need to be implemented in the preload script
      // window.phantomVault.on.navigate(handleNavigate);
    }

    return () => {
      // Cleanup navigation listener
      // window.phantomVault.off.navigate(handleNavigate);
    };
  }, []);

  // Show loading screen
  if (appState.isLoading) {
    return <LoadingScreen />;
  }

  return (
    <ErrorBoundary>
      <Box
        sx={{
          minHeight: '100vh',
          bgcolor: 'background.default',
          color: 'text.primary',
        }}
      >
        <Container maxWidth="xl" sx={{ py: 2 }}>
          <Routes>
            <Route path="/" element={<Navigate to="/dashboard" replace />} />
            <Route 
              path="/dashboard" 
              element={
                <Dashboard 
                  isAdmin={appState.isAdmin}
                  serviceStatus={serviceStatus}
                />
              } 
            />
            <Route 
              path="/analytics" 
              element={<Analytics />} 
            />
            <Route 
              path="/settings" 
              element={
                <Settings 
                  theme={theme}
                  onThemeToggle={toggleTheme}
                />
              } 
            />
            <Route path="*" element={<Navigate to="/dashboard" replace />} />
          </Routes>
        </Container>

        {/* Error notification */}
        <Snackbar
          open={!!appState.error}
          autoHideDuration={6000}
          onClose={() => setAppState(prev => ({ ...prev, error: null }))}
          anchorOrigin={{ vertical: 'bottom', horizontal: 'right' }}
        >
          <Alert 
            severity="error" 
            onClose={() => setAppState(prev => ({ ...prev, error: null }))}
            sx={{ width: '100%' }}
          >
            {appState.error}
          </Alert>
        </Snackbar>

        {/* Service connection status */}
        <Snackbar
          open={!isConnected && !appState.isLoading}
          anchorOrigin={{ vertical: 'top', horizontal: 'center' }}
        >
          <Alert severity="warning" sx={{ width: '100%' }}>
            Service connection lost. Some features may not be available.
          </Alert>
        </Snackbar>
      </Box>
    </ErrorBoundary>
  );
};

export default App;