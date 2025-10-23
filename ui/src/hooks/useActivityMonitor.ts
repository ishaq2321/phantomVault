/**
 * Activity Monitor Hook
 * 
 * React hook for managing activity log monitoring and real-time updates
 */

import { useState, useCallback, useEffect, useRef } from 'react';
import { ActivityLogEntry, ActivityFilter, ActivityStats } from '../types';

export interface UseActivityMonitorOptions {
  maxEntries?: number;
  enableRealTime?: boolean;
  autoRefresh?: boolean;
  refreshInterval?: number;
}

export interface UseActivityMonitorReturn {
  // Data
  entries: ActivityLogEntry[];
  filteredEntries: ActivityLogEntry[];
  stats: ActivityStats;
  
  // State
  isLoading: boolean;
  isConnected: boolean;
  error: string | null;
  
  // Filters
  filters: ActivityFilter;
  setFilters: (filters: Partial<ActivityFilter>) => void;
  clearFilters: () => void;
  
  // Actions
  loadEntries: () => Promise<void>;
  clearLog: () => Promise<void>;
  exportLog: (format?: 'json' | 'csv') => void;
  
  // Real-time
  subscribeToUpdates: () => () => void;
  unsubscribeFromUpdates: () => void;
}

/**
 * Hook for activity monitoring functionality
 */
export function useActivityMonitor(options: UseActivityMonitorOptions = {}): UseActivityMonitorReturn {
  const {
    maxEntries = 1000,
    enableRealTime = true,
    autoRefresh = false,
    refreshInterval = 30000,
  } = options;

  // State
  const [entries, setEntries] = useState<ActivityLogEntry[]>([]);
  const [filteredEntries, setFilteredEntries] = useState<ActivityLogEntry[]>([]);
  const [isLoading, setIsLoading] = useState(false);
  const [isConnected, setIsConnected] = useState(false);
  const [error, setError] = useState<string | null>(null);
  
  // Filters
  const [filters, setFiltersState] = useState<ActivityFilter>({
    vaultId: 'all',
    level: 'all',
    timeRange: 'all',
    search: '',
    dateFrom: null,
    dateTo: null,
  });

  // Refs
  const unsubscribeRef = useRef<(() => void) | null>(null);
  const refreshIntervalRef = useRef<NodeJS.Timeout | null>(null);

  // Calculate stats
  const stats: ActivityStats = {
    total: entries.length,
    byLevel: {
      info: entries.filter(e => e.level === 'info').length,
      warning: entries.filter(e => e.level === 'warning').length,
      error: entries.filter(e => e.level === 'error').length,
      debug: entries.filter(e => e.level === 'debug').length,
    },
    byVault: entries.reduce((acc, entry) => {
      const vaultId = entry.vaultId || 'system';
      acc[vaultId] = (acc[vaultId] || 0) + 1;
      return acc;
    }, {} as Record<string, number>),
    timeRange: {
      start: entries.length > 0 ? entries[entries.length - 1].timestamp : null,
      end: entries.length > 0 ? entries[0].timestamp : null,
    },
  };

  // Apply filters to entries
  const applyFilters = useCallback((allEntries: ActivityLogEntry[], currentFilters: ActivityFilter): ActivityLogEntry[] => {
    return allEntries.filter(entry => {
      // Vault filter
      if (currentFilters.vaultId !== 'all') {
        const entryVaultId = entry.vaultId || 'system';
        if (entryVaultId !== currentFilters.vaultId) {
          return false;
        }
      }

      // Level filter
      if (currentFilters.level !== 'all' && entry.level !== currentFilters.level) {
        return false;
      }

      // Search filter
      if (currentFilters.search) {
        const searchTerm = currentFilters.search.toLowerCase();
        const searchableText = [
          entry.message,
          entry.vaultName || '',
          entry.details || '',
        ].join(' ').toLowerCase();
        
        if (!searchableText.includes(searchTerm)) {
          return false;
        }
      }

      // Time range filter
      const entryTime = entry.timestamp.getTime();
      const now = new Date().getTime();
      
      switch (currentFilters.timeRange) {
        case 'hour':
          if (now - entryTime > 60 * 60 * 1000) return false;
          break;
        case 'day':
          if (now - entryTime > 24 * 60 * 60 * 1000) return false;
          break;
        case 'week':
          if (now - entryTime > 7 * 24 * 60 * 60 * 1000) return false;
          break;
        case 'month':
          if (now - entryTime > 30 * 24 * 60 * 60 * 1000) return false;
          break;
        case 'custom':
          if (currentFilters.dateFrom && entryTime < currentFilters.dateFrom.getTime()) return false;
          if (currentFilters.dateTo && entryTime > currentFilters.dateTo.getTime()) return false;
          break;
      }

      return true;
    });
  }, []);

  // Update filtered entries when entries or filters change
  useEffect(() => {
    const filtered = applyFilters(entries, filters);
    setFilteredEntries(filtered);
  }, [entries, filters, applyFilters]);

  // Load entries from backend
  const loadEntries = useCallback(async () => {
    setIsLoading(true);
    setError(null);
    
    try {
      const response = await window.electronAPI?.getActivityLog?.(filters);
      
      if (response?.success && response.data) {
        const sortedEntries = response.data
          .sort((a, b) => b.timestamp.getTime() - a.timestamp.getTime())
          .slice(0, maxEntries);
        
        setEntries(sortedEntries);
      } else {
        throw new Error(response?.error || 'Failed to load activity log');
      }
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : 'Unknown error';
      setError(errorMessage);
      console.error('Failed to load activity log:', err);
    } finally {
      setIsLoading(false);
    }
  }, [filters, maxEntries]);

  // Clear log
  const clearLog = useCallback(async () => {
    try {
      await window.electronAPI?.clearActivityLog?.();
      setEntries([]);
      setError(null);
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : 'Failed to clear log';
      setError(errorMessage);
      console.error('Failed to clear activity log:', err);
    }
  }, []);

  // Export log
  const exportLog = useCallback((format: 'json' | 'csv' = 'json') => {
    const dataToExport = filteredEntries.map(entry => ({
      timestamp: entry.timestamp.toISOString(),
      level: entry.level,
      message: entry.message,
      vaultName: entry.vaultName || 'System',
      vaultId: entry.vaultId || 'system',
      details: entry.details || '',
    }));

    let content: string;
    let mimeType: string;
    let extension: string;

    if (format === 'csv') {
      const headers = ['Timestamp', 'Level', 'Message', 'Vault Name', 'Vault ID', 'Details'];
      const csvRows = [
        headers.join(','),
        ...dataToExport.map(row => [
          `"${row.timestamp}"`,
          `"${row.level}"`,
          `"${row.message.replace(/"/g, '""')}"`,
          `"${row.vaultName}"`,
          `"${row.vaultId}"`,
          `"${row.details.replace(/"/g, '""')}"`,
        ].join(','))
      ];
      content = csvRows.join('\n');
      mimeType = 'text/csv';
      extension = 'csv';
    } else {
      content = JSON.stringify(dataToExport, null, 2);
      mimeType = 'application/json';
      extension = 'json';
    }

    const blob = new Blob([content], { type: mimeType });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `activity-log-${new Date().toISOString().split('T')[0]}.${extension}`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  }, [filteredEntries]);

  // Subscribe to real-time updates
  const subscribeToUpdates = useCallback(() => {
    if (!enableRealTime || unsubscribeRef.current) {
      return () => {};
    }

    const handleNewEntry = (entry: ActivityLogEntry) => {
      setEntries(prev => {
        const newEntries = [entry, ...prev].slice(0, maxEntries);
        return newEntries;
      });
      setIsConnected(true);
    };

    const handleConnectionError = () => {
      setIsConnected(false);
      setError('Lost connection to activity monitor');
    };

    try {
      const unsubscribe = window.electronAPI?.subscribeToActivity?.(handleNewEntry);
      
      if (unsubscribe) {
        unsubscribeRef.current = unsubscribe;
        setIsConnected(true);
        setError(null);
        
        return () => {
          if (unsubscribeRef.current) {
            unsubscribeRef.current();
            unsubscribeRef.current = null;
          }
          setIsConnected(false);
        };
      } else {
        handleConnectionError();
        return () => {};
      }
    } catch (err) {
      handleConnectionError();
      return () => {};
    }
  }, [enableRealTime, maxEntries]);

  // Unsubscribe from updates
  const unsubscribeFromUpdates = useCallback(() => {
    if (unsubscribeRef.current) {
      unsubscribeRef.current();
      unsubscribeRef.current = null;
    }
    setIsConnected(false);
  }, []);

  // Set filters with validation
  const setFilters = useCallback((newFilters: Partial<ActivityFilter>) => {
    setFiltersState(prev => {
      const updated = { ...prev, ...newFilters };
      
      // Validate date range
      if (updated.dateFrom && updated.dateTo && updated.dateFrom > updated.dateTo) {
        updated.dateTo = updated.dateFrom;
      }
      
      return updated;
    });
  }, []);

  // Clear all filters
  const clearFilters = useCallback(() => {
    setFiltersState({
      vaultId: 'all',
      level: 'all',
      timeRange: 'all',
      search: '',
      dateFrom: null,
      dateTo: null,
    });
  }, []);

  // Auto-refresh effect
  useEffect(() => {
    if (autoRefresh && refreshInterval > 0) {
      refreshIntervalRef.current = setInterval(() => {
        if (!isConnected) {
          loadEntries();
        }
      }, refreshInterval);

      return () => {
        if (refreshIntervalRef.current) {
          clearInterval(refreshIntervalRef.current);
        }
      };
    }
  }, [autoRefresh, refreshInterval, isConnected, loadEntries]);

  // Initial load
  useEffect(() => {
    loadEntries();
  }, []);

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      unsubscribeFromUpdates();
      if (refreshIntervalRef.current) {
        clearInterval(refreshIntervalRef.current);
      }
    };
  }, [unsubscribeFromUpdates]);

  return {
    // Data
    entries,
    filteredEntries,
    stats,
    
    // State
    isLoading,
    isConnected,
    error,
    
    // Filters
    filters,
    setFilters,
    clearFilters,
    
    // Actions
    loadEntries,
    clearLog,
    exportLog,
    
    // Real-time
    subscribeToUpdates,
    unsubscribeFromUpdates,
  };
}