/**
 * Error Details Modal Component
 * 
 * Modal for displaying detailed error information and recovery options
 */

import React, { useState, useCallback } from 'react';
import { OperationHistory } from '../../types';

export interface ErrorDetailsModalProps {
  error: OperationHistory;
  onClose: () => void;
  onRetry?: () => void;
  className?: string;
}

interface ErrorAnalysis {
  category: 'authentication' | 'permission' | 'network' | 'filesystem' | 'corruption' | 'unknown';
  severity: 'low' | 'medium' | 'high' | 'critical';
  suggestions: string[];
  technicalDetails: string[];
}

/**
 * Error details modal component
 */
export const ErrorDetailsModal: React.FC<ErrorDetailsModalProps> = ({
  error,
  onClose,
  onRetry,
  className = '',
}) => {
  const [activeTab, setActiveTab] = useState<'overview' | 'technical' | 'solutions'>('overview');
  const [showRawDetails, setShowRawDetails] = useState(false);

  // ==================== ERROR ANALYSIS ====================

  const analyzeError = useCallback((errorItem: OperationHistory): ErrorAnalysis => {
    const errorMessage = errorItem.error?.toLowerCase() || '';
    
    // Categorize error
    let category: ErrorAnalysis['category'] = 'unknown';
    if (errorMessage.includes('password') || errorMessage.includes('auth')) {
      category = 'authentication';
    } else if (errorMessage.includes('permission') || errorMessage.includes('access')) {
      category = 'permission';
    } else if (errorMessage.includes('network') || errorMessage.includes('connection')) {
      category = 'network';
    } else if (errorMessage.includes('file') || errorMessage.includes('disk')) {
      category = 'filesystem';
    } else if (errorMessage.includes('corrupt') || errorMessage.includes('invalid')) {
      category = 'corruption';
    }

    // Determine severity
    let severity: ErrorAnalysis['severity'] = 'medium';
    if (errorMessage.includes('critical') || errorMessage.includes('fatal')) {
      severity = 'critical';
    } else if (errorMessage.includes('warning')) {
      severity = 'low';
    } else if (errorMessage.includes('corrupt') || errorMessage.includes('lost')) {
      severity = 'high';
    }

    // Generate suggestions
    const suggestions: string[] = [];
    const technicalDetails: string[] = [];

    switch (category) {
      case 'authentication':
        suggestions.push(
          'Verify that you entered the correct password',
          'Check if the vault password has been changed recently',
          'Try resetting the vault password if you have recovery options'
        );
        technicalDetails.push(
          'Authentication failed during vault unlock process',
          'Password verification returned false',
          'Consider checking vault integrity'
        );
        break;

      case 'permission':
        suggestions.push(
          'Check file system permissions for the vault folder',
          'Ensure PhantomVault has necessary access rights',
          'Try running as administrator if on Windows'
        );
        technicalDetails.push(
          'Insufficient permissions to access vault files',
          'File system access denied',
          'Check user account control settings'
        );
        break;

      case 'network':
        suggestions.push(
          'Check your network connection',
          'Verify that the vault path is accessible',
          'Try accessing the vault location manually'
        );
        technicalDetails.push(
          'Network connectivity issue detected',
          'Remote vault location unreachable',
          'Check network configuration and firewall settings'
        );
        break;

      case 'filesystem':
        suggestions.push(
          'Check if the vault folder exists and is accessible',
          'Verify disk space availability',
          'Run disk check utility if necessary'
        );
        technicalDetails.push(
          'File system error encountered',
          'Vault folder may be corrupted or missing',
          'Check disk health and available space'
        );
        break;

      case 'corruption':
        suggestions.push(
          'Try running vault repair utility',
          'Restore from backup if available',
          'Contact support for data recovery options'
        );
        technicalDetails.push(
          'Vault data corruption detected',
          'File integrity check failed',
          'Backup restoration may be required'
        );
        break;

      default:
        suggestions.push(
          'Try the operation again',
          'Restart PhantomVault and retry',
          'Check the application logs for more details'
        );
        technicalDetails.push(
          'Unknown error occurred',
          'Check application logs for more information',
          'Contact support if the issue persists'
        );
    }

    return { category, severity, suggestions, technicalDetails };
  }, []);

  const errorAnalysis = analyzeError(error);

  // ==================== HANDLERS ====================

  const handleTabChange = useCallback((tab: typeof activeTab) => {
    setActiveTab(tab);
  }, []);

  const handleRetry = useCallback(() => {
    if (onRetry) {
      onRetry();
    }
    onClose();
  }, [onRetry, onClose]);

  const copyErrorDetails = useCallback(() => {
    const details = {
      operation: error.operation,
      vaultName: error.vaultName,
      timestamp: error.timestamp.toISOString(),
      error: error.error,
      message: error.message,
      details: error.details,
    };

    navigator.clipboard.writeText(JSON.stringify(details, null, 2));
  }, [error]);

  // ==================== RENDER HELPERS ====================

  const getCategoryIcon = (category: string) => {
    switch (category) {
      case 'authentication': return '🔐';
      case 'permission': return '🚫';
      case 'network': return '🌐';
      case 'filesystem': return '💾';
      case 'corruption': return '⚠️';
      default: return '❓';
    }
  };

  const getSeverityColor = (severity: string) => {
    switch (severity) {
      case 'critical': return '#F44336';
      case 'high': return '#FF9800';
      case 'medium': return '#2196F3';
      case 'low': return '#4CAF50';
      default: return '#9E9E9E';
    }
  };

  const formatTimestamp = (timestamp: Date) => {
    return timestamp.toLocaleString();
  };

  const renderOverviewTab = () => (
    <div className="error-overview">
      <div className="error-summary">
        <div className="error-header">
          <span className="error-category-icon">
            {getCategoryIcon(errorAnalysis.category)}
          </span>
          <div className="error-info">
            <h3 className="error-title">
              {error.operation.charAt(0).toUpperCase() + error.operation.slice(1)} Operation Failed
            </h3>
            <p className="error-vault">Vault: "{error.vaultName}"</p>
            <p className="error-time">Time: {formatTimestamp(error.timestamp)}</p>
          </div>
          <div className="error-severity">
            <span 
              className="severity-badge"
              style={{ backgroundColor: getSeverityColor(errorAnalysis.severity) }}
            >
              {errorAnalysis.severity.toUpperCase()}
            </span>
          </div>
        </div>

        <div className="error-message">
          <h4 className="message-title">Error Message</h4>
          <p className="message-text">{error.error}</p>
        </div>

        <div className="error-category">
          <h4 className="category-title">Error Category</h4>
          <p className="category-text">
            {errorAnalysis.category.charAt(0).toUpperCase() + errorAnalysis.category.slice(1)} Error
          </p>
        </div>
      </div>
    </div>
  );

  const renderTechnicalTab = () => (
    <div className="error-technical">
      <div className="technical-section">
        <h4 className="section-title">Technical Details</h4>
        <ul className="technical-list">
          {errorAnalysis.technicalDetails.map((detail, index) => (
            <li key={index} className="technical-item">{detail}</li>
          ))}
        </ul>
      </div>

      <div className="technical-section">
        <h4 className="section-title">Operation Context</h4>
        <div className="context-grid">
          <div className="context-item">
            <span className="context-label">Operation:</span>
            <span className="context-value">{error.operation}</span>
          </div>
          <div className="context-item">
            <span className="context-label">Vault ID:</span>
            <span className="context-value">{error.vaultId}</span>
          </div>
          <div className="context-item">
            <span className="context-label">Timestamp:</span>
            <span className="context-value">{error.timestamp.toISOString()}</span>
          </div>
          <div className="context-item">
            <span className="context-label">Retryable:</span>
            <span className="context-value">{error.retryable ? 'Yes' : 'No'}</span>
          </div>
        </div>
      </div>

      {error.details && (
        <div className="technical-section">
          <h4 className="section-title">Raw Error Details</h4>
          <button
            onClick={() => setShowRawDetails(!showRawDetails)}
            className="toggle-raw-details"
          >
            {showRawDetails ? 'Hide' : 'Show'} Raw Details
          </button>
          {showRawDetails && (
            <pre className="raw-details">
              {typeof error.details === 'string' 
                ? error.details 
                : JSON.stringify(error.details, null, 2)
              }
            </pre>
          )}
        </div>
      )}
    </div>
  );

  const renderSolutionsTab = () => (
    <div className="error-solutions">
      <div className="solutions-section">
        <h4 className="section-title">Suggested Solutions</h4>
        <ol className="solutions-list">
          {errorAnalysis.suggestions.map((suggestion, index) => (
            <li key={index} className="solution-item">
              <span className="solution-number">{index + 1}</span>
              <span className="solution-text">{suggestion}</span>
            </li>
          ))}
        </ol>
      </div>

      <div className="solutions-section">
        <h4 className="section-title">Quick Actions</h4>
        <div className="quick-actions">
          {error.retryable && (
            <button onClick={handleRetry} className="action-button primary">
              <span className="action-icon">🔄</span>
              Retry Operation
            </button>
          )}
          <button onClick={copyErrorDetails} className="action-button secondary">
            <span className="action-icon">📋</span>
            Copy Error Details
          </button>
        </div>
      </div>

      <div className="solutions-section">
        <h4 className="section-title">Need More Help?</h4>
        <div className="help-options">
          <p className="help-text">
            If these solutions don't resolve the issue, you can:
          </p>
          <ul className="help-list">
            <li>Check the application logs for more detailed information</li>
            <li>Visit our documentation for troubleshooting guides</li>
            <li>Contact support with the error details</li>
          </ul>
        </div>
      </div>
    </div>
  );

  // ==================== MAIN RENDER ====================

  return (
    <div className="error-details-modal-overlay">
      <div className={`error-details-modal ${className}`}>
        {/* Header */}
        <div className="modal-header">
          <h2 className="modal-title">Error Details</h2>
          <button onClick={onClose} className="modal-close" title="Close">
            ✕
          </button>
        </div>

        {/* Tab Navigation */}
        <div className="modal-tabs">
          <button
            onClick={() => handleTabChange('overview')}
            className={`tab-button ${activeTab === 'overview' ? 'active' : ''}`}
          >
            <span className="tab-icon">📋</span>
            Overview
          </button>
          <button
            onClick={() => handleTabChange('technical')}
            className={`tab-button ${activeTab === 'technical' ? 'active' : ''}`}
          >
            <span className="tab-icon">🔧</span>
            Technical
          </button>
          <button
            onClick={() => handleTabChange('solutions')}
            className={`tab-button ${activeTab === 'solutions' ? 'active' : ''}`}
          >
            <span className="tab-icon">💡</span>
            Solutions
          </button>
        </div>

        {/* Tab Content */}
        <div className="modal-content">
          {activeTab === 'overview' && renderOverviewTab()}
          {activeTab === 'technical' && renderTechnicalTab()}
          {activeTab === 'solutions' && renderSolutionsTab()}
        </div>

        {/* Footer */}
        <div className="modal-footer">
          <button onClick={onClose} className="footer-button secondary">
            Close
          </button>
          {error.retryable && (
            <button onClick={handleRetry} className="footer-button primary">
              <span className="button-icon">🔄</span>
              Retry Operation
            </button>
          )}
        </div>
      </div>
    </div>
  );
};