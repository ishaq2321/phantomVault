/**
 * Activity Stats Panel Component
 * 
 * Component for displaying activity statistics and metrics
 */

import React, { useMemo } from 'react';
import { ActivityStats } from '../../types';

export interface ActivityStatsPanelProps {
  stats: ActivityStats;
  isLoading?: boolean;
  className?: string;
}

/**
 * Activity stats panel component
 */
export const ActivityStatsPanel: React.FC<ActivityStatsPanelProps> = ({
  stats,
  isLoading = false,
  className = '',
}) => {
  // ==================== CALCULATIONS ====================

  const levelPercentages = useMemo(() => {
    if (stats.total === 0) {
      return { info: 0, warning: 0, error: 0, debug: 0 };
    }

    return {
      info: Math.round((stats.byLevel.info / stats.total) * 100),
      warning: Math.round((stats.byLevel.warning / stats.total) * 100),
      error: Math.round((stats.byLevel.error / stats.total) * 100),
      debug: Math.round((stats.byLevel.debug / stats.total) * 100),
    };
  }, [stats]);

  const topVaults = useMemo(() => {
    return Object.entries(stats.byVault)
      .sort(([, a], [, b]) => b - a)
      .slice(0, 5);
  }, [stats.byVault]);

  const timeRangeText = useMemo(() => {
    if (!stats.timeRange.start || !stats.timeRange.end) {
      return 'No time range';
    }

    const start = stats.timeRange.start;
    const end = stats.timeRange.end;
    const duration = end.getTime() - start.getTime();

    if (duration < 3600000) { // Less than 1 hour
      const minutes = Math.round(duration / 60000);
      return `${minutes} minute${minutes !== 1 ? 's' : ''}`;
    } else if (duration < 86400000) { // Less than 1 day
      const hours = Math.round(duration / 3600000);
      return `${hours} hour${hours !== 1 ? 's' : ''}`;
    } else {
      const days = Math.round(duration / 86400000);
      return `${days} day${days !== 1 ? 's' : ''}`;
    }
  }, [stats.timeRange]);

  // ==================== RENDER HELPERS ====================

  const getLevelColor = (level: string) => {
    switch (level) {
      case 'info': return '#2196F3';
      case 'warning': return '#FF9800';
      case 'error': return '#F44336';
      case 'debug': return '#9E9E9E';
      default: return '#B4B4B4';
    }
  };

  const getLevelIcon = (level: string) => {
    switch (level) {
      case 'info': return 'ℹ️';
      case 'warning': return '⚠️';
      case 'error': return '❌';
      case 'debug': return '🐛';
      default: return '📝';
    }
  };

  const formatNumber = (num: number) => {
    if (num >= 1000000) {
      return `${(num / 1000000).toFixed(1)}M`;
    } else if (num >= 1000) {
      return `${(num / 1000).toFixed(1)}K`;
    }
    return num.toString();
  };

  const renderLevelStats = () => (
    <div className="level-stats">
      <h4 className="stats-section-title">By Level</h4>
      
      <div className="level-breakdown">
        {Object.entries(stats.byLevel).map(([level, count]) => (
          <div key={level} className="level-stat-item">
            <div className="level-info">
              <span className="level-icon">{getLevelIcon(level)}</span>
              <span className="level-name">{level.charAt(0).toUpperCase() + level.slice(1)}</span>
            </div>
            
            <div className="level-metrics">
              <span className="level-count">{formatNumber(count)}</span>
              <span className="level-percentage">({levelPercentages[level as keyof typeof levelPercentages]}%)</span>
            </div>
            
            <div className="level-bar">
              <div 
                className="level-bar-fill"
                style={{ 
                  width: `${levelPercentages[level as keyof typeof levelPercentages]}%`,
                  backgroundColor: getLevelColor(level)
                }}
              />
            </div>
          </div>
        ))}
      </div>
    </div>
  );

  const renderVaultStats = () => (
    <div className="vault-stats">
      <h4 className="stats-section-title">Top Vaults</h4>
      
      {topVaults.length > 0 ? (
        <div className="vault-breakdown">
          {topVaults.map(([vaultName, count]) => {
            const percentage = Math.round((count / stats.total) * 100);
            return (
              <div key={vaultName} className="vault-stat-item">
                <div className="vault-info">
                  <span className="vault-icon">🔐</span>
                  <span className="vault-name" title={vaultName}>
                    {vaultName.length > 20 ? `${vaultName.substring(0, 20)}...` : vaultName}
                  </span>
                </div>
                
                <div className="vault-metrics">
                  <span className="vault-count">{formatNumber(count)}</span>
                  <span className="vault-percentage">({percentage}%)</span>
                </div>
                
                <div className="vault-bar">
                  <div 
                    className="vault-bar-fill"
                    style={{ width: `${percentage}%` }}
                  />
                </div>
              </div>
            );
          })}
        </div>
      ) : (
        <div className="no-vault-stats">
          <span className="no-stats-icon">📭</span>
          <span className="no-stats-text">No vault activity</span>
        </div>
      )}
    </div>
  );

  const renderOverviewStats = () => (
    <div className="overview-stats">
      <div className="overview-grid">
        <div className="overview-stat">
          <div className="stat-value">{formatNumber(stats.total)}</div>
          <div className="stat-label">Total Entries</div>
        </div>
        
        <div className="overview-stat error">
          <div className="stat-value">{formatNumber(stats.byLevel.error)}</div>
          <div className="stat-label">Errors</div>
        </div>
        
        <div className="overview-stat warning">
          <div className="stat-value">{formatNumber(stats.byLevel.warning)}</div>
          <div className="stat-label">Warnings</div>
        </div>
        
        <div className="overview-stat info">
          <div className="stat-value">{formatNumber(stats.byLevel.info)}</div>
          <div className="stat-label">Info</div>
        </div>
      </div>
      
      <div className="time-range-info">
        <span className="time-range-icon">🕒</span>
        <span className="time-range-text">Spanning {timeRangeText}</span>
      </div>
    </div>
  );

  // ==================== MAIN RENDER ====================

  if (isLoading) {
    return (
      <div className={`activity-stats-panel loading ${className}`}>
        <div className="stats-loading">
          <span className="loading-spinner">⏳</span>
          <span className="loading-text">Loading statistics...</span>
        </div>
      </div>
    );
  }

  if (stats.total === 0) {
    return (
      <div className={`activity-stats-panel empty ${className}`}>
        <div className="stats-empty">
          <span className="empty-icon">📊</span>
          <span className="empty-text">No activity data available</span>
        </div>
      </div>
    );
  }

  return (
    <div className={`activity-stats-panel ${className}`}>
      {/* Panel Header */}
      <div className="stats-panel-header">
        <h3 className="panel-title">Activity Statistics</h3>
        <div className="panel-actions">
          <button 
            className="refresh-stats-button"
            title="Refresh statistics"
          >
            🔄
          </button>
        </div>
      </div>

      {/* Panel Content */}
      <div className="stats-panel-content">
        {/* Overview */}
        {renderOverviewStats()}

        {/* Level Breakdown */}
        {renderLevelStats()}

        {/* Vault Breakdown */}
        {renderVaultStats()}
      </div>
    </div>
  );
};