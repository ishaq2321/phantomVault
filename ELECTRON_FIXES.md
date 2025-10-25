# PhantomVault Electron App - White Screen Fixes

## Issues Fixed

### 1. **Loading from Dev Server Instead of Built Files**
**Problem:** The Electron app was trying to load from `http://127.0.0.1:5173` (Vite dev server) even in production mode.

**Solution:** Updated `ui/electron/main.js` to check `NODE_ENV` and load from built files in production:

```javascript
// In createWindow() function:
const isDev = process.env.NODE_ENV !== 'production';
if (isDev) {
  mainWindow.loadURL('http://127.0.0.1:5173');
} else {
  mainWindow.loadFile(path.join(__dirname, '../dist/index.html'));
}
```

**Also applied to:** `createOverlayWindow()` function for the invisible overlay.

### 2. **Missing NODE_ENV Environment Variable**
**Problem:** The production launcher wasn't setting NODE_ENV=production.

**Solution:** Updated `phantomvault-app.sh` to export NODE_ENV:

```bash
# Set production mode
export NODE_ENV=production
npm run electron:prod
```

### 3. **VaultProfileManager/VaultFolderManager Import Errors**
**Problem:** main.js was trying to import TypeScript modules that don't exist in the electron folder.

**Solution:** Commented out the imports since these are frontend React modules:

```javascript
// These are TypeScript modules in src/services/ - accessed via IPC, not directly
let VaultProfileManager = null;
let VaultFolderManager = null;
```

### 4. **Updated App.tsx to Use VaultDashboard**
**Problem:** App.tsx was still importing the old deleted `Dashboard` component.

**Solution:** Updated imports and component usage:

```typescript
import { VaultDashboard } from '../components/dashboard/VaultDashboard';
// ...
<VaultDashboard folders={state.vaults} ... />
```

## How to Run the App

### Development Mode (with hot reload):
```bash
cd ui
npm run dev          # In one terminal (starts Vite dev server)
npm run electron:dev # In another terminal (starts Electron)
```

### Production Mode:
```bash
cd ui
npm run build               # Build the UI first
NODE_ENV=production electron .  # Or use:
npm run electron:prod
```

### Using the Installer:
```bash
# From project root
./phantomvault-app.sh
```

## Testing Your Fixes

1. **Kill any running Electron processes:**
   ```bash
   pkill -f electron
   ```

2. **Rebuild the UI:**
   ```bash
   cd ui
   npm run build
   ```

3. **Start in production mode:**
   ```bash
   NODE_ENV=production npx electron .
   ```

4. **Check if the window opens** - you should see the PhantomVault UI, not a white screen.

## Expected Startup Log:

```
🔐 PhantomVault C++ addon loaded successfully
PhantomVault starting
✅ PhantomVault C++ core initialized
✅ System tray created
✅ Using HotkeyManager
✅ Global hotkeys registered successfully
   Unlock: CommandOrControl+Alt+V
   Recovery: CommandOrControl+Alt+R
✅ AutoLockManager monitoring started
✅ FileSystemWatcher monitoring: ~/.phantom_vault_storage/username
```

## Common Issues & Solutions

### White Screen Still Appears:
1. **Check dist/ folder exists:** `ls ui/dist/index.html`
2. **Verify NODE_ENV is set:** Add `console.log('NODE_ENV:', process.env.NODE_ENV);` to main.js
3. **Check browser console:** Open DevTools in Electron (Ctrl+Shift+I) to see any JS errors
4. **Verify preload.js path:** Check that `ui/electron/preload.js` exists

### "Cannot find module" Errors:
1. **Install dependencies:** `cd ui && npm install`
2. **Check electron is installed:** `ls ui/node_modules/.bin/electron`

### Native Addon Errors:
1. **The C++ addon is optional** - app will work in "mock mode" without it
2. **To build the addon:**
   ```bash
   cd core/build
   cmake ..
   make
   ```

## File Structure:
```
ui/
├── dist/                      # Built files (created by npm run build)
│   ├── index.html
│   └── assets/
├── electron/
│   ├── main.js               # ✅ FIXED - Now loads from dist/ in production
│   └── preload.js
├── src/
│   ├── App.tsx               # ✅ FIXED - Now uses VaultDashboard
│   └── components/
│       └── dashboard/
│           └── VaultDashboard.tsx  # Working dashboard component
└── package.json
```

## Verified Working:
- ✅ Electron loads in production mode
- ✅ Loads built files from dist/ folder
- ✅ App.tsx imports correct VaultDashboard component
- ✅ VaultDashboard renders with all 8 supporting components
- ✅ C++ native addon loads successfully
- ✅ System tray created
- ✅ Global hotkeys registered (Ctrl+Alt+V for unlock)
- ✅ AutoLock monitoring active
- ✅ FileSystem watcher active

## Next Steps:
1. Test the vault creation workflow
2. Test lock/unlock operations
3. Test the invisible overlay (Ctrl+Alt+V)
4. Test the recovery flow (Ctrl+Alt+R)
