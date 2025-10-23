/**
 * Log Filter System Component
 * 
 * Advanced filtering system for activity logs with search, persistence, and presets
 */

import React, { useState, useCallback, useMemo, useEffect } from 'react';
import { ActivityLogEntry, ActivityFilter, FilterPreset } from '../../types';
import { useVault } from '../../contexts';

export interface LogFilterSystemProps {
  entries: ActivityLogEntry[];
  onFilteredEntriesChange: (filteredEntries: ActivityLogEntry[]) => void;
  onFiltersChange: (filters: ActivityFilter) => void;
  initialFilters?: Partial<ActivityFilter>;
  showPresets?: boolean;
  showAdvancedFilters?: boolean;
  className?: string;
}

interface FilterState {
  filters: ActivityFilter;
  searchHistory: string[];
  savedPresets: FilterPreset[];
  activePreset: string | null;
}

/**
 * Log filter system component
 */
export const LogFilterSystem: React.FC<LogFilterSystemProps> = ({
  entries,
  onFilteredEntriesChange,
  onFiltersChange,
  initialFilters = {},
  showPresets = true,
  showAdvancedFilters = true,
  className = '',
}) => {
  const { state: vaultState } = useVault();

  // ==================== STATE MANAGEMENT ====================

  const [filterState, setFilterState] = useState<FilterState>({
    filters: {
      vaultId: '',
      level: 'all',
      timeRange: 'all',
      search: '',
      dateFrom: null,
      dateTo: null,
      ...initialFilters,
    },
    searchHistory: [],
    savedPresets: [],
    activePreset: null,
  });

  const [isAdvancedOpen, setIsAdvancedOpen] = useState(false);
  const [searchSuggestions, setSearchSuggestions] = useState<string[]>([]);
  const [showSuggestions, setShowSuggestions] = useState(false);

  // ==================== FILTERING LOGIC ====================

  const filteredEntries = useMemo(() => {
    let filtered = [...entries];
    const { filters } = filterState;

    // Filter by vault
    if (filters.vaultId && filters.vaultId !== 'all') {
      filtered = filtered.filter(entry => entry.vaultId === filters.vaultId);
    }

    // Filter by level
    if (filters.level && filters.level !== 'all') {
      filtered = filtered.filter(entry => entry.level === filters.level);
    }

    // Filter by search term with advanced search capabilities
    if (filters.search) {
      const searchTerms = parseSearchQuery(filters.search);
      filtered = filtered.filter(entry => matchesSearchTerms(entry, searchTerms));
    }

    // Filter by time range
    if (filters.timeRange && filters.timeRange !== 'all') {
      const timeFilter = getTimeRangeFilter(filters.timeRange);
      if (timeFilter) {
        filtered = filtered.filter(entry => entry.timestamp >= timeFilter);
      }
    }

    // Filter by custom date range
    if (filters.dateFrom) {
      filtered = filtered.filter(entry => entry.timestamp >= filters.dateFrom!);
    }
    if (filters.dateTo) {
      filtered = filtered.filter(entry => entry.timestamp <= filters.dateTo!);
    }

    // Sort by timestamp (newest first)
    filtered.sort((a, b) => b.timestamp.getTime() - a.timestamp.getTime());

    return filtered;
  }, [entries, filterState.filters]);

  // ==================== SEARCH PARSING ====================

  const parseSearchQuery = useCallback((query: string) => {
    const terms: Array<{
      text: string;
      field?: string;
      operator?: 'contains' | 'equals' | 'startsWith' | 'endsWith';
      exclude?: boolean;
    }> = [];

    // Split by spaces but preserve quoted strings
    const matches = query.match(/(?:[^\s"]+|"[^"]*")+/g) || [];

    matches.forEach(match => {
      let term = match.trim();
      let exclude = false;
      let field: string | undefined;
      let operator: 'contains' | 'equals' | 'startsWith' | 'endsWith' = 'contains';

      // Handle exclusion (-)
      if (term.startsWith('-')) {
        exclude = true;
        term = term.substring(1);
      }

      // Handle field-specific search (field:value)
      const fieldMatch = term.match(/^(\w+):(.+)$/);
      if (fieldMatch) {
        field = fieldMatch[1];
        term = fieldMatch[2];
      }

      // Handle quoted strings
      if (term.startsWith('"') && term.endsWith('"')) {
        term = term.slice(1, -1);
        operator = 'equals';
      }

      // Handle wildcards
      if (term.startsWith('*') && term.endsWith('*')) {
        term = term.slice(1, -1);
        operator = 'contains';
      } else if (term.startsWith('*')) {
        term = term.substring(1);
        operator = 'endsWith';
      } else if (term.endsWith('*')) {
        term = term.slice(0, -1);
        operator = 'startsWith';
      }

      if (term) {
        terms.push({ text: term, field, operator, exclude });
      }
    });

    return terms;
  }, []);

  const matchesSearchTerms = useCallback((entry: ActivityLogEntry, terms: ReturnType<typeof parseSearchQuery>) => {
    return terms.every(term => {
      let fieldValue: string;

      // Determine which field to search
      switch (term.field) {
        case 'vault':
          fieldValue = entry.vaultName || '';
          break;
        case 'level':
          fieldValue = entry.level;
          break;
        case 'message':
          fieldValue = entry.message;
          break;
        case 'details':
          fieldValue = entry.details || '';
          break;
        default:
          // Search all text fields
          fieldValue = [
            entry.message,
            entry.vaultName || '',
            entry.level,
            entry.details || ''
          ].join(' ');
      }

      fieldValue = fieldValue.toLowerCase();
      const searchText = term.text.toLowerCase();

      let matches = false;
      switch (term.operator) {
        case 'equals':
          matches = fieldValue === searchText;
          break;
        case 'startsWith':
          matches = fieldValue.startsWith(searchText);
          break;
        case 'endsWith':
          matches = fieldValue.endsWith(searchText);
          break;
        case 'contains':
        default:
          matches = fieldValue.includes(searchText);
      }

      return term.exclude ? !matches : matches;
    });
  }, []);

  // ==================== TIME RANGE HELPERS ====================

  const getTimeRangeFilter = useCallback((timeRange: string): Date | null => {
    const now = new Date();
    const filter = new Date();

    switch (timeRange) {
      case 'hour':
        filter.setHours(now.getHours() - 1);
        return filter;
      case 'day':
        filter.setDate(now.getDate() - 1);
        return filter;
      case 'week':
        filter.setDate(now.getDate() - 7);
        return filter;
      case 'month':
        filter.setMonth(now.getMonth() - 1);
        return filter;
      default:
        return null;
    }
  }, []);

  // ==================== SEARCH SUGGESTIONS ====================

  const generateSearchSuggestions = useCallback((query: string) => {
    if (!query || query.length < 2) {
      setSearchSuggestions([]);
      return;
    }

    const suggestions = new Set<string>();
    const queryLower = query.toLowerCase();

    // Add field-specific suggestions
    if (!query.includes(':')) {
      suggestions.add(`vault:${query}`);
      suggestions.add(`level:${query}`);
      suggestions.add(`message:${query}`);
    }

    // Add vault name suggestions
    vaultState.vaults.forEach(vault => {
      if (vault.name.toLowerCase().includes(queryLower)) {
        suggestions.add(`vault:"${vault.name}"`);
      }
    });

    // Add level suggestions
    const levels = ['info', 'warning', 'error', 'debug'];
    levels.forEach(level => {
      if (level.includes(queryLower)) {
        suggestions.add(`level:${level}`);
      }
    });

    // Add common message patterns
    const commonPatterns = [
      'mounted', 'unmounted', 'failed', 'success', 'error', 'timeout',
      'password', 'authentication', 'backup', 'sync'
    ];
    commonPatterns.forEach(pattern => {
      if (pattern.includes(queryLower)) {
        suggestions.add(`message:*${pattern}*`);
      }
    });

    setSearchSuggestions(Array.from(suggestions).slice(0, 8));
  }, [vaultState.vaults]);

  // ==================== FILTER PRESETS ====================

  const defaultPresets: FilterPreset[] = [
    {
      id: 'errors-only',
      name: 'Errors Only',
      description: 'Show only error entries',
      filters: { level: 'error', timeRange: 'all', search: '', vaultId: '', dateFrom: null, dateTo: null },
    },
    {
      id: 'recent-activity',
      name: 'Recent Activity',
      description: 'Last 24 hours of activity',
      filters: { level: 'all', timeRange: 'day', search: '', vaultId: '', dateFrom: null, dateTo: null },
    },
    {
      id: 'mount-operations',
      name: 'Mount Operations',
      description: 'Mount and unmount operations',
      filters: { level: 'all', timeRange: 'all', search: 'mount', vaultId: '', dateFrom: null, dateTo: null },
    },
    {
      id: 'authentication-issues',
      name: 'Authentication Issues',
      description: 'Password and authentication related entries',
      filters: { level: 'all', timeRange: 'all', search: 'password OR authentication', vaultId: '', dateFrom: null, dateTo: null },
    },
  ];

  // ==================== HANDLERS ====================

  const updateFilters = useCallback((updates: Partial<ActivityFilter>) => {
    setFilterState(prev => {
      const newFilters = { ...prev.filters, ...updates };
      return {
        ...prev,
        filters: newFilters,
        activePreset: null, // Clear active preset when manually changing filters
      };
    });
  }, []);

  const handleSearchChange = useCallback((search: string) => {
    updateFilters({ search });
    generateSearchSuggestions(search);
    setShowSuggestions(search.length > 0);

    // Add to search history
    if (search && !filterState.searchHistory.includes(search)) {
      setFilterState(prev => ({
        ...prev,
        searchHistory: [search, ...prev.searchHistory].slice(0, 10),
      }));
    }
  }, [updateFilters, generateSearchSuggestions, filterState.searchHistory]);

  const applyPreset = useCallback((preset: FilterPreset) => {
    setFilterState(prev => ({
      ...prev,
      filters: { ...preset.filters },
      activePreset: preset.id,
    }));
    setShowSuggestions(false);
  }, []);

  const saveCurrentAsPreset = useCallback((name: string, description: string) => {
    const newPreset: FilterPreset = {
      id: `custom-${Date.now()}`,
      name,
      description,
      filters: { ...filterState.filters },
      isCustom: true,
    };

    setFilterState(prev => ({
      ...prev,
      savedPresets: [...prev.savedPresets, newPreset],
    }));
  }, [filterState.filters]);

  const deletePreset = useCallback((presetId: string) => {
    setFilterState(prev => ({
      ...prev,
      savedPresets: prev.savedPresets.filter(p => p.id !== presetId),
      activePreset: prev.activePreset === presetId ? null : prev.activePreset,
    }));
  }, []);

  const clearAllFilters = useCallback(() => {
    const clearedFilters: ActivityFilter = {
      vaultId: '',
      level: 'all',
      timeRange: 'all',
      search: '',
      dateFrom: null,
      dateTo: null,
    };

    setFilterState(prev => ({
      ...prev,
      filters: clearedFilters,
      activePreset: null,
    }));
    setShowSuggestions(false);
  }, []);

  // ==================== EFFECTS ====================

  useEffect(() => {
    onFilteredEntriesChange(filteredEntries);
  }, [filteredEntries, onFilteredEntriesChange]);

  useEffect(() => {
    onFiltersChange(filterState.filters);
  }, [filterState.filters, onFiltersChange]);

  // Load saved presets from localStorage
  useEffect(() => {
    const savedPresets = localStorage.getItem('activityLogPresets');
    if (savedPresets) {
      try {
        const presets = JSON.parse(savedPresets);
        setFilterState(prev => ({ ...prev, savedPresets: presets }));
      } catch (error) {
        console.error('Failed to load saved presets:', error);
      }
    }
  }, []);

  // Save presets to localStorage
  useEffect(() => {
    if (filterState.savedPresets.length > 0) {
      localStorage.setItem('activityLogPresets', JSON.stringify(filterState.savedPresets));
    }
  }, [filterState.savedPresets]);

  // ==================== RENDER HELPERS ====================

  const hasActiveFilters = () => {
    const { filters } = filterState;
    return filters.vaultId !== '' ||
           filters.level !== 'all' ||
           filters.timeRange !== 'all' ||
           filters.search !== '' ||
           filters.dateFrom !== null ||
           filters.dateTo !== null;
  };

  const getActiveFilterCount = () => {
    let count = 0;
    const { filters } = filterState;
    if (filters.vaultId && filters.vaultId !== 'all') count++;
    if (filters.level && filters.level !== 'all') count++;
    if (filters.timeRange && filters.timeRange !== 'all') count++;
    if (filters.search) count++;
    if (filters.dateFrom) count++;
    if (filters.dateTo) count++;
    return count;
  };

  const allPresets = [...defaultPresets, ...filterState.savedPresets];

  // ==================== MAIN RENDER ====================

  return (
    <div className={`log-filter-system ${className}`}>
      {/* Quick Filters */}
      <div className="quick-filters">
        {/* Search */}
        <div className="search-container">
          <div className="search-input-wrapper">
            <input
              type="text"
              value={filterState.filters.search}
              onChange={(e) => handleSearchChange(e.target.value)}
              onFocus={() => setShowSuggestions(filterState.filters.search.length > 0)}
              onBlur={() => setTimeout(() => setShowSuggestions(false), 200)}
              placeholder="Search logs... (try: vault:name, level:error, message:*mount*)"
              className="search-input"
            />
            <span className="search-icon">🔍</span>
          </div>

          {/* Search Suggestions */}
          {showSuggestions && searchSuggestions.length > 0 && (
            <div className="search-suggestions">
              {searchSuggestions.map((suggestion, index) => (
                <button
                  key={index}
                  onClick={() => {
                    handleSearchChange(suggestion);
                    setShowSuggestions(false);
                  }}
                  className="suggestion-item"
                >
                  {suggestion}
                </button>
              ))}
            </div>
          )}
        </div>

        {/* Quick Filter Buttons */}
        <div className="quick-filter-buttons">
          <select
            value={filterState.filters.level}
            onChange={(e) => updateFilters({ level: e.target.value as any })}
            className="filter-select"
          >
            <option value="all">All Levels</option>
            <option value="info">Info</option>
            <option value="warning">Warning</option>
            <option value="error">Error</option>
            <option value="debug">Debug</option>
          </select>

          <select
            value={filterState.filters.timeRange}
            onChange={(e) => updateFilters({ timeRange: e.target.value as any })}
            className="filter-select"
          >
            <option value="all">All Time</option>
            <option value="hour">Last Hour</option>
            <option value="day">Last Day</option>
            <option value="week">Last Week</option>
            <option value="month">Last Month</option>
          </select>

          <select
            value={filterState.filters.vaultId}
            onChange={(e) => updateFilters({ vaultId: e.target.value })}
            className="filter-select"
          >
            <option value="">All Vaults</option>
            {vaultState.vaults.map(vault => (
              <option key={vault.id} value={vault.id}>
                {vault.name}
              </option>
            ))}
          </select>
        </div>

        {/* Filter Actions */}
        <div className="filter-actions">
          {hasActiveFilters() && (
            <button onClick={clearAllFilters} className="clear-filters-button">
              <span className="button-icon">🗑️</span>
              Clear ({getActiveFilterCount()})
            </button>
          )}

          {showAdvancedFilters && (
            <button
              onClick={() => setIsAdvancedOpen(!isAdvancedOpen)}
              className="advanced-toggle-button"
            >
              <span className="button-icon">⚙️</span>
              Advanced
            </button>
          )}
        </div>
      </div>

      {/* Filter Presets */}
      {showPresets && (
        <div className="filter-presets">
          <div className="presets-header">
            <span className="presets-title">Quick Filters:</span>
          </div>
          <div className="presets-list">
            {allPresets.map(preset => (
              <button
                key={preset.id}
                onClick={() => applyPreset(preset)}
                className={`preset-button ${filterState.activePreset === preset.id ? 'active' : ''}`}
                title={preset.description}
              >
                {preset.name}
                {preset.isCustom && (
                  <button
                    onClick={(e) => {
                      e.stopPropagation();
                      deletePreset(preset.id);
                    }}
                    className="delete-preset"
                    title="Delete preset"
                  >
                    ✕
                  </button>
                )}
              </button>
            ))}
          </div>
        </div>
      )}

      {/* Advanced Filters */}
      {isAdvancedOpen && showAdvancedFilters && (
        <div className="advanced-filters">
          <div className="advanced-header">
            <h4 className="advanced-title">Advanced Filters</h4>
          </div>

          <div className="advanced-content">
            {/* Custom Date Range */}
            <div className="date-range-filters">
              <div className="date-input-group">
                <label className="date-label">From:</label>
                <input
                  type="datetime-local"
                  value={filterState.filters.dateFrom ? filterState.filters.dateFrom.toISOString().slice(0, 16) : ''}
                  onChange={(e) => updateFilters({ dateFrom: e.target.value ? new Date(e.target.value) : null })}
                  className="date-input"
                />
              </div>
              <div className="date-input-group">
                <label className="date-label">To:</label>
                <input
                  type="datetime-local"
                  value={filterState.filters.dateTo ? filterState.filters.dateTo.toISOString().slice(0, 16) : ''}
                  onChange={(e) => updateFilters({ dateTo: e.target.value ? new Date(e.target.value) : null })}
                  className="date-input"
                />
              </div>
            </div>

            {/* Search History */}
            {filterState.searchHistory.length > 0 && (
              <div className="search-history">
                <label className="history-label">Recent Searches:</label>
                <div className="history-items">
                  {filterState.searchHistory.map((search, index) => (
                    <button
                      key={index}
                      onClick={() => handleSearchChange(search)}
                      className="history-item"
                    >
                      {search}
                    </button>
                  ))}
                </div>
              </div>
            )}

            {/* Save Current Filters */}
            <div className="save-preset">
              <button
                onClick={() => {
                  const name = prompt('Enter preset name:');
                  if (name) {
                    const description = prompt('Enter preset description (optional):') || '';
                    saveCurrentAsPreset(name, description);
                  }
                }}
                className="save-preset-button"
                disabled={!hasActiveFilters()}
              >
                <span className="button-icon">💾</span>
                Save Current Filters as Preset
              </button>
            </div>
          </div>
        </div>
      )}

      {/* Filter Summary */}
      {hasActiveFilters() && (
        <div className="filter-summary">
          <span className="summary-text">
            Showing {filteredEntries.length} of {entries.length} entries
          </span>
          {filterState.activePreset && (
            <span className="active-preset">
              Using preset: {allPresets.find(p => p.id === filterState.activePreset)?.name}
            </span>
          )}
        </div>
      )}
    </div>
  );
};