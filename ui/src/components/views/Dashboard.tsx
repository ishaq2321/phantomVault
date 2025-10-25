/**
 * Dashboard View Component
 * 
 * Main dashboard view for the application
 */

import React from 'react';
import { TestRestart } from './TestRestart';
import { useVault } from '../../contexts';
import './Dashboard.css';

export const Dashboard: React.FC = () => {
  const { state: vaultState, actions: vaultActions } = useVault();



  // Handle add vault
  const handleAddVault = async () => {
    const vaultName = prompt('Enter vault name:');
    if (!vaultName || !vaultName.trim()) {
      return;
    }

    const vaultPath = prompt('Enter vault path (folder to encrypt):');
    if (!vaultPath || !vaultPath.trim()) {
      return;
    }

    try {
      const result = await vaultActions.createVault({
        name: vaultName.trim(),
        path: vaultPath.trim(),
        password: '', // Will be handled by the system
      });

      if (result.success) {
        alert(`✅ Vault "${vaultName}" created successfully!`);
      } else {
        alert(`❌ Failed to create vault: ${result.error || result.message}`);
      }
    } catch (error) {
      alert(`❌ Error creating vault: ${error}`);
    }
  };



  // Get real vault statistics
  const getVaultStats = () => {
    const total = vaultState.vaults.length;
    const locked = vaultState.vaults.filter(v => v.status === 'unmounted').length;
    const unlocked = vaultState.vaults.filter(v => v.status === 'mounted').length;
    
    return { total, locked, unlocked };
  };

  const vaultStats = getVaultStats();

  return (
    <div className="dashboard">
      <h1 className="dashboard-title">🔐 PhantomVault Dashboard</h1>
      <p className="dashboard-subtitle">Secure folder management with invisible encryption</p>
      
      <div className="dashboard-grid">
        <div className="dashboard-card">
          <h3 className="card-title">
            📊 Vault Statistics
          </h3>
          <div className="card-content">
            <p><strong>Total Vaults:</strong> {vaultStats.total}</p>
            <p><strong>Locked:</strong> {vaultStats.locked}</p>
            <p><strong>Unlocked:</strong> {vaultStats.unlocked}</p>
          </div>
        </div>
        
        <div className="dashboard-card">
          <h3 className="card-title">
            🔧 Service Status
          </h3>
          <div className="card-content">
            <div className="service-status-list">
              <div className="service-status-item">
                <span className="service-label">C++ Service:</span>
                <span className="status-indicator connected">
                  <span className="status-dot"></span>
                  Connected
                </span>
              </div>
              <div className="service-status-item">
                <span className="service-label">Encryption:</span>
                <span className="status-indicator active">
                  <span className="status-dot"></span>
                  Active
                </span>
              </div>
              <div className="service-status-item">
                <span className="service-label">Hotkeys:</span>
                <span className="status-indicator active">
                  <span className="status-dot"></span>
                  Registered
                </span>
              </div>
            </div>
          </div>
        </div>
        
        <div className="dashboard-card">
          <h3 className="card-title">
            ⚡ Quick Actions
          </h3>
          <div className="card-content">
            <div className="quick-actions">
              <button 
                className="action-button"
                onClick={handleAddVault}
                title="Create a new encrypted vault"
              >
                ➕ Add Vault
              </button>
              <button 
                className="action-button secondary"
                onClick={() => alert('💡 Tip: Use the sidebar to navigate to Vaults for individual vault management.\n\n🔐 For security, vaults must be managed individually.')}
                title="Vault management info"
              >
                ℹ️ Vault Info
              </button>
            </div>
          </div>
        </div>
      </div>
      
      <div className="test-section">
        <TestRestart />
      </div>
    </div>
  );
};