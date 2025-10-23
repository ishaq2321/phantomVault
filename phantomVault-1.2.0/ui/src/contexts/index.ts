/**
 * Contexts Index
 * 
 * Central export point for all React contexts and providers
 */

// ==================== CONTEXT EXPORTS ====================

export { default as VaultContext, VaultProvider, useVault } from './VaultContext';
export { default as AppContext, AppProvider, useApp } from './AppContext';

// ==================== COMBINED PROVIDER ====================

import React from 'react';
import { VaultProvider } from './VaultContext';
import { AppProvider } from './AppContext';

/**
 * Combined provider that wraps the entire application
 * Provides all contexts in the correct order
 */
interface ProvidersProps {
  children: React.ReactNode;
}

export const Providers: React.FC<ProvidersProps> = ({ children }) => {
  return (
    <AppProvider>
      <VaultProvider>
        {children}
      </VaultProvider>
    </AppProvider>
  );
};

// ==================== CONTEXT HOOKS ====================

/**
 * Combined hook that provides access to all contexts
 * Useful for components that need multiple contexts
 */
export const useContexts = () => {
  const app = useApp();
  const vault = useVault();
  
  return {
    app,
    vault,
  };
};

// ==================== TYPE EXPORTS ====================

export type {
  VaultContextState,
  VaultContextActions,
  AppContextState,
  AppContextActions,
} from '../types';