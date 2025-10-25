/**
 * Vault Metadata Component
 * 
 * Displays comprehensive vault metadata including size, access times, and statistics
 */

import React, { useState, useEffect } from 'react';
import { VaultInfo } from '../../types';

interface VaultMetadataProps {
  vault: VaultInfo;
  layout?: 'compact' | 'detailed' | 'grid';
  showLabels?: boolean;
  className?: string;
}

interface VaultStats {
  totalFiles: number;
  totalFolders: number;
  lastModified: Date;
  createdDate: Date;
  encryptionLevel: string;
  compressionRatio: number;
}

/**
 * Vault metadata display component
 */
export const VaultMetadata: React.FC<VaultMetadataProps> = ({
  vault,
  layout = 'compact',
  showLabels = true,
  className = '',
}) => {
  const [stats, setStats] = useState<VaultStats | null>(null);
  const [loading, setLoading] = useState(false);

  // ==================== DATA FETCHING ====================

  useEffect(() => {
    loadVaultStats();
  }, [vault.id]);

  const loadVaultStats = async () => {
    setLoading(true);
    try {
      // In a real implementation, this would fetch actual stats from the backend
      // For now, we'll simulate the data
      await new Promise(resolve => setTimeout(resolve, 500)); // Simulate API call
      
      const mockStats: VaultStats = {
        totalFiles: Math.floor(Math.random() * 1000) + 50,
        totalFolders: Math.floor(Math.random() * 50) + 5,
        lastModified: new Date(Date.now() - Math.random() * 30 * 24 * 60 * 60 * 1000), // Random date within last 30 days
        createdDate: new Date(Date.now() - Math.random() * 365 * 24 * 60 * 60 * 1000), // Random date within last year
        encryptionLevel: 'AES-256-GCM',
        compressionRatio: Math.random() * 0.4 + 0.6, // 60-100% (higher is less compressed)
      };
      
      setStats(mockStats);
    } catch (error) {
      console.error('Failed to load vault stats:', error);
    } finally {
      setLoading(false);
    }
  };

  // ==================== FORMATTING HELPERS ====================

  const formatFileSize = (bytes: number): string => {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
  };

  const formatDate = (date: Date, format: 'relative' | 'absolute' = 'relative'): string => {
    if (format === 'absolute') {
      return date.toLocaleDateString() + ' ' + date.toLocaleTimeString();
    }

    const now = new Date();
    const diffMs = now.getTime() - date.getTime();
    const diffMins = Math.floor(diffMs / 60000);
    const diffHours = Math.floor(diffMs / 3600000);
    const diffDays = Math.floor(diffMs / 86400000);

    if (diffMins < 1) return 'Just now';
    if (diffMins < 60) return `${diffMins}m ago`;
    if (diffHours < 24) return `${diffHours}h ago`;
    if (diffDays < 7) return `${diffDays}d ago`;
    if (diffDays < 30) return `${Math.floor(diffDays / 7)}w ago`;
    if (diffDays < 365) return `${Math.floor(diffDays / 30)}mo ago`;
    return `${Math.floor(diffDays / 365)}y ago`;
  };

  const formatCompressionRatio = (ratio: number): string => {
    const compressionPercent = Math.round((1 - ratio) * 100);
    return compressionPercent > 0 ? `${compressionPercent}% compressed` : 'No compression';
  };

  const getVaultAge = (): string => {
    if (!stats) return 'Unknown';
    return formatDate(stats.createdDate);
  };

  // ==================== RENDER METHODS ====================

  const renderCompactLayout = () => (
    <div className={`vault-metadata vault-metadata-compact ${className}`}>
      <div className="metadata-row">
        <div className="metadata-item">
          <span className="metadata-icon">📊</span>
          <span className="metadata-value">{formatFileSize(vault.size)}</span>
          {showLabels && <span className="metadata-label">Size</span>}
        </div>
        
        <div className="metadata-item">
          <span className="metadata-icon">📁</span>
          <span className="metadata-value">{vault.folderCount}</span>
          {showLabels && <span className="metadata-label">Folders</span>}
        </div>
        
        <div className="metadata-item">
          <span className="metadata-icon">🕒</span>
          <span className="metadata-value">{formatDate(vault.lastAccess)}</span>
          {showLabels && <span className="metadata-label">Last Access</span>}
        </div>
      </div>
    </div>
  );

  const renderDetailedLayout = () => (
    <div className={`vault-metadata vault-metadata-detailed ${className}`}>
      {loading ? (
        <div className="metadata-loading">
          <span className="loading-spinner">⏳</span>
          <span>Loading metadata...</span>
        </div>
      ) : (
        <div className="metadata-sections">
          {/* Basic Information */}
          <div className="metadata-section">
            <h4 className="section-title">Basic Information</h4>
            <div className="metadata-grid">
              <div className="metadata-item">
                <span className="metadata-label">Size</span>
                <span className="metadata-value">{formatFileSize(vault.size)}</span>
              </div>
              <div className="metadata-item">
                <span className="metadata-label">Folders</span>
                <span className="metadata-value">{vault.folderCount}</span>
              </div>
              <div className="metadata-item">
                <span className="metadata-label">Status</span>
                <span className={`metadata-value status-${vault.status}`}>
                  {vault.status.charAt(0).toUpperCase() + vault.status.slice(1)}
                </span>
              </div>
            </div>
          </div>

          {/* File Statistics */}
          {stats && (
            <div className="metadata-section">
              <h4 className="section-title">File Statistics</h4>
              <div className="metadata-grid">
                <div className="metadata-item">
                  <span className="metadata-label">Files</span>
                  <span className="metadata-value">{stats.totalFiles.toLocaleString()}</span>
                </div>
                <div className="metadata-item">
                  <span className="metadata-label">Directories</span>
                  <span className="metadata-value">{stats.totalFolders.toLocaleString()}</span>
                </div>
                <div className="metadata-item">
                  <span className="metadata-label">Compression</span>
                  <span className="metadata-value">{formatCompressionRatio(stats.compressionRatio)}</span>
                </div>
              </div>
            </div>
          )}

          {/* Timestamps */}
          <div className="metadata-section">
            <h4 className="section-title">Timestamps</h4>
            <div className="metadata-grid">
              <div className="metadata-item">
                <span className="metadata-label">Last Access</span>
                <span className="metadata-value" title={formatDate(vault.lastAccess, 'absolute')}>
                  {formatDate(vault.lastAccess)}
                </span>
              </div>
              {stats && (
                <>
                  <div className="metadata-item">
                    <span className="metadata-label">Last Modified</span>
                    <span className="metadata-value" title={formatDate(stats.lastModified, 'absolute')}>
                      {formatDate(stats.lastModified)}
                    </span>
                  </div>
                  <div className="metadata-item">
                    <span className="metadata-label">Created</span>
                    <span className="metadata-value" title={formatDate(stats.createdDate, 'absolute')}>
                      {getVaultAge()}
                    </span>
                  </div>
                </>
              )}
            </div>
          </div>

          {/* Security Information */}
          {stats && (
            <div className="metadata-section">
              <h4 className="section-title">Security</h4>
              <div className="metadata-grid">
                <div className="metadata-item">
                  <span className="metadata-label">Encryption</span>
                  <span className="metadata-value">{stats.encryptionLevel}</span>
                </div>
                <div className="metadata-item">
                  <span className="metadata-label">Profile</span>
                  <span className="metadata-value">{vault.profile.name}</span>
                </div>
              </div>
            </div>
          )}

          {/* Path Information */}
          <div className="metadata-section">
            <h4 className="section-title">Location</h4>
            <div className="metadata-item metadata-path">
              <span className="metadata-label">Path</span>
              <span className="metadata-value path-value" title={vault.path}>
                {vault.path}
              </span>
            </div>
          </div>
        </div>
      )}
    </div>
  );

  const renderGridLayout = () => (
    <div className={`vault-metadata vault-metadata-grid ${className}`}>
      <div className="metadata-grid-container">
        <div className="metadata-card">
          <div className="card-icon">📊</div>
          <div className="card-content">
            <div className="card-value">{formatFileSize(vault.size)}</div>
            <div className="card-label">Total Size</div>
          </div>
        </div>
        
        <div className="metadata-card">
          <div className="card-icon">📁</div>
          <div className="card-content">
            <div className="card-value">{vault.folderCount}</div>
            <div className="card-label">Folders</div>
          </div>
        </div>
        
        {stats && (
          <>
            <div className="metadata-card">
              <div className="card-icon">📄</div>
              <div className="card-content">
                <div className="card-value">{stats.totalFiles.toLocaleString()}</div>
                <div className="card-label">Files</div>
              </div>
            </div>
            
            <div className="metadata-card">
              <div className="card-icon">🕒</div>
              <div className="card-content">
                <div className="card-value">{formatDate(vault.lastAccess)}</div>
                <div className="card-label">Last Access</div>
              </div>
            </div>
          </>
        )}
      </div>
    </div>
  );

  // ==================== MAIN RENDER ====================

  switch (layout) {
    case 'detailed':
      return renderDetailedLayout();
    case 'grid':
      return renderGridLayout();
    case 'compact':
    default:
      return renderCompactLayout();
  }
};

// ==================== METADATA TOOLTIP COMPONENT ====================

interface MetadataTooltipProps {
  vault: VaultInfo;
  children: React.ReactNode;
}

export const MetadataTooltip: React.FC<MetadataTooltipProps> = ({ vault, children }) => {
  const [showTooltip, setShowTooltip] = useState(false);

  return (
    <div 
      className="metadata-tooltip-container"
      onMouseEnter={() => setShowTooltip(true)}
      onMouseLeave={() => setShowTooltip(false)}
    >
      {children}
      {showTooltip && (
        <div className="metadata-tooltip">
          <VaultMetadata vault={vault} layout="detailed" showLabels={true} />
        </div>
      )}
    </div>
  );
};