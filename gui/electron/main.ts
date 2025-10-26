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
const isDev = process.env.NODE_ENV === 'development';
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
        servicePath = join(__dirname, '../../core/build/bin/phantomvault-service');
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
 * IPC Handlers
 */

// Get app version
ipcMain.handle('app:getVersion', () => {
  return app.getVersion();
});

// Get admin status
ipcMain.handle('app:isAdmin', () => {
  return checkAdminPrivileges();
});

// Get service status
ipcMain.handle('service:getStatus', () => {
  return {
    running: serviceProcess !== null,
    pid: serviceProcess?.pid || null,
  };
});

// Restart service
ipcMain.handle('service:restart', async () => {
  stopService();
  await new Promise(resolve => setTimeout(resolve, 1000));
  return await startService();
});

// Show message box
ipcMain.handle('dialog:showMessage', async (_, options) => {
  const result = await dialog.showMessageBox(mainWindow!, options);
  return result;
});

// Show open dialog
ipcMain.handle('dialog:showOpenDialog', async (_, options) => {
  const result = await dialog.showOpenDialog(mainWindow!, options);
  return result;
});

// Show save dialog
ipcMain.handle('dialog:showSaveDialog', async (_, options) => {
  const result = await dialog.showSaveDialog(mainWindow!, options);
  return result;
});

console.log('[Main] PhantomVault main process initialized');