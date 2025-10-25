/**
 * Vault Selection List Component
 * 
 * Component for selecting multiple vaults with filtering and sorting
 */

import React, { useState, useMemo, useCallback } from 'react';
import { VaultInfo } from '../../types';

export interface VaultSelectionListProps {
  vaults: VaultInfo[];
  selectedVaults: Set<string>;
  onSelectionChange: (vaultId: string, selected: boolean) => void;
  selectableFilter?: (vault: VaultInfo) => boolean;
  className?: string;
  showSearch?: boolean;
  showFilters?: boolean;
  maxHeight?: string;
}

interface FilterState {
  search: string;
  status: 'all' | 'mounted' | 'unmounted' | 'error';
  sortBy: 'name' | 'lastAccessed' | 'size' | 'status';
  sortOrder: 'asc' | 'desc';
}

/**
 * Vault selection list component
 */
export const VaultSelectionList: React.FC<VaultSelectionListProps> = ({
  vaults,
  selectedVaults,
  onSelectionChange,
  selectableFilter,
  className = '',
  showSearch = true,
  showFilters = true,
  maxHeight = '400px',
}) => {
  const [filters, setFilters] = useState<FilterState>({
    search: '',
    status: 'all',
    sortBy: 'name',
    sortOrder: 'asc',
  });

  // ==================== FILTERING AND SORTING ====================

  const filteredAndSortedVaults = useMemo(() => {
    let filtered = [...vaults];

    // Apply search filter
    if (filters.search) {
      const searchLower = filters.search.toLowerCase();
      filtered = filtered.filter(vault =>
        vault.name.toLowerCase().includes(searchLower) ||
        vault.path.toLowerCase().includes(searchLower)
      );
    }

    // Apply status filter
    if (filters.status !== 'all') {
      filtered = filtered.filter(vault => vault.status === filters.status);
    }

    // Apply selectability filter
    if (selectableFilter) {
      filtered = filtered.map(vault => ({
        ...vault,
        _selectable: selectableFilter(vault),
      }));
    }

    // Sort vaults
    filtered.sort((a, b) => {
      let comparison = 0;

      switch (filters.sortBy) {
        case 'name':
          comparison = a.name.localeCompare(b.name);
          break;
        case 'lastAccessed':
          comparison = a.lastAccessed.getTime() - b.lastAccessed.getTime();
          break;
        case 'size':
          comparison = a.size - b.size;
          break;
        case 'status':
          comparison = a.status.localeCompare(b.status);
          break;
      }

      return filters.sortOrder === 'asc' ? comparison : -comparison;
    });

    return filtered;
  }, [vaults, filters, selectableFilter]);

  // ==================== HANDLERS ====================

  const handleSearchChange = useCallback((search: string) => {
    setFilters(prev => ({ ...prev, search }));
  }, []);

  const handleFilterChange = useCallback((key: keyof FilterState, value: string) => {
    setFilters(prev => ({ ...prev, [key]: value }));
  }, []);

  const handleVaultToggle = useCallback((vault: VaultInfo) => {
    const isSelectable = !selectableFilter || selectableFilter(vault);
    if (!isSelectable) return;

    const isSelected = selectedVaults.has(vault.id);
    onSelectionChange(vault.id, !isSelected);
  }, [selectedVaults, onSelectionChange, selectableFilter]);

  const handleSelectAll = useCallback(() => {
    const selectableVaults = filteredAndSortedVaults.filter(vault => 
      !selectableFilter || selectableFilter(vault)
    );
    
    selectableVaults.forEach(vault => {
      if (!selectedVaults.has(vault.id)) {
        onSelectionChange(vault.id, true);
      }
    });
  }, [filteredAndSortedVaults, selectedVaults, onSelectionChange, selectableFilter]);

  const handleDeselectAll = useCallback(() => {
    filteredAndSortedVaults.forEach(vault => {
      if (selectedVaults.has(vault.id)) {
        onSelectionChange(vault.id, false);
      }
    });
  }, [filteredAndSortedVaults, selectedVaults, onSelectionChange]);

  // ==================== RENDER HELPERS ====================

  const getStatusIcon = (status: VaultInfo['status']) => {
    switch (status) {
      case 'mounted': return '🔓';
      case 'unmounted': return '🔒';
      case 'error': return '❌';
      default: return '❓';
    }
  };

  const getStatusColor = (status: VaultInfo['status']) => {
    switch (status) {
      case 'mounted': return '#4CAF50';
      case 'unmounted': return '#9E9E9E';
      case 'error': return '#F44336';
      default: return '#9E9E9E';
    }
  };

  const formatSize = (bytes: number) => {
    const units = ['B', 'KB', 'MB', 'GB', 'TB'];
    let size = bytes;
    let unitIndex = 0;

    while (size >= 1024 && unitIndex < units.length - 1) {
      size /= 1024;
      unitIndex++;
    }

    return `${size.toFixed(1)} ${units[unitIndex]}`;
  };

  const formatLastAccessed = (date: Date) => {
    const now = new Date();
    const diff = now.getTime() - date.getTime();
    const days = Math.floor(diff / (1000 * 60 * 60 * 24));

    if (days === 0) return 'Today';
    if (days === 1) return 'Yesterday';
    if (days < 7) return `${days} days ago`;
    if (days < 30) return `${Math.floor(days / 7)} weeks ago`;
    return date.toLocaleDateString();
  };

  const selectedCount = filteredAndSortedVaults.filter(v => selectedVaults.has(v.id)).length;
  const selectableCount = filteredAndSortedVaults.filter(v => 
    !selectableFilter || selectableFilter(v)
  ).length;

  // ==================== MAIN RENDER ====================

  return (
    <div className={`vault-selection-list ${className}`}>
      {/* Search and Filters */}
      {(showSearch || showFilters) && (
        <div className="selection-controls">
          {showSearch && (
            <div className="search-container">
              <input
                type="text"
                value={filters.search}
                onChange={(e) => handleSearchChange(e.target.value)}
                placeholder="Search vaults..."
                className="search-input"
              />
              <span className="search-icon">🔍</span>
            </div>
          )}

          {showFilters && (
            <div className="filter-controls">
              <select
                value={filters.status}
                onChange={(e) => handleFilterChange('status', e.target.value)}
                className="filter-select"
              >
                <option value="all">All Status</option>
                <option value="mounted">Mounted</option>
                <option value="unmounted">Unmounted</option>
                <option value="error">Error</option>
              </select>

              <select
                value={filters.sortBy}
                onChange={(e) => handleFilterChange('sortBy', e.target.value)}
                className="filter-select"
              >
                <option value="name">Sort by Name</option>
                <option value="lastAccessed">Sort by Last Accessed</option>
                <option value="size">Sort by Size</option>
                <option value="status">Sort by Status</option>
              </select>

              <button
                onClick={() => handleFilterChange('sortOrder', filters.sortOrder === 'asc' ? 'desc' : 'asc')}
                className="sort-order-button"
                title={`Sort ${filters.sortOrder === 'asc' ? 'Descending' : 'Ascending'}`}
              >
                {filters.sortOrder === 'asc' ? '↑' : '↓'}
              </button>
            </div>
          )}
        </div>
      )}

      {/* Selection Summary */}
      <div className="selection-summary">
        <div className="summary-info">
          <span className="summary-text">
            {selectedCount} of {filteredAndSortedVaults.length} vaults selected
            {selectableCount < filteredAndSortedVaults.length && (
              <span className="selectable-info">
                ({selectableCount} selectable)
              </span>
            )}
          </span>
        </div>
        
        <div className="summary-actions">
          <button
            onClick={handleSelectAll}
            className="summary-button"
            disabled={selectableCount === 0 || selectedCount === selectableCount}
          >
            Select All
          </button>
          <button
            onClick={handleDeselectAll}
            className="summary-button"
            disabled={selectedCount === 0}
          >
            Deselect All
          </button>
        </div>
      </div>

      {/* Vault List */}
      <div className="vault-list" style={{ maxHeight }}>
        {filteredAndSortedVaults.length > 0 ? (
          filteredAndSortedVaults.map(vault => {
            const isSelected = selectedVaults.has(vault.id);
            const isSelectable = !selectableFilter || selectableFilter(vault);

            return (
              <div
                key={vault.id}
                className={`vault-item ${isSelected ? 'selected' : ''} ${!isSelectable ? 'disabled' : ''}`}
                onClick={() => handleVaultToggle(vault)}
              >
                {/* Selection Checkbox */}
                <div className="vault-checkbox">
                  <input
                    type="checkbox"
                    checked={isSelected}
                    onChange={() => handleVaultToggle(vault)}
                    disabled={!isSelectable}
                    className="checkbox-input"
                  />
                </div>

                {/* Vault Info */}
                <div className="vault-info">
                  <div className="vault-header">
                    <div className="vault-name-status">
                      <span className="vault-name">{vault.name}</span>
                      <div className="vault-status">
                        <span 
                          className="status-icon"
                          style={{ color: getStatusColor(vault.status) }}
                        >
                          {getStatusIcon(vault.status)}
                        </span>
                        <span 
                          className="status-text"
                          style={{ color: getStatusColor(vault.status) }}
                        >
                          {vault.status.charAt(0).toUpperCase() + vault.status.slice(1)}
                        </span>
                      </div>
                    </div>
                  </div>

                  <div className="vault-details">
                    <div className="vault-path">
                      <span className="detail-icon">📁</span>
                      <span className="detail-text">{vault.path}</span>
                    </div>
                    
                    <div className="vault-metadata">
                      <div className="metadata-item">
                        <span className="metadata-icon">💾</span>
                        <span className="metadata-text">{formatSize(vault.size)}</span>
                      </div>
                      
                      <div className="metadata-item">
                        <span className="metadata-icon">📂</span>
                        <span className="metadata-text">{vault.folderCount} folders</span>
                      </div>
                      
                      <div className="metadata-item">
                        <span className="metadata-icon">🕒</span>
                        <span className="metadata-text">{formatLastAccessed(vault.lastAccessed)}</span>
                      </div>
                    </div>
                  </div>
                </div>

                {/* Selection Indicator */}
                {isSelected && (
                  <div className="selection-indicator">
                    <span className="indicator-icon">✓</span>
                  </div>
                )}

                {/* Non-selectable Overlay */}
                {!isSelectable && (
                  <div className="disabled-overlay">
                    <span className="disabled-icon">🚫</span>
                    <span className="disabled-text">Not available for this operation</span>
                  </div>
                )}
              </div>
            );
          })
        ) : (
          <div className="empty-list">
            <span className="empty-icon">📭</span>
            <p className="empty-text">
              {vaults.length === 0 
                ? 'No vaults available' 
                : 'No vaults match the current filters'
              }
            </p>
          </div>
        )}
      </div>
    </div>
  );
};