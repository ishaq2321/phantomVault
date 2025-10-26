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
  Divider,
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
  AccessTime as TimeIcon,
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
  isLocked: boolean;
  size: number;
  createdAt: string;
}

const Dashboard: React.FC<DashboardProps> = ({ isAdmin, serviceStatus }) => {
  const [profiles, setProfiles] = useState<Profile[]>([]);
  const [selectedProfile, setSelectedProfile] = useState<Profile | null>(null);
  const [folders, setFolders] = useState<SecuredFolder[]>([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  
  // Dialog states
  const [createProfileDialog, setCreateProfileDialog] = useState(false);
  const [authDialog, setAuthDialog] = useState(false);
  const [addFolderDialog, setAddFolderDialog] = useState(false);
  
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
      // Simulate API call - replace with actual service call
      const mockProfiles: Profile[] = [
        {
          id: 'profile1',
          name: 'Personal',
          createdAt: '2024-01-15',
          lastAccess: '2024-01-20',
          folderCount: 3,
        },
        {
          id: 'profile2',
          name: 'Work',
          createdAt: '2024-01-10',
          lastAccess: '2024-01-19',
          folderCount: 5,
        },
      ];
      setProfiles(mockProfiles);
    } catch (err) {
      setError('Failed to load profiles');
    } finally {
      setLoading(false);
    }
  };

  const loadFolders = async (profileId: string) => {
    try {
      setLoading(true);
      // Simulate API call - replace with actual service call
      const mockFolders: SecuredFolder[] = [
        {
          id: 'folder1',
          name: 'Documents',
          originalPath: '/home/user/Documents',
          isLocked: true,
          size: 1024 * 1024 * 50, // 50MB
          createdAt: '2024-01-15',
        },
        {
          id: 'folder2',
          name: 'Photos',
          originalPath: '/home/user/Photos',
          isLocked: false,
          size: 1024 * 1024 * 200, // 200MB
          createdAt: '2024-01-16',
        },
      ];
      setFolders(mockFolders);
    } catch (err) {
      setError('Failed to load folders');
    } finally {
      setLoading(false);
    }
  };

  const handleCreateProfile = async () => {
    if (!profileName || !masterKey || masterKey !== confirmKey) {
      setError('Please fill all fields and ensure passwords match');
      return;
    }

    try {
      setLoading(true);
      // TODO: Call actual service API
      console.log('Creating profile:', profileName);
      
      setCreateProfileDialog(false);
      setProfileName('');
      setMasterKey('');
      setConfirmKey('');
      loadProfiles();
    } catch (err) {
      setError('Failed to create profile');
    } finally {
      setLoading(false);
    }
  };

  const handleAuthenticateProfile = async (profile: Profile) => {
    setSelectedProfile(profile);
    setAuthDialog(true);
  };

  const handleAuthSubmit = async () => {
    if (!authKey) {
      setError('Please enter master key');
      return;
    }

    try {
      setLoading(true);
      // TODO: Call actual authentication API
      console.log('Authenticating profile:', selectedProfile?.name);
      
      setAuthDialog(false);
      setAuthKey('');
      // Profile is now authenticated and folders will load
    } catch (err) {
      setError('Authentication failed');
    } finally {
      setLoading(false);
    }
  };

  const handleAddFolder = async () => {
    if (!selectedFolderPath) {
      setError('Please select a folder');
      return;
    }

    try {
      setLoading(true);
      // TODO: Call actual service API
      console.log('Adding folder:', selectedFolderPath);
      
      setAddFolderDialog(false);
      setSelectedFolderPath('');
      if (selectedProfile) {
        loadFolders(selectedProfile.id);
      }
    } catch (err) {
      setError('Failed to add folder');
    } finally {
      setLoading(false);
    }
  };

  const handleUnlockFolders = async (permanent: boolean = false) => {
    try {
      setLoading(true);
      // TODO: Call actual service API
      console.log('Unlocking folders:', permanent ? 'permanent' : 'temporary');
      
      if (selectedProfile) {
        loadFolders(selectedProfile.id);
      }
    } catch (err) {
      setError('Failed to unlock folders');
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
                    <Stack direction="row" spacing={1}>
                      <Button
                        variant="contained"
                        startIcon={<LockOpenIcon />}
                        onClick={() => handleUnlockFolders(false)}
                        size="small"
                      >
                        Unlock Temporary
                      </Button>
                      <Button
                        variant="outlined"
                        startIcon={<LockOpenIcon />}
                        onClick={() => handleUnlockFolders(true)}
                        size="small"
                        color="warning"
                      >
                        Unlock Permanent
                      </Button>
                    </Stack>
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
            The selected folder will be encrypted and moved to secure storage. Make sure you have a backup!
          </Alert>
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setAddFolderDialog(false)}>Cancel</Button>
          <Button onClick={handleAddFolder} variant="contained">Add Folder</Button>
        </DialogActions>
      </Dialog>
    </Box>
  );
};

export default Dashboard;