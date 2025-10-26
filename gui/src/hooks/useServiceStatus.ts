/**
 * PhantomVault Service Status Hook
 */

import { useState, useEffect } from 'react';

interface ServiceStatus {
  running: boolean;
  pid: number | null;
}

export const useServiceStatus = () => {
  const [serviceStatus, setServiceStatus] = useState<ServiceStatus>({
    running: false,
    pid: null,
  });
  const [isConnected, setIsConnected] = useState(false);

  useEffect(() => {
    const checkServiceStatus = async () => {
      try {
        if (window.phantomVault?.service) {
          const status = await window.phantomVault.service.getStatus();
          setServiceStatus(status);
          setIsConnected(true);
        } else {
          setIsConnected(false);
        }
      } catch (error) {
        console.error('[useServiceStatus] Failed to get service status:', error);
        setIsConnected(false);
      }
    };

    // Check immediately
    checkServiceStatus();

    // Check every 5 seconds
    const interval = setInterval(checkServiceStatus, 5000);

    return () => clearInterval(interval);
  }, []);

  return { serviceStatus, isConnected };
};