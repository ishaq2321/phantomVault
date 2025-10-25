/**
 * Vaults View Component
 */

import React, { useState, useEffect } from 'react';

interface Vault {
  id: string;
  name: string;
  path: string;
  isLocked: boolean;
  size: string;
  lastModified: string;
}

export const Vaults: React.FC = () => {
  const [vaults, setVaults] = useState<Vault[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    // Simulate loading vaults
    setTimeout(() => {
      setVaults([
        {
          id: '1',
          name: 'Personal Documents',
          path: '/home/user/Documents/Personal',
          isLocked: true,
          size: '2.3 GB',
          lastModified: '2 hours ago'
        },
        {
          id: '2', 
          name: 'Work Projects',
          path: '/home/user/Work/Projects',
          isLocked: false,
          size: '1.8 GB',
          lastModified: '1 day ago'
        },
        {
          id: '3',
          name: 'Photos Archive',
          path: '/home/user/Pictures/Archive',
          isLocked: true,
          size: '5.2 GB',
          lastModified: '3 days ago'
        }
      ]);
      setLoading(false);
    }, 1000);
  }, []);

  if (loading) {
    return (
      <div style={{ padding: '2rem', textAlign: 'center' }}>
        <h1>🗂️ Vault Management</h1>
        <p>Loading vaults...</p>
      </div>
    );
  }

  return (
    <div style={{ padding: '2rem' }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: '2rem' }}>
        <h1>🗂️ Vault Management</h1>
        <button style={{
          padding: '0.75rem 1.5rem',
          backgroundColor: 'var(--color-primary)',
          color: 'white',
          border: 'none',
          borderRadius: '4px',
          cursor: 'pointer'
        }}>
          ➕ Add New Vault
        </button>
      </div>

      <div style={{ 
        display: 'grid', 
        gap: '1rem',
        gridTemplateColumns: 'repeat(auto-fill, minmax(350px, 1fr))'
      }}>
        {vaults.map(vault => (
          <div key={vault.id} style={{
            padding: '1.5rem',
            border: '1px solid var(--color-border)',
            borderRadius: '8px',
            backgroundColor: 'var(--color-bg-elevated)',
            boxShadow: '0 2px 4px rgba(0,0,0,0.1)'
          }}>
            <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start', marginBottom: '1rem' }}>
              <div>
                <h3 style={{ margin: '0 0 0.5rem 0', color: 'var(--color-text-primary)' }}>
                  {vault.name}
                </h3>
                <p style={{ margin: 0, fontSize: '0.875rem', color: 'var(--color-text-secondary)' }}>
                  {vault.path}
                </p>
              </div>
              <span style={{
                padding: '0.25rem 0.75rem',
                borderRadius: '12px',
                fontSize: '0.75rem',
                fontWeight: '500',
                backgroundColor: vault.isLocked ? 'rgba(244, 67, 54, 0.1)' : 'rgba(76, 175, 80, 0.1)',
                color: vault.isLocked ? 'var(--color-error)' : 'var(--color-success)'
              }}>
                {vault.isLocked ? '🔒 Locked' : '🔓 Unlocked'}
              </span>
            </div>
            
            <div style={{ marginBottom: '1rem', fontSize: '0.875rem', color: 'var(--color-text-secondary)' }}>
              <p style={{ margin: '0.25rem 0' }}>Size: {vault.size}</p>
              <p style={{ margin: '0.25rem 0' }}>Modified: {vault.lastModified}</p>
            </div>

            <div style={{ display: 'flex', gap: '0.5rem' }}>
              <button style={{
                padding: '0.5rem 1rem',
                backgroundColor: vault.isLocked ? 'var(--color-success)' : 'var(--color-warning)',
                color: 'white',
                border: 'none',
                borderRadius: '4px',
                cursor: 'pointer',
                fontSize: '0.875rem'
              }}>
                {vault.isLocked ? '🔓 Unlock' : '🔒 Lock'}
              </button>
              <button style={{
                padding: '0.5rem 1rem',
                backgroundColor: 'transparent',
                color: 'var(--color-text-secondary)',
                border: '1px solid var(--color-border)',
                borderRadius: '4px',
                cursor: 'pointer',
                fontSize: '0.875rem'
              }}>
                ⚙️ Settings
              </button>
              <button style={{
                padding: '0.5rem 1rem',
                backgroundColor: 'transparent',
                color: 'var(--color-error)',
                border: '1px solid var(--color-error)',
                borderRadius: '4px',
                cursor: 'pointer',
                fontSize: '0.875rem'
              }}>
                🗑️ Delete
              </button>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
};