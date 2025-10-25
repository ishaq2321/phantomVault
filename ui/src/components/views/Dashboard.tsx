/**
 * Dashboard View Component
 * 
 * Main dashboard view for the application
 */

import React from 'react';
import { TestRestart } from './TestRestart';
import './Dashboard.css';

export const Dashboard: React.FC = () => {
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
            <p><strong>Total Vaults:</strong> 0</p>
            <p><strong>Locked:</strong> 0</p>
            <p><strong>Unlocked:</strong> 0</p>
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
              <button className="action-button">
                ➕ Add Vault
              </button>
              <button className="action-button secondary">
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