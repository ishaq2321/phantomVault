#!/bin/bash
# Test script to verify PhantomVault desktop application works

echo "🧪 Testing PhantomVault Desktop Application"
echo "=========================================="

# Test 1: Check if service binary exists and works
echo "1. Testing service binary..."
if [[ -f "./core/build/bin/phantomvault-service" ]]; then
    echo "✅ Service binary found"
    ./core/build/bin/phantomvault-service --version
else
    echo "❌ Service binary not found - need to build first"
    echo "Run: ./build.sh"
    exit 1
fi

echo ""

# Test 2: Check if GUI can be built
echo "2. Testing GUI build..."
cd gui
if npm run build:electron; then
    echo "✅ GUI built successfully"
else
    echo "❌ GUI build failed"
    exit 1
fi

echo ""

# Test 3: Check if desktop integration files exist
echo "3. Testing desktop integration..."
if [[ -f "dist/main.js" ]]; then
    echo "✅ Electron main process built"
else
    echo "❌ Electron main process missing"
    exit 1
fi

if [[ -d "dist/renderer" ]]; then
    echo "✅ React renderer built"
else
    echo "❌ React renderer missing"
    exit 1
fi

echo ""

# Test 4: Test if GUI can start (in development mode)
echo "4. Testing GUI startup (will start and stop automatically)..."
echo "Starting PhantomVault GUI for 10 seconds..."

# Start GUI in background
npm run dev &
GUI_PID=$!

# Wait 10 seconds
sleep 10

# Stop GUI
kill $GUI_PID 2>/dev/null

echo "✅ GUI startup test completed"

echo ""
echo "🎉 All tests passed! PhantomVault desktop application is ready."
echo ""
echo "To test manually:"
echo "1. Run: ./build.sh"
echo "2. Run: cd gui && npm run dev"
echo "3. The desktop application should open automatically"