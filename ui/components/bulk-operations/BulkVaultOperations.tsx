/**
 * Bulk Vault Operations Component
 * 
 * Component for performing operations on multiple vaults simultaneously
 */

import React, { useState, useCallback, useMemo, useEffect } from 'react';
import { VaultInfo, VaultOperationResult, BulkOperationProgress, BulkOperationResult } from '../../types';
import { useVault, useApp } from '../../src/contexts';
import { useVaultOperations } from '../../src/hooks';
import { BulkOperationModal } from './BulkOperationModal';
import { VaultSelectionList } from './VaultSelectionList';
import { BulkOperationSummary } from './BulkOperationSummary';

export interface BulkVaultOperationsProps {
  vaults: VaultInfo[];
  onSelectionChange?: (selectedVaults: VaultInfo[]) => void;
  className?: string;
  showSelectionControls?: boolean;
  maxConcurrentOperations?: number;
}

interface BulkOperationState {
  selectedVaults: Set<string>;
  operation: 'mount' | 'unmount' | 'delete' | null;
  isExecuting: boolean;
  progress: BulkOperationProgress | null;
  results: BulkOperationResult | null;
  showModal: boolean;
  showSummary: boolean;
}

/**
 * Bulk vault operations component
 */
export const BulkVaultOperations: React.FC<BulkVaultOperationsProps> = ({
  vaults,
  onSelectionChange,
  className = '',
  showSelectionControls = true,
  maxConcurrentOperations = 3,
}) => {
  const { actions: appActions } = useApp();
  const vaultOps = useVaultOperations();

  // ==================== STATE MANAGEMENT ====================

  const [bulkState, setBulkState] = useState<BulkOperationState>({
    selectedVaults: new Set(),
    operation: null,
    isExecuting: false,
    progress: null,
    results: null,
    showModal: false,
    showSummary: false,
  });

  // ==================== SELECTION MANAGEMENT ====================

  const selectedVaultsList = useMemo(() => {
    return vaults.filter(vault => bulkState.selectedVaults.has(vault.id));
  }, [vaults, bulkState.selectedVaults]);

  const selectableVaults = useMemo(() => {
    return vaults.filter(vault => {
      // Filter based on current operation context
      switch (bulkState.operation) {
        case 'mount':
          return vault.status === 'unmounted' || vault.status === 'error';
        case 'unmount':
          return vault.status === 'mounted';
        case 'delete':
          return vault.status !== 'mounted'; // Can't delete mounted vaults
        default:
          return true;
      }
    });
  }, [vaults, bulkState.operation]);

  const handleVaultSelection = useCallback((vaultId: string, selected: boolean) => {
    setBulkState(prev => {
      const newSelected = new Set(prev.selectedVaults);
      if (selected) {
        newSelected.add(vaultId);
      } else {
        newSelected.delete(vaultId);
      }
      return { ...prev, selectedVaults: newSelected };
    });
  }, []);

  const selectAll = useCallback(() => {
    setBulkState(prev => ({
      ...prev,
      selectedVaults: new Set(selectableVaults.map(v => v.id)),
    }));
  }, [selectableVaults]);

  const selectNone = useCallback(() => {
    setBulkState(prev => ({
      ...prev,
      selectedVaults: new Set(),
    }));
  }, []);

  const selectByStatus = useCallback((status: VaultInfo['status']) => {
    const vaultsWithStatus = vaults.filter(v => v.status === status);
    setBulkState(prev => ({
      ...prev,
      selectedVaults: new Set(vaultsWithStatus.map(v => v.id)),
    }));
  }, [vaults]);

  // ==================== OPERATION EXECUTION ====================

  const executeBulkOperation = useCallback(async (
    operation: 'mount' | 'unmount' | 'delete',
    selectedVaults: VaultInfo[],
    options?: { passwords?: Record<string, string>; force?: boolean }
  ) => {
    setBulkState(prev => ({
      ...prev,
      isExecuting: true,
      operation,
      progress: {
        total: selectedVaults.length,
        completed: 0,
        failed: 0,
        current: null,
        results: [],
      },
      showModal: true,
    }));

    const results: BulkOperationResult = {
      operation,
      total: selectedVaults.length,
      successful: [],
      failed: [],
      skipped: [],
      startTime: new Date(),
      endTime: new Date(),
    };

    try {
      // Process vaults in batches to avoid overwhelming the system
      const batches = [];
      for (let i = 0; i < selectedVaults.length; i += maxConcurrentOperations) {
        batches.push(selectedVaults.slice(i, i + maxConcurrentOperations));
      }

      for (const batch of batches) {
        const batchPromises = batch.map(async (vault) => {
          setBulkState(prev => ({
            ...prev,
            progress: prev.progress ? {
              ...prev.progress,
              current: vault,
            } : null,
          }));

          try {
            let result: VaultOperationResult;

            switch (operation) {
              case 'mount':
                const password = options?.passwords?.[vault.id];
                result = await vaultOps.mountVault(vault.id, password);
                break;
              case 'unmount':
                result = await vaultOps.unmountVault(vault.id, { force: options?.force });
                break;
              case 'delete':
                result = await vaultOps.deleteVault(vault.id);
                break;
              default:
                throw new Error(`Unknown operation: ${operation}`);
            }

            if (result.success) {
              results.successful.push({
                vault,
                result,
                message: `${operation} completed successfully`,
              });
            } else {
              results.failed.push({
                vault,
                result,
                error: result.error || 'Unknown error',
                message: result.error || `${operation} failed`,
              });
            }
          } catch (error) {
            results.failed.push({
              vault,
              result: { success: false, error: error instanceof Error ? error.message : 'Unknown error' },
              error: error instanceof Error ? error.message : 'Unknown error',
              message: `${operation} failed with error`,
            });
          }

          // Update progress
          setBulkState(prev => ({
            ...prev,
            progress: prev.progress ? {
              ...prev.progress,
              completed: prev.progress.completed + 1,
              failed: results.failed.length,
              results: [...results.successful, ...results.failed],
            } : null,
          }));
        });

        // Wait for current batch to complete before starting next
        await Promise.all(batchPromises);
      }

      results.endTime = new Date();

      // Show completion notification
      const successCount = results.successful.length;
      const failedCount = results.failed.length;

      if (failedCount === 0) {
        appActions.addNotification({
          type: 'success',
          title: 'Bulk Operation Completed',
          message: `Successfully ${operation}ed ${successCount} vault${successCount !== 1 ? 's' : ''}.`,
          duration: 5000,
        });
      } else if (successCount === 0) {
        appActions.addNotification({
          type: 'error',
          title: 'Bulk Operation Failed',
          message: `Failed to ${operation} all ${failedCount} vault${failedCount !== 1 ? 's' : ''}.`,
          duration: 8000,
        });
      } else {
        appActions.addNotification({
          type: 'warning',
          title: 'Bulk Operation Partially Completed',
          message: `${successCount} successful, ${failedCount} failed out of ${results.total} vaults.`,
          duration: 8000,
        });
      }

    } catch (error) {
      appActions.addNotification({
        type: 'error',
        title: 'Bulk Operation Error',
        message: error instanceof Error ? error.message : 'An unexpected error occurred.',
        duration: 8000,
      });
    } finally {
      setBulkState(prev => ({
        ...prev,
        isExecuting: false,
        results,
        showSummary: true,
        progress: null,
        current: null,
      }));
    }
  }, [maxConcurrentOperations, vaultOps, appActions]);

  // ==================== OPERATION HANDLERS ====================

  const handleBulkMount = useCallback(() => {
    if (selectedVaultsList.length === 0) {
      appActions.addNotification({
        type: 'warning',
        title: 'No Vaults Selected',
        message: 'Please select at least one vault to mount.',
        duration: 3000,
      });
      return;
    }

    setBulkState(prev => ({
      ...prev,
      operation: 'mount',
      showModal: true,
    }));
  }, [selectedVaultsList.length, appActions]);

  const handleBulkUnmount = useCallback(() => {
    const mountedVaults = selectedVaultsList.filter(v => v.status === 'mounted');
    
    if (mountedVaults.length === 0) {
      appActions.addNotification({
        type: 'warning',
        title: 'No Mounted Vaults Selected',
        message: 'Please select at least one mounted vault to unmount.',
        duration: 3000,
      });
      return;
    }

    setBulkState(prev => ({
      ...prev,
      operation: 'unmount',
      showModal: true,
    }));
  }, [selectedVaultsList, appActions]);

  const handleBulkDelete = useCallback(() => {
    const deletableVaults = selectedVaultsList.filter(v => v.status !== 'mounted');
    
    if (deletableVaults.length === 0) {
      appActions.addNotification({
        type: 'warning',
        title: 'No Deletable Vaults Selected',
        message: 'Cannot delete mounted vaults. Please unmount them first.',
        duration: 3000,
      });
      return;
    }

    setBulkState(prev => ({
      ...prev,
      operation: 'delete',
      showModal: true,
    }));
  }, [selectedVaultsList, appActions]);

  const handleModalConfirm = useCallback((options?: { passwords?: Record<string, string>; force?: boolean }) => {
    if (bulkState.operation && selectedVaultsList.length > 0) {
      executeBulkOperation(bulkState.operation, selectedVaultsList, options);
    }
  }, [bulkState.operation, selectedVaultsList, executeBulkOperation]);

  const handleModalCancel = useCallback(() => {
    setBulkState(prev => ({
      ...prev,
      operation: null,
      showModal: false,
    }));
  }, []);

  const handleSummaryClose = useCallback(() => {
    setBulkState(prev => ({
      ...prev,
      showSummary: false,
      results: null,
      selectedVaults: new Set(), // Clear selection after operation
    }));
  }, []);

  // ==================== EFFECTS ====================

  useEffect(() => {
    if (onSelectionChange) {
      onSelectionChange(selectedVaultsList);
    }
  }, [selectedVaultsList, onSelectionChange]);

  // ==================== RENDER HELPERS ====================

  const getOperationCounts = () => {
    const mountable = selectedVaultsList.filter(v => v.status === 'unmounted' || v.status === 'error').length;
    const unmountable = selectedVaultsList.filter(v => v.status === 'mounted').length;
    const deletable = selectedVaultsList.filter(v => v.status !== 'mounted').length;

    return { mountable, unmountable, deletable };
  };

  const operationCounts = getOperationCounts();

  // ==================== MAIN RENDER ====================

  return (
    <div className={`bulk-vault-operations ${className}`}>
      {/* Selection Controls */}
      {showSelectionControls && (
        <div className="bulk-selection-controls">
          <div className="selection-info">
            <span className="selection-count">
              {bulkState.selectedVaults.size} of {vaults.length} vaults selected
            </span>
          </div>

          <div className="selection-actions">
            <button
              onClick={selectAll}
              className="selection-button"
              disabled={selectableVaults.length === 0}
            >
              Select All
            </button>
            <button
              onClick={selectNone}
              className="selection-button"
              disabled={bulkState.selectedVaults.size === 0}
            >
              Select None
            </button>
            <button
              onClick={() => selectByStatus('unmounted')}
              className="selection-button"
            >
              Select Unmounted
            </button>
            <button
              onClick={() => selectByStatus('mounted')}
              className="selection-button"
            >
              Select Mounted
            </button>
          </div>
        </div>
      )}

      {/* Vault Selection List */}
      <VaultSelectionList
        vaults={vaults}
        selectedVaults={bulkState.selectedVaults}
        onSelectionChange={handleVaultSelection}
        selectableFilter={(vault) => selectableVaults.includes(vault)}
      />

      {/* Bulk Operation Controls */}
      {bulkState.selectedVaults.size > 0 && (
        <div className="bulk-operation-controls">
          <div className="operation-buttons">
            <button
              onClick={handleBulkMount}
              className="bulk-operation-button mount"
              disabled={operationCounts.mountable === 0 || bulkState.isExecuting}
            >
              <span className="button-icon">🔓</span>
              <span className="button-text">
                Mount ({operationCounts.mountable})
              </span>
            </button>

            <button
              onClick={handleBulkUnmount}
              className="bulk-operation-button unmount"
              disabled={operationCounts.unmountable === 0 || bulkState.isExecuting}
            >
              <span className="button-icon">🔒</span>
              <span className="button-text">
                Unmount ({operationCounts.unmountable})
              </span>
            </button>

            <button
              onClick={handleBulkDelete}
              className="bulk-operation-button delete"
              disabled={operationCounts.deletable === 0 || bulkState.isExecuting}
            >
              <span className="button-icon">🗑️</span>
              <span className="button-text">
                Delete ({operationCounts.deletable})
              </span>
            </button>
          </div>

          <div className="operation-info">
            <span className="info-text">
              Operations will be performed on selected vaults only
            </span>
          </div>
        </div>
      )}

      {/* Bulk Operation Modal */}
      {bulkState.showModal && bulkState.operation && (
        <BulkOperationModal
          operation={bulkState.operation}
          vaults={selectedVaultsList}
          progress={bulkState.progress}
          isExecuting={bulkState.isExecuting}
          onConfirm={handleModalConfirm}
          onCancel={handleModalCancel}
        />
      )}

      {/* Operation Summary */}
      {bulkState.showSummary && bulkState.results && (
        <BulkOperationSummary
          results={bulkState.results}
          onClose={handleSummaryClose}
          onRetryFailed={() => {
            // Retry failed operations
            const failedVaults = bulkState.results!.failed.map(f => f.vault);
            if (failedVaults.length > 0 && bulkState.results!.operation) {
              executeBulkOperation(bulkState.results!.operation, failedVaults);
            }
          }}
        />
      )}
    </div>
  );
};