/**
 * Service Connection Hook
 * 
 * React hook for managing service connection status and operations
 */

import { useState, useEffect, useCallback, useRef } from 'react';

export interface ServiceConnectionState {
  connected: boolean;
  reconnecting: boolean;
  error: string | null;
  lastConnected: Date | null;
  reconnectAttempts: number;
  health: {
    status: 'healthy' | 'degraded' | 'unhealthy' | 'disconnected';
    latency: number | null;
    uptime: number;
  };
}

export interface UseServiceConnectionReturn {
  // State
  connectionState: ServiceConnectionState;
  isConnected: boolean;
  isHealthy: boolean;
  
  // Actions
  reconnect: () => Promise<void>;
  disconnect: () => Promise<void>;
  checkHealth: () => Promise<void>;
  
  // Event handlers
  onConnectionChange: (callback: (connected: boolean) => void) => () => void;
  onHealthChange: (callback: (health: ServiceConnectionState['health']) => void) => () => void;
}

/**
 * Hook for service connection management
 */
export function useServiceConnection(): UseServiceConnectionReturn {
  const [connectionState, setConnectionState] = useState<ServiceConnectionState>({
    connected: true, // Default to connected since the app is running
    reconnecting: false,
    error: null,
    lastConnected: new Date(),
    reconnectAttempts: 0,
    health: {
      status: 'healthy', // Default to healthy
      latency: null,
      uptime: 0,
    },
  });

  const connectionCallbacksRef = useRef<Set<(connected: boolean) => void>>(new Set());
  const healthCallbacksRef = useRef<Set<(health: ServiceConnectionState['health']) => void>>(new Set());

  // Check service status on mount
  useEffect(() => {
    const checkServiceStatus = async () => {
      try {
        if (window.phantomVault?.service?.getStatus) {
          const result = await window.phantomVault.service.getStatus();
          if (result.success) {
            setConnectionState(prev => ({
              ...prev,
              connected: true,
              error: null,
              health: { ...prev.health, status: 'healthy' }
            }));
          }
        } else if (window.phantomVault?.getVersion) {
          // If PhantomVault API exists, consider it connected
          setConnectionState(prev => ({
            ...prev,
            connected: true,
            error: null,
            health: { ...prev.health, status: 'healthy' }
          }));
        }
      } catch (error) {
        console.warn('Service status check failed:', error);
      }
    };

    checkServiceStatus();
  }, []);

  // Handle service connected event
  const handleServiceConnected = useCallback((event: CustomEvent) => {
    const data = event.detail;
    
    setConnectionState(prev => ({
      ...prev,
      connected: true,
      reconnecting: false,
      error: null,
      lastConnected: new Date(),
      reconnectAttempts: 0,
      health: {
        ...prev.health,
        status: 'healthy',
      },
    }));

    // Notify callbacks
    connectionCallbacksRef.current.forEach(callback => callback(true));
  }, []);

  // Handle service disconnected event
  const handleServiceDisconnected = useCallback((event: CustomEvent) => {
    const data = event.detail;
    
    setConnectionState(prev => ({
      ...prev,
      connected: false,
      reconnecting: false,
      error: 'Service disconnected',
      health: {
        status: 'disconnected',
        latency: null,
        uptime: 0,
      },
    }));

    // Notify callbacks
    connectionCallbacksRef.current.forEach(callback => callback(false));
  }, []);

  // Handle service connection failed event
  const handleServiceConnectionFailed = useCallback((event: CustomEvent) => {
    const data = event.detail;
    
    setConnectionState(prev => ({
      ...prev,
      connected: false,
      reconnecting: false,
      error: 'Connection failed permanently',
      health: {
        status: 'disconnected',
        latency: null,
        uptime: 0,
      },
    }));

    // Notify callbacks
    connectionCallbacksRef.current.forEach(callback => callback(false));
  }, []);

  // Handle service restarting event
  const handleServiceRestarting = useCallback((event: CustomEvent) => {
    setConnectionState(prev => ({
      ...prev,
      reconnecting: true,
      error: null,
    }));
  }, []);

  // Handle health check updates
  const handleHealthCheck = useCallback((event: CustomEvent) => {
    const healthData = event.detail;
    
    setConnectionState(prev => ({
      ...prev,
      health: {
        status: healthData.status || prev.health.status,
        latency: healthData.latency || prev.health.latency,
        uptime: healthData.uptime || prev.health.uptime,
      },
    }));

    // Notify health callbacks
    healthCallbacksRef.current.forEach(callback => callback({
      status: healthData.status || 'disconnected',
      latency: healthData.latency || null,
      uptime: healthData.uptime || 0,
    }));
  }, []);

  // Set up event listeners
  useEffect(() => {
    window.addEventListener('service-connected', handleServiceConnected as EventListener);
    window.addEventListener('service-disconnected', handleServiceDisconnected as EventListener);
    window.addEventListener('service-connection-failed', handleServiceConnectionFailed as EventListener);
    window.addEventListener('service-restarting', handleServiceRestarting as EventListener);
    window.addEventListener('service-health-check', handleHealthCheck as EventListener);

    return () => {
      window.removeEventListener('service-connected', handleServiceConnected as EventListener);
      window.removeEventListener('service-disconnected', handleServiceDisconnected as EventListener);
      window.removeEventListener('service-connection-failed', handleServiceConnectionFailed as EventListener);
      window.removeEventListener('service-restarting', handleServiceRestarting as EventListener);
      window.removeEventListener('service-health-check', handleHealthCheck as EventListener);
    };
  }, [
    handleServiceConnected,
    handleServiceDisconnected,
    handleServiceConnectionFailed,
    handleServiceRestarting,
    handleHealthCheck,
  ]);

  // Reconnect to service
  const reconnect = useCallback(async () => {
    try {
      setConnectionState(prev => ({
        ...prev,
        reconnecting: true,
        error: null,
        reconnectAttempts: prev.reconnectAttempts + 1,
      }));

      // Call the PhantomVault service restart API
      if (window.phantomVault?.service?.restart) {
        const result = await window.phantomVault.service.restart();
        if (result.success) {
          handleServiceConnected(new CustomEvent('service-connected', { detail: {} }));
        } else {
          throw new Error(result.error || 'Service restart failed');
        }
      } else {
        // Simulate reconnection for development
        await new Promise(resolve => setTimeout(resolve, 2000));
        handleServiceConnected(new CustomEvent('service-connected', { detail: {} }));
      }
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Reconnection failed';
      setConnectionState(prev => ({
        ...prev,
        reconnecting: false,
        error: errorMessage,
      }));
      throw error;
    }
  }, [handleServiceConnected]);

  // Disconnect from service
  const disconnect = useCallback(async () => {
    try {
      // Call the PhantomVault service disconnect API (not implemented, just simulate)
      if (window.phantomVault?.service?.getStatus) {
        // Just simulate disconnection for now
        handleServiceDisconnected(new CustomEvent('service-disconnected', { detail: {} }));
      } else {
        // Simulate disconnection for development
        handleServiceDisconnected(new CustomEvent('service-disconnected', { detail: {} }));
      }
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Disconnection failed';
      setConnectionState(prev => ({
        ...prev,
        error: errorMessage,
      }));
      throw error;
    }
  }, [handleServiceDisconnected]);

  // Check service health
  const checkHealth = useCallback(async () => {
    try {
      // In a real implementation, this would call the Electron API
      if (window.electronAPI?.invoke) {
        const healthData = await window.electronAPI.invoke('service:health-check');
        handleHealthCheck(new CustomEvent('service-health-check', { detail: healthData }));
      }
    } catch (error) {
      console.error('Health check failed:', error);
    }
  }, [handleHealthCheck]);

  // Register connection change callback
  const onConnectionChange = useCallback((callback: (connected: boolean) => void) => {
    connectionCallbacksRef.current.add(callback);
    
    // Call immediately with current state
    callback(connectionState.connected);
    
    // Return cleanup function
    return () => {
      connectionCallbacksRef.current.delete(callback);
    };
  }, [connectionState.connected]);

  // Register health change callback
  const onHealthChange = useCallback((callback: (health: ServiceConnectionState['health']) => void) => {
    healthCallbacksRef.current.add(callback);
    
    // Call immediately with current state
    callback(connectionState.health);
    
    // Return cleanup function
    return () => {
      healthCallbacksRef.current.delete(callback);
    };
  }, [connectionState.health]);

  // Derived state
  const isConnected = connectionState.connected;
  const isHealthy = connectionState.connected && connectionState.health.status === 'healthy';

  return {
    // State
    connectionState,
    isConnected,
    isHealthy,
    
    // Actions
    reconnect,
    disconnect,
    checkHealth,
    
    // Event handlers
    onConnectionChange,
    onHealthChange,
  };
}