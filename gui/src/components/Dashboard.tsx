/**
 * PhantomVault Dashboard Component
 */

import React from 'react';
import { Box, Typography, Card, CardContent, Grid, Chip } from '@mui/material';

interface DashboardProps {
  isAdmin: boolean;
  serviceStatus: any;
}

const Dashboard: React.FC<DashboardProps> = ({ isAdmin, serviceStatus }) => {
  return (
    <Box sx={{ p: 3 }}>
      <Typography variant="h3" sx={{ mb: 3, fontWeight: 600 }}>
        Dashboard
      </Typography>
      
      <Grid container spacing={3}>
        {/* Service Status Card */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Typography variant="h6" sx={{ mb: 2 }}>
                Service Status
              </Typography>
              <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                <Chip 
                  label={serviceStatus?.running ? "Running" : "Stopped"} 
                  color={serviceStatus?.running ? "success" : "error"}
                  size="small"
                />
                {serviceStatus?.pid && (
                  <Typography variant="body2" sx={{ opacity: 0.7 }}>
                    PID: {serviceStatus.pid}
                  </Typography>
                )}
              </Box>
            </CardContent>
          </Card>
        </Grid>

        {/* Admin Status Card */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Typography variant="h6" sx={{ mb: 2 }}>
                Admin Status
              </Typography>
              <Chip 
                label={isAdmin ? "Admin Mode" : "User Mode"} 
                color={isAdmin ? "primary" : "default"}
                size="small"
              />
              {!isAdmin && (
                <Typography variant="body2" sx={{ mt: 1, opacity: 0.7 }}>
                  Run as admin to create profiles
                </Typography>
              )}
            </CardContent>
          </Card>
        </Grid>

        {/* Profiles Section */}
        <Grid item xs={12}>
          <Card>
            <CardContent>
              <Typography variant="h6" sx={{ mb: 2 }}>
                Profiles
              </Typography>
              <Typography variant="body1" sx={{ opacity: 0.7 }}>
                Profile management will be implemented in the next phase.
              </Typography>
            </CardContent>
          </Card>
        </Grid>
      </Grid>
    </Box>
  );
};

export default Dashboard;