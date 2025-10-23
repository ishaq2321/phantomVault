/**
 * Vault Operations Hook
 * 
 * Custom React hook for managing vault operations
 * Provides high-level vault management functions with error handling and loading states
 */

import { useState, useCallback } from 'react';
import {
  VaultConfig,
  VaultOperationResult,
  BulkOperationResult,
  UnlockMode,
  AppError,
  UseVaultOperations
} from '../types';
import { useVault } from '../contexts';

/**
 * Custom hook for vault operations
 * Provides methods for creating, mounting, unmounting, and managing vaults
 */
export const useVaultOperations = (): UseVaultOperations => {
  const { state, actions } = useVault();
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<AppError | null>(null);

  // ==================== HELPER FUNCTIONS ====================

  const handleError = useCallback((error: unknown, operation: string): AppError => {
    const appError: AppError = {
      type: 'system',
      code: `${operation.toUpperCase()}_FAILED`,
      message: error instanceof Error ? error.message : `Failed to ${operation}`,
      timestamp: new Date(),
      recoverable: true,
    };
    setError(appError);
    return appError;
  }, []);

  const withErrorHandling = useCallback(async <T>(
    operation: () => Promise<T>,
    operationName: string
  ): Promise<T> => {
    try {
      setLoading(true);
      setError(null);
      return await operation();
    } catch (err) {
      throw handleError(err, operationName);
    } finally {
      setLoading(false);
    }
  }, [handleError]);

  // ==================== VAULT OPERATIONS ====================

  const createVault = useCallback(async (config: VaultConfig): Promise<VaultOperationResult> => {
    return withErrorHandling(async () => {
      // Validate configuration
      if (!config.name.trim()) {
        throw new Error('Vault name is required');
      }
      if (!config.path.trim()) {
        throw new Error('Vault path is required');
      }
      if (!config.password.trim()) {
        throw new Error('Password is required');
      }

      // Use the vault context to create the vault
      return await actions.createVault(config);
    }, 'create vault');
  }, [actions, withErrorHandling]);

  const mountVault = useCallback(async (
    vaultId: string, 
    password?: string
  ): Promise<VaultOperationResult> => {
    return withErrorHandling(async () => {
      if (!state.activeProfile) {
        throw new Error('No active profile selected');
      }

      // Use the PhantomVault API to unlock the folder
      const response = await window.phantomVault.folder.unlock(
        state.activeProfile.id,
        vaultId,
        password || '',
        'temporary'
      );

      if (response.success) {
        // Update vault status in context
        const vault = state.vaults.find(v => v.id === vaultId);
        if (vault) {
          // This would trigger a re-render with updated status
          await actions.loadVaults();
        }

        return {
          success: true,
          message: 'Vault mounted successfully',
          vaultId,
        };
      } else {
        throw new Error(response.error || 'Failed to mount vault');
      }
    }, 'mount vault');
  }, [state.activeProfile, state.vaults, actions, withErrorHandling]);

  const unmountVault = useCallback(async (vaultId: string): Promise<VaultOperationResult> => {
    return withErrorHandling(async () => {
      if (!state.activeProfile) {
        throw new Error('No active profile selected');
      }

      // Use the PhantomVault API to lock the folder
      const response = await window.phantomVault.folder.lock(
        state.activeProfile.id,
        vaultId
      );

      if (response.success) {
        // Update vault status in context
        await actions.loadVaults();

        return {
          success: true,
          message: 'Vault unmounted successfully',
          vaultId,
        };
      } else {
        throw new Error(response.error || 'Failed to unmount vault');
      }
    }, 'unmount vault');
  }, [state.activeProfile, actions, withErrorHandling]);

  const deleteVault = useCallback(async (vaultId: string): Promise<VaultOperationResult> => {
    return withErrorHandling(async () => {
      // Use the vault context to delete the vault
      return await actions.deleteVault(vaultId);
    }, 'delete vault');
  }, [actions, withErrorHandling]);

  const lockVault = useCallback(async (vaultId: string): Promise<VaultOperationResult> => {
    return withErrorHandling(async () => {
      if (!state.activeProfile) {
        throw new Error('No active profile selected');
      }

      // Use the PhantomVault API to lock the folder with password
      const vault = state.vaults.find(v => v.id === vaultId);
      if (!vault) {
        throw new Error('Vault not found');
      }

      // For locking, we need to encrypt and hide the folder
      // This would typically require the user's password
      const response = await window.phantomVault.folder.lockWithPassword(
        state.activeProfile.id,
        vaultId,
        '' // Password would be provided by user input
      );

      if (response.success) {
        await actions.loadVaults();

        return {
          success: true,
          message: 'Vault locked successfully',
          vaultId,
        };
      } else {
        throw new Error(response.error || 'Failed to lock vault');
      }
    }, 'lock vault');
  }, [state.activeProfile, state.vaults, actions, withErrorHandling]);

  const unlockVault = useCallback(async (
    vaultId: string,
    password: string,
    mode: UnlockMode = 'temporary'
  ): Promise<VaultOperationResult> => {
    return withErrorHandling(async () => {
      if (!state.activeProfile) {
        throw new Error('No active profile selected');
      }

      if (!password.trim()) {
        throw new Error('Password is required');
      }

      // Use the PhantomVault API to unlock the folder
      const response = await window.phantomVault.folder.unlock(
        state.activeProfile.id,
        vaultId,
        password,
        mode
      );

      if (response.success) {
        await actions.loadVaults();

        return {
          success: true,
          message: `Vault unlocked successfully in ${mode} mode`,
          vaultId,
        };
      } else {
        throw new Error(response.error || 'Failed to unlock vault');
      }
    }, 'unlock vault');
  }, [state.activeProfile, actions, withErrorHandling]);

  // ==================== BULK OPERATIONS ====================

  const mountMultiple = useCallback(async (
    vaultIds: string[],
    password?: string
  ): Promise<BulkOperationResult> => {
    return withErrorHandling(async () => {
      const results: VaultOperationResult[] = [];
      let successCount = 0;
      let failedCount = 0;

      for (const vaultId of vaultIds) {
        try {
          const result = await mountVault(vaultId, password);
          results.push(result);
          if (result.success) {
            successCount++;
          } else {
            failedCount++;
          }
        } catch (error) {
          const errorResult: VaultOperationResult = {
            success: false,
            message: error instanceof Error ? error.message : 'Unknown error',
            vaultId,
            error: error instanceof Error ? error.message : 'Unknown error',
          };
          results.push(errorResult);
          failedCount++;
        }
      }

      return {
        totalCount: vaultIds.length,
        successCount,
        failedCount,
        results,
      };
    }, 'mount multiple vaults');
  }, [mountVault, withErrorHandling]);

  const unmountMultiple = useCallback(async (vaultIds: string[]): Promise<BulkOperationResult> => {
    return withErrorHandling(async () => {
      const results: VaultOperationResult[] = [];
      let successCount = 0;
      let failedCount = 0;

      for (const vaultId of vaultIds) {
        try {
          const result = await unmountVault(vaultId);
          results.push(result);
          if (result.success) {
            successCount++;
          } else {
            failedCount++;
          }
        } catch (error) {
          const errorResult: VaultOperationResult = {
            success: false,
            message: error instanceof Error ? error.message : 'Unknown error',
            vaultId,
            error: error instanceof Error ? error.message : 'Unknown error',
          };
          results.push(errorResult);
          failedCount++;
        }
      }

      return {
        totalCount: vaultIds.length,
        successCount,
        failedCount,
        results,
      };
    }, 'unmount multiple vaults');
  }, [unmountVault, withErrorHandling]);

  // ==================== RETURN HOOK INTERFACE ====================

  return {
    // Operations
    createVault,
    mountVault,
    unmountVault,
    deleteVault,
    lockVault,
    unlockVault,
    
    // Bulk operations
    mountMultiple,
    unmountMultiple,
    
    // State
    loading: loading || state.loading,
    error: error || state.error,
  };
};