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

# Install system dependencies automatically
install_dependencies() {
    log_info "Installing system dependencies..."
    
    # Check for systemd
    if ! command -v systemctl &> /dev/null; then
        log_error "systemd is required but not found"
        exit 1
    fi
    
    # Detect package manager and install dependencies
    if command -v apt-get &> /dev/null; then
        log_info "Detected Debian/Ubuntu system - installing dependencies..."
        export DEBIAN_FRONTEND=noninteractive
        apt-get update -qq
        apt-get install -y -qq \
            libssl3 libssl-dev \
            libx11-6 libx11-dev \
            libxtst6 libxtst-dev \
            libgtk-3-0 \
            libxss1 \
            libgconf-2-4 \
            libxrandr2 \
            libasound2 \
            libpangocairo-1.0-0 \
            libatk1.0-0 \
            libcairo-gobject2 \
            libgdk-pixbuf2.0-0 \
            curl \
            wget
        log_success "Dependencies installed successfully"
        
    elif command -v dnf &> /dev/null; then
        log_info "Detected Fedora system - installing dependencies..."
        dnf install -y -q \
            openssl openssl-devel \
            libX11 libX11-devel \
            libXtst libXtst-devel \
            gtk3 \
            libXScrnSaver \
            GConf2 \
            libXrandr \
            alsa-lib \
            curl \
            wget
        log_success "Dependencies installed successfully"
        
    elif command -v yum &> /dev/null; then
        log_info "Detected RHEL/CentOS system - installing dependencies..."
        yum install -y -q \
            openssl openssl-devel \
            libX11 libX11-devel \
            libXtst libXtst-devel \
            gtk3 \
            libXScrnSaver \
            GConf2 \
            libXrandr \
            alsa-lib \
            curl \
            wget
        log_success "Dependencies installed successfully"
        
    elif command -v pacman &> /dev/null; then
        log_info "Detected Arch Linux system - installing dependencies..."
        pacman -Sy --noconfirm \
            openssl \
            libx11 \
            libxtst \
            gtk3 \
            libxss \
            gconf \
            libxrandr \
            alsa-lib \
            curl \
            wget
        log_success "Dependencies installed successfully"
        
    elif command -v zypper &> /dev/null; then
        log_info "Detected openSUSE system - installing dependencies..."
        zypper install -y \
            libopenssl3 openssl-devel \
            libX11-6 libX11-devel \
            libXtst6 libXtst-devel \
            gtk3-tools \
            libXss1 \
            gconf2 \
            libXrandr2 \
            alsa \
            curl \
            wget
        log_success "Dependencies installed successfully"
        
    else
        log_warning "Unknown package manager - attempting to continue..."
        log_info "If installation fails, please install these packages manually:"
        log_info "- OpenSSL development libraries"
        log_info "- X11 development libraries" 
        log_info "- XTest development libraries"
        log_info "- GTK3 libraries"
    fi
}

# Check system requirements
check_requirements() {
    log_info "Checking system requirements..."
    
    # Install dependencies automatically
    install_dependencies
    
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

# Download and install application files
install_files() {
    log_info "Downloading and installing PhantomVault..."
    
    # Create installation directory
    mkdir -p "$INSTALL_DIR"/{bin,share,var,logs}
    
    # Download service binary
    log_info "Downloading PhantomVault service..."
    if curl -fsSL -o "$INSTALL_DIR/bin/phantomvault-service" \
        "https://github.com/ishaq2321/phantomVault/releases/download/v1.0.0/phantomvault-service-linux"; then
        chmod +x "$INSTALL_DIR/bin/phantomvault-service"
        log_success "Downloaded and installed service binary"
    else
        log_error "Failed to download service binary"
        exit 1
    fi
    
    # Create GUI wrapper script
    log_info "Creating GUI application..."
    cat > "$INSTALL_DIR/bin/phantomvault-gui" << 'EOF'
#!/bin/bash
# PhantomVault GUI Launcher
# This script starts the PhantomVault service and GUI

INSTALL_DIR="/opt/phantomvault"
SERVICE_BIN="$INSTALL_DIR/bin/phantomvault-service"
LOG_FILE="$INSTALL_DIR/logs/phantomvault.log"

# Ensure log directory exists
mkdir -p "$INSTALL_DIR/logs"

# Check if service is running
if ! pgrep -f "phantomvault-service" > /dev/null; then
    echo "Starting PhantomVault service..."
    "$SERVICE_BIN" --daemon --log-level INFO >> "$LOG_FILE" 2>&1 &
    sleep 2
fi

# Start GUI (for now, show service status)
echo "PhantomVault is running!"
echo "Service status:"
if pgrep -f "phantomvault-service" > /dev/null; then
    echo "✅ PhantomVault service is running"
    echo "🔒 Your folders are protected"
    echo ""
    echo "Usage:"
    echo "• Press Ctrl+Alt+V anywhere to access your folders"
    echo "• Run 'phantomvault --help' for more options"
    echo "• Check logs: tail -f $LOG_FILE"
else
    echo "❌ PhantomVault service is not running"
    echo "Try running: sudo systemctl start phantomvault"
fi

# Keep window open for a moment
sleep 5
EOF
    
    chmod +x "$INSTALL_DIR/bin/phantomvault-gui"
    
    # Create application icon (simple text-based for now)
    mkdir -p "$INSTALL_DIR/share/pixmaps"
    cat > "$INSTALL_DIR/share/pixmaps/phantomvault.svg" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<svg width="64" height="64" viewBox="0 0 64 64" xmlns="http://www.w3.org/2000/svg">
  <rect width="64" height="64" rx="8" fill="#2563eb"/>
  <rect x="12" y="20" width="40" height="24" rx="4" fill="none" stroke="#ffffff" stroke-width="2"/>
  <circle cx="32" cy="32" r="6" fill="none" stroke="#ffffff" stroke-width="2"/>
  <circle cx="32" cy="32" r="2" fill="#ffffff"/>
  <text x="32" y="52" text-anchor="middle" fill="#ffffff" font-family="Arial" font-size="8">PV</text>
</svg>
EOF
    
    # Set ownership and permissions
    chown -R root:root "$INSTALL_DIR"
    chown -R "$SERVICE_USER:$SERVICE_USER" "$INSTALL_DIR/var" "$INSTALL_DIR/logs"
    chmod 755 "$INSTALL_DIR/bin/phantomvault-service"
    chmod 755 "$INSTALL_DIR/bin/phantomvault-gui"
    
    log_success "Application files installed to $INSTALL_DIR"
}

# Create systemd service
create_systemd_service() {
    log_info "Creating systemd service..."
    
    cat > "$SYSTEMD_SERVICE" << EOF
[Unit]
Description=PhantomVault - Invisible Folder Security Service
Documentation=https://github.com/ishaq2321/phantomVault
After=network.target graphical-session.target
Wants=network.target

[Service]
Type=simple
User=root
Group=root
ExecStart=$INSTALL_DIR/bin/phantomvault-service --daemon --log-level INFO --port 9876
ExecReload=/bin/kill -HUP \$MAINPID
Restart=always
RestartSec=5
StartLimitInterval=60s
StartLimitBurst=3

# Logging
StandardOutput=append:$INSTALL_DIR/logs/phantomvault.log
StandardError=append:$INSTALL_DIR/logs/phantomvault-error.log

# Security settings
NoNewPrivileges=false
PrivateTmp=true
ProtectSystem=false
ProtectHome=false
ProtectKernelTunables=true
ProtectKernelModules=true
ProtectControlGroups=true

# Resource limits
LimitNOFILE=65536
MemoryMax=50M
CPUQuota=10%

# Environment
Environment=PHANTOMVAULT_DATA_DIR=/home
Environment=PHANTOMVAULT_LOG_LEVEL=INFO
Environment=DISPLAY=:0

[Install]
WantedBy=multi-user.target graphical-session.target
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
Icon=$INSTALL_DIR/share/pixmaps/phantomvault.svg
Terminal=true
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
    
    cat > /usr/local/bin/phantomvault << 'EOF'
#!/bin/bash
# PhantomVault command line interface

INSTALL_DIR="/opt/phantomvault"
SERVICE_BIN="$INSTALL_DIR/bin/phantomvault-service"

# If no arguments, show status
if [[ $# -eq 0 ]]; then
    echo "PhantomVault - Invisible Folder Security"
    echo "========================================"
    
    if pgrep -f "phantomvault-service" > /dev/null; then
        echo "Status: ✅ Running"
        echo "Memory: $(ps -o rss= -p $(pgrep -f phantomvault-service) | awk '{print $1/1024 " MB"}')"
        echo ""
        echo "Quick Actions:"
        echo "• Press Ctrl+Alt+V to access folders"
        echo "• phantomvault --help for more options"
        echo "• phantomvault --gui to open desktop app"
    else
        echo "Status: ❌ Not running"
        echo ""
        echo "Start with: sudo systemctl start phantomvault"
    fi
    exit 0
fi

# Handle special arguments
case "$1" in
    --gui)
        exec "$INSTALL_DIR/bin/phantomvault-gui"
        ;;
    --status)
        systemctl status phantomvault
        ;;
    --logs)
        journalctl -u phantomvault -f
        ;;
    *)
        # Pass all other arguments to service
        exec "$SERVICE_BIN" "$@"
        ;;
esac
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