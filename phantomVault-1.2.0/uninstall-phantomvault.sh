#!/bin/bash

# PhantomVault Uninstaller Script

INSTALL_DIR="/opt/phantomvault"
DESKTOP_FILE="/usr/share/applications/phantomvault.desktop"
SERVICE_FILE="/etc/systemd/system/phantom-vault.service"
BIN_LINK="/usr/local/bin/phantomvault"

echo "🗑️  PhantomVault Uninstaller"
echo "============================"
echo ""
echo "This will remove PhantomVault from your system."
echo ""
echo "⚠️  WARNING: This will:"
echo "   • Stop the background service"
echo "   • Remove the application files"
echo "   • Remove desktop entries"
echo "   • Keep your vault data safe (in ~/.phantom_vault_storage)"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "❌ Please run this uninstaller as root (use sudo)"
    echo "   sudo ./uninstall-phantomvault.sh"
    exit 1
fi

ACTUAL_USER=${SUDO_USER:-$USER}

read -p "Are you sure you want to uninstall PhantomVault? (y/N): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Uninstallation cancelled."
    exit 1
fi

echo ""
echo "🛑 Step 1: Stopping services..."
echo "==============================="

# Stop and disable service
if systemctl is-active --quiet phantom-vault.service; then
    systemctl stop phantom-vault.service
    echo "✅ Service stopped"
fi

if systemctl is-enabled --quiet phantom-vault.service; then
    systemctl disable phantom-vault.service
    echo "✅ Service disabled"
fi

# Stop user service
sudo -u "$ACTUAL_USER" systemctl --user stop phantom-vault.service 2>/dev/null || true
sudo -u "$ACTUAL_USER" systemctl --user disable phantom-vault.service 2>/dev/null || true

echo ""
echo "🗑️  Step 2: Removing files..."
echo "============================="

# Remove installation directory
if [ -d "$INSTALL_DIR" ]; then
    rm -rf "$INSTALL_DIR"
    echo "✅ Removed: $INSTALL_DIR"
fi

# Remove desktop file
if [ -f "$DESKTOP_FILE" ]; then
    rm -f "$DESKTOP_FILE"
    echo "✅ Removed: $DESKTOP_FILE"
fi

# Remove service file
if [ -f "$SERVICE_FILE" ]; then
    rm -f "$SERVICE_FILE"
    echo "✅ Removed: $SERVICE_FILE"
fi

# Remove command-line launcher
if [ -f "$BIN_LINK" ]; then
    rm -f "$BIN_LINK"
    echo "✅ Removed: $BIN_LINK"
fi

# Reload systemd
systemctl daemon-reload
sudo -u "$ACTUAL_USER" systemctl --user daemon-reload 2>/dev/null || true

echo ""
echo "✅ PhantomVault Uninstalled Successfully!"
echo "========================================"
echo ""
echo "📁 Your vault data is preserved in:"
echo "   ~/.phantom_vault_storage"
echo ""
echo "💡 To completely remove all data:"
echo "   rm -rf ~/.phantom_vault_storage"
echo ""
echo "🔄 To reinstall PhantomVault:"
echo "   sudo ./install-phantomvault.sh"
echo ""