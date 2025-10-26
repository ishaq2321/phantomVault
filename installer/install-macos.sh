#!/bin/bash
# PhantomVault macOS Installation Script
# Installs PhantomVault service and desktop application

set -e

# Configuration
INSTALL_DIR="/Applications/PhantomVault.app"
SERVICE_DIR="/Library/LaunchDaemons"
SERVICE_PLIST="dev.phantomvault.service.plist"
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

# Check if running as root for system-wide installation
check_permissions() {
    if [[ $EUID -ne 0 ]]; then
        log_warning "Running without root privileges"
        log_info "Some features may require administrator access"
        
        # Ask for user preference
        echo "Installation options:"
        echo "1. User installation (current user only)"
        echo "2. System installation (requires sudo)"
        read -p "Choose option (1-2): " -n 1 -r
        echo
        
        if [[ $REPLY == "2" ]]; then
            log_info "Requesting administrator privileges..."
            exec sudo "$0" "$@"
        fi
        
        # Adjust paths for user installation
        INSTALL_DIR="$HOME/Applications/PhantomVault.app"
        SERVICE_DIR="$HOME/Library/LaunchAgents"
        CLI_TOOL="$HOME/.local/bin/phantomvault"
    fi
}

# Check system requirements
check_requirements() {
    log_info "Checking system requirements..."
    
    # Check macOS version
    local macos_version=$(sw_vers -productVersion)
    local major_version=$(echo "$macos_version" | cut -d. -f1)
    
    if [[ $major_version -lt 10 ]]; then
        log_error "macOS 10.15 or later is required (found: $macos_version)"
        exit 1
    fi
    
    # Check for required frameworks
    local frameworks_dir="/System/Library/Frameworks"
    local required_frameworks=("Security.framework" "CoreFoundation.framework" "ApplicationServices.framework")
    
    for framework in "${required_frameworks[@]}"; do
        if [[ ! -d "$frameworks_dir/$framework" ]]; then
            log_error "Required framework not found: $framework"
            exit 1
        fi
    done
    
    log_success "System requirements check completed"
}

# Install application bundle
install_app_bundle() {
    log_info "Installing application bundle..."
    
    if [[ -d "PhantomVault.app" ]]; then
        # Remove existing installation
        if [[ -d "$INSTALL_DIR" ]]; then
            rm -rf "$INSTALL_DIR"
        fi
        
        # Create parent directory if needed
        mkdir -p "$(dirname "$INSTALL_DIR")"
        
        # Copy application bundle
        cp -R "PhantomVault.app" "$INSTALL_DIR"
        
        # Set proper permissions
        chmod -R 755 "$INSTALL_DIR"
        
        log_success "Application bundle installed to $INSTALL_DIR"
    else
        log_error "PhantomVault.app bundle not found"
        exit 1
    fi
}

# Create launch daemon/agent
create_launch_service() {
    log_info "Creating launch service..."
    
    # Create service directory if needed
    mkdir -p "$SERVICE_DIR"
    
    # Determine service type and user
    local service_type="Daemon"
    local run_at_load="true"
    local user_name=""
    
    if [[ "$SERVICE_DIR" == *"LaunchAgents"* ]]; then
        service_type="Agent"
        user_name="<key>UserName</key><string>$(whoami)</string>"
    fi
    
    cat > "$SERVICE_DIR/$SERVICE_PLIST" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>dev.phantomvault.service</string>
    
    <key>ProgramArguments</key>
    <array>
        <string>$INSTALL_DIR/Contents/Resources/bin/phantomvault-service</string>
        <string>--daemon</string>
        <string>--log-level</string>
        <string>INFO</string>
    </array>
    
    <key>RunAtLoad</key>
    <$run_at_load/>
    
    <key>KeepAlive</key>
    <dict>
        <key>SuccessfulExit</key>
        <false/>
        <key>Crashed</key>
        <true/>
    </dict>
    
    <key>ProcessType</key>
    <string>Background</string>
    
    <key>StandardOutPath</key>
    <string>/var/log/phantomvault.log</string>
    
    <key>StandardErrorPath</key>
    <string>/var/log/phantomvault.error.log</string>
    
    <key>EnvironmentVariables</key>
    <dict>
        <key>PHANTOMVAULT_DATA_DIR</key>
        <string>\$HOME/.phantomvault</string>
        <key>PHANTOMVAULT_LOG_LEVEL</key>
        <string>INFO</string>
    </dict>
    
    <key>SoftResourceLimits</key>
    <dict>
        <key>NumberOfFiles</key>
        <integer>1024</integer>
        <key>ResidentSetSize</key>
        <integer>52428800</integer>
    </dict>
    
    $user_name
</dict>
</plist>
EOF

    # Set proper permissions
    chmod 644 "$SERVICE_DIR/$SERVICE_PLIST"
    
    # Load the service
    if [[ "$SERVICE_DIR" == *"LaunchDaemons"* ]]; then
        launchctl load "$SERVICE_DIR/$SERVICE_PLIST"
    else
        launchctl load "$SERVICE_DIR/$SERVICE_PLIST"
    fi
    
    log_success "Created and loaded launch service"
}

# Create command line tool
create_cli_tool() {
    log_info "Creating command line tool..."
    
    # Create directory if needed
    mkdir -p "$(dirname "$CLI_TOOL")"
    
    cat > "$CLI_TOOL" << EOF
#!/bin/bash
# PhantomVault command line interface
exec "$INSTALL_DIR/Contents/Resources/bin/phantomvault-service" "\$@"
EOF

    chmod +x "$CLI_TOOL"
    
    # Add to PATH if not already there
    local shell_rc=""
    if [[ "$SHELL" == *"zsh"* ]]; then
        shell_rc="$HOME/.zshrc"
    elif [[ "$SHELL" == *"bash"* ]]; then
        shell_rc="$HOME/.bash_profile"
    fi
    
    if [[ -n "$shell_rc" ]] && [[ -f "$shell_rc" ]]; then
        local bin_dir="$(dirname "$CLI_TOOL")"
        if ! grep -q "$bin_dir" "$shell_rc"; then
            echo "export PATH=\"$bin_dir:\$PATH\"" >> "$shell_rc"
            log_info "Added $bin_dir to PATH in $shell_rc"
        fi
    fi
    
    log_success "Created command line tool: $CLI_TOOL"
}

# Request necessary permissions
request_permissions() {
    log_info "Requesting necessary permissions..."
    
    # Check if we need to request accessibility permissions
    if ! /usr/bin/osascript -e 'tell application "System Events" to get processes' &>/dev/null; then
        log_warning "Accessibility permissions required"
        log_info "Please grant accessibility permissions in System Preferences > Security & Privacy > Privacy > Accessibility"
        
        # Open System Preferences
        open "x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility"
        
        read -p "Press Enter after granting permissions..."
    fi
    
    # Check input monitoring permissions (macOS 10.15+)
    local macos_version=$(sw_vers -productVersion)
    local major_version=$(echo "$macos_version" | cut -d. -f1)
    local minor_version=$(echo "$macos_version" | cut -d. -f2)
    
    if [[ $major_version -gt 10 ]] || [[ $major_version -eq 10 && $minor_version -ge 15 ]]; then
        log_info "Input monitoring permissions may be required"
        log_info "If prompted, please allow PhantomVault to monitor input"
    fi
    
    log_success "Permission requests completed"
}

# Start service
start_service() {
    log_info "Starting PhantomVault service..."
    
    # Start the service
    if launchctl start dev.phantomvault.service; then
        log_success "PhantomVault service started successfully"
        
        # Check if service is running
        sleep 2
        if launchctl list | grep -q "dev.phantomvault.service"; then
            log_success "Service is running and healthy"
        else
            log_warning "Service may not be running properly"
            log_info "Check logs: tail -f /var/log/phantomvault.log"
        fi
    else
        log_error "Failed to start PhantomVault service"
        exit 1
    fi
}

# Main installation function
main() {
    echo "=================================================="
    echo "PhantomVault macOS Installation Script"
    echo "Invisible Folder Security with Profile Management"
    echo "=================================================="
    echo
    
    check_permissions
    check_requirements
    install_app_bundle
    create_launch_service
    create_cli_tool
    request_permissions
    start_service
    
    echo
    log_success "PhantomVault installation completed successfully!"
    echo
    echo "Next steps:"
    echo "1. Launch PhantomVault from Applications folder"
    echo "2. Or use the command line: phantomvault --help"
    echo "3. Check service status: launchctl list | grep phantomvault"
    echo "4. View logs: tail -f /var/log/phantomvault.log"
    echo
    echo "Note: You may need to grant additional permissions when first running PhantomVault"
    echo "For support, visit: https://github.com/ishaq2321/phantomVault"
}

# Run main function
main "$@"