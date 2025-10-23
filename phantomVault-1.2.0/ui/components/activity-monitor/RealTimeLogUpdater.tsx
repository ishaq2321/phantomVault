/**
 * Real-Time Log Updater Component
 * 
 * Handles real-time log updates from the service backend via IPC events
 */

import React, { useEffect, useCallback, useRef, useState } from 'react';
import { ActivityLogEntry } from '../../types';

export interface RealTimeLogUpdaterProps {
  onNewEntry: (entry: ActivityLogEntry) => void;
  onConnectionStatusChange: (connected: boolean) => void;
  isEnabled: boolean;
  maxRetries?: number;
  retryInterval?: number;
  className?: string;
}

interface ConnectionStatus {
  connected: boolean;
  lastHeartbeat: Date | null;
  retryCount: number;
  error: string | null;
}

/**
 * Real-time log updater component
 */
export const RealTimeLogUpdater: React.FC<RealTimeLogUpdaterProps> = ({
  onNewEntry,
  onConnectionStatusChange,
  isEnabled,
  maxRetries = 5,
  retryInterval = 3000,
  className = '',
}) => {
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>({
    connected: false,
    lastHeartbeat: null,
    retryCount: 0,
    error: null,
  });

  const retryTimeoutRef = useRef<NodeJS.Timeout | null>(null);
  const heartbeatIntervalRef = useRef<NodeJS.Timeout | null>(null);
  const unsubscribeRef = useRef<(() => void) | null>(null);

  // Handle new log entries from IPC
  const handleLogEntry = useCallback((entry: ActivityLogEntry) => {
    // Update heartbeat timestamp
    setConnectionStatus(prev => ({
      ...prev,
      lastHeartbeat: new Date(),
      connected: true,
      error: null,
    }));

    // Forward to parent component
    onNewEntry(entry);
  }, [onNewEntry]);

  // Handle connection errors
  const handleConnectionError = useCallback((error: string) => {
    setConnectionStatus(prev => ({
      ...prev,
      connected: false,
      error,
    }));
    onConnectionStatusChange(false);
  }, [onConnectionStatusChange]);

  // Subscribe to activity log events
  const subscribeToLogs = useCallback(async () => {
    try {
      // Clear any existing subscription
      if (unsubscribeRef.current) {
        unsubscribeRef.current();
        unsubscribeRef.current = null;
      }

      // Subscribe to new log entries via IPC
      const unsubscribe = await window.electronAPI?.subscribeToActivity?.(handleLogEntry);
      
      if (unsubscribe) {
        unsubscribeRef.current = unsubscribe;
        
        setConnectionStatus(prev => ({
          ...prev,
          connected: true,
          retryCount: 0,
          error: null,
          lastHeartbeat: new Date(),
        }));
        
        onConnectionStatusChange(true);
      } else {
        throw new Error('Failed to subscribe to activity log');
      }
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Unknown connection error';
      handleConnectionError(errorMessage);
      
      // Retry connection if under max retries
      if (connectionStatus.retryCount < maxRetries) {
        setConnectionStatus(prev => ({
          ...prev,
          retryCount: prev.retryCount + 1,
        }));
        
        retryTimeoutRef.current = setTimeout(() => {
          subscribeToLogs();
        }, retryInterval);
      }
    }
  }, [handleLogEntry, handleConnectionError, onConnectionStatusChange, connectionStatus.retryCount, maxRetries, retryInterval]);

  // Unsubscribe from logs
  const unsubscribeFromLogs = useCallback(() => {
    if (unsubscribeRef.current) {
      unsubscribeRef.current();
      unsubscribeRef.current = null;
    }
    
    if (retryTimeoutRef.current) {
      clearTimeout(retryTimeoutRef.current);
      retryTimeoutRef.current = null;
    }
    
    if (heartbeatIntervalRef.current) {
      clearInterval(heartbeatIntervalRef.current);
      heartbeatIntervalRef.current = null;
    }
    
    setConnectionStatus({
      connected: false,
      lastHeartbeat: null,
      retryCount: 0,
      error: null,
    });
    
    onConnectionStatusChange(false);
  }, [onConnectionStatusChange]);

  // Start heartbeat monitoring
  const startHeartbeatMonitor = useCallback(() => {
    if (heartbeatIntervalRef.current) {
      clearInterval(heartbeatIntervalRef.current);
    }
    
    heartbeatIntervalRef.current = setInterval(() => {
      const now = new Date();
      const lastHeartbeat = connectionStatus.lastHeartbeat;
      
      // Check if we haven't received a heartbeat in the last 30 seconds
      if (lastHeartbeat && (now.getTime() - lastHeartbeat.getTime()) > 30000) {
        handleConnectionError('Connection timeout - no heartbeat received');
        
        // Attempt to reconnect
        if (connectionStatus.retryCount < maxRetries) {
          subscribeToLogs();
        }
      }
    }, 10000); // Check every 10 seconds
  }, [connectionStatus.lastHeartbeat, connectionStatus.retryCount, handleConnectionError, maxRetries, subscribeToLogs]);

  // Manual reconnection
  const reconnect = useCallback(() => {
    setConnectionStatus(prev => ({
      ...prev,
      retryCount: 0,
      error: null,
    }));
    subscribeToLogs();
  }, [subscribeToLogs]);

  // Effect to manage subscription based on enabled state
  useEffect(() => {
    if (isEnabled) {
      subscribeToLogs();
      startHeartbeatMonitor();
    } else {
      unsubscribeFromLogs();
    }

    return () => {
      unsubscribeFromLogs();
    };
  }, [isEnabled, subscribeToLogs, startHeartbeatMonitor, unsubscribeFromLogs]);

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      unsubscribeFromLogs();
    };
  }, [unsubscribeFromLogs]);

  // Render connection status indicator
  const renderConnectionStatus = () => {
    if (!isEnabled) {
      return (
        <div className="connection-status disabled">
          <span className="status-icon">⏸️</span>
          <span className="status-text">Real-time updates disabled</span>
        </div>
      );
    }

    if (connectionStatus.connected) {
      return (
        <div className="connection-status connected">
          <span className="status-icon">🟢</span>
          <span className="status-text">Connected</span>
          {connectionStatus.lastHeartbeat && (
            <span className="last-update">
              Last update: {connectionStatus.lastHeartbeat.toLocaleTimeString()}
            </span>
          )}
        </div>
      );
    }

    if (connectionStatus.error) {
      return (
        <div className="connection-status error">
          <span className="status-icon">🔴</span>
          <span className="status-text">
            Connection error: {connectionStatus.error}
          </span>
          {connectionStatus.retryCount < maxRetries && (
            <span className="retry-info">
              Retrying... ({connectionStatus.retryCount}/{maxRetries})
            </span>
          )}
          {connectionStatus.retryCount >= maxRetries && (
            <button 
              onClick={reconnect}
              className="reconnect-button"
              title="Manually reconnect to service"
            >
              Reconnect
            </button>
          )}
        </div>
      );
    }

    return (
      <div className="connection-status connecting">
        <span className="status-icon">🟡</span>
        <span className="status-text">Connecting...</span>
      </div>
    );
  };

  return (
    <div className={`real-time-log-updater ${className}`}>
      {renderConnectionStatus()}
    </div>
  );
};