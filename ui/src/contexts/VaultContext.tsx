/**
 * Vault Context
 * 
 * React Context for managing vault-related state throughout the application
 * Provides centralized state management for vaults, profiles, and vault operations
 */

import React, { createContext, useContext, useReducer, useCallback, useEffect } from 'react';
import {
  VaultInfo,
  VaultProfile,
  VaultConfig,
  VaultOperationResult,
  AppError,
  VaultContextState,
  VaultContextActions,
  IPCMessageType
} from '../types';

// ==================== CONTEXT STATE ====================

interface VaultState extends VaultContextState {
  // Additional internal state
  lastUpdated: Date | null;
  operationInProgress: boolean;
}

// ==================== ACTIONS ====================

type VaultAction =
  | { type: 'SET_LOADING'; payload: boolean }
  | { type: 'SET_ERROR'; payload: AppError | null }
  | { type: 'SET_VAULTS'; payload: VaultInfo[] }
  | { type: 'ADD_VAULT'; payload: VaultInfo }
  | { type: 'UPDATE_VAULT'; payload: { id: string; updates: Partial<VaultInfo> } }
  | { type: 'REMOVE_VAULT'; payload: string }
  | { type: 'SELECT_VAULT'; payload: string | null }
  | { type: 'SET_PROFILES'; payload: VaultProfile[] }
  | { type: 'SET_ACTIVE_PROFILE'; payload: VaultProfile | null }
  | { type: 'ADD_PROFILE'; payload: VaultProfile }
  | { type: 'SET_OPERATION_IN_PROGRESS'; payload: boolean }
  | { type: 'CLEAR_ERROR' };

// ==================== REDUCER ====================

const initialState: VaultState = {
  vaults: [],
  selectedVault: null,
  profiles: [],
  activeProfile: null,
  loading: false,
  error: null,
  lastUpdated: null,
  operationInProgress: false,
};

function vaultReducer(state: VaultState, action: VaultAction): VaultState {
  switch (action.type) {
    case 'SET_LOADING':
      return {
        ...state,
        loading: action.payload,
      };

    case 'SET_ERROR':
      return {
        ...state,
        error: action.payload,
        loading: false,
        operationInProgress: false,
      };

    case 'SET_VAULTS':
      return {
        ...state,
        vaults: action.payload,
        lastUpdated: new Date(),
        loading: false,
      };

    case 'ADD_VAULT':
      return {
        ...state,
        vaults: [...state.vaults, action.payload],
        lastUpdated: new Date(),
      };

    case 'UPDATE_VAULT': {
      const updatedVaults = state.vaults.map(vault =>
        vault.id === action.payload.id
          ? { ...vault, ...action.payload.updates }
          : vault
      );
      
      // Update selected vault if it's the one being updated
      const selectedVault = state.selectedVault?.id === action.payload.id
        ? updatedVaults.find(v => v.id === action.payload.id) || null
        : state.selectedVault;

      return {
        ...state,
        vaults: updatedVaults,
        selectedVault,
        lastUpdated: new Date(),
      };
    }

    case 'REMOVE_VAULT': {
      const filteredVaults = state.vaults.filter(vault => vault.id !== action.payload);
      const selectedVault = state.selectedVault?.id === action.payload ? null : state.selectedVault;

      return {
        ...state,
        vaults: filteredVaults,
        selectedVault,
        lastUpdated: new Date(),
      };
    }

    case 'SELECT_VAULT': {
      const vault = action.payload 
        ? state.vaults.find(v => v.id === action.payload) || null
        : null;

      return {
        ...state,
        selectedVault: vault,
      };
    }

    case 'SET_PROFILES':
      return {
        ...state,
        profiles: action.payload,
        lastUpdated: new Date(),
      };

    case 'SET_ACTIVE_PROFILE':
      return {
        ...state,
        activeProfile: action.payload,
        lastUpdated: new Date(),
      };

    case 'ADD_PROFILE':
      return {
        ...state,
        profiles: [...state.profiles, action.payload],
        lastUpdated: new Date(),
      };

    case 'SET_OPERATION_IN_PROGRESS':
      return {
        ...state,
        operationInProgress: action.payload,
      };

    case 'CLEAR_ERROR':
      return {
        ...state,
        error: null,
      };

    default:
      return state;
  }
}

// ==================== CONTEXT CREATION ====================

interface VaultContextValue {
  state: VaultContextState;
  actions: VaultContextActions;
}

const VaultContext = createContext<VaultContextValue | undefined>(undefined);

// ==================== PROVIDER COMPONENT ====================

interface VaultProviderProps {
  children: React.ReactNode;
}

export const VaultProvider: React.FC<VaultProviderProps> = ({ children }) => {
  const [state, dispatch] = useReducer(vaultReducer, initialState);

  // ==================== ACTION IMPLEMENTATIONS ====================

  const loadVaults = useCallback(async (): Promise<void> => {
    try {
      dispatch({ type: 'SET_LOADING', payload: true });
      dispatch({ type: 'CLEAR_ERROR' });

      // Use the PhantomVault API to get vault list
      const response = await window.phantomVault.folder.getAll(
        state.activeProfile?.id || ''
      );

      if (response.success && response.folders) {
        // Transform folder data to VaultInfo format
        const vaults: VaultInfo[] = response.folders.map(folder => ({
          id: folder.id,
          name: folder.folder_name || folder.name,
          path: folder.original_path,
          status: folder.is_locked ? 'unmounted' : 'mounted',
          lastAccess: new Date(), // TODO: Get actual last access time
          size: 0, // TODO: Calculate folder size
          folderCount: 1,
          profile: state.activeProfile!,
        }));

        dispatch({ type: 'SET_VAULTS', payload: vaults });
      } else {
        throw new Error(response.error || 'Failed to load vaults');
      }
    } catch (error) {
      const appError: AppError = {
        type: 'system',
        code: 'LOAD_VAULTS_FAILED',
        message: error instanceof Error ? error.message : 'Failed to load vaults',
        timestamp: new Date(),
        recoverable: true,
      };
      dispatch({ type: 'SET_ERROR', payload: appError });
    }
  }, [state.activeProfile]);

  const selectVault = useCallback((vaultId: string | null): void => {
    dispatch({ type: 'SELECT_VAULT', payload: vaultId });
  }, []);

  const createVault = useCallback(async (config: VaultConfig): Promise<VaultOperationResult> => {
    try {
      dispatch({ type: 'SET_OPERATION_IN_PROGRESS', payload: true });
      dispatch({ type: 'CLEAR_ERROR' });

      if (!state.activeProfile) {
        throw new Error('No active profile selected');
      }

      // Add folder to the active profile
      const response = await window.phantomVault.folder.add(
        state.activeProfile.id,
        config.path,
        config.name
      );

      if (response.success && response.folder) {
        const newVault: VaultInfo = {
          id: response.folderId || response.folder.id,
          name: config.name,
          path: config.path,
          status: 'unmounted',
          lastAccess: new Date(),
          size: 0,
          folderCount: 1,
          profile: state.activeProfile,
        };

        dispatch({ type: 'ADD_VAULT', payload: newVault });

        return {
          success: true,
          message: 'Vault created successfully',
          vaultId: newVault.id,
        };
      } else {
        throw new Error(response.error || 'Failed to create vault');
      }
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to create vault';
      const appError: AppError = {
        type: 'system',
        code: 'CREATE_VAULT_FAILED',
        message: errorMessage,
        timestamp: new Date(),
        recoverable: true,
      };
      dispatch({ type: 'SET_ERROR', payload: appError });

      return {
        success: false,
        message: errorMessage,
        error: errorMessage,
      };
    } finally {
      dispatch({ type: 'SET_OPERATION_IN_PROGRESS', payload: false });
    }
  }, [state.activeProfile]);

  const updateVault = useCallback(async (
    vaultId: string, 
    config: Partial<VaultConfig>
  ): Promise<VaultOperationResult> => {
    try {
      dispatch({ type: 'SET_OPERATION_IN_PROGRESS', payload: true });
      dispatch({ type: 'CLEAR_ERROR' });

      // Update vault in local state
      // Note: In a full implementation, this would also update the backend
      dispatch({
        type: 'UPDATE_VAULT',
        payload: {
          id: vaultId,
          updates: {
            name: config.name,
            path: config.path,
          },
        },
      });

      return {
        success: true,
        message: 'Vault updated successfully',
        vaultId,
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to update vault';
      const appError: AppError = {
        type: 'system',
        code: 'UPDATE_VAULT_FAILED',
        message: errorMessage,
        timestamp: new Date(),
        recoverable: true,
      };
      dispatch({ type: 'SET_ERROR', payload: appError });

      return {
        success: false,
        message: errorMessage,
        error: errorMessage,
      };
    } finally {
      dispatch({ type: 'SET_OPERATION_IN_PROGRESS', payload: false });
    }
  }, []);

  const deleteVault = useCallback(async (vaultId: string): Promise<VaultOperationResult> => {
    try {
      dispatch({ type: 'SET_OPERATION_IN_PROGRESS', payload: true });
      dispatch({ type: 'CLEAR_ERROR' });

      if (!state.activeProfile) {
        throw new Error('No active profile selected');
      }

      // Remove folder from the active profile
      const response = await window.phantomVault.folder.remove(
        state.activeProfile.id,
        vaultId
      );

      if (response.success) {
        dispatch({ type: 'REMOVE_VAULT', payload: vaultId });

        return {
          success: true,
          message: 'Vault deleted successfully',
          vaultId,
        };
      } else {
        throw new Error(response.error || 'Failed to delete vault');
      }
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to delete vault';
      const appError: AppError = {
        type: 'system',
        code: 'DELETE_VAULT_FAILED',
        message: errorMessage,
        timestamp: new Date(),
        recoverable: true,
      };
      dispatch({ type: 'SET_ERROR', payload: appError });

      return {
        success: false,
        message: errorMessage,
        error: errorMessage,
      };
    } finally {
      dispatch({ type: 'SET_OPERATION_IN_PROGRESS', payload: false });
    }
  }, [state.activeProfile]);

  const loadProfiles = useCallback(async (): Promise<void> => {
    try {
      dispatch({ type: 'SET_LOADING', payload: true });
      dispatch({ type: 'CLEAR_ERROR' });

      // Get all profiles
      const response = await window.phantomVault.profile.getAll();

      if (response.success && response.profiles) {
        const profiles: VaultProfile[] = response.profiles.map(profile => ({
          id: profile.id,
          name: profile.name,
          createdAt: new Date(profile.created_at * 1000), // Convert from Unix timestamp
          lastModified: new Date(profile.created_at * 1000),
          isActive: false, // Will be set by getActive call
        }));

        dispatch({ type: 'SET_PROFILES', payload: profiles });

        // Get active profile
        const activeResponse = await window.phantomVault.profile.getActive();
        if (activeResponse.success && activeResponse.profile) {
          const activeProfile: VaultProfile = {
            id: activeResponse.profile.id,
            name: activeResponse.profile.name,
            createdAt: new Date(activeResponse.profile.created_at * 1000),
            lastModified: new Date(activeResponse.profile.created_at * 1000),
            isActive: true,
          };

          dispatch({ type: 'SET_ACTIVE_PROFILE', payload: activeProfile });
        }
      } else {
        throw new Error(response.error || 'Failed to load profiles');
      }
    } catch (error) {
      const appError: AppError = {
        type: 'system',
        code: 'LOAD_PROFILES_FAILED',
        message: error instanceof Error ? error.message : 'Failed to load profiles',
        timestamp: new Date(),
        recoverable: true,
      };
      dispatch({ type: 'SET_ERROR', payload: appError });
    }
  }, []);

  const setActiveProfile = useCallback(async (profileId: string): Promise<void> => {
    try {
      dispatch({ type: 'SET_LOADING', payload: true });
      dispatch({ type: 'CLEAR_ERROR' });

      const response = await window.phantomVault.profile.setActive(profileId);

      if (response.success && response.profile) {
        const activeProfile: VaultProfile = {
          id: response.profile.id,
          name: response.profile.name,
          createdAt: new Date(response.profile.created_at * 1000),
          lastModified: new Date(response.profile.created_at * 1000),
          isActive: true,
        };

        dispatch({ type: 'SET_ACTIVE_PROFILE', payload: activeProfile });

        // Reload vaults for the new active profile
        await loadVaults();
      } else {
        throw new Error(response.error || 'Failed to set active profile');
      }
    } catch (error) {
      const appError: AppError = {
        type: 'system',
        code: 'SET_ACTIVE_PROFILE_FAILED',
        message: error instanceof Error ? error.message : 'Failed to set active profile',
        timestamp: new Date(),
        recoverable: true,
      };
      dispatch({ type: 'SET_ERROR', payload: appError });
    }
  }, [loadVaults]);

  const createProfile = useCallback(async (
    name: string,
    password: string,
    recoveryKey: string
  ): Promise<VaultOperationResult> => {
    try {
      dispatch({ type: 'SET_OPERATION_IN_PROGRESS', payload: true });
      dispatch({ type: 'CLEAR_ERROR' });

      const response = await window.phantomVault.profile.create(name, password, recoveryKey);

      if (response.success && response.profile) {
        const newProfile: VaultProfile = {
          id: response.profile.id,
          name: response.profile.name,
          createdAt: new Date(response.profile.created_at * 1000),
          lastModified: new Date(response.profile.created_at * 1000),
          isActive: false,
        };

        dispatch({ type: 'ADD_PROFILE', payload: newProfile });

        return {
          success: true,
          message: 'Profile created successfully',
          vaultId: newProfile.id,
        };
      } else {
        throw new Error(response.error || 'Failed to create profile');
      }
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : 'Failed to create profile';
      const appError: AppError = {
        type: 'system',
        code: 'CREATE_PROFILE_FAILED',
        message: errorMessage,
        timestamp: new Date(),
        recoverable: true,
      };
      dispatch({ type: 'SET_ERROR', payload: appError });

      return {
        success: false,
        message: errorMessage,
        error: errorMessage,
      };
    } finally {
      dispatch({ type: 'SET_OPERATION_IN_PROGRESS', payload: false });
    }
  }, []);

  const clearError = useCallback((): void => {
    dispatch({ type: 'CLEAR_ERROR' });
  }, []);

  // ==================== EFFECTS ====================

  // Load profiles on mount
  useEffect(() => {
    loadProfiles();
  }, [loadProfiles]);

  // Load vaults when active profile changes
  useEffect(() => {
    if (state.activeProfile) {
      loadVaults();
    }
  }, [state.activeProfile, loadVaults]);

  // ==================== CONTEXT VALUE ====================

  const contextValue: VaultContextValue = {
    state: {
      vaults: state.vaults,
      selectedVault: state.selectedVault,
      profiles: state.profiles,
      activeProfile: state.activeProfile,
      loading: state.loading,
      error: state.error,
    },
    actions: {
      loadVaults,
      selectVault,
      createVault,
      updateVault,
      deleteVault,
      loadProfiles,
      setActiveProfile,
      createProfile,
      clearError,
    },
  };

  return (
    <VaultContext.Provider value={contextValue}>
      {children}
    </VaultContext.Provider>
  );
};

// ==================== HOOK ====================

export const useVault = (): VaultContextValue => {
  const context = useContext(VaultContext);
  if (context === undefined) {
    throw new Error('useVault must be used within a VaultProvider');
  }
  return context;
};

// ==================== EXPORTS ====================

export default VaultContext;