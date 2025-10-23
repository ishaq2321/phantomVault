#!/bin/bash

echo "🔨 PhantomVault Build Script"
echo "============================"

# Check if we're in the right directory
if [ ! -d "core" ] || [ ! -d "ui" ]; then
    echo "❌ Error: Please run this script from the PhantomVault root directory"
    exit 1
fi

echo ""
echo "📦 Building C++ Core Service..."
echo "--------------------------------"

# Build C++ core
cd core
if [ ! -d "build" ]; then
    mkdir build
fi

cd build
echo "🔧 Running CMake..."
cmake .. || { echo "❌ CMake failed"; exit 1; }

echo "🔨 Compiling..."
make -j$(nproc) || { echo "❌ Compilation failed"; exit 1; }

echo "✅ C++ Core built successfully!"
echo "   Service: $(pwd)/phantom_vault_service"

cd ../..

echo ""
echo "📦 Building Electron UI..."
echo "---------------------------"

cd ui
if [ ! -d "node_modules" ]; then
    echo "📥 Installing npm dependencies..."
    npm install || { echo "❌ npm install failed"; exit 1; }
fi

echo "🔨 Building UI..."
npm run build || { echo "❌ UI build failed"; exit 1; }

echo "✅ Electron UI built successfully!"

cd ..

echo ""
echo "🎉 Build Complete!"
echo "=================="
echo ""
echo "🚀 To test the service:"
echo "   ./test_no_black_screen.sh"
echo ""
echo "🖥️  To start the GUI:"
echo "   cd ui && npm run electron:dev"
echo ""
echo "📋 Service executable:"
echo "   ./core/build/phantom_vault_service"
echo ""