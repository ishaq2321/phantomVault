#!/bin/bash

# PhantomVault - Reset for Testing Script
# This script resets the app to first-run state without breaking functionality

echo "╔════════════════════════════════════════════════════════════╗"
echo "║                                                            ║"
echo "║   🔄 PhantomVault - Reset for Testing                     ║"
echo "║                                                            ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Kill any running instances
echo "🔪 Killing running Electron/Vite processes..."
pkill -9 electron 2>/dev/null
pkill -9 vite 2>/dev/null
sleep 1
echo "✅ Processes killed"
echo ""

# Check if storage exists
if [ -d ~/.phantom_vault_storage ]; then
    echo "📂 Found existing storage: ~/.phantom_vault_storage"
    
    # Create backup
    BACKUP_DIR=~/.phantom_vault_storage.backup_$(date +%Y%m%d_%H%M%S)
    echo "💾 Creating backup: $BACKUP_DIR"
    cp -r ~/.phantom_vault_storage "$BACKUP_DIR"
    echo "✅ Backup created"
    echo ""
    
    # Show what's being deleted
    echo "📋 Current profiles:"
    if [ -f ~/.phantom_vault_storage/*/vault_metadata.json ]; then
        cat ~/.phantom_vault_storage/*/vault_metadata.json 2>/dev/null | grep -o '"name": "[^"]*"' | head -5
    fi
    echo ""
    
    # Delete storage
    echo "🗑️  Deleting storage..."
    rm -rf ~/.phantom_vault_storage
    echo "✅ Storage cleared"
else
    echo "ℹ️  No existing storage found - already clean"
fi

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║                                                            ║"
echo "║   ✅ Reset Complete!                                       ║"
echo "║                                                            ║"
echo "║   Next run will show the SetupWizard                       ║"
echo "║   All functionality preserved                              ║"
echo "║                                                            ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "💡 To restore from backup later:"
echo "   cp -r $BACKUP_DIR ~/.phantom_vault_storage"
echo ""
