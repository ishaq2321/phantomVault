#!/bin/bash

echo "════════════════════════════════════════════════════════════════"
echo "   PHANTOMVAULT CODE ANALYSIS"
echo "════════════════════════════════════════════════════════════════"
echo ""

echo "🔍 ANALYSIS 1: IPC Event Flow"
echo "────────────────────────────────────────────────────────────────"
echo ""
echo "Step 1: Main.js sends 'show-unlock-overlay' event"
echo "  Looking for: mainWindow.webContents.send('show-unlock-overlay'"
grep -n "show-unlock-overlay" electron/main.js
echo ""

echo "Step 2: Preload.js receives and exposes event"
echo "  Looking for: onShowUnlockOverlay definition"
grep -A 3 "onShowUnlockOverlay" electron/preload.js
echo ""

echo "Step 3: App.tsx registers listener"
echo "  Looking for: window.phantomVault.onShowUnlockOverlay"
grep -n "onShowUnlockOverlay" src/App.tsx
echo ""

echo "Step 4: App.tsx sets overlay state"
echo "  Looking for: setOverlayState"
grep -n "setOverlayState" src/App.tsx | head -10
echo ""

echo "Step 5: InvisibleOverlay receives isVisible prop"
echo "  Looking for: isVisible prop usage"
grep -n "isVisible" components/unlock-overlay/InvisibleOverlay.tsx | head -5
echo ""

echo "════════════════════════════════════════════════════════════════"
echo ""
echo "🔍 ANALYSIS 2: Password Parsing Logic"
echo "────────────────────────────────────────────────────────────────"
echo ""
echo "InvisibleOverlay.tsx - parsePasswordInput function:"
grep -A 20 "parsePasswordInput" components/unlock-overlay/InvisibleOverlay.tsx | head -25
echo ""

echo "════════════════════════════════════════════════════════════════"
echo ""
echo "🔍 ANALYSIS 3: Event Listener Registration"
echo "────────────────────────────────────────────────────────────────"
echo ""
echo "Checking if React.StrictMode is being used (causes double mount):"
grep -n "StrictMode" src/main.tsx
echo ""
echo "Checking useEffect dependencies in App.tsx:"
grep -B 2 -A 1 "useEffect" src/App.tsx | head -20
echo ""

echo "════════════════════════════════════════════════════════════════"
echo ""
echo "🔍 ANALYSIS 4: Profile & Folder Manager Integration"
echo "────────────────────────────────────────────────────════════────"
echo ""
echo "Main.js - VaultProfileManager import:"
grep -A 5 "VaultProfileManager" electron/main.js | head -10
echo ""
echo "Main.js - profileManager initialization:"
grep -A 3 "profileManager = " electron/main.js
echo ""

echo "════════════════════════════════════════════════════════════════"
echo ""
echo "🔍 ANALYSIS 5: Potential Issues Found"
echo "────────────────────────────────────────────────────────────════"
echo ""

# Check for React.StrictMode
if grep -q "StrictMode" src/main.tsx; then
  echo "⚠️  WARNING: React.StrictMode is enabled"
  echo "   This causes components to mount twice in development"
  echo "   Event listeners may be registered multiple times"
  echo ""
fi

# Check for missing dependencies in useEffect
USEEFFECT_COUNT=$(grep -c "useEffect" src/App.tsx)
echo "📊 useEffect hooks in App.tsx: $USEEFFECT_COUNT"
echo ""

# Check if overlay is rendered conditionally
echo "Checking if InvisibleOverlay is always rendered:"
grep -B 5 -A 2 "InvisibleOverlay" src/App.tsx | tail -10
echo ""

# Check for async issues
echo "Checking for potential race conditions:"
echo "  - Event listener registration timing:"
grep -A 10 "useEffect" src/App.tsx | grep -A 5 "onShowUnlockOverlay"
echo ""

echo "════════════════════════════════════════════════════════════════"
echo "   ANALYSIS COMPLETE"
echo "════════════════════════════════════════════════════════════════"
