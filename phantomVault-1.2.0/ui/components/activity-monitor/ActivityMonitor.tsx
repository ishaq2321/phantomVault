/**
 * Activity Monitor Component
 * 
 * Component for displaying and monitoring vault activity logs with real-time updates
 */

import React, { useState, useCallback, useEffect, useMemo, useRef } from 'react';
import { ActivityLogEntry, ActivityFilter, ActivityStats } from '../../types';
import { useApp } from '../../contexts';
import { useActivityMonitor } from '../../hooks';
import { VirtualScrollList } from '../common/VirtualScrollList';
import { ActivityLogItem } from './ActivityLogItem';
import { ActivityFilterPanel } from './ActivityFilterPanel';
import { ActivityStatsPanel } from './ActivityStatsPanel';

export interface ActivityMonitorProps {
  className?: string;
  maxEntries?: number;
  autoScroll?: boolean;
  showFilters?: boolean;
  showStats?: boolean;
  height?: string;
  refreshInterval?: number;
}

interface ActivityState {
  entries: ActivityLogEntry[];
  filteredEntries: ActivityLogEntry[];
  isLoading: boolean;
  error: string | null;
  isAutoScrollEnabled: boolean;
  hasNewEntries: boolean;
}

/**
 * Activity monitor component
 */
export const ActivityMonitor: React.FC<ActivityMonitorProps> = ({
  className = '',
  maxEntries = 1000,
  autoScroll = true,
  showFilters = true,
  showStats = true,
  height = '600px',
  refreshInterval = 5000,
}) => {
  const { actions: appActions } = useApp();
  const activityHook = useActivityMonitor();
  const scrollContainerRef = useRef<HTMLDivElement>(null);
  const [lastScrollTop, setLastScrollTop] = useState(0);

  // ==================== STATE MANAGEMENT ====================

  const [activityState, setActivityState] = useState<ActivityState>({
    entries: [],
    filteredEntries: [],
    isLoading: true,
    error: null,
    isAutoScrollEnabled: autoScroll,
    hasNewEntries: false,
  });

  const [filters, setFilters] = useState<ActivityFilter>({
    vaultId: '',
    level: 'all',
    timeRange: 'all',
    search: '',
    dateFrom: null,
    dateTo: null,
  });

  const [viewSettings, setViewSettings] = useState({
    showTimestamps: true,
    showVaultNames: true,
    showLevels: true,
    compactMode: false,
    groupByVault: false,
  });

  // ==================== ACTIVITY LOG MANAGEMENT ====================

  const loadActivityLog = useCallback(async () => {
    try {
      setActivityState(prev => ({ ...prev, isLoading: true, error: null }));
      
      const entries = await activityHook.getActivityLog({
        limit: maxEntries,
        ...filters,
      });

      setActivityState(prev => ({
        ...prev,
        entries,
        isLoading: false,
      }));
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to load activity log';
      setActivityState(prev => ({
        ...prev,
        isLoading: false,
        error: errorMessage,
      }));

      appActions.addNotification({
        type: 'error',
        title: 'Activity Log Error',
        message: errorMessage,
        duration: 5000,
      });
    }
  }, [activityHook, maxEntries, filters, appActions]);

  // ==================== FILTERING ====================

  const filteredEntries = useMemo(() => {
    let filtered = [...activityState.entries];

    // Filter by vault
    if (filters.vaultId && filters.vaultId !== 'all') {
      filtered = filtered.filter(entry => entry.vaultId === filters.vaultId);
    }

    // Filter by level
    if (filters.level && filters.level !== 'all') {
      filtered = filtered.filter(entry => entry.level === filters.level);
    }

    // Filter by search term
    if (filters.search) {
      const searchLower = filters.search.toLowerCase();
      filtered = filtered.filter(entry =>
        entry.message.toLowerCase().includes(searchLower) ||
        entry.vaultName?.toLowerCase().includes(searchLower) ||
        entry.details?.toLowerCase().includes(searchLower)
      );
    }

    // Filter by time range
    if (filters.timeRange && filters.timeRange !== 'all') {
      const now = new Date();
      const cutoff = new Date();

      switch (filters.timeRange) {
        case 'hour':
          cutoff.setHours(now.getHours() - 1);
          break;
        case 'day':
          cutoff.setDate(now.getDate() - 1);
          break;
        case 'week':
          cutoff.setDate(now.getDate() - 7);
          break;
        case 'month':
          cutoff.setMonth(now.getMonth() - 1);
          break;
      }

      filtered = filtered.filter(entry => entry.timestamp > cutoff);
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
  }, [activityState.entries, filters]);

  // ==================== STATISTICS ====================

  const activityStats = useMemo((): ActivityStats => {
    const stats: ActivityStats = {
      total: filteredEntries.length,
      byLevel: {
        info: 0,
        warning: 0,
        error: 0,
        debug: 0,
      },
      byVault: {},
      timeRange: {
        start: null,
        end: null,
      },
    };

    if (filteredEntries.length > 0) {
      stats.timeRange.start = filteredEntries[filteredEntries.length - 1].timestamp;
      stats.timeRange.end = filteredEntries[0].timestamp;

      filteredEntries.forEach(entry => {
        // Count by level
        stats.byLevel[entry.level]++;

        // Count by vault
        if (entry.vaultName) {
          if (!stats.byVault[entry.vaultName]) {
            stats.byVault[entry.vaultName] = 0;
          }
          stats.byVault[entry.vaultName]++;
        }
      });
    }

    return stats;
  }, [filteredEntries]);

  // ==================== REAL-TIME UPDATES ====================

  useEffect(() => {
    // Subscribe to real-time activity updates
    const unsubscribe = activityHook.subscribeToActivity((newEntry: ActivityLogEntry) => {
      setActivityState(prev => {
        const updatedEntries = [newEntry, ...prev.entries].slice(0, maxEntries);
        return {
          ...prev,
          entries: updatedEntries,
          hasNewEntries: !prev.isAutoScrollEnabled,
        };
      });

      // Auto-scroll to new entries if enabled
      if (activityState.isAutoScrollEnabled && scrollContainerRef.current) {
        scrollContainerRef.current.scrollTop = 0;
      }
    });

    return unsubscribe;
  }, [activityHook, maxEntries, activityState.isAutoScrollEnabled]);

  // ==================== AUTO-REFRESH ====================

  useEffect(() => {
    if (refreshInterval > 0) {
      const interval = setInterval(loadActivityLog, refreshInterval);
      return () => clearInterval(interval);
    }
  }, [loadActivityLog, refreshInterval]);

  // ==================== SCROLL MANAGEMENT ====================

  const handleScroll = useCallback((event: React.UIEvent<HTMLDivElement>) => {
    const { scrollTop, scrollHeight, clientHeight } = event.currentTarget;
    const isAtTop = scrollTop === 0;
    const isNearTop = scrollTop < 100;

    // Update auto-scroll state based on user interaction
    if (scrollTop !== lastScrollTop) {
      setActivityState(prev => ({
        ...prev,
        isAutoScrollEnabled: isNearTop,
        hasNewEntries: false,
      }));
      setLastScrollTop(scrollTop);
    }
  }, [lastScrollTop]);

  const scrollToTop = useCallback(() => {
    if (scrollContainerRef.current) {
      scrollContainerRef.current.scrollTop = 0;
      setActivityState(prev => ({
        ...prev,
        isAutoScrollEnabled: true,
        hasNewEntries: false,
      }));
    }
  }, []);

  const scrollToBottom = useCallback(() => {
    if (scrollContainerRef.current) {
      scrollContainerRef.current.scrollTop = scrollContainerRef.current.scrollHeight;
      setActivityState(prev => ({
        ...prev,
        isAutoScrollEnabled: false,
      }));
    }
  }, []);

  // ==================== HANDLERS ====================

  const handleFilterChange = useCallback((newFilters: Partial<ActivityFilter>) => {
    setFilters(prev => ({ ...prev, ...newFilters }));
  }, []);

  const handleViewSettingsChange = useCallback((newSettings: Partial<typeof viewSettings>) => {
    setViewSettings(prev => ({ ...prev, ...newSettings }));
  }, []);

  const handleClearLog = useCallback(async () => {
    const confirmed = window.confirm('Are you sure you want to clear the activity log?');
    if (confirmed) {
      try {
        await activityHook.clearActivityLog();
        setActivityState(prev => ({
          ...prev,
          entries: [],
        }));

        appActions.addNotification({
          type: 'success',
          title: 'Activity Log Cleared',
          message: 'All activity log entries have been cleared.',
          duration: 3000,
        });
      } catch (error) {
        appActions.addNotification({
          type: 'error',
          title: 'Clear Failed',
          message: 'Failed to clear activity log.',
          duration: 5000,
        });
      }
    }
  }, [activityHook, appActions]);

  const handleExportLog = useCallback(() => {
    const exportData = {
      timestamp: new Date().toISOString(),
      filters,
      entries: filteredEntries.map(entry => ({
        timestamp: entry.timestamp.toISOString(),
        level: entry.level,
        message: entry.message,
        vaultName: entry.vaultName,
        vaultId: entry.vaultId,
        details: entry.details,
      })),
    };

    const blob = new Blob([JSON.stringify(exportData, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `activity-log-${Date.now()}.json`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  }, [filteredEntries, filters]);

  // ==================== EFFECTS ====================

  useEffect(() => {
    loadActivityLog();
  }, [loadActivityLog]);

  useEffect(() => {
    setActivityState(prev => ({
      ...prev,
      filteredEntries,
    }));
  }, [filteredEntries]);

  // ==================== RENDER HELPERS ====================

  const renderLogEntry = useCallback((entry: ActivityLogEntry, index: number) => (
    <ActivityLogItem
      key={`${entry.id}-${index}`}
      entry={entry}
      showTimestamp={viewSettings.showTimestamps}
      showVaultName={viewSettings.showVaultNames}
      showLevel={viewSettings.showLevels}
      compactMode={viewSettings.compactMode}
    />
  ), [viewSettings]);

  const getEmptyMessage = () => {
    if (activityState.isLoading) {
      return 'Loading activity log...';
    }
    if (activityState.error) {
      return `Error: ${activityState.error}`;
    }
    if (activityState.entries.length === 0) {
      return 'No activity recorded yet.';
    }
    return 'No entries match the current filters.';
  };

  // ==================== MAIN RENDER ====================

  return (
    <div className={`activity-monitor ${className}`}>
      {/* Header */}
      <div className="activity-monitor-header">
        <div className="header-left">
          <h2 className="monitor-title">Activity Monitor</h2>
          <span className="entry-count">
            {filteredEntries.length} of {activityState.entries.length} entries
          </span>
        </div>

        <div className="header-right">
          <div className="header-controls">
            {activityState.hasNewEntries && (
              <button
                onClick={scrollToTop}
                className="new-entries-button"
                title="Scroll to new entries"
              >
                <span className="button-icon">🔔</span>
                New entries
              </button>
            )}

            <button
              onClick={loadActivityLog}
              className="refresh-button"
              disabled={activityState.isLoading}
              title="Refresh log"
            >
              <span className="button-icon">🔄</span>
              {activityState.isLoading ? 'Loading...' : 'Refresh'}
            </button>

            <button
              onClick={handleExportLog}
              className="export-button"
              disabled={filteredEntries.length === 0}
              title="Export log"
            >
              <span className="button-icon">📊</span>
              Export
            </button>

            <button
              onClick={handleClearLog}
              className="clear-button"
              disabled={activityState.entries.length === 0}
              title="Clear log"
            >
              <span className="button-icon">🗑️</span>
              Clear
            </button>
          </div>
        </div>
      </div>

      {/* Content */}
      <div className="activity-monitor-content">
        {/* Filters Panel */}
        {showFilters && (
          <ActivityFilterPanel
            filters={filters}
            onFiltersChange={handleFilterChange}
            viewSettings={viewSettings}
            onViewSettingsChange={handleViewSettingsChange}
          />
        )}

        {/* Stats Panel */}
        {showStats && (
          <ActivityStatsPanel
            stats={activityStats}
            isLoading={activityState.isLoading}
          />
        )}

        {/* Log Display */}
        <div className="activity-log-container">
          <div className="log-header">
            <div className="log-controls">
              <button
                onClick={scrollToTop}
                className="scroll-button"
                title="Scroll to top"
              >
                ⬆️ Top
              </button>
              <button
                onClick={scrollToBottom}
                className="scroll-button"
                title="Scroll to bottom"
              >
                ⬇️ Bottom
              </button>
              
              <label className="auto-scroll-toggle">
                <input
                  type="checkbox"
                  checked={activityState.isAutoScrollEnabled}
                  onChange={(e) => setActivityState(prev => ({
                    ...prev,
                    isAutoScrollEnabled: e.target.checked,
                  }))}
                />
                <span>Auto-scroll</span>
              </label>
            </div>
          </div>

          <div
            ref={scrollContainerRef}
            className="activity-log-scroll"
            style={{ height }}
            onScroll={handleScroll}
          >
            {filteredEntries.length > 0 ? (
              <VirtualScrollList
                items={filteredEntries}
                renderItem={renderLogEntry}
                itemHeight={viewSettings.compactMode ? 40 : 80}
                containerHeight={height}
                overscan={5}
              />
            ) : (
              <div className="empty-log">
                <span className="empty-icon">📋</span>
                <p className="empty-message">{getEmptyMessage()}</p>
                {activityState.error && (
                  <button
                    onClick={loadActivityLog}
                    className="retry-button"
                  >
                    Retry
                  </button>
                )}
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
};