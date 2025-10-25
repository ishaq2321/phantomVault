/**
 * Bulk Operation Summary Component
 * 
 * Component for displaying results of bulk operations
 */

import React, { useState, useCallback } from 'react';
import { BulkOperationResult } from '../../types';

export interface BulkOperationSummaryProps {
  results: BulkOperationResult;
  onClose: () => void;
  onRetryFailed?: () => void;
  onExportResults?: () => void;
  className?: string;
}

/**
 * Bulk operation summary component
 */
export const BulkOperationSummary: React.FC<BulkOperationSummaryProps> = ({
  results,
  onClose,
  onRetryFailed,
  onExportResults,
  className = '',
}) => {
  const [activeTab, setActiveTab] = useState<'overview' | 'successful' | 'failed'>('overview');
  const [showDetails, setShowDetails] = useState<Record<string, boolean>>({});

  // ==================== CALCULATIONS ====================

  const duration = results.endTime.getTime() - results.startTime.getTime();
  const successRate = results.total > 0 ? (results.successful.length / results.total) * 100 : 0;
  const hasFailures = results.failed.length > 0;
  const hasSkipped = results.skipped.length > 0;

  // ==================== HANDLERS ====================

  const handleTabChange = useCallback((tab: typeof activeTab) => {
    setActiveTab(tab);
  }, []);

  const toggleDetails = useCallback((id: string) => {
    setShowDetails(prev => ({
      ...prev,
      [id]: !prev[id],
    }));
  }, []);

  const handleExportResults = useCallback(() => {
    if (onExportResults) {
      onExportResults();
    } else {
      // Default export functionality
      const exportData = {
        operation: results.operation,
        timestamp: new Date().toISOString(),
        summary: {
          total: results.total,
          successful: results.successful.length,
          failed: results.failed.length,
          skipped: results.skipped.length,
          duration: `${Math.round(duration / 1000)}s`,
          successRate: `${Math.round(successRate)}%`,
        },
        results: {
          successful: results.successful.map(s => ({
            vaultName: s.vault.name,
            vaultPath: s.vault.path,
            message: s.message,
          })),
          failed: results.failed.map(f => ({
            vaultName: f.vault.name,
            vaultPath: f.vault.path,
            error: f.error,
            message: f.message,
          })),
          skipped: results.skipped.map(s => ({
            vaultName: s.vault.name,
            vaultPath: s.vault.path,
            reason: s.reason,
          })),
        },
      };

      const blob = new Blob([JSON.stringify(exportData, null, 2)], { type: 'application/json' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = `bulk-operation-results-${Date.now()}.json`;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      URL.revokeObjectURL(url);
    }
  }, [results, duration, successRate, onExportResults]);

  // ==================== RENDER HELPERS ====================

  const getOperationIcon = () => {
    switch (results.operation) {
      case 'mount': return '🔓';
      case 'unmount': return '🔒';
      case 'delete': return '🗑️';
      default: return '⚙️';
    }
  };

  const getOverallStatus = () => {
    if (results.failed.length === 0) {
      return { status: 'success', text: 'All operations completed successfully', color: '#4CAF50' };
    } else if (results.successful.length === 0) {
      return { status: 'failed', text: 'All operations failed', color: '#F44336' };
    } else {
      return { status: 'partial', text: 'Some operations failed', color: '#FF9800' };
    }
  };

  const formatDuration = (ms: number) => {
    const seconds = Math.floor(ms / 1000);
    const minutes = Math.floor(seconds / 60);
    
    if (minutes > 0) {
      return `${minutes}m ${seconds % 60}s`;
    }
    return `${seconds}s`;
  };

  const overallStatus = getOverallStatus();

  // ==================== TAB RENDERERS ====================

  const renderOverviewTab = () => (
    <div className="overview-tab">
      {/* Summary Stats */}
      <div className="summary-stats">
        <div className="stat-card total">
          <div className="stat-value">{results.total}</div>
          <div className="stat-label">Total Vaults</div>
        </div>
        
        <div className="stat-card successful">
          <div className="stat-value">{results.successful.length}</div>
          <div className="stat-label">Successful</div>
        </div>
        
        <div className="stat-card failed">
          <div className="stat-value">{results.failed.length}</div>
          <div className="stat-label">Failed</div>
        </div>
        
        {hasSkipped && (
          <div className="stat-card skipped">
            <div className="stat-value">{results.skipped.length}</div>
            <div className="stat-label">Skipped</div>
          </div>
        )}
      </div>

      {/* Success Rate */}
      <div className="success-rate-section">
        <div className="success-rate-header">
          <span className="success-rate-label">Success Rate</span>
          <span className="success-rate-value">{Math.round(successRate)}%</span>
        </div>
        <div className="success-rate-bar">
          <div 
            className="success-rate-fill"
            style={{ 
              width: `${successRate}%`,
              backgroundColor: successRate >= 80 ? '#4CAF50' : successRate >= 50 ? '#FF9800' : '#F44336'
            }}
          />
        </div>
      </div>

      {/* Operation Details */}
      <div className="operation-details">
        <div className="detail-row">
          <span className="detail-label">Operation:</span>
          <span className="detail-value">
            {getOperationIcon()} {results.operation.charAt(0).toUpperCase() + results.operation.slice(1)}
          </span>
        </div>
        
        <div className="detail-row">
          <span className="detail-label">Duration:</span>
          <span className="detail-value">{formatDuration(duration)}</span>
        </div>
        
        <div className="detail-row">
          <span className="detail-label">Started:</span>
          <span className="detail-value">{results.startTime.toLocaleString()}</span>
        </div>
        
        <div className="detail-row">
          <span className="detail-label">Completed:</span>
          <span className="detail-value">{results.endTime.toLocaleString()}</span>
        </div>
      </div>

      {/* Quick Actions */}
      <div className="quick-actions">
        {hasFailures && onRetryFailed && (
          <button onClick={onRetryFailed} className="action-button retry">
            <span className="action-icon">🔄</span>
            Retry Failed Operations
          </button>
        )}
        
        <button onClick={handleExportResults} className="action-button export">
          <span className="action-icon">📊</span>
          Export Results
        </button>
      </div>
    </div>
  );

  const renderSuccessfulTab = () => (
    <div className="results-tab successful-tab">
      <div className="tab-header">
        <h4 className="tab-title">
          Successful Operations ({results.successful.length})
        </h4>
      </div>
      
      <div className="results-list">
        {results.successful.map((result, index) => (
          <div key={`success-${index}`} className="result-item success">
            <div className="result-header">
              <div className="result-info">
                <span className="result-icon">✅</span>
                <span className="vault-name">{result.vault.name}</span>
              </div>
              <span className="result-status">Success</span>
            </div>
            
            <div className="result-details">
              <div className="vault-path">{result.vault.path}</div>
              <div className="result-message">{result.message}</div>
            </div>
          </div>
        ))}
        
        {results.successful.length === 0 && (
          <div className="empty-results">
            <span className="empty-icon">📭</span>
            <p className="empty-text">No successful operations</p>
          </div>
        )}
      </div>
    </div>
  );

  const renderFailedTab = () => (
    <div className="results-tab failed-tab">
      <div className="tab-header">
        <h4 className="tab-title">
          Failed Operations ({results.failed.length})
        </h4>
      </div>
      
      <div className="results-list">
        {results.failed.map((result, index) => (
          <div key={`failed-${index}`} className="result-item failed">
            <div className="result-header">
              <div className="result-info">
                <span className="result-icon">❌</span>
                <span className="vault-name">{result.vault.name}</span>
              </div>
              <button
                onClick={() => toggleDetails(`failed-${index}`)}
                className="details-toggle"
              >
                {showDetails[`failed-${index}`] ? 'Hide Details' : 'Show Details'}
              </button>
            </div>
            
            <div className="result-details">
              <div className="vault-path">{result.vault.path}</div>
              <div className="error-message">{result.error}</div>
              
              {showDetails[`failed-${index}`] && (
                <div className="error-details">
                  <div className="detail-section">
                    <strong>Error Message:</strong>
                    <p>{result.error}</p>
                  </div>
                  
                  {result.result.details && (
                    <div className="detail-section">
                      <strong>Technical Details:</strong>
                      <pre className="technical-details">
                        {typeof result.result.details === 'string' 
                          ? result.result.details 
                          : JSON.stringify(result.result.details, null, 2)
                        }
                      </pre>
                    </div>
                  )}
                </div>
              )}
            </div>
          </div>
        ))}
        
        {results.failed.length === 0 && (
          <div className="empty-results">
            <span className="empty-icon">✅</span>
            <p className="empty-text">No failed operations</p>
          </div>
        )}
      </div>
    </div>
  );

  // ==================== MAIN RENDER ====================

  return (
    <div className="bulk-operation-summary-overlay">
      <div className={`bulk-operation-summary ${className}`}>
        {/* Header */}
        <div className="summary-header">
          <div className="header-content">
            <span className="operation-icon">{getOperationIcon()}</span>
            <div className="header-text">
              <h2 className="summary-title">Operation Complete</h2>
              <p className="summary-subtitle">
                {results.operation.charAt(0).toUpperCase() + results.operation.slice(1)} operation results
              </p>
            </div>
          </div>
          <button onClick={onClose} className="summary-close">✕</button>
        </div>

        {/* Status Banner */}
        <div className={`status-banner ${overallStatus.status}`}>
          <div className="status-content">
            <span 
              className="status-icon"
              style={{ color: overallStatus.color }}
            >
              {overallStatus.status === 'success' ? '✅' : 
               overallStatus.status === 'failed' ? '❌' : '⚠️'}
            </span>
            <span className="status-text">{overallStatus.text}</span>
          </div>
        </div>

        {/* Tab Navigation */}
        <div className="summary-tabs">
          <button
            onClick={() => handleTabChange('overview')}
            className={`tab-button ${activeTab === 'overview' ? 'active' : ''}`}
          >
            <span className="tab-icon">📊</span>
            Overview
          </button>
          
          <button
            onClick={() => handleTabChange('successful')}
            className={`tab-button ${activeTab === 'successful' ? 'active' : ''}`}
          >
            <span className="tab-icon">✅</span>
            Successful ({results.successful.length})
          </button>
          
          <button
            onClick={() => handleTabChange('failed')}
            className={`tab-button ${activeTab === 'failed' ? 'active' : ''}`}
          >
            <span className="tab-icon">❌</span>
            Failed ({results.failed.length})
          </button>
        </div>

        {/* Tab Content */}
        <div className="summary-content">
          {activeTab === 'overview' && renderOverviewTab()}
          {activeTab === 'successful' && renderSuccessfulTab()}
          {activeTab === 'failed' && renderFailedTab()}
        </div>

        {/* Footer */}
        <div className="summary-footer">
          <div className="footer-info">
            <span className="completion-time">
              Completed in {formatDuration(duration)}
            </span>
          </div>
          
          <div className="footer-actions">
            {hasFailures && onRetryFailed && (
              <button onClick={onRetryFailed} className="footer-button retry">
                <span className="button-icon">🔄</span>
                Retry Failed
              </button>
            )}
            
            <button onClick={onClose} className="footer-button primary">
              <span className="button-icon">✓</span>
              Done
            </button>
          </div>
        </div>
      </div>
    </div>
  );
};