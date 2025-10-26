/**
 * PhantomVault Analytics Component
 */

import React from 'react';
import { Box, Typography, Card, CardContent } from '@mui/material';

const Analytics: React.FC = () => {
  return (
    <Box sx={{ p: 3 }}>
      <Typography variant="h3" sx={{ mb: 3, fontWeight: 600 }}>
        Analytics
      </Typography>
      
      <Card>
        <CardContent>
          <Typography variant="h6" sx={{ mb: 2 }}>
            Usage Statistics
          </Typography>
          <Typography variant="body1" sx={{ opacity: 0.7 }}>
            Analytics tracking will be implemented in a future phase.
          </Typography>
        </CardContent>
      </Card>
    </Box>
  );
};

export default Analytics;