/**
 * Enhanced Activity Monitor Component
 * 
 * Activity monitor with real-time updates, filtering, and visual indicators
 */

import React, { useState, useCallback, useEffect, useRef } from 'react';
import { ActivityLogEntry, ActivityFilter } from '../../types';
import { RealTimeLogUpdater } from './RealTimeLogUpdater';
import { ActivityFilterPanel } from './ActivityFilterPanel';

export interface EnhancedActivityMonitorProps {
  maxEntries?: number;
  autoScroll?: boolean;
  showFilters?: boolean;
  enableRealTime?: boolean;
  className?: string;
}

interface LogState {
  entries: ActivityLogEntry[];
  filteredEntries: ActivityLogEntry[];
  newEntryCount: number;
  isAtBottom: boolean;
  highlightedEntries: Set<string>;
}

/**
 * Enhanced activity monitor component with real-time updates
 */
export const EnhancedActivityMonitor: React.FC<EnhancedActivityMonitorProps> = ({
  maxEntries = 1000,
  autoScroll = true,
  showFilters = true,
  enableRealTime = true,
  className = '',
}) => {
  const [logState, setLogState] = useState<LogState>({
    entries: [],
    filteredEntries: [],
    newEntryCount: 0,
    isAtBottom: true,
    highlightedEntries: new Set(),
  });

  const [filters, setFilters] = useState<ActivityFilter>({
    vaultId: 'all',
    level: 'all',
    timeRange: 'all',
    search: '',
    dateFrom: null,
    dateTo: null,
  });

  const [isRealTimeConnected, setIsRealTimeConnected] = useState(false);
  const [showNewEntryNotification, setShowNewEntryNotification] = useState(false);
  
  const logContainerRef = useRef<HTMLDivElement>(null);
  const notificationTimeoutRef = useRef<NodeJS.Timeout | null>(null);

  // Handle new log entry from real-time updater
  const handleNewLogEntry = useCallback((entry: ActivityLogEntry) => {
    setLogState(prev => {
      const newEntries = [entry, ...prev.entries].slice(0, maxEntries);
      const newHighlighted = new Set(prev.highlightedEntries);
      newHighlighted.add(entry.id);
      
      // Remove highlight after 5 seconds
      setTimeout(() => {
        setLogState(current => ({
          ...current,
          highlightedEntries: new Set([...current.highlightedEntries].filter(id => id !== entry.id)),
        }));
      }, 5000);

      return {
        ...prev,
        entries: newEntries,
        newEntryCount: prev.isAtBottom ? 0 : prev.newEntryCount + 1,
        highlightedEntries: newHighlighted,
      };
    });

    // Show notification if not at bottom
    if (!logState.isAtBottom) {
      setShowNewEntryNotification(true);
      
      if (notificationTimeoutRef.current) {
        clearTimeout(notificationTimeoutRef.current);
      }
      
      notificationTimeoutRef.current = setTimeout(() => {
        setShowNewEntryNotification(false);
      }, 3000);
    }

    // Auto-scroll if enabled and at bottom
    if (autoScroll && logState.isAtBottom) {
      setTimeout(() => {
        scrollToBottom();
      }, 100);
    }
  }, [maxEntries, logState.isAtBottom, autoScroll]);

  // Handle real-time connection status
  const handleConnectionStatusChange = useCallback((connected: boolean) => {
    setIsRealTimeConnected(connected);
  }, []);

  // Apply filters to log entries
  const applyFilters = useCallback((entries: ActivityLogEntry[], currentFilters: ActivityFilter): ActivityLogEntry[] => {
    return entries.filter(entry => {
      // Vault filter
      if (currentFilters.vaultId !== 'all' && entry.vaultId !== currentFilters.vaultId) {
        return false;
      }

      // Level filter
      if (currentFilters.level !== 'all' && entry.level !== currentFilters.level) {
        return false;
      }

      // Search filter
      if (currentFilters.search && !entry.message.toLowerCase().includes(currentFilters.search.toLowerCase())) {
        return false;
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
    const filtered = applyFilters(logState.entries, filters);
    setLogState(prev => ({
      ...prev,
      filteredEntries: filtered,
    }));
  }, [logState.entries, filters, applyFilters]);

  // Handle filter changes
  const handleFiltersChange = useCallback((newFilters: Partial<ActivityFilter>) => {
    setFilters(prev => ({ ...prev, ...newFilters }));
  }, []);

  // Scroll to bottom
  const scrollToBottom = useCallback(() => {
    if (logContainerRef.current) {
      logContainerRef.current.scrollTop = logContainerRef.current.scrollHeight;
    }
  }, []);

  // Handle scroll events to detect if user is at bottom
  const handleScroll = useCallback(() => {
    if (logContainerRef.current) {
      const { scrollTop, scrollHeight, clientHeight } = logContainerRef.current;
      const isAtBottom = scrollTop + clientHeight >= scrollHeight - 10; // 10px tolerance
      
      setLogState(prev => ({
        ...prev,
        isAtBottom,
        newEntryCount: isAtBottom ? 0 : prev.newEntryCount,
      }));
      
      if (isAtBottom) {
        setShowNewEntryNotification(false);
      }
    }
  }, []);

  // Load initial log entries
  useEffect(() => {
    const loadInitialEntries = async () => {
      try {
        const response = await window.electronAPI?.getActivityLog?.();
        if (response?.success && response.data) {
          setLogState(prev => ({
            ...prev,
            entries: response.data.slice(0, maxEntries),
          }));
        }
      } catch (error) {
        console.error('Failed to load initial activity log:', error);
      }
    };

    loadInitialEntries();
  }, [maxEntries]);

  // Clear all entries
  const clearLog = useCallback(async () => {
    try {
      await window.electronAPI?.clearActivityLog?.();
      setLogState(prev => ({
        ...prev,
        entries: [],
        filteredEntries: [],
        newEntryCount: 0,
        highlightedEntries: new Set(),
      }));
    } catch (error) {
      console.error('Failed to clear activity log:', error);
    }
  }, []);

  // Export log entries
  const exportLog = useCallback(() => {
    const logData = logState.filteredEntries.map(entry => ({
      timestamp: entry.timestamp.toISOString(),
      level: entry.level,
      message: entry.message,
      vaultName: entry.vaultName || 'System',
      details: entry.details,
    }));

    const blob = new Blob([JSON.stringify(logData, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `activity-log-${new Date().toISOString().split('T')[0]}.json`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  }, [logState.filteredEntries]);

  // Get level color
  const getLevelColor = (level: string) => {
    switch (level) {
      case 'error': return '#F44336';
      case 'warning': return '#FF9800';
      case 'info': return '#2196F3';
      case 'debug': return '#9E9E9E';
      default: return '#757575';
    }
  };

  // Get level icon
  const getLevelIcon = (level: string) => {
    switch (level) {
      case 'error': return '❌';
      case 'warning': return '⚠️';
      case 'info': return 'ℹ️';
      case 'debug': return '🔍';
      default: return '📝';
    }
  };

  return (
    <div className={`enhanced-activity-monitor ${className}`}>
      {/* Header */}
      <div className="monitor-header">
        <div className="header-title">
          <h3>Activity Monitor</h3>
          <span className="entry-count">
            {logState.filteredEntries.length} entries
            {logState.entries.length !== logState.filteredEntries.length && 
              ` (${logState.entries.length} total)`
            }
          </span>
        </div>
        
        <div className="header-actions">
          <button 
            onClick={exportLog}
            className="action-button"
            title="Export log entries"
            disabled={logState.filteredEntries.length === 0}
          >
            📤 Export
          </button>
          <button 
            onClick={clearLog}
            className="action-button danger"
            title="Clear all log entries"
          >
            🗑️ Clear
          </button>
        </div>
      </div>

      {/* Real-time updater */}
      {enableRealTime && (
        <RealTimeLogUpdater
          onNewEntry={handleNewLogEntry}
          onConnectionStatusChange={handleConnectionStatusChange}
          isEnabled={enableRealTime}
          className="real-time-updater"
        />
      )}

      {/* Filters */}
      {showFilters && (
        <ActivityFilterPanel
          filters={filters}
          onFiltersChange={handleFiltersChange}
          entryCount={logState.filteredEntries.length}
          className="filter-panel"
        />
      )}

      {/* New entry notification */}
      {showNewEntryNotification && logState.newEntryCount > 0 && (
        <div className="new-entry-notification">
          <span className="notification-text">
            {logState.newEntryCount} new {logState.newEntryCount === 1 ? 'entry' : 'entries'}
          </span>
          <button 
            onClick={scrollToBottom}
            className="scroll-to-bottom-button"
          >
            Scroll to bottom
          </button>
        </div>
      )}

      {/* Log container */}
      <div 
        ref={logContainerRef}
        className="log-container"
        onScroll={handleScroll}
      >
        {logState.filteredEntries.length === 0 ? (
          <div className="empty-log">
            <span className="empty-icon">📝</span>
            <p>No log entries found</p>
            {filters.search || filters.vaultId !== 'all' || filters.level !== 'all' ? (
              <p className="empty-suggestion">Try adjusting your filters</p>
            ) : (
              <p className="empty-suggestion">Activity will appear here as it happens</p>
            )}
          </div>
        ) : (
          <div className="log-entries">
            {logState.filteredEntries.map((entry) => (
              <div 
                key={entry.id}
                className={`log-entry ${entry.level} ${logState.highlightedEntries.has(entry.id) ? 'highlighted' : ''}`}
              >
                <div className="entry-header">
                  <span 
                    className="level-indicator"
                    style={{ color: getLevelColor(entry.level) }}
                  >
                    {getLevelIcon(entry.level)}
                  </span>
                  <span className="timestamp">
                    {entry.timestamp.toLocaleTimeString()}
                  </span>
                  {entry.vaultName && (
                    <span className="vault-name">
                      {entry.vaultName}
                    </span>
                  )}
                </div>
                <div className="entry-message">
                  {entry.message}
                </div>
                {entry.details && (
                  <div className="entry-details">
                    <pre>{entry.details}</pre>
                  </div>
                )}
              </div>
            ))}
          </div>
        )}
      </div>

      {/* Auto-scroll indicator */}
      {!logState.isAtBottom && (
        <div className="scroll-indicator">
          <button 
            onClick={scrollToBottom}
            className="scroll-to-bottom-fab"
            title="Scroll to bottom"
          >
            ⬇️
          </button>
        </div>
      )}
    </div>
  );
};