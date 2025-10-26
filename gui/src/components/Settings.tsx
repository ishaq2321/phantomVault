/**
 * PhantomVault Settings Component
 */

import React from 'react';
import { Box, Typography, Card, CardContent, Switch, FormControlLabel } from '@mui/material';

interface SettingsProps {
  theme: 'light' | 'dark';
  onThemeToggle: () => void;
}

const Settings: React.FC<SettingsProps> = ({ theme, onThemeToggle }) => {
  return (
    <Box sx={{ p: 3 }}>
      <Typography variant="h3" sx={{ mb: 3, fontWeight: 600 }}>
        Settings
      </Typography>
      
      <Card sx={{ mb: 3 }}>
        <CardContent>
          <Typography variant="h6" sx={{ mb: 2 }}>
            Appearance
          </Typography>
          <FormControlLabel
            control={
              <Switch
                checked={theme === 'dark'}
                onChange={onThemeToggle}
              />
            }
            label="Dark Mode"
          />
        </CardContent>
      </Card>

      <Card>
        <CardContent>
          <Typography variant="h6" sx={{ mb: 2 }}>
            Recovery
          </Typography>
          <Typography variant="body1" sx={{ opacity: 0.7 }}>
            Recovery key management will be implemented in a future phase.
          </Typography>
        </CardContent>
      </Card>
    </Box>
  );
};

export default Settings;