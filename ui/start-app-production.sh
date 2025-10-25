#!/bin/bash

# PhantomVault Production Start Script
# This script starts the Electron app in production mode

set -e

echo "🔐 Starting PhantomVault (Production Mode)..."

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Check if dist folder exists
if [ ! -d "dist" ]; then
    echo "❌ Error: dist/ folder not found!"
    echo "   Please build the app first:"
    echo "   npm run build"
    exit 1
fi

# Check if node_modules exists
if [ ! -d "node_modules" ]; then
    echo "⚠️  Installing dependencies..."
    npm install
fi

# Set production mode
export NODE_ENV=production

# Start Electron
echo "🚀 Launching Electron..."
npx electron .
