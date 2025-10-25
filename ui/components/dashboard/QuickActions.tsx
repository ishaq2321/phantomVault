/**
 * Quick Actions Component
 * 
 * Provides quick action buttons for vault operations
 */

import React from 'react';
import { VaultAction } from '../../types';

interface QuickActionsProps {
  selectedCount: number;
  onBulkAction: (action: VaultAction) => void;
  onRefresh: () => void;
  onCreateVault?: () => void;
  loading?: boolean;
}

/**
 * Quick actions toolbar component
 */
export const QuickActions: React.FC<QuickActionsProps> = ({
  selectedCount,
  onBulkAction,
  onRefresh,
  onCreateVault,
  loading = false,
}) => {
  return (
    <div className="quick-actions">
      {/* Bulk Actions (shown when vaults are selected) */}
      {selectedCount > 0 && (
        <div className="bulk-actions">
          <span className="bulk-count">{selectedCount} selected</span>
          
          <button
            onClick={() => onBulkAction('mount')}
            disabled={loading}
            className="bulk-action-button bulk-mount"
            title="Mount selected vaults"
          >
            <span className="button-icon">📁</span>
            <span className="button-text">Mount All</span>
          </button>
          
          <button
            onClick={() => onBulkAction('unmount')}
            disabled={loading}
            className="bulk-action-button bulk-unmount"
            title="Unmount selected vaults"
          >
            <span className="button-icon">⏏️</span>
            <span className="button-text">Unmount All</span>
          </button>
        </div>
      )}
      
      {/* Standard Actions */}
      <div className="standard-actions">
        <button
          onClick={() => {
            console.log('🔄 Refresh button clicked!');
            onRefresh();
          }}
          disabled={loading}
          className="action-button action-refresh"
          title="Refresh vault list"
        >
          <span className={`button-icon ${loading ? 'spinning' : ''}`}>🔄</span>
          <span className="button-text">Refresh</span>
        </button>
        
        <button
          onClick={() => {
            console.log('➕ New Vault button clicked!');
            if (onCreateVault) {
              onCreateVault();
            } else {
              alert('New Vault button clicked! This feature needs to be connected to the create vault dialog.');
            }
          }}
          disabled={loading}
          className="action-button action-create"
          title="Create new vault"
        >
          <span className="button-icon">➕</span>
          <span className="button-text">New Vault</span>
        </button>
      </div>
    </div>
  );
};