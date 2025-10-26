/**
 * PhantomVault Loading Screen Component
 */

import React from 'react';
import { Box, Typography, CircularProgress } from '@mui/material';

const LoadingScreen: React.FC = () => {
  return (
    <Box
      sx={{
        display: 'flex',
        flexDirection: 'column',
        justifyContent: 'center',
        alignItems: 'center',
        minHeight: '100vh',
        background: 'linear-gradient(135deg, #1a1a2e 0%, #16213e 100%)',
        color: 'white',
      }}
    >
      <Typography variant="h1" sx={{ fontSize: '3rem', mb: 2 }}>
        👻
      </Typography>
      <Typography variant="h4" sx={{ mb: 4, fontWeight: 300 }}>
        PhantomVault
      </Typography>
      <CircularProgress size={40} sx={{ color: '#4f46e5' }} />
      <Typography variant="body1" sx={{ mt: 2, opacity: 0.8 }}>
        Initializing secure folder management...
      </Typography>
    </Box>
  );
};

export default LoadingScreen;