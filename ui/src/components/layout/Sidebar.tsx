/**
 * Sidebar Component
 * 
 * Navigation sidebar with view switching and quick actions
 */

import React, { useCallback } from 'react';
import { AppView } from '../../App';
import { useVault } from '../../contexts';
import './Sidebar.css';

export interface SidebarProps {
  currentView: AppView;
  collapsed: boolean;
  onViewChange: (view: AppView) => void;
  onToggle: () => void;
  className?: string;
}

interface NavigationItem {
  id: AppView;
  label: string;
  icon: string;
  description: string;
  shortcut: string;
}

const NAVIGATION_ITEMS: NavigationItem[] = [
  {
    id: 'dashboard',
    label: 'Dashboard',
    icon: '🏠',
    description: 'Overview of all vaults',
    shortcut: 'Ctrl+1',
  },
  {
    id: 'vaults',
    label: 'Vaults',
    icon: '🗄️',
    description: 'Manage your vaults',
    shortcut: 'Ctrl+2',
  },
  {
    id: 'activity',
    label: 'Activity',
    icon: '📊',
    description: 'View activity logs',
    shortcut: 'Ctrl+3',
  },
  {
    id: 'settings',
    label: 'Settings',
    icon: '⚙️',
    description: 'Application settings',
    shortcut: 'Ctrl+4',
  },
];

/**
 * Sidebar navigation component
 */
export const Sidebar: React.FC<SidebarProps> = ({
  currentView,
  collapsed,
  onViewChange,
  onToggle,
  className = '',
}) => {
  const { state: vaultState } = useVault();

  // Handle navigation item click
  const handleNavClick = useCallback((view: AppView) => {
    onViewChange(view);
  }, [onViewChange]);

  // Get vault status summary
  const getVaultSummary = () => {
    const total = vaultState.vaults.length;
    const mounted = vaultState.vaults.filter(v => v.status === 'mounted').length;
    const errors = vaultState.vaults.filter(v => v.status === 'error').length;
    
    return { total, mounted, errors };
  };

  const vaultSummary = getVaultSummary();

  return (
    <div className={`sidebar ${collapsed ? 'collapsed' : ''} ${className}`}>
      {/* Sidebar Header */}
      <div className="sidebar-header">
        <div className="app-logo">
          <span className="logo-icon">👻</span>
          {!collapsed && (
            <div className="logo-text">
              <h1>PhantomVault</h1>
              <span className="version">v1.2.1</span>
            </div>
          )}
        </div>
        
        <button 
          onClick={onToggle}
          className="sidebar-toggle\"
          title={collapsed ? 'Expand sidebar' : 'Collapse sidebar'}
        >
          {collapsed ? '▶' : '◀'}
        </button>
      </div>

      {/* Navigation */}
      <nav className="sidebar-nav">
        <ul className="nav-list">
          {NAVIGATION_ITEMS.map(item => (
            <li key={item.id} className="nav-item">
              <button
                onClick={() => handleNavClick(item.id)}
                className={`nav-button ${currentView === item.id ? 'active' : ''}`}
                title={collapsed ? `${item.label} (${item.shortcut})` : item.description}
              >
                <span className="nav-icon">{item.icon}</span>
                {!collapsed && (
                  <>
                    <span className="nav-label">{item.label}</span>
                    <span className="nav-shortcut">{item.shortcut}</span>
                  </>
                )}
                
                {/* Badge for activity count */}
                {item.id === 'vaults' && vaultSummary.errors > 0 && (
                  <span className="nav-badge error">{vaultSummary.errors}</span>
                )}
              </button>
            </li>
          ))}
        </ul>
      </nav>

      {/* Quick Stats */}
      {!collapsed && (
        <div className="sidebar-stats">
          <h3 className="stats-title">Quick Stats</h3>
          <div className="stats-grid">
            <div className="stat-item">
              <span className="stat-icon">🗄️</span>
              <div className="stat-content">
                <span className="stat-value">{vaultSummary.total}</span>
                <span className="stat-label">Total Vaults</span>
              </div>
            </div>
            
            <div className="stat-item">
              <span className="stat-icon">🟢</span>
              <div className="stat-content">
                <span className="stat-value">{vaultSummary.mounted}</span>
                <span className="stat-label">Mounted</span>
              </div>
            </div>
            
            {vaultSummary.errors > 0 && (
              <div className="stat-item error">
                <span className="stat-icon">❌</span>
                <div className="stat-content">
                  <span className="stat-value">{vaultSummary.errors}</span>
                  <span className="stat-label">Errors</span>
                </div>
              </div>
            )}
          </div>
        </div>
      )}

      {/* Quick Actions */}
      {!collapsed && (
        <div className="sidebar-actions">
          <h3 className="actions-title">Quick Actions</h3>
          <div className="action-buttons">
            <button 
              className="action-button primary\"
              onClick={() => onViewChange('vaults')}
              title="Create a new vault\"
            >
              <span className="action-icon">➕</span>
              <span className="action-label">New Vault</span>
            </button>
            
            <button 
              className="action-button secondary\"
              onClick={() => onViewChange('activity')}
              title="View recent activity\"
            >
              <span className="action-icon">📊</span>
              <span className="action-label">View Activity</span>
            </button>
          </div>
        </div>
      )}

      {/* Sidebar Footer */}
      <div className="sidebar-footer">
        {!collapsed && (
          <div className="footer-content">
            <div className="system-info">
              <span className="info-item">
                <span className="info-icon">💾</span>
                <span className="info-text">Memory: 45MB</span>
              </span>
              <span className="info-item">
                <span className="info-icon">⚡</span>
                <span className="info-text">Status: Active</span>
              </span>
            </div>
          </div>
        )}
        
        <div className="footer-actions">
          <button 
            className="footer-button\"
            title="Help & Support\"
            onClick={() => {
              // In a real implementation, this would open help
              console.log('Opening help...');
            }}
          >
            {collapsed ? '❓' : '❓ Help'}
          </button>
        </div>
      </div>
    </div>
  );
};