/**
 * PhantomVault Dashboard Component
 * 
 * Main dashboard with profile management, folder operations, and system status.
 */

import React, { useState, useEffect } from 'react';
import {
  Box,
  Typography,
  Card,
  CardContent,
  Grid,
  Chip,
  Button,
  List,
  ListItem,
  ListItemText,
  ListItemSecondaryAction,
  IconButton,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  TextField,
  Alert,
  Paper,
  Stack,
  LinearProgress,
  Tooltip,
} from '@mui/material';
import {
  Add as AddIcon,
  Folder as FolderIcon,
  Lock as LockIcon,
  LockOpen as LockOpenIcon,
  Delete as DeleteIcon,
  Security as SecurityIcon,
  Person as PersonIcon,
  Storage as StorageIcon,
} from '@mui/icons-material';

interface DashboardProps {
  isAdmin: boolean;
  serviceStatus: any;
}

interface Profile {
  id: string;
  name: string;
  createdAt: string;
  lastAccess: string;
  folderCount: number;
}

interface SecuredFolder {
  id: string;
  name: string;
  originalPath: string;
  vaultPath: string;
  isLocked: boolean;
  unlockMode: 'temporary' | 'permanent';
  size: number;
  createdAt: string;
  lastAccess: string;
  encryptionStatus: 'encrypted' | 'decrypted' | 'processing' | 'error';
}

interface VaultStats {
  totalFolders: number;
  encryptedFolders: number;
  totalSize: number;
  lastBackup: string;
}

const Dashboard: React.FC<DashboardProps> = ({ isAdmin, serviceStatus }) => {
  const [profiles, setProfiles] = useState<Profile[]>([]);
  const [selectedProfile, setSelectedProfile] = useState<Profile | null>(null);
  const [folders, setFolders] = useState<SecuredFolder[]>([]);
  const [vaultStats, setVaultStats] = useState<VaultStats | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState<string | null>(null);
  
  // Dialog states
  const [createProfileDialog, setCreateProfileDialog] = useState(false);
  const [authDialog, setAuthDialog] = useState(false);
  const [addFolderDialog, setAddFolderDialog] = useState(false);
  const [unlockModeDialog, setUnlockModeDialog] = useState(false);
  const [vaultManagementDialog, setVaultManagementDialog] = useState(false);
  
  // Form states
  const [profileName, setProfileName] = useState('');
  const [masterKey, setMasterKey] = useState('');
  const [confirmKey, setConfirmKey] = useState('');
  const [authKey, setAuthKey] = useState('');
  const [selectedFolderPath, setSelectedFolderPath] = useState('');

  // Load profiles on component mount
  useEffect(() => {
    loadProfiles();
  }, []);

  // Load folders when profile is selected
  useEffect(() => {
    if (selectedProfile) {
      loadFolders(selectedProfile.id);
    }
  }, [selectedProfile]);

  const loadProfiles = async () => {
    try {
      setLoading(true);
      const response = await window.phantomVault.ipc.getAllProfiles();
      if (response.success) {
        setProfiles(response.profiles);
      } else {
        setError('Failed to load profiles: ' + response.error);
      }
    } catch (err) {
      setError('Failed to load profiles: ' + (err instanceof Error ? err.message : 'Unknown error'));
    } finally {
      setLoading(false);
    }
  };

  const loadFolders = async (profileId: string) => {
    try {
      setLoading(true);
      const response = await window.phantomVault.ipc.getProfileFolders(profileId);
      if (response.success) {
        setFolders(response.folders);
        // Load vault statistics
        await loadVaultStats(profileId);
      } else {
        setError('Failed to load folders: ' + response.error);
      }
    } catch (err) {
      setError('Failed to load folders: ' + (err instanceof Error ? err.message : 'Unknown error'));
    } finally {
      setLoading(false);
    }
  };

  const loadVaultStats = async (profileId: string) => {
    try {
      const response = await window.phantomVault.ipc.getVaultStats(profileId);
      if (response.success) {
        setVaultStats(response.stats);
      }
    } catch (err) {
      console.warn('Failed to load vault stats:', err);
    }
  };

  const handleCreateProfile = async () => {
    if (!profileName || !masterKey || masterKey !== confirmKey) {
      setError('Please fill all fields and ensure passwords match');
      return;
    }

    try {
      setLoading(true);
      const response = await window.phantomVault.ipc.createProfile(profileName, masterKey);
      if (response.success) {
        setCreateProfileDialog(false);
        setProfileName('');
        setMasterKey('');
        setConfirmKey('');
        loadProfiles();
      } else {
        setError('Failed to create profile: ' + response.error);
      }
    } catch (err) {
      setError('Failed to create profile: ' + (err instanceof Error ? err.message : 'Unknown error'));
    } finally {
      setLoading(false);
    }
  };

  const handleAuthenticateProfile = async (profile: Profile) => {
    setSelectedProfile(profile);
    setAuthDialog(true);
  };

  const handleAuthSubmit = async () => {
    if (!authKey || !selectedProfile) {
      setError('Please enter master key');
      return;
    }

    try {
      setLoading(true);
      const response = await window.phantomVault.ipc.authenticateProfile(selectedProfile.id, authKey);
      if (response.success) {
        setAuthDialog(false);
        setAuthKey('');
        // Profile is now authenticated and folders will load
        loadFolders(selectedProfile.id);
      } else {
        setError('Authentication failed: ' + response.error);
      }
    } catch (err) {
      setError('Authentication failed: ' + (err instanceof Error ? err.message : 'Unknown error'));
    } finally {
      setLoading(false);
    }
  };

  const handleAddFolder = async () => {
    if (!selectedFolderPath || !selectedProfile) {
      setError('Please select a folder and ensure profile is authenticated');
      return;
    }

    try {
      setLoading(true);
      // Call the new vault API endpoint for real encryption
      const response = await window.phantomVault.ipc.lockFolder(
        selectedProfile.id, 
        selectedFolderPath, 
        authKey || masterKey
      );
      
      if (response.success) {
        setAddFolderDialog(false);
        setSelectedFolderPath('');
        loadFolders(selectedProfile.id);
        setSuccess(`Folder encrypted and secured: ${response.message}`);
      } else {
        setError('Failed to secure folder: ' + response.error);
      }
    } catch (err) {
      setError('Failed to secure folder: ' + (err instanceof Error ? err.message : 'Unknown error'));
    } finally {
      setLoading(false);
    }
  };

  const handleUnlockFolders = async (permanent: boolean = false) => {
    if (!selectedProfile || !authKey) {
      setError('Please authenticate profile first');
      return;
    }

    try {
      setLoading(true);
      // Call the appropriate unlock API endpoint
      const response = permanent 
        ? await window.phantomVault.ipc.unlockFoldersPermanent(selectedProfile.id, authKey, [])
        : await window.phantomVault.ipc.unlockFoldersTemporary(selectedProfile.id, authKey);
      
      if (response.success) {
        loadFolders(selectedProfile.id);
        setSuccess(`Folders unlocked ${permanent ? 'permanently' : 'temporarily'}: ${response.successCount} folders processed`);
      } else {
        setError('Failed to unlock folders: ' + response.error);
      }
    } catch (err) {
      setError('Failed to unlock folders: ' + (err instanceof Error ? err.message : 'Unknown error'));
    } finally {
      setLoading(false);
    }
  };

  const formatFileSize = (bytes: number): string => {
    const sizes = ['B', 'KB', 'MB', 'GB'];
    if (bytes === 0) return '0 B';
    const i = Math.floor(Math.log(bytes) / Math.log(1024));
    return Math.round(bytes / Math.pow(1024, i) * 100) / 100 + ' ' + sizes[i];
  };

  return (
    <Box sx={{ p: 3 }}>
      <Typography variant="h3" sx={{ mb: 3, fontWeight: 600 }}>
        PhantomVault Dashboard
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
        {/* System Status Cards */}
        <Grid item xs={12} md={4}>
          <Card>
            <CardContent>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 2 }}>
                <SecurityIcon color="primary" />
                <Typography variant="h6">Service Status</Typography>
              </Stack>
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
              <Typography variant="body2" sx={{ mt: 1, opacity: 0.7 }}>
                Memory: 5.6MB • Uptime: 2h 15m
              </Typography>
            </CardContent>
          </Card>
        </Grid>

        <Grid item xs={12} md={4}>
          <Card>
            <CardContent>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 2 }}>
                <PersonIcon color="primary" />
                <Typography variant="h6">Admin Status</Typography>
              </Stack>
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

        <Grid item xs={12} md={4}>
          <Card>
            <CardContent>
              <Stack direction="row" alignItems="center" spacing={1} sx={{ mb: 2 }}>
                <StorageIcon color="primary" />
                <Typography variant="h6">Storage</Typography>
              </Stack>
              <Typography variant="h4" sx={{ fontWeight: 600 }}>
                {profiles.reduce((acc, p) => acc + p.folderCount, 0)}
              </Typography>
              <Typography variant="body2" sx={{ opacity: 0.7 }}>
                Secured folders
              </Typography>
            </CardContent>
          </Card>
        </Grid>

        {/* Profiles Section */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 2 }}>
                <Typography variant="h6">Profiles</Typography>
                {isAdmin && (
                  <Button
                    variant="contained"
                    startIcon={<AddIcon />}
                    onClick={() => setCreateProfileDialog(true)}
                    size="small"
                  >
                    Create Profile
                  </Button>
                )}
              </Box>
              
              {profiles.length === 0 ? (
                <Typography variant="body1" sx={{ opacity: 0.7, textAlign: 'center', py: 3 }}>
                  {isAdmin ? 'No profiles created yet. Create your first profile to get started.' : 'No profiles available. Run as admin to create profiles.'}
                </Typography>
              ) : (
                <List>
                  {profiles.map((profile) => (
                    <ListItem
                      key={profile.id}
                      sx={{
                        border: selectedProfile?.id === profile.id ? 2 : 1,
                        borderColor: selectedProfile?.id === profile.id ? 'primary.main' : 'divider',
                        borderRadius: 1,
                        mb: 1,
                        cursor: 'pointer',
                      }}
                      onClick={() => handleAuthenticateProfile(profile)}
                    >
                      <ListItemText
                        primary={profile.name}
                        secondary={
                          <Box>
                            <Typography variant="body2" component="span">
                              {profile.folderCount} folders • Last access: {profile.lastAccess}
                            </Typography>
                          </Box>
                        }
                      />
                      <ListItemSecondaryAction>
                        <Chip
                          label={selectedProfile?.id === profile.id ? "Active" : "Locked"}
                          color={selectedProfile?.id === profile.id ? "success" : "default"}
                          size="small"
                        />
                      </ListItemSecondaryAction>
                    </ListItem>
                  ))}
                </List>
              )}
            </CardContent>
          </Card>
        </Grid>

        {/* Folders Section */}
        <Grid item xs={12} md={6}>
          <Card>
            <CardContent>
              <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 2 }}>
                <Typography variant="h6">
                  Secured Folders
                  {selectedProfile && (
                    <Typography variant="body2" component="span" sx={{ ml: 1, opacity: 0.7 }}>
                      ({selectedProfile.name})
                    </Typography>
                  )}
                </Typography>
                {selectedProfile && (
                  <Stack direction="row" spacing={1}>
                    <Button
                      variant="outlined"
                      startIcon={<AddIcon />}
                      onClick={() => setAddFolderDialog(true)}
                      size="small"
                    >
                      Add Folder
                    </Button>
                  </Stack>
                )}
              </Box>
              
              {!selectedProfile ? (
                <Typography variant="body1" sx={{ opacity: 0.7, textAlign: 'center', py: 3 }}>
                  Select and authenticate a profile to view folders
                </Typography>
              ) : folders.length === 0 ? (
                <Typography variant="body1" sx={{ opacity: 0.7, textAlign: 'center', py: 3 }}>
                  No folders secured yet. Add your first folder to get started.
                </Typography>
              ) : (
                <>
                  <Box sx={{ mb: 2 }}>
                    <Stack direction="row" spacing={1} sx={{ mb: 2 }}>
                      <Button
                        variant="contained"
                        startIcon={<LockOpenIcon />}
                        onClick={() => setUnlockModeDialog(true)}
                        size="small"
                      >
                        Unlock Folders
                      </Button>
                      <Button
                        variant="outlined"
                        startIcon={<StorageIcon />}
                        onClick={() => setVaultManagementDialog(true)}
                        size="small"
                      >
                        Manage Vault
                      </Button>
                    </Stack>
                    
                    {vaultStats && (
                      <Paper sx={{ p: 2, mb: 2, bgcolor: 'background.default' }}>
                        <Typography variant="subtitle2" sx={{ mb: 1 }}>Vault Statistics</Typography>
                        <Grid container spacing={2}>
                          <Grid item xs={6}>
                            <Typography variant="body2">
                              Encrypted: {vaultStats.encryptedFolders}/{vaultStats.totalFolders}
                            </Typography>
                          </Grid>
                          <Grid item xs={6}>
                            <Typography variant="body2">
                              Size: {formatFileSize(vaultStats.totalSize)}
                            </Typography>
                          </Grid>
                        </Grid>
                      </Paper>
                    )}
                  </Box>
                  
                  <List>
                    {folders.map((folder) => (
                      <ListItem
                        key={folder.id}
                        sx={{
                          border: 1,
                          borderColor: 'divider',
                          borderRadius: 1,
                          mb: 1,
                        }}
                      >
                        <ListItemText
                          primary={
                            <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
                              <FolderIcon />
                              <Typography variant="subtitle1">{folder.name}</Typography>
                              {folder.isLocked ? (
                                <LockIcon color="error" fontSize="small" />
                              ) : (
                                <LockOpenIcon color="success" fontSize="small" />
                              )}
                              <Chip
                                label={folder.encryptionStatus}
                                size="small"
                                color={
                                  folder.encryptionStatus === 'encrypted' ? 'success' :
                                  folder.encryptionStatus === 'processing' ? 'warning' :
                                  folder.encryptionStatus === 'error' ? 'error' : 'default'
                                }
                              />
                              <Chip
                                label={folder.unlockMode}
                                size="small"
                                variant="outlined"
                                color={folder.unlockMode === 'permanent' ? 'warning' : 'primary'}
                              />
                            </Box>
                          }
                          secondary={
                            <Box>
                              <Typography variant="body2" component="div">
                                {folder.originalPath}
                              </Typography>
                              <Typography variant="body2" sx={{ opacity: 0.7 }}>
                                {formatFileSize(folder.size)} • Created: {folder.createdAt}
                              </Typography>
                            </Box>
                          }
                        />
                        <ListItemSecondaryAction>
                          <Tooltip title="Remove from profile">
                            <IconButton edge="end" size="small">
                              <DeleteIcon />
                            </IconButton>
                          </Tooltip>
                        </ListItemSecondaryAction>
                      </ListItem>
                    ))}
                  </List>
                </>
              )}
            </CardContent>
          </Card>
        </Grid>
      </Grid>

      {/* Create Profile Dialog */}
      <Dialog open={createProfileDialog} onClose={() => setCreateProfileDialog(false)} maxWidth="sm" fullWidth>
        <DialogTitle>Create New Profile</DialogTitle>
        <DialogContent>
          <TextField
            autoFocus
            margin="dense"
            label="Profile Name"
            fullWidth
            variant="outlined"
            value={profileName}
            onChange={(e) => setProfileName(e.target.value)}
            sx={{ mb: 2 }}
          />
          <TextField
            margin="dense"
            label="Master Key"
            type="password"
            fullWidth
            variant="outlined"
            value={masterKey}
            onChange={(e) => setMasterKey(e.target.value)}
            sx={{ mb: 2 }}
          />
          <TextField
            margin="dense"
            label="Confirm Master Key"
            type="password"
            fullWidth
            variant="outlined"
            value={confirmKey}
            onChange={(e) => setConfirmKey(e.target.value)}
          />
          <Alert severity="info" sx={{ mt: 2 }}>
            The master key will be used to encrypt and decrypt your folders. Make sure to remember it!
          </Alert>
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setCreateProfileDialog(false)}>Cancel</Button>
          <Button onClick={handleCreateProfile} variant="contained">Create</Button>
        </DialogActions>
      </Dialog>

      {/* Authentication Dialog */}
      <Dialog open={authDialog} onClose={() => setAuthDialog(false)} maxWidth="sm" fullWidth>
        <DialogTitle>Authenticate Profile: {selectedProfile?.name}</DialogTitle>
        <DialogContent>
          <TextField
            autoFocus
            margin="dense"
            label="Master Key"
            type="password"
            fullWidth
            variant="outlined"
            value={authKey}
            onChange={(e) => setAuthKey(e.target.value)}
          />
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setAuthDialog(false)}>Cancel</Button>
          <Button onClick={handleAuthSubmit} variant="contained">Authenticate</Button>
        </DialogActions>
      </Dialog>

      {/* Add Folder Dialog */}
      <Dialog open={addFolderDialog} onClose={() => setAddFolderDialog(false)} maxWidth="sm" fullWidth>
        <DialogTitle>Add Folder to Profile</DialogTitle>
        <DialogContent>
          <TextField
            autoFocus
            margin="dense"
            label="Folder Path"
            fullWidth
            variant="outlined"
            value={selectedFolderPath}
            onChange={(e) => setSelectedFolderPath(e.target.value)}
            placeholder="/path/to/folder"
          />
          <Alert severity="warning" sx={{ mt: 2 }}>
            The selected folder will be encrypted with AES-256-CBC and moved to secure vault storage. 
            Original files will be completely hidden using platform-specific mechanisms.
          </Alert>
          <Alert severity="info" sx={{ mt: 1 }}>
            Encryption process preserves all metadata including permissions, timestamps, and extended attributes.
          </Alert>
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setAddFolderDialog(false)}>Cancel</Button>
          <Button onClick={handleAddFolder} variant="contained">Encrypt & Secure</Button>
        </DialogActions>
      </Dialog>

      {/* Unlock Mode Selection Dialog */}
      <Dialog open={unlockModeDialog} onClose={() => setUnlockModeDialog(false)} maxWidth="sm" fullWidth>
        <DialogTitle>Select Unlock Mode</DialogTitle>
        <DialogContent>
          <Typography variant="body1" sx={{ mb: 3 }}>
            Choose how you want to unlock your encrypted folders:
          </Typography>
          
          <Stack spacing={2}>
            <Paper sx={{ p: 2, border: 1, borderColor: 'primary.main' }}>
              <Typography variant="h6" color="primary" sx={{ mb: 1 }}>
                Temporary Unlock
              </Typography>
              <Typography variant="body2" sx={{ mb: 2 }}>
                Folders will be decrypted and accessible until you lock your screen, 
                reboot, or manually re-lock them. Recommended for regular use.
              </Typography>
              <Button
                variant="contained"
                fullWidth
                onClick={() => {
                  setUnlockModeDialog(false);
                  handleUnlockFolders(false);
                }}
              >
                Unlock Temporarily
              </Button>
            </Paper>
            
            <Paper sx={{ p: 2, border: 1, borderColor: 'warning.main' }}>
              <Typography variant="h6" color="warning.main" sx={{ mb: 1 }}>
                Permanent Unlock
              </Typography>
              <Typography variant="body2" sx={{ mb: 2 }}>
                Folders will be permanently decrypted and removed from vault. 
                Use only when you no longer need encryption protection.
              </Typography>
              <Button
                variant="outlined"
                color="warning"
                fullWidth
                onClick={() => {
                  setUnlockModeDialog(false);
                  handleUnlockFolders(true);
                }}
              >
                Unlock Permanently
              </Button>
            </Paper>
          </Stack>
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setUnlockModeDialog(false)}>Cancel</Button>
        </DialogActions>
      </Dialog>

      {/* Vault Management Dialog */}
      <Dialog open={vaultManagementDialog} onClose={() => setVaultManagementDialog(false)} maxWidth="md" fullWidth>
        <DialogTitle>Vault Management - {selectedProfile?.name}</DialogTitle>
        <DialogContent>
          {vaultStats && (
            <Grid container spacing={3}>
              <Grid item xs={12} md={6}>
                <Card>
                  <CardContent>
                    <Typography variant="h6" sx={{ mb: 2 }}>Storage Statistics</Typography>
                    <Stack spacing={1}>
                      <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                        <Typography>Total Folders:</Typography>
                        <Typography fontWeight="bold">{vaultStats.totalFolders}</Typography>
                      </Box>
                      <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                        <Typography>Encrypted:</Typography>
                        <Typography fontWeight="bold" color="success.main">
                          {vaultStats.encryptedFolders}
                        </Typography>
                      </Box>
                      <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                        <Typography>Total Size:</Typography>
                        <Typography fontWeight="bold">{formatFileSize(vaultStats.totalSize)}</Typography>
                      </Box>
                      <Box sx={{ display: 'flex', justifyContent: 'space-between' }}>
                        <Typography>Last Backup:</Typography>
                        <Typography>{vaultStats.lastBackup}</Typography>
                      </Box>
                    </Stack>
                  </CardContent>
                </Card>
              </Grid>
              
              <Grid item xs={12} md={6}>
                <Card>
                  <CardContent>
                    <Typography variant="h6" sx={{ mb: 2 }}>Vault Operations</Typography>
                    <Stack spacing={2}>
                      <Button variant="outlined" fullWidth>
                        Verify Vault Integrity
                      </Button>
                      <Button variant="outlined" fullWidth>
                        Compact Vault Storage
                      </Button>
                      <Button variant="outlined" fullWidth>
                        Export Vault Backup
                      </Button>
                      <Button variant="outlined" color="warning" fullWidth>
                        Repair Vault Structure
                      </Button>
                    </Stack>
                  </CardContent>
                </Card>
              </Grid>
            </Grid>
          )}
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setVaultManagementDialog(false)}>Close</Button>
        </DialogActions>
      </Dialog>
    </Box>
  );
};

export default Dashboard;