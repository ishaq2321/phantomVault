#!/bin/bash

# PhantomVault Service Uninstallation Script
# This script removes the PhantomVault background service

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SERVICE_NAME="phantom-vault"
BINARY_NAME="phantom_vault_service"
SYSTEMD_DIR="$HOME/.config/systemd/user"
INSTALL_DIR="$HOME/.local/bin"

echo -e "${BLUE}PhantomVault Service Uninstallation${NC}"
echo "===================================="

# Check if running as root
if [[ $EUID -eq 0 ]]; then
   echo -e "${RED}Error: This script should not be run as root${NC}"
   exit 1
fi

# Check if systemd is available
if ! command -v systemctl &> /dev/null; then
    echo -e "${RED}Error: systemd not found${NC}"
    exit 1
fi

# Stop and disable service
if systemctl --user is-enabled --quiet "$SERVICE_NAME" 2>/dev/null; then
    echo -e "${YELLOW}Stopping and disabling service...${NC}"
    systemctl --user stop "$SERVICE_NAME" 2>/dev/null || true
    systemctl --user disable "$SERVICE_NAME"
    echo -e "${GREEN}✅ Service stopped and disabled${NC}"
else
    echo -e "${YELLOW}Service not enabled, skipping...${NC}"
fi

# Remove systemd service file
if [[ -f "$SYSTEMD_DIR/$SERVICE_NAME.service" ]]; then
    echo -e "${YELLOW}Removing systemd service file...${NC}"
    rm "$SYSTEMD_DIR/$SERVICE_NAME.service"
    echo -e "${GREEN}✅ Service file removed${NC}"
fi

# Remove binary
if [[ -f "$INSTALL_DIR/$BINARY_NAME" ]]; then
    echo -e "${YELLOW}Removing service binary...${NC}"
    rm "$INSTALL_DIR/$BINARY_NAME"
    echo -e "${GREEN}✅ Binary removed${NC}"
fi

# Reload systemd
echo -e "${YELLOW}Reloading systemd...${NC}"
systemctl --user daemon-reload

echo -e "\n${GREEN}Uninstallation completed successfully!${NC}"

# Ask about data removal
echo -e "\n${BLUE}Data Directories:${NC}"
echo "The following directories contain your vault data:"
echo "  $HOME/.phantom_vault_storage"
echo "  $HOME/.config/phantom-vault"
echo "  $HOME/.local/share/phantom-vault"
echo ""
echo -e "${YELLOW}These directories have NOT been removed automatically.${NC}"
echo -e "${RED}WARNING: Removing these directories will delete all your vault data!${NC}"
echo ""
read -p "Do you want to remove all PhantomVault data? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo -e "${YELLOW}Removing data directories...${NC}"
    rm -rf "$HOME/.phantom_vault_storage" 2>/dev/null || true
    rm -rf "$HOME/.config/phantom-vault" 2>/dev/null || true
    rm -rf "$HOME/.local/share/phantom-vault" 2>/dev/null || true
    echo -e "${GREEN}✅ Data directories removed${NC}"
else
    echo -e "${BLUE}Data directories preserved${NC}"
fi

echo -e "\n${GREEN}PhantomVault service has been completely uninstalled.${NC}"