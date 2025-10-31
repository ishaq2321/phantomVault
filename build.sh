#!/bin/bash

# PhantomVault Build Script
# Builds both C++ service and Electron GUI

set -e

echo "🚀 Building PhantomVault..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[BUILD]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "README.md" ] || [ ! -d "core" ] || [ ! -d "gui" ]; then
    print_error "Please run this script from the PhantomVault root directory"
    exit 1
fi

# Build C++ service
print_status "Building C++ service..."
cd core

# Create build directory
if [ ! -d "build" ]; then
    mkdir build
fi

cd build

# Configure with CMake
print_status "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build
print_status "Compiling C++ service..."
make -j$(nproc)

if [ $? -eq 0 ]; then
    print_success "C++ service built successfully"
    print_status "Service executable: $(pwd)/bin/phantomvault-service"
else
    print_error "Failed to build C++ service"
    exit 1
fi

# Go back to root
cd ../..

# Build GUI (if Node.js is available)
if command -v node &> /dev/null && command -v npm &> /dev/null; then
    print_status "Building Electron GUI..."
    cd gui
    
    # Install dependencies if node_modules doesn't exist
    if [ ! -d "node_modules" ]; then
        print_status "Installing Node.js dependencies..."
        npm install
    fi
    
    # Build the GUI
    print_status "Building GUI application..."
    npm run build
    
    if [ $? -eq 0 ]; then
        print_success "GUI built successfully"
    else
        print_warning "GUI build failed, but C++ service is ready"
    fi
    
    cd ..
else
    print_warning "Node.js not found, skipping GUI build"
    print_status "To build GUI later: cd gui && npm install && npm run build"
fi

print_success "Build complete!"

# Copy unified executable to bin directory
print_status "Setting up unified executable..."
mkdir -p bin
if [ -f "core/build/bin/phantomvault-service" ]; then
    cp core/build/bin/phantomvault-service bin/phantomvault
    chmod +x bin/phantomvault
    print_success "Unified executable ready: bin/phantomvault"
else
    print_warning "Service executable not found, skipping unified setup"
fi

# Create installer packages if requested
if [[ "$1" == "--installer" || "$1" == "--package" ]]; then
    print_status "Creating complete installer packages with uninstaller..."
    
    # Ensure installer directory exists
    mkdir -p installer/build
    
    # Detect platform and create appropriate installer
    case "$(uname -s)" in
        Linux*)
            print_status "Creating Linux packages (DEB/RPM) with uninstaller..."
            ./installer/scripts/build-linux-installer.sh
            ;;
        Darwin*)
            print_status "Creating macOS installer (DMG) with uninstaller..."
            ./installer/scripts/build-macos-installer.sh
            ;;
        CYGWIN*|MINGW32*|MSYS*|MINGW*)
            print_status "Creating Windows installer (MSI) with uninstaller..."
            ./installer/scripts/build-windows-installer.sh
            ;;
        *)
            print_warning "Unknown platform - skipping installer creation"
            ;;
    esac
fi

# Create maintenance and diagnostic tools
if [[ "$1" == "--tools" || "$1" == "--installer" ]]; then
    print_status "Creating maintenance and diagnostic tools..."
    ./installer/scripts/build-maintenance-tools.sh
fi

print_status "To test the service:"
print_status "  ./core/build/bin/phantomvault-service --help"
print_status "  ./core/build/bin/phantomvault-service"

if command -v node &> /dev/null; then
    print_status "To test the GUI:"
    print_status "  cd gui && npm run dev"
fi

print_status "To create installers:"
print_status "  ./build.sh --installer"