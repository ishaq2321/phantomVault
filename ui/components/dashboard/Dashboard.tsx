import React, { useState } from 'react';
import { StatusOverview } from './StatusOverview';
import { Settings } from '../settings/Settings';
import { InputDialog } from '../common/InputDialog';

interface Vault {
  id: string;
  name: string;
  path: string;
  isSecure: boolean;
  isLocked: boolean;
  lastModified: Date;
  size: number;
}

interface DashboardProps {
  folders: Vault[];
  onLock: (vaultId: string) => void;
  onUnlock: (vaultId: string) => void;
  onDelete: (vaultId: string) => void;
  onRefresh?: () => void;
}

type DialogStep = 'name' | 'password' | null;

export const Dashboard: React.FC<DashboardProps> = ({ folders, onLock, onUnlock, onDelete, onRefresh }) => {
  const [showSettings, setShowSettings] = useState(false);
  const [masterKeyDialog, setMasterKeyDialog] = useState<{
    show: boolean;
    folderPath: string;
    folderName: string;
  }>({ show: false, folderPath: '', folderName: '' });

  const handleAddFolder = async () => {
    try {
      // Get active profile first
      const profileResponse = await window.phantomVault.profile.getActive();
      if (!profileResponse.success || !profileResponse.profile) {
        throw new Error('No active profile found');
      }

      // Open folder selection dialog
      const folderPath = await window.phantomVault.selectFolder();
      
      if (!folderPath) {
        console.log('No folder selected');
        return;
      }
      
      const folderName = folderPath.split('/').pop() || 'New Folder';
      
      // Show master password dialog
      setMasterKeyDialog({
        show: true,
        folderPath,
        folderName,
      });
      
    } catch (error: any) {
      console.error('Failed to select folder:', error);
      window.phantomVault.showNotification(
        'Error',
        error.message || 'Failed to select folder'
      );
    }
  };

  const handleMasterKeyConfirm = async (masterPassword: string) => {
    const { folderPath, folderName } = masterKeyDialog;
    
    try {
      // Get active profile
      const profileResponse = await window.phantomVault.profile.getActive();
      if (!profileResponse.success || !profileResponse.profile) {
        throw new Error('No active profile found');
      }

      // STEP 1: Verify master password
      const verifyResponse = await window.phantomVault.profile.verifyPassword(
        profileResponse.profile.id,
        masterPassword
      );
      
      if (!verifyResponse.success) {
        window.phantomVault.showNotification(
          'Authentication Failed',
          'Incorrect master password. Folder not added.'
        );
        setMasterKeyDialog({ show: false, folderPath: '', folderName: '' });
        return;
      }
      
      // STEP 2: Add folder to vault metadata
      console.log('✅ Master password verified. Adding folder to vault:', folderPath);
      const addResponse = await window.phantomVault.folder.add(
        profileResponse.profile.id,
        folderPath
      );
      
      if (!addResponse.success) {
        throw new Error(addResponse.error || 'Failed to add folder to vault');
      }
      
      const folderId = addResponse.folderId;
      console.log('✅ Folder added to vault with ID:', folderId);
      
      // STEP 3: Immediately lock (encrypt + hide) the folder
      console.log('🔒 Auto-locking folder with master password...');
      const lockResponse = await window.phantomVault.folder.lockWithPassword(
        profileResponse.profile.id,
        folderId,
        masterPassword
      );
      
      if (lockResponse.success) {
        window.phantomVault.showNotification(
          'Folder Secured',
          `${folderName} has been encrypted and hidden in your vault.`
        );
      } else {
        // Lock failed - remove from vault to maintain consistency
        console.error('❌ Failed to lock folder, removing from vault...');
        await window.phantomVault.folder.remove(
          profileResponse.profile.id,
          folderId
        );
        throw new Error(lockResponse.error || 'Failed to encrypt folder');
      }
      
      // Close dialog and refresh
      setMasterKeyDialog({ show: false, folderPath: '', folderName: '' });
      
      if (onRefresh) {
        await onRefresh();
      }
      
    } catch (error: any) {
      console.error('Failed to add and lock folder:', error);
      window.phantomVault.showNotification(
        'Error',
        error.message || 'Failed to protect folder'
      );
      setMasterKeyDialog({ show: false, folderPath: '', folderName: '' });
    }
  };

  return (
    <>
      {showSettings && <Settings onClose={() => setShowSettings(false)} />}
      
      {/* Master Password Dialog for Adding Folders */}
      {masterKeyDialog.show && (
        <InputDialog
          title="Verify Master Password"
          message={`Enter your master password to encrypt and lock:\n${masterKeyDialog.folderName}`}
          placeholder="Enter master password"
          type="password"
          onConfirm={handleMasterKeyConfirm}
          onCancel={() => setMasterKeyDialog({ show: false, folderPath: '', folderName: '' })}
        />
      )}
      
      <div style={{
        display: 'grid',
        gridTemplateColumns: '1fr 350px',
        gridTemplateRows: 'auto 1fr',
        height: '100vh',
        backgroundColor: '#1B1F3B',
        color: '#F6F6F6',
        fontFamily: 'Inter, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif',
      }}>
        <header style={{
          gridColumn: '1 / -1',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'space-between',
          padding: '1.5rem 2rem',
          backgroundColor: '#2D3250',
          borderBottom: '1px solid #424769',
        }}>
          <div style={{
            display: 'flex',
            alignItems: 'center',
            gap: '1rem',
          }}>
            <div style={{ fontSize: '1.5rem' }}>🔐</div>
            <h1 style={{ margin: 0, fontSize: '1.5rem', fontWeight: '700' }}>Phantom Vault</h1>
          </div>
          <div style={{
            display: 'flex',
            gap: '1rem',
          }}>
            <button onClick={() => setShowSettings(true)} style={{
              padding: '0.5rem 1rem',
              backgroundColor: 'transparent',
              color: '#F6F6F6',
              border: '1px solid #424769',
              borderRadius: '8px',
              fontSize: '0.875rem',
              fontWeight: '500',
              cursor: 'pointer',
              display: 'flex',
              alignItems: 'center',
              gap: '0.5rem',
            }}>
              <span>⚙️</span>
              Settings
            </button>
            <button style={{
              padding: '0.5rem 1.5rem',
              backgroundColor: '#7077A1',
              color: '#F6F6F6',
              border: 'none',
              borderRadius: '8px',
              fontSize: '0.875rem',
              fontWeight: '500',
              cursor: 'pointer',
              display: 'flex',
              alignItems: 'center',
              gap: '0.5rem',
            }} onClick={handleAddFolder}>
              <span>➕</span>
              Add Folder
            </button>
          </div>
        </header>

        <main style={{
          padding: '2rem',
          overflowY: 'auto',
        }}>
          {folders.length === 0 ? (
            <div style={{
              display: 'flex',
              flexDirection: 'column',
              alignItems: 'center',
              justifyContent: 'center',
              padding: '4rem 2rem',
              textAlign: 'center',
            }}>
              <div style={{ fontSize: '4rem', marginBottom: '1.5rem' }}>📁</div>
              <h2 style={{ fontSize: '1.5rem', margin: '0 0 0.5rem', fontWeight: '600' }}>
                No folders found
              </h2>
              <p style={{ fontSize: '1rem', color: '#B4B4B4', marginBottom: '2rem' }}>
                Start by adding a new folder to secure your files.
              </p>
              <button style={{
                padding: '0.75rem 2rem',
                backgroundColor: '#7077A1',
                color: '#F6F6F6',
                border: 'none',
                borderRadius: '8px',
                fontSize: '1rem',
                fontWeight: '500',
                cursor: 'pointer',
                display: 'flex',
                alignItems: 'center',
                gap: '0.5rem',
              }} onClick={handleAddFolder}>
                <span>➕</span>
                Add Folder
              </button>
            </div>
          ) : (
            <div style={{
              display: 'grid',
              gridTemplateColumns: 'repeat(auto-fill, minmax(280px, 1fr))',
              gap: '1rem',
            }}>
              {folders.map((folder) => (
                <div
                  key={folder.id}
                  style={{
                    padding: '1.5rem',
                    backgroundColor: '#2D3250',
                    borderRadius: '12px',
                    border: folder.isLocked ? '2px solid #FF9800' : '2px solid #4CAF50',
                    transition: 'all 0.2s',
                  }}
                >
                  <div style={{ display: 'flex', alignItems: 'flex-start', justifyContent: 'space-between', marginBottom: '1rem' }}>
                    <div style={{ display: 'flex', alignItems: 'center', gap: '0.75rem' }}>
                      <div style={{ fontSize: '2rem' }}>
                        {folder.isLocked ? '🔒' : '🔓'}
                      </div>
                      <div>
                        <h3 style={{ margin: 0, fontSize: '1.125rem', fontWeight: '600' }}>
                          {folder.name}
                        </h3>
                        <p style={{ margin: '0.25rem 0 0', fontSize: '0.75rem', color: '#B4B4B4' }}>
                          {folder.isLocked ? 'Locked' : 'Unlocked'}
                        </p>
                      </div>
                    </div>
                  </div>
                  
                  <div style={{ fontSize: '0.875rem', color: '#B4B4B4', marginBottom: '1rem', wordBreak: 'break-all' }}>
                    📂 {folder.path}
                  </div>
                  
                  <div style={{ display: 'flex', gap: '0.5rem' }}>
                    {/* Only show Unlock button for locked folders */}
                    {folder.isLocked && (
                      <button
                        onClick={(e) => {
                          e.stopPropagation();
                          onUnlock(folder.id);
                        }}
                        style={{
                          flex: 1,
                          padding: '0.5rem',
                          backgroundColor: '#4CAF50',
                          color: '#FFF',
                          border: 'none',
                          borderRadius: '6px',
                          fontSize: '0.875rem',
                          fontWeight: '500',
                          cursor: 'pointer',
                        }}
                      >
                        🔓 Unlock
                      </button>
                    )}
                    
                    {/* Show message for unlocked folders */}
                    {!folder.isLocked && (
                      <div style={{
                        flex: 1,
                        padding: '0.5rem',
                        backgroundColor: '#4CAF50',
                        color: '#FFF',
                        borderRadius: '6px',
                        fontSize: '0.875rem',
                        fontWeight: '500',
                        textAlign: 'center',
                      }}>
                        ✅ Unlocked
                      </div>
                    )}
                    
                    <button
                      onClick={(e) => {
                        e.stopPropagation();
                        onDelete(folder.id);
                      }}
                      style={{
                        padding: '0.5rem 1rem',
                        backgroundColor: '#F44336',
                        color: '#FFF',
                        border: 'none',
                        borderRadius: '6px',
                        fontSize: '0.875rem',
                        cursor: 'pointer',
                      }}
                    >
                      🗑️
                    </button>
                  </div>
                </div>
              ))}
            </div>
          )}
        </main>

        <aside style={{
          padding: '2rem',
          backgroundColor: '#2D3250',
          borderLeft: '1px solid #424769',
          overflowY: 'auto',
        }}>
          <StatusOverview folders={folders} />
        </aside>
      </div>
    </>
  );
}; 
