/**
 * PhantomVault - Electron Main Process
 * 
 * Main process for the PhantomVault GUI application.
 * Handles window management, service communication, and system integration.
 */

import { app, BrowserWindow, ipcMain, Menu, shell, dialog } from 'electron';
import { join } from 'path';
import { spawn, ChildProcess } from 'child_process';
import { existsSync } from 'fs';

// Development mode detection
// Check if we're in a proper development environment (not just unpackaged)
const isDev = !app.isPackaged && process.env.NODE_ENV !== 'production' && existsSync(join(__dirname, '../../src'));
const isPackaged = app.isPackaged;

// Service process reference
let serviceProcess: ChildProcess | null = null;
let mainWindow: BrowserWindow | null = null;

/**
 * Create the main application window
 */
function createMainWindow(): void {
  mainWindow = new BrowserWindow({
    width: 1200,
    height: 800,
    minWidth: 800,
    minHeight: 600,
    show: false,
    icon: join(__dirname, '../assets/icon.png'),
    titleBarStyle: process.platform === 'darwin' ? 'hiddenInset' : 'default',
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: join(__dirname, 'preload.js'),
      webSecurity: !isDev,
    },
  });

  // Load the application
  if (isDev) {
    mainWindow.loadURL('http://localhost:5173');
    mainWindow.webContents.openDevTools();
  } else {
    // In production, load the built renderer files
    const rendererPath = join(__dirname, '../renderer/index.html');
    console.log('[Main] Loading renderer from:', rendererPath);
    mainWindow.loadFile(rendererPath);
  }

  // Show window when ready
  mainWindow.once('ready-to-show', () => {
    mainWindow?.show();
    
    // Only open dev tools in actual development mode
    if (isDev) {
      mainWindow?.webContents.openDevTools();
    }
  });

  // Handle window closed
  mainWindow.on('closed', () => {
    mainWindow = null;
  });

  // Handle external links
  mainWindow.webContents.setWindowOpenHandler(({ url }) => {
    shell.openExternal(url);
    return { action: 'deny' };
  });
}

/**
 * Start the PhantomVault service
 */
function startService(): Promise<boolean> {
  return new Promise((resolve) => {
    try {
      // Determine service executable path
      let servicePath: string;
      
      if (isDev) {
        // In development, use absolute path to the service
        const projectRoot = join(__dirname, '../..');
        servicePath = join(projectRoot, 'core/build/bin/phantomvault-service');
        console.log('[Main] Development service path:', servicePath);
      } else if (isPackaged) {
        // In packaged app, use resources path
        servicePath = join(process.resourcesPath, 'bin/phantomvault-service');
      } else {
        // In installed but unpackaged version (like our Linux installation)
        servicePath = '/opt/phantomvault/bin/phantomvault-service';
        console.log('[Main] Using system service path:', servicePath);
      }
      
      // Add .exe extension on Windows
      if (process.platform === 'win32' && !servicePath.endsWith('.exe')) {
        servicePath += '.exe';
      }
      
      // Check if service executable exists
      if (!existsSync(servicePath)) {
        console.error('[Main] Service executable not found:', servicePath);
        resolve(false);
        return;
      }
      
      console.log('[Main] Starting service:', servicePath);
      
      // Start service process
      serviceProcess = spawn(servicePath, ['--log-level', 'INFO'], {
        stdio: ['ignore', 'pipe', 'pipe'],
        detached: false,
      });
      
      // Handle service output
      serviceProcess.stdout?.on('data', (data) => {
        console.log('[Service]', data.toString().trim());
      });
      
      serviceProcess.stderr?.on('data', (data) => {
        console.error('[Service Error]', data.toString().trim());
      });
      
      // Handle service exit
      serviceProcess.on('exit', (code, signal) => {
        console.log(`[Main] Service exited with code ${code}, signal ${signal}`);
        serviceProcess = null;
      });
      
      serviceProcess.on('error', (error) => {
        console.error('[Main] Service error:', error);
        serviceProcess = null;
        resolve(false);
      });
      
      // Give service time to start
      setTimeout(() => {
        resolve(serviceProcess !== null);
      }, 2000);
      
    } catch (error) {
      console.error('[Main] Failed to start service:', error);
      resolve(false);
    }
  });
}

/**
 * Stop the PhantomVault service
 */
function stopService(): void {
  if (serviceProcess) {
    console.log('[Main] Stopping service...');
    serviceProcess.kill('SIGTERM');
    serviceProcess = null;
  }
}

/**
 * Check if running with admin privileges
 */
function checkAdminPrivileges(): boolean {
  // This is a simplified check - in production, we'd use platform-specific methods
  return process.getuid ? process.getuid() === 0 : true;
}

/**
 * Create application menu
 */
function createMenu(): void {
  const template: Electron.MenuItemConstructorOptions[] = [
    {
      label: 'PhantomVault',
      submenu: [
        {
          label: 'About PhantomVault',
          click: () => {
            dialog.showMessageBox(mainWindow!, {
              type: 'info',
              title: 'About PhantomVault',
              message: 'PhantomVault v1.0.0',
              detail: 'Invisible Folder Security with Profile-Based Management\n\nBuilt with Electron and C++\nCopyright © 2025 PhantomVault Team',
            });
          },
        },
        { type: 'separator' },
        { role: 'services' },
        { type: 'separator' },
        { role: 'hide' },
        { role: 'hideOthers' },
        { role: 'unhide' },
        { type: 'separator' },
        { role: 'quit' },
      ],
    },
    {
      label: 'Edit',
      submenu: [
        { role: 'undo' },
        { role: 'redo' },
        { type: 'separator' },
        { role: 'cut' },
        { role: 'copy' },
        { role: 'paste' },
        { role: 'selectAll' },
      ],
    },
    {
      label: 'View',
      submenu: [
        { role: 'reload' },
        { role: 'forceReload' },
        { role: 'toggleDevTools' },
        { type: 'separator' },
        { role: 'resetZoom' },
        { role: 'zoomIn' },
        { role: 'zoomOut' },
        { type: 'separator' },
        { role: 'togglefullscreen' },
      ],
    },
    {
      label: 'Window',
      submenu: [
        { role: 'minimize' },
        { role: 'close' },
      ],
    },
    {
      label: 'Help',
      submenu: [
        {
          label: 'Documentation',
          click: () => {
            shell.openExternal('https://github.com/ishaq2321/phantomVault');
          },
        },
        {
          label: 'Report Issue',
          click: () => {
            shell.openExternal('https://github.com/ishaq2321/phantomVault/issues');
          },
        },
      ],
    },
  ];

  const menu = Menu.buildFromTemplate(template);
  Menu.setApplicationMenu(menu);
}

/**
 * App event handlers
 */
app.whenReady().then(async () => {
  console.log('[Main] App ready, starting PhantomVault...');
  
  // Check admin privileges
  const isAdmin = checkAdminPrivileges();
  console.log('[Main] Admin privileges:', isAdmin);
  
  // Start service
  const serviceStarted = await startService();
  if (!serviceStarted) {
    console.error('[Main] Failed to start service');
    dialog.showErrorBox(
      'Service Error',
      'Failed to start PhantomVault service. Please check your installation.'
    );
  }
  
  // Create main window
  createMainWindow();
  
  // Create menu
  createMenu();
  
  // Handle app activation (macOS)
  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createMainWindow();
    }
  });
});

// Handle all windows closed
app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

// Handle app quit
app.on('before-quit', () => {
  console.log('[Main] App quitting, stopping service...');
  stopService();
});

// Security: Prevent new window creation
app.on('web-contents-created', (_, contents) => {
  contents.setWindowOpenHandler(({ url }) => {
    shell.openExternal(url);
    return { action: 'deny' };
  });
});

/**
 * Service Communication Helper
 */
async function sendServiceRequest(endpoint: string, data?: any, method: string = 'GET'): Promise<any> {
  const serviceUrl = 'http://127.0.0.1:9876';
  const url = `${serviceUrl}/api/${endpoint}`;
  
  console.log(`[Main] HTTP ${method} request: ${url}`, data);
  
  try {
    const options: RequestInit = {
      method: method,
      headers: {
        'Content-Type': 'application/json',
      },
    };
    
    if (data && (method === 'POST' || method === 'PUT')) {
      options.body = JSON.stringify(data);
    }
    
    const response = await fetch(url, options);
    
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }
    
    const result = await response.json();
    console.log(`[Main] HTTP response:`, result);
    
    return result;
    
  } catch (error) {
    console.error(`[Main] HTTP request failed:`, error);
    
    // Return error response
    return {
      success: false,
      error: error instanceof Error ? error.message : 'Unknown error',
    };
  }
}

/**
 * IPC Handlers
 */

// App information
ipcMain.handle('app:getVersion', () => {
  return app.getVersion();
});

ipcMain.handle('app:isAdmin', () => {
  return checkAdminPrivileges();
});

// Service management
ipcMain.handle('service:getStatus', () => {
  return {
    running: serviceProcess !== null,
    pid: serviceProcess?.pid || null,
  };
});

ipcMain.handle('service:restart', async () => {
  stopService();
  await new Promise(resolve => setTimeout(resolve, 1000));
  return await startService();
});

// Profile operations
ipcMain.handle('ipc:createProfile', async (_, { name, masterKey }) => {
  return await sendServiceRequest('profiles', { name, masterKey }, 'POST');
});

ipcMain.handle('ipc:getAllProfiles', async () => {
  return await sendServiceRequest('profiles');
});

ipcMain.handle('ipc:authenticateProfile', async (_, { profileId, masterKey }) => {
  return await sendServiceRequest(`profiles/${profileId}/authenticate`, { masterKey }, 'POST');
});

ipcMain.handle('ipc:changeProfilePassword', async (_, { profileId, oldKey, newKey }) => {
  return await sendServiceRequest(`profiles/${profileId}/password`, { oldKey, newKey }, 'PUT');
});

// Folder operations (Enhanced with real encryption)
ipcMain.handle('ipc:lockFolder', async (_, { profileId, folderPath, masterKey }) => {
  return await sendServiceRequest('vault/lock', { profileId, folderPath, masterKey }, 'POST');
});

ipcMain.handle('ipc:unlockFoldersTemporary', async (_, { profileId, masterKey }) => {
  return await sendServiceRequest('vault/unlock/temporary', { profileId, masterKey }, 'POST');
});

ipcMain.handle('ipc:unlockFoldersPermanent', async (_, { profileId, masterKey, folderIds }) => {
  return await sendServiceRequest('vault/unlock/permanent', { profileId, masterKey, folderIds }, 'POST');
});

ipcMain.handle('ipc:getProfileFolders', async (_, { profileId }) => {
  return await sendServiceRequest(`vault/folders?profileId=${profileId}`);
});

ipcMain.handle('ipc:getVaultStats', async (_, { profileId }) => {
  return await sendServiceRequest(`vault/stats?profileId=${profileId}`);
});

ipcMain.handle('ipc:lockTemporaryFolders', async (_, { profileId }) => {
  return await sendServiceRequest('vault/lock/temporary', { profileId }, 'POST');
});

// Legacy folder operations (for backward compatibility)
ipcMain.handle('ipc:addFolder', async (_, { profileId, folderPath }) => {
  return await sendServiceRequest('folders', { profileId, folderPath }, 'POST');
});

ipcMain.handle('ipc:unlockFolderTemporary', async (_, { profileId, folderId }) => {
  return await sendServiceRequest(`folders/${folderId}/unlock`, { profileId, temporary: true }, 'POST');
});

ipcMain.handle('ipc:unlockFolderPermanent', async (_, { profileId, folderId }) => {
  return await sendServiceRequest(`folders/${folderId}/unlock`, { profileId, temporary: false }, 'POST');
});

// Analytics operations
ipcMain.handle('ipc:getProfileAnalytics', async (_, { profileId, timeRange }) => {
  return await sendServiceRequest(`analytics?profileId=${profileId}&timeRange=${timeRange}`);
});

ipcMain.handle('ipc:getSystemAnalytics', async (_, { timeRange }) => {
  return await sendServiceRequest(`analytics?timeRange=${timeRange}`);
});

// Recovery operations (Enhanced with AES-256 encryption)
ipcMain.handle('ipc:recoverWithKey', async (_, { recoveryKey }) => {
  return await sendServiceRequest('recovery/validate', { recoveryKey }, 'POST');
});

ipcMain.handle('ipc:generateRecoveryKey', async (_, { profileId }) => {
  return await sendServiceRequest('recovery/generate', { profileId }, 'POST');
});

ipcMain.handle('ipc:getCurrentRecoveryKey', async (_, { profileId, masterKey }) => {
  return await sendServiceRequest('recovery/current', { profileId, masterKey }, 'POST');
});

ipcMain.handle('ipc:changePassword', async (_, { profileId, currentPassword, newPassword }) => {
  return await sendServiceRequest('recovery/change-password', { profileId, currentPassword, newPassword }, 'POST');
});

// Platform operations
ipcMain.handle('ipc:getPlatformInfo', async () => {
  return await sendServiceRequest('platform');
});

ipcMain.handle('ipc:getUnlockMethods', async () => {
  return await sendServiceRequest('platform/methods');
});

ipcMain.handle('ipc:setPreferredUnlockMethod', async (_, { method }) => {
  return await sendServiceRequest('platform/method', { method }, 'PUT');
});

// Dialog operations
ipcMain.handle('dialog:showMessage', async (_, options) => {
  const result = await dialog.showMessageBox(mainWindow!, options);
  return result;
});

ipcMain.handle('dialog:showOpenDialog', async (_, options) => {
  const result = await dialog.showOpenDialog(mainWindow!, options);
  return result;
});

ipcMain.handle('dialog:showSaveDialog', async (_, options) => {
  const result = await dialog.showSaveDialog(mainWindow!, options);
  return result;
});

console.log('[Main] PhantomVault main process initialized');