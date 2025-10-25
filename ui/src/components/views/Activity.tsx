/**
 * Activity View Component
 */

import React from 'react';
import { ActivityMonitor } from '../../../components/activity-monitor/ActivityMonitor';

export const Activity: React.FC = () => {
  return (
    <div style={{ padding: '1rem', height: '100%' }}>
      <ActivityMonitor 
        maxEntries={1000}
        autoScroll={true}
        showFilters={true}
        showStats={true}
        height="calc(100vh - 200px)"
      />
    </div>
  );
};