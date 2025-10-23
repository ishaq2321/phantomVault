#!/bin/bash

# Test script to verify the installer works correctly
# This script tests the installer in a clean environment

set -e

echo "🧪 PhantomVault Installer Test"
echo "=============================="
echo ""

# Check if we're running as root
if [ "$EUID" -ne 0 ]; then
    echo "❌ This test script must be run as root (use sudo)"
    echo "   sudo ./test-installer.sh"
    exit 1
fi

# Get the actual user
ACTUAL_USER=${SUDO_USER:-$USER}
echo "📋 Test Environment:"
echo "   User: $ACTUAL_USER"
echo "   Working Directory: $(pwd)"
echo ""

# Test 1: Check if installer exists
echo "🔍 Test 1: Checking installer exists..."
if [ -f "./install-phantomvault.sh" ]; then
    echo "✅ Installer found"
else
    echo "❌ Installer not found"
    exit 1
fi

# Test 2: Check if installer is executable
echo "🔍 Test 2: Checking installer permissions..."
if [ -x "./install-phantomvault.sh" ]; then
    echo "✅ Installer is executable"
else
    echo "⚠️  Making installer executable..."
    chmod +x "./install-phantomvault.sh"
    echo "✅ Installer made executable"
fi

# Test 3: Check required source files
echo "🔍 Test 3: Checking source files..."
REQUIRED_FILES=(
    "core/CMakeLists.txt"
    "ui/package.json"
    "ui/electron/main.js"
    "assets/phantomvault.png"
)

for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "✅ Found: $file"
    else
        echo "❌ Missing: $file"
        exit 1
    fi
done

# Test 4: Check system dependencies
echo "🔍 Test 4: Checking system dependencies..."
REQUIRED_COMMANDS=(
    "node"
    "npm"
    "cmake"
    "make"
    "g++"
)

for cmd in "${REQUIRED_COMMANDS[@]}"; do
    if command -v "$cmd" &> /dev/null; then
        echo "✅ Found: $cmd"
    else
        echo "❌ Missing: $cmd"
        echo "   Please install system dependencies first"
        exit 1
    fi
done

echo ""
echo "🎉 All tests passed!"
echo "✅ Installer is ready to run"
echo ""
echo "📋 To install PhantomVault:"
echo "   sudo ./install-phantomvault.sh"
echo ""
echo "💡 The installer will:"
echo "   • Install to /opt/phantomvault"
echo "   • Create desktop application entry"
echo "   • Set up background service"
echo "   • Create command-line launcher"
echo "   • Handle all permissions correctly"
echo ""