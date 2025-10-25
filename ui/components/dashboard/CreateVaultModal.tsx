/**
 * Create Vault Modal Component
 * 
 * Modal for adding a new folder/vault to protect
 */

import React, { useState, useCallback } from 'react';
import { Modal } from '../common/Modal';
import { FolderSelector } from '../common/FolderSelector';
import { Input } from '../common/Input';
import { Button } from '../common/Button';
import './CreateVaultModal.css';

interface CreateVaultModalProps {
  isOpen: boolean;
  onClose: () => void;
  onVaultCreated: () => void;
  profileId: string;
}

export const CreateVaultModal: React.FC<CreateVaultModalProps> = ({
  isOpen,
  onClose,
  onVaultCreated,
  profileId,
}) => {
  const [folderPath, setFolderPath] = useState('');
  const [vaultName, setVaultName] = useState('');
  const [isCreating, setIsCreating] = useState(false);
  const [error, setError] = useState('');

  // ==================== FOLDER PATH HANDLING ====================

  const handleFolderChange = useCallback((path: string) => {
    setFolderPath(path);
    setError('');
    
    // Auto-fill vault name from folder name if not already set
    if (!vaultName && path) {
      const folderName = path.split('/').filter(Boolean).pop() || '';
      setVaultName(folderName);
    }
  }, [vaultName]);

  // ==================== VAULT CREATION ====================

  const handleCreateVault = async () => {
    try {
      // Validation
      if (!folderPath) {
        setError('Please select a folder');
        return;
      }

      if (!vaultName.trim()) {
        setError('Please enter a vault name');
        return;
      }

      setIsCreating(true);
      setError('');

      console.log('Creating vault:', { profileId, folderPath, vaultName });

      // Call the folder.add IPC handler
      const response = await window.phantomVault.folder.add(
        profileId,
        folderPath,
        vaultName.trim()
      );

      if (!response.success) {
        throw new Error(response.error || 'Failed to create vault');
      }

      console.log('✅ Vault created successfully:', response.folder);

      // Show success notification
      window.phantomVault.showNotification(
        'Vault Created',
        `"${vaultName}" has been added to your vaults!`
      );

      // Reset form
      setFolderPath('');
      setVaultName('');
      
      // Notify parent to refresh vaults list
      onVaultCreated();
      
      // Close modal
      onClose();
    } catch (err) {
      console.error('Failed to create vault:', err);
      setError(err instanceof Error ? err.message : 'Failed to create vault');
    } finally {
      setIsCreating(false);
    }
  };

  const handleCancel = () => {
    // Reset form
    setFolderPath('');
    setVaultName('');
    setError('');
    onClose();
  };

  // ==================== RENDER ====================

  return (
    <Modal
      isOpen={isOpen}
      onClose={handleCancel}
      title="Add New Vault"
      className="create-vault-modal"
    >
      <>
        <div className="modal-description">
          Select a folder on your computer to protect with PhantomVault.
          The folder will be encrypted and hidden when locked.
        </div>

        {/* Vault Name Input */}
        <div className="form-group">
          <label className="form-label">
            Vault Name
          </label>
          <Input
            type="text"
            value={vaultName}
            onChange={(value) => {
              setVaultName(value);
              setError('');
            }}
            placeholder="My Secret Files"
            error={error && !vaultName ? 'Vault name is required' : ''}
            disabled={isCreating}
          />
          <div className="form-hint">
            Give your vault a memorable name
          </div>
        </div>

        {/* Folder Selector */}
        <div className="form-group">
          <label className="form-label">
            Folder Location
          </label>
          <FolderSelector
            value={folderPath}
            onChange={handleFolderChange}
            placeholder="Choose a folder to protect"
            error={!!error && !folderPath}
            className=""
          />
        </div>

        {/* Error Message */}
        {error && (
          <div className="error-message">
            <span className="error-icon">⚠️</span>
            <span className="error-text">{error}</span>
          </div>
        )}

        {/* Action Buttons */}
        <div className="modal-actions">
          <Button
            variant="secondary"
            onClick={handleCancel}
            disabled={isCreating}
          >
            Cancel
          </Button>
          <Button
            variant="primary"
            onClick={handleCreateVault}
            disabled={isCreating || !folderPath || !vaultName.trim()}
          >
            {isCreating ? (
              <>
                <span className="loading-spinner">⏳</span>
                Creating...
              </>
            ) : (
              <>
                <span className="button-icon">➕</span>
                Create Vault
              </>
            )}
          </Button>
        </div>
      </>
    </Modal>
  );
};
