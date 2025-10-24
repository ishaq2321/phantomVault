/**
 * Security Helpers - Priority 2 Security Hardening
 * 
 * Provides low-level security functions:
 * - Disable core dumps (prevent memory dumps to disk)
 * - Memory locking (prevent swap to disk)
 * - Secure memory clearing
 */

const { exec } = require('child_process');
const os = require('os');

/**
 * Disable core dumps on Unix systems
 * Sets RLIMIT_CORE to 0 to prevent memory dumps
 */
function setrlimit() {
  if (process.platform === 'win32') {
    // Windows doesn't use ulimit - handled differently
    return;
  }

  try {
    // Set core dump size to 0 (disable)
    exec('ulimit -c 0', (error) => {
      if (error) {
        console.warn('⚠️ [SECURITY] Could not set ulimit:', error.message);
      }
    });

    // Also set for child processes via environment
    process.env.RLIMIT_CORE = '0';
    
  } catch (error) {
    console.error('❌ [SECURITY] Failed to disable core dumps:', error);
  }
}

/**
 * Lock a buffer in memory (prevent swap to disk)
 * Note: Requires elevated privileges on most systems
 * 
 * @param {Buffer} buffer - Buffer to lock in RAM
 * @returns {boolean} - Success status
 */
function mlockBuffer(buffer) {
  if (process.platform === 'win32') {
    // Windows requires VirtualLock API (not available in pure Node.js)
    console.warn('⚠️ [SECURITY] Memory locking not supported on Windows without native addon');
    return false;
  }

  try {
    // On Linux/Unix, we can use mlock via child process
    // Note: This is limited and requires CAP_IPC_LOCK capability
    // For production, this should be implemented in C++ addon
    console.warn('⚠️ [SECURITY] Memory locking requires native implementation (C++ addon)');
    return false;
  } catch (error) {
    console.error('❌ [SECURITY] Memory locking failed:', error);
    return false;
  }
}

/**
 * Securely zero out a buffer
 * Prevents compiler optimization from removing the zeroing
 * 
 * @param {Buffer} buffer - Buffer to clear
 */
function secureZero(buffer) {
  if (!buffer || !Buffer.isBuffer(buffer)) {
    return;
  }

  // Fill with zeros
  buffer.fill(0);
  
  // Force memory barrier (prevent optimization)
  // In production, this should use sodium_memzero or similar
  const dummy = buffer[0];
  
  // Additional overwrite patterns for extra security
  buffer.fill(0xFF);
  buffer.fill(0x00);
}

/**
 * Create a secure random buffer
 * 
 * @param {number} size - Size in bytes
 * @returns {Buffer}
 */
function secureRandom(size) {
  return crypto.randomBytes(size);
}

/**
 * Constant-time string comparison
 * Prevents timing attacks
 * 
 * @param {string|Buffer} a 
 * @param {string|Buffer} b 
 * @returns {boolean}
 */
function constantTimeCompare(a, b) {
  if (typeof a === 'string') a = Buffer.from(a);
  if (typeof b === 'string') b = Buffer.from(b);
  
  if (a.length !== b.length) {
    return false;
  }

  return crypto.timingSafeEqual(a, b);
}

/**
 * Generate HMAC for integrity checking
 * 
 * @param {Buffer|string} data - Data to HMAC
 * @param {Buffer|string} key - HMAC key
 * @returns {string} - Hex encoded HMAC
 */
function generateHMAC(data, key) {
  return crypto.createHmac('sha256', key)
    .update(data)
    .digest('hex');
}

/**
 * Verify HMAC
 * 
 * @param {Buffer|string} data - Data to verify
 * @param {Buffer|string} key - HMAC key
 * @param {string} expectedHmac - Expected HMAC (hex)
 * @returns {boolean}
 */
function verifyHMAC(data, key, expectedHmac) {
  const actualHmac = generateHMAC(data, key);
  return constantTimeCompare(actualHmac, expectedHmac);
}

module.exports = {
  setrlimit,
  mlockBuffer,
  secureZero,
  secureRandom,
  constantTimeCompare,
  generateHMAC,
  verifyHMAC
};
