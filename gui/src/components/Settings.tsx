/**
 * PhantomVault Settings Component
 * 
 * Application settings including update management, diagnostics, and maintenance.
 */

import React, { useState, useEffect } from 'react';
import {
  Box,
  Typography,
  Card,
  CardContent,
  Grid,
  Button,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Alert,
  LinearProgress,
  Chip,
  Stack,
  Divider,
  List,
  ListItem,
  ListItemText,
  ListItemSecondaryAction,
  IconButton,
  Tooltip,
} from '@mui/material';
import {
  Update as UpdateIcon,
  Download as DownloadIcon,
  Settings as SettingsIcon,
  Info as InfoIcon,
  Build as BuildIcon,
  Security as SecurityIcon,
  Refresh as RefreshIcon,
  CheckCircle as CheckCircleIcon,
  Warning as WarningIcon,
  Error as ErrorIcon,
} from '@mui/icons-material';

interface SettingsProps {
  theme: string;
  onThemeToggle: () => void;
}

interface UpdateInfo {
  success: boolean;
  currentVersion: string;
  latestVersion: string;
  isUpdateAvailable: boolean;
  downloadUrl?: string;
  releaseNotes?: string;
  publishedAt?: string;
  error?: string;
}

const Settings: React.FC<SettingsProps> = ({ theme, onThemeToggle }) => {
  const [updateInfo, setUpdateInfo] = useState<UpdateInfo | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);
  const [updateDialog, setUpdateDialog] = useState(false);
  const [releaseNotesDialog, setReleaseNotesDialog] = useState(false);

  // Load initial data
  useEffect(() => {
    loadCurrentVersion();
  }, []);

  const loadCurrentVersion = async () => {
    try {
      const version = await window.phantomVault.app.getVersion();
      setUpdateInfo(prev => ({
        ...prev,
        success: true,
        currentVersion: version,
        latestVersion: version,
        isUpdateAvailable: false,
      } as UpdateInfo));
    } catch (err) {
      setError('Failed to get current version');
    }
  };

  const checkForUpdates = async () => {
    try {
      setLoading(true);
      setError(null);
      
      const result = await window.phantomVault.app.checkForUpdates();
      
      if (result.success) {
        setUpdateInfo(result);
        if (result.isUpdateAvailable) {
          setSuccess(`Update available: v${result.latestVersion}`);
          setUpdateDialog(true);
        } else {
          setSuccess('You have the latest version');
        }
      } else {
        setError('Failed to check for updates: ' + result.error);
      }
    } catch (err) {
      setError('Failed to check for updates: ' + (err instanceof Error ? err.message : 'Unknown error'));
    } finally {
      setLoading(false);
    }
  };

  const downloadUpdate = async () => {
    if (!updateInfo?.latestVersion) return;
    
    try {
      setLoading(true);
      const result = await window.phantomVault.app.downloadUpdate(updateInfo.latestVersion);
      
      if (result.success) {
        setSuccess('Redirected to download page');
        setUpdateDialog(false);
      } else {
        setError('Failed to download update: ' + result.error);
      }
    } catch (err) {
      setError('Failed to download update: ' + (err instanceof Error ? err.message : 'Unknown error'));
    } finally {
      setLoading(false);
    }
  };

  const formatDate = (dateString: string) => {
    return new Date(dateString).toLocaleDateString();
  };

  return (
    <Box sx={{ p: 3 }}>
      <Typography variant="h3" sx={{ mb: 3, fontWeight: 600 }}>
        Settings
      </Typography>
      
      {loading && <LinearProgress sx={{ mb: 2 }} />}
      
      {error && (
        <Alert severity="error" sx={{ mb: 2 }} onClose={() => setError(null)}>
          {error}
        </Alert>
      )}
      
      {success && (
        <Alert severity="success" sx={{ mb: 2 }} onClose={() => setSuccess(null)}>
          {success}
        </Alert>
      )}
      
      <Grid container spacing={3}>
        {/* Application Information */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 2 }}>
                <InfoIcon color="primary" />
                <Typography variant="h6">Application Information</Typography>
              </Stack>
              
              <List>
                <ListItem>
                  <ListItemText
                    primary="Version"
                    secondary={updateInfo?.currentVersion || 'Loading...'}
                  />
                </ListItem>
                <ListItem>
                  <ListItemText
                    primary="Platform"
                    secondary={`${process.platform} ${process.arch}`}
                  />
                </ListItem>
                <ListItem>
                  <ListItemText
                    primary="Electron Version"
                    secondary={process.versions.electron}
                  />
                </ListItem>
                <ListItem>
                  <ListItemText
                    primary="Node.js Version"
                    secondary={process.versions.node}
                  />
                </ListItem>
              </List>
            </CardContent>
          </Card>
        </Grid>

        {/* Update Management */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 2 }}>
                <UpdateIcon color="primary" />
                <Typography variant="h6">Update Management</Typography>
              </Stack>
              
              <Box sx={{ mb: 2 }}>
                <Typography variant="body2" sx={{ mb: 1 }}>
                  Current Version: {updateInfo?.currentVersion || 'Unknown'}
                </Typography>
                {updateInfo?.latestVersion && (
                  <Typography variant="body2" sx={{ mb: 1 }}>
                    Latest Version: {updateInfo.latestVersion}
                  </Typography>
                )}
                {updateInfo?.isUpdateAvailable !== undefined && (
                  <Chip
                    icon={updateInfo.isUpdateAvailable ? <WarningIcon /> : <CheckCircleIcon />}
                    label={updateInfo.isUpdateAvailable ? 'Update Available' : 'Up to Date'}
                    color={updateInfo.isUpdateAvailable ? 'warning' : 'success'}
                    size="small"
                    sx={{ mb: 2 }}
                  />
                )}
              </Box>
              
              <Stack spacing={2}>
                <Button
                  variant="contained"
                  startIcon={<RefreshIcon />}
                  onClick={checkForUpdates}
                  disabled={loading}
                  fullWidth
                >
                  Check for Updates
                </Button>
                
                {updateInfo?.isUpdateAvailable && (
                  <Button
                    variant="outlined"
                    startIcon={<DownloadIcon />}
                    onClick={() => setUpdateDialog(true)}
                    fullWidth
                  >
                    Download Update
                  </Button>
                )}
                
                {updateInfo?.releaseNotes && (
                  <Button
                    variant="text"
                    onClick={() => setReleaseNotesDialog(true)}
                    fullWidth
                  >
                    View Release Notes
                  </Button>
                )}
              </Stack>
            </CardContent>
          </Card>
        </Grid>

        {/* Appearance Settings */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 2 }}>
                <SettingsIcon color="primary" />
                <Typography variant="h6">Appearance</Typography>
              </Stack>
              
              <List>
                <ListItem>
                  <ListItemText
                    primary="Theme"
                    secondary={`Current: ${theme === 'dark' ? 'Dark' : 'Light'} mode`}
                  />
                  <ListItemSecondaryAction>
                    <Button
                      variant="outlined"
                      size="small"
                      onClick={onThemeToggle}
                    >
                      Toggle Theme
                    </Button>
                  </ListItemSecondaryAction>
                </ListItem>
              </List>
            </CardContent>
          </Card>
        </Grid>

        {/* Maintenance Tools */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 2 }}>
                <BuildIcon color="primary" />
                <Typography variant="h6">Maintenance Tools</Typography>
              </Stack>
              
              <Typography variant="body2" sx={{ mb: 2, opacity: 0.7 }}>
                Advanced maintenance and diagnostic tools are available via command line:
              </Typography>
              
              <Stack spacing={1}>
                <Box sx={{ p: 1, bgcolor: 'background.default', borderRadius: 1 }}>
                  <Typography variant="body2" fontFamily="monospace">
                    phantomvault-diagnostic --report
                  </Typography>
                </Box>
                <Box sx={{ p: 1, bgcolor: 'background.default', borderRadius: 1 }}>
                  <Typography variant="body2" fontFamily="monospace">
                    phantomvault-maintenance cleanup
                  </Typography>
                </Box>
                <Box sx={{ p: 1, bgcolor: 'background.default', borderRadius: 1 }}>
                  <Typography variant="body2" fontFamily="monospace">
                    phantomvault-updater check
                  </Typography>
                </Box>
              </Stack>
            </CardContent>
          </Card>
        </Grid>

        {/* Security Information */}
        <Grid item xs={12}>
          <Card>
            <CardContent>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 2 }}>
                <SecurityIcon color="primary" />
                <Typography variant="h6">Security Information</Typography>
              </Stack>
              
              <Grid container spacing={2}>
                <Grid item xs={12} md={4}>
                  <Box sx={{ textAlign: 'center', p: 2 }}>
                    <Typography variant="h6" color="success.main">AES-256</Typography>
                    <Typography variant="body2">Encryption Standard</Typography>
                  </Box>
                </Grid>
                <Grid item xs={12} md={4}>
                  <Box sx={{ textAlign: 'center', p: 2 }}>
                    <Typography variant="h6" color="success.main">PBKDF2</Typography>
                    <Typography variant="body2">Key Derivation</Typography>
                  </Box>
                </Grid>
                <Grid item xs={12} md={4}>
                  <Box sx={{ textAlign: 'center', p: 2 }}>
                    <Typography variant="h6" color="success.main">SHA-256</Typography>
                    <Typography variant="body2">Hash Algorithm</Typography>
                  </Box>
                </Grid>
              </Grid>
              
              <Divider sx={{ my: 2 }} />
              
              <Typography variant="body2" sx={{ opacity: 0.7 }}>
                PhantomVault uses military-grade encryption standards to protect your data. 
                All cryptographic operations are performed using industry-standard libraries 
                and follow security best practices.
              </Typography>
            </CardContent>
          </Card>
        </Grid>
      </Grid>

      {/* Update Dialog */}
      <Dialog open={updateDialog} onClose={() => setUpdateDialog(false)} maxWidth="sm" fullWidth>
        <DialogTitle>Update Available</DialogTitle>
        <DialogContent>
          {updateInfo && (
            <Box>
              <Typography variant="body1" sx={{ mb: 2 }}>
                A new version of PhantomVault is available!
              </Typography>
              
              <Box sx={{ mb: 2 }}>
                <Typography variant="body2">
                  Current Version: {updateInfo.currentVersion}
                </Typography>
                <Typography variant="body2">
                  Latest Version: {updateInfo.latestVersion}
                </Typography>
                {updateInfo.publishedAt && (
                  <Typography variant="body2">
                    Released: {formatDate(updateInfo.publishedAt)}
                  </Typography>
                )}
              </Box>
              
              <Alert severity="info" sx={{ mb: 2 }}>
                The update will be downloaded from GitHub. You'll need administrator 
                privileges to install the update.
              </Alert>
            </Box>
          )}
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setUpdateDialog(false)}>Cancel</Button>
          <Button onClick={downloadUpdate} variant="contained" disabled={loading}>
            Download Update
          </Button>
        </DialogActions>
      </Dialog>

      {/* Release Notes Dialog */}
      <Dialog open={releaseNotesDialog} onClose={() => setReleaseNotesDialog(false)} maxWidth="md" fullWidth>
        <DialogTitle>Release Notes</DialogTitle>
        <DialogContent>
          <Box sx={{ whiteSpace: 'pre-wrap', fontFamily: 'monospace', fontSize: '0.875rem' }}>
            {updateInfo?.releaseNotes || 'No release notes available.'}
          </Box>
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setReleaseNotesDialog(false)}>Close</Button>
        </DialogActions>
      </Dialog>
    </Box>
  );
};

export default Settings;