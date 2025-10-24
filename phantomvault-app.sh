#!/bin/bash

# PhantomVault Application Launcher
# This script ensures the service is running and launches the GUI

INSTALL_DIR="/opt/phantomvault"
SERVICE_NAME="phantom-vault.service"

echo "🔐 Starting PhantomVault..."

# Function to check if service is running
check_service() {
    systemctl --user is-active --quiet "$SERVICE_NAME" 2>/dev/null
}

# Function to start service
start_service() {
    echo "🚀 Starting background service..."
    
    # Try user service first
    if systemctl --user start "$SERVICE_NAME" 2>/dev/null; then
        echo "✅ User service started"
        return 0
    fi
    
    # Fallback to manual start
    echo "⚠️  User service not available, starting manually..."
    if [ -f "$INSTALL_DIR/core/build/phantom_vault_service" ]; then
        "$INSTALL_DIR/core/build/phantom_vault_service" --daemon &
        sleep 2
        echo "✅ Service started manually"
        return 0
    else
        echo "❌ Service executable not found: $INSTALL_DIR/core/build/phantom_vault_service"
        return 1
    fi
}

# Check if installation exists
if [ ! -d "$INSTALL_DIR" ]; then
    echo "❌ PhantomVault not installed. Please run:"
    echo "   sudo ./install-phantomvault.sh"
    exit 1
fi

# Check if service is running
if ! check_service; then
    echo "🔧 Service not running, starting it..."
    if ! start_service; then
        echo "❌ Failed to start service"
        exit 1
    fi
else
    echo "✅ Service already running"
fi

# Wait a moment for service to initialize
sleep 1

# Launch GUI
echo "🖥️  Launching PhantomVault GUI..."
cd "$INSTALL_DIR/ui"

# Check if we can run electron
if [ ! -d "node_modules" ]; then
    echo "⚠️  Installing UI dependencies..."
    npm install
fi

# Start Electron GUI
if command -v npm &> /dev/null; then
    npm run electron:prod
else
    echo "❌ npm not found. Please install Node.js"
    exit 1
fi