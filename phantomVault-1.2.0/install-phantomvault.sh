#!/bin/bash

# PhantomVault Installer Script
# This script installs PhantomVault as a desktop application

set -e  # Exit on any error

# Beautiful Colors and Formatting
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
BOLD='\033[1m'
DIM='\033[2m'
NC='\033[0m' # No Color

# Progress tracking
TOTAL_STEPS=8
CURRENT_STEP=0
START_TIME=$(date +%s)

# Beautiful progress bar function
show_progress() {
    local current=$1
    local total=$2
    local width=50
    local percentage=$((current * 100 / total))
    local completed=$((current * width / total))
    local remaining=$((width - completed))
    
    printf "\r${CYAN}["
    printf "%*s" $completed | tr ' ' '█'
    printf "%*s" $remaining | tr ' ' '░'
    printf "] ${WHITE}%d%%${NC} " $percentage
}

# Step header function
step_header() {
    CURRENT_STEP=$((CURRENT_STEP + 1))
    local title="$1"
    local estimated_time="$2"
    
    echo ""
    echo -e "${PURPLE}╔══════════════════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${PURPLE}║${NC} ${BOLD}Step $CURRENT_STEP/$TOTAL_STEPS: $title${NC}"
    echo -e "${PURPLE}║${NC} ${DIM}Estimated time: $estimated_time${NC}"
    echo -e "${PURPLE}╚══════════════════════════════════════════════════════════════════════════════╗${NC}"
    show_progress $CURRENT_STEP $TOTAL_STEPS
    echo ""
}

# Success message function
success_msg() {
    echo -e "   ${GREEN}✅ $1${NC}"
}

# Info message function
info_msg() {
    echo -e "   ${BLUE}ℹ️  $1${NC}"
}

# Warning message function
warn_msg() {
    echo -e "   ${YELLOW}⚠️  $1${NC}"
}

# Error message function
error_msg() {
    echo -e "   ${RED}❌ $1${NC}"
}

# Spinner function for long operations
spinner() {
    local pid=$1
    local delay=0.1
    local spinstr='⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏'
    while [ "$(ps a | awk '{print $1}' | grep $pid)" ]; do
        local temp=${spinstr#?}
        printf " [%c]  " "$spinstr"
        local spinstr=$temp${spinstr%"$temp"}
        sleep $delay
        printf "\b\b\b\b\b\b"
    done
    printf "    \b\b\b\b"
}

INSTALL_DIR="/opt/phantomvault"
DESKTOP_FILE="/usr/share/applications/phantomvault.desktop"
SERVICE_FILE="/etc/systemd/system/phantom-vault.service"
BIN_LINK="/usr/local/bin/phantomvault"

# Beautiful header
clear
echo -e "${PURPLE}"
echo "╔══════════════════════════════════════════════════════════════════════════════╗"
echo "║                                                                              ║"
echo "║                    🔐 ${WHITE}PhantomVault v1.1.0 Installer${PURPLE}                      ║"
echo "║                                                                              ║"
echo "║              ${CYAN}Revolutionary Keyboard Sequence Detection${PURPLE}                ║"
echo "║                                                                              ║"
echo "╚══════════════════════════════════════════════════════════════════════════════╝"
echo -e "${NC}"
echo ""
echo -e "${BOLD}${WHITE}🚀 World's First Invisible Folder Encryption System${NC}"
echo ""
echo -e "${GREEN}This installer will set up:${NC}"
echo -e "  ${GREEN}✨${NC} Invisible keyboard sequence detection"
echo -e "  ${GREEN}🔐${NC} Military-grade AES-256-GCM encryption"
echo -e "  ${GREEN}⚡${NC} Global hotkey system (Ctrl+Alt+V)"
echo -e "  ${GREEN}🖥️${NC} Desktop application with system tray"
echo -e "  ${GREEN}🛡️${NC} Auto-start background service"
echo -e "  ${GREEN}📦${NC} Complete dependency management"
echo ""
echo -e "${CYAN}Installation Details:${NC}"
echo -e "  ${DIM}Target Directory:${NC} $INSTALL_DIR"
echo -e "  ${DIM}Estimated Time:${NC} 2-5 minutes"
echo -e "  ${DIM}Internet Required:${NC} Yes (for dependencies)"
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    error_msg "Please run this installer as root (use sudo)"
    echo -e "   ${YELLOW}sudo ./install-phantomvault.sh${NC}"
    exit 1
fi

# Get the actual user (not root)
ACTUAL_USER=${SUDO_USER:-$USER}
ACTUAL_HOME=$(eval echo ~$ACTUAL_USER)

echo -e "${BOLD}System Information:${NC}"
echo -e "  ${DIM}User:${NC} $ACTUAL_USER"
echo -e "  ${DIM}Home:${NC} $ACTUAL_HOME"
echo -e "  ${DIM}OS:${NC} $(lsb_release -d 2>/dev/null | cut -f2 || echo "Linux")"
echo -e "  ${DIM}Architecture:${NC} $(uname -m)"
echo ""

echo -e "${YELLOW}⚠️  This installer requires root privileges to:${NC}"
echo -e "  • Install system dependencies"
echo -e "  • Create system service files"
echo -e "  • Set up desktop integration"
echo ""

read -p "$(echo -e ${BOLD}Continue with installation? [y/N]:${NC} )" -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo -e "${RED}Installation cancelled.${NC}"
    exit 1
fi

step_header "Installing System Dependencies" "30-60 seconds"

info_msg "Using pre-made PNG icon (no ImageMagick required)"

# Detect package manager and install dependencies
if command -v apt &> /dev/null; then
    info_msg "Detected APT package manager (Ubuntu/Debian)"
    echo -e "   ${DIM}Updating package lists...${NC}"
    apt update > /dev/null 2>&1 &
    spinner $!
    success_msg "Package lists updated"
    
    # Check if Node.js is already installed
    if command -v node &> /dev/null; then
        success_msg "Node.js already installed: $(node --version)"
        # Install npm using Node.js if not present
        if ! command -v npm &> /dev/null; then
            info_msg "Installing npm via Node.js..."
            # Use corepack (comes with Node.js 16+) or install npm directly
            if command -v corepack &> /dev/null; then
                corepack enable npm > /dev/null 2>&1
            else
                # Install npm globally using Node.js
                curl -qL https://www.npmjs.com/install.sh | sh > /dev/null 2>&1
            fi
        else
            success_msg "npm already installed: $(npm --version)"
        fi
        # Install system dependencies without nodejs/npm
        info_msg "Installing C++ build tools and libraries..."
        apt install -y build-essential cmake libssl-dev libx11-dev libxtst-dev \
                       libxi-dev qtbase5-dev qtbase5-dev-tools nlohmann-json3-dev \
                       curl git > /dev/null 2>&1 &
        spinner $!
    else
        # Install everything including nodejs and npm
        info_msg "Installing complete development environment..."
        apt install -y build-essential cmake libssl-dev libx11-dev libxtst-dev \
                       libxi-dev qtbase5-dev qtbase5-dev-tools nlohmann-json3-dev \
                       nodejs npm curl git > /dev/null 2>&1 &
        spinner $!
    fi
elif command -v dnf &> /dev/null; then
    info_msg "Detected DNF package manager (Fedora)"
    dnf install -y gcc-c++ cmake openssl-devel libX11-devel libXtst-devel \
                   libXi-devel qt5-qtbase-devel nlohmann-json-devel \
                   nodejs npm curl git > /dev/null 2>&1 &
    spinner $!
elif command -v pacman &> /dev/null; then
    info_msg "Detected Pacman package manager (Arch Linux)"
    pacman -S --noconfirm base-devel cmake openssl libx11 libxtst libxi \
                          qt5-base nlohmann-json nodejs npm curl git > /dev/null 2>&1 &
    spinner $!
else
    error_msg "Unsupported package manager. Please install dependencies manually:"
    echo -e "   ${RED}•${NC} build-essential/base-devel"
    echo -e "   ${RED}•${NC} cmake, openssl-dev, libx11-dev, libxtst-dev, libxi-dev"
    echo -e "   ${RED}•${NC} qt5-base-dev, nlohmann-json-dev"
    echo -e "   ${RED}•${NC} nodejs, npm"
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

# Calculate installation time
END_TIME=$(date +%s)
INSTALL_TIME=$((END_TIME - START_TIME))
MINUTES=$((INSTALL_TIME / 60))
SECONDS=$((INSTALL_TIME % 60))

echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║                                                                              ║${NC}"
echo -e "${GREEN}║                    🎉 ${WHITE}Installation Complete!${GREEN}                         ║${NC}"
echo -e "${GREEN}║                                                                              ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════════════════════════╝${NC}"
echo ""
show_progress $TOTAL_STEPS $TOTAL_STEPS
echo ""
echo ""
echo -e "${BOLD}${WHITE}🚀 PhantomVault v1.1.0 Successfully Installed!${NC}"
echo ""
echo -e "${GREEN}⏱️  Installation completed in: ${WHITE}${MINUTES}m ${SECONDS}s${NC}"
echo ""
echo -e "${CYAN}🎯 Revolutionary Features Now Available:${NC}"
echo -e "  ${GREEN}✨${NC} Invisible keyboard sequence detection"
echo -e "  ${GREEN}🔐${NC} Military-grade AES-256-GCM encryption"
echo -e "  ${GREEN}⚡${NC} Global hotkey system (Ctrl+Alt+V)"
echo -e "  ${GREEN}🖥️${NC} Desktop application with system tray"
echo -e "  ${GREEN}🛡️${NC} Auto-start background service"
echo ""
echo -e "${BOLD}${YELLOW}📱 How to Use PhantomVault:${NC}"
echo -e "  ${WHITE}1.${NC} Open ${CYAN}'PhantomVault'${NC} from Applications menu"
echo -e "  ${WHITE}2.${NC} Or run ${CYAN}'phantomvault'${NC} in terminal"
echo -e "  ${WHITE}3.${NC} Press ${YELLOW}Ctrl+Alt+V${NC} anywhere on your system"
echo -e "  ${WHITE}4.${NC} Type your password mixed with other text:"
echo -e "     ${DIM}• ${CYAN}hello T1234 world${NC} ${DIM}→ Temporary unlock${NC}"
echo -e "     ${DIM}• ${CYAN}abc P1234 def${NC} ${DIM}→ Permanent unlock${NC}"
echo -e "     ${DIM}• ${CYAN}test 1234 end${NC} ${DIM}→ Default temporary${NC}"
echo ""
echo -e "${BOLD}${BLUE}📁 Installation Details:${NC}"
echo -e "  ${DIM}Application:${NC} $INSTALL_DIR"
echo -e "  ${DIM}Desktop Entry:${NC} $DESKTOP_FILE"
echo -e "  ${DIM}Vault Storage:${NC} $VAULT_STORAGE"
echo -e "  ${DIM}System Service:${NC} $SERVICE_FILE"
echo ""
echo -e "${BOLD}${PURPLE}🔧 Service Status:${NC}"
if systemctl --user is-active phantom-vault.service --quiet 2>/dev/null; then
    echo -e "  ${GREEN}✅ Background service is running${NC}"
else
    echo -e "  ${YELLOW}⚠️  Background service status unknown${NC}"
fi
echo ""
echo -e "${BOLD}${GREEN}🎊 Ready to Experience Invisible Security!${NC}"
echo ""
echo -e "${WHITE}💡 Quick Start Guide:${NC}"
echo -e "  ${CYAN}1.${NC} Launch PhantomVault from Applications menu"
echo -e "  ${CYAN}2.${NC} Create your master password and recovery key"
echo -e "  ${CYAN}3.${NC} Add folders you want to protect"
echo -e "  ${CYAN}4.${NC} Press ${YELLOW}Ctrl+Alt+V${NC} and type your password anywhere!"
echo ""
echo -e "${DIM}🔗 For support and updates: https://github.com/ishaq2321/phantomvault${NC}"
echo ""