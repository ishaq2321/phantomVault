/**
 * Vault Details Modal Component
 * 
 * Modal dialog displaying comprehensive vault information and metadata
 */

import React, { useState, useCallback } from 'react';
import { VaultInfo, VaultAction } from '../../types';
import { VaultMetadata } from './VaultMetadata';
import { VaultStatusAnimation } from './VaultStatusAnimation';
import { useVaultStatusAnimations } from '../../src/hooks';
import './VaultDetailsModal.css';

interface VaultDetailsModalProps {
  vault: VaultInfo;
  isOpen: boolean;
  onClose: () => void;
  onAction?: (action: VaultAction) => void;
}

/**
 * Vault details modal component
 */
export const VaultDetailsModal: React.FC<VaultDetailsModalProps> = ({
  vault,
  isOpen,
  onClose,
  onAction,
}) => {
  const [activeTab, setActiveTab] = useState<'overview' | 'metadata' | 'security' | 'activity'>('overview');
  const { getVaultAnimation } = useVaultStatusAnimations();
  const statusChange = getVaultAnimation(vault.id);

  // ==================== EVENT HANDLERS ====================

  const handleBackdropClick = useCallback((event: React.MouseEvent) => {
    if (event.target === event.currentTarget) {
      onClose();
    }
  }, [onClose]);

  const handleAction = useCallback((action: VaultAction) => {
    if (onAction) {
      onAction(action);
    }
  }, [onAction]);

  // ==================== HELPER FUNCTIONS ====================

  const getStatusColor = (status: string): string => {
    switch (status) {
      case 'mounted': return '#4CAF50';
      case 'unmounted': return '#FF9800';
      case 'error': return '#F44336';
      case 'loading': 
      case 'encrypting': 
      case 'decrypting': return '#2196F3';
      default: return '#9E9E9E';
    }
  };

  const formatPath = (path: string): string => {
    const parts = path.split('/');
    if (parts.length > 4) {
      return `.../${parts.slice(-3).join('/')}`;
    }
    return path;
  };

  // ==================== TAB CONTENT RENDERERS ====================

  const renderOverviewTab = () => (
    <div className="tab-content overview-content">
      <div className="vault-header">
        <div className="vault-icon-large">📁</div>
        <div className="vault-info">
          <h2 className="vault-title">{vault.name}</h2>
          <p className="vault-path" title={vault.path}>
            {formatPath(vault.path)}
          </p>
          <div className="vault-status-display">
            <VaultStatusAnimation
              vaultId={vault.id}
              currentStatus={vault.status}
              statusChange={statusChange}
              className="status-animation-large"
            />
            <span className="status-text">
              {vault.status.charAt(0).toUpperCase() + vault.status.slice(1)}
            </span>
          </div>
        </div>
      </div>

      <div className="overview-stats">
        <VaultMetadata vault={vault} layout="grid" showLabels={true} />
      </div>

      <div className="quick-actions-section">
        <h3>Quick Actions</h3>
        <div className="action-buttons">
          {vault.status === 'unmounted' && (
            <button
              onClick={() => handleAction('mount')}
              className="action-btn action-btn-primary"
            >
              <span className="btn-icon">📁</span>
              Mount Vault
            </button>
          )}
          {vault.status === 'mounted' && (
            <button
              onClick={() => handleAction('unmount')}
              className="action-btn action-btn-secondary"
            >
              <span className="btn-icon">⏏️</span>
              Unmount Vault
            </button>
          )}
          <button
            onClick={() => handleAction('edit')}
            className="action-btn action-btn-secondary"
          >
            <span className="btn-icon">✏️</span>
            Edit Settings
          </button>
        </div>
      </div>
    </div>
  );

  const renderMetadataTab = () => (
    <div className="tab-content metadata-content">
      <VaultMetadata vault={vault} layout="detailed" showLabels={true} />
    </div>
  );

  const renderSecurityTab = () => (
    <div className="tab-content security-content">
      <div className="security-section">
        <h3>Encryption Information</h3>
        <div className="security-grid">
          <div className="security-item">
            <span className="security-label">Algorithm</span>
            <span className="security-value">AES-256-GCM</span>
          </div>
          <div className="security-item">
            <span className="security-label">Key Derivation</span>
            <span className="security-value">PBKDF2-SHA256</span>
          </div>
          <div className="security-item">
            <span className="security-label">Iterations</span>
            <span className="security-value">100,000</span>
          </div>
        </div>
      </div>

      <div className="security-section">
        <h3>Access Control</h3>
        <div className="security-grid">
          <div className="security-item">
            <span className="security-label">Profile</span>
            <span className="security-value">{vault.profile.name}</span>
          </div>
          <div className="security-item">
            <span className="security-label">Created</span>
            <span className="security-value">
              {vault.profile.createdAt.toLocaleDateString()}
            </span>
          </div>
          <div className="security-item">
            <span className="security-label">Auto-Lock</span>
            <span className="security-value">Enabled</span>
          </div>
        </div>
      </div>

      <div className="security-section">
        <h3>Security Actions</h3>
        <div className="action-buttons">
          <button className="action-btn action-btn-secondary">
            <span className="btn-icon">🔑</span>
            Change Password
          </button>
          <button className="action-btn action-btn-secondary">
            <span className="btn-icon">🛡️</span>
            Update Recovery Key
          </button>
          <button className="action-btn action-btn-danger">
            <span className="btn-icon">🗑️</span>
            Delete Vault
          </button>
        </div>
      </div>
    </div>
  );

  const renderActivityTab = () => (
    <div className="tab-content activity-content">
      <div className="activity-section">
        <h3>Recent Activity</h3>
        <div className="activity-list">
          <div className="activity-item">
            <div className="activity-icon">🔓</div>
            <div className="activity-details">
              <div className="activity-action">Vault mounted</div>
              <div className="activity-time">2 hours ago</div>
            </div>
          </div>
          <div className="activity-item">
            <div className="activity-icon">📝</div>
            <div className="activity-details">
              <div className="activity-action">Files modified</div>
              <div className="activity-time">1 day ago</div>
            </div>
          </div>
          <div className="activity-item">
            <div className="activity-icon">🔒</div>
            <div className="activity-details">
              <div className="activity-action">Vault locked</div>
              <div className="activity-time">2 days ago</div>
            </div>
          </div>
        </div>
      </div>

      <div className="activity-section">
        <h3>Statistics</h3>
        <div className="stats-grid">
          <div className="stat-item">
            <div className="stat-value">47</div>
            <div className="stat-label">Mount Operations</div>
          </div>
          <div className="stat-item">
            <div className="stat-value">23</div>
            <div className="stat-label">Files Added</div>
          </div>
          <div className="stat-item">
            <div className="stat-value">12</div>
            <div className="stat-label">Days Active</div>
          </div>
        </div>
      </div>
    </div>
  );

  // ==================== MAIN RENDER ====================

  if (!isOpen) return null;

  return (
    <div className="vault-details-modal-backdrop" onClick={handleBackdropClick}>
      <div className="vault-details-modal">
        {/* Modal Header */}
        <div className="modal-header">
          <div className="header-left">
            <h1 className="modal-title">Vault Details</h1>
            <span className="vault-name-subtitle">{vault.name}</span>
          </div>
          <button
            onClick={onClose}
            className="modal-close-btn"
            title="Close"
          >
            ✕
          </button>
        </div>

        {/* Tab Navigation */}
        <div className="modal-tabs">
          <button
            className={`tab-btn ${activeTab === 'overview' ? 'active' : ''}`}
            onClick={() => setActiveTab('overview')}
          >
            <span className="tab-icon">📊</span>
            Overview
          </button>
          <button
            className={`tab-btn ${activeTab === 'metadata' ? 'active' : ''}`}
            onClick={() => setActiveTab('metadata')}
          >
            <span className="tab-icon">📋</span>
            Metadata
          </button>
          <button
            className={`tab-btn ${activeTab === 'security' ? 'active' : ''}`}
            onClick={() => setActiveTab('security')}
          >
            <span className="tab-icon">🔒</span>
            Security
          </button>
          <button
            className={`tab-btn ${activeTab === 'activity' ? 'active' : ''}`}
            onClick={() => setActiveTab('activity')}
          >
            <span className="tab-icon">📈</span>
            Activity
          </button>
        </div>

        {/* Tab Content */}
        <div className="modal-content">
          {activeTab === 'overview' && renderOverviewTab()}
          {activeTab === 'metadata' && renderMetadataTab()}
          {activeTab === 'security' && renderSecurityTab()}
          {activeTab === 'activity' && renderActivityTab()}
        </div>

        {/* Modal Footer */}
        <div className="modal-footer">
          <button
            onClick={onClose}
            className="footer-btn footer-btn-secondary"
          >
            Close
          </button>
          <button
            onClick={() => handleAction('edit')}
            className="footer-btn footer-btn-primary"
          >
            <span className="btn-icon">✏️</span>
            Edit Vault
          </button>
        </div>
      </div>
    </div>
  );
};