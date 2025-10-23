/**
 * Security Helpers for PhantomVault Electron App
 * 
 * Provides security hardening functions for the main process
 */

const { execSync } = require('child_process');
const os = require('os');

/**
 * Disable core dumps on Unix systems
 */
function setrlimit() {
  if (process.platform === 'win32') {
    return; // Not applicable on Windows
  }
  
  try {
    // Set core dump size limit to 0
    execSync('ulimit -c 0', { stdio: 'ignore' });
  } catch (error) {
    console.warn('⚠️ Could not disable core dumps:', error.message);
  }
}

/**
 * Set secure memory limits
 */
function setMemoryLimits() {
  try {
    // Set reasonable memory limits to prevent memory exhaustion attacks
    if (process.platform !== 'win32') {
      execSync('ulimit -v 2097152', { stdio: 'ignore' }); // 2GB virtual memory limit
    }
  } catch (error) {
    console.warn('⚠️ Could not set memory limits:', error.message);
  }
}

/**
 * Clear sensitive environment variables
 */
function clearSensitiveEnv() {
  const sensitiveVars = [
    'PHANTOM_VAULT_DEBUG',
    'PHANTOM_VAULT_DEV_MODE',
    'NODE_ENV'
  ];
  
  sensitiveVars.forEach(varName => {
    if (process.env[varName] && process.env[varName].toLowerCase() === 'true') {
      console.warn(`⚠️ Sensitive environment variable ${varName} is set`);
    }
  });
}

/**
 * Initialize security hardening
 */
function initializeSecurity() {
  setrlimit();
  setMemoryLimits();
  clearSensitiveEnv();
  
  // Disable Node.js warnings in production
  if (process.env.NODE_ENV === 'production') {
    process.removeAllListeners('warning');
  }
}

module.exports = {
  setrlimit,
  setMemoryLimits,
  clearSensitiveEnv,
  initializeSecurity
};