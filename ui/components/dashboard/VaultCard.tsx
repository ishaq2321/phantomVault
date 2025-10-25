/**
 * Vault Card Component
 * 
 * Individual vault display card with actions and status indicators
 */

import React, { useState, useCallback } from 'react';
import {
  VaultInfo,
  VaultAction,
  VaultStatus,
  UnlockMode
} from '../../types';
import { PasswordPromptModal } from '../common/PasswordPromptModal';
import { VaultStatusAnimation } from './VaultStatusAnimation';
import { VaultMetadata, MetadataTooltip } from './VaultMetadata';
import './VaultCard.css';
import { useVaultStatusAnimations } from '../../src/hooks';

interface VaultCardProps {
  vault: VaultInfo;
  viewMode: 'grid' | 'list';
  selected: boolean;
  onSelect: (selected: boolean) => void;
  onAction: (action: VaultAction, options?: { password?: string; mode?: UnlockMode }) => void;
  onViewDetails?: (vault: VaultInfo) => void;
  loading?: boolean;
}

/**
 * Individual vault card component
 */
export const VaultCard: React.FC<VaultCardProps> = ({
  vault,
  viewMode,
  selected,
  onSelect,
  onAction,
  onViewDetails,
  loading = false,
}) => {
  const [showPasswordPrompt, setShowPasswordPrompt] = useState(false);
  const [pendingAction, setPendingAction] = useState<{
    action: VaultAction;
    requiresPassword: boolean;
  } | null>(null);
  
  // Get status animations
  const { getVaultAnimation } = useVaultStatusAnimations();
  const statusChange = getVaultAnimation(vault.id);

  // ==================== STATUS HELPERS ====================

  const getStatusIcon = (status: VaultStatus): string => {
    switch (status) {
      case 'mounted': return '🔓';
      case 'unmounted': return '🔒';
      case 'error': return '❌';
      case 'loading': return '⏳';
      case 'encrypting': return '🔐';
      case 'decrypting': return '🔑';
      default: return '❓';
    }
  };

  const getStatusColor = (status: VaultStatus): string => {
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

  const getStatusText = (status: VaultStatus): string => {
    switch (status) {
      case 'mounted': return 'Mounted';
      case 'unmounted': return 'Unmounted';
      case 'error': return 'Error';
      case 'loading': return 'Loading...';
      case 'encrypting': return 'Encrypting...';
      case 'decrypting': return 'Decrypting...';
      default: return 'Unknown';
    }
  };

  // ==================== ACTION HANDLERS ====================

  const handleAction = useCallback((action: VaultAction) => {
    const requiresPassword = ['unlock', 'lock'].includes(action);
    
    if (action === 'view' && onViewDetails) {
      onViewDetails(vault);
      return;
    }
    
    if (requiresPassword) {
      setPendingAction({ action, requiresPassword });
      setShowPasswordPrompt(true);
    } else {
      onAction(action);
    }
  }, [onAction, onViewDetails, vault]);

  const handlePasswordSubmit = useCallback((password: string, mode?: UnlockMode) => {
    if (pendingAction) {
      onAction(pendingAction.action, { password, mode });
      setPendingAction(null);
      setShowPasswordPrompt(false);
    }
  }, [pendingAction, onAction]);

  const handlePasswordCancel = useCallback(() => {
    setPendingAction(null);
    setShowPasswordPrompt(false);
  }, []);

  // ==================== UTILITY FUNCTIONS ====================

  const formatFileSize = (bytes: number): string => {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
  };

  const formatLastAccess = (date: Date): string => {
    const now = new Date();
    const diffMs = now.getTime() - date.getTime();
    const diffMins = Math.floor(diffMs / 60000);
    const diffHours = Math.floor(diffMs / 3600000);
    const diffDays = Math.floor(diffMs / 86400000);

    if (diffMins < 1) return 'Just now';
    if (diffMins < 60) return `${diffMins}m ago`;
    if (diffHours < 24) return `${diffHours}h ago`;
    if (diffDays < 7) return `${diffDays}d ago`;
    return date.toLocaleDateString();
  };

  // ==================== AVAILABLE ACTIONS ====================

  const getAvailableActions = (): Array<{ action: VaultAction; label: string; icon: string; variant: 'primary' | 'secondary' | 'danger' }> => {
    const actions = [];

    switch (vault.status) {
      case 'mounted':
        actions.push(
          { action: 'unmount' as VaultAction, label: 'Unmount', icon: '⏏️', variant: 'secondary' as const },
          { action: 'lock' as VaultAction, label: 'Lock', icon: '🔒', variant: 'secondary' as const }
        );
        break;
      case 'unmounted':
        actions.push(
          { action: 'mount' as VaultAction, label: 'Mount', icon: '📁', variant: 'primary' as const },
          { action: 'unlock' as VaultAction, label: 'Unlock', icon: '🔓', variant: 'primary' as const }
        );
        break;
      case 'error':
        actions.push(
          { action: 'mount' as VaultAction, label: 'Retry', icon: '🔄', variant: 'primary' as const }
        );
        break;
    }

    // Always show view details and delete actions (when not loading)
    if (!['loading', 'encrypting', 'decrypting'].includes(vault.status)) {
      actions.push(
        { action: 'view' as VaultAction, label: 'Details', icon: '👁️', variant: 'secondary' as const },
        { action: 'delete' as VaultAction, label: 'Delete', icon: '🗑️', variant: 'danger' as const }
      );
    }

    return actions;
  };

  // ==================== RENDER METHODS ====================

  const renderGridCard = () => (
    <div 
      className={`vault-card vault-card-grid ${selected ? 'selected' : ''}`}
      style={{
        borderColor: getStatusColor(vault.status),
        opacity: loading ? 0.7 : 1,
      }}
    >
      {/* Selection Checkbox */}
      <div className="card-header">
        <label className="selection-checkbox">
          <input
            type="checkbox"
            checked={selected}
            onChange={(e) => onSelect(e.target.checked)}
            disabled={loading}
          />
        </label>
        <div className="status-badge" style={{ backgroundColor: getStatusColor(vault.status) }}>
          <VaultStatusAnimation
            vaultId={vault.id}
            currentStatus={vault.status}
            statusChange={statusChange}
            className="status-animation"
          />
          <span className="status-text">{getStatusText(vault.status)}</span>
        </div>
      </div>

      {/* Vault Info */}
      <div className="card-content">
        <div className="vault-icon">📁</div>
        <h3 className="vault-name" title={vault.name}>{vault.name}</h3>
        <p className="vault-path" title={vault.path}>{vault.path}</p>
        
        <MetadataTooltip vault={vault}>
          <VaultMetadata 
            vault={vault} 
            layout="compact" 
            showLabels={false}
            className="card-metadata"
          />
        </MetadataTooltip>
      </div>

      {/* Actions */}
      <div className="card-actions">
        {getAvailableActions().slice(0, 2).map(({ action, label, icon, variant }) => (
          <button
            key={action}
            onClick={() => handleAction(action)}
            disabled={loading}
            className={`action-button action-button-${variant}`}
            title={label}
          >
            <span className="button-icon">{icon}</span>
            <span className="button-text">{label}</span>
          </button>
        ))}
        
        {getAvailableActions().length > 2 && (
          <div className="more-actions">
            <button className="more-actions-button" title="More actions">
              ⋯
            </button>
            <div className="more-actions-menu">
              {getAvailableActions().slice(2).map(({ action, label, icon, variant }) => (
                <button
                  key={action}
                  onClick={() => handleAction(action)}
                  disabled={loading}
                  className={`menu-action menu-action-${variant}`}
                >
                  <span className="menu-icon">{icon}</span>
                  <span className="menu-text">{label}</span>
                </button>
              ))}
            </div>
          </div>
        )}
      </div>
    </div>
  );

  const renderListCard = () => (
    <div 
      className={`vault-card vault-card-list ${selected ? 'selected' : ''}`}
      style={{
        borderLeftColor: getStatusColor(vault.status),
        opacity: loading ? 0.7 : 1,
      }}
    >
      <div className="list-content">
        <div className="list-left">
          <label className="selection-checkbox">
            <input
              type="checkbox"
              checked={selected}
              onChange={(e) => onSelect(e.target.checked)}
              disabled={loading}
            />
          </label>
          
          <div className="vault-info">
            <div className="vault-primary">
              <span className="vault-icon">📁</span>
              <span className="vault-name">{vault.name}</span>
              <div className="status-badge" style={{ backgroundColor: getStatusColor(vault.status) }}>
                <VaultStatusAnimation
                  vaultId={vault.id}
                  currentStatus={vault.status}
                  statusChange={statusChange}
                  className="status-animation"
                />
                <span className="status-text">{getStatusText(vault.status)}</span>
              </div>
            </div>
            <div className="vault-secondary">
              <span className="vault-path">{vault.path}</span>
            </div>
          </div>
        </div>
        
        <div className="list-center">
          <MetadataTooltip vault={vault}>
            <VaultMetadata 
              vault={vault} 
              layout="grid" 
              showLabels={true}
              className="list-metadata"
            />
          </MetadataTooltip>
        </div>
        
        <div className="list-right">
          <div className="list-actions">
            {getAvailableActions().map(({ action, label, icon, variant }) => (
              <button
                key={action}
                onClick={() => handleAction(action)}
                disabled={loading}
                className={`action-button action-button-${variant} action-button-compact`}
                title={label}
              >
                <span className="button-icon">{icon}</span>
              </button>
            ))}
          </div>
        </div>
      </div>
    </div>
  );

  // ==================== MAIN RENDER ====================

  return (
    <>
      {viewMode === 'grid' ? renderGridCard() : renderListCard()}
      
      {/* Password Prompt Modal */}
      {showPasswordPrompt && pendingAction && (
        <PasswordPromptModal
          title={`${pendingAction.action === 'unlock' ? 'Unlock' : 'Lock'} Vault`}
          message={`Enter password to ${pendingAction.action} "${vault.name}"`}
          showModeSelection={pendingAction.action === 'unlock'}
          onSubmit={handlePasswordSubmit}
          onCancel={handlePasswordCancel}
        />
      )}
    </>
  );
};