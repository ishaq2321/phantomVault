/**
 * Test Restart Component
 */

import React, { useState } from 'react';
import { useServiceConnection } from '../../hooks/useServiceConnection';

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
    <div style={{ padding: '2rem' }}>
      <h1>🔧 Service Restart Test</h1>
      
      <div style={{ marginBottom: '2rem' }}>
        <h3>Connection State:</h3>
        <pre style={{ background: '#f5f5f5', padding: '1rem', borderRadius: '4px' }}>
          {JSON.stringify(connectionState, null, 2)}
        </pre>
      </div>
      
      <div style={{ marginBottom: '2rem' }}>
        <button 
          onClick={handleTestRestart}
          disabled={testing}
          style={{
            padding: '0.75rem 1.5rem',
            marginRight: '1rem',
            backgroundColor: '#007bff',
            color: 'white',
            border: 'none',
            borderRadius: '4px',
            cursor: testing ? 'not-allowed' : 'pointer',
            opacity: testing ? 0.6 : 1
          }}
        >
          {testing ? 'Testing...' : 'Test Restart'}
        </button>
        
        <button 
          onClick={handleTestServiceAPI}
          disabled={testing}
          style={{
            padding: '0.75rem 1.5rem',
            backgroundColor: '#28a745',
            color: 'white',
            border: 'none',
            borderRadius: '4px',
            cursor: testing ? 'not-allowed' : 'pointer',
            opacity: testing ? 0.6 : 1
          }}
        >
          {testing ? 'Testing...' : 'Test Service API'}
        </button>
      </div>
      
      {result && (
        <div style={{ 
          padding: '1rem', 
          background: result.includes('✅') ? '#d4edda' : '#f8d7da',
          border: `1px solid ${result.includes('✅') ? '#c3e6cb' : '#f5c6cb'}`,
          borderRadius: '4px',
          whiteSpace: 'pre-wrap'
        }}>
          {result}
        </div>
      )}
    </div>
  );
};