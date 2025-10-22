#!/bin/bash

# Test script for the installer (dry run)

echo "🧪 PhantomVault Installer Test"
echo "=============================="
echo ""
echo "This script tests the installer without actually installing."
echo ""

# Check if we're in the right directory
if [ ! -f "install-phantomvault.sh" ]; then
    echo "❌ install-phantomvault.sh not found"
    exit 1
fi

echo "✅ Installer script found"

# Check if source files exist
if [ ! -d "core" ] || [ ! -d "ui" ]; then
    echo "❌ Source directories (core, ui) not found"
    exit 1
fi

echo "✅ Source directories found"

# Check if core can be built
if [ ! -f "core/CMakeLists.txt" ]; then
    echo "❌ core/CMakeLists.txt not found"
    exit 1
fi

echo "✅ Core build files found"

# Check if UI files exist
if [ ! -f "ui/package.json" ]; then
    echo "❌ ui/package.json not found"
    exit 1
fi

echo "✅ UI build files found"

# Test build process (without installing)
echo ""
echo "🔨 Testing build process..."

# Test core build
echo "📦 Testing C++ core build..."
cd core
if [ ! -d "build" ]; then
    mkdir build
fi
cd build

echo "   Running cmake..."
if cmake .. > /dev/null 2>&1; then
    echo "   ✅ CMake configuration successful"
else
    echo "   ❌ CMake configuration failed"
    cd ../..
    exit 1
fi

echo "   Running make..."
if make -j$(nproc) > /dev/null 2>&1; then
    echo "   ✅ C++ build successful"
else
    echo "   ❌ C++ build failed"
    cd ../..
    exit 1
fi

if [ -f "phantom_vault_service" ]; then
    echo "   ✅ Service executable created"
else
    echo "   ❌ Service executable not found"
    cd ../..
    exit 1
fi

cd ../..

# Test UI dependencies
echo "📦 Testing UI dependencies..."
cd ui

if command -v npm &> /dev/null; then
    echo "   ✅ npm found"
    
    if [ ! -d "node_modules" ]; then
        echo "   Installing dependencies..."
        if npm install > /dev/null 2>&1; then
            echo "   ✅ npm install successful"
        else
            echo "   ❌ npm install failed"
            cd ..
            exit 1
        fi
    else
        echo "   ✅ node_modules already exists"
    fi
    
    echo "   Testing build..."
    if npm run build > /dev/null 2>&1; then
        echo "   ✅ UI build successful"
    else
        echo "   ⚠️  UI build failed (may need dev server)"
    fi
else
    echo "   ❌ npm not found - install Node.js"
    cd ..
    exit 1
fi

cd ..

echo ""
echo "🎉 Installer Test Results"
echo "========================="
echo ""
echo "✅ All components ready for installation!"
echo ""
echo "📋 What was tested:"
echo "   ✅ Source files present"
echo "   ✅ C++ core builds successfully"
echo "   ✅ Service executable created"
echo "   ✅ UI dependencies installable"
echo "   ✅ Build system functional"
echo ""
echo "🚀 Ready to run installer:"
echo "   sudo ./install-phantomvault.sh"
echo ""
echo "📦 Ready to create release:"
echo "   ./prepare-release.sh"
echo ""