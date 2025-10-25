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
  const handleAddVault = () => {
    // In a real implementation, this would open a vault creation modal
    const vaultName = prompt('Enter vault name:');
    if (vaultName && vaultName.trim()) {
      // This would normally call the vault creation API
      alert(`Creating vault: ${vaultName}\n\nNote: This is a demo. In the real app, this would create an encrypted vault.`);
    }
  };

  // Handle unlock all vaults
  const handleUnlockAll = () => {
    if (vaultState.vaults.length === 0) {
      alert('No vaults to unlock. Create a vault first.');
      return;
    }

    const password = prompt('Enter master password to unlock all vaults:');
    if (password) {
      // This would normally unlock all vaults
      alert(`Unlocking ${vaultState.vaults.length} vault(s)...\n\nNote: This is a demo. In the real app, this would unlock all encrypted vaults.`);
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
                onClick={handleUnlockAll}
                title="Unlock all vaults with master password"
                disabled={vaultStats.total === 0}
              >
                🔓 Unlock All
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