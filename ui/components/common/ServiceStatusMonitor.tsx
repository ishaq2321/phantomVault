/**
 * Service Status Monitor Component
 * 
 * Displays the current status of the PhantomVault service connection
 */

import React, { useState, useEffect, useCallback } from 'react';

export interface ServiceStatusMonitorProps {
  className?: string;
  showDetails?: boolean;
  onStatusChange?: (connected: boolean) => void;
}

interface ServiceStatus {
  connected: boolean;
  reconnecting: boolean;
  error: string | null;
  lastConnected: Date | null;
  reconnectAttempts: number;
}

/**
 * Service status monitor component
 */
export const ServiceStatusMonitor: React.FC<ServiceStatusMonitorProps> = ({
  className = '',
  showDetails = false,
  onStatusChange,
}) => {
  const [status, setStatus] = useState<ServiceStatus>({
    connected: false,
    reconnecting: false,
    error: null,
    lastConnected: null,
    reconnectAttempts: 0,
  });

  // Handle service connected event
  const handleServiceConnected = useCallback((event: CustomEvent) => {
    const data = event.detail;
    setStatus(prev => ({
      ...prev,
      connected: true,
      reconnecting: false,
      error: null,
      lastConnected: new Date(),
      reconnectAttempts: 0,
    }));
    
    onStatusChange?.(true);
  }, [onStatusChange]);

  // Handle service disconnected event
  const handleServiceDisconnected = useCallback((event: CustomEvent) => {
    const data = event.detail;
    setStatus(prev => ({
      ...prev,
      connected: false,
      reconnecting: false,
      error: 'Service disconnected',
    }));
    
    onStatusChange?.(false);
  }, [onStatusChange]);

  // Handle service connection failed event
  const handleServiceConnectionFailed = useCallback((event: CustomEvent) => {
    const data = event.detail;
    setStatus(prev => ({
      ...prev,
      connected: false,
      reconnecting: false,
      error: 'Connection failed permanently',
    }));
    
    onStatusChange?.(false);
  }, [onStatusChange]);

  // Handle service restarting event
  const handleServiceRestarting = useCallback((event: CustomEvent) => {
    setStatus(prev => ({
      ...prev,
      reconnecting: true,
      error: null,
    }));
  }, []);

  // Set up event listeners
  useEffect(() => {
    window.addEventListener('service-connected', handleServiceConnected as EventListener);
    window.addEventListener('service-disconnected', handleServiceDisconnected as EventListener);
    window.addEventListener('service-connection-failed', handleServiceConnectionFailed as EventListener);
    window.addEventListener('service-restarting', handleServiceRestarting as EventListener);

    return () => {
      window.removeEventListener('service-connected', handleServiceConnected as EventListener);
      window.removeEventListener('service-disconnected', handleServiceDisconnected as EventListener);
      window.removeEventListener('service-connection-failed', handleServiceConnectionFailed as EventListener);
      window.removeEventListener('service-restarting', handleServiceRestarting as EventListener);
    };
  }, [handleServiceConnected, handleServiceDisconnected, handleServiceConnectionFailed, handleServiceRestarting]);

  // Get status color
  const getStatusColor = () => {
    if (status.reconnecting) return '#FF9800'; // Orange
    if (status.connected) return '#4CAF50'; // Green
    return '#F44336'; // Red
  };

  // Get status icon
  const getStatusIcon = () => {
    if (status.reconnecting) return '🔄';
    if (status.connected) return '🟢';
    return '🔴';
  };

  // Get status text
  const getStatusText = () => {
    if (status.reconnecting) return 'Reconnecting...';
    if (status.connected) return 'Connected';
    if (status.error) return status.error;
    return 'Disconnected';
  };

  // Handle manual reconnect
  const handleReconnect = useCallback(async () => {
    try {
      setStatus(prev => ({ ...prev, reconnecting: true, error: null }));
      
      // In a real implementation, this would trigger a reconnection
      // For now, we'll simulate it
      setTimeout(() => {
        setStatus(prev => ({
          ...prev,
          connected: true,
          reconnecting: false,
          error: null,
          lastConnected: new Date(),
        }));
        onStatusChange?.(true);
      }, 2000);
    } catch (error) {
      setStatus(prev => ({
        ...prev,
        reconnecting: false,
        error: 'Reconnection failed',
      }));
    }
  }, [onStatusChange]);

  return (
    <div className={`service-status-monitor ${className}`}>
      <div className=\"status-indicator\">
        <span 
          className=\"status-icon\"
          style={{ color: getStatusColor() }}
        >
          {getStatusIcon()}
        </span>
        <span className=\"status-text\">{getStatusText()}</span>
        
        {!status.connected && !status.reconnecting && (
          <button 
            onClick={handleReconnect}
            className=\"reconnect-button\"
            title=\"Reconnect to service\"
          >
            🔄 Reconnect
          </button>
        )}
      </div>

      {showDetails && (
        <div className=\"status-details\">
          {status.lastConnected && (
            <div className=\"detail-item\">
              <span className=\"detail-label\">Last Connected:</span>
              <span className=\"detail-value\">
                {status.lastConnected.toLocaleString()}
              </span>
            </div>
          )}
          
          {status.reconnectAttempts > 0 && (
            <div className=\"detail-item\">
              <span className=\"detail-label\">Reconnect Attempts:</span>
              <span className=\"detail-value\">{status.reconnectAttempts}</span>
            </div>
          )}
          
          {status.error && (
            <div className=\"detail-item error\">
              <span className=\"detail-label\">Error:</span>
              <span className=\"detail-value\">{status.error}</span>
            </div>
          )}
        </div>
      )}
    </div>
  );
};