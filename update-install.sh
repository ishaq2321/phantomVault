#!/bin/bash

# PhantomVault Reinstaller
# This updates the installed version at /opt/phantomvault with your cleaned code

set -e

echo "🔄 Updating PhantomVault installation..."

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "❌ This script must be run as root (sudo)"
    echo "   Usage: sudo ./update-install.sh"
    exit 1
fi

INSTALL_DIR="/opt/phantomvault"
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Stop any running instances
echo "🛑 Stopping running instances..."
pkill -f "phantom_vault_service" || true
pkill -f "electron.*phantomvault" || true
systemctl --user stop phantom-vault.service 2>/dev/null || true
sleep 2

# Backup old installation
if [ -d "$INSTALL_DIR" ]; then
    echo "📦 Backing up old installation..."
    mv "$INSTALL_DIR" "${INSTALL_DIR}.backup.$(date +%Y%m%d_%H%M%S)"
fi

# Copy cleaned code to /opt/phantomvault
echo "📁 Installing cleaned code..."
mkdir -p "$INSTALL_DIR"
cp -r "$SCRIPT_DIR"/* "$INSTALL_DIR/"

# Set proper permissions
echo "🔐 Setting permissions..."
chown -R root:root "$INSTALL_DIR"
chmod -R 755 "$INSTALL_DIR"

# Make scripts executable
chmod +x "$INSTALL_DIR"/phantomvault-app.sh
chmod +x "$INSTALL_DIR"/install-phantomvault.sh
chmod +x "$INSTALL_DIR"/uninstall-phantomvault.sh
chmod +x "$INSTALL_DIR"/build-all.sh
chmod +x "$INSTALL_DIR"/scripts/*.sh

# Set UI directory to be writable by users (for node_modules and dist)
chown -R $SUDO_USER:$SUDO_USER "$INSTALL_DIR/ui"

# Build UI
echo "🔨 Building UI..."
cd "$INSTALL_DIR/ui"
sudo -u $SUDO_USER npm install
sudo -u $SUDO_USER npm run build

# Build C++ core
echo "🔨 Building C++ core..."
cd "$INSTALL_DIR/core/build"
cmake ..
make -j$(nproc)

echo ""
echo "✅ PhantomVault updated successfully!"
echo ""
echo "You can now run PhantomVault using:"
echo "  $INSTALL_DIR/phantomvault-app.sh"
echo ""
echo "Or create a desktop launcher:"
echo "  sudo $INSTALL_DIR/install-phantomvault.sh"
