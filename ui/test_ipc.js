// Simple test to check if IPC events work
const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');

let win;

app.whenReady().then(() => {
  win = new BrowserWindow({
    width: 800,
    height: 600,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, 'electron/preload.js'),
    },
  });

  win.loadURL('http://127.0.0.1:5173');
  win.webContents.openDevTools();

  // Send test event every 3 seconds
  setInterval(() => {
    console.log('🧪 Sending test event from main...');
    win.webContents.send('show-unlock-overlay', { isRecoveryMode: false });
  }, 3000);
});
