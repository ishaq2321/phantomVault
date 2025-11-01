/**
 * PhantomVault - Electron Main Process
 * 
 * Main process for the PhantomVault GUI application.
 * Handles window management, service communication, and system integration.
 */

import { app, BrowserWindow, ipcMain, Menu, shell, dialog, Tray, nativeImage } from 'electron';
import { join } from 'path';
import { spawn, ChildProcess } from 'child_process';
import { existsSync } from 'fs';

// Development mode detection
// Check if we're in a proper development environment (not just unpackaged)
const isDev = !app.isPackaged && process.env.NODE_ENV !== 'production' && existsSync(join(__dirname, '../src'));
const isPackaged = app.isPackaged;

// Service process reference
let serviceProcess: ChildProcess | null = null;
let mainWindow: BrowserWindow | null = null;
let tray: Tray | null = null;

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
    const rendererPath = join(__dirname, 'renderer/index.html');
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
  
  // Handle window close button (hide to tray instead of quit)
  mainWindow.on('close', (event) => {
    if (!(app as any).isQuitting && tray) {
      event.preventDefault();
      mainWindow?.hide();
      
      // Show notification on first hide
      if (process.platform === 'win32' || process.platform === 'linux') {
        tray.displayBalloon({
          title: 'PhantomVault',
          content: 'Application was minimized to tray. Click the tray icon to restore.',
        });
      }
    }
  });

  // Handle external links
  mainWindow.webContents.setWindowOpenHandler(({ url }) => {
    shell.openExternal(url);
    return { action: 'deny' };
  });
}

/**
 * Start the PhantomVault unified service
 */
function startService(): Promise<boolean> {
  return new Promise(async (resolve) => {
    try {
      // First check if a service is already running
      try {
        const response = await fetch('http://127.0.0.1:9876/api/platform');
        if (response.ok) {
          console.log('[Main] Service already running, using existing service');
          resolve(true);
          return;
        }
      } catch (e) {
        // Service not running, continue with startup
      }
      
      // Determine unified service executable path
      let servicePath: string;
      
      if (isDev) {
        // In development, use the unified phantomvault executable from build directory
        const projectRoot = join(__dirname, '../..');
        const buildPath = join(projectRoot, 'build/bin/phantomvault');
        const altBuildPath = join(projectRoot, 'bin/phantomvault');
        
        // Check multiple possible build locations
        if (existsSync(buildPath)) {
          servicePath = buildPath;
        } else if (existsSync(altBuildPath)) {
          servicePath = altBuildPath;
        } else {
          servicePath = join(projectRoot, 'phantomvault');
        }
        console.log('[Main] Development unified service path:', servicePath);
      } else if (isPackaged) {
        // In packaged app, use resources path
        servicePath = join(process.resourcesPath, 'bin/phantomvault');
      } else {
        // In installed but unpackaged version
        servicePath = '/opt/phantomvault/bin/phantomvault';
        console.log('[Main] Using system unified service path:', servicePath);
      }
      
      // Add .exe extension on Windows
      if (process.platform === 'win32' && !servicePath.endsWith('.exe')) {
        servicePath += '.exe';
      }
      
      // Check if unified service executable exists
      if (!existsSync(servicePath)) {
        console.error('[Main] Unified service executable not found:', servicePath);
        
        // Try multiple fallback paths for development
        const fallbackPaths = [
          join(__dirname, '../..', 'core/build/bin/phantomvault-service'),
          join(__dirname, '../..', 'build/phantomvault'),
          '/opt/phantomvault/bin/phantomvault-service',
          '/usr/local/bin/phantomvault',
          './phantomvault'
        ];
        
        let foundPath = null;
        for (const fallbackPath of fallbackPaths) {
          if (existsSync(fallbackPath)) {
            foundPath = fallbackPath;
            break;
          }
        }
        
        if (foundPath) {
          console.log('[Main] Using fallback service path:', foundPath);
          servicePath = foundPath;
        } else {
          console.error('[Main] No valid service executable found in any location');
          resolve(false);
          return;
        }
      }
      
      console.log('[Main] Starting unified service:', servicePath);
      
      // Start unified service in service mode
      serviceProcess = spawn(servicePath, ['--service', '--log-level', 'INFO', '--port', '9876'], {
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
        console.log(`[Main] Unified service exited with code ${code}, signal ${signal}`);
        serviceProcess = null;
        
        // Update tray status
        updateTrayStatus(false);
        
        // Notify renderer about service status change
        if (mainWindow && !mainWindow.isDestroyed()) {
          mainWindow.webContents.send('service:statusChanged', { running: false, pid: null });
        }
      });
      
      serviceProcess.on('error', (error) => {
        console.error('[Main] Unified service error:', error);
        serviceProcess = null;
        resolve(false);
      });
      
      // Wait for service to be ready and verify it's responding
      let attempts = 0;
      const maxAttempts = 10;
      const checkService = async () => {
        try {
          const response = await fetch('http://127.0.0.1:9876/api/platform');
          if (response.ok) {
            console.log('[Main] Unified service is ready and responding');
            
            // Update tray status
            updateTrayStatus(true);
            
            // Notify renderer about service status
            if (mainWindow && !mainWindow.isDestroyed()) {
              mainWindow.webContents.send('service:statusChanged', { 
                running: true, 
                pid: serviceProcess?.pid || null 
              });
            }
            resolve(true);
            return;
          }
        } catch (e) {
          // Service not ready yet
        }
        
        attempts++;
        if (attempts < maxAttempts && serviceProcess) {
          setTimeout(checkService, 500);
        } else {
          console.error('[Main] Unified service failed to respond after', maxAttempts, 'attempts');
          resolve(false);
        }
      };
      
      // Start checking after initial delay
      setTimeout(checkService, 1000);
      
    } catch (error) {
      console.error('[Main] Failed to start unified service:', error);
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
 * Create desktop shortcuts and system integration
 */
function createSystemIntegration(): void {
  try {
    // Set app user model ID for Windows
    if (process.platform === 'win32') {
      app.setAppUserModelId('dev.phantomvault.app');
    }
    
    // Register protocol handler for phantomvault:// URLs
    if (!app.isDefaultProtocolClient('phantomvault')) {
      app.setAsDefaultProtocolClient('phantomvault');
    }
    
    // Create desktop shortcut on first run
    createDesktopShortcut();
    
    // Register file associations for .phantomvault files
    if (process.platform === 'win32') {
      // Windows file association would be handled by the installer
      console.log('[Main] File associations handled by installer on Windows');
    }
    
    // Set up auto-start (optional)
    setupAutoStart();
    
    console.log('[Main] System integration completed');
    
  } catch (error) {
    console.warn('[Main] System integration failed:', error);
  }
}

/**
 * Create desktop shortcut
 */
function createDesktopShortcut(): void {
  try {
    const { execPath } = process;
    const appName = 'PhantomVault';
    
    if (process.platform === 'win32') {
      // Windows desktop shortcut creation
      const desktopPath = join(require('os').homedir(), 'Desktop');
      const shortcutPath = join(desktopPath, `${appName}.lnk`);
      
      if (!existsSync(shortcutPath)) {
        // In a real implementation, we'd use a Windows API or shell command
        console.log('[Main] Would create Windows desktop shortcut at:', shortcutPath);
      }
    } else if (process.platform === 'linux') {
      // Linux desktop entry creation
      const desktopPath = join(require('os').homedir(), 'Desktop');
      const applicationsPath = join(require('os').homedir(), '.local/share/applications');
      const desktopFile = `${appName.toLowerCase()}.desktop`;
      
      const desktopEntry = `[Desktop Entry]
Version=1.0
Type=Application
Name=${appName}
Comment=Invisible Folder Security with Profile-Based Management
Exec=${execPath}
Icon=${join(__dirname, '../assets/icon.png')}
Terminal=false
Categories=Security;Utility;
StartupWMClass=${appName}
`;
      
      // Create in applications directory
      if (existsSync(applicationsPath)) {
        const appDesktopPath = join(applicationsPath, desktopFile);
        if (!existsSync(appDesktopPath)) {
          require('fs').writeFileSync(appDesktopPath, desktopEntry);
          console.log('[Main] Created Linux application entry:', appDesktopPath);
        }
      }
      
      // Create on desktop
      if (existsSync(desktopPath)) {
        const desktopShortcutPath = join(desktopPath, desktopFile);
        if (!existsSync(desktopShortcutPath)) {
          require('fs').writeFileSync(desktopShortcutPath, desktopEntry);
          // Make executable
          require('fs').chmodSync(desktopShortcutPath, 0o755);
          console.log('[Main] Created Linux desktop shortcut:', desktopShortcutPath);
        }
      }
    } else if (process.platform === 'darwin') {
      // macOS - shortcuts are typically handled by the installer
      console.log('[Main] macOS shortcuts handled by installer');
    }
  } catch (error) {
    console.warn('[Main] Desktop shortcut creation failed:', error);
  }
}

/**
 * Setup auto-start functionality
 */
function setupAutoStart(): void {
  try {
    // Only enable auto-start if user has admin privileges and it's not already set
    const loginSettings = app.getLoginItemSettings();
    
    if (!loginSettings.openAtLogin && checkAdminPrivileges()) {
      // Enable auto-start for admin users
      app.setLoginItemSettings({
        openAtLogin: true,
        openAsHidden: true,
        name: 'PhantomVault',
        args: ['--hidden', '--service']
      });
      console.log('[Main] Auto-start enabled for admin user');
    }
  } catch (error) {
    console.warn('[Main] Auto-start setup failed:', error);
  }
}

/**
 * Check for application updates
 */
async function checkForUpdates(): Promise<any> {
  try {
    const currentVersion = app.getVersion();
    const response = await fetch('https://api.github.com/repos/ishaq2321/phantomVault/releases/latest');
    
    if (!response.ok) {
      throw new Error(`GitHub API error: ${response.status}`);
    }
    
    const release = await response.json();
    const latestVersion = release.tag_name.replace('v', '');
    
    // Simple version comparison
    const isUpdateAvailable = compareVersions(currentVersion, latestVersion) < 0;
    
    return {
      success: true,
      currentVersion,
      latestVersion,
      isUpdateAvailable,
      downloadUrl: release.html_url,
      releaseNotes: release.body,
      publishedAt: release.published_at,
    };
  } catch (error) {
    console.error('[Main] Update check failed:', error);
    return {
      success: false,
      error: error instanceof Error ? error.message : 'Unknown error',
    };
  }
}

/**
 * Download application update
 */
async function downloadUpdate(version: string): Promise<any> {
  try {
    console.log('[Main] Downloading update for version:', version);
    
    // In a real implementation, this would download the installer
    // For now, we'll redirect to the GitHub releases page
    shell.openExternal(`https://github.com/ishaq2321/phantomVault/releases/tag/v${version}`);
    
    return {
      success: true,
      message: 'Redirected to download page',
    };
  } catch (error) {
    console.error('[Main] Update download failed:', error);
    return {
      success: false,
      error: error instanceof Error ? error.message : 'Unknown error',
    };
  }
}

/**
 * Install application update
 */
async function installUpdate(packagePath: string): Promise<any> {
  try {
    console.log('[Main] Installing update from:', packagePath);
    
    // In a real implementation, this would handle the installation process
    // For now, we'll show a message to the user
    const result = await dialog.showMessageBox(mainWindow || undefined as any, {
      type: 'info',
      title: 'Update Installation',
      message: 'Update installation requires administrator privileges',
      detail: 'Please run the downloaded installer as administrator to complete the update.',
      buttons: ['OK'],
    });
    
    return {
      success: true,
      message: 'Update installation initiated',
    };
  } catch (error) {
    console.error('[Main] Update installation failed:', error);
    return {
      success: false,
      error: error instanceof Error ? error.message : 'Unknown error',
    };
  }
}

/**
 * Compare two version strings
 */
function compareVersions(version1: string, version2: string): number {
  const v1parts = version1.split('.').map(Number);
  const v2parts = version2.split('.').map(Number);
  
  for (let i = 0; i < Math.max(v1parts.length, v2parts.length); i++) {
    const v1part = v1parts[i] || 0;
    const v2part = v2parts[i] || 0;
    
    if (v1part < v2part) return -1;
    if (v1part > v2part) return 1;
  }
  
  return 0;
}

/**
 * Handle protocol URLs (phantomvault://action)
 */
function handleProtocolUrl(url: string): void {
  console.log('[Main] Handling protocol URL:', url);
  
  try {
    const urlObj = new URL(url);
    const action = urlObj.pathname;
    
    switch (action) {
      case '/unlock':
        // Show main window and trigger unlock
        if (mainWindow) {
          mainWindow.show();
          mainWindow.focus();
          mainWindow.webContents.send('protocol:unlock', urlObj.searchParams.toString());
        }
        break;
        
      case '/show':
        // Show main window
        if (mainWindow) {
          mainWindow.show();
          mainWindow.focus();
        } else {
          createMainWindow();
        }
        break;
        
      default:
        console.warn('[Main] Unknown protocol action:', action);
    }
  } catch (error) {
    console.error('[Main] Failed to handle protocol URL:', error);
  }
}

/**
 * Create system tray with service status monitoring
 */
function createSystemTray(): void {
  // Create tray icon
  const iconPath = join(__dirname, '../assets/icon.png');
  let trayIcon: Electron.NativeImage;
  
  try {
    trayIcon = nativeImage.createFromPath(iconPath);
    if (trayIcon.isEmpty()) {
      // Fallback to a simple icon if the file doesn't exist
      trayIcon = nativeImage.createEmpty();
    }
    // Resize for tray (16x16 on most platforms)
    trayIcon = trayIcon.resize({ width: 16, height: 16 });
  } catch (error) {
    console.warn('[Main] Failed to load tray icon, using empty icon:', error);
    trayIcon = nativeImage.createEmpty();
  }
  
  tray = new Tray(trayIcon);
  
  // Set initial tooltip
  updateTrayStatus(serviceProcess !== null);
  
  // Handle tray click
  tray.on('click', () => {
    if (mainWindow) {
      if (mainWindow.isVisible()) {
        mainWindow.hide();
      } else {
        mainWindow.show();
        mainWindow.focus();
      }
    } else {
      createMainWindow();
    }
  });
  
  // Create context menu
  updateTrayMenu();
}

/**
 * Update tray status and tooltip
 */
function updateTrayStatus(serviceRunning: boolean): void {
  if (!tray) return;
  
  const status = serviceRunning ? 'Running' : 'Stopped';
  const tooltip = `PhantomVault - Service ${status}`;
  
  tray.setToolTip(tooltip);
  
  // Update tray menu
  updateTrayMenu();
}

/**
 * Update tray context menu
 */
function updateTrayMenu(): void {
  if (!tray) return;
  
  const serviceRunning = serviceProcess !== null;
  
  const contextMenu = Menu.buildFromTemplate([
    {
      label: 'PhantomVault',
      type: 'normal',
      enabled: false,
    },
    {
      type: 'separator',
    },
    {
      label: `Service: ${serviceRunning ? 'Running' : 'Stopped'}`,
      type: 'normal',
      enabled: false,
    },
    {
      label: serviceRunning ? 'Restart Service' : 'Start Service',
      type: 'normal',
      click: async () => {
        if (serviceRunning) {
          stopService();
          setTimeout(async () => {
            const started = await startService();
            updateTrayStatus(started);
          }, 1000);
        } else {
          const started = await startService();
          updateTrayStatus(started);
        }
      },
    },
    {
      type: 'separator',
    },
    {
      label: 'Quick Unlock (Ctrl+Alt+V)',
      type: 'normal',
      enabled: serviceRunning,
      click: () => {
        // Simulate Ctrl+Alt+V hotkey
        if (mainWindow && !mainWindow.isDestroyed()) {
          mainWindow.webContents.send('protocol:unlock', '');
        }
      },
    },
    {
      type: 'separator',
    },
    {
      label: 'Show Dashboard',
      type: 'normal',
      click: () => {
        if (mainWindow) {
          mainWindow.show();
          mainWindow.focus();
        } else {
          createMainWindow();
        }
      },
    },
    {
      label: 'Hide to Tray',
      type: 'normal',
      enabled: mainWindow?.isVisible() || false,
      click: () => {
        if (mainWindow) {
          mainWindow.hide();
        }
      },
    },
    {
      type: 'separator',
    },
    {
      label: 'Settings',
      type: 'normal',
      click: () => {
        if (mainWindow) {
          mainWindow.show();
          mainWindow.focus();
          // Navigate to settings page
          mainWindow.webContents.send('navigate', '/settings');
        } else {
          createMainWindow();
        }
      },
    },
    {
      label: 'Analytics',
      type: 'normal',
      click: () => {
        if (mainWindow) {
          mainWindow.show();
          mainWindow.focus();
          // Navigate to analytics page
          mainWindow.webContents.send('navigate', '/analytics');
        } else {
          createMainWindow();
        }
      },
    },
    {
      type: 'separator',
    },
    {
      label: 'About PhantomVault',
      type: 'normal',
      click: () => {
        dialog.showMessageBox(mainWindow || undefined as any, {
          type: 'info',
          title: 'About PhantomVault',
          message: 'PhantomVault v1.0.0',
          detail: 'Invisible Folder Security with Profile-Based Management\n\nBuilt with Electron and C++\nCopyright © 2025 PhantomVault Team\n\nGlobal Hotkey: Ctrl+Alt+V',
        });
      },
    },
    {
      type: 'separator',
    },
    {
      label: 'Quit PhantomVault',
      type: 'normal',
      click: () => {
        app.quit();
      },
    },
  ]);
  
  tray.setContextMenu(contextMenu);
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
  
  // Create system integration
  createSystemIntegration();
  
  // Create system tray
  createSystemTray();
  
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
  // Don't quit when all windows are closed if we have a tray
  if (process.platform !== 'darwin' && !tray) {
    app.quit();
  }
});

// Handle app quit
app.on('before-quit', (event) => {
  (app as any).isQuitting = true;
  console.log('[Main] App quitting, stopping service...');
  stopService();
  
  // Destroy tray
  if (tray) {
    tray.destroy();
    tray = null;
  }
});

// Handle protocol URLs
app.on('open-url', (event, url) => {
  event.preventDefault();
  handleProtocolUrl(url);
});

// Handle protocol URLs on Windows/Linux
app.on('second-instance', (event, commandLine, workingDirectory) => {
  // Someone tried to run a second instance, focus our window instead
  if (mainWindow) {
    if (mainWindow.isMinimized()) mainWindow.restore();
    mainWindow.focus();
  }
  
  // Handle protocol URL if present
  const protocolUrl = commandLine.find(arg => arg.startsWith('phantomvault://'));
  if (protocolUrl) {
    handleProtocolUrl(protocolUrl);
  }
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

// Update management
ipcMain.handle('app:checkForUpdates', async () => {
  return await checkForUpdates();
});

ipcMain.handle('app:downloadUpdate', async (_, version) => {
  return await downloadUpdate(version);
});

ipcMain.handle('app:installUpdate', async (_, packagePath) => {
  return await installUpdate(packagePath);
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

// System integration
ipcMain.handle('system:hideToTray', () => {
  if (mainWindow && tray) {
    mainWindow.hide();
    return true;
  }
  return false;
});

ipcMain.handle('system:showFromTray', () => {
  if (mainWindow) {
    mainWindow.show();
    mainWindow.focus();
    return true;
  }
  return false;
});

ipcMain.handle('system:getTrayStatus', () => {
  return {
    hasTray: tray !== null,
    isVisible: mainWindow?.isVisible() || false,
  };
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