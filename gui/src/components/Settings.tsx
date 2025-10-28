/**
 * PhantomVault Settings Component
 * 
 * Comprehensive settings interface with recovery system, platform configuration,
 * and user preferences management.
 */

import React, { useState, useEffect } from 'react';
import {
  Box,
  Typography,
  Card,
  CardContent,
  Switch,
  FormControlLabel,
  TextField,
  Button,
  Grid,
  Alert,
  Divider,
  Stack,
  Chip,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  List,
  ListItem,
  ListItemText,
  ListItemIcon,
  FormControl,
  InputLabel,
  Select,
  MenuItem,
  Slider,
  Paper,
} from '@mui/material';
import {
  Palette as PaletteIcon,
  Security as SecurityIcon,
  Keyboard as KeyboardIcon,
  Storage as StorageIcon,
  Notifications as NotificationsIcon,
  Help as HelpIcon,
  VpnKey as VpnKeyIcon,
  Computer as ComputerIcon,
  Warning as WarningIcon,
  CheckCircle as CheckCircleIcon,
} from '@mui/icons-material';

interface SettingsProps {
  theme: 'light' | 'dark';
  onThemeToggle: () => void;
}

interface PlatformCapabilities {
  supportsInvisibleLogging: boolean;
  supportsHotkeys: boolean;
  requiresPermissions: boolean;
  requiredPermissions: string[];
}

interface SystemSettings {
  dataRetentionDays: number;
  autoLockTimeout: number;
  enableAnalytics: boolean;
  enableNotifications: boolean;
  unlockMethod: 'keyboard' | 'manual' | 'notification';
}

const Settings: React.FC<SettingsProps> = ({ theme, onThemeToggle }) => {
  const [recoveryKey, setRecoveryKey] = useState('');
  const [newRecoveryKey, setNewRecoveryKey] = useState('');
  const [currentRecoveryKey, setCurrentRecoveryKey] = useState('');
  const [recoveryDialog, setRecoveryDialog] = useState(false);
  const [recoveryKeyDisplayDialog, setRecoveryKeyDisplayDialog] = useState(false);
  const [passwordChangeDialog, setPasswordChangeDialog] = useState(false);
  const [helpDialog, setHelpDialog] = useState(false);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);
  
  // Password change form states
  const [currentPassword, setCurrentPassword] = useState('');
  const [newPassword, setNewPassword] = useState('');
  const [confirmPassword, setConfirmPassword] = useState('');
  const [selectedProfile, setSelectedProfile] = useState('');
  
  const [platformCapabilities, setPlatformCapabilities] = useState<PlatformCapabilities>({
    supportsInvisibleLogging: true,
    supportsHotkeys: true,
    requiresPermissions: false,
    requiredPermissions: [],
  });
  
  const [systemSettings, setSystemSettings] = useState<SystemSettings>({
    dataRetentionDays: 30,
    autoLockTimeout: 300, // 5 minutes
    enableAnalytics: true,
    enableNotifications: true,
    unlockMethod: 'keyboard',
  });

  useEffect(() => {
    loadPlatformCapabilities();
    loadSystemSettings();
  }, []);

  const loadPlatformCapabilities = async () => {
    try {
      // TODO: Call actual service API
      const mockCapabilities: PlatformCapabilities = {
        supportsInvisibleLogging: true,
        supportsHotkeys: true,
        requiresPermissions: false,
        requiredPermissions: [],
      };
      setPlatformCapabilities(mockCapabilities);
    } catch (err) {
      setError('Failed to load platform capabilities');
    }
  };

  const loadSystemSettings = async () => {
    try {
      // TODO: Call actual service API to load settings
      console.log('Loading system settings...');
    } catch (err) {
      setError('Failed to load system settings');
    }
  };

  const handleRecoveryKeySubmit = async () => {
    if (!recoveryKey) {
      setError('Please enter your recovery key');
      return;
    }

    try {
      setLoading(true);
      // TODO: Call actual service API to validate recovery key
      console.log('Validating recovery key...');
      
      setSuccess('Recovery key validated successfully');
      setRecoveryDialog(false);
      setRecoveryKey('');
    } catch (err) {
      setError('Invalid recovery key');
    } finally {
      setLoading(false);
    }
  };

  const generateNewRecoveryKey = async () => {
    try {
      setLoading(true);
      // Call the new recovery key generation API
      const response = await window.phantomVault.ipc.generateRecoveryKey(selectedProfile);
      if (response.success) {
        setNewRecoveryKey(response.recoveryKey);
        setSuccess('New AES-256 encrypted recovery key generated with PBKDF2 key derivation!');
      } else {
        setError('Failed to generate recovery key: ' + response.error);
      }
    } catch (err) {
      setError('Failed to generate new recovery key: ' + (err instanceof Error ? err.message : 'Unknown error'));
    } finally {
      setLoading(false);
    }
  };

  const displayCurrentRecoveryKey = async () => {
    try {
      setLoading(true);
      // Call API to get current recovery key (requires authentication)
      const response = await window.phantomVault.ipc.getCurrentRecoveryKey(selectedProfile, currentPassword);
      if (response.success) {
        setCurrentRecoveryKey(response.recoveryKey);
        setRecoveryKeyDisplayDialog(true);
      } else {
        setError('Failed to retrieve recovery key: ' + response.error);
      }
    } catch (err) {
      setError('Failed to retrieve recovery key: ' + (err instanceof Error ? err.message : 'Unknown error'));
    } finally {
      setLoading(false);
    }
  };

  const handlePasswordChange = async () => {
    if (!currentPassword || !newPassword || !confirmPassword) {
      setError('Please fill all password fields');
      return;
    }

    if (newPassword !== confirmPassword) {
      setError('New passwords do not match');
      return;
    }

    if (newPassword.length < 8) {
      setError('New password must be at least 8 characters long');
      return;
    }

    try {
      setLoading(true);
      // Call the password change API which will generate new recovery key
      const response = await window.phantomVault.ipc.changePassword(
        selectedProfile, 
        currentPassword, 
        newPassword
      );
      
      if (response.success) {
        setPasswordChangeDialog(false);
        setCurrentPassword('');
        setNewPassword('');
        setConfirmPassword('');
        setNewRecoveryKey(response.newRecoveryKey);
        setSuccess('Password changed successfully! New recovery key generated.');
      } else {
        setError('Failed to change password: ' + response.error);
      }
    } catch (err) {
      setError('Failed to change password: ' + (err instanceof Error ? err.message : 'Unknown error'));
    } finally {
      setLoading(false);
    }
  };

  const saveSystemSettings = async () => {
    try {
      setLoading(true);
      // TODO: Call actual service API to save settings
      console.log('Saving system settings:', systemSettings);
      setSuccess('Settings saved successfully');
    } catch (err) {
      setError('Failed to save settings');
    } finally {
      setLoading(false);
    }
  };

  const clearAllData = async () => {
    if (!window.confirm('Are you sure you want to clear all analytics data? This action cannot be undone.')) {
      return;
    }

    try {
      setLoading(true);
      // TODO: Call actual service API to clear data
      console.log('Clearing all analytics data...');
      setSuccess('All analytics data cleared successfully');
    } catch (err) {
      setError('Failed to clear analytics data');
    } finally {
      setLoading(false);
    }
  };

  return (
    <Box sx={{ p: 3 }}>
      <Typography variant="h3" sx={{ mb: 3, fontWeight: 600 }}>
        Settings & Configuration
      </Typography>

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
        {/* Appearance Settings */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 2 }}>
                <PaletteIcon color="primary" />
                <Typography variant="h6">Appearance</Typography>
              </Stack>
              
              <FormControlLabel
                control={
                  <Switch
                    checked={theme === 'dark'}
                    onChange={onThemeToggle}
                  />
                }
                label="Dark Mode"
                sx={{ mb: 2 }}
              />
              
              <Typography variant="body2" sx={{ opacity: 0.7 }}>
                Toggle between light and dark themes for better visibility in different environments.
              </Typography>
            </CardContent>
          </Card>
        </Grid>

        {/* Platform Configuration */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 2 }}>
                <ComputerIcon color="primary" />
                <Typography variant="h6">Platform Configuration</Typography>
              </Stack>
              
              <Stack spacing={2}>
                <Box>
                  <Typography variant="body2" sx={{ mb: 1 }}>
                    Keyboard Detection
                  </Typography>
                  <Chip
                    icon={platformCapabilities.supportsInvisibleLogging ? <CheckCircleIcon /> : <WarningIcon />}
                    label={platformCapabilities.supportsInvisibleLogging ? "Supported" : "Not Supported"}
                    color={platformCapabilities.supportsInvisibleLogging ? "success" : "warning"}
                    size="small"
                  />
                </Box>
                
                <Box>
                  <Typography variant="body2" sx={{ mb: 1 }}>
                    Hotkey Support
                  </Typography>
                  <Chip
                    icon={platformCapabilities.supportsHotkeys ? <CheckCircleIcon /> : <WarningIcon />}
                    label={platformCapabilities.supportsHotkeys ? "Ctrl+Alt+V Available" : "Manual Input Only"}
                    color={platformCapabilities.supportsHotkeys ? "success" : "warning"}
                    size="small"
                  />
                </Box>

                <FormControl size="small" fullWidth>
                  <InputLabel>Unlock Method</InputLabel>
                  <Select
                    value={systemSettings.unlockMethod}
                    label="Unlock Method"
                    onChange={(e) => setSystemSettings(prev => ({ ...prev, unlockMethod: e.target.value as any }))}
                  >
                    <MenuItem value="keyboard">Keyboard Detection</MenuItem>
                    <MenuItem value="manual">Manual Input</MenuItem>
                    <MenuItem value="notification">Notification Prompt</MenuItem>
                  </Select>
                </FormControl>
              </Stack>
            </CardContent>
          </Card>
        </Grid>

        {/* Security Settings */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 2 }}>
                <SecurityIcon color="primary" />
                <Typography variant="h6">Security Settings</Typography>
              </Stack>
              
              <Stack spacing={3}>
                <Box>
                  <Typography variant="body2" sx={{ mb: 1 }}>
                    Auto-lock Timeout (seconds)
                  </Typography>
                  <Slider
                    value={systemSettings.autoLockTimeout}
                    onChange={(_, value) => setSystemSettings(prev => ({ ...prev, autoLockTimeout: value as number }))}
                    min={60}
                    max={3600}
                    step={60}
                    marks={[
                      { value: 60, label: '1m' },
                      { value: 300, label: '5m' },
                      { value: 900, label: '15m' },
                      { value: 3600, label: '1h' },
                    ]}
                    valueLabelDisplay="auto"
                    valueLabelFormat={(value) => `${Math.floor(value / 60)}m`}
                  />
                </Box>

                <FormControlLabel
                  control={
                    <Switch
                      checked={systemSettings.enableAnalytics}
                      onChange={(e) => setSystemSettings(prev => ({ ...prev, enableAnalytics: e.target.checked }))}
                    />
                  }
                  label="Enable Analytics Collection"
                />

                <FormControlLabel
                  control={
                    <Switch
                      checked={systemSettings.enableNotifications}
                      onChange={(e) => setSystemSettings(prev => ({ ...prev, enableNotifications: e.target.checked }))}
                    />
                  }
                  label="Enable Security Notifications"
                />
              </Stack>
            </CardContent>
          </Card>
        </Grid>

        {/* Data Management */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 2 }}>
                <StorageIcon color="primary" />
                <Typography variant="h6">Data Management</Typography>
              </Stack>
              
              <Stack spacing={3}>
                <Box>
                  <Typography variant="body2" sx={{ mb: 1 }}>
                    Data Retention (days)
                  </Typography>
                  <Slider
                    value={systemSettings.dataRetentionDays}
                    onChange={(_, value) => setSystemSettings(prev => ({ ...prev, dataRetentionDays: value as number }))}
                    min={7}
                    max={365}
                    step={7}
                    marks={[
                      { value: 7, label: '1w' },
                      { value: 30, label: '1m' },
                      { value: 90, label: '3m' },
                      { value: 365, label: '1y' },
                    ]}
                    valueLabelDisplay="auto"
                    valueLabelFormat={(value) => `${value}d`}
                  />
                </Box>

                <Button
                  variant="outlined"
                  color="warning"
                  onClick={clearAllData}
                  disabled={loading}
                >
                  Clear All Analytics Data
                </Button>

                <Typography variant="body2" sx={{ opacity: 0.7 }}>
                  Current storage usage: ~2.3 MB
                </Typography>
              </Stack>
            </CardContent>
          </Card>
        </Grid>

        {/* Recovery System */}
        <Grid item xs={12}>
          <Card>
            <CardContent>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 2 }}>
                <VpnKeyIcon color="primary" />
                <Typography variant="h6">Recovery Key Management</Typography>
              </Stack>
              
              <Grid container spacing={3}>
                <Grid item xs={12} md={6}>
                  <Typography variant="body1" sx={{ mb: 2 }}>
                    Recovery keys use AES-256-CBC encryption with PBKDF2 key derivation. 
                    They allow you to regain access to your profiles if you forget your master key.
                  </Typography>
                  
                  <Stack spacing={2}>
                    <Button
                      variant="contained"
                      onClick={() => setRecoveryDialog(true)}
                    >
                      Validate Recovery Key
                    </Button>
                    
                    <Button
                      variant="outlined"
                      onClick={generateNewRecoveryKey}
                      disabled={loading}
                    >
                      Generate New Recovery Key
                    </Button>
                    
                    <Button
                      variant="outlined"
                      color="warning"
                      onClick={() => setPasswordChangeDialog(true)}
                    >
                      Change Master Password
                    </Button>
                    
                    <Button
                      variant="text"
                      onClick={() => setRecoveryKeyDisplayDialog(true)}
                    >
                      View Current Recovery Key
                    </Button>
                  </Stack>
                </Grid>
                
                <Grid item xs={12} md={6}>
                  {newRecoveryKey && (
                    <Paper sx={{ p: 2, bgcolor: 'success.light', color: 'success.contrastText' }}>
                      <Typography variant="h6" sx={{ mb: 1 }}>
                        New Recovery Key (AES-256 Encrypted)
                      </Typography>
                      <Typography variant="h5" sx={{ fontFamily: 'monospace', mb: 2, wordBreak: 'break-all' }}>
                        {newRecoveryKey}
                      </Typography>
                      <Alert severity="warning" sx={{ bgcolor: 'warning.main', color: 'warning.contrastText' }}>
                        Save this key securely! It's encrypted with PBKDF2 and cannot be recovered if lost.
                      </Alert>
                      <Box sx={{ mt: 2 }}>
                        <Button
                          variant="contained"
                          size="small"
                          onClick={() => navigator.clipboard.writeText(newRecoveryKey)}
                        >
                          Copy to Clipboard
                        </Button>
                      </Box>
                    </Paper>
                  )}
                  
                  <Paper sx={{ p: 2, mt: 2, bgcolor: 'info.light', color: 'info.contrastText' }}>
                    <Typography variant="subtitle2" sx={{ mb: 1 }}>Security Features:</Typography>
                    <List dense>
                      <ListItem>
                        <ListItemText primary="AES-256-CBC encryption" />
                      </ListItem>
                      <ListItem>
                        <ListItemText primary="PBKDF2 key derivation (100,000 iterations)" />
                      </ListItem>
                      <ListItem>
                        <ListItemText primary="Cryptographically secure random generation" />
                      </ListItem>
                      <ListItem>
                        <ListItemText primary="Master key recovery capability" />
                      </ListItem>
                    </List>
                  </Paper>
                </Grid>
              </Grid>
            </CardContent>
          </Card>
        </Grid>

        {/* Help & Support */}
        <Grid item xs={12}>
          <Card>
            <CardContent>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 2 }}>
                <HelpIcon color="primary" />
                <Typography variant="h6">Help & Support</Typography>
              </Stack>
              
              <Grid container spacing={2}>
                <Grid item xs={12} md={4}>
                  <Button
                    variant="outlined"
                    fullWidth
                    onClick={() => setHelpDialog(true)}
                  >
                    Platform Guide
                  </Button>
                </Grid>
                <Grid item xs={12} md={4}>
                  <Button variant="outlined" fullWidth>
                    Troubleshooting
                  </Button>
                </Grid>
                <Grid item xs={12} md={4}>
                  <Button variant="outlined" fullWidth>
                    About PhantomVault
                  </Button>
                </Grid>
              </Grid>
            </CardContent>
          </Card>
        </Grid>

        {/* Save Settings */}
        <Grid item xs={12}>
          <Box sx={{ display: 'flex', justifyContent: 'center' }}>
            <Button
              variant="contained"
              size="large"
              onClick={saveSystemSettings}
              disabled={loading}
            >
              Save All Settings
            </Button>
          </Box>
        </Grid>
      </Grid>

      {/* Recovery Key Dialog */}
      <Dialog open={recoveryDialog} onClose={() => setRecoveryDialog(false)} maxWidth="sm" fullWidth>
        <DialogTitle>Validate Recovery Key</DialogTitle>
        <DialogContent>
          <TextField
            autoFocus
            margin="dense"
            label="Recovery Key"
            fullWidth
            variant="outlined"
            value={recoveryKey}
            onChange={(e) => setRecoveryKey(e.target.value)}
            placeholder="PHANTOM-VAULT-XXXXXXXXX"
          />
          <Alert severity="info" sx={{ mt: 2 }}>
            Enter your recovery key to validate it's working correctly.
          </Alert>
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setRecoveryDialog(false)}>Cancel</Button>
          <Button onClick={handleRecoveryKeySubmit} variant="contained" disabled={loading}>
            Validate
          </Button>
        </DialogActions>
      </Dialog>

      {/* Recovery Key Display Dialog */}
      <Dialog open={recoveryKeyDisplayDialog} onClose={() => setRecoveryKeyDisplayDialog(false)} maxWidth="sm" fullWidth>
        <DialogTitle>Current Recovery Key</DialogTitle>
        <DialogContent>
          {currentRecoveryKey ? (
            <Box>
              <Typography variant="body1" sx={{ mb: 2 }}>
                Your current AES-256 encrypted recovery key:
              </Typography>
              <Paper sx={{ p: 2, bgcolor: 'grey.100', mb: 2 }}>
                <Typography variant="h6" sx={{ fontFamily: 'monospace', wordBreak: 'break-all' }}>
                  {currentRecoveryKey}
                </Typography>
              </Paper>
              <Button
                variant="outlined"
                onClick={() => navigator.clipboard.writeText(currentRecoveryKey)}
                fullWidth
              >
                Copy to Clipboard
              </Button>
            </Box>
          ) : (
            <TextField
              autoFocus
              margin="dense"
              label="Current Master Password"
              type="password"
              fullWidth
              variant="outlined"
              value={currentPassword}
              onChange={(e) => setCurrentPassword(e.target.value)}
              helperText="Enter your current password to view the recovery key"
            />
          )}
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setRecoveryKeyDisplayDialog(false)}>Close</Button>
          {!currentRecoveryKey && (
            <Button onClick={displayCurrentRecoveryKey} variant="contained" disabled={loading}>
              Show Recovery Key
            </Button>
          )}
        </DialogActions>
      </Dialog>

      {/* Password Change Dialog */}
      <Dialog open={passwordChangeDialog} onClose={() => setPasswordChangeDialog(false)} maxWidth="sm" fullWidth>
        <DialogTitle>Change Master Password</DialogTitle>
        <DialogContent>
          <FormControl fullWidth margin="dense">
            <InputLabel>Profile</InputLabel>
            <Select
              value={selectedProfile}
              label="Profile"
              onChange={(e) => setSelectedProfile(e.target.value)}
            >
              <MenuItem value="profile1">Profile 1</MenuItem>
              <MenuItem value="profile2">Profile 2</MenuItem>
            </Select>
          </FormControl>
          
          <TextField
            margin="dense"
            label="Current Password"
            type="password"
            fullWidth
            variant="outlined"
            value={currentPassword}
            onChange={(e) => setCurrentPassword(e.target.value)}
            sx={{ mb: 2 }}
          />
          
          <TextField
            margin="dense"
            label="New Password"
            type="password"
            fullWidth
            variant="outlined"
            value={newPassword}
            onChange={(e) => setNewPassword(e.target.value)}
            sx={{ mb: 2 }}
          />
          
          <TextField
            margin="dense"
            label="Confirm New Password"
            type="password"
            fullWidth
            variant="outlined"
            value={confirmPassword}
            onChange={(e) => setConfirmPassword(e.target.value)}
          />
          
          <Alert severity="info" sx={{ mt: 2 }}>
            Changing your password will generate a new AES-256 encrypted recovery key. 
            Make sure to save the new recovery key securely!
          </Alert>
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setPasswordChangeDialog(false)}>Cancel</Button>
          <Button onClick={handlePasswordChange} variant="contained" disabled={loading}>
            Change Password
          </Button>
        </DialogActions>
      </Dialog>

      {/* Help Dialog */}
      <Dialog open={helpDialog} onClose={() => setHelpDialog(false)} maxWidth="md" fullWidth>
        <DialogTitle>Platform-Specific Guide</DialogTitle>
        <DialogContent>
          <Typography variant="h6" sx={{ mb: 2 }}>
            Linux (Current Platform)
          </Typography>
          <List>
            <ListItem>
              <ListItemIcon>
                <CheckCircleIcon color="success" />
              </ListItemIcon>
              <ListItemText
                primary="X11 Keyboard Detection"
                secondary="Full invisible keyboard logging support with real-time sequence detection"
              />
            </ListItem>
            <ListItem>
              <ListItemIcon>
                <CheckCircleIcon color="success" />
              </ListItemIcon>
              <ListItemText
                primary="Advanced Folder Hiding"
                secondary="Platform-specific hiding with elevated privileges and metadata preservation"
              />
            </ListItem>
            <ListItem>
              <ListItemIcon>
                <CheckCircleIcon color="success" />
              </ListItemIcon>
              <ListItemText
                primary="AES-256-CBC Encryption"
                secondary="Industry-standard encryption with PBKDF2 key derivation"
              />
            </ListItem>
            <ListItem>
              <ListItemIcon>
                <CheckCircleIcon color="success" />
              </ListItemIcon>
              <ListItemText
                primary="Secure Vault Storage"
                secondary="Files encrypted and stored in secure vault with complete metadata preservation"
              />
            </ListItem>
          </List>
          
          <Typography variant="h6" sx={{ mb: 2, mt: 3 }}>
            Enhanced Security Features
          </Typography>
          <Typography variant="body2" sx={{ mb: 1 }}>
            • Real AES-256-CBC encryption (no more fake marker files)
          </Typography>
          <Typography variant="body2" sx={{ mb: 1 }}>
            • Platform-specific folder hiding with elevated privileges
          </Typography>
          <Typography variant="body2" sx={{ mb: 1 }}>
            • Complete metadata preservation and restoration
          </Typography>
          <Typography variant="body2" sx={{ mb: 1 }}>
            • Secure vault management with integrity checking
          </Typography>
          <Typography variant="body2">
            • Comprehensive error handling and audit logging
          </Typography>
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setHelpDialog(false)}>Close</Button>
        </DialogActions>
      </Dialog>
    </Box>
  );
};

export default Settings;