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
        # Install base dependencies
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
        
        # Install Node.js and npm if not present
        if ! command -v node &> /dev/null || ! command -v npm &> /dev/null; then
            log_info "Installing Node.js and npm..."
            apt-get install -y -qq nodejs npm || log_warning "Failed to install nodejs/npm - GUI may not work"
        fi
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
        useradd --system --create-home --shell /bin/false "$SERVICE_USER"
        log_success "Created service user: $SERVICE_USER"
    fi
    
    # Ensure home directory exists and has proper permissions
    if [[ ! -d "/home/$SERVICE_USER" ]]; then
        mkdir -p "/home/$SERVICE_USER"
        chown "$SERVICE_USER:$SERVICE_USER" "/home/$SERVICE_USER"
        chmod 755 "/home/$SERVICE_USER"
        log_success "Created home directory for $SERVICE_USER"
    fi
    
    # Add current user to systemd-journal group for log access
    if [[ -n "$SUDO_USER" ]]; then
        usermod -a -G systemd-journal "$SUDO_USER" 2>/dev/null || log_warning "Could not add user to systemd-journal group"
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
    
    # Download and install GUI application
    log_info "Downloading PhantomVault GUI..."
    if curl -fsSL -o "/tmp/phantomvault-gui.tar.gz" \
        "https://github.com/ishaq2321/phantomVault/releases/download/v1.0.0/phantomvault-gui-linux.tar.gz"; then
        
        # Extract GUI to installation directory
        mkdir -p "$INSTALL_DIR/gui"
        tar -xzf "/tmp/phantomvault-gui.tar.gz" -C "$INSTALL_DIR/gui"
        rm "/tmp/phantomvault-gui.tar.gz"
        
        # Set permissions
        chown -R root:root "$INSTALL_DIR/gui"
        chmod -R 755 "$INSTALL_DIR/gui"
        
        log_success "Downloaded and installed GUI application"
        
        # Install Electron globally if not present
        if ! command -v electron &> /dev/null; then
            log_info "Installing Electron..."
            if npm install -g electron@latest 2>/dev/null; then
                log_success "Electron installed successfully"
            else
                log_warning "Failed to install Electron globally - GUI may not work"
            fi
        fi
    else
        log_warning "Failed to download GUI application - GUI will not be available"
    fi
    
    # Create GUI wrapper script
    log_info "Creating GUI application..."
    cat > "$INSTALL_DIR/bin/phantomvault-gui" << 'EOF'
#!/bin/bash
# PhantomVault GUI Launcher
# This script starts the PhantomVault GUI application

INSTALL_DIR="/opt/phantomvault"
GUI_DIR="$INSTALL_DIR/gui"
SERVICE_BIN="$INSTALL_DIR/bin/phantomvault-service"
LOG_FILE="$INSTALL_DIR/logs/phantomvault.log"

# Function to check if service is running
is_service_running() {
    systemctl is-active --quiet phantomvault 2>/dev/null
}

# Function to check if service is responding
is_service_responding() {
    curl -s --connect-timeout 2 http://localhost:9876/health >/dev/null 2>&1
}

# Function to start service if not running
ensure_service_running() {
    if ! is_service_running; then
        echo "PhantomVault service is not running. Starting it..."
        
        # Try to start the service using systemctl (no sudo needed if user is in right groups)
        if systemctl --user start phantomvault 2>/dev/null; then
            echo "Service started successfully"
        elif pkexec systemctl start phantomvault 2>/dev/null; then
            echo "Service started with elevated privileges"
        else
            echo "Failed to start service automatically."
            echo "Please run: sudo systemctl start phantomvault"
            echo "Or check if the service is installed correctly."
            return 1
        fi
        
        # Wait for service to be ready
        echo "Waiting for service to be ready..."
        for i in {1..10}; do
            if is_service_running && is_service_responding; then
                echo "Service is ready!"
                return 0
            fi
            sleep 1
        done
        
        echo "Service started but may not be fully ready"
        return 1
    fi
    return 0
}

# Check if GUI is available and start it
start_gui() {
    if [[ -d "$GUI_DIR" && -f "$GUI_DIR/package.json" ]]; then
        # Change to GUI directory
        cd "$GUI_DIR" || {
            echo "Failed to access GUI directory"
            return 1
        }
        
        # Try to start Electron GUI
        if command -v electron &> /dev/null; then
            echo "Starting PhantomVault GUI..."
            electron . 2>/dev/null &
            return 0
        elif command -v node &> /dev/null && [[ -f "$GUI_DIR/node_modules/.bin/electron" ]]; then
            echo "Starting PhantomVault GUI..."
            "$GUI_DIR/node_modules/.bin/electron" . 2>/dev/null &
            return 0
        else
            echo "Electron not found. Please install it:"
            echo "sudo npm install -g electron"
            return 1
        fi
    else
        echo "GUI application not found at $GUI_DIR"
        echo "Please reinstall PhantomVault to get the GUI"
        return 1
    fi
}

# Main execution
main() {
    # Ensure service is running
    if ! ensure_service_running; then
        echo "Cannot start GUI without the service running"
        exit 1
    fi
    
    # Start the GUI
    if ! start_gui; then
        echo "Failed to start GUI, showing status instead:"
        echo ""
        echo "PhantomVault Status"
        echo "=================="
        if is_service_running; then
            echo "✅ PhantomVault service is running"
            echo "🔒 Your folders are protected"
            echo ""
            echo "Usage:"
            echo "• Press Ctrl+Alt+V anywhere to access your folders"
            echo "• Run 'phantomvault --help' for more options"
        else
            echo "❌ PhantomVault service is not running"
            echo "Try running: sudo systemctl start phantomvault"
        fi
        
        # Keep terminal open for a moment if running in terminal
        if [[ -t 1 ]]; then
            echo ""
            echo "Press Enter to continue..."
            read -r
        fi
    fi
}

# Run main function
main "$@"
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
After=network.target
Wants=network.target

[Service]
Type=forking
User=$SERVICE_USER
Group=$SERVICE_USER
WorkingDirectory=/home/$SERVICE_USER
Environment=HOME=/home/$SERVICE_USER
ExecStart=$INSTALL_DIR/bin/phantomvault-service --daemon --log-level INFO --port 9876
ExecReload=/bin/kill -HUP \$MAINPID
Restart=on-failure
RestartSec=10
StartLimitInterval=300s
StartLimitBurst=5

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

[Install]
WantedBy=multi-user.target
EOF

    # Reload systemd and enable service
    systemctl daemon-reload
    systemctl enable "$SERVICE_NAME"
    
    log_success "Created and enabled systemd service"
    
    # Create polkit rule to allow users to manage PhantomVault service
    log_info "Creating polkit rule for service management..."
    mkdir -p /etc/polkit-1/rules.d
    cat > /etc/polkit-1/rules.d/50-phantomvault.rules << 'POLKIT_EOF'
// Allow users to manage PhantomVault service
polkit.addRule(function(action, subject) {
    if (action.id == "org.freedesktop.systemd1.manage-units" &&
        action.lookup("unit") == "phantomvault.service" &&
        subject.isInGroup("users")) {
        return polkit.Result.YES;
    }
});

polkit.addRule(function(action, subject) {
    if (action.id == "org.freedesktop.systemd1.manage-unit-files" &&
        action.lookup("unit") == "phantomvault.service" &&
        subject.isInGroup("users")) {
        return polkit.Result.YES;
    }
});
POLKIT_EOF
    
    # Reload polkit rules
    if command -v systemctl &> /dev/null; then
        systemctl reload polkit 2>/dev/null || true
    fi
    
    log_success "Created polkit rule for service management"
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
Terminal=false
Categories=Security;Utility;FileManager;
Keywords=security;encryption;privacy;folders;vault;
StartupWMClass=PhantomVault
MimeType=application/x-phantomvault-profile;
StartupNotify=true
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

# Function to check if service is running
is_service_running() {
    systemctl is-active --quiet phantomvault 2>/dev/null
}

# Function to get service status
get_service_status() {
    if is_service_running; then
        echo "✅ Running"
        if command -v ps &> /dev/null && pgrep -f "phantomvault-service" > /dev/null; then
            local memory=$(ps -o rss= -p $(pgrep -f phantomvault-service) 2>/dev/null | awk '{print $1/1024 " MB"}')
            echo "Memory: ${memory:-"Unknown"}"
        fi
    else
        echo "❌ Not running"
    fi
}

# If no arguments, show status
if [[ $# -eq 0 ]]; then
    echo "PhantomVault - Invisible Folder Security"
    echo "========================================"
    echo "Status: $(get_service_status)"
    echo ""
    
    if is_service_running; then
        echo "Quick Actions:"
        echo "• Press Ctrl+Alt+V to access folders"
        echo "• phantomvault --gui to open desktop app"
        echo "• phantomvault --help for more options"
    else
        echo "Service is not running."
        echo "Start with: sudo systemctl start phantomvault"
        echo "Or check installation: phantomvault --status"
    fi
    exit 0
fi

# Handle special arguments
case "$1" in
    --gui)
        exec "$INSTALL_DIR/bin/phantomvault-gui"
        ;;
    --status)
        echo "Service Status:"
        systemctl status phantomvault --no-pager -l
        ;;
    --logs)
        echo "PhantomVault Logs (press Ctrl+C to exit):"
        journalctl -u phantomvault -f --no-pager
        ;;
    --start)
        echo "Starting PhantomVault service..."
        if pkexec systemctl start phantomvault; then
            echo "Service started successfully"
        else
            echo "Failed to start service"
            exit 1
        fi
        ;;
    --stop)
        echo "Stopping PhantomVault service..."
        if pkexec systemctl stop phantomvault; then
            echo "Service stopped successfully"
        else
            echo "Failed to stop service"
            exit 1
        fi
        ;;
    --restart)
        echo "Restarting PhantomVault service..."
        if pkexec systemctl restart phantomvault; then
            echo "Service restarted successfully"
        else
            echo "Failed to restart service"
            exit 1
        fi
        ;;
    --help|-h)
        echo "PhantomVault - Invisible Folder Security"
        echo "========================================"
        echo ""
        echo "Usage: phantomvault [OPTION]"
        echo ""
        echo "Options:"
        echo "  (no args)     Show service status"
        echo "  --gui         Open graphical interface"
        echo "  --status      Show detailed service status"
        echo "  --logs        Show service logs (live)"
        echo "  --start       Start the service"
        echo "  --stop        Stop the service"
        echo "  --restart     Restart the service"
        echo "  --help, -h    Show this help message"
        echo ""
        echo "Service Options (passed to phantomvault-service):"
        echo "  --version     Show service version"
        echo ""
        echo "Examples:"
        echo "  phantomvault              # Show status"
        echo "  phantomvault --gui        # Open GUI"
        echo "  phantomvault --start      # Start service"
        echo "  phantomvault --logs       # View logs"
        ;;
    *)
        # Pass all other arguments to service binary
        if [[ -x "$SERVICE_BIN" ]]; then
            exec "$SERVICE_BIN" "$@"
        else
            echo "PhantomVault service binary not found at $SERVICE_BIN"
            echo "Please check your installation"
            exit 1
        fi
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