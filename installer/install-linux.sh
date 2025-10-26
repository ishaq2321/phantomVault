#!/bin/bash
# PhantomVault Linux Installation Script
# Installs PhantomVault service and desktop application

set -e

# Configuration
INSTALL_DIR="/opt/phantomvault"
SERVICE_USER="phantomvault"
SERVICE_NAME="phantomvault"
DESKTOP_FILE="/usr/share/applications/phantomvault.desktop"
SYSTEMD_SERVICE="/etc/systemd/system/phantomvault.service"

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

# Check system requirements
check_requirements() {
    log_info "Checking system requirements..."
    
    # Check for required packages
    local missing_packages=()
    
    # Check for systemd
    if ! command -v systemctl &> /dev/null; then
        log_error "systemd is required but not found"
        exit 1
    fi
    
    # Check for required libraries
    local required_libs=("libssl" "libcrypto" "libx11" "libxtst")
    for lib in "${required_libs[@]}"; do
        if ! ldconfig -p | grep -q "$lib"; then
            missing_packages+=("$lib")
        fi
    done
    
    if [[ ${#missing_packages[@]} -gt 0 ]]; then
        log_warning "Missing required libraries: ${missing_packages[*]}"
        log_info "Please install them using your package manager"
        
        # Suggest installation commands for common distros
        if command -v apt-get &> /dev/null; then
            log_info "Ubuntu/Debian: sudo apt-get install libssl-dev libx11-dev libxtst-dev"
        elif command -v yum &> /dev/null; then
            log_info "RHEL/CentOS: sudo yum install openssl-devel libX11-devel libXtst-devel"
        elif command -v dnf &> /dev/null; then
            log_info "Fedora: sudo dnf install openssl-devel libX11-devel libXtst-devel"
        fi
        
        read -p "Continue anyway? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            exit 1
        fi
    fi
    
    log_success "System requirements check completed"
}

# Create service user
create_service_user() {
    log_info "Creating service user..."
    
    if id "$SERVICE_USER" &>/dev/null; then
        log_info "User $SERVICE_USER already exists"
    else
        useradd --system --no-create-home --shell /bin/false "$SERVICE_USER"
        log_success "Created service user: $SERVICE_USER"
    fi
}

# Install application files
install_files() {
    log_info "Installing application files..."
    
    # Create installation directory
    mkdir -p "$INSTALL_DIR"/{bin,lib,share,var}
    
    # Copy service binary
    if [[ -f "phantomvault-service" ]]; then
        cp phantomvault-service "$INSTALL_DIR/bin/"
        chmod +x "$INSTALL_DIR/bin/phantomvault-service"
        log_success "Installed service binary"
    else
        log_error "Service binary not found"
        exit 1
    fi
    
    # Copy GUI application (if available)
    if [[ -d "gui" ]]; then
        cp -r gui/* "$INSTALL_DIR/share/"
        log_success "Installed GUI application"
    fi
    
    # Set ownership and permissions
    chown -R root:root "$INSTALL_DIR"
    chown -R "$SERVICE_USER:$SERVICE_USER" "$INSTALL_DIR/var"
    chmod 755 "$INSTALL_DIR/bin/phantomvault-service"
    
    log_success "Application files installed to $INSTALL_DIR"
}

# Create systemd service
create_systemd_service() {
    log_info "Creating systemd service..."
    
    cat > "$SYSTEMD_SERVICE" << EOF
[Unit]
Description=PhantomVault - Invisible Folder Security Service
Documentation=https://github.com/ishaq2321/phantomVault
After=network.target
Wants=network.target

[Service]
Type=simple
User=$SERVICE_USER
Group=$SERVICE_USER
ExecStart=$INSTALL_DIR/bin/phantomvault-service --daemon --log-level INFO
ExecReload=/bin/kill -HUP \$MAINPID
Restart=always
RestartSec=5
StartLimitInterval=60s
StartLimitBurst=3

# Security settings
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/home
ProtectKernelTunables=true
ProtectKernelModules=true
ProtectControlGroups=true

# Resource limits
LimitNOFILE=65536
MemoryMax=50M
CPUQuota=10%

# Environment
Environment=PHANTOMVAULT_DATA_DIR=/home/%i/.phantomvault
Environment=PHANTOMVAULT_LOG_LEVEL=INFO

[Install]
WantedBy=multi-user.target
EOF

    # Reload systemd and enable service
    systemctl daemon-reload
    systemctl enable "$SERVICE_NAME"
    
    log_success "Created and enabled systemd service"
}

# Create desktop entry
create_desktop_entry() {
    log_info "Creating desktop entry..."
    
    cat > "$DESKTOP_FILE" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=PhantomVault
Comment=Invisible Folder Security with Profile-Based Management
Exec=$INSTALL_DIR/bin/phantomvault-gui
Icon=$INSTALL_DIR/share/assets/icon.png
Terminal=false
Categories=Security;Utility;FileManager;
Keywords=security;encryption;privacy;folders;vault;
StartupWMClass=PhantomVault
MimeType=application/x-phantomvault-profile;
EOF

    # Update desktop database
    if command -v update-desktop-database &> /dev/null; then
        update-desktop-database /usr/share/applications
    fi
    
    log_success "Created desktop entry"
}

# Create command line tool
create_cli_tool() {
    log_info "Creating command line tool..."
    
    cat > /usr/local/bin/phantomvault << EOF
#!/bin/bash
# PhantomVault command line interface
exec $INSTALL_DIR/bin/phantomvault-service "\$@"
EOF

    chmod +x /usr/local/bin/phantomvault
    log_success "Created command line tool: phantomvault"
}

# Start service
start_service() {
    log_info "Starting PhantomVault service..."
    
    if systemctl start "$SERVICE_NAME"; then
        log_success "PhantomVault service started successfully"
        
        # Check service status
        sleep 2
        if systemctl is-active --quiet "$SERVICE_NAME"; then
            log_success "Service is running and healthy"
        else
            log_warning "Service started but may not be healthy"
            log_info "Check status with: systemctl status $SERVICE_NAME"
        fi
    else
        log_error "Failed to start PhantomVault service"
        log_info "Check logs with: journalctl -u $SERVICE_NAME"
        exit 1
    fi
}

# Main installation function
main() {
    echo "=================================================="
    echo "PhantomVault Installation Script"
    echo "Invisible Folder Security with Profile Management"
    echo "=================================================="
    echo
    
    check_root
    check_requirements
    create_service_user
    install_files
    create_systemd_service
    create_desktop_entry
    create_cli_tool
    start_service
    
    echo
    log_success "PhantomVault installation completed successfully!"
    echo
    echo "Next steps:"
    echo "1. Launch PhantomVault from your applications menu"
    echo "2. Or use the command line: phantomvault --help"
    echo "3. Check service status: systemctl status phantomvault"
    echo "4. View logs: journalctl -u phantomvault -f"
    echo
    echo "For support, visit: https://github.com/ishaq2321/phantomVault"
}

# Run main function
main "$@"