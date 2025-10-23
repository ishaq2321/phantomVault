/**
 * Vault Operations Test Setup
 * 
 * Common setup and utilities for vault operations tests
 */

import React from 'react';
import '@testing-library/jest-dom/vitest';
import { AppProvider } from '../../../contexts/AppContext';
import { VaultProvider } from '../../../contexts/VaultContext';

// ==================== GLOBAL MOCKS ====================

// Mock timers for testing async operations
global.setTimeout = jest.fn((fn, delay) => {
  if (typeof fn === 'function') {
    fn();
  }
  return 1 as any;
});

global.clearTimeout = jest.fn();

// Mock window methods
Object.defineProperty(window, 'confirm', {
  writable: true,
  value: jest.fn(() => true),
});

Object.defineProperty(window, 'alert', {
  writable: true,
  value: jest.fn(),
});

// Mock clipboard API
Object.defineProperty(navigator, 'clipboard', {
  writable: true,
  value: {
    writeText: jest.fn(() => Promise.resolve()),
    readText: jest.fn(() => Promise.resolve('')),
  },
});

// ==================== TEST UTILITIES ====================

/**
 * Create a mock vault operation result
 */
export const createMockOperationResult = (success: boolean, error?: string) => ({
  success,
  error,
  details: success ? undefined : { timestamp: new Date().toISOString(), error },
});

/**
 * Create a mock progress callback
 */
export const createMockProgressCallback = () => {
  const callback = jest.fn();
  const progressSteps = [
    { stage: 'initializing', percentage: 0, message: 'Starting operation...' },
    { stage: 'authenticating', percentage: 25, message: 'Verifying credentials...' },
    { stage: 'mounting', percentage: 50, message: 'Mounting vault...' },
    { stage: 'finalizing', percentage: 100, message: 'Operation complete' },
  ];

  // Simulate progress updates
  callback.mockImplementation((onProgress) => {
    if (onProgress) {
      progressSteps.forEach((step, index) => {
        setTimeout(() => onProgress(step), index * 100);
      });
    }
  });

  return { callback, progressSteps };
};

/**
 * Wait for async operations to complete
 */
export const waitForAsync = (ms: number = 0) => {
  return new Promise(resolve => setTimeout(resolve, ms));
};

/**
 * Simulate user interaction delay
 */
export const simulateUserDelay = () => waitForAsync(100);

/**
 * Create a mock vault info object
 */
export const createMockVaultInfo = (overrides = {}) => ({
  id: `vault-${Math.random().toString(36).substr(2, 9)}`,
  name: 'Test Vault',
  path: '/test/path',
  status: 'unmounted' as const,
  lastAccessed: new Date('2024-01-01'),
  size: 1024 * 1024 * 100, // 100MB
  folderCount: 25,
  ...overrides,
});

/**
 * Create multiple mock vaults
 */
export const createMockVaults = (count: number, baseOverrides = {}) => {
  return Array.from({ length: count }, (_, index) =>
    createMockVaultInfo({
      id: `vault-${index + 1}`,
      name: `Vault ${index + 1}`,
      ...baseOverrides,
    })
  );
};

/**
 * Mock vault operations with configurable responses
 */
export const createMockVaultOperations = (responses: Record<string, any> = {}) => ({
  mountVault: jest.fn().mockResolvedValue(
    responses.mount || createMockOperationResult(true)
  ),
  unmountVault: jest.fn().mockResolvedValue(
    responses.unmount || createMockOperationResult(true)
  ),
  createVault: jest.fn().mockResolvedValue(
    responses.create || createMockOperationResult(true)
  ),
  deleteVault: jest.fn().mockResolvedValue(
    responses.delete || createMockOperationResult(true)
  ),
  updateVault: jest.fn().mockResolvedValue(
    responses.update || createMockOperationResult(true)
  ),
});

/**
 * Mock app actions
 */
export const createMockAppActions = () => ({
  addNotification: jest.fn(),
  removeNotification: jest.fn(),
  updateSettings: jest.fn(),
});

/**
 * Create a test wrapper with all necessary providers
 */
export const createTestWrapper = (mockVaults = [], mockActions = createMockAppActions()) => {
  return ({ children }: { children: React.ReactNode }) => {
    return (
      <AppProvider>
        <VaultProvider>
          {children}
        </VaultProvider>
      </AppProvider>
    );
  };
};
/**

 * Simulate a slow operation
 */
export const createSlowOperation = (delay: number = 1000, success: boolean = true) => {
  return jest.fn().mockImplementation(() => {
    return new Promise(resolve => {
      setTimeout(() => {
        resolve(createMockOperationResult(success));
      }, delay);
    });
  });
};

/**
 * Simulate an operation with progress updates
 */
export const createProgressOperation = (steps: number = 4, success: boolean = true) => {
  return jest.fn().mockImplementation((vaultId, password, options) => {
    return new Promise(resolve => {
      let currentStep = 0;
      
      const updateProgress = () => {
        if (options?.onProgress && currentStep < steps) {
          options.onProgress({
            stage: `step-${currentStep}`,
            percentage: (currentStep / steps) * 100,
            message: `Step ${currentStep + 1} of ${steps}`,
          });
          currentStep++;
          setTimeout(updateProgress, 100);
        } else {
          resolve(createMockOperationResult(success));
        }
      };
      
      updateProgress();
    });
  });
};

/**
 * Assert that a notification was shown
 */
export const expectNotification = (mockAddNotification: jest.Mock, type: string, title: string) => {
  expect(mockAddNotification).toHaveBeenCalledWith(
    expect.objectContaining({
      type,
      title,
    })
  );
};

/**
 * Assert that an operation was called with correct parameters
 */
export const expectOperationCall = (
  mockOperation: jest.Mock,
  vaultId: string,
  ...additionalArgs: any[]
) => {
  expect(mockOperation).toHaveBeenCalledWith(vaultId, ...additionalArgs);
};

/**
 * Create a mock keyboard event
 */
export const createMockKeyboardEvent = (key: string, options: KeyboardEventInit = {}) => {
  return new KeyboardEvent('keydown', {
    key,
    bubbles: true,
    cancelable: true,
    ...options,
  });
};

/**
 * Simulate pressing a key
 */
export const simulateKeyPress = (element: Element, key: string) => {
  const event = createMockKeyboardEvent(key);
  element.dispatchEvent(event);
};

// ==================== CUSTOM MATCHERS ====================

declare global {
  namespace jest {
    interface Matchers<R> {
      toBeValidVaultOperation(): R;
      toHaveProgressUpdates(): R;
    }
  }
}

// Extend Jest matchers
expect.extend({
  toBeValidVaultOperation(received) {
    const isValid = received && 
                   typeof received.success === 'boolean' &&
                   (received.success || typeof received.error === 'string');
    
    return {
      message: () => `Expected ${received} to be a valid vault operation result`,
      pass: isValid,
    };
  },
  
  toHaveProgressUpdates(received) {
    const hasProgress = received && 
                       Array.isArray(received.calls) &&
                       received.calls.some((call: any[]) => 
                         call[0] && typeof call[0].percentage === 'number'
                       );
    
    return {
      message: () => `Expected ${received} to have progress updates`,
      pass: hasProgress,
    };
  },
});

// ==================== CLEANUP ====================

afterEach(() => {
  jest.clearAllMocks();
  jest.clearAllTimers();
});