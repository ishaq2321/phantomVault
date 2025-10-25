/**
 * Vault Dashboard Component
 * 
 * Main dashboard interface for managing vaults
 * Displays vault status, provides quick actions, and shows real-time updates
 */

import React, { useState, useCallback, useEffect } from 'react';
import {
  VaultInfo,
  VaultAction,
  VaultDashboardProps,
  VaultStatus,
  UnlockMode
} from '../../types';
import { useVault, useApp } from '../../src/contexts';
import { useVaultOperations, useInterval, useVaultStatusMonitor } from '../../src/hooks';
import { VaultCard } from './VaultCard';
import { VaultStatusIndicator } from './VaultStatusIndicator';
import { QuickActions } from './QuickActions';
import { EmptyState } from './EmptyState';
import { VaultDetailsModal } from './VaultDetailsModal';
import './VaultDashboard.css';

/**
 * Main vault dashboard component
 */
export const VaultDashboard: React.FC<VaultDashboardProps> = ({
  vaults: externalVaults,
  onVaultAction,
  onCreateVault,
  refreshInterval = 5000,
  loading: externalLoading = false,
}) => {
  const { state: vaultState, actions: vaultActions } = useVault();
  const { state: appState, actions: appActions } = useApp();
  const vaultOps = useVaultOperations();
  
  // Use the status monitor for real-time updates
  const statusMonitor = useVaultStatusMonitor({
    config: {
      refreshInterval,
      enableAutoRefresh: true,
      enableStatusChangeEvents: true,
      enableStatusAnimations: true,
    },
    onStatusChange: (change) => {
      console.log('Dashboard received status change:', change);
    },
  });
  
  // Use vaults from status monitor if available, otherwise use external vaults
  const vaults = statusMonitor.vaults.length > 0 ? statusMonitor.vaults : (externalVaults || []);
  
  // Local state
  const [viewMode, setViewMode] = useState<'grid' | 'list'>('grid');
  const [sortBy, setSortBy] = useState<'name' | 'status' | 'lastAccess'>('name');
  const [filterStatus, setFilterStatus] = useState<VaultStatus | 'all'>('all');
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedVaultForDetails, setSelectedVaultForDetails] = useState<VaultInfo | null>(null);

  // Combine external and internal loading states
  const isLoading = externalLoading || vaultState.loading || vaultOps.loading || !statusMonitor.isConnected;

  // ==================== VAULT OPERATIONS ====================

  const handleVaultAction = useCallback(async (
    action: VaultAction,
    vaultId: string,
    options?: { password?: string; mode?: UnlockMode }
  ) => {
    try {
      let result;
      
      switch (action) {
        case 'mount':
          result = await vaultOps.mountVault(vaultId, options?.password);
          break;
        case 'unmount':
          result = await vaultOps.unmountVault(vaultId);
          break;
        case 'lock':
          result = await vaultOps.lockVault(vaultId);
          break;
        case 'unlock':
          if (!options?.password) {
            // Show password prompt
            // This would typically open a modal dialog
            return;
          }
          result = await vaultOps.unlockVault(vaultId, options.password, options.mode);
          break;
        case 'delete':
          // Show confirmation dialog first
          const confirmed = window.confirm('Are you sure you want to delete this vault?');
          if (!confirmed) return;
          
          result = await vaultOps.deleteVault(vaultId);
          break;
        default:
          console.warn(`Unsupported vault action: ${action}`);
          return;
      }

      if (result?.success) {
        appActions.addNotification({
          type: 'success',
          title: 'Operation Successful',
          message: result.message,
          duration: 3000,
        });
      } else {
        appActions.addNotification({
          type: 'error',
          title: 'Operation Failed',
          message: result?.error || 'Unknown error occurred',
          duration: 5000,
        });
      }

      // Call external handler if provided
      if (onVaultAction) {
        onVaultAction(action, vaultId);
      }
    } catch (error) {
      console.error(`Vault action ${action} failed:`, error);
      appActions.addNotification({
        type: 'error',
        title: 'Operation Failed',
        message: error instanceof Error ? error.message : 'Unknown error occurred',
        duration: 5000,
      });
    }
  }, [vaultOps, appActions, onVaultAction]);

  // ==================== FILTERING AND SORTING ====================

  const filteredAndSortedVaults = React.useMemo(() => {
    let filtered = vaults;

    // Apply search filter
    if (searchQuery.trim()) {
      const query = searchQuery.toLowerCase();
      filtered = filtered.filter(vault =>
        vault.name.toLowerCase().includes(query) ||
        vault.path.toLowerCase().includes(query)
      );
    }

    // Apply status filter
    if (filterStatus !== 'all') {
      filtered = filtered.filter(vault => vault.status === filterStatus);
    }

    // Apply sorting
    filtered.sort((a, b) => {
      switch (sortBy) {
        case 'name':
          return a.name.localeCompare(b.name);
        case 'status':
          return a.status.localeCompare(b.status);
        case 'lastAccess':
          return b.lastAccess.getTime() - a.lastAccess.getTime();
        default:
          return 0;
      }
    });

    return filtered;
  }, [vaults, searchQuery, filterStatus, sortBy]);

  // ==================== AUTO-REFRESH ====================
  
  // The status monitor handles auto-refresh, but we can also manually refresh
  const handleManualRefresh = useCallback(async () => {
    try {
      await statusMonitor.refresh();
      await vaultActions.loadVaults();
    } catch (error) {
      console.error('Manual refresh failed:', error);
    }
  }, [statusMonitor, vaultActions]);

  const handleViewDetails = useCallback((vault: VaultInfo) => {
    setSelectedVaultForDetails(vault);
  }, []);

  const handleCloseDetails = useCallback(() => {
    setSelectedVaultForDetails(null);
  }, []);

  // ==================== BULK OPERATIONS ====================

  const [selectedVaults, setSelectedVaults] = useState<Set<string>>(new Set());

  const handleSelectVault = useCallback((vaultId: string, selected: boolean) => {
    setSelectedVaults(prev => {
      const newSet = new Set(prev);
      if (selected) {
        newSet.add(vaultId);
      } else {
        newSet.delete(vaultId);
      }
      return newSet;
    });
  }, []);

  const handleSelectAll = useCallback((selected: boolean) => {
    if (selected) {
      setSelectedVaults(new Set(filteredAndSortedVaults.map(v => v.id)));
    } else {
      setSelectedVaults(new Set());
    }
  }, [filteredAndSortedVaults]);

  const handleBulkAction = useCallback(async (action: VaultAction) => {
    const vaultIds = Array.from(selectedVaults);
    
    if (vaultIds.length === 0) {
      appActions.addNotification({
        type: 'warning',
        title: 'No Vaults Selected',
        message: 'Please select one or more vaults to perform bulk operations.',
        duration: 3000,
      });
      return;
    }

    try {
      let result;
      
      switch (action) {
        case 'mount':
          result = await vaultOps.mountMultiple(vaultIds);
          break;
        case 'unmount':
          result = await vaultOps.unmountMultiple(vaultIds);
          break;
        default:
          appActions.addNotification({
            type: 'warning',
            title: 'Unsupported Action',
            message: `Bulk ${action} is not supported yet.`,
            duration: 3000,
          });
          return;
      }

      if (result) {
        appActions.addNotification({
          type: result.successCount > 0 ? 'success' : 'error',
          title: 'Bulk Operation Complete',
          message: `${result.successCount} succeeded, ${result.failedCount} failed`,
          duration: 5000,
        });
        
        // Clear selection after bulk operation
        setSelectedVaults(new Set());
      }
    } catch (error) {
      console.error(`Bulk ${action} failed:`, error);
      appActions.addNotification({
        type: 'error',
        title: 'Bulk Operation Failed',
        message: error instanceof Error ? error.message : 'Unknown error occurred',
        duration: 5000,
      });
    }
  }, [selectedVaults, vaultOps, appActions]);

  // ==================== RENDER ====================

  return (
    <div className="vault-dashboard">
      {/* Dashboard Header */}
      <div className="dashboard-header">
        <div className="header-left">
          <h1 className="dashboard-title">
            <span className="title-icon">🔐</span>
            Vault Dashboard
          </h1>
          <VaultStatusIndicator 
            totalVaults={vaults.length}
            mountedVaults={vaults.filter(v => v.status === 'mounted').length}
            errorVaults={vaults.filter(v => v.status === 'error').length}
          />
        </div>
        
        <div className="header-right">
          <QuickActions
            selectedCount={selectedVaults.size}
            onBulkAction={handleBulkAction}
            onRefresh={handleManualRefresh}
            onCreateVault={onCreateVault}
            loading={isLoading}
          />
        </div>
      </div>

      {/* Filters and Controls */}
      <div className="dashboard-controls">
        <div className="controls-left">
          <div className="search-box">
            <input
              type="text"
              placeholder="Search vaults..."
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              className="search-input"
            />
            <span className="search-icon">🔍</span>
          </div>
          
          <select
            value={filterStatus}
            onChange={(e) => setFilterStatus(e.target.value as VaultStatus | 'all')}
            className="filter-select"
          >
            <option value="all">All Status</option>
            <option value="mounted">Mounted</option>
            <option value="unmounted">Unmounted</option>
            <option value="error">Error</option>
            <option value="loading">Loading</option>
          </select>
          
          <select
            value={sortBy}
            onChange={(e) => setSortBy(e.target.value as typeof sortBy)}
            className="sort-select"
          >
            <option value="name">Sort by Name</option>
            <option value="status">Sort by Status</option>
            <option value="lastAccess">Sort by Last Access</option>
          </select>
        </div>
        
        <div className="controls-right">
          <div className="view-toggle">
            <button
              className={`view-button ${viewMode === 'grid' ? 'active' : ''}`}
              onClick={() => setViewMode('grid')}
              title="Grid View"
            >
              ⊞
            </button>
            <button
              className={`view-button ${viewMode === 'list' ? 'active' : ''}`}
              onClick={() => setViewMode('list')}
              title="List View"
            >
              ☰
            </button>
          </div>
          
          {filteredAndSortedVaults.length > 0 && (
            <div className="bulk-select">
              <label className="select-all-label">
                <input
                  type="checkbox"
                  checked={selectedVaults.size === filteredAndSortedVaults.length}
                  onChange={(e) => handleSelectAll(e.target.checked)}
                />
                Select All ({filteredAndSortedVaults.length})
              </label>
            </div>
          )}
        </div>
      </div>

      {/* Vault Content */}
      <div className="dashboard-content">
        {isLoading && vaults.length === 0 ? (
          <div className="loading-state">
            <div className="loading-spinner"></div>
            <p>Loading vaults...</p>
          </div>
        ) : filteredAndSortedVaults.length === 0 ? (
          searchQuery || filterStatus !== 'all' ? (
            <div className="no-results-state">
              <div className="no-results-icon">🔍</div>
              <h3>No vaults found</h3>
              <p>Try adjusting your search or filter criteria.</p>
              <button
                onClick={() => {
                  setSearchQuery('');
                  setFilterStatus('all');
                }}
                className="clear-filters-button"
              >
                Clear Filters
              </button>
            </div>
          ) : (
            <EmptyState onCreateVault={onCreateVault || (() => console.log('No onCreateVault handler provided'))} />
          )
        ) : (
          <div className={`vault-${viewMode}`}>
            {filteredAndSortedVaults.map((vault) => (
              <VaultCard
                key={vault.id}
                vault={vault}
                viewMode={viewMode}
                selected={selectedVaults.has(vault.id)}
                onSelect={(selected) => handleSelectVault(vault.id, selected)}
                onAction={(action, options) => handleVaultAction(action, vault.id, options)}
                onViewDetails={handleViewDetails}
                loading={isLoading}
              />
            ))}
          </div>
        )}
      </div>

      {/* Vault Details Modal */}
      {selectedVaultForDetails && (
        <VaultDetailsModal
          vault={selectedVaultForDetails}
          isOpen={true}
          onClose={handleCloseDetails}
          onAction={(action) => handleVaultAction(action, selectedVaultForDetails.id)}
        />
      )}

      {/* Status Bar */}
      {filteredAndSortedVaults.length > 0 && (
        <div className="dashboard-status-bar">
          <span className="status-text">
            Showing {filteredAndSortedVaults.length} of {vaults.length} vaults
          </span>
          {selectedVaults.size > 0 && (
            <span className="selection-text">
              {selectedVaults.size} selected
            </span>
          )}
          <span className="last-updated">
            Last updated: {new Date().toLocaleTimeString()}
          </span>
        </div>
      )}
    </div>
  );
};