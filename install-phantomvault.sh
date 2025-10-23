#!/bin/bash

# PhantomVault Installer Script
# This script installs PhantomVault as a desktop application

set -e  # Exit on any error

INSTALL_DIR="/opt/phantomvault"
DESKTOP_FILE="/usr/share/applications/phantomvault.desktop"
SERVICE_FILE="/etc/systemd/system/phantom-vault.service"
BIN_LINK="/usr/local/bin/phantomvault"

echo "🔐 PhantomVault Installer"
echo "========================"
echo ""
echo "This installer will:"
echo "  ✅ Install PhantomVault to $INSTALL_DIR"
echo "  ✅ Create desktop application entry"
echo "  ✅ Set up background service (auto-start)"
echo "  ✅ Install dependencies"
echo "  ✅ Create application launcher"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "❌ Please run this installer as root (use sudo)"
    echo "   sudo ./install-phantomvault.sh"
    exit 1
fi

# Get the actual user (not root)
ACTUAL_USER=${SUDO_USER:-$USER}
ACTUAL_HOME=$(eval echo ~$ACTUAL_USER)

echo "📋 Installation Details:"
echo "   User: $ACTUAL_USER"
echo "   Home: $ACTUAL_HOME"
echo "   Install Directory: $INSTALL_DIR"
echo ""

read -p "Continue with installation? (y/N): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Installation cancelled."
    exit 1
fi

echo ""
echo "🔧 Step 1: Installing system dependencies..."
echo "============================================"

# No need to install ImageMagick - we include a pre-made PNG icon
echo "✅ Using pre-made PNG icon (no ImageMagick required)"

# Detect package manager and install dependencies
if command -v apt &> /dev/null; then
    echo "📦 Detected APT package manager (Ubuntu/Debian)"
    apt update
    
    # Check if Node.js is already installed
    if command -v node &> /dev/null; then
        echo "✅ Node.js already installed: $(node --version)"
        # Install npm using Node.js if not present
        if ! command -v npm &> /dev/null; then
            echo "📦 Installing npm via Node.js..."
            # Use corepack (comes with Node.js 16+) or install npm directly
            if command -v corepack &> /dev/null; then
                corepack enable npm
            else
                # Install npm globally using Node.js
                curl -qL https://www.npmjs.com/install.sh | sh
            fi
        else
            echo "✅ npm already installed: $(npm --version)"
        fi
        # Install system dependencies without nodejs/npm
        apt install -y build-essential cmake libssl-dev libx11-dev libxtst-dev \
                       libxi-dev qtbase5-dev qtbase5-dev-tools nlohmann-json3-dev \
                       curl git
    else
        # Install everything including nodejs and npm
        apt install -y build-essential cmake libssl-dev libx11-dev libxtst-dev \
                       libxi-dev qtbase5-dev qtbase5-dev-tools nlohmann-json3-dev \
                       nodejs npm curl git
    fi
elif command -v dnf &> /dev/null; then
    echo "📦 Detected DNF package manager (Fedora)"
    dnf install -y gcc-c++ cmake openssl-devel libX11-devel libXtst-devel \
                   libXi-devel qt5-qtbase-devel nlohmann-json-devel \
                   nodejs npm curl git
elif command -v pacman &> /dev/null; then
    echo "📦 Detected Pacman package manager (Arch Linux)"
    pacman -S --noconfirm base-devel cmake openssl libx11 libxtst libxi \
                          qt5-base nlohmann-json nodejs npm curl git
else
    echo "❌ Unsupported package manager. Please install dependencies manually:"
    echo "   - build-essential/base-devel"
    echo "   - cmake, openssl-dev, libx11-dev, libxtst-dev, libxi-dev"
    echo "   - qt5-base-dev, nlohmann-json-dev"
    echo "   - nodejs, npm"
    exit 1
fi

# Verify Node.js and npm are working
echo "🔍 Verifying Node.js and npm installation..."
if command -v node &> /dev/null && command -v npm &> /dev/null; then
    echo "✅ Node.js version: $(node --version)"
    echo "✅ npm version: $(npm --version)"
    echo "✅ Dependencies installed successfully"
else
    echo "❌ Node.js or npm installation failed"
    echo "Please install Node.js and npm manually and try again"
    exit 1
fi

echo ""
echo "📁 Step 2: Creating installation directory..."
echo "============================================="

# Get the directory where the installer script is located BEFORE changing directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Check if we can find the source files
if [ -f "$SCRIPT_DIR/core/CMakeLists.txt" ]; then
    echo "📋 Found source files in: $SCRIPT_DIR"
    SOURCE_DIR="$SCRIPT_DIR"
else
    echo "❌ Source files not found. Expected to find core/CMakeLists.txt"
    echo "   Script location: $SCRIPT_DIR"
    echo "   Looking for: $SCRIPT_DIR/core/CMakeLists.txt"
    echo "   Please ensure you're running this installer from the PhantomVault directory."
    exit 1
fi

# Create installation directory
mkdir -p "$INSTALL_DIR"

# Copy source files to installation directory
echo "📋 Copying source files from $SOURCE_DIR to $INSTALL_DIR..."
cp -r "$SOURCE_DIR"/* "$INSTALL_DIR/"

echo "✅ Installation directory created: $INSTALL_DIR"

echo ""
echo "🔨 Step 3: Building PhantomVault..."
echo "==================================="

# Change to installation directory for all builds
cd "$INSTALL_DIR"

# Build C++ core
echo "🔧 Building C++ core service..."
cd "$INSTALL_DIR/core"
mkdir -p build
cd build
cmake ..
make -j$(nproc)

if [ ! -f "phantom_vault_service" ]; then
    echo "❌ Failed to build C++ service"
    exit 1
fi

echo "✅ C++ service built successfully"

# Build Electron UI
echo "🔧 Building Electron UI..."
cd "$INSTALL_DIR/ui"

# Fix ownership for npm operations (since we're running as root)
chown -R "$ACTUAL_USER:$ACTUAL_USER" .

# Install all dependencies including dev dependencies as the actual user
echo "📦 Installing UI dependencies..."
sudo -u "$ACTUAL_USER" npm install

# Install missing dev dependencies that are needed for the launcher
echo "📦 Installing additional dev dependencies..."
sudo -u "$ACTUAL_USER" npm install --save-dev concurrently wait-on typescript

# Build the UI as the actual user
echo "🏗️  Building UI with TypeScript and Vite..."
sudo -u "$ACTUAL_USER" npm run build

# Verify the build was successful
if [ ! -f "dist/index.html" ]; then
    echo "❌ UI build failed - dist/index.html not found"
    echo "Attempting to rebuild..."
    sudo -u "$ACTUAL_USER" npm run build
    if [ ! -f "dist/index.html" ]; then
        echo "❌ UI build failed after retry"
        echo "Checking for errors..."
        ls -la .
        echo "Package.json content:"
        cat package.json | grep -A 10 -B 10 "scripts"
        exit 1
    fi
fi

echo "✅ Electron UI built successfully"

# Build native addon (now that C++ core is built and available)
echo "🔧 Building native addon..."
cd "$INSTALL_DIR/ui/native"

# Fix ownership for native addon build
chown -R "$ACTUAL_USER:$ACTUAL_USER" .

# Install and build native addon as the actual user
sudo -u "$ACTUAL_USER" npm install
sudo -u "$ACTUAL_USER" npm run build

# Verify native addon was built
if [ ! -f "build/Release/phantom_vault_addon.node" ]; then
    echo "❌ Native addon build failed"
    exit 1
fi

echo "✅ Native addon built successfully"

echo ""
echo "🖼️  Step 4: Creating application icon..."
echo "========================================"

# Use pre-made PNG icon (no ImageMagick required)
mkdir -p "$INSTALL_DIR/assets"

# Copy the pre-made PNG icon from the installation directory
if [ -f "$INSTALL_DIR/assets/phantomvault.png" ]; then
    ICON_PATH="$INSTALL_DIR/assets/phantomvault.png"
    echo "✅ Using pre-made PNG icon"
else
    # Fallback: create a simple SVG icon if PNG is missing
    cat > "$INSTALL_DIR/assets/phantomvault.svg" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<svg width="128" height="128" viewBox="0 0 128 128" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <linearGradient id="grad1" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" style="stop-color:#2D3250;stop-opacity:1" />
      <stop offset="100%" style="stop-color:#424769;stop-opacity:1" />
    </linearGradient>
  </defs>
  
  <!-- Background circle -->
  <circle cx="64" cy="64" r="60" fill="url(#grad1)" stroke="#7077A1" stroke-width="4"/>
  
  <!-- Lock body -->
  <rect x="44" y="60" width="40" height="30" rx="4" fill="#F6F6F6"/>
  
  <!-- Lock shackle -->
  <path d="M 52 60 L 52 48 Q 52 36 64 36 Q 76 36 76 48 L 76 60" 
        stroke="#F6F6F6" stroke-width="6" fill="none"/>
  
  <!-- Keyhole -->
  <circle cx="64" cy="72" r="4" fill="#2D3250"/>
  <rect x="62" y="72" width="4" height="8" fill="#2D3250"/>
  
  <!-- Ghost effect (phantom) -->
  <circle cx="64" cy="64" r="60" fill="none" stroke="#7077A1" stroke-width="2" opacity="0.3"/>
  <circle cx="64" cy="64" r="50" fill="none" stroke="#7077A1" stroke-width="1" opacity="0.2"/>
</svg>
EOF
    ICON_PATH="$INSTALL_DIR/assets/phantomvault.svg"
    echo "⚠️  Using fallback SVG icon (PNG not found in package)"
fi

echo ""
echo "🖥️  Step 5: Creating desktop application..."
echo "=========================================="

# Create desktop entry
cat > "$DESKTOP_FILE" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=PhantomVault
Comment=Secure Folder Encryption and Management
Exec=$INSTALL_DIR/phantomvault-launcher.sh
Icon=$ICON_PATH
Terminal=false
Categories=Utility;Security;
Keywords=encryption;security;vault;privacy;folders;
StartupNotify=true
EOF

# Create launcher script
cat > "$INSTALL_DIR/phantomvault-launcher.sh" << EOF
#!/bin/bash

# PhantomVault Application Launcher
cd "$INSTALL_DIR/ui"

# Check if service is running
if ! pgrep -f "phantom_vault_service" > /dev/null; then
    echo "Starting PhantomVault background service..."
    "$INSTALL_DIR/core/build/phantom_vault_service" --daemon &
    sleep 2
fi

# Verify UI build exists
if [ ! -f "$INSTALL_DIR/ui/dist/index.html" ]; then
    echo "❌ UI build not found at $INSTALL_DIR/ui/dist/index.html"
    echo "This should not happen after a successful installation."
    echo "Please reinstall PhantomVault."
    exit 1
fi

# Start Electron GUI in production mode
echo "🚀 Starting PhantomVault..."
npx electron electron/main.js
EOF

chmod +x "$INSTALL_DIR/phantomvault-launcher.sh"
chown "$ACTUAL_USER:$ACTUAL_USER" "$INSTALL_DIR/phantomvault-launcher.sh"

# Create command-line launcher
cat > "$BIN_LINK" << EOF
#!/bin/bash
exec "$INSTALL_DIR/phantomvault-launcher.sh" "\$@"
EOF

chmod +x "$BIN_LINK"

echo "✅ Desktop application created"
echo "✅ Command-line launcher: phantomvault"

echo ""
echo "🔧 Step 6: Setting up background service..."
echo "=========================================="

# Create systemd service
cat > "$SERVICE_FILE" << EOF
[Unit]
Description=PhantomVault Background Service
After=graphical-session.target

[Service]
Type=simple
User=$ACTUAL_USER
Environment=DISPLAY=:0
Environment=HOME=$ACTUAL_HOME
ExecStart=$INSTALL_DIR/core/build/phantom_vault_service
Restart=always
RestartSec=5

[Install]
WantedBy=default.target
EOF

# Enable and start service
systemctl daemon-reload
systemctl enable phantom-vault.service

# Start service for the current user
sudo -u "$ACTUAL_USER" systemctl --user daemon-reload
sudo -u "$ACTUAL_USER" systemctl --user enable phantom-vault.service
sudo -u "$ACTUAL_USER" systemctl --user start phantom-vault.service

echo "✅ Background service configured and started"

echo ""
echo "🔧 Step 7: Setting up vault storage..."
echo "====================================="

# Create vault storage directory
VAULT_STORAGE="$ACTUAL_HOME/.phantom_vault_storage"
mkdir -p "$VAULT_STORAGE/$ACTUAL_USER"
chown -R "$ACTUAL_USER:$ACTUAL_USER" "$VAULT_STORAGE"

echo "✅ Vault storage created: $VAULT_STORAGE"

echo ""
echo "🔧 Step 8: Setting correct permissions..."
echo "========================================"

# Ensure the actual user owns the installation directory contents that they need to access
chown -R "$ACTUAL_USER:$ACTUAL_USER" "$INSTALL_DIR/ui"
chown "$ACTUAL_USER:$ACTUAL_USER" "$INSTALL_DIR/phantomvault-launcher.sh"

# Ensure the service binary is executable
chmod +x "$INSTALL_DIR/core/build/phantom_vault_service"

# Verify critical files exist
echo "🔍 Verifying installation..."

# Check UI build
if [ ! -f "$INSTALL_DIR/ui/dist/index.html" ]; then
    echo "❌ Critical file missing: $INSTALL_DIR/ui/dist/index.html"
    echo "Attempting final UI build..."
    cd "$INSTALL_DIR/ui"
    sudo -u "$ACTUAL_USER" npm run build
    if [ ! -f "$INSTALL_DIR/ui/dist/index.html" ]; then
        echo "❌ Final UI build failed"
        echo "Directory contents:"
        ls -la "$INSTALL_DIR/ui/"
        echo "Dist directory:"
        ls -la "$INSTALL_DIR/ui/dist/" 2>/dev/null || echo "Dist directory does not exist"
        exit 1
    fi
fi

# Check C++ service
if [ ! -f "$INSTALL_DIR/core/build/phantom_vault_service" ]; then
    echo "❌ Critical file missing: $INSTALL_DIR/core/build/phantom_vault_service"
    echo "Installation may be incomplete"
    exit 1
fi

# Check native addon
if [ ! -f "$INSTALL_DIR/ui/native/build/Release/phantom_vault_addon.node" ]; then
    echo "❌ Critical file missing: $INSTALL_DIR/ui/native/build/Release/phantom_vault_addon.node"
    echo "Installation may be incomplete"
    exit 1
fi

echo "✅ All critical files verified"
echo "   • UI build: $INSTALL_DIR/ui/dist/index.html"
echo "   • C++ service: $INSTALL_DIR/core/build/phantom_vault_service"
echo "   • Native addon: $INSTALL_DIR/ui/native/build/Release/phantom_vault_addon.node"
echo "✅ Permissions set correctly"

echo ""
echo "🎉 Installation Complete!"
echo "========================="
echo ""
echo "PhantomVault has been successfully installed!"
echo ""
echo "📱 How to use:"
echo "   • Open 'PhantomVault' from Applications menu"
echo "   • Or run 'phantomvault' in terminal"
echo "   • Use Ctrl+Alt+V for quick folder lock/unlock"
echo "   • Use Ctrl+Alt+R for recovery key access"
echo ""
echo "📁 Installation locations:"
echo "   • Application: $INSTALL_DIR"
echo "   • Desktop entry: $DESKTOP_FILE"
echo "   • Vault storage: $VAULT_STORAGE"
echo "   • Service: $SERVICE_FILE"
echo ""
echo "🔧 Service status:"
systemctl --user status phantom-vault.service --no-pager -l || true
echo ""
echo "🚀 Ready to secure your folders!"
echo ""
echo "💡 First time setup:"
echo "   1. Open PhantomVault from Applications"
echo "   2. Create your master password"
echo "   3. Add folders to protect"
echo "   4. Use Ctrl+Alt+V to lock/unlock anytime"
echo ""