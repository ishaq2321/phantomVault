/**
 * Status Bar Component
 * 
 * Bottom status bar showing system information and current status
 */

import React, { useState, useEffect } from 'react';
import { AppView } from '../../App';
import { ServiceConnectionState } from '../../hooks/useServiceConnection';
import { useVault } from '../../contexts';

export interface StatusBarProps {
  currentView: AppView;
  serviceStatus: ServiceConnectionState;
  className?: string;
}

/**
 * Status bar component
 */
export const StatusBar: React.FC<StatusBarProps> = ({
  currentView,
  serviceStatus,
  className = '',
}) => {
  const { state: vaultState } = useVault();
  const [currentTime, setCurrentTime] = useState(new Date());

  // Update time every second
  useEffect(() => {
    const timer = setInterval(() => {
      setCurrentTime(new Date());
    }, 1000);

    return () => clearInterval(timer);
  }, []);

  // Get vault statistics
  const getVaultStats = () => {
    const total = vaultState.vaults.length;
    const mounted = vaultState.vaults.filter(v => v.status === 'mounted').length;
    const unmounted = vaultState.vaults.filter(v => v.status === 'unmounted').length;
    const errors = vaultState.vaults.filter(v => v.status === 'error').length;
    
    return { total, mounted, unmounted, errors };
  };

  const vaultStats = getVaultStats();

  // Get service status indicator
  const getServiceStatusInfo = () => {
    if (serviceStatus.connected) {
      return {
        icon: '🟢',
        text: 'Connected',
        color: '#4CAF50',
        details: serviceStatus.health.latency ? `${serviceStatus.health.latency}ms` : '',
      };
    } else if (serviceStatus.reconnecting) {
      return {
        icon: '🟡',
        text: 'Reconnecting...',
        color: '#FF9800',
        details: `Attempt ${serviceStatus.reconnectAttempts}`,
      };
    } else {
      return {
        icon: '🔴',
        text: 'Disconnected',
        color: '#F44336',
        details: serviceStatus.error || '',
      };
    }
  };

  const serviceInfo = getServiceStatusInfo();

  // Get memory usage (mock data for now)
  const getMemoryUsage = () => {
    // In a real implementation, this would get actual memory usage
    return {
      used: 45,
      total: 128,
      percentage: Math.round((45 / 128) * 100),
    };
  };

  const memoryUsage = getMemoryUsage();

  // Format uptime
  const formatUptime = (seconds: number) => {
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;
    
    if (hours > 0) {
      return `${hours}h ${minutes}m`;
    } else if (minutes > 0) {
      return `${minutes}m ${secs}s`;
    } else {
      return `${secs}s`;
    }
  };

  return (
    <div className={`status-bar ${className}`}>
      {/* Left Section - View Status */}
      <div className="status-section left">
        <div className="status-item">
          <span className="status-label">View:</span>
          <span className="status-value">{currentView}</span>
        </div>
        
        {vaultState.isLoading && (
          <div className="status-item loading">
            <span className="loading-spinner small">⏳</span>
            <span className="status-value">Loading...</span>
          </div>
        )}
        
        {vaultState.error && (
          <div className="status-item error">
            <span className="status-icon">❌</span>
            <span className="status-value">{vaultState.error}</span>
          </div>
        )}
      </div>

      {/* Center Section - Vault Statistics */}
      <div className="status-section center">
        <div className="vault-stats">
          <div className="stat-item\" title="Total vaults">
            <span className="stat-icon">🗄️</span>
            <span className="stat-value">{vaultStats.total}</span>
          </div>
          
          <div className="stat-separator">|</div>
          
          <div className="stat-item\" title="Mounted vaults">
            <span className="stat-icon">🟢</span>
            <span className="stat-value">{vaultStats.mounted}</span>
          </div>
          
          <div className="stat-separator">|</div>
          
          <div className="stat-item\" title="Unmounted vaults">
            <span className="stat-icon">⚪</span>
            <span className="stat-value">{vaultStats.unmounted}</span>
          </div>
          
          {vaultStats.errors > 0 && (
            <>
              <div className="stat-separator">|</div>
              <div className="stat-item error" title="Vaults with errors">
                <span className="stat-icon">❌</span>
                <span className="stat-value">{vaultStats.errors}</span>
              </div>
            </>
          )}
        </div>
      </div>

      {/* Right Section - System Status */}
      <div className="status-section right">
        {/* Service Status */}
        <div className="status-item service-status">
          <span 
            className="status-icon"
            style={{ color: serviceInfo.color }}
          >
            {serviceInfo.icon}
          </span>
          <span className="status-value">{serviceInfo.text}</span>
          {serviceInfo.details && (
            <span className="status-details">({serviceInfo.details})</span>
          )}
        </div>

        <div className="status-separator">|</div>

        {/* Memory Usage */}
        <div className="status-item memory-usage" title={`Memory: ${memoryUsage.used}MB / ${memoryUsage.total}MB`}>
          <span className="status-icon">💾</span>
          <span className="status-value">{memoryUsage.used}MB</span>
          <div className="memory-bar">
            <div 
              className="memory-fill"
              style={{ width: `${memoryUsage.percentage}%` }}
            />
          </div>
        </div>

        <div className="status-separator">|</div>

        {/* Uptime */}
        {serviceStatus.health.uptime > 0 && (
          <>
            <div className="status-item uptime" title="Service uptime">
              <span className="status-icon">⏱️</span>
              <span className="status-value">{formatUptime(serviceStatus.health.uptime)}</span>
            </div>
            
            <div className="status-separator">|</div>
          </>
        )}

        {/* Current Time */}
        <div className="status-item current-time">
          <span className="status-icon">🕐</span>
          <span className="status-value">
            {currentTime.toLocaleTimeString([], { 
              hour: '2-digit', 
              minute: '2-digit',
              second: '2-digit'
            })}
          </span>
        </div>
      </div>
    </div>
  );
};