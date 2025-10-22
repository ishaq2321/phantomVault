#!/bin/bash

# PhantomVault Service Installation Script
# This script installs the PhantomVault background service as a systemd user service

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
SERVICE_NAME="phantom-vault"
BINARY_NAME="phantom_vault_service"
BUILD_DIR="build/bin"
SYSTEMD_DIR="$HOME/.config/systemd/user"
INSTALL_DIR="$HOME/.local/bin"

echo -e "${BLUE}PhantomVault Service Installation${NC}"
echo "=================================="

# Check if running as root
if [[ $EUID -eq 0 ]]; then
   echo -e "${RED}Error: This script should not be run as root${NC}"
   echo "PhantomVault runs as a user service for security reasons"
   exit 1
fi

# Run system detection
echo -e "${YELLOW}Detecting system configuration...${NC}"
if [[ -f "$SCRIPT_DIR/detect-system.sh" ]]; then
    source "$SCRIPT_DIR/detect-system.sh"
    if ! main; then
        echo -e "${YELLOW}⚠️  System compatibility issues detected${NC}"
        read -p "Continue with installation anyway? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo "Installation cancelled"
            exit 1
        fi
    fi
else
    echo -e "${YELLOW}⚠️  System detection script not found, continuing...${NC}"
fi

# Check if binary exists
if [[ ! -f "$BUILD_DIR/$BINARY_NAME" ]]; then
    echo -e "${RED}Error: Service binary not found at $BUILD_DIR/$BINARY_NAME${NC}"
    echo "Please build the project first with: make -C build"
    exit 1
fi

# Check if systemd is available
if ! command -v systemctl &> /dev/null; then
    echo -e "${RED}Error: systemd not found${NC}"
    echo "This installation script requires systemd"
    exit 1
fi

# Create directories
echo -e "${YELLOW}Creating directories...${NC}"
mkdir -p "$SYSTEMD_DIR"
mkdir -p "$INSTALL_DIR"
mkdir -p "$HOME/.phantom_vault_storage"
mkdir -p "$HOME/.config/phantom-vault"
mkdir -p "$HOME/.local/share/phantom-vault"

# Copy binary
echo -e "${YELLOW}Installing service binary...${NC}"
cp "$BUILD_DIR/$BINARY_NAME" "$INSTALL_DIR/"
chmod +x "$INSTALL_DIR/$BINARY_NAME"

# Copy systemd service file
echo -e "${YELLOW}Installing systemd service...${NC}"
cp "systemd/$SERVICE_NAME.service" "$SYSTEMD_DIR/"

# Stop existing service if running
if systemctl --user is-active --quiet "$SERVICE_NAME" 2>/dev/null; then
    echo -e "${YELLOW}Stopping existing service...${NC}"
    systemctl --user stop "$SERVICE_NAME"
fi

# Reload systemd and enable service
echo -e "${YELLOW}Configuring systemd service...${NC}"
systemctl --user daemon-reload
systemctl --user enable "$SERVICE_NAME"

# Start the service
echo -e "${YELLOW}Starting PhantomVault service...${NC}"
if systemctl --user start "$SERVICE_NAME"; then
    echo -e "${GREEN}✅ Service started successfully${NC}"
else
    echo -e "${RED}❌ Failed to start service${NC}"
    echo "Check logs with: journalctl --user -u $SERVICE_NAME -f"
    exit 1
fi

# Check service status
sleep 2
if systemctl --user is-active --quiet "$SERVICE_NAME"; then
    echo -e "${GREEN}✅ Service is running${NC}"
    
    # Show service status
    echo -e "\n${BLUE}Service Status:${NC}"
    systemctl --user status "$SERVICE_NAME" --no-pager -l
    
    echo -e "\n${GREEN}Installation completed successfully!${NC}"
    echo -e "\n${BLUE}Global Hotkeys:${NC}"
    echo "  Ctrl+Alt+V - Unlock/Lock folders"
    echo "  Ctrl+Alt+R - Recovery key input"
    
    echo -e "\n${BLUE}Service Management:${NC}"
    echo "  Status:  systemctl --user status $SERVICE_NAME"
    echo "  Stop:    systemctl --user stop $SERVICE_NAME"
    echo "  Start:   systemctl --user start $SERVICE_NAME"
    echo "  Restart: systemctl --user restart $SERVICE_NAME"
    echo "  Logs:    journalctl --user -u $SERVICE_NAME -f"
    
    echo -e "\n${BLUE}Auto-start:${NC}"
    echo "  The service will automatically start when you log in"
    echo "  To disable: systemctl --user disable $SERVICE_NAME"
    
else
    echo -e "${RED}❌ Service failed to start${NC}"
    echo "Check logs with: journalctl --user -u $SERVICE_NAME -f"
    exit 1
fi