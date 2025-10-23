/**
 * Test Setup Configuration
 * 
 * Global test setup and configuration for Vitest and React Testing Library
 */

import React from 'react';
import '@testing-library/jest-dom';
import { vi } from 'vitest';

// ==================== GLOBAL MOCKS ====================

// Mock Electron APIs
global.electronAPI = {
  // IPC methods
  invoke: vi.fn(),
  on: vi.fn(),
  removeAllListeners: vi.fn(),
  
  // Vault operations
  createVault: vi.fn(),
  mountVault: vi.fn(),
  unmountVault: vi.fn(),
  deleteVault: vi.fn(),
  updateVault: vi.fn(),
  listVaults: vi.fn(),
  
  // File system operations
  selectFolder: vi.fn(),
  checkPath: vi.fn(),
  
  // Settings
  getSettings: vi.fn(),
  updateSettings: vi.fn(),
  
  // Activity monitoring
  getActivityLog: vi.fn(),
  subscribeToActivity: vi.fn(),
  
  // System integration
  showNotification: vi.fn(),
  setTrayMenu: vi.fn(),
};

// Mock window.confirm and window.alert
global.confirm = vi.fn(() => true);
global.alert = vi.fn();

// Mock ResizeObserver
global.ResizeObserver = vi.fn().mockImplementation(() => ({
  observe: vi.fn(),
  unobserve: vi.fn(),
  disconnect: vi.fn(),
}));

// Mock IntersectionObserver
global.IntersectionObserver = vi.fn().mockImplementation(() => ({
  observe: vi.fn(),
  unobserve: vi.fn(),
  disconnect: vi.fn(),
}));

// ==================== CONSOLE MOCKS ====================

// Suppress console warnings in tests unless explicitly needed
const originalConsoleError = console.error;
const originalConsoleWarn = console.warn;

beforeEach(() => {
  console.error = vi.fn();
  console.warn = vi.fn();
});

afterEach(() => {
  console.error = originalConsoleError;
  console.warn = originalConsoleWarn;
});

// ==================== CUSTOM MATCHERS ====================

expect.extend({
  toBeValidVaultConfig(received) {
    const requiredFields = ['name', 'path', 'password'];
    const missingFields = requiredFields.filter(field => !received[field]);
    
    if (missingFields.length > 0) {
      return {
        message: () => `Expected vault config to be valid, but missing fields: ${missingFields.join(', ')}`,
        pass: false,
      };
    }
    
    return {
      message: () => `Expected vault config to be invalid`,
      pass: true,
    };
  },
  
  toHaveValidationError(received, field) {
    const hasError = received.errors?.some((error: any) => error.field === field);
    
    if (!hasError) {
      return {
        message: () => `Expected validation result to have error for field '${field}'`,
        pass: false,
      };
    }
    
    return {
      message: () => `Expected validation result not to have error for field '${field}'`,
      pass: true,
    };
  },
});

// ==================== TEST UTILITIES ====================

// Helper to create mock vault info
export const createMockVault = (overrides: Partial<any> = {}) => ({
  id: 'mock-vault-id',
  name: 'Mock Vault',
  path: '/mock/path',
  status: 'unmounted',
  lastAccessed: new Date('2024-01-01'),
  size: 1024 * 1024 * 100, // 100MB
  folderCount: 25,
  ...overrides,
});

// Helper to create mock vault config
export const createMockVaultConfig = (overrides: Partial<any> = {}) => ({
  name: 'Mock Vault',
  path: '/mock/path',
  password: 'MockPassword123!',
  keyboardSequence: 'mockseq',
  autoMount: false,
  encryptionLevel: 'standard',
  autoLock: true,
  lockTimeout: 15,
  ...overrides,
});

// Helper to wait for async operations
export const waitForAsync = () => new Promise(resolve => setTimeout(resolve, 0));

// Helper to trigger validation
export const triggerValidation = async (element: HTMLElement) => {
  element.focus();
  element.blur();
  await waitForAsync();
};

// ==================== MOCK DATA ====================

export const mockVaults = [
  createMockVault({
    id: 'vault-1',
    name: 'Personal Documents',
    path: '/home/user/personal',
  }),
  createMockVault({
    id: 'vault-2',
    name: 'Work Files',
    path: '/home/user/work',
    status: 'mounted',
  }),
  createMockVault({
    id: 'vault-3',
    name: 'Archive',
    path: '/home/user/archive',
    status: 'error',
  }),
];

export const mockSettings = {
  theme: 'dark',
  autoStart: false,
  notifications: true,
  minimizeToTray: true,
  autoLock: true,
  lockTimeout: 15,
};

export const mockActivityLog = [
  {
    id: '1',
    timestamp: new Date('2024-01-01T10:00:00Z'),
    level: 'info',
    message: 'Vault mounted successfully',
    vaultId: 'vault-1',
    vaultName: 'Personal Documents',
  },
  {
    id: '2',
    timestamp: new Date('2024-01-01T10:05:00Z'),
    level: 'warning',
    message: 'Vault auto-lock triggered',
    vaultId: 'vault-1',
    vaultName: 'Personal Documents',
  },
  {
    id: '3',
    timestamp: new Date('2024-01-01T10:10:00Z'),
    level: 'error',
    message: 'Failed to mount vault',
    vaultId: 'vault-3',
    vaultName: 'Archive',
  },
];

// ==================== PERFORMANCE HELPERS ====================

// Helper to measure component render time
export const measureRenderTime = (renderFn: () => void) => {
  const start = performance.now();
  renderFn();
  const end = performance.now();
  return end - start;
};

// Helper to simulate slow operations
export const simulateSlowOperation = (ms: number = 1000) => {
  return new Promise(resolve => setTimeout(resolve, ms));
};

// ==================== ACCESSIBILITY HELPERS ====================

// Helper to check if element is accessible
export const checkAccessibility = (element: HTMLElement) => {
  const checks = {
    hasAriaLabel: element.hasAttribute('aria-label') || element.hasAttribute('aria-labelledby'),
    hasRole: element.hasAttribute('role'),
    isFocusable: element.tabIndex >= 0 || ['button', 'input', 'select', 'textarea', 'a'].includes(element.tagName.toLowerCase()),
    hasKeyboardSupport: element.hasAttribute('onkeydown') || element.hasAttribute('onkeypress'),
  };
  
  return checks;
};

// ==================== CLEANUP ====================

// Global cleanup after each test
afterEach(() => {
  // Clear all mocks
  vi.clearAllMocks();
  
  // Clear any timers
  vi.clearAllTimers();
  
  // Reset DOM
  document.body.innerHTML = '';
});