/**
 * Vaults View Component
 */

import React from 'react';
import { useVault } from '../../contexts';
import './Vaults.css';

export const Vaults: React.FC = () => {
  const { state: vaultState, actions: vaultActions } = useVault();

  // Handle creating a new vault
  const handleCreateVault = async () => {
    const vaultName = prompt('Enter vault name:');
    if (!vaultName || !vaultName.trim()) {
      return;
    }

    const vaultPath = prompt('Enter folder path to encrypt:');
    if (!vaultPath || !vaultPath.trim()) {
      return;
    }

    try {
      const result = await vaultActions.createVault({
        name: vaultName.trim(),
        path: vaultPath.trim(),
        password: '', // Will be handled by the system
      });

      if (result.success) {
        alert(`✅ Vault "${vaultName}" created successfully!`);
      } else {
        alert(`❌ Failed to create vault: ${result.error || result.message}`);
      }
    } catch (error) {
      alert(`❌ Error creating vault: ${error}`);
    }
  };

  // Handle locking/unlocking a vault
  const handleToggleVault = async (vaultId: string, currentStatus: string) => {
    const vault = vaultState.vaults.find(v => v.id === vaultId);
    if (!vault) return;

    const isLocked = currentStatus === 'unmounted';
    
    try {
      if (isLocked) {
        // Unlock vault - prompt for password
        const password = prompt(`🔓 Enter password to unlock vault "${vault.name}":`);
        if (!password) return;

        const result = await window.phantomVault.folder.unlock(
          vault.profile.id,
          vaultId,
          password,
          'temporary' // Default to temporary unlock
        );

        if (result.success) {
          alert(`✅ Vault "${vault.name}" unlocked successfully!`);
        } else {
          alert(`❌ Failed to unlock vault: ${result.error || 'Invalid password'}`);
        }
      } else {
        // Lock vault - confirm action
        const confirmed = confirm(`🔒 Lock vault "${vault.name}"?\n\nThis will encrypt and hide the folder.`);
        if (!confirmed) return;

        const result = await window.phantomVault.folder.lock(
          vault.profile.id,
          vaultId
        );

        if (result.success) {
          alert(`✅ Vault "${vault.name}" locked successfully!`);
        } else {
          alert(`❌ Failed to lock vault: ${result.error || 'Unknown error'}`);
        }
      }
      
      // Reload vaults to get updated status
      await vaultActions.loadVaults();
    } catch (error) {
      alert(`❌ Error ${isLocked ? 'unlock' : 'lock'}ing vault: ${error}`);
    }
  };

  // Handle deleting a vault
  const handleDeleteVault = async (vaultId: string) => {
    const vault = vaultState.vaults.find(v => v.id === vaultId);
    if (!vault) return;

    const confirmed = confirm(`⚠️ Delete vault "${vault.name}"?\n\nThis will permanently remove the vault configuration.\nThe original folder will remain but will no longer be encrypted.\n\nThis action cannot be undone.`);
    if (!confirmed) return;

    try {
      const result = await vaultActions.deleteVault(vaultId);
      if (result.success) {
        alert(`✅ Vault "${vault.name}" deleted successfully!`);
      } else {
        alert(`❌ Failed to delete vault: ${result.error || result.message}`);
      }
    } catch (error) {
      alert(`❌ Error deleting vault: ${error}`);
    }
  };

  // Format file size
  const formatSize = (bytes: number) => {
    if (bytes === 0) return 'Unknown';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
  };

  // Format date
  const formatDate = (date: Date) => {
    return date.toLocaleDateString() + ' ' + date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
  };

  if (vaultState.loading) {
    return (
      <div className="vaults-container">
        <h1 className="vaults-title">🗂️ Vault Management</h1>
        <p className="loading-text">Loading vaults...</p>
      </div>
    );
  }

  return (
    <div className="vaults-container">
      <div className="vaults-header">
        <div className="header-info">
          <h1 className="vaults-title">🗂️ Vault Management</h1>
          <p className="vaults-subtitle">Manage your encrypted folders securely</p>
        </div>
        <button 
          className="add-vault-button"
          onClick={handleCreateVault}
          title="Create a new encrypted vault"
        >
          ➕ Add New Vault
        </button>
      </div>

      {vaultState.error && (
        <div className="error-message">
          ❌ {vaultState.error.message}
        </div>
      )}

      {vaultState.vaults.length === 0 ? (
        <div className="empty-state">
          <div className="empty-icon">🗂️</div>
          <h3>No Vaults Found</h3>
          <p>Create your first encrypted vault to get started.</p>
          <button 
            className="create-first-vault-button"
            onClick={handleCreateVault}
          >
            ➕ Create Your First Vault
          </button>
        </div>
      ) : (
        <div className="vaults-grid">
          {vaultState.vaults.map(vault => {
            const isLocked = vault.status === 'unmounted';
            return (
              <div key={vault.id} className="vault-card">
                <div className="vault-header">
                  <div className="vault-info">
                    <h3 className="vault-name">{vault.name}</h3>
                    <p className="vault-path" title={vault.path}>{vault.path}</p>
                  </div>
                  <span className={`vault-status ${isLocked ? 'locked' : 'unlocked'}`}>
                    {isLocked ? '🔒 Locked' : '🔓 Unlocked'}
                  </span>
                </div>
                
                <div className="vault-details">
                  <p className="vault-detail">
                    <span className="detail-label">Size:</span> {formatSize(vault.size)}
                  </p>
                  <p className="vault-detail">
                    <span className="detail-label">Last Access:</span> {formatDate(vault.lastAccess)}
                  </p>
                  <p className="vault-detail">
                    <span className="detail-label">Profile:</span> {vault.profile.name}
                  </p>
                </div>

                <div className="vault-actions">
                  <button 
                    className={`action-button ${isLocked ? 'unlock' : 'lock'}`}
                    onClick={() => handleToggleVault(vault.id, vault.status)}
                    title={isLocked ? 'Unlock this vault' : 'Lock this vault'}
                  >
                    {isLocked ? '🔓 Unlock' : '🔒 Lock'}
                  </button>
                  <button 
                    className="action-button settings"
                    onClick={() => alert(`⚙️ Vault Settings for "${vault.name}"\n\nPath: ${vault.path}\nStatus: ${vault.status}\nProfile: ${vault.profile.name}\n\nNote: Settings panel would open here in full implementation.`)}
                    title="Vault settings"
                  >
                    ⚙️ Settings
                  </button>
                  <button 
                    className="action-button delete"
                    onClick={() => handleDeleteVault(vault.id)}
                    title="Delete this vault"
                  >
                    🗑️ Delete
                  </button>
                </div>
              </div>
            );
          })}
        </div>
      )}
    </div>
  );
};