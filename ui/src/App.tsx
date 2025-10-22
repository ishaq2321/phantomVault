import React, { useState, useEffect } from 'react';
import { Dashboard } from '../components/dashboard/Dashboard';
import { SetupWizard } from '../components/setup-wizard/SetupWizard';
import { PasswordRecovery } from '../components/recovery/PasswordRecovery';
import { InputDialog } from '../components/common/InputDialog';
import { InvisibleOverlay, PasswordInput } from '../components/unlock-overlay/InvisibleOverlay';

type AppView = 'setup' | 'dashboard' | 'recovery';

interface AppState {
  isInitialized: boolean;
  currentView: AppView;
  vaults: any[];
  isLoading: boolean;
}

interface UnlockState {
  vaultId: string | null;
  showDialog: boolean;
}

interface DeleteState {
  vaultId: string | null;
  showConfirm: boolean;
}

export const App: React.FC = () => {
  const [state, setState] = useState<AppState>({
    isInitialized: false,
    currentView: 'setup',
    vaults: [],
    isLoading: true,
  });
  
  const [unlockState, setUnlockState] = useState<UnlockState>({
    vaultId: null,
    showDialog: false,
  });
  
  const [deleteState, setDeleteState] = useState<DeleteState>({
    vaultId: null,
    showConfirm: false,
  });

  // PhantomVault 2.0 - Invisible Overlay State
  const [overlayState, setOverlayState] = useState({
    isVisible: false,
    isRecoveryMode: false,
    isRelockMode: false, // NEW: Flag for re-lock mode
    temporaryCount: 0, // NEW: Number of temporary folders
  });

  // Debug: Log when overlay state changes
  useEffect(() => {
    console.log('📊 Overlay state changed:', overlayState);
  }, [overlayState]);

  useEffect(() => {
    console.log('🎬 App.tsx: Setting up event listeners...');
    initializeApp();
    
    // Listen for hotkey events (Phase 2)
    console.log('🔊 App.tsx: Registering onShowUnlockOverlay listener...');
    const cleanupOverlay = window.phantomVault.onShowUnlockOverlay((data) => {
      console.log('🔓 Show unlock overlay EVENT RECEIVED:', data);
      
      // Prevent re-opening if already visible
      setOverlayState((prev) => {
        if (prev.isVisible) {
          console.log('⚠️ Overlay already visible, ignoring duplicate event');
          return prev;
        }
        console.log('🔓 Setting overlay state to VISIBLE');
        return {
          isVisible: true,
          isRecoveryMode: data.isRecoveryMode || false,
          isRelockMode: (data as any).isRelockMode || false, // NEW
          temporaryCount: (data as any).temporaryCount || 0, // NEW
        };
      });
      console.log('🔓 Overlay state updated');
    });
    console.log('✅ App.tsx: onShowUnlockOverlay listener registered');
    
    // Listen for auto-lock events (Phase 3)
    const cleanupAutoLock = window.phantomVault.onAutoLockFolder(async (data) => {
      console.log('🔒 Auto-lock folder:', data.folderId);
      
      // Lock the folder using IPC
      try {
        const profileResponse = await window.phantomVault.profile.getActive();
        if (profileResponse.success && profileResponse.profile) {
          const lockResponse = await window.phantomVault.folder.lock(
            profileResponse.profile.id,
            data.folderId
          );
          if (lockResponse.success) {
            console.log(`✅ Folder ${data.folderId} auto-locked`);
            // Refresh vaults to show updated lock status
            await refreshVaults();
          }
        }
      } catch (error) {
        console.error('Failed to auto-lock folder:', error);
      }
    });
    
    // Listen for metadata changes (Phase 3)
    const cleanupMetadata = window.phantomVault.onMetadataChanged((data) => {
      console.log('📝 Metadata changed:', data.filePath);
      // Refresh vaults to reflect changes
      refreshVaults();
    });
    
    // Listen for suspicious activity (Phase 3)
    const cleanupSuspicious = window.phantomVault.onSuspiciousActivity((data) => {
      console.warn('⚠️ Suspicious activity:', data.activity);
      window.phantomVault.showNotification(
        'Security Alert',
        data.activity
      );
    });
    
    return () => {
      cleanupOverlay();
      cleanupAutoLock();
      cleanupMetadata();
      cleanupSuspicious();
    };
  }, []);

  const initializeApp = async () => {
    try {
      setState(prev => ({ ...prev, isLoading: true }));

      const version = await window.phantomVault.getVersion();
      console.log('PhantomVault Core version:', version);

      // NEW SYSTEM: Load from VaultProfileManager
      const profileResponse = await window.phantomVault.profile.getActive();
      
      if (!profileResponse.success || !profileResponse.profile) {
        // No profile exists - show setup wizard
        console.log('No active profile found - showing setup wizard');
        setState({
          isInitialized: true,
          currentView: 'setup',
          vaults: [],
          isLoading: false,
        });
        return;
      }

      const profile = profileResponse.profile;
      console.log('Loaded active profile:', profile.name);

      // Load folders for this profile
      const foldersResponse = await window.phantomVault.folder.getAll(profile.id);
      
      const vaultData = [];
      if (foldersResponse.success && foldersResponse.folders) {
        for (const folder of foldersResponse.folders) {
          vaultData.push({
            id: folder.id,
            name: folder.name || 'Unnamed Folder',
            path: folder.path || '',
            isSecure: true,
            isLocked: folder.isLocked !== false, // Default to locked
            lastModified: folder.lastModified ? new Date(folder.lastModified) : new Date(),
            size: folder.size || 0,
          });
        }
      }
      
      console.log('Loaded folders:', vaultData.length);

      setState({
        isInitialized: true,
        currentView: 'dashboard',
        vaults: vaultData,
        isLoading: false,
      });
    } catch (error) {
      console.error('Failed to initialize app:', error);
      setState(prev => ({ ...prev, isLoading: false }));
      
      window.phantomVault.showNotification(
        'Initialization Error',
        'Failed to initialize PhantomVault.'
      );
    }
  };
  
  const refreshVaults = async () => {
    await initializeApp();
  };

  const handleSetupComplete = () => {
    setState(prev => ({ ...prev, currentView: 'dashboard' }));
    window.phantomVault.showNotification(
      'Setup Complete',
      'PhantomVault is now ready to secure your folders!'
    );
  };

  const handleFolderLock = async (folderId: string) => {
    try {
      console.log('🔒 Locking folder:', folderId);
      
      // Get active profile
      const profileResponse = await window.phantomVault.profile.getActive();
      if (!profileResponse.success || !profileResponse.profile) {
        throw new Error('No active profile found');
      }

      const profile = profileResponse.profile;
      
      // Get folder details
      const folder = state.vaults.find(v => v.id === folderId);
      if (!folder) {
        throw new Error('Folder not found');
      }

      console.log('🔐 Locking folder with master password:', folder.path);
      
      // Lock the folder using the profile's master password
      const lockResult = await window.phantomVault.folder.lock(
        profile.username,
        folderId
      );

      if (lockResult.success) {
        console.log('✅ Folder locked successfully');
        window.phantomVault.showNotification(
          'Folder Locked',
          `${folder.name} has been encrypted and hidden`
        );
        
        // Refresh vault list
        await refreshVaults();
      } else {
        throw new Error(lockResult.error || 'Failed to lock folder');
      }
      
    } catch (error: any) {
      console.error('❌ Failed to lock folder:', error);
      window.phantomVault.showNotification(
        'Error',
        error.message || 'Failed to lock folder'
      );
    }
  };

  const handleFolderUnlock = async (folderId: string) => {
    // Show password dialog
    setUnlockState({
      vaultId: folderId,
      showDialog: true,
    });
  };
  
  const handleUnlockConfirm = async (password: string) => {
    const { vaultId } = unlockState;
    if (!vaultId) return;
    
    try {
      console.log('Unlocking folder:', vaultId);
      
      // Get active profile
      const profileResponse = await window.phantomVault.profile.getActive();
      if (!profileResponse.success || !profileResponse.profile) {
        throw new Error('No active profile found');
      }

      // Parse password and mode (e.g., "mypassT" or "mypassP")
      let actualPassword = password;
      let mode: 'temporary' | 'permanent' = 'temporary';
      
      if (password.endsWith('T') || password.endsWith('t')) {
        actualPassword = password.slice(0, -1);
        mode = 'temporary';
      } else if (password.endsWith('P') || password.endsWith('p')) {
        actualPassword = password.slice(0, -1);
        mode = 'permanent';
      }
      
      // Unlock the folder
      const result = await window.phantomVault.folder.unlock(
        profileResponse.profile.id,
        vaultId,
        actualPassword,
        mode
      );
      
      if (result.success) {
        window.phantomVault.showNotification(
          'Folder Unlocked',
          mode === 'temporary' 
            ? 'Folder unlocked temporarily (will lock when PC locks)'
            : 'Folder unlocked permanently (removed from vault)'
        );
        await refreshVaults();
      } else {
        throw new Error(result.error || 'Failed to unlock folder');
      }
      
      // Close dialog
      setUnlockState({ vaultId: null, showDialog: false });
    } catch (error: any) {
      console.error('Failed to unlock folder:', error);
      window.phantomVault.showNotification(
        'Error',
        error.message || 'Failed to unlock folder. Check your password.'
      );
      // Close dialog on error too
      setUnlockState({ vaultId: null, showDialog: false });
    }
  };

  const handleFolderDelete = async (folderId: string) => {
    // Show confirmation dialog
    setDeleteState({
      vaultId: folderId,
      showConfirm: true,
    });
  };
  
  const handleDeleteConfirm = async () => {
    const { vaultId } = deleteState;
    if (!vaultId) return;
    
    try {
      console.log('Removing folder:', vaultId);
      
      // Get active profile
      const profileResponse = await window.phantomVault.profile.getActive();
      if (!profileResponse.success || !profileResponse.profile) {
        throw new Error('No active profile found');
      }
      
      // Remove folder from profile
      const result = await window.phantomVault.folder.remove(
        profileResponse.profile.id,
        vaultId
      );
      
      if (result.success) {
        window.phantomVault.showNotification(
          'Folder Removed',
          'The folder has been removed from your vault'
        );
        await refreshVaults();
      } else {
        throw new Error(result.error || 'Failed to remove folder');
      }
      
      // Close confirmation
      setDeleteState({ vaultId: null, showConfirm: false });
    } catch (error: any) {
      console.error('Failed to remove folder:', error);
      window.phantomVault.showNotification(
        'Error',
        error.message || 'Failed to remove folder'
      );
      // Close confirmation on error
      setDeleteState({ vaultId: null, showConfirm: false });
    }
  };

  // const handleSearch = (query: string) => {
  //   console.log('Searching:', query);
  // };

  // PhantomVault 2.0 - Handle invisible overlay submission
  const handleOverlaySubmit = async (input: PasswordInput) => {
    console.log('🔐 Overlay submitted:', { 
      mode: input.mode, 
      isRecovery: input.isRecoveryKey, 
      isRelock: input.isRelock 
    });
    
    try {
      const profileResponse = await window.phantomVault.profile.getActive();
      
      if (!profileResponse.success || !profileResponse.profile) {
        console.error('No active profile');
        setOverlayState({ 
          isVisible: false, 
          isRecoveryMode: false, 
          isRelockMode: false, 
          temporaryCount: 0 
        });
        window.phantomVault.showNotification(
          'Error',
          'No active profile found. Please run setup first.'
        );
        return;
      }
      
      const activeProfile = profileResponse.profile;
      
      if (input.isRelock) {
        // Handle re-lock of temporarily unlocked folders
        console.log('🔒 Re-lock mode: Locking all temporary folders...');
        // TODO: Implement lockAllTemporary in the API
        const relockResponse = { 
          success: true, 
          message: 'Relock not implemented yet',
          locked: 0,
          failed: 0,
          errors: [] as string[]
        };
        // const relockResponse = await window.phantomVault.folder.lockAllTemporary(
        //   activeProfile.id,
        //   input.password
        // );
        
        if (relockResponse.success) {
          const { locked, failed, errors } = relockResponse;
          
          if (locked > 0) {
            console.log(`✅ Re-locked ${locked} folder(s)`);
            window.phantomVault.showNotification(
              'Re-Lock Successful',
              `${locked} temporary folder${locked !== 1 ? 's' : ''} locked successfully`
            );
            await refreshVaults();
          }
          
          if (failed > 0) {
            console.error(`⚠️ Failed to lock ${failed} folder(s):`, errors);
            window.phantomVault.showNotification(
              'Re-Lock Partial',
              `${locked} locked, ${failed} failed. Check console for details.`
            );
          }
          
          if (locked === 0 && failed === 0) {
            console.log('⚠️ No temporary folders to lock');
            // Silent - no notification needed
          }
        } else {
          console.error('Re-lock failed:', (relockResponse as any).error);
          window.phantomVault.showNotification(
            'Re-Lock Failed',
            (relockResponse as any).error || 'Failed to lock temporary folders'
          );
        }
      } else if (input.isRecoveryKey) {
        // Handle recovery key - for now just show message
        // TODO: Implement recovery key decryption via IPC
        console.log('✅ Recovery key entered');
        window.phantomVault.showNotification(
          'Recovery',
          'Recovery key functionality coming soon'
        );
      } else {
        // Handle unlock all folders
        const unlockResponse = await window.phantomVault.folder.unlockAll(
          activeProfile.id,
          input.password,
          input.mode
        );
        
        if (unlockResponse.success && unlockResponse.result) {
          const result = unlockResponse.result;
          
          if (result.success > 0) {
            console.log(`✅ Unlocked ${result.success} folder(s)`);
            
            // Register temporary unlocks with AutoLockManager
            if (input.mode === 'temporary') {
              const foldersResponse = await window.phantomVault.folder.getAll(activeProfile.id);
              if (foldersResponse.success && foldersResponse.folders) {
                const tempFolders = foldersResponse.folders.filter(
                  (f: any) => !f.isLocked && f.unlockMode === 'temporary'
                );
                for (const folder of tempFolders) {
                  await window.phantomVault.autoLock.registerUnlock(
                    folder.id,
                    activeProfile.id,
                    'temporary',
                    folder.folderPath
                  );
                  console.log(`📝 Registered temporary unlock: ${folder.folderName}`);
                }
              }
            }
            
            window.phantomVault.showNotification(
              'Unlock Successful',
              `${result.success} folder(s) unlocked in ${input.mode} mode`
            );
            await refreshVaults();
          } else {
            console.log('❌ No folders unlocked (wrong password or no folders)');
            // Silent fail - no error shown
          }
        }
      }
    } catch (error) {
      console.error('Overlay submission error:', error);
      // Silent fail
    } finally {
      setOverlayState({ 
        isVisible: false, 
        isRecoveryMode: false, 
        isRelockMode: false, 
        temporaryCount: 0 
      });
      // Close overlay window if it exists
      // TODO: Implement closeOverlayWindow in the API
      // await window.phantomVault.closeOverlayWindow().catch(() => {});
    }
  };

  const handleOverlayCancel = async () => {
    console.log('🚫 Overlay cancelled');
    setOverlayState({ 
      isVisible: false, 
      isRecoveryMode: false, 
      isRelockMode: false, 
      temporaryCount: 0 
    });
    // Close overlay window if it exists
    // TODO: Implement closeOverlayWindow in the API
    // await window.phantomVault.closeOverlayWindow().catch(() => {});
  };

  if (state.isLoading) {
    return (
      <div style={{
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
        height: '100vh',
        backgroundColor: '#1B1F3B',
        color: '#F6F6F6',
        flexDirection: 'column',
        gap: '1rem',
      }}>
        <div style={{
          width: '50px',
          height: '50px',
          border: '3px solid #424769',
          borderTop: '3px solid #7077A1',
          borderRadius: '50%',
          animation: 'spin 1s linear infinite',
        }} />
        <h2>Loading PhantomVault...</h2>
        <style>{`
          @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
          }
        `}</style>
      </div>
    );
  }

  return (
    <div className="app">
      {/* Unlock Password Dialog */}
      {unlockState.showDialog && (
        <InputDialog
          title="Unlock Folder"
          message="Enter mode + your master password:\n• T for Temporary (locks when PC locks)\n• P for Permanent (removes from vault)\n\nExample: T1234 or P1234"
          placeholder="Enter password + T or P"
          type="password"
          onConfirm={handleUnlockConfirm}
          onCancel={() => setUnlockState({ vaultId: null, showDialog: false })}
        />
      )}
      
      {/* Delete Confirmation Dialog */}
      {deleteState.showConfirm && (
        <div
          style={{
            position: 'fixed',
            top: 0,
            left: 0,
            right: 0,
            bottom: 0,
            backgroundColor: 'rgba(0, 0, 0, 0.8)',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            zIndex: 2000,
          }}
          onClick={() => setDeleteState({ vaultId: null, showConfirm: false })}
        >
          <div
            style={{
              backgroundColor: '#2D3250',
              borderRadius: '12px',
              padding: '2rem',
              maxWidth: '450px',
              width: '90%',
              boxShadow: '0 20px 60px rgba(0, 0, 0, 0.5)',
            }}
            onClick={(e) => e.stopPropagation()}
          >
            <h2 style={{ margin: '0 0 0.5rem', fontSize: '1.25rem', fontWeight: '600', color: '#F6F6F6' }}>
              Delete Vault?
            </h2>
            <p style={{ margin: '0 0 1.5rem', fontSize: '0.875rem', color: '#B4B4B4' }}>
              Are you sure you want to delete this vault? This action cannot be undone.
            </p>
            <div style={{ display: 'flex', gap: '0.75rem', justifyContent: 'flex-end' }}>
              <button
                onClick={() => setDeleteState({ vaultId: null, showConfirm: false })}
                style={{
                  padding: '0.75rem 1.5rem',
                  backgroundColor: 'transparent',
                  color: '#F6F6F6',
                  border: '1px solid #424769',
                  borderRadius: '8px',
                  fontSize: '0.875rem',
                  fontWeight: '500',
                  cursor: 'pointer',
                }}
              >
                Cancel
              </button>
              <button
                onClick={handleDeleteConfirm}
                style={{
                  padding: '0.75rem 1.5rem',
                  backgroundColor: '#F44336',
                  color: '#F6F6F6',
                  border: 'none',
                  borderRadius: '8px',
                  fontSize: '0.875rem',
                  fontWeight: '500',
                  cursor: 'pointer',
                }}
              >
                Delete
              </button>
            </div>
          </div>
        </div>
      )}
      
      {state.currentView === 'setup' && (
        <SetupWizard onComplete={handleSetupComplete} />
      )}
      
      {state.currentView === 'dashboard' && (
        <Dashboard
          folders={state.vaults}
          onLock={handleFolderLock}
          onUnlock={handleFolderUnlock}
          onDelete={handleFolderDelete}
          onRefresh={refreshVaults}
        />
      )}
      
      {state.currentView === 'recovery' && (
        <PasswordRecovery 
          vaultId="default"
          questions={[]}
          onRecoverySuccess={() => {}}
          onRecoveryCancel={() => setState(prev => ({ ...prev, currentView: 'dashboard' }))}
        />
      )}

      {/* Invisible Unlock Overlay - Triggered by global hotkey */}
      <InvisibleOverlay
        isVisible={overlayState.isVisible}
        isRecoveryMode={overlayState.isRecoveryMode}
        isRelockMode={overlayState.isRelockMode}
        temporaryCount={overlayState.temporaryCount}
        onSubmit={handleOverlaySubmit}
        onCancel={handleOverlayCancel}
      />
    </div>
  );
};
