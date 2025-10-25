/**
 * Dashboard View Component
 * 
 * Main dashboard view for the application
 */

import React from 'react';
import { TestRestart } from './TestRestart';

export const Dashboard: React.FC = () => {
  return (
    <div style={{ padding: '2rem' }}>
      <h1>🔐 PhantomVault Dashboard</h1>
      <p>Advanced dashboard is loading...</p>
      
      <div style={{ 
        display: 'grid', 
        gridTemplateColumns: 'repeat(auto-fit, minmax(300px, 1fr))', 
        gap: '1rem',
        marginTop: '2rem'
      }}>
        <div style={{ 
          padding: '1rem', 
          border: '1px solid #ddd', 
          borderRadius: '8px',
          background: '#f9f9f9'
        }}>
          <h3>📊 Vault Statistics</h3>
          <p>Total Vaults: 0</p>
          <p>Locked: 0</p>
          <p>Unlocked: 0</p>
        </div>
        
        <div style={{ 
          padding: '1rem', 
          border: '1px solid #ddd', 
          borderRadius: '8px',
          background: '#f9f9f9'
        }}>
          <h3>🔧 Service Status</h3>
          <p>C++ Service: Connected</p>
          <p>Encryption: Active</p>
          <p>Hotkeys: Registered</p>
        </div>
        
        <div style={{ 
          padding: '1rem', 
          border: '1px solid #ddd', 
          borderRadius: '8px',
          background: '#f9f9f9'
        }}>
          <h3>⚡ Quick Actions</h3>
          <button style={{ 
            padding: '0.5rem 1rem', 
            margin: '0.25rem',
            border: '1px solid #007bff',
            background: '#007bff',
            color: 'white',
            borderRadius: '4px',
            cursor: 'pointer'
          }}>
            Add Vault
          </button>
          <button style={{ 
            padding: '0.5rem 1rem', 
            margin: '0.25rem',
            border: '1px solid #28a745',
            background: '#28a745',
            color: 'white',
            borderRadius: '4px',
            cursor: 'pointer'
          }}>
            Unlock All
          </button>
        </div>
      </div>
      
      <TestRestart />
    </div>
  );
};