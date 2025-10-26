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
const isDev = !app.isPackaged;
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
    mainWindow.loadFile(join(__dirname, '../renderer/index.html'));
  }

  // Show window when ready
  mainWindow.once('ready-to-show', () => {
    mainWindow?.show();
    
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
      } else {
        servicePath = join(process.resourcesPath, 'bin/phantomvault-service');
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
async function sendServiceRequest(endpoint: string, data?: any): Promise<any> {
  // For now, return mock data since we don't have HTTP communication yet
  // This will be replaced with actual HTTP/IPC communication in Phase 7
  
  console.log(`[Main] Service request: ${endpoint}`, data);
  
  // Mock responses based on endpoint
  switch (endpoint) {
    case 'profiles/list':
      return {
        success: true,
        profiles: [
          {
            id: 'profile1',
            name: 'Personal',
            createdAt: '2024-01-15',
            lastAccess: '2024-01-20',
            folderCount: 3,
          },
          {
            id: 'profile2',
            name: 'Work',
            createdAt: '2024-01-10',
            lastAccess: '2024-01-19',
            folderCount: 5,
          },
        ],
      };
    
    case 'folders/list':
      return {
        success: true,
        folders: [
          {
            id: 'folder1',
            name: 'Documents',
            originalPath: '/home/user/Documents',
            isLocked: true,
            size: 1024 * 1024 * 50,
            createdAt: '2024-01-15',
          },
          {
            id: 'folder2',
            name: 'Photos',
            originalPath: '/home/user/Photos',
            isLocked: false,
            size: 1024 * 1024 * 200,
            createdAt: '2024-01-16',
          },
        ],
      };
    
    case 'analytics/system':
      return {
        success: true,
        statistics: {
          totalProfiles: 3,
          totalFolders: 12,
          totalUnlockAttempts: 45,
          successfulUnlocks: 42,
          failedUnlocks: 3,
          keyboardSequenceDetections: 28,
          securityViolations: 2,
          totalUptime: '15d 8h 32m',
          firstUse: '2024-01-01',
          lastActivity: '2024-01-20 14:30',
        },
      };
    
    case 'platform/info':
      return {
        success: true,
        platform: process.platform,
        capabilities: {
          supportsInvisibleLogging: true,
          supportsHotkeys: true,
          requiresPermissions: false,
        },
      };
    
    default:
      return {
        success: true,
        message: `Mock response for ${endpoint}`,
        data: data,
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
  return await sendServiceRequest('profiles/create', { name, masterKey });
});

ipcMain.handle('ipc:getAllProfiles', async () => {
  return await sendServiceRequest('profiles/list');
});

ipcMain.handle('ipc:authenticateProfile', async (_, { profileId, masterKey }) => {
  return await sendServiceRequest('profiles/authenticate', { profileId, masterKey });
});

ipcMain.handle('ipc:changeProfilePassword', async (_, { profileId, oldKey, newKey }) => {
  return await sendServiceRequest('profiles/changePassword', { profileId, oldKey, newKey });
});

// Folder operations
ipcMain.handle('ipc:addFolder', async (_, { profileId, folderPath }) => {
  return await sendServiceRequest('folders/add', { profileId, folderPath });
});

ipcMain.handle('ipc:getProfileFolders', async (_, { profileId }) => {
  return await sendServiceRequest('folders/list', { profileId });
});

ipcMain.handle('ipc:unlockFolderTemporary', async (_, { profileId, folderId }) => {
  return await sendServiceRequest('folders/unlockTemporary', { profileId, folderId });
});

ipcMain.handle('ipc:unlockFolderPermanent', async (_, { profileId, folderId }) => {
  return await sendServiceRequest('folders/unlockPermanent', { profileId, folderId });
});

ipcMain.handle('ipc:lockTemporaryFolders', async (_, { profileId }) => {
  return await sendServiceRequest('folders/lockTemporary', { profileId });
});

// Analytics operations
ipcMain.handle('ipc:getProfileAnalytics', async (_, { profileId, timeRange }) => {
  return await sendServiceRequest('analytics/profile', { profileId, timeRange });
});

ipcMain.handle('ipc:getSystemAnalytics', async (_, { timeRange }) => {
  return await sendServiceRequest('analytics/system', { timeRange });
});

// Recovery operations
ipcMain.handle('ipc:recoverWithKey', async (_, { recoveryKey }) => {
  return await sendServiceRequest('recovery/validate', { recoveryKey });
});

// Platform operations
ipcMain.handle('ipc:getPlatformInfo', async () => {
  return await sendServiceRequest('platform/info');
});

ipcMain.handle('ipc:getUnlockMethods', async () => {
  return await sendServiceRequest('platform/unlockMethods');
});

ipcMain.handle('ipc:setPreferredUnlockMethod', async (_, { method }) => {
  return await sendServiceRequest('platform/setUnlockMethod', { method });
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