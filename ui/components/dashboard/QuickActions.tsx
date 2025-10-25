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
  loading?: boolean;
}

/**
 * Quick actions toolbar component
 */
export const QuickActions: React.FC<QuickActionsProps> = ({
  selectedCount,
  onBulkAction,
  onRefresh,
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
          onClick={onRefresh}
          disabled={loading}
          className="action-button action-refresh"
          title="Refresh vault list"
        >
          <span className={`button-icon ${loading ? 'spinning' : ''}`}>🔄</span>
          <span className="button-text">Refresh</span>
        </button>
        
        <button
          onClick={() => {
            // This would typically open the vault creation wizard
            // For now, we'll just log it
            console.log('Create new vault clicked');
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