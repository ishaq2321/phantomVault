#!/bin/bash

echo "╔═══════════════════════════════════════════════════════════════════════╗"
echo "║                                                                       ║"
echo "║   🚀 Starting PhantomVault - Manual Launch Script                    ║"
echo "║                                                                       ║"
echo "╚═══════════════════════════════════════════════════════════════════════╝"
echo ""

# Navigate to the correct directory
cd /home/ishaq2321/phantomVault/ui

echo "📂 Working directory: $(pwd)"
echo ""

# Check if package.json exists
if [ ! -f "package.json" ]; then
    echo "❌ ERROR: package.json not found!"
    echo "   Make sure you're in the correct directory"
    exit 1
fi

echo "✅ Found package.json"
echo ""

# Kill any existing processes
echo "🔪 Killing existing Electron/Vite processes..."
pkill -9 electron 2>/dev/null
pkill -9 vite 2>/dev/null
lsof -ti:5173 | xargs kill -9 2>/dev/null
sleep 1
echo "✅ Cleanup complete"
echo ""

# Start Vite dev server
echo "🌐 Starting Vite dev server..."
npm run dev &
VITE_PID=$!
echo "   PID: $VITE_PID"
echo ""

# Wait for Vite to be ready
echo "⏳ Waiting for Vite to start..."
for i in {1..30}; do
    if curl -s http://localhost:5173 > /dev/null 2>&1; then
        echo "✅ Vite is ready!"
        break
    fi
    echo "   Attempt $i/30..."
    sleep 1
done

if ! curl -s http://localhost:5173 > /dev/null 2>&1; then
    echo "❌ Vite failed to start!"
    kill $VITE_PID 2>/dev/null
    exit 1
fi

echo ""
echo "🖥️  Starting Electron..."
sleep 2
npx electron .

echo ""
echo "👋 Electron closed. Cleaning up..."
kill $VITE_PID 2>/dev/null
echo "✅ Done"
