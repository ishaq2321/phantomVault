/**
 * Activity View Component
 */

import React, { useState, useEffect } from 'react';

interface ActivityEntry {
  id: string;
  timestamp: string;
  type: 'vault_lock' | 'vault_unlock' | 'hotkey_press' | 'service_start' | 'error';
  message: string;
  details?: string;
}

export const Activity: React.FC = () => {
  const [activities, setActivities] = useState<ActivityEntry[]>([]);
  const [filter, setFilter] = useState<string>('all');

  useEffect(() => {
    // Load real activity data here
    // For now, start with empty array - will be populated by real service
    setActivities([]);
  }, []);

  const getTypeIcon = (type: string) => {
    switch (type) {
      case 'vault_lock': return '🔒';
      case 'vault_unlock': return '🔓';
      case 'hotkey_press': return '⌨️';
      case 'service_start': return '🚀';
      case 'error': return '❌';
      default: return '📝';
    }
  };

  const getTypeColor = (type: string) => {
    switch (type) {
      case 'vault_lock': return 'var(--color-error)';
      case 'vault_unlock': return 'var(--color-success)';
      case 'hotkey_press': return 'var(--color-info)';
      case 'service_start': return 'var(--color-primary)';
      case 'error': return 'var(--color-error)';
      default: return 'var(--color-text-secondary)';
    }
  };

  const filteredActivities = filter === 'all' 
    ? activities 
    : activities.filter(a => a.type === filter);

  return (
    <div style={{ padding: '2rem', height: '100%' }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '2rem' }}>
        <h1 style={{ margin: 0, color: 'var(--color-text-primary)' }}>📊 Activity Monitor</h1>
        
        <div style={{ display: 'flex', gap: '1rem', alignItems: 'center' }}>
          <select 
            value={filter} 
            onChange={(e) => setFilter(e.target.value)}
            style={{
              padding: '0.5rem',
              border: '1px solid var(--color-border)',
              borderRadius: '4px',
              background: 'var(--color-bg-primary)',
              color: 'var(--color-text-primary)'
            }}
          >
            <option value="all">All Activities</option>
            <option value="vault_lock">Vault Locks</option>
            <option value="vault_unlock">Vault Unlocks</option>
            <option value="hotkey_press">Hotkey Events</option>
            <option value="service_start">Service Events</option>
          </select>
          
          <button style={{
            padding: '0.5rem 1rem',
            backgroundColor: 'var(--color-primary)',
            color: 'white',
            border: 'none',
            borderRadius: '4px',
            cursor: 'pointer'
          }}>
            🔄 Refresh
          </button>
        </div>
      </div>

      <div style={{ 
        display: 'grid', 
        gridTemplateColumns: 'repeat(auto-fit, minmax(300px, 1fr))', 
        gap: '1rem',
        marginBottom: '2rem'
      }}>
        <div style={{
          padding: '1.5rem',
          border: '1px solid var(--color-border)',
          borderRadius: '8px',
          backgroundColor: 'var(--color-bg-elevated)',
          textAlign: 'center'
        }}>
          <h3 style={{ margin: '0 0 0.5rem 0', color: 'var(--color-text-primary)' }}>📈 Total Events</h3>
          <p style={{ margin: 0, fontSize: '2rem', fontWeight: 'bold', color: 'var(--color-primary)' }}>
            0
          </p>
        </div>
        
        <div style={{
          padding: '1.5rem',
          border: '1px solid var(--color-border)',
          borderRadius: '8px',
          backgroundColor: 'var(--color-bg-elevated)',
          textAlign: 'center'
        }}>
          <h3 style={{ margin: '0 0 0.5rem 0', color: 'var(--color-text-primary)' }}>🔓 Unlocks Today</h3>
          <p style={{ margin: 0, fontSize: '2rem', fontWeight: 'bold', color: 'var(--color-success)' }}>
            0
          </p>
        </div>
        
        <div style={{
          padding: '1.5rem',
          border: '1px solid var(--color-border)',
          borderRadius: '8px',
          backgroundColor: 'var(--color-bg-elevated)',
          textAlign: 'center'
        }}>
          <h3 style={{ margin: '0 0 0.5rem 0', color: 'var(--color-text-primary)' }}>⌨️ Hotkey Presses</h3>
          <p style={{ margin: 0, fontSize: '2rem', fontWeight: 'bold', color: 'var(--color-info)' }}>
            0
          </p>
        </div>
      </div>

      <div style={{
        border: '1px solid var(--color-border)',
        borderRadius: '8px',
        backgroundColor: 'var(--color-bg-elevated)',
        maxHeight: 'calc(100vh - 400px)',
        overflowY: 'auto'
      }}>
        <div style={{
          padding: '1rem',
          borderBottom: '1px solid var(--color-border)',
          backgroundColor: 'var(--color-bg-secondary)'
        }}>
          <h3 style={{ margin: 0, color: 'var(--color-text-primary)' }}>Recent Activity</h3>
        </div>
        
        <div style={{ padding: '1rem' }}>
          {filteredActivities.length === 0 ? (
            <p style={{ textAlign: 'center', color: 'var(--color-text-secondary)', margin: '2rem 0' }}>
              No activities found for the selected filter.
            </p>
          ) : (
            filteredActivities.map(activity => (
              <div key={activity.id} style={{
                display: 'flex',
                alignItems: 'flex-start',
                gap: '1rem',
                padding: '1rem',
                borderBottom: '1px solid var(--color-border-light)',
                transition: 'background-color 0.2s ease'
              }}>
                <span style={{ 
                  fontSize: '1.5rem',
                  color: getTypeColor(activity.type)
                }}>
                  {getTypeIcon(activity.type)}
                </span>
                
                <div style={{ flex: 1 }}>
                  <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start' }}>
                    <h4 style={{ 
                      margin: '0 0 0.25rem 0', 
                      color: 'var(--color-text-primary)',
                      fontSize: '0.875rem',
                      fontWeight: '600'
                    }}>
                      {activity.message}
                    </h4>
                    <span style={{ 
                      fontSize: '0.75rem', 
                      color: 'var(--color-text-secondary)',
                      whiteSpace: 'nowrap'
                    }}>
                      {activity.timestamp}
                    </span>
                  </div>
                  
                  {activity.details && (
                    <p style={{ 
                      margin: 0, 
                      fontSize: '0.75rem', 
                      color: 'var(--color-text-secondary)',
                      lineHeight: '1.4'
                    }}>
                      {activity.details}
                    </p>
                  )}
                </div>
              </div>
            ))
          )}
        </div>
      </div>
    </div>
  );
};