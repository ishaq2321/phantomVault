#!/bin/bash
# PhantomVault Linux Uninstallation Script
# Removes PhantomVault service and desktop application

set -e

# Configuration
INSTALL_DIR="/opt/phantomvault"
SERVICE_USER="phantomvault"
SERVICE_NAME="phantomvault"
DESKTOP_FILE="/usr/share/applications/phantomvault.desktop"
SYSTEMD_SERVICE="/etc/systemd/system/phantomvault.service"
CLI_TOOL="/usr/local/bin/phantomvault"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        log_error "This script must be run as root (use sudo)"
        exit 1
    fi
}

# Confirm uninstallation
confirm_uninstall() {
    echo "=================================================="
    echo "PhantomVault Uninstallation Script"
    echo "=================================================="
    echo
    log_warning "This will completely remove PhantomVault from your system"
    echo
    echo "The following will be removed:"
    echo "• PhantomVault service and application files"
    echo "• Systemd service configuration"
    echo "• Desktop integration (shortcuts, file associations)"
    echo "• Command line tool"
    echo "• Service user account"
    echo
    log_info "User data in ~/.phantomvault will be preserved"
    echo
    
    read -p "Are you sure you want to continue? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        log_info "Uninstallation cancelled"
        exit 0
    fi
}

# Stop and disable service
stop_service() {
    log_info "Stopping PhantomVault service..."
    
    if systemctl is-active --quiet "$SERVICE_NAME"; then
        systemctl stop "$SERVICE_NAME"
        log_success "Service stopped"
    else
        log_info "Service was not running"
    fi
    
    if systemctl is-enabled --quiet "$SERVICE_NAME"; then
        systemctl disable "$SERVICE_NAME"
        log_success "Service disabled"
    else
        log_info "Service was not enabled"
    fi
}

# Remove systemd service
remove_systemd_service() {
    log_info "Removing systemd service..."
    
    if [[ -f "$SYSTEMD_SERVICE" ]]; then
        rm -f "$SYSTEMD_SERVICE"
        systemctl daemon-reload
        log_success "Systemd service removed"
    else
        log_info "Systemd service file not found"
    fi
}

# Remove application files
remove_files() {
    log_info "Removing application files..."
    
    if [[ -d "$INSTALL_DIR" ]]; then
        rm -rf "$INSTALL_DIR"
        log_success "Application files removed from $INSTALL_DIR"
    else
        log_info "Installation directory not found"
    fi
}

# Remove desktop integration
remove_desktop_integration() {
    log_info "Removing desktop integration..."
    
    # Remove desktop file
    if [[ -f "$DESKTOP_FILE" ]]; then
        rm -f "$DESKTOP_FILE"
        log_success "Desktop entry removed"
    fi
    
    # Update desktop database
    if command -v update-desktop-database &> /dev/null; then
        update-desktop-database /usr/share/applications
    fi
    
    # Remove MIME type associations
    if [[ -f "/usr/share/mime/packages/phantomvault.xml" ]]; then
        rm -f "/usr/share/mime/packages/phantomvault.xml"
        if command -v update-mime-database &> /dev/null; then
            update-mime-database /usr/share/mime
        fi
    fi
}

# Remove command line tool
remove_cli_tool() {
    log_info "Removing command line tool..."
    
    if [[ -f "$CLI_TOOL" ]]; then
        rm -f "$CLI_TOOL"
        log_success "Command line tool removed"
    else
        log_info "Command line tool not found"
    fi
}

# Remove service user
remove_service_user() {
    log_info "Removing service user..."
    
    if id "$SERVICE_USER" &>/dev/null; then
        # Kill any remaining processes
        pkill -u "$SERVICE_USER" || true
        
        # Remove user
        userdel "$SERVICE_USER" 2>/dev/null || true
        log_success "Service user removed"
    else
        log_info "Service user not found"
    fi
}

# Clean up logs and temporary files
cleanup_logs() {
    log_info "Cleaning up logs and temporary files..."
    
    # Remove systemd logs
    if command -v journalctl &> /dev/null; then
        journalctl --vacuum-time=1s --unit="$SERVICE_NAME" &>/dev/null || true
    fi
    
    # Remove any temporary files
    rm -rf /tmp/phantomvault-* 2>/dev/null || true
    
    log_success "Cleanup completed"
}

# Offer to remove user data
offer_data_removal() {
    echo
    log_info "User data locations that may contain PhantomVault data:"
    echo "• ~/.phantomvault (user profiles and encrypted folders)"
    echo "• ~/.config/phantomvault (application settings)"
    echo
    
    read -p "Do you want to remove user data as well? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        log_warning "Removing user data from all user home directories..."
        
        # Remove data from all user home directories
        for home_dir in /home/*; do
            if [[ -d "$home_dir" ]]; then
                local username=$(basename "$home_dir")
                if [[ -d "$home_dir/.phantomvault" ]]; then
                    rm -rf "$home_dir/.phantomvault"
                    log_info "Removed data for user: $username"
                fi
                if [[ -d "$home_dir/.config/phantomvault" ]]; then
                    rm -rf "$home_dir/.config/phantomvault"
                fi
            fi
        done
        
        # Remove root user data
        if [[ -d "/root/.phantomvault" ]]; then
            rm -rf "/root/.phantomvault"
            log_info "Removed data for root user"
        fi
        
        log_success "User data removed"
    else
        log_info "User data preserved"
        echo "To manually remove user data later:"
        echo "  rm -rf ~/.phantomvault"
        echo "  rm -rf ~/.config/phantomvault"
    fi
}

# Main uninstallation function
main() {
    check_root
    confirm_uninstall
    
    echo
    log_info "Starting PhantomVault uninstallation..."
    
    stop_service
    remove_systemd_service
    remove_files
    remove_desktop_integration
    remove_cli_tool
    remove_service_user
    cleanup_logs
    
    offer_data_removal
    
    echo
    log_success "PhantomVault has been successfully uninstalled!"
    echo
    log_info "Thank you for using PhantomVault"
    log_info "If you encountered any issues, please report them at:"
    log_info "https://github.com/ishaq2321/phantomVault/issues"
}

# Run main function
main "$@"