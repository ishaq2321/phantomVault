/**
 * Vault Status Indicator Component
 * 
 * Displays overall vault statistics and status summary
 */

import React from 'react';

interface VaultStatusIndicatorProps {
  totalVaults: number;
  mountedVaults: number;
  errorVaults: number;
}

/**
 * Status indicator showing vault statistics
 */
export const VaultStatusIndicator: React.FC<VaultStatusIndicatorProps> = ({
  totalVaults,
  mountedVaults,
  errorVaults,
}) => {
  const unmountedVaults = totalVaults - mountedVaults - errorVaults;
  const healthPercentage = totalVaults > 0 ? Math.round((mountedVaults / totalVaults) * 100) : 0;

  return (
    <div className="vault-status-indicator">
      <div className="status-summary">
        <div className="status-item status-total">
          <span className="status-icon">📁</span>
          <span className="status-count">{totalVaults}</span>
          <span className="status-label">Total</span>
        </div>
        
        <div className="status-item status-mounted">
          <span className="status-icon">🔓</span>
          <span className="status-count">{mountedVaults}</span>
          <span className="status-label">Mounted</span>
        </div>
        
        <div className="status-item status-unmounted">
          <span className="status-icon">🔒</span>
          <span className="status-count">{unmountedVaults}</span>
          <span className="status-label">Unmounted</span>
        </div>
        
        {errorVaults > 0 && (
          <div className="status-item status-error">
            <span className="status-icon">❌</span>
            <span className="status-count">{errorVaults}</span>
            <span className="status-label">Errors</span>
          </div>
        )}
      </div>
      
      <div className="health-indicator">
        <div className="health-bar">
          <div 
            className="health-fill"
            style={{ 
              width: `${healthPercentage}%`,
              backgroundColor: healthPercentage >= 80 ? '#4CAF50' : 
                             healthPercentage >= 60 ? '#FF9800' : '#F44336'
            }}
          />
        </div>
        <span className="health-text">{healthPercentage}% Healthy</span>
      </div>
    </div>
  );
};