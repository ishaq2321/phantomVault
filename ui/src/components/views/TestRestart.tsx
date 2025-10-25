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
      if (window.phantomVault?.service?.getStatus) {
        const status = await window.phantomVault.service.getStatus();
        setResult(`✅ Service API works: ${JSON.stringify(status, null, 2)}`);
      } else {
        setResult('❌ Service API not available');
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