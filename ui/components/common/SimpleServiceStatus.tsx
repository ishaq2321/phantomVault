/**
 * Simple Service Status Component
 */

import React from 'react';

export interface SimpleServiceStatusProps {
  className?: string;
  showDetails?: boolean;
}

export const SimpleServiceStatus: React.FC<SimpleServiceStatusProps> = ({
  className = '',
  showDetails = false,
}) => {
  return (
    <div className={`service-status connected ${className}`}>
      <div className="status-indicator"></div>
      <span>Service Connected</span>
      {showDetails && (
        <div style={{ fontSize: '12px', marginLeft: '8px' }}>
          C++ Core: Active
        </div>
      )}
    </div>
  );
};