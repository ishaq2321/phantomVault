/**
 * PhantomVault Error Boundary Component
 */

import React, { Component, ErrorInfo, ReactNode } from 'react';
import { Box, Typography, Button, Alert } from '@mui/material';

interface Props {
  children: ReactNode;
}

interface State {
  hasError: boolean;
  error?: Error;
}

class ErrorBoundary extends Component<Props, State> {
  public state: State = {
    hasError: false,
  };

  public static getDerivedStateFromError(error: Error): State {
    return { hasError: true, error };
  }

  public componentDidCatch(error: Error, errorInfo: ErrorInfo) {
    console.error('[ErrorBoundary] Uncaught error:', error, errorInfo);
  }

  private handleReload = () => {
    window.location.reload();
  };

  public render() {
    if (this.state.hasError) {
      return (
        <Box
          sx={{
            display: 'flex',
            flexDirection: 'column',
            justifyContent: 'center',
            alignItems: 'center',
            minHeight: '100vh',
            p: 4,
            bgcolor: 'background.default',
          }}
        >
          <Typography variant="h1" sx={{ fontSize: '4rem', mb: 2 }}>
            💥
          </Typography>
          <Typography variant="h4" sx={{ mb: 2, textAlign: 'center' }}>
            Something went wrong
          </Typography>
          <Alert severity="error" sx={{ mb: 4, maxWidth: 600 }}>
            <Typography variant="body1">
              {this.state.error?.message || 'An unexpected error occurred'}
            </Typography>
          </Alert>
          <Button
            variant="contained"
            onClick={this.handleReload}
            sx={{ mb: 2 }}
          >
            Reload Application
          </Button>
          <Typography variant="body2" sx={{ opacity: 0.7, textAlign: 'center' }}>
            If this problem persists, please report it on GitHub
          </Typography>
        </Box>
      );
    }

    return this.props.children;
  }
}

export default ErrorBoundary;