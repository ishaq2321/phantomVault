/**
 * Activity Log Item Component
 * 
 * Component for displaying individual activity log entries
 */

import React, { useState, useCallback } from 'react';
import { ActivityLogEntry } from '../../types';

export interface ActivityLogItemProps {
  entry: ActivityLogEntry;
  showTimestamp?: boolean;
  showVaultName?: boolean;
  showLevel?: boolean;
  compactMode?: boolean;
  className?: string;
}

/**
 * Activity log item component
 */
export const ActivityLogItem: React.FC<ActivityLogItemProps> = ({
  entry,
  showTimestamp = true,
  showVaultName = true,
  showLevel = true,
  compactMode = false,
  className = '',
}) => {
  const [showDetails, setShowDetails] = useState(false);

  // ==================== HANDLERS ====================

  const toggleDetails = useCallback(() => {
    setShowDetails(prev => !prev);
  }, []);

  // ==================== RENDER HELPERS ====================

  const getLevelIcon = (level: ActivityLogEntry['level']) => {
    switch (level) {
      case 'info': return 'ℹ️';
      case 'warning': return '⚠️';
      case 'error': return '❌';
      case 'debug': return '🐛';
      default: return '📝';
    }
  };

  const getLevelColor = (level: ActivityLogEntry['level']) => {
    switch (level) {
      case 'info': return '#2196F3';
      case 'warning': return '#FF9800';
      case 'error': return '#F44336';
      case 'debug': return '#9E9E9E';
      default: return '#B4B4B4';
    }
  };

  const formatTimestamp = (timestamp: Date) => {
    const now = new Date();
    const diff = now.getTime() - timestamp.getTime();
    
    // If less than 1 minute ago, show "just now"
    if (diff < 60000) {
      return 'Just now';
    }
    
    // If less than 1 hour ago, show minutes
    if (diff < 3600000) {
      const minutes = Math.floor(diff / 60000);
      return `${minutes}m ago`;
    }
    
    // If less than 24 hours ago, show hours
    if (diff < 86400000) {
      const hours = Math.floor(diff / 3600000);
      return `${hours}h ago`;
    }
    
    // If less than 7 days ago, show days
    if (diff < 604800000) {
      const days = Math.floor(diff / 86400000);
      return `${days}d ago`;
    }
    
    // Otherwise show full date
    return timestamp.toLocaleDateString();
  };

  const formatFullTimestamp = (timestamp: Date) => {
    return timestamp.toLocaleString();
  };

  const truncateMessage = (message: string, maxLength: number = 100) => {
    if (message.length <= maxLength) return message;
    return message.substring(0, maxLength) + '...';
  };

  const hasDetails = entry.details && entry.details.trim().length > 0;

  // ==================== MAIN RENDER ====================

  return (
    <div className={`activity-log-item ${entry.level} ${compactMode ? 'compact' : ''} ${className}`}>
      {/* Main Content */}
      <div className="log-item-main">
        {/* Level Indicator */}
        {showLevel && (
          <div className="level-indicator">
            <span 
              className="level-icon"
              style={{ color: getLevelColor(entry.level) }}
              title={entry.level.toUpperCase()}
            >
              {getLevelIcon(entry.level)}
            </span>
          </div>
        )}

        {/* Content */}
        <div className="log-item-content">
          {/* Header */}
          <div className="log-item-header">
            {/* Timestamp */}
            {showTimestamp && (
              <div className="timestamp">
                <span 
                  className="timestamp-text"
                  title={formatFullTimestamp(entry.timestamp)}
                >
                  {formatTimestamp(entry.timestamp)}
                </span>
              </div>
            )}

            {/* Vault Name */}
            {showVaultName && entry.vaultName && (
              <div className="vault-name">
                <span className="vault-icon">🔐</span>
                <span className="vault-text">{entry.vaultName}</span>
              </div>
            )}

            {/* Level Badge (compact mode) */}
            {compactMode && showLevel && (
              <div className="level-badge">
                <span 
                  className="level-text"
                  style={{ color: getLevelColor(entry.level) }}
                >
                  {entry.level.toUpperCase()}
                </span>
              </div>
            )}
          </div>

          {/* Message */}
          <div className="log-message">
            <span className="message-text">
              {compactMode ? truncateMessage(entry.message, 80) : entry.message}
            </span>
            
            {/* Details Toggle */}
            {hasDetails && (
              <button
                onClick={toggleDetails}
                className="details-toggle"
                title={showDetails ? 'Hide details' : 'Show details'}
              >
                <span className="toggle-icon">
                  {showDetails ? '▼' : '▶'}
                </span>
                <span className="toggle-text">
                  {showDetails ? 'Hide' : 'Details'}
                </span>
              </button>
            )}
          </div>

          {/* Expanded Details */}
          {showDetails && hasDetails && (
            <div className="log-details">
              <div className="details-header">
                <span className="details-title">Details</span>
              </div>
              <div className="details-content">
                <pre className="details-text">{entry.details}</pre>
              </div>
            </div>
          )}
        </div>

        {/* Actions */}
        <div className="log-item-actions">
          {/* Copy Button */}
          <button
            onClick={() => {
              const text = `[${formatFullTimestamp(entry.timestamp)}] ${entry.level.toUpperCase()}: ${entry.message}${entry.details ? '\nDetails: ' + entry.details : ''}`;
              navigator.clipboard.writeText(text);
            }}
            className="action-button copy-button"
            title="Copy to clipboard"
          >
            📋
          </button>

          {/* Expand/Collapse Button */}
          {hasDetails && (
            <button
              onClick={toggleDetails}
              className="action-button expand-button"
              title={showDetails ? 'Collapse' : 'Expand'}
            >
              {showDetails ? '🔼' : '🔽'}
            </button>
          )}
        </div>
      </div>

      {/* Severity Indicator Bar */}
      <div 
        className="severity-bar"
        style={{ backgroundColor: getLevelColor(entry.level) }}
      />
    </div>
  );
};