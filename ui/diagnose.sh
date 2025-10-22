#!/bin/bash

echo "════════════════════════════════════════════════════════════════"
echo "   PHANTOMVAULT DIAGNOSTIC REPORT"
echo "════════════════════════════════════════════════════════════════"
echo ""

# Kill existing processes
echo "🧹 Cleaning up existing processes..."
pkill -9 electron 2>/dev/null
pkill -9 vite 2>/dev/null
sleep 1

# Check project structure
echo ""
echo "📁 PROJECT STRUCTURE CHECK"
echo "────────────────────────────────────────────────────────────────"
echo "Main files exist:"
[ -f "electron/main.js" ] && echo "  ✅ electron/main.js" || echo "  ❌ electron/main.js MISSING"
[ -f "electron/preload.js" ] && echo "  ✅ electron/preload.js" || echo "  ❌ electron/preload.js MISSING"
[ -f "src/App.tsx" ] && echo "  ✅ src/App.tsx" || echo "  ❌ src/App.tsx MISSING"
[ -f "src/main.tsx" ] && echo "  ✅ src/main.tsx" || echo "  ❌ src/main.tsx MISSING"
[ -f "components/unlock-overlay/InvisibleOverlay.tsx" ] && echo "  ✅ InvisibleOverlay.tsx" || echo "  ❌ InvisibleOverlay.tsx MISSING"

# Check for old/conflicting code
echo ""
echo "🔍 CHECKING FOR OLD/CONFLICTING CODE"
echo "────────────────────────────────────────────────────────────────"
echo "Searching for old vault methods..."
OLD_METHODS=$(grep -rn "createVault\|lockVault\|unlockVault" src/ components/ 2>/dev/null | grep -v "folder\." | grep -v "//" | wc -l)
if [ "$OLD_METHODS" -eq 0 ]; then
  echo "  ✅ No old vault methods found in React code"
else
  echo "  ⚠️  Found $OLD_METHODS references to old vault methods:"
  grep -rn "createVault\|lockVault\|unlockVault" src/ components/ 2>/dev/null | grep -v "folder\." | grep -v "//" | head -5
fi

# Check IPC handlers in main.js
echo ""
echo "🔌 IPC HANDLERS IN MAIN.JS"
echo "────────────────────────────────────────────────────────────────"
echo "Profile handlers:"
grep -n "ipcMain.handle('profile:" electron/main.js | wc -l | xargs echo "  Count:"
echo "Folder handlers:"
grep -n "ipcMain.handle('folder:" electron/main.js | wc -l | xargs echo "  Count:"
echo "Event senders (show-unlock-overlay):"
grep -n "show-unlock-overlay" electron/main.js | wc -l | xargs echo "  Count:"

# Check preload.js exports
echo ""
echo "📤 PRELOAD.JS EXPORTS"
echo "────────────────────────────────────────────────────────────────"
echo "Checking if onShowUnlockOverlay is exported..."
grep -n "onShowUnlockOverlay" electron/preload.js && echo "  ✅ Found" || echo "  ❌ NOT FOUND"

# Check TypeScript compilation
echo ""
echo "🔧 TYPESCRIPT/BUILD CHECK"
echo "────────────────────────────────────────────────────────────────"
if [ -f "tsconfig.json" ]; then
  echo "  ✅ tsconfig.json exists"
  echo "  JSX setting: $(grep -A 2 '"jsx"' tsconfig.json | head -1)"
else
  echo "  ❌ tsconfig.json MISSING"
fi

# Check for console.log debugging
echo ""
echo "🐛 DEBUGGING LOGS COUNT"
echo "────────────────────────────────────────────────────────────────"
echo "In main.js: $(grep -c 'console\.' electron/main.js)"
echo "In preload.js: $(grep -c 'console\.' electron/preload.js)"
echo "In App.tsx: $(grep -c 'console\.' src/App.tsx)"
echo "In InvisibleOverlay.tsx: $(grep -c 'console\.' components/unlock-overlay/InvisibleOverlay.tsx)"

# Start Vite in background
echo ""
echo "🚀 STARTING VITE DEV SERVER"
echo "────────────────────────────────────────────────────────────────"
npm run dev > /tmp/vite_output.log 2>&1 &
VITE_PID=$!
echo "  Vite PID: $VITE_PID"

# Wait for Vite
echo "  Waiting for Vite to start..."
for i in {1..10}; do
  sleep 1
  if curl -s http://127.0.0.1:5173 > /dev/null 2>&1; then
    echo "  ✅ Vite is running on http://127.0.0.1:5173"
    break
  fi
  echo "    Attempt $i/10..."
done

# Check if Vite is serving the app
echo ""
echo "🌐 VITE SERVER CHECK"
echo "────────────────────────────────────────────────────────────────"
HTTP_STATUS=$(curl -s -o /dev/null -w "%{http_code}" http://127.0.0.1:5173 2>/dev/null)
if [ "$HTTP_STATUS" = "200" ]; then
  echo "  ✅ Vite is serving content (HTTP $HTTP_STATUS)"
  echo "  Checking for React app..."
  CONTENT=$(curl -s http://127.0.0.1:5173 2>/dev/null)
  if echo "$CONTENT" | grep -q "root"; then
    echo "  ✅ HTML contains root div"
  else
    echo "  ⚠️  HTML does NOT contain root div"
  fi
else
  echo "  ❌ Vite is NOT responding (HTTP $HTTP_STATUS)"
fi

# Start Electron
echo ""
echo "🖥️  STARTING ELECTRON"
echo "────────────────────────────────────────────────────────────────"
echo "  Starting Electron with full logging..."
npx electron electron/main.js > /tmp/electron_output.log 2>&1 &
ELECTRON_PID=$!
echo "  Electron PID: $ELECTRON_PID"
sleep 3

# Capture logs
echo ""
echo "📋 INITIAL LOGS (First 50 lines)"
echo "────────────────────────────────────────────────────────────────"
echo "=== ELECTRON LOGS ==="
head -50 /tmp/electron_output.log 2>/dev/null || echo "No electron logs yet"

echo ""
echo "════════════════════════════════════════════════════════════════"
echo "   DIAGNOSTIC COMPLETE"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "📝 Full logs saved to:"
echo "   Vite:     /tmp/vite_output.log"
echo "   Electron: /tmp/electron_output.log"
echo ""
echo "🎯 NEXT STEPS:"
echo "   1. Press Ctrl+Alt+V to trigger unlock overlay"
echo "   2. Watch the logs in real-time:"
echo "      tail -f /tmp/electron_output.log"
echo ""
echo "   To view React console logs, open DevTools in the Electron window"
echo ""
echo "Press Ctrl+C to stop all processes"
echo "════════════════════════════════════════════════════════════════"

# Keep running
wait $ELECTRON_PID
