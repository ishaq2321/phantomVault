/**
 * Activity Filter Panel Component
 * 
 * Comprehensive filtering interface for activity log entries
 */

import React, { useState, useCallback } from 'react';
import { ActivityFilter } from '../../types';
import { useVault } from '../../contexts';

export interface ActivityFilterPanelProps {
  filters: ActivityFilter;
  onFiltersChange: (filters: Partial<ActivityFilter>) => void;
  entryCount: number;
  className?: string;
}

/**
 * Activity filter panel component
 */
export const ActivityFilterPanel: React.FC<ActivityFilterPanelProps> = ({
  filters,
  onFiltersChange,
  entryCount,
  className = '',
}) => {
  const { state: vaultState } = useVault();
  const [isExpanded, setIsExpanded] = useState(false);

  // Handle filter changes
  const handleFilterChange = useCallback((key: keyof ActivityFilter, value: any) => {
    onFiltersChange({ [key]: value });
  }, [onFiltersChange]);

  // Clear all filters
  const clearFilters = useCallback(() => {
    onFiltersChange({
      vaultId: 'all',
      level: 'all',
      timeRange: 'all',
      search: '',
      dateFrom: null,
      dateTo: null,
    });
  }, [onFiltersChange]);

  // Check if any filters are active
  const hasActiveFilters = filters.vaultId !== 'all' || 
                          filters.level !== 'all' || 
                          filters.timeRange !== 'all' || 
                          filters.search !== '' ||
                          filters.dateFrom !== null ||
                          filters.dateTo !== null;

  return (
    <div className={`activity-filter-panel ${isExpanded ? 'expanded' : 'collapsed'} ${className}`}>
      {/* Filter Header */}
      <div className="filter-header">
        <div className="filter-info">
          <button 
            onClick={() => setIsExpanded(!isExpanded)}
            className="expand-button"
            title={isExpanded ? 'Collapse filters' : 'Expand filters'}
          >
            <span className="expand-icon">{isExpanded ? '▼' : '▶'}</span>
            <span className="filter-label">Filters</span>
          </button>
          
          <span className="result-count">
            {entryCount} {entryCount === 1 ? 'entry' : 'entries'}
          </span>
          
          {hasActiveFilters && (
            <span className="active-filters-indicator">
              🔍 Active
            </span>
          )}
        </div>
        
        {hasActiveFilters && (
          <button 
            onClick={clearFilters}
            className="clear-filters-button"
            title="Clear all filters"
          >
            Clear
          </button>
        )}
      </div>

      {/* Filter Controls */}
      {isExpanded && (
        <div className="filter-controls">
          {/* Quick Search */}
          <div className="filter-group">
            <label className="filter-label">Search</label>
            <div className="search-input-container">
              <input
                type="text"
                value={filters.search}
                onChange={(e) => handleFilterChange('search', e.target.value)}
                placeholder="Search log messages..."
                className="search-input"
              />
              {filters.search && (
                <button 
                  onClick={() => handleFilterChange('search', '')}
                  className="clear-search-button"
                  title="Clear search"
                >
                  ✕
                </button>
              )}
            </div>
          </div>

          {/* Vault Filter */}
          <div className="filter-group">
            <label className="filter-label">Vault</label>
            <select
              value={filters.vaultId}
              onChange={(e) => handleFilterChange('vaultId', e.target.value)}
              className="filter-select"
            >
              <option value="all">All Vaults</option>
              <option value="system">System</option>
              {vaultState.vaults.map(vault => (
                <option key={vault.id} value={vault.id}>
                  {vault.name}
                </option>
              ))}
            </select>
          </div>

          {/* Level Filter */}
          <div className="filter-group">
            <label className="filter-label">Level</label>
            <select
              value={filters.level}
              onChange={(e) => handleFilterChange('level', e.target.value)}
              className="filter-select"
            >
              <option value="all">All Levels</option>
              <option value="error">❌ Error</option>
              <option value="warning">⚠️ Warning</option>
              <option value="info">ℹ️ Info</option>
              <option value="debug">🔍 Debug</option>
            </select>
          </div>

          {/* Time Range Filter */}
          <div className="filter-group">
            <label className="filter-label">Time Range</label>
            <select
              value={filters.timeRange}
              onChange={(e) => handleFilterChange('timeRange', e.target.value)}
              className="filter-select"
            >
              <option value="all">All Time</option>
              <option value="hour">Last Hour</option>
              <option value="day">Last 24 Hours</option>
              <option value="week">Last Week</option>
              <option value="month">Last Month</option>
              <option value="custom">Custom Range</option>
            </select>
          </div>

          {/* Custom Date Range */}
          {filters.timeRange === 'custom' && (
            <div className="filter-group custom-date-range">
              <div className="date-input-group">
                <label className="date-label">From</label>
                <input
                  type="datetime-local"
                  value={filters.dateFrom ? filters.dateFrom.toISOString().slice(0, 16) : ''}
                  onChange={(e) => handleFilterChange('dateFrom', e.target.value ? new Date(e.target.value) : null)}
                  className="date-input"
                />
              </div>
              <div className="date-input-group">
                <label className="date-label">To</label>
                <input
                  type="datetime-local"
                  value={filters.dateTo ? filters.dateTo.toISOString().slice(0, 16) : ''}
                  onChange={(e) => handleFilterChange('dateTo', e.target.value ? new Date(e.target.value) : null)}
                  className="date-input"
                />
              </div>
            </div>
          )}

          {/* Quick Filter Buttons */}
          <div className="filter-group quick-filters">
            <label className="filter-label">Quick Filters</label>
            <div className="quick-filter-buttons">
              <button 
                onClick={() => handleFilterChange('level', 'error')}
                className={`quick-filter-button ${filters.level === 'error' ? 'active' : ''}`}
              >
                Errors Only
              </button>
              <button 
                onClick={() => handleFilterChange('level', 'warning')}
                className={`quick-filter-button ${filters.level === 'warning' ? 'active' : ''}`}
              >
                Warnings Only
              </button>
              <button 
                onClick={() => handleFilterChange('timeRange', 'hour')}
                className={`quick-filter-button ${filters.timeRange === 'hour' ? 'active' : ''}`}
              >
                Last Hour
              </button>
            </div>
          </div>
        </div>
      )}

      {/* Active Filters Summary */}
      {hasActiveFilters && !isExpanded && (
        <div className="active-filters-summary">
          {filters.search && (
            <span className="filter-tag">
              Search: "{filters.search}"
              <button onClick={() => handleFilterChange('search', '')}>✕</button>
            </span>
          )}
          {filters.vaultId !== 'all' && (
            <span className="filter-tag">
              Vault: {vaultState.vaults.find(v => v.id === filters.vaultId)?.name || filters.vaultId}
              <button onClick={() => handleFilterChange('vaultId', 'all')}>✕</button>
            </span>
          )}
          {filters.level !== 'all' && (
            <span className="filter-tag">
              Level: {filters.level}
              <button onClick={() => handleFilterChange('level', 'all')}>✕</button>
            </span>
          )}
          {filters.timeRange !== 'all' && (
            <span className="filter-tag">
              Time: {filters.timeRange}
              <button onClick={() => handleFilterChange('timeRange', 'all')}>✕</button>
            </span>
          )}
        </div>
      )}
    </div>
  );
};