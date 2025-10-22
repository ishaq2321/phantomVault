import React from 'react';
import { Icon } from '../common/Icon';
import { Button } from '../common/Button';
import { formatBytes, formatDate } from '../../utils/formatters';
// import './StatusOverview.scss';

interface StatusOverviewProps {
  folders: Array<{
    id: string;
    name: string;
    path: string;
    isSecure: boolean;
    isLocked: boolean;
    lastModified: Date;
    size: number;
  }>;
}

export const StatusOverview: React.FC<StatusOverviewProps> = ({ folders }) => {
  const totalFolders = folders.length;
  const securedFolders = folders.filter((f) => f.isSecure).length;
  const lockedFolders = folders.filter((f) => f.isLocked).length;
  const totalSize = folders.reduce((acc, f) => acc + f.size, 0);

  const securityChecks = [
    {
      id: 'encryption',
      name: 'Encryption Status',
      status: securedFolders === totalFolders ? 'secure' : 'warning',
      icon: securedFolders === totalFolders ? 'shield-check' : 'shield-warning',
      message:
        securedFolders === totalFolders
          ? 'All folders are encrypted'
          : `${totalFolders - securedFolders} folders need encryption`,
    },
    {
      id: 'locks',
      name: 'Lock Status',
      status: lockedFolders === totalFolders ? 'secure' : 'warning',
      icon: lockedFolders === totalFolders ? 'lock' : 'unlock',
      message:
        lockedFolders === totalFolders
          ? 'All folders are locked'
          : `${totalFolders - lockedFolders} folders are unlocked`,
    },
    {
      id: 'backup',
      name: 'Backup Status',
      status: 'secure',
      icon: 'cloud-check',
      message: 'Last backup: 2 hours ago',
    },
  ];

  // Sort folders by last modified date for recent activity
  const recentActivity = [...folders]
    .sort((a, b) => b.lastModified.getTime() - a.lastModified.getTime())
    .slice(0, 5);

  return (
    <div>
      <div style={{ marginBottom: '2rem' }}>
        <h2 style={{ fontSize: '1.25rem', margin: '0 0 0.25rem', fontWeight: '600' }}>
          Security Overview
        </h2>
        <p style={{ fontSize: '0.875rem', color: '#B4B4B4', margin: 0 }}>
          Monitor your vault's security status
        </p>
      </div>

      <div style={{
        display: 'grid',
        gridTemplateColumns: 'repeat(2, 1fr)',
        gap: '1rem',
        marginBottom: '2rem',
      }}>
        <div style={{
          padding: '1rem',
          backgroundColor: '#1B1F3B',
          borderRadius: '8px',
          textAlign: 'center',
        }}>
          <div style={{ fontSize: '1.5rem', marginBottom: '0.5rem' }}>📁</div>
          <div style={{ fontSize: '2rem', fontWeight: '700', margin: '0.25rem 0' }}>
            {totalFolders}
          </div>
          <div style={{ fontSize: '0.75rem', color: '#B4B4B4' }}>Total Folders</div>
        </div>

        <div style={{
          padding: '1rem',
          backgroundColor: '#1B1F3B',
          borderRadius: '8px',
          textAlign: 'center',
          border: securedFolders === totalFolders ? '1px solid #4CAF50' : '1px solid #FF9800',
        }}>
          <div style={{ fontSize: '1.5rem', marginBottom: '0.5rem' }}>
            {securedFolders === totalFolders ? '🛡️' : '⚠️'}
          </div>
          <div style={{
            fontSize: '2rem',
            fontWeight: '700',
            margin: '0.25rem 0',
            color: securedFolders === totalFolders ? '#4CAF50' : '#FF9800',
          }}>
            {securedFolders}
          </div>
          <div style={{ fontSize: '0.75rem', color: '#B4B4B4' }}>Secured Folders</div>
        </div>

        <div style={{
          padding: '1rem',
          backgroundColor: '#1B1F3B',
          borderRadius: '8px',
          textAlign: 'center',
          border: lockedFolders === totalFolders ? '1px solid #4CAF50' : '1px solid #FF9800',
        }}>
          <div style={{ fontSize: '1.5rem', marginBottom: '0.5rem' }}>
            {lockedFolders === totalFolders ? '🔒' : '🔓'}
          </div>
          <div style={{
            fontSize: '2rem',
            fontWeight: '700',
            margin: '0.25rem 0',
            color: lockedFolders === totalFolders ? '#4CAF50' : '#FF9800',
          }}>
            {lockedFolders}
          </div>
          <div style={{ fontSize: '0.75rem', color: '#B4B4B4' }}>Locked Folders</div>
        </div>

        <div style={{
          padding: '1rem',
          backgroundColor: '#1B1F3B',
          borderRadius: '8px',
          textAlign: 'center',
        }}>
          <div style={{ fontSize: '1.5rem', marginBottom: '0.5rem' }}>💾</div>
          <div style={{ fontSize: '1.25rem', fontWeight: '700', margin: '0.25rem 0' }}>
            {formatBytes(totalSize)}
          </div>
          <div style={{ fontSize: '0.75rem', color: '#B4B4B4' }}>Total Size</div>
        </div>
      </div>

      <div style={{ marginBottom: '2rem' }}>
        <div style={{
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'space-between',
          marginBottom: '1rem',
        }}>
          <h3 style={{ fontSize: '1rem', margin: 0, fontWeight: '600' }}>Security Status</h3>
          <button style={{
            padding: '0.25rem 0.75rem',
            backgroundColor: 'transparent',
            color: '#F6F6F6',
            border: '1px solid #424769',
            borderRadius: '6px',
            fontSize: '0.75rem',
            cursor: 'pointer',
            display: 'flex',
            alignItems: 'center',
            gap: '0.25rem',
          }}>
            <span>🔄</span>
            Refresh
          </button>
        </div>

        <div style={{
          display: 'flex',
          flexDirection: 'column',
          gap: '0.75rem',
        }}>
          {securityChecks.map((check) => (
            <div
              key={check.id}
              style={{
                padding: '0.75rem',
                backgroundColor: '#1B1F3B',
                borderRadius: '8px',
                display: 'flex',
                alignItems: 'flex-start',
                gap: '0.75rem',
                border: check.status === 'secure' ? '1px solid #4CAF50' : '1px solid #FF9800',
              }}
            >
              <div style={{ fontSize: '1.25rem' }}>
                {check.icon === 'shield-check' ? '🛡️' :
                 check.icon === 'shield-warning' ? '⚠️' :
                 check.icon === 'lock' ? '🔒' :
                 check.icon === 'unlock' ? '🔓' : '☁️'}
              </div>
              <div style={{ flex: 1 }}>
                <div style={{ fontSize: '0.875rem', fontWeight: '500', marginBottom: '0.25rem' }}>
                  {check.name}
                </div>
                <div style={{
                  fontSize: '0.75rem',
                  color: check.status === 'secure' ? '#4CAF50' : '#FF9800',
                }}>
                  {check.message}
                </div>
              </div>
            </div>
          ))}
        </div>
      </div>

      <div>
        <div style={{
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'space-between',
          marginBottom: '1rem',
        }}>
          <h3 style={{ fontSize: '1rem', margin: 0, fontWeight: '600' }}>Recent Activity</h3>
          <button style={{
            padding: '0.25rem 0.75rem',
            backgroundColor: 'transparent',
            color: '#7077A1',
            border: 'none',
            fontSize: '0.75rem',
            cursor: 'pointer',
            fontWeight: '500',
          }}>
            View All
          </button>
        </div>

        <div style={{
          display: 'flex',
          flexDirection: 'column',
          gap: '0.75rem',
        }}>
          {recentActivity.length > 0 ? recentActivity.map((folder) => (
            <div
              key={folder.id}
              style={{
                padding: '0.75rem',
                backgroundColor: '#1B1F3B',
                borderRadius: '8px',
                display: 'flex',
                alignItems: 'center',
                gap: '0.75rem',
              }}
            >
              <div style={{ fontSize: '1.25rem' }}>
                {folder.isSecure ? '🔐' : '📁'}
              </div>
              <div style={{ flex: 1, minWidth: 0 }}>
                <div style={{
                  fontSize: '0.875rem',
                  fontWeight: '500',
                  marginBottom: '0.25rem',
                  overflow: 'hidden',
                  textOverflow: 'ellipsis',
                  whiteSpace: 'nowrap',
                }}>
                  {folder.name}
                </div>
                <div style={{
                  fontSize: '0.75rem',
                  color: '#B4B4B4',
                  display: 'flex',
                  gap: '0.5rem',
                }}>
                  <span>{formatDate(folder.lastModified)}</span>
                  <span>•</span>
                  <span>{formatBytes(folder.size)}</span>
                </div>
              </div>
            </div>
          )) : (
            <div style={{
              padding: '2rem 1rem',
              textAlign: 'center',
              color: '#B4B4B4',
              fontSize: '0.875rem',
            }}>
              No recent activity
            </div>
          )}
        </div>
      </div>
    </div>
  );
}; 