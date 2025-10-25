/**
 * Settings View Component
 */

import React from 'react';
import { Settings as AdvancedSettings } from '../../../components/settings/Settings';

export const Settings: React.FC = () => {
  return (
    <div style={{ padding: '1rem', height: '100%' }}>
      <AdvancedSettings />
    </div>
  );
};