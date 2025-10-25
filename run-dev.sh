#!/bin/bash

# PhantomVault Development Runner
# Runs the app from your home directory (not the installed version)

set -e

echo "🔐 Starting PhantomVault from development directory..."

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Check if UI build exists
if [ ! -d "ui/dist" ] || [ ! -f "ui/dist/index.html" ]; then
    echo "⚠️  Building UI for the first time..."
    cd ui
    npm install
    npm run build
    cd ..
fi

# Start the C++ service if not running
echo "🔧 Checking background service..."
if ! pgrep -f "phantom_vault_service" > /dev/null; then
    echo "🚀 Starting background service..."
    if [ -f "core/build/phantom_vault_service" ]; then
        ./core/build/phantom_vault_service --daemon &
        sleep 2
        echo "✅ Service started"
    else
        echo "⚠️  C++ service not built, running in GUI-only mode"
    fi
else
    echo "✅ Service already running"
fi

# Launch Electron GUI from home directory
echo "🖥️  Launching PhantomVault GUI..."
cd ui

# Set production mode
export NODE_ENV=production

# Start Electron
echo "🚀 Starting Electron..."
npx electron .
