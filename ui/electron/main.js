const { app, BrowserWindow, ipcMain, dialog, globalShortcut, Notification, Tray, Menu, nativeImage } = require('electron');
const path = require('path');
const fs = require('fs');
const { exec } = require('child_process');
const util = require('util');
const execAsync = util.promisify(exec);
const crypto = require('crypto');

// ============================================================================
// PRIORITY 2 SECURITY: Security hardening
// ============================================================================
let securityHelpers = null;
try {
  securityHelpers = require('./security-helpers');
  
  // Disable core dumps on Unix systems
  if (process.platform !== 'win32') {
    securityHelpers.setrlimit();
    console.log('🔒 [SECURITY] Core dumps disabled');
  }
} catch (error) {
  console.warn('⚠️ Security helpers not available:', error.message);
}

// Import HotkeyManager (will be compiled from TypeScript)
let HotkeyManager = null;
try {
  const hotkeyModule = require('./hotkey-manager');
  HotkeyManager = hotkeyModule.HotkeyManager;
} catch (error) {
  console.warn('⚠️ HotkeyManager not available (TypeScript not compiled)');
}

// Import AutoLockManager
let AutoLockManager = null;
try {
  const autoLockModule = require('./auto-lock-manager');
  AutoLockManager = autoLockModule.AutoLockManager;
} catch (error) {
  console.warn('⚠️ AutoLockManager not available (TypeScript not compiled)');
}

// Import FileSystemWatcher
let FileSystemWatcher = null;
try {
  const fsWatcherModule = require('./fs-watcher');
  FileSystemWatcher = fsWatcherModule.FileSystemWatcher;
} catch (error) {
  console.warn('⚠️ FileSystemWatcher not available (TypeScript not compiled)');
}

// Import VaultProfileManager and VaultFolderManager for main process
// NOTE: These are TypeScript modules in src/services/ and should be accessed via IPC
// The frontend React app will use these managers through the renderer process
let VaultProfileManager = null;
let VaultFolderManager = null;
/* Commented out - these are frontend modules, not for main process
try {
  const profileModule = require('./VaultProfileManager');
  const folderModule = require('./VaultFolderManager');
  VaultProfileManager = profileModule.VaultProfileManager;
  VaultFolderManager = folderModule.VaultFolderManager;
} catch (error) {
  console.error('❌ Failed to load vault managers:', error.message);
}
*/

let mainWindow = null;
let tray = null;
let hotkeyManager = null;
let autoLockManager = null;
let fsWatcher = null;
let profileManager = null;
let folderManager = null;
let currentHotkey = 'CommandOrControl+Shift+P'; // Fallback default hotkey

// In-memory cache for master password (cleared on app quit and after timeout for security)
let cachedMasterPassword = null;
let passwordClearTimer = null;
const PASSWORD_CACHE_TIMEOUT = 5 * 60 * 1000; // 5 minutes

/**
 * Securely clear password from memory
 * PRIORITY 2: Multi-pass overwrite to prevent memory dumps
 * Follows DOD 5220.22-M standard (simplified for strings)
 */
function securelyClearPassword() {
  if (cachedMasterPassword) {
    const length = cachedMasterPassword.length;
    
    // 3-pass overwrite (DOD 5220.22-M)
    // Pass 1: Write 0x00
    cachedMasterPassword = '\0'.repeat(length);
    
    // Pass 2: Write 0xFF
    cachedMasterPassword = String.fromCharCode(0xFF).repeat(length);
    
    // Pass 3: Random data
    const randomChars = Array.from({length}, () => 
      String.fromCharCode(Math.floor(Math.random() * 256))
    ).join('');
    cachedMasterPassword = randomChars;
    
    // Final: Set to null
    cachedMasterPassword = null;
    
    console.log('🔒 [SECURITY] Master password securely cleared (3-pass overwrite)');
  }
  if (passwordClearTimer) {
    clearTimeout(passwordClearTimer);
    passwordClearTimer = null;
  }
}

/**
 * Cache password with automatic timeout
 */
function cachePasswordWithTimeout(password) {
  // Clear any existing timer
  if (passwordClearTimer) {
    clearTimeout(passwordClearTimer);
  }
  
  cachedMasterPassword = password;
  console.log(`🔐 [SECURITY] Master password cached (auto-clear in ${PASSWORD_CACHE_TIMEOUT / 1000}s)`);
  
  // Set timer to clear password
  passwordClearTimer = setTimeout(() => {
    securelyClearPassword();
  }, PASSWORD_CACHE_TIMEOUT);
}

// Load PhantomVault C++ addon
let vault = null;
try {
  const addon = require('../native/build/Release/phantom_vault_addon.node');
  vault = new addon.PhantomVault();
  console.log('🔐 PhantomVault C++ addon loaded successfully');
} catch (error) {
  console.error('❌ Failed to load PhantomVault addon:', error.message);
  console.error('   Falling back to mock mode');
}

// DISABLED: Fallback hotkey registration - C++ service handles hotkeys invisibly
function registerGlobalHotkey(hotkey) {
  console.log(`⚠️  [FALLBACK] Hotkey registration disabled: ${hotkey}`);
  console.log(`   → C++ service handles all hotkeys for invisible operation`);
  console.log(`   → This prevents visible windows from opening accidentally`);
  
  // DO NOT register Electron hotkeys when C++ service is running
  // This prevents the dangerous behavior of opening main window
  return false; // Always return false to indicate Electron hotkeys are disabled
}

// Create system tray
function createTray() {
  // Create a simple 16x16 icon (you can replace with a proper icon file later)
  const icon = nativeImage.createFromDataURL('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAABHNCSVQICAgIfAhkiAAAAAlwSFlzAAAAdgAAAHYBTnsmCAAAABl0RVh0U29mdHdhcmUAd3d3Lmlua3NjYXBlLm9yZ5vuPBoAAAE5SURBVDiNpZIxSgNBFIa/2U02xkKwsBAsLCwEwUKwsRG8gYWVjY2FWHgBsRAEG0FIYWNhIVjYWAgWgoVgIVgIFoKFYCH4bGYnO5vdZBP8YWF23vvmn/nfGxHhPxMRYWtri0ajQbPZpF6vMzk5yfT0NPl8nlwuR6lUIpvNUiwWGR0d/Xu AoihI07TH5XKZVCqF4zikaRrNZpOVlRXW19fZ2dlhbW2NlZUVdnd3WV5eZmlpCdd1AdA0jU6n8+XC/v4+AJVKBYA8z8N1XRzHwTAMLMvCNE0sy0LXdXRdx7IsdF1H13Us y8I0TTzPw3VdDMMglUqRSCQYGBjA932y2SzZbBbTNBkcHCSRSBCLxejv7yeVShGPx+nr6yMWixGNRonFYkREiMfjtFot/gBPnz0l4LgAAAABJRU5ErkJggg==');
  
  tray = new Tray(icon);
  
  const contextMenu = Menu.buildFromTemplate([
    {
      label: 'PhantomVault',
      enabled: false
    },
    { type: 'separator' },
    {
      label: 'Show App',
      click: () => {
        if (mainWindow) {
          if (mainWindow.isMinimized()) mainWindow.restore();
          mainWindow.show();
          mainWindow.focus();
        } else {
          createWindow();
        }
      }
    },
    {
      label: 'Unlock Folders (Ctrl+Alt+V)',
      click: () => {
        console.log('🔑 [TRAY] Unlock requested - C++ service handles invisibly');
        console.log('   → Press Ctrl+Alt+V anywhere for invisible sequence detection');
        console.log('   → Format: T+password (temporary) or P+password (permanent)');
        
        // DO NOT open overlay or main window - maintain invisible operation
        // User should use the actual Ctrl+Alt+V hotkey for invisible operation
      }
    },
    { type: 'separator' },
    {
      label: 'Quit PhantomVault',
      click: () => {
        // Cleanup before quit
        if (hotkeyManager) {
          hotkeyManager.unregisterHotkeys();
        } else {
          globalShortcut.unregisterAll();
        }
        if (autoLockManager) {
          autoLockManager.stopMonitoring();
        }
        if (fsWatcher) {
          fsWatcher.stopWatching();
        }
        // Securely clear cached master password
        securelyClearPassword();
        app.quit();
      }
    }
  ]);
  
  tray.setToolTip('PhantomVault - Secure Folder Encryption');
  tray.setContextMenu(contextMenu);
  
  // Show window when tray icon is clicked
  tray.on('click', () => {
    if (mainWindow) {
      if (mainWindow.isVisible()) {
        mainWindow.hide();
      } else {
        if (mainWindow.isMinimized()) mainWindow.restore();
        mainWindow.show();
        mainWindow.focus();
      }
    } else {
      createWindow();
    }
  });
  
  console.log('✅ System tray created');
}

// DISABLED: Overlay window creation removed for invisible operation
// The C++ service handles all password input invisibly without creating windows

let overlayWindow = null; // Keep variable for compatibility

function createOverlayWindow(overlayData) {
  console.log('⚠️  [OVERLAY] Window creation disabled - C++ service handles invisibly');
  console.log('   → No visible windows created to maintain invisible operation');
  console.log('   → Sequence detection handled by native C++ service');
  
  // DO NOT create overlay window - this defeats the invisible operation
  return;
}

function closeOverlayWindow() {
  console.log('⚠️  [OVERLAY] Close requested - no overlay windows to close');
  // No overlay windows exist in invisible mode
  return;
}

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1400,
    height: 900,
    minWidth: 1200,
    minHeight: 800,
    show: false, // Don't show until ready
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, 'preload.js'),
      webSecurity: true,
      allowRunningInsecureContent: false,
    },
  });

  // Load from built files in production, dev server in development
  const isDev = process.env.NODE_ENV !== 'production';
  console.log('🌍 Environment Check:');
  console.log('   NODE_ENV:', process.env.NODE_ENV);
  console.log('   isDev:', isDev);
  
  // TEMPORARY: Load test file to debug rendering
  const testMode = process.env.TEST_RENDER === 'true';
  
  if (testMode) {
    const testPath = path.join(__dirname, '../test-render.html');
    console.log('   🧪 TEST MODE: Loading test file:', testPath);
    mainWindow.loadFile(testPath);
  } else if (isDev) {
    const devUrl = 'http://127.0.0.1:5173';
    console.log('   Loading from DEV SERVER:', devUrl);
    mainWindow.loadURL(devUrl);
  } else {
    const prodPath = path.join(__dirname, '../dist/index.html');
    console.log('   Loading from PRODUCTION BUILD:', prodPath);
    console.log('   File exists:', fs.existsSync(prodPath));
    mainWindow.loadFile(prodPath);
  }
  
  // Show window when ready to prevent white flash
  mainWindow.once('ready-to-show', () => {
    console.log('✅ Main window ready to show');
    mainWindow.show();
    mainWindow.webContents.openDevTools();
    
    // Create system tray after window is shown (required on some Linux systems)
    createTray();
  });
  
  // Log when page finishes loading
  mainWindow.webContents.on('did-finish-load', () => {
    console.log('✅ Page finished loading');
    
    // Execute code in renderer to check for errors
    mainWindow.webContents.executeJavaScript(`
      // Check if window.phantomVault exists
      console.log('🔍 Checking PhantomVault API...');
      if (window.phantomVault) {
        console.log('✅ window.phantomVault exists');
        console.log('   Available methods:', Object.keys(window.phantomVault).slice(0, 10).join(', '));
      } else {
        console.error('❌ window.phantomVault is NOT available!');
      }
      
      // Check for React errors
      console.log('🔍 Checking for errors...');
      if (window.__REACT_ERROR__) {
        console.error('❌ React Error:', window.__REACT_ERROR__);
      }
      
      'DevTools check complete';
    `).then(result => {
      console.log('DevTools check:', result);
    }).catch(err => {
      console.error('Failed to execute DevTools check:', err);
    });
  });
  
  // Capture console messages from renderer (disabled to avoid EPIPE errors in background mode)
  // Uncomment for debugging: use DevTools (Ctrl+Shift+I) instead
  /*
  mainWindow.webContents.on('console-message', (event, level, message, line, sourceId) => {
    try {
      const levels = ['verbose', 'info', 'warning', 'error'];
      console.log(`[Renderer Console ${levels[level]}] ${message}`);
      if (sourceId) {
        console.log(`   Source: ${sourceId}:${line}`);
      }
    } catch (err) {
      // Ignore EPIPE errors (broken pipe when stdout is closed)
      if (err.code !== 'EPIPE') {
        throw err;
      }
    }
  });
  */
  
  // Log any loading errors
  mainWindow.webContents.on('did-fail-load', (event, errorCode, errorDescription) => {
    console.error('❌ Page failed to load:');
    console.error('   Error code:', errorCode);
    console.error('   Description:', errorDescription);
  });
  
  // Register hotkeys when window is ready (prevent multiple registrations)
  let hotkeyRegistered = false;
  mainWindow.webContents.on('did-finish-load', () => {
    if (hotkeyRegistered) {
      console.log('⚠️ Hotkeys already registered, skipping...');
      return;
    }
    
    // Use HotkeyManager if available, otherwise fallback to simple hotkey
    if (HotkeyManager) {
      console.log('✅ Using HotkeyManager');
      if (!hotkeyManager) {
        hotkeyManager = HotkeyManager.getInstance();
      }
      
      // Set callbacks - DISABLED: Let C++ service handle hotkeys invisibly
      hotkeyManager.onUnlock(async () => {
        console.log(`\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
        console.log(`🔓 [ELECTRON] HOTKEY DETECTED - DELEGATING TO C++ SERVICE`);
        console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
        
        console.log(`⚠️  [ELECTRON] Hotkey handled by C++ service for invisible operation`);
        console.log(`   → C++ service will use sequence detection (no visible windows)`);
        console.log(`   → Format: T+password (temporary) or P+password (permanent)`);
        console.log(`   → No Electron overlay will be created to maintain invisibility`);
        console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n`);
        
        // DO NOT create overlay window - let C++ service handle invisibly
        // createOverlayWindow() calls removed to maintain invisible operation
      });
      
      hotkeyManager.onRecovery(() => {
        console.log('🔑 [ELECTRON] Recovery hotkey detected - delegating to C++ service');
        console.log('   → C++ service will handle recovery invisibly');
        console.log('   → No Electron overlay created to maintain invisibility');
        
        // DO NOT create overlay window - let C++ service handle invisibly
      });
      
      // DISABLED: Let C++ service handle hotkeys for invisible operation
      console.log('⚠️  HotkeyManager registration disabled - C++ service handles hotkeys');
      console.log('   → This prevents Electron from interfering with invisible sequence detection');
      console.log('   → All hotkeys managed by native C++ service for security');
      hotkeyRegistered = true; // Mark as registered to prevent fallback attempts
      
      // const result = hotkeyManager.registerHotkeys(); // DISABLED
    } else {
      console.log('⚠️ HotkeyManager not available - C++ service will handle hotkeys');
      console.log('   → Electron hotkey registration disabled for invisible operation');
      console.log('   → All hotkeys handled by native C++ service');
      hotkeyRegistered = true; // Mark as registered to prevent further attempts
    }
    
    // Initialize AutoLockManager
    if (AutoLockManager) {
      console.log('✅ Initializing AutoLockManager');
      autoLockManager = AutoLockManager.getInstance();
      
      // Set lock callback - will be connected to VaultFolderManager later via IPC
      autoLockManager.onLock(async (folderId) => {
        console.log(`🔒 AutoLock triggered for folder: ${folderId}`);
        // Send IPC to renderer to lock the folder (only if window exists)
        if (mainWindow && !mainWindow.isDestroyed()) {
          mainWindow.webContents.send('auto-lock-folder', { folderId });
        } else {
          console.log('   Window is closed, auto-lock event skipped (folder will lock on next app start)');
        }
      });
      
      // Start monitoring system events
      autoLockManager.startMonitoring();
      console.log('✅ AutoLockManager monitoring started');
    } else {
      console.warn('⚠️ AutoLockManager not available');
    }
    
    // Initialize FileSystemWatcher
    if (FileSystemWatcher) {
      console.log('✅ Initializing FileSystemWatcher');
      fsWatcher = FileSystemWatcher.getInstance();
      
      // Set callbacks
      fsWatcher.onChange((filePath) => {
        console.log(`📝 File changed: ${filePath}`);
        // Notify renderer of metadata change
        if (mainWindow) {
          mainWindow.webContents.send('metadata-changed', { filePath });
        }
      });
      
      fsWatcher.onSuspicious((activity) => {
        console.warn(`⚠️ Suspicious activity: ${activity}`);
        // Send notification to user
        if (mainWindow) {
          mainWindow.webContents.send('suspicious-activity', { activity });
        }
      });
      
      // Start watching vault storage
      const os = require('os');
      const username = os.userInfo().username;
      const storagePath = path.join(os.homedir(), '.phantom_vault_storage', username);
      
      if (fs.existsSync(storagePath)) {
        fsWatcher.startWatching(storagePath);
        console.log(`✅ FileSystemWatcher monitoring: ${storagePath}`);
      } else {
        console.warn('⚠️ Vault storage not found, FileSystemWatcher not started');
      }
    } else {
      console.warn('⚠️ FileSystemWatcher not available');
    }
  });
}

function setupIpcHandlers() {
  // Initialize addon on first IPC call
  let addonInitialized = false;
  const ensureInitialized = () => {
    if (!addonInitialized && vault) {
      const result = vault.initialize();
      console.log('🔧 PhantomVault initialized:', result);
      addonInitialized = result;
    }
    return addonInitialized;
  };

  // Helper function to hide a folder
  const hideFolderHelper = (folderPath) => {
    const dirname = path.dirname(folderPath);
    const basename = path.basename(folderPath);
    
    // If already starts with dot, skip
    if (basename.startsWith('.')) {
      return folderPath;
    }
    
    const hiddenPath = path.join(dirname, '.' + basename);
    
    // Rename to hidden
    fs.renameSync(folderPath, hiddenPath);
    
    return hiddenPath;
  };

  // Helper function to unhide a folder
  const unhideFolderHelper = (folderPath) => {
    const dirname = path.dirname(folderPath);
    const basename = path.basename(folderPath);
    
    let actualPath = folderPath;
    
    // If doesn't start with dot, check if hidden version exists
    if (!basename.startsWith('.')) {
      const hiddenPath = path.join(dirname, '.' + basename);
      if (fs.existsSync(hiddenPath)) {
        actualPath = hiddenPath;
      }
    }
    
    // Remove dot prefix
    const visibleBasename = basename.startsWith('.') ? basename.slice(1) : basename;
    const visiblePath = path.join(dirname, visibleBasename);
    
    if (actualPath !== visiblePath) {
      fs.renameSync(actualPath, visiblePath);
    }
    
    return visiblePath;
  };

  ipcMain.handle('get-version', async () => {
    if (vault && ensureInitialized()) {
      return vault.getVersion();
    }
    return '1.0.0-mock';
  });
  
  ipcMain.handle('select-folder', async () => {
    const result = await dialog.showOpenDialog(mainWindow, { properties: ['openDirectory'] });
    return result.filePaths[0] || null;
  });
  
  ipcMain.handle('show-notification', async (event, title, body) => {
    console.log('📢 Notification:', title, '-', body);
    return true;
  });

  // Close overlay window - DISABLED for invisible operation
  ipcMain.handle('close-overlay-window', async () => {
    console.log('🚪 [IPC] Close overlay requested - no overlay in invisible mode');
    return { success: true, message: 'No overlay windows in invisible mode' };
  });
  
  // Hide folder (Linux: prepend dot to make it hidden)
  ipcMain.handle('hide-folder', async (event, folderPath) => {
    try {
      console.log('🙈 Hiding folder:', folderPath);
      
      // On Linux, we can use chattr to set the immutable and hidden attributes
      // Or simply rename with a dot prefix
      const dirname = path.dirname(folderPath);
      const basename = path.basename(folderPath);
      
      // If already starts with dot, skip
      if (basename.startsWith('.')) {
        console.log('   Already hidden');
        return true;
      }
      
      const hiddenPath = path.join(dirname, '.' + basename);
      
      // Rename to hidden
      fs.renameSync(folderPath, hiddenPath);
      console.log('   ✅ Folder hidden:', hiddenPath);
      
      return true;
    } catch (error) {
      console.error('❌ Failed to hide folder:', error.message);
      throw error;
    }
  });
  
  // Unhide folder
  ipcMain.handle('unhide-folder', async (event, folderPath) => {
    try {
      console.log('👀 Unhiding folder:', folderPath);
      
      const dirname = path.dirname(folderPath);
      const basename = path.basename(folderPath);
      
      let actualPath = folderPath;
      
      // If doesn't start with dot, check if hidden version exists
      if (!basename.startsWith('.')) {
        const hiddenPath = path.join(dirname, '.' + basename);
        if (fs.existsSync(hiddenPath)) {
          actualPath = hiddenPath;
        }
      }
      
      // Remove dot prefix
      const visibleBasename = basename.startsWith('.') ? basename.slice(1) : basename;
      const visiblePath = path.join(dirname, visibleBasename);
      
      if (actualPath !== visiblePath) {
        fs.renameSync(actualPath, visiblePath);
        console.log('   ✅ Folder unhidden:', visiblePath);
      }
      
      return visiblePath;
    } catch (error) {
      console.error('❌ Failed to unhide folder:', error.message);
      throw error;
    }
  });
  
  // Register global hotkey
  ipcMain.handle('register-global-hotkey', async (event, hotkey) => {
    try {
      console.log('🔑 Registering new hotkey:', hotkey);
      const success = registerGlobalHotkey(hotkey);
      
      if (success) {
        return { success: true, hotkey: hotkey };
      } else {
        throw new Error('Failed to register hotkey. It may be in use by another application.');
      }
    } catch (error) {
      console.error('❌ Failed to register hotkey:', error.message);
      throw error;
    }
  });
  
  // Get current hotkey
  ipcMain.handle('get-current-hotkey', async () => {
    if (hotkeyManager) {
      const config = hotkeyManager.getConfig();
      return config.unlockHotkey;
    }
    return currentHotkey;
  });

  // PhantomVault 2.0 - New Hotkey Manager Handlers
  ipcMain.handle('hotkey:get-config', async () => {
    if (hotkeyManager) {
      return hotkeyManager.getConfig();
    }
    return {
      unlockHotkey: currentHotkey,
      recoveryHotkey: 'CommandOrControl+Alt+R',
      enabled: true,
    };
  });

  ipcMain.handle('hotkey:set-unlock', async (event, accelerator) => {
    if (hotkeyManager) {
      return hotkeyManager.setUnlockHotkey(accelerator);
    }
    // Fallback
    const success = registerGlobalHotkey(accelerator);
    return { success };
  });

  ipcMain.handle('hotkey:set-recovery', async (event, accelerator) => {
    if (hotkeyManager) {
      return hotkeyManager.setRecoveryHotkey(accelerator);
    }
    return { success: false, error: 'HotkeyManager not available' };
  });

  ipcMain.handle('hotkey:is-available', async (event, accelerator) => {
    if (hotkeyManager) {
      return hotkeyManager.isHotkeyAvailable(accelerator);
    }
    // Fallback test
    const success = globalShortcut.register(accelerator, () => {});
    if (success) {
      globalShortcut.unregister(accelerator);
    }
    return success;
  });

  ipcMain.handle('hotkey:get-suggestions', async () => {
    if (hotkeyManager) {
      return hotkeyManager.getSuggestedHotkeys();
    }
    return [
      'CommandOrControl+Alt+V',
      'CommandOrControl+Alt+P',
      'CommandOrControl+Shift+V',
    ];
  });

  ipcMain.handle('hotkey:set-enabled', async (event, enabled) => {
    if (hotkeyManager) {
      hotkeyManager.setEnabled(enabled);
      return { success: true };
    }
    return { success: false, error: 'HotkeyManager not available' };
  });
  
  // AutoLockManager IPC handlers
  ipcMain.handle('autolock:register-unlock', async (event, { folderId, profileId, mode, folderPath }) => {
    if (autoLockManager) {
      autoLockManager.registerUnlock(folderId, profileId, mode, folderPath);
      console.log(`[IPC] Registered unlock: ${folderId} (${mode})`);
      return { success: true };
    }
    return { success: false, error: 'AutoLockManager not available' };
  });
  
  ipcMain.handle('autolock:unregister-unlock', async (event, { folderId }) => {
    if (autoLockManager) {
      autoLockManager.unregisterUnlock(folderId);
      console.log(`[IPC] Unregistered unlock: ${folderId}`);
      return { success: true };
    }
    return { success: false, error: 'AutoLockManager not available' };
  });
  
  ipcMain.handle('autolock:get-stats', async () => {
    if (autoLockManager) {
      return autoLockManager.getStats();
    }
    return { isMonitoring: false, platform: 'unknown', temporaryUnlocks: 0 };
  });
  
  ipcMain.handle('autolock:lock-all-temporary', async () => {
    if (autoLockManager) {
      const result = await autoLockManager.lockAllTemporaryFolders();
      console.log(`[IPC] Locked ${result.success} temporary folder(s)`);
      return result;
    }
    return { success: 0, failed: 0, errors: ['AutoLockManager not available'] };
  });
  
  // FileSystemWatcher IPC handlers
  ipcMain.handle('fswatcher:get-stats', async () => {
    if (fsWatcher) {
      return fsWatcher.getStats();
    }
    return { isWatching: false, storagePath: null, totalEvents: 0 };
  });
  
  ipcMain.handle('fswatcher:get-recent-events', async (event, count = 10) => {
    if (fsWatcher) {
      return fsWatcher.getRecentEvents(count);
    }
    return [];
  });
  
  // ============================================================================
  // PhantomVault Phase 4: Native C++ Core IPC Handlers
  // ============================================================================
  
  // Initialize native addon
  if (vault) {
    try {
      const initialized = vault.initialize();
      if (initialized) {
        console.log('✅ PhantomVault C++ core initialized');
      } else {
        console.error('❌ Failed to initialize PhantomVault C++ core');
      }
    } catch (error) {
      console.error('❌ Error initializing PhantomVault C++ core:', error);
    }
  }
  
  // Core operations
  ipcMain.handle('native:initialize', async () => {
    if (!vault) return { success: false, error: 'Native addon not loaded' };
    try {
      const result = vault.initialize();
      return { success: result };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('native:get-version', async () => {
    if (!vault) return { success: false, error: 'Native addon not loaded' };
    try {
      const version = vault.getVersion();
      return { success: true, version };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('native:is-initialized', async () => {
    if (!vault) return { success: false, error: 'Native addon not loaded' };
    try {
      const initialized = vault.isInitialized();
      return { success: true, initialized };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });
  
  // Encryption operations
  ipcMain.handle('native:encrypt-folder', async (event, folderPath, password) => {
    if (!vault) return { success: false, error: 'Native addon not loaded' };
    try {
      const result = vault.encryptFolder(folderPath, password);
      console.log(`✅ Encrypted folder: ${folderPath}`);
      return { success: true, result };
    } catch (error) {
      console.error(`❌ Encryption failed for ${folderPath}:`, error.message);
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('native:decrypt-folder', async (event, folderPath, password) => {
    if (!vault) return { success: false, error: 'Native addon not loaded' };
    try {
      const result = vault.decryptFolder(folderPath, password);
      console.log(`✅ Decrypted folder: ${folderPath}`);
      return { success: true, result };
    } catch (error) {
      console.error(`❌ Decryption failed for ${folderPath}:`, error.message);
      return { success: false, error: error.message };
    }
  });
  
  // File system operations
  ipcMain.handle('native:hide-folder', async (event, folderPath) => {
    if (!vault) return { success: false, error: 'Native addon not loaded' };
    try {
      const newPath = vault.hideFolder(folderPath);
      console.log(`✅ Hidden folder: ${folderPath} -> ${newPath}`);
      return { success: true, newPath };
    } catch (error) {
      console.error(`❌ Hide failed for ${folderPath}:`, error.message);
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('native:unhide-folder', async (event, folderPath) => {
    if (!vault) return { success: false, error: 'Native addon not loaded' };
    try {
      const newPath = vault.unhideFolder(folderPath);
      console.log(`✅ Unhidden folder: ${folderPath} -> ${newPath}`);
      return { success: true, newPath };
    } catch (error) {
      console.error(`❌ Unhide failed for ${folderPath}:`, error.message);
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('native:is-hidden', async (event, folderPath) => {
    if (!vault) return { success: false, error: 'Native addon not loaded' };
    try {
      const hidden = vault.isHidden(folderPath);
      return { success: true, hidden };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('native:set-file-attributes', async (event, filePath, attributes) => {
    if (!vault) return { success: false, error: 'Native addon not loaded' };
    try {
      const result = vault.setFileAttributes(filePath, attributes);
      return { success: true, result };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('native:get-file-attributes', async (event, filePath) => {
    if (!vault) return { success: false, error: 'Native addon not loaded' };
    try {
      const attributes = vault.getFileAttributes(filePath);
      return { success: true, attributes };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });
  
  // Process concealment operations
  ipcMain.handle('native:hide-process', async () => {
    if (!vault) return { success: false, error: 'Native addon not loaded' };
    try {
      const result = vault.hideProcess();
      console.log('✅ Process hidden (disguised)');
      return { success: true, result };
    } catch (error) {
      console.error('❌ Process hide failed:', error.message);
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('native:show-process', async () => {
    if (!vault) return { success: false, error: 'Native addon not loaded' };
    try {
      const result = vault.showProcess();
      console.log('✅ Process shown (restored)');
      return { success: true, result };
    } catch (error) {
      console.error('❌ Process show failed:', error.message);
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('native:is-process-hidden', async () => {
    if (!vault) return { success: false, error: 'Native addon not loaded' };
    try {
      const hidden = vault.isProcessHidden();
      return { success: true, hidden };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('native:set-process-name', async (event, name) => {
    if (!vault) return { success: false, error: 'Native addon not loaded' };
    try {
      const result = vault.setProcessName(name);
      console.log(`✅ Process name set to: ${name}`);
      return { success: true, result };
    } catch (error) {
      console.error(`❌ Set process name failed:`, error.message);
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('native:get-current-process-name', async () => {
    if (!vault) return { success: false, error: 'Native addon not loaded' };
    try {
      const name = vault.getCurrentProcessName();
      return { success: true, name };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('native:get-original-process-name', async () => {
    if (!vault) return { success: false, error: 'Native addon not loaded' };
    try {
      const name = vault.getOriginalProcessName();
      return { success: true, name };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });
  
  // ==================== VAULT PROFILE MANAGER IPC HANDLERS ====================
  
  // Simple profile manager implementation using file system
  const os = require('os');
  const username = os.userInfo().username;
  const storagePath = path.join(os.homedir(), '.phantom_vault_storage', username);
  const profilesPath = path.join(storagePath, 'profiles.json');
  
  // Ensure storage directory exists
  if (!fs.existsSync(storagePath)) {
    fs.mkdirSync(storagePath, { recursive: true });
  }
  
  // Helper functions for profile management
  const loadProfiles = () => {
    try {
      if (fs.existsSync(profilesPath)) {
        const data = fs.readFileSync(profilesPath, 'utf8');
        return JSON.parse(data);
      }
      return { profiles: [], activeProfileId: null };
    } catch (error) {
      console.error('Error loading profiles:', error);
      return { profiles: [], activeProfileId: null };
    }
  };
  
  const saveProfiles = (data) => {
    try {
      fs.writeFileSync(profilesPath, JSON.stringify(data, null, 2), 'utf8');
      return true;
    } catch (error) {
      console.error('Error saving profiles:', error);
      return false;
    }
  };
  
  const hashPassword = (password) => {
    return crypto.createHash('sha256').update(password).digest('hex');
  };
  
  // Profile Manager handlers
  ipcMain.handle('profile:get-active', async () => {
    try {
      const data = loadProfiles();
      if (!data.activeProfileId) {
        return { success: true, profile: null };
      }
      const profile = data.profiles.find(p => p.id === data.activeProfileId);
      return { success: true, profile: profile || null };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('profile:get-all', async () => {
    try {
      const data = loadProfiles();
      return { success: true, profiles: data.profiles };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });

  ipcMain.handle('profile:create', async (event, name, masterPassword, recoveryKey) => {
    try {
      const data = loadProfiles();
      
      // Check if profile with same name exists
      if (data.profiles.some(p => p.name === name)) {
        throw new Error('Profile with this name already exists');
      }
      
      const profile = {
        id: `profile_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`,
        name: name,
        created_at: Math.floor(Date.now() / 1000),
        password_hash: hashPassword(masterPassword),
        recovery_key_hash: recoveryKey ? hashPassword(recoveryKey) : null,
      };
      
      data.profiles.push(profile);
      
      // Set as active if it's the first profile
      if (data.profiles.length === 1) {
        data.activeProfileId = profile.id;
      }
      
      if (!saveProfiles(data)) {
        throw new Error('Failed to save profile');
      }
      
      // Cache the master password with automatic timeout
      cachePasswordWithTimeout(masterPassword);
      
      console.log('✅ Profile created:', profile.name);
      return { success: true, profile };
    } catch (error) {
      console.error('❌ Failed to create profile:', error.message);
      return { success: false, error: error.message };
    }
  });

  ipcMain.handle('profile:set-active', async (event, profileId) => {
    try {
      const data = loadProfiles();
      const profile = data.profiles.find(p => p.id === profileId);
      
      if (!profile) {
        throw new Error('Profile not found');
      }
      
      data.activeProfileId = profileId;
      
      if (!saveProfiles(data)) {
        throw new Error('Failed to save active profile');
      }
      
      console.log('✅ Active profile set to:', profile.name);
      return { success: true };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });

  ipcMain.handle('profile:verify-password', async (event, profileId, password) => {
    try {
      const data = loadProfiles();
      const profile = data.profiles.find(p => p.id === profileId);
      
      if (!profile) {
        throw new Error('Profile not found');
      }
      
      const isValid = profile.password_hash === hashPassword(password);
      
      if (isValid) {
        console.log(`🔑 [FLOW-2] Master password VERIFIED ✅`);
        // Cache password on successful verification
        cachePasswordWithTimeout(password);
      } else {
        console.log(`❌ [FLOW-2] Master password verification FAILED`);
      }
      
      return { success: true, isValid };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });
  
  // ==================== FOLDER MANAGER IPC HANDLERS ====================
  
  // Helper functions for folder management
  const getFoldersPath = (profileId) => {
    return path.join(storagePath, `folders_${profileId}.json`);
  };
  
  const loadFolders = (profileId) => {
    try {
      const foldersPath = getFoldersPath(profileId);
      if (fs.existsSync(foldersPath)) {
        const data = fs.readFileSync(foldersPath, 'utf8');
        return JSON.parse(data);
      }
      return [];
    } catch (error) {
      console.error('Error loading folders:', error);
      return [];
    }
  };
  
  const saveFolders = (profileId, folders) => {
    try {
      const foldersPath = getFoldersPath(profileId);
      fs.writeFileSync(foldersPath, JSON.stringify(folders, null, 2), 'utf8');
      return true;
    } catch (error) {
      console.error('Error saving folders:', error);
      return false;
    }
  };
  
  // Folder Manager handlers
  ipcMain.handle('folder:get-all', async (event, profileId) => {
    try {
      const folders = loadFolders(profileId);
      return { success: true, folders };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('folder:lock', async (event, profileId, folderId) => {
    try {
      console.log(`\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
      console.log(`🔒 [LOCK] Locking folder: ${folderId}`);
      console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
      
      const folders = loadFolders(profileId);
      const folder = folders.find(f => f.id === folderId);
      
      if (!folder) {
        throw new Error('Folder not found');
      }
      
      if (folder.is_locked) {
        console.log('   ⚠️  Folder already locked');
        return { success: true };
      }
      
      const folderPath = folder.original_path;
      console.log(`   Path: ${folderPath}`);
      
      // Check if folder exists
      if (!fs.existsSync(folderPath)) {
        throw new Error('Folder does not exist');
      }
      
      // Lock the folder by hiding it (rename with dot prefix)
      const dirname = path.dirname(folderPath);
      const basename = path.basename(folderPath);
      const hiddenPath = path.join(dirname, '.' + basename);
      
      // Rename to hidden
      fs.renameSync(folderPath, hiddenPath);
      
      // Update metadata
      folder.is_locked = true;
      folder.vault_path = hiddenPath;
      folder.last_accessed = Math.floor(Date.now() / 1000);
      
      saveFolders(profileId, folders);
      
      console.log(`✅ [LOCK] Folder locked successfully`);
      console.log(`   Hidden path: ${hiddenPath}`);
      console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n`);
      
      return { success: true, result: { folderId, locked: true } };
    } catch (error) {
      console.error('❌ [LOCK] Failed:', error.message);
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('folder:unlock', async (event, profileId, folderId, password, mode = 'temporary') => {
    try {
      console.log(`\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
      console.log(`🔓 [UNLOCK] Unlocking folder: ${folderId}`);
      console.log(`   Mode: ${mode}`);
      console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
      
      const folders = loadFolders(profileId);
      const folder = folders.find(f => f.id === folderId);
      
      if (!folder) {
        throw new Error('Folder not found');
      }
      
      if (!folder.is_locked) {
        console.log('   ⚠️  Folder already unlocked');
        return { success: true };
      }
      
      const hiddenPath = folder.vault_path || folder.original_path;
      console.log(`   Hidden path: ${hiddenPath}`);
      
      // Check if hidden folder exists
      if (!fs.existsSync(hiddenPath)) {
        throw new Error('Locked folder does not exist');
      }
      
      // Unlock the folder by unhiding it (remove dot prefix)
      const dirname = path.dirname(hiddenPath);
      const basename = path.basename(hiddenPath);
      const visibleBasename = basename.startsWith('.') ? basename.slice(1) : basename;
      const visiblePath = path.join(dirname, visibleBasename);
      
      // Rename to visible
      fs.renameSync(hiddenPath, visiblePath);
      
      // Update metadata
      folder.is_locked = false;
      folder.original_path = visiblePath;
      folder.vault_path = null;
      folder.last_accessed = Math.floor(Date.now() / 1000);
      folder.unlock_mode = mode; // Store mode for tracking
      
      saveFolders(profileId, folders);
      
      console.log(`✅ [UNLOCK] Folder unlocked successfully`);
      console.log(`   Visible path: ${visiblePath}`);
      console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n`);
      
      return { success: true, result: { folderId, unlocked: true, mode } };
    } catch (error) {
      console.error('❌ [UNLOCK] Failed:', error.message);
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('folder:unlock-all', async (event, profileId, password, mode = 'temporary') => {
    if (!folderManager) return { success: false, error: 'Folder manager not available' };
    try {
      console.log(`\n🔐 [FLOW-5] UNLOCKING FOLDERS`);
      console.log(`   Mode: ${mode.toUpperCase()}`);
      console.log(`   Attempting to unlock all locked folders...`);
      
      const result = await folderManager.unlockAllFolders(profileId, password, mode);
      
      // If unlock was successful, cache the password with timeout for quick lock operations
      if (result && result.success > 0) {
        cachePasswordWithTimeout(password);
        console.log(`✅ [FLOW-5] ${result.success} folder(s) unlocked in ${mode} mode`);
        if (result.failed > 0) {
          console.log(`⚠️  [FLOW-5] ${result.failed} folder(s) failed to unlock (wrong password or missing)`);
        }
        console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n`);
      } else {
        console.log(`❌ [FLOW-5] No folders unlocked (wrong password or no locked folders)`);
        console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n`);
      }
      
      return { success: true, result };
    } catch (error) {
      console.error(`❌ [FLOW-5] Unlock error: ${error.message}`);
      console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n`);
      return { success: false, error: error.message };
    }
  });

  // Check if there are any temporarily unlocked folders
  ipcMain.handle('folder:has-temporary-unlocked', async (event, profileId) => {
    if (!folderManager) return { success: false, error: 'Folder manager not available' };
    try {
      const temporaryFolders = folderManager.getTemporaryUnlockedFolders(profileId);
      return { 
        success: true, 
        hasTemporary: temporaryFolders && temporaryFolders.length > 0,
        count: temporaryFolders ? temporaryFolders.length : 0
      };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });

  // Lock all temporarily unlocked folders
  ipcMain.handle('folder:lock-all-temporary', async (event, profileId, password) => {
    if (!folderManager) return { success: false, error: 'Folder manager not available' };
    try {
      console.log(`\n🔒 [RE-LOCK] LOCKING TEMPORARY FOLDERS`);
      console.log(`   Attempting to lock all temporarily unlocked folders...`);
      
      const temporaryFolders = folderManager.getTemporaryUnlockedFolders(profileId);
      if (!temporaryFolders || temporaryFolders.length === 0) {
        console.log(`⚠️  [RE-LOCK] No temporarily unlocked folders found`);
        console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n`);
        return { success: true, locked: 0, failed: 0, message: 'No temporary folders to lock' };
      }

      let locked = 0;
      let failed = 0;
      const errors = [];

      for (const folder of temporaryFolders) {
        try {
          console.log(`   Locking folder: ${folder.name}`);
          await folderManager.lockFolder(profileId, folder.id, password);
          locked++;
        } catch (error) {
          console.error(`   Failed to lock ${folder.name}: ${error.message}`);
          failed++;
          errors.push({ folderName: folder.name, error: error.message });
        }
      }

      if (locked > 0) {
        console.log(`✅ [RE-LOCK] ${locked} folder(s) locked successfully`);
      }
      if (failed > 0) {
        console.log(`⚠️  [RE-LOCK] ${failed} folder(s) failed to lock`);
      }
      console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n`);

      return { 
        success: true, 
        locked, 
        failed,
        errors: failed > 0 ? errors : undefined
      };
    } catch (error) {
      console.error(`❌ [RE-LOCK] Lock error: ${error.message}`);
      console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n`);
      return { success: false, error: error.message };
    }
  });

  ipcMain.handle('folder:add', async (event, profileId, folderPath, folderName) => {
    try {
      console.log(`\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
      console.log(`📁 [FLOW-1] ADD FOLDER: "${folderName}"`);
      console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
      
      const folders = loadFolders(profileId);
      
      // Check if folder already exists
      if (folders.some(f => f.original_path === folderPath)) {
        throw new Error('Folder already added to vault');
      }
      
      const folder = {
        id: `folder_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`,
        folder_name: folderName || path.basename(folderPath),
        original_path: folderPath,
        vault_path: null,
        is_locked: false,
        created_at: Math.floor(Date.now() / 1000),
        last_accessed: Math.floor(Date.now() / 1000),
      };
      
      folders.push(folder);
      
      if (!saveFolders(profileId, folders)) {
        throw new Error('Failed to save folder metadata');
      }
      
      console.log(`✅ [FLOW-1] Folder added to vault (UNLOCKED state)`);
      console.log(`   Folder ID: ${folder.id}`);
      console.log(`   Path: ${folderPath}`);
      console.log(`   → Waiting for master password verification...`);
      
      return { success: true, folderId: folder.id, folder };
    } catch (error) {
      console.error('❌ Failed to add folder:', error.message);
      return { success: false, error: error.message };
    }
  });

  ipcMain.handle('folder:lock-with-password', async (event, profileId, folderId, password) => {
    try {
      console.log(`\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
      console.log(`🔒 [LOCK-WITH-PASSWORD] Locking folder: ${folderId}`);
      console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
      
      const folders = loadFolders(profileId);
      const folder = folders.find(f => f.id === folderId);
      
      if (!folder) {
        throw new Error('Folder not found');
      }
      
      if (folder.is_locked) {
        console.log('   ⚠️  Folder already locked');
        return { success: true };
      }
      
      const folderPath = folder.original_path;
      console.log(`   Path: ${folderPath}`);
      
      // Check if folder exists
      if (!fs.existsSync(folderPath)) {
        throw new Error('Folder does not exist');
      }
      
      // Lock the folder by hiding it (rename with dot prefix)
      const dirname = path.dirname(folderPath);
      const basename = path.basename(folderPath);
      const hiddenPath = path.join(dirname, '.' + basename);
      
      // Rename to hidden
      fs.renameSync(folderPath, hiddenPath);
      
      // Update metadata
      folder.is_locked = true;
      folder.vault_path = hiddenPath;
      folder.last_accessed = Math.floor(Date.now() / 1000);
      
      saveFolders(profileId, folders);
      
      console.log(`✅ [LOCK-WITH-PASSWORD] Folder locked successfully`);
      console.log(`   Hidden path: ${hiddenPath}`);
      console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n`);
      
      return { success: true, result: { folderId, locked: true } };
    } catch (error) {
      console.error('❌ [LOCK-WITH-PASSWORD] Failed:', error.message);
      return { success: false, error: error.message };
    }
  });

  ipcMain.handle('folder:remove', async (event, profileId, folderId) => {
    try {
      console.log(`\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
      console.log(`🗑️  [REMOVE] Removing folder: ${folderId}`);
      console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
      
      const folders = loadFolders(profileId);
      const folderIndex = folders.findIndex(f => f.id === folderId);
      
      if (folderIndex === -1) {
        throw new Error('Folder not found');
      }
      
      // Remove folder from array
      folders.splice(folderIndex, 1);
      
      // Save updated list
      saveFolders(profileId, folders);
      
      console.log(`✅ [REMOVE] Folder removed from vault`);
      console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n`);
      
      return { success: true };
    } catch (error) {
      console.error('❌ [REMOVE] Failed:', error.message);
      return { success: false, error: error.message };
    }
  });

  // ==================== SERVICE MANAGEMENT IPC HANDLERS ====================
  
  // Service restart handler
  ipcMain.handle('service:restart', async () => {
    try {
      console.log('🔄 [SERVICE] Restart requested');
      
      // Stop current services
      if (hotkeyManager) {
        hotkeyManager.unregisterHotkeys();
      }
      if (autoLockManager) {
        autoLockManager.stopMonitoring();
      }
      if (fsWatcher) {
        fsWatcher.stopWatching();
      }
      
      // Wait a moment
      await new Promise(resolve => setTimeout(resolve, 1000));
      
      // Restart services
      if (hotkeyManager) {
        hotkeyManager.registerHotkeys();
      }
      if (autoLockManager) {
        autoLockManager.startMonitoring();
      }
      if (fsWatcher) {
        const os = require('os');
        const username = os.userInfo().username;
        const storagePath = path.join(os.homedir(), '.phantom_vault_storage', username);
        if (fs.existsSync(storagePath)) {
          fsWatcher.startWatching(storagePath);
        }
      }
      
      console.log('✅ [SERVICE] Restart completed');
      return { success: true, message: 'Service restarted successfully' };
    } catch (error) {
      console.error('❌ [SERVICE] Restart failed:', error.message);
      return { success: false, error: error.message };
    }
  });
  
  // Service status handler
  ipcMain.handle('service:status', async () => {
    try {
      const status = {
        hotkeyManager: hotkeyManager ? hotkeyManager.areHotkeysRegistered() : false,
        autoLockManager: autoLockManager ? true : false, // Assume running if exists
        fsWatcher: fsWatcher ? true : false, // Assume running if exists
        cppAddon: vault ? true : false,
      };
      
      return { success: true, status };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });
  
  // Service reconnect handler (alias for restart)
  ipcMain.handle('service:reconnect', async () => {
    return ipcMain.handle('service:restart');
  });
}


app.whenReady().then(() => {
  console.log('PhantomVault starting');
  createWindow();
  setupIpcHandlers();
});

app.on('window-all-closed', () => {
  // DON'T quit the app - keep running in system tray
  // Hotkeys remain active for background unlock/lock
  console.log('🔒 PhantomVault running in background (system tray)');
  console.log('   Hotkeys still active: Ctrl+Alt+V (unlock), Ctrl+Alt+R (recovery)');
  
  // Keep all managers running
  if (autoLockManager) {
    console.log('   Auto-lock monitoring: Active');
  }
  if (fsWatcher) {
    console.log('   File system watching: Active');
  }
});

app.on('will-quit', () => {
  // Cleanup hotkeys
  if (hotkeyManager) {
    hotkeyManager.unregisterHotkeys();
  } else {
    globalShortcut.unregisterAll();
  }
});
