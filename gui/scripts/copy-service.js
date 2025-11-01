/**
 * Copy PhantomVault unified service to GUI build directory
 */

const fs = require('fs');
const path = require('path');

const projectRoot = path.join(__dirname, '../..');
const guiRoot = path.join(__dirname, '..');

// Possible service locations
const servicePaths = [
  path.join(projectRoot, 'build/bin/phantomvault'),
  path.join(projectRoot, 'bin/phantomvault'),
  path.join(projectRoot, 'phantomvault'),
  path.join(projectRoot, 'core/build/bin/phantomvault-service'),
];

// Add platform-specific extensions
if (process.platform === 'win32') {
  servicePaths.forEach((servicePath, index) => {
    servicePaths[index] = servicePath + '.exe';
  });
}

// Find the service executable
let foundServicePath = null;
for (const servicePath of servicePaths) {
  if (fs.existsSync(servicePath)) {
    foundServicePath = servicePath;
    console.log('[Copy Service] Found service at:', servicePath);
    break;
  }
}

if (!foundServicePath) {
  console.warn('[Copy Service] Warning: No service executable found. GUI will attempt to find it at runtime.');
  console.log('[Copy Service] Searched paths:', servicePaths);
  process.exit(0); // Don't fail the build
}

// Create bin directory in GUI
const guiBinDir = path.join(guiRoot, 'dist/bin');
if (!fs.existsSync(guiBinDir)) {
  fs.mkdirSync(guiBinDir, { recursive: true });
}

// Copy service to GUI bin directory
const targetPath = path.join(guiBinDir, path.basename(foundServicePath));
try {
  fs.copyFileSync(foundServicePath, targetPath);
  
  // Make executable on Unix systems
  if (process.platform !== 'win32') {
    fs.chmodSync(targetPath, 0o755);
  }
  
  console.log('[Copy Service] Copied service to:', targetPath);
} catch (error) {
  console.error('[Copy Service] Failed to copy service:', error.message);
  process.exit(1);
}

console.log('[Copy Service] Service copy completed successfully');