#!/bin/bash

# PhantomVault Development Startup Script

echo "🚀 Starting PhantomVault in Development Mode..."
echo ""

# Check if port 5173 is already in use
if lsof -Pi :5173 -sTCP:LISTEN -t >/dev/null ; then
    echo "✅ Vite dev server already running on port 5173"
else
    echo "📦 Starting Vite dev server..."
    cd /home/ishaq2321/phantomVault/ui
    npm run dev &
    VITE_PID=$!
    echo "   Vite PID: $VITE_PID"
    
    # Wait for Vite to be ready
    echo "   Waiting for Vite to start..."
    sleep 3
fi

echo ""
echo "🖥️  Starting Electron..."
cd /home/ishaq2321/phantomVault/ui
NODE_ENV=development npx electron electron/main.js

echo ""
echo "👋 PhantomVault closed"
