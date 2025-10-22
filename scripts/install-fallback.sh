#!/bin/bash

# PhantomVault Fallback Installation Script
# For systems without systemd or with compatibility issues

set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BINARY_NAME="phantom_vault_service"
BUILD_DIR="build/bin"
INSTALL_DIR="$HOME/.local/bin"
AUTOSTART_DIR="$HOME/.config/autostart"

echo -e "${BLUE}PhantomVault Fallback Installation${NC}"
echo "==================================="

# Check if running as root
if [[ $EUID -eq 0 ]]; then
   echo -e "${RED}Error: This script should not be run as root${NC}"
   exit 1
fi

# Detect system if available
if [[ -f "$SCRIPT_DIR/detect-system.sh" ]]; then
    source "$SCRIPT_DIR/detect-system.sh"
    main > /dev/null 2>&1 || true
fi

# Check if binary exists
if [[ ! -f "$BUILD_DIR/$BINARY_NAME" ]]; then
    echo -e "${RED}Error: Service binary not found at $BUILD_DIR/$BINARY_NAME${NC}"
    echo "Please build the project first with: make -C build"
    exit 1
fi

# Create directories
echo -e "${YELLOW}Creating directories...${NC}"
mkdir -p "$INSTALL_DIR"
mkdir -p "$AUTOSTART_DIR"
mkdir -p "$HOME/.phantom_vault_storage"
mkdir -p "$HOME/.config/phantom-vault"
mkdir -p "$HOME/.local/share/phantom-vault"

# Copy binary
echo -e "${YELLOW}Installing service binary...${NC}"
cp "$BUILD_DIR/$BINARY_NAME" "$INSTALL_DIR/"
chmod +x "$INSTALL_DIR/$BINARY_NAME"

# Create desktop autostart file
echo -e "${YELLOW}Creating autostart entry...${NC}"
cat > "$AUTOSTART_DIR/phantom-vault.desktop" << EOF
[Desktop Entry]
Type=Application
Name=PhantomVault Background Service
Comment=Invisible folder encryption service with global hotkeys
Exec=$INSTALL_DIR/$BINARY_NAME
Hidden=false
NoDisplay=true
X-GNOME-Autostart-enabled=true
StartupNotify=false
Terminal=false
Categories=Security;Utility;
EOF

# Create startup script for non-XDG systems
echo -e "${YELLOW}Creating startup script...${NC}"
cat > "$HOME/.local/bin/start-phantom-vault.sh" << 'EOF'
#!/bin/bash

# PhantomVault Startup Script
# This script starts PhantomVault in the background

BINARY="$HOME/.local/bin/phantom_vault_service"
PIDFILE="$HOME/.local/share/phantom-vault/phantom-vault.pid"

# Check if already running
if [[ -f "$PIDFILE" ]]; then
    PID=$(cat "$PIDFILE")
    if kill -0 "$PID" 2>/dev/null; then
        echo "PhantomVault is already running (PID: $PID)"
        exit 0
    else
        rm -f "$PIDFILE"
    fi
fi

# Start the service
if [[ -x "$BINARY" ]]; then
    echo "Starting PhantomVault background service..."
    nohup "$BINARY" > "$HOME/.local/share/phantom-vault/phantom-vault.log" 2>&1 &
    PID=$!
    echo $PID > "$PIDFILE"
    echo "PhantomVault started with PID: $PID"
    echo "Global hotkeys active:"
    echo "  Ctrl+Alt+V - Unlock/Lock folders"
    echo "  Ctrl+Alt+R - Recovery key input"
else
    echo "Error: PhantomVault binary not found at $BINARY"
    exit 1
fi
EOF

chmod +x "$HOME/.local/bin/start-phantom-vault.sh"

# Create stop script
cat > "$HOME/.local/bin/stop-phantom-vault.sh" << 'EOF'
#!/bin/bash

# PhantomVault Stop Script

PIDFILE="$HOME/.local/share/phantom-vault/phantom-vault.pid"

if [[ -f "$PIDFILE" ]]; then
    PID=$(cat "$PIDFILE")
    if kill -0 "$PID" 2>/dev/null; then
        echo "Stopping PhantomVault (PID: $PID)..."
        kill "$PID"
        rm -f "$PIDFILE"
        echo "PhantomVault stopped"
    else
        echo "PhantomVault is not running"
        rm -f "$PIDFILE"
    fi
else
    echo "PhantomVault is not running (no PID file)"
fi
EOF

chmod +x "$HOME/.local/bin/stop-phantom-vault.sh"

# Start the service
echo -e "${YELLOW}Starting PhantomVault service...${NC}"
if "$HOME/.local/bin/start-phantom-vault.sh"; then
    echo -e "${GREEN}✅ Service started successfully${NC}"
else
    echo -e "${RED}❌ Failed to start service${NC}"
    exit 1
fi

# Check if service is running
sleep 2
PIDFILE="$HOME/.local/share/phantom-vault/phantom-vault.pid"
if [[ -f "$PIDFILE" ]]; then
    PID=$(cat "$PIDFILE")
    if kill -0 "$PID" 2>/dev/null; then
        echo -e "${GREEN}✅ Service is running (PID: $PID)${NC}"
    else
        echo -e "${RED}❌ Service failed to start${NC}"
        exit 1
    fi
fi

echo -e "\n${GREEN}Fallback installation completed successfully!${NC}"

echo -e "\n${BLUE}Auto-start Configuration:${NC}"
case "$DESKTOP_ENV" in
    "GNOME"|"KDE"|"XFCE"|"MATE"|"CINNAMON"|"LXDE")
        echo "  ✅ Desktop autostart file created"
        echo "  Service will start automatically when you log in"
        ;;
    *)
        echo "  ⚠️  Unknown desktop environment"
        echo "  You may need to manually add startup configuration"
        echo "  Add this to your shell profile (.bashrc, .profile, etc.):"
        echo "    $HOME/.local/bin/start-phantom-vault.sh"
        ;;
esac

echo -e "\n${BLUE}Global Hotkeys:${NC}"
echo "  Ctrl+Alt+V - Unlock/Lock folders"
echo "  Ctrl+Alt+R - Recovery key input"

echo -e "\n${BLUE}Service Management:${NC}"
echo "  Start:   $HOME/.local/bin/start-phantom-vault.sh"
echo "  Stop:    $HOME/.local/bin/stop-phantom-vault.sh"
echo "  Logs:    tail -f $HOME/.local/share/phantom-vault/phantom-vault.log"

echo -e "\n${BLUE}Files Created:${NC}"
echo "  Binary: $INSTALL_DIR/$BINARY_NAME"
echo "  Autostart: $AUTOSTART_DIR/phantom-vault.desktop"
echo "  Scripts: $HOME/.local/bin/start-phantom-vault.sh"
echo "           $HOME/.local/bin/stop-phantom-vault.sh"