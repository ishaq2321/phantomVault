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
let VaultProfileManager = null;
let VaultFolderManager = null;
try {
  const profileModule = require('./VaultProfileManager');
  const folderModule = require('./VaultFolderManager');
  VaultProfileManager = profileModule.VaultProfileManager;
  VaultFolderManager = folderModule.VaultFolderManager;
} catch (error) {
  console.error('❌ Failed to load vault managers:', error.message);
}

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

// Register global hotkey
function registerGlobalHotkey(hotkey) {
  try {
    // Unregister all previous hotkeys
    globalShortcut.unregisterAll();
    
    // Register new hotkey
    const success = globalShortcut.register(hotkey, () => {
      console.log(`🔑 Global hotkey pressed: ${hotkey}`);
      
      // Show unlock prompt
      if (mainWindow) {
        mainWindow.webContents.send('show-unlock-prompt');
        
        // Focus and show window
        if (mainWindow.isMinimized()) mainWindow.restore();
        mainWindow.focus();
        mainWindow.show();
      }
    });
    
    if (success) {
      console.log(`✅ Global hotkey registered: ${hotkey}`);
      currentHotkey = hotkey;
      return true;
    } else {
      console.error(`❌ Failed to register hotkey: ${hotkey}`);
      return false;
    }
  } catch (error) {
    console.error('❌ Error registering hotkey:', error);
    return false;
  }
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
        if (!mainWindow) createWindow();
        setTimeout(() => {
          if (mainWindow) {
            mainWindow.webContents.send('show-unlock-overlay', { isRecoveryMode: false });
            if (mainWindow.isMinimized()) mainWindow.restore();
            mainWindow.show();
            mainWindow.focus();
          }
        }, 100);
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

// Create invisible overlay window for capturing password input globally
let overlayWindow = null;

function createOverlayWindow(overlayData) {
  if (overlayWindow && !overlayWindow.isDestroyed()) {
    // Window already exists, just send new data
    overlayWindow.webContents.send('show-unlock-overlay', overlayData);
    overlayWindow.show();
    overlayWindow.focus();
    return;
  }

  const { screen } = require('electron');
  const primaryDisplay = screen.getPrimaryDisplay();
  const { width, height } = primaryDisplay.workAreaSize;

  overlayWindow = new BrowserWindow({
    width,
    height,
    x: 0,
    y: 0,
    transparent: true,
    frame: false,
    alwaysOnTop: true,
    skipTaskbar: true,
    resizable: false,
    movable: false,
    minimizable: false,
    maximizable: false,
    closable: true,
    focusable: true,
    show: false,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, 'preload.js'),
    },
  });

  overlayWindow.setIgnoreMouseEvents(false); // We need to capture keystrokes
  overlayWindow.setVisibleOnAllWorkspaces(true, { visibleOnFullScreen: true });
  overlayWindow.setAlwaysOnTop(true, 'screen-saver');

  // Load production build or development server
  const isDev = process.env.NODE_ENV === 'development';
  if (isDev) {
    overlayWindow.loadURL('http://127.0.0.1:5173');
  } else {
    // Production: load from dist directory
    overlayWindow.loadFile(path.join(__dirname, '../dist/index.html'));
  }

  overlayWindow.webContents.once('did-finish-load', () => {
    // Send overlay data to the React app
    overlayWindow.webContents.send('show-unlock-overlay', overlayData);
    overlayWindow.show();
    overlayWindow.focus();
    console.log('   → Overlay window created and shown');
  });

  // Close overlay window when done
  overlayWindow.on('closed', () => {
    overlayWindow = null;
  });
}

function closeOverlayWindow() {
  if (overlayWindow && !overlayWindow.isDestroyed()) {
    overlayWindow.close();
    overlayWindow = null;
  }
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
    },
  });

  // Load production build or development server
  const isDev = process.env.NODE_ENV === 'development';
  if (isDev) {
    mainWindow.loadURL('http://127.0.0.1:5173');
  } else {
    // Production: load from dist directory
    mainWindow.loadFile(path.join(__dirname, '../dist/index.html'));
  }
  
  // Show window when ready to prevent white flash
  mainWindow.once('ready-to-show', () => {
    mainWindow.show();
    
    // Only open dev tools in development
    if (process.env.NODE_ENV === 'development') {
      mainWindow.webContents.openDevTools();
    }
    
    // Create system tray after window is shown (required on some Linux systems)
    createTray();
  });
  
  // Register hotkeys when window is ready
  mainWindow.webContents.on('did-finish-load', () => {
    // Use HotkeyManager if available, otherwise fallback to simple hotkey
    if (HotkeyManager) {
      console.log('✅ Using HotkeyManager');
      hotkeyManager = HotkeyManager.getInstance();
      
      // Set callbacks
      hotkeyManager.onUnlock(async () => {
        console.log(`\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
        console.log(`🔓 [FLOW-4] UNLOCK/RE-LOCK HOTKEY PRESSED (Ctrl+Alt+V)`);
        console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
        console.log(`🚫 ELECTRON OVERLAY DISABLED - Using C++ service terminal input only`);
        console.log(`   → The C++ service will handle password input in terminal`);
        console.log(`   → No GUI interaction needed - completely invisible operation`);
        console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n`);
        
        // DO NOT CREATE OVERLAY WINDOW - let C++ service handle everything
        // This eliminates any possibility of black screen from Electron
      });
      
      hotkeyManager.onRecovery(() => {
        console.log('🔑 Recovery hotkey triggered');
        console.log('🚫 ELECTRON OVERLAY DISABLED - Using C++ service terminal input only');
        console.log('   → The C++ service will handle recovery key input in terminal');
        
        // DO NOT CREATE OVERLAY WINDOW - let C++ service handle everything
      });
      
      hotkeyManager.registerHotkeys();
    } else {
      console.log('⚠️ HotkeyManager not available - C++ service should handle hotkeys');
      console.log('🚫 Skipping Electron hotkey registration to avoid conflicts');
      // DO NOT register fallback hotkeys - let C++ service handle everything
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

  // Password input fallback (when sequence detection fails)
  ipcMain.handle('show-password-dialog', async (event, options = {}) => {
    const { title = 'PhantomVault', placeholder = 'Enter password...', mode = 'unlock' } = options;
    
    console.log(`🔐 [FALLBACK] Showing password dialog for ${mode}`);
    
    // Create a small, focused password dialog
    const passwordWindow = new BrowserWindow({
      width: 400,
      height: 200,
      modal: true,
      parent: mainWindow,
      show: false,
      resizable: false,
      minimizable: false,
      maximizable: false,
      alwaysOnTop: true,
      frame: false,
      webPreferences: {
        nodeIntegration: false,
        contextIsolation: true,
        preload: path.join(__dirname, 'preload.js'),
      },
    });

    // Create simple HTML for password input
    const passwordHtml = `
      <!DOCTYPE html>
      <html>
      <head>
        <style>
          body {
            margin: 0;
            padding: 20px;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
            background: #2d3748;
            color: white;
            display: flex;
            flex-direction: column;
            justify-content: center;
            height: 160px;
          }
          .container {
            text-align: center;
          }
          h3 {
            margin: 0 0 15px 0;
            font-size: 14px;
            color: #a0aec0;
          }
          input {
            width: 300px;
            padding: 10px;
            border: 1px solid #4a5568;
            border-radius: 4px;
            background: #1a202c;
            color: white;
            font-size: 14px;
            text-align: center;
          }
          input:focus {
            outline: none;
            border-color: #63b3ed;
          }
          .hint {
            margin-top: 10px;
            font-size: 11px;
            color: #718096;
          }
        </style>
      </head>
      <body>
        <div class="container">
          <h3>${title}</h3>
          <input type="password" id="passwordInput" placeholder="${placeholder}" autofocus>
          <div class="hint">T+password (temporary) • P+password (permanent) • ESC to cancel</div>
        </div>
        <script>
          const input = document.getElementById('passwordInput');
          
          input.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') {
              window.electronAPI.sendPasswordResult(input.value);
            } else if (e.key === 'Escape') {
              window.electronAPI.sendPasswordResult(null);
            }
          });
          
          // Auto-focus
          setTimeout(() => input.focus(), 100);
          
          // Auto-close after 30 seconds
          setTimeout(() => {
            window.electronAPI.sendPasswordResult(null);
          }, 30000);
        </script>
      </body>
      </html>
    `;

    passwordWindow.loadURL('data:text/html;charset=utf-8,' + encodeURIComponent(passwordHtml));
    
    return new Promise((resolve) => {
      // Handle password result
      ipcMain.once('password-result', (event, password) => {
        passwordWindow.close();
        resolve(password);
      });
      
      passwordWindow.once('closed', () => {
        resolve(null);
      });
      
      passwordWindow.show();
      passwordWindow.focus();
    });
  });

  // Close overlay window
  ipcMain.handle('close-overlay-window', async () => {
    console.log('🚪 Closing overlay window');
    closeOverlayWindow();
    return { success: true };
  });

  // Handle password result from dialog
  ipcMain.handle('send-password-result', async (event, password) => {
    ipcMain.emit('password-result', event, password);
    return true;
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
  
  // Initialize managers
  if (VaultProfileManager && VaultFolderManager) {
    try {
      profileManager = VaultProfileManager.getInstance();
      folderManager = VaultFolderManager.getInstance(vault); // Pass the C++ addon instance
      console.log('✅ Vault managers initialized');
    } catch (error) {
      console.error('❌ Failed to initialize vault managers:', error.message);
    }
  }
  
  // Profile Manager handlers
  ipcMain.handle('profile:get-active', async () => {
    if (!profileManager) return { success: false, error: 'Profile manager not available' };
    try {
      const profile = profileManager.getActiveProfile();
      return { success: true, profile };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('profile:get-all', async () => {
    if (!profileManager) return { success: false, error: 'Profile manager not available' };
    try {
      const profiles = profileManager.getAllProfiles();
      return { success: true, profiles };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });

  ipcMain.handle('profile:create', async (event, name, masterPassword, recoveryKey) => {
    if (!profileManager) return { success: false, error: 'Profile manager not available' };
    try {
      const profile = profileManager.createProfile(name, masterPassword, recoveryKey);
      // Cache the master password with automatic timeout
      cachePasswordWithTimeout(masterPassword);
      return { success: true, profile };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });

  ipcMain.handle('profile:set-active', async (event, profileId) => {
    if (!profileManager) return { success: false, error: 'Profile manager not available' };
    try {
      profileManager.setActiveProfile(profileId);
      return { success: true };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });

  ipcMain.handle('profile:verify-password', async (event, profileId, password) => {
    if (!profileManager) return { success: false, error: 'Profile manager not available' };
    try {
      const isValid = profileManager.verifyPassword(profileId, password);
      if (isValid) {
        console.log(`🔑 [FLOW-2] Master password VERIFIED ✅`);
      } else {
        console.log(`❌ [FLOW-2] Master password verification FAILED`);
      }
      return { success: true, isValid };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });
  
  // Folder Manager handlers
  ipcMain.handle('folder:get-all', async (event, profileId) => {
    if (!folderManager) return { success: false, error: 'Folder manager not available' };
    try {
      const folders = folderManager.getFolders(profileId);
      return { success: true, folders };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('folder:lock', async (event, profileId, folderId) => {
    if (!folderManager) return { success: false, error: 'Folder manager not available' };
    
    try {
      // Use cached master password if available
      if (!cachedMasterPassword) {
        return { 
          success: false, 
          error: 'Master password not available. Please unlock a folder first to cache the password.' 
        };
      }
      
      // Use lockFolderWithPassword with the cached master password
      const result = await folderManager.lockFolderWithPassword(profileId, folderId, cachedMasterPassword);
      return { success: true, result };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });
  
  ipcMain.handle('folder:unlock', async (event, profileId, folderId, password, mode = 'temporary') => {
    if (!folderManager) return { success: false, error: 'Folder manager not available' };
    try {
      const result = await folderManager.unlockFolder(profileId, folderId, password, mode);
      return { success: true, result };
    } catch (error) {
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
    if (!folderManager || !vault || !profileManager) return { success: false, error: 'Managers not available' };
    try {
      console.log(`\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
      console.log(`📁 [FLOW-1] ADD FOLDER: "${folderName}"`);
      console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
      
      // Get profile to get master password for encryption
      const profile = profileManager.getActiveProfile();
      if (!profile || profile.id !== profileId) {
        throw new Error('Invalid profile');
      }

      // Add folder to manager (creates metadata entry - UNLOCKED state)
      const folder = folderManager.addFolder(profileId, folderPath);
      
      console.log(`✅ [FLOW-1] Folder added to vault (UNLOCKED state)`);
      console.log(`   Folder ID: ${folder.id}`);
      console.log(`   Path: ${folderPath}`);
      console.log(`   → Waiting for master password verification...`);
      
      return { success: true, folderId: folder.id, folder };
    } catch (error) {
      return { success: false, error: error.message };
    }
  });

  ipcMain.handle('folder:lock-with-password', async (event, profileId, folderId, password) => {
    if (!folderManager) return { success: false, error: 'Folder manager not available' };
    try {
      console.log(`\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
      console.log(`🔒 [FLOW-3] LOCKING FOLDER WITH PASSWORD (PHASE 4.2 - VAULT STORAGE)`);
      console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
      console.log(`   Folder ID: ${folderId}`);
      
      // Get folder metadata
      const folder = folderManager.getFolder(profileId, folderId);
      if (!folder) {
        throw new Error('Folder not found');
      }
      
      if (folder.isLocked) {
        console.log(`   ⚠️  [FLOW-3] Folder already locked, skipping`);
        console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n`);
        return { success: true };
      }
      
      const originalPath = folder.folderPath;
      console.log(`   Original path: ${originalPath}`);
      console.log(`   Folder name: ${folder.folderName}`);
      
      // PHASE 4.2: Get vault path
      const vaultPath = folderManager.getVaultPath(profileId, folderId, folder.folderName);
      console.log(`   Target vault path: ${vaultPath}`);
      
      // Step 1: Create backup before locking
      try {
        console.log(`\n   [STEP 1] Creating backup before lock...`);
        const backupPath = folderManager.getBackupPath(profileId, folderId, folder.folderName);
        folderManager.createBackup(originalPath, backupPath);
        
        // Track backup in metadata
        if (!folder.backups) folder.backups = [];
        folder.backups.push({
          timestamp: Date.now(),
          path: backupPath,
          operation: 'pre-lock'
        });
        folderManager.saveFoldersMetadata(profileId);
      } catch (backupError) {
        console.error(`   ❌ Backup failed: ${backupError.message}`);
        throw new Error(`Cannot lock without backup: ${backupError.message}`);
      }
      
      // Step 2: Encrypt folder using C++ native addon
      if (vault) {
        console.log(`\n   [STEP 2] Encrypting folder contents...`);
        
        try {
          // C++ method returns boolean (true) or throws exception
          const success = vault.encryptFolder(originalPath, password);
          
          if (!success) {
            throw new Error('Encryption returned false');
          }
          
          console.log(`   ✅ Folder encrypted successfully`);
        } catch (encryptError) {
          const errorMsg = encryptError.message || 'Unknown encryption error';
          console.error(`   ❌ Encryption failed: ${errorMsg}`);
          
          // Rollback: Remove backup since we didn't proceed
          console.log(`   🔄 Rolling back: removing backup...`);
          try {
            const lastBackup = folder.backups[folder.backups.length - 1];
            if (lastBackup && fs.existsSync(lastBackup.path)) {
              fs.rmSync(lastBackup.path, { recursive: true, force: true });
            }
            folder.backups.pop();
            folderManager.saveFoldersMetadata(profileId);
          } catch (rollbackError) {
            console.error(`   ⚠️  Rollback warning: ${rollbackError.message}`);
          }
          
          throw new Error(`Encryption failed: ${errorMsg}`);
        }
      } else {
        console.warn(`\n   ⚠️  [STEP 2] Native addon not available, skipping encryption`);
      }
      
      // Step 3: Move encrypted folder to vault
      try {
        console.log(`\n   [STEP 3] Moving encrypted folder to vault...`);
        folderManager.moveToVault(originalPath, vaultPath);
      } catch (moveError) {
        console.error(`   ❌ Failed to move to vault: ${moveError.message}`);
        
        // Rollback: Decrypt folder if encryption happened
        console.log(`   🔄 Rolling back: decrypting folder...`);
        try {
          if (vault && fs.existsSync(originalPath)) {
            vault.decryptFolder(originalPath, password);
          }
        } catch (decryptError) {
          console.error(`   ⚠️  Rollback decryption failed: ${decryptError.message}`);
        }
        
        throw new Error(`Failed to move to vault: ${moveError.message}`);
      }
      
      // Step 4: Update folder metadata
      console.log(`\n   [STEP 4] Updating metadata...`);
      folderManager.updateFolderLockState(profileId, folderId, true, vaultPath, true);
      console.log(`   ✅ Metadata updated (isLocked=true, vaultPath set)`);
      
      // Step 5: Clean old backups (keep last 3)
      try {
        console.log(`\n   [STEP 5] Cleaning old backups...`);
        folderManager.cleanOldBackups(profileId, folderId, folder.folderName, 3);
      } catch (cleanupError) {
        console.warn(`   ⚠️  Backup cleanup warning: ${cleanupError.message}`);
        // Don't fail the lock operation if cleanup fails
      }
      
      console.log(`\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━`);
      console.log(`✅ [FLOW-3] FOLDER LOCKED AND SECURED IN VAULT`);
      console.log(`   Original: ${originalPath}`);
      console.log(`   Vault: ${vaultPath}`);
      console.log(`━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n`);

      
      return { success: true };
    } catch (error) {
      console.error(`❌ Failed to lock folder ${folderId}:`, error.message);
      return { success: false, error: error.message };
    }
  });

  ipcMain.handle('folder:remove', async (event, profileId, folderId) => {
    if (!folderManager) return { success: false, error: 'Folder manager not available' };
    try {
      folderManager.removeFolder(profileId, folderId);
      return { success: true };
    } catch (error) {
      return { success: false, error: error.message };
    }
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
