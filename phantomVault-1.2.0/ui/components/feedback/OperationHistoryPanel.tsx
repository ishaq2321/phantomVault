/**
 * Operation History Panel Component
 * 
 * Panel for displaying operation history and statistics
 */

import React, { useState, useMemo, useCallback } from 'react';
import { OperationHistory } from '../../types';

export interface OperationHistoryPanelProps {
  history: OperationHistory[];
  stats: {
    total: number;
    successful: number;
    failed: number;
    retried: number;
    byOperation: Record<string, { success: number; failed: number }>;
  };
  onRetry: (item: OperationHistory) => void;
  onShowDetails: (item: OperationHistory) => void;
  onClear: () => void;
  className?: string;
}

interface FilterState {
  operation: string;
  status: 'all' | 'success' | 'failed';
  vaultName: string;
  timeRange: 'all' | 'hour' | 'day' | 'week';
}

/**
 * Operation history panel component
 */
export const OperationHistoryPanel: React.FC<OperationHistoryPanelProps> = ({
  history,
  stats,
  onRetry,
  onShowDetails,
  onClear,
  className = '',
}) => {
  const [isExpanded, setIsExpanded] = useState(false);
  const [filters, setFilters] = useState<FilterState>({
    operation: 'all',
    status: 'all',
    vaultName: '',
    timeRange: 'all',
  });

  // ==================== FILTERING ====================

  const filteredHistory = useMemo(() => {
    let filtered = [...history];

    // Filter by operation
    if (filters.operation !== 'all') {
      filtered = filtered.filter(item => item.operation === filters.operation);
    }

    // Filter by status
    if (filters.status !== 'all') {
      filtered = filtered.filter(item => 
        filters.status === 'success' ? item.success : !item.success
      );
    }

    // Filter by vault name
    if (filters.vaultName) {
      filtered = filtered.filter(item => 
        item.vaultName.toLowerCase().includes(filters.vaultName.toLowerCase())
      );
    }

    // Filter by time range
    if (filters.timeRange !== 'all') {
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
      }
      
      filtered = filtered.filter(item => item.timestamp > cutoff);
    }

    return filtered;
  }, [history, filters]);

  // ==================== HANDLERS ====================

  const handleFilterChange = useCallback((key: keyof FilterState, value: string) => {
    setFilters(prev => ({ ...prev, [key]: value }));
  }, []);

  const clearFilters = useCallback(() => {
    setFilters({
      operation: 'all',
      status: 'all',
      vaultName: '',
      timeRange: 'all',
    });
  }, []);

  const toggleExpanded = useCallback(() => {
    setIsExpanded(prev => !prev);
  }, []);

  // ==================== RENDER HELPERS ====================

  const formatTimestamp = (timestamp: Date) => {
    const now = new Date();
    const diff = now.getTime() - timestamp.getTime();
    
    if (diff < 60000) { // Less than 1 minute
      return 'Just now';
    } else if (diff < 3600000) { // Less than 1 hour
      return `${Math.floor(diff / 60000)}m ago`;
    } else if (diff < 86400000) { // Less than 1 day
      return `${Math.floor(diff / 3600000)}h ago`;
    } else {
      return timestamp.toLocaleDateString();
    }
  };

  const getOperationIcon = (operation: string) => {
    switch (operation) {
      case 'mount': return '🔓';
      case 'unmount': return '🔒';
      case 'create': return '🆕';
      case 'delete': return '🗑️';
      case 'update': return '✏️';
      default: return '⚙️';
    }
  };

  const getStatusIcon = (success: boolean) => {
    return success ? '✅' : '❌';
  };

  const renderStatsCard = () => (
    <div className="history-stats">
      <div className="stats-header">
        <h3 className="stats-title">Operation Statistics</h3>
      </div>
      <div className="stats-grid">
        <div className="stat-item">
          <span className="stat-value">{stats.total}</span>
          <span className="stat-label">Total</span>
        </div>
        <div className="stat-item success">
          <span className="stat-value">{stats.successful}</span>
          <span className="stat-label">Successful</span>
        </div>
        <div className="stat-item failed">
          <span className="stat-value">{stats.failed}</span>
          <span className="stat-label">Failed</span>
        </div>
        <div className="stat-item retried">
          <span className="stat-value">{stats.retried}</span>
          <span className="stat-label">Retried</span>
        </div>
      </div>
      
      {/* Success Rate */}
      <div className="success-rate">
        <div className="success-rate-label">Success Rate</div>
        <div className="success-rate-bar">
          <div 
            className="success-rate-fill"
            style={{ 
              width: `${stats.total > 0 ? (stats.successful / stats.total) * 100 : 0}%` 
            }}
          />
        </div>
        <div className="success-rate-text">
          {stats.total > 0 ? Math.round((stats.successful / stats.total) * 100) : 0}%
        </div>
      </div>
    </div>
  );

  const renderFilters = () => (
    <div className="history-filters">
      <div className="filter-group">
        <label className="filter-label">Operation:</label>
        <select
          value={filters.operation}
          onChange={(e) => handleFilterChange('operation', e.target.value)}
          className="filter-select"
        >
          <option value="all">All Operations</option>
          <option value="mount">Mount</option>
          <option value="unmount">Unmount</option>
          <option value="create">Create</option>
          <option value="delete">Delete</option>
          <option value="update">Update</option>
        </select>
      </div>

      <div className="filter-group">
        <label className="filter-label">Status:</label>
        <select
          value={filters.status}
          onChange={(e) => handleFilterChange('status', e.target.value)}
          className="filter-select"
        >
          <option value="all">All</option>
          <option value="success">Success</option>
          <option value="failed">Failed</option>
        </select>
      </div>

      <div className="filter-group">
        <label className="filter-label">Time:</label>
        <select
          value={filters.timeRange}
          onChange={(e) => handleFilterChange('timeRange', e.target.value)}
          className="filter-select"
        >
          <option value="all">All Time</option>
          <option value="hour">Last Hour</option>
          <option value="day">Last Day</option>
          <option value="week">Last Week</option>
        </select>
      </div>

      <div className="filter-group">
        <label className="filter-label">Vault:</label>
        <input
          type="text"
          value={filters.vaultName}
          onChange={(e) => handleFilterChange('vaultName', e.target.value)}
          placeholder="Filter by vault name..."
          className="filter-input"
        />
      </div>

      <button onClick={clearFilters} className="clear-filters-button">
        Clear Filters
      </button>
    </div>
  );

  const renderHistoryItem = (item: OperationHistory) => (
    <div key={item.id} className={`history-item ${item.success ? 'success' : 'failed'}`}>
      <div className="history-item-header">
        <div className="history-item-info">
          <span className="operation-icon">{getOperationIcon(item.operation)}</span>
          <span className="operation-name">{item.operation}</span>
          <span className="vault-name">"{item.vaultName}"</span>
          <span className="status-icon">{getStatusIcon(item.success)}</span>
        </div>
        <div className="history-item-meta">
          <span className="timestamp">{formatTimestamp(item.timestamp)}</span>
        </div>
      </div>

      <div className="history-item-content">
        <p className="history-message">{item.message}</p>
        {item.error && (
          <p className="history-error">Error: {item.error}</p>
        )}
      </div>

      <div className="history-item-actions">
        {!item.success && (
          <button
            onClick={() => onShowDetails(item)}
            className="history-action-button details"
          >
            View Details
          </button>
        )}
        {item.retryable && (
          <button
            onClick={() => onRetry(item)}
            className="history-action-button retry"
          >
            Retry
          </button>
        )}
      </div>
    </div>
  );

  // ==================== MAIN RENDER ====================

  return (
    <div className={`operation-history-panel ${isExpanded ? 'expanded' : 'collapsed'} ${className}`}>
      {/* Panel Header */}
      <div className="history-panel-header">
        <button onClick={toggleExpanded} className="history-toggle">
          <span className="toggle-icon">{isExpanded ? '▼' : '▶'}</span>
          <span className="toggle-text">Operation History</span>
          <span className="history-count">({history.length})</span>
        </button>
        
        {isExpanded && (
          <div className="header-actions">
            <button onClick={onClear} className="clear-history-button">
              Clear History
            </button>
          </div>
        )}
      </div>

      {/* Panel Content */}
      {isExpanded && (
        <div className="history-panel-content">
          {/* Statistics */}
          {renderStatsCard()}

          {/* Filters */}
          {renderFilters()}

          {/* History List */}
          <div className="history-list">
            <div className="history-list-header">
              <h4 className="history-list-title">
                Recent Operations ({filteredHistory.length})
              </h4>
            </div>
            
            <div className="history-items">
              {filteredHistory.length > 0 ? (
                filteredHistory.map(renderHistoryItem)
              ) : (
                <div className="no-history">
                  <span className="no-history-icon">📋</span>
                  <p className="no-history-text">
                    {history.length === 0 
                      ? 'No operations performed yet.' 
                      : 'No operations match the current filters.'
                    }
                  </p>
                </div>
              )}
            </div>
          </div>
        </div>
      )}
    </div>
  );
};