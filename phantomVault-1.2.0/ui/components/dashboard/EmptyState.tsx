/**
 * Empty State Component
 * 
 * Displays when no vaults are available
 */

import React from 'react';

interface EmptyStateProps {
  onCreateVault: () => void;
}

/**
 * Empty state component for when no vaults exist
 */
export const EmptyState: React.FC<EmptyStateProps> = ({ onCreateVault }) => {
  return (
    <div className="empty-state">
      <div className="empty-state-content">
        <div className="empty-state-icon">
          <span className="icon-primary">🔐</span>
          <span className="icon-secondary">📁</span>
        </div>
        
        <h2 className="empty-state-title">No Vaults Found</h2>
        
        <p className="empty-state-description">
          Get started by creating your first vault to securely store and manage your files.
          Vaults provide encrypted storage with easy access control.
        </p>
        
        <div className="empty-state-actions">
          <button
            onClick={onCreateVault}
            className="primary-action-button"
          >
            <span className="button-icon">➕</span>
            <span className="button-text">Create Your First Vault</span>
          </button>
        </div>
        
        <div className="empty-state-features">
          <div className="feature-item">
            <span className="feature-icon">🔒</span>
            <span className="feature-text">End-to-end encryption</span>
          </div>
          <div className="feature-item">
            <span className="feature-icon">⚡</span>
            <span className="feature-text">Quick access with keyboard shortcuts</span>
          </div>
          <div className="feature-item">
            <span className="feature-icon">🛡️</span>
            <span className="feature-text">Secure password management</span>
          </div>
        </div>
        
        <div className="empty-state-help">
          <p className="help-text">
            Need help getting started? 
            <button className="help-link">View Documentation</button>
          </p>
        </div>
      </div>
    </div>
  );
};