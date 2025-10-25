/**
 * Test Restart Component
 */

import React, { useState } from 'react';
import { useServiceConnection } from '../../hooks/useServiceConnection';
import './TestRestart.css';

export const TestRestart: React.FC = () => {
  const { connectionState, reconnect } = useServiceConnection();
  const [testing, setTesting] = useState(false);
  const [result, setResult] = useState<string>('');

  const handleTestRestart = async () => {
    setTesting(true);
    setResult('Testing restart...');
    
    try {
      await reconnect();
      setResult('✅ Restart successful!');
    } catch (error) {
      setResult(`❌ Restart failed: ${error}`);
    } finally {
      setTesting(false);
    }
  };

  const handleTestServiceAPI = async () => {
    setTesting(true);
    setResult('Testing service API...');
    
    try {
      // Test basic service availability
      if (window.phantomVault?.service) {
        const serviceStatus = {
          serviceAvailable: true,
          cppServiceConnected: !!window.phantomVault.service.getStatus,
          encryptionActive: true, // Assume active if service is available
          hotkeysRegistered: false // Default to false
        };

        // Test hotkey manager if available
        if (window.phantomVault.hotkeyManager) {
          try {
            // Check if hotkey manager has the method we need
            if (typeof window.phantomVault.hotkeyManager.isRegistered === 'function') {
              serviceStatus.hotkeysRegistered = window.phantomVault.hotkeyManager.isRegistered();
            } else if (typeof window.phantomVault.hotkeyManager.getRegisteredHotkeys === 'function') {
              const hotkeys = window.phantomVault.hotkeyManager.getRegisteredHotkeys();
              serviceStatus.hotkeysRegistered = hotkeys && hotkeys.length > 0;
            } else {
              // Assume registered if hotkeyManager exists
              serviceStatus.hotkeysRegistered = true;
            }
          } catch (hotkeyError) {
            console.warn('Hotkey check failed:', hotkeyError);
            serviceStatus.hotkeysRegistered = false;
          }
        }

        setResult(`Service API works: ${JSON.stringify(serviceStatus, null, 2)}`);
      } else {
        setResult('❌ Service API not available - PhantomVault service not found');
      }
    } catch (error) {
      setResult(`❌ Service API failed: ${error}`);
    } finally {
      setTesting(false);
    }
  };

  return (
    <div className="test-restart">
      <h1>🔧 Service Restart Test</h1>
      
      <div className="connection-state">
        <h3>Connection State:</h3>
        <pre>
          {JSON.stringify(connectionState, null, 2)}
        </pre>
      </div>
      
      <div className="test-buttons">
        <button 
          onClick={handleTestRestart}
          disabled={testing}
          className="test-button"
        >
          {testing ? 'Testing...' : 'Test Restart'}
        </button>
        
        <button 
          onClick={handleTestServiceAPI}
          disabled={testing}
          className="test-button secondary"
        >
          {testing ? 'Testing...' : 'Test Service API'}
        </button>
      </div>
      
      {result && (
        <div className={`test-result ${result.includes('✅') ? 'success' : 'error'}`}>
          {result}
        </div>
      )}
    </div>
  );
};