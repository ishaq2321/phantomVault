#!/bin/bash
# PhantomVault macOS Installer Builder
# Creates DMG installer with proper code signing and LaunchDaemon

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
INSTALLER_DIR="$PROJECT_ROOT/installer"
BUILD_DIR="$PROJECT_ROOT/build"
PACKAGE_DIR="$BUILD_DIR/packages/macos"
VERSION="1.0.0"
APP_NAME="PhantomVault"
BUNDLE_ID="com.phantomvault.app"
DEVELOPER_ID="${DEVELOPER_ID:-}"  # Set via environment variable

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() { echo -e "${BLUE}[MACOS]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check macOS environment
check_macos_env() {
    print_status "Checking macOS build environment..."
    
    if [[ "$(uname -s)" != "Darwin" ]]; then
        print_error "macOS installer must be built on macOS"
        exit 1
    fi
    
    # Check for required tools
    local missing_tools=()
    
    if ! command -v pkgbuild &> /dev/null; then
        missing_tools+=("pkgbuild")
    fi
    
    if ! command -v productbuild &> /dev/null; then
        missing_tools+=("productbuild")
    fi
    
    if ! command -v hdiutil &> /dev/null; then
        missing_tools+=("hdiutil")
    fi
    
    if [[ ${#missing_tools[@]} -gt 0 ]]; then
        print_error "Missing required tools: ${missing_tools[*]}"
        print_error "Install Xcode Command Line Tools: xcode-select --install"
        exit 1
    fi
    
    # Check for code signing certificate
    if [[ -n "$DEVELOPER_ID" ]]; then
        if ! security find-identity -v -p codesigning | grep -q "$DEVELOPER_ID"; then
            print_warning "Developer ID certificate not found: $DEVELOPER_ID"
            print_warning "Code signing will be skipped"
            DEVELOPER_ID=""
        else
            print_status "Found code signing certificate: $DEVELOPER_ID"
        fi
    else
        print_warning "No DEVELOPER_ID set - code signing will be skipped"
        print_warning "Set DEVELOPER_ID environment variable for production builds"
    fi
}

# Prepare macOS build
prepare_macos_build() {
    print_status "Preparing macOS build..."
    
    # Clean and create directories
    rm -rf "$PACKAGE_DIR"
    mkdir -p "$PACKAGE_DIR"/{staging,app,pkg,dmg}
    
    # Check if macOS binary exists
    local macos_binary="$BUILD_DIR/phantomvault"
    if [[ ! -f "$macos_binary" ]]; then
        print_status "Building PhantomVault for macOS..."
        cd "$PROJECT_ROOT"
        ./build.sh
        
        if [[ ! -f "$BUILD_DIR/phantomvault" ]]; then
            print_error "Failed to build macOS binary"
            exit 1
        fi
    fi
    
    # Copy binary to staging
    cp "$BUILD_DIR/phantomvault" "$PACKAGE_DIR/staging/phantomvault-service"
    chmod +x "$PACKAGE_DIR/staging/phantomvault-service"
    
    print_success "macOS build prepared"
}

# Create macOS application bundle
create_app_bundle() {
    print_status "Creating application bundle..."
    
    local app_bundle="$PACKAGE_DIR/app/$APP_NAME.app"
    local contents_dir="$app_bundle/Contents"
    local macos_dir="$contents_dir/MacOS"
    local resources_dir="$contents_dir/Resources"
    
    # Create bundle structure
    mkdir -p "$macos_dir" "$resources_dir"
    
    # Copy executable
    cp "$PACKAGE_DIR/staging/phantomvault-service" "$macos_dir/"
    
    # Create GUI wrapper script
    cat > "$macos_dir/phantomvault-gui" << 'EOF'
#!/bin/bash
# PhantomVault GUI Launcher for macOS

BUNDLE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SERVICE_BIN="$BUNDLE_DIR/MacOS/phantomvault-service"
PLIST_PATH="/Library/LaunchDaemons/com.phantomvault.service.plist"

# Function to check if service is running
is_service_running() {
    launchctl list | grep -q "com.phantomvault.service"
}

# Function to start service
start_service() {
    if [[ -f "$PLIST_PATH" ]]; then
        sudo launchctl load "$PLIST_PATH" 2>/dev/null || true
    else
        echo "Service not installed. Please run the installer."
        return 1
    fi
}

# Check if service is running
if ! is_service_running; then
    echo "Starting PhantomVault service..."
    if ! start_service; then
        echo "Failed to start service automatically."
        echo "Please install PhantomVault properly or run with administrator privileges."
        exit 1
    fi
    sleep 2
fi

# Show status in terminal
osascript << 'APPLESCRIPT'
tell application "Terminal"
    activate
    do script "
echo 'PhantomVault - Invisible Folder Security'
echo '========================================'

if pgrep -f phantomvault-service > /dev/null; then
    echo 'Status: ✅ Running'
    echo 'Your folders are protected'
    echo ''
    echo 'Usage:'
    echo '• Press Ctrl+Alt+V anywhere to access your folders'
    echo '• Use phantomvault --help for more options'
else
    echo 'Status: ❌ Not running'
    echo 'Please check the installation'
fi

echo ''
echo 'Press any key to continue...'
read -n 1
exit
"
end tell
APPLESCRIPT
EOF
    chmod +x "$macos_dir/phantomvault-gui"
    
    # Create Info.plist
    cat > "$contents_dir/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>phantomvault-gui</string>
    <key>CFBundleIdentifier</key>
    <string>$BUNDLE_ID</string>
    <key>CFBundleName</key>
    <string>$APP_NAME</string>
    <key>CFBundleDisplayName</key>
    <string>PhantomVault</string>
    <key>CFBundleVersion</key>
    <string>$VERSION</string>
    <key>CFBundleShortVersionString</key>
    <string>$VERSION</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleSignature</key>
    <string>PVLT</string>
    <key>CFBundleIconFile</key>
    <string>phantomvault.icns</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.12</string>
    <key>LSApplicationCategoryType</key>
    <string>public.app-category.security</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSRequiresAquaSystemAppearance</key>
    <false/>
</dict>
</plist>
EOF
    
    # Create minimal application icon
    # Note: For production builds, replace with custom branded icon
    # Create minimal ICNS file structure
    echo "icns" > "$resources_dir/phantomvault.icns"
    
    print_success "Application bundle created"
}

# Create LaunchDaemon plist
create_launch_daemon() {
    print_status "Creating LaunchDaemon configuration..."
    
    local plist_file="$PACKAGE_DIR/staging/com.phantomvault.service.plist"
    
    cat > "$plist_file" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>com.phantomvault.service</string>
    
    <key>ProgramArguments</key>
    <array>
        <string>/usr/local/bin/phantomvault-service</string>
        <string>--daemon</string>
        <string>--log-level</string>
        <string>INFO</string>
        <string>--port</string>
        <string>9876</string>
    </array>
    
    <key>RunAtLoad</key>
    <true/>
    
    <key>KeepAlive</key>
    <dict>
        <key>SuccessfulExit</key>
        <false/>
    </dict>
    
    <key>StandardOutPath</key>
    <string>/usr/local/var/log/phantomvault.log</string>
    
    <key>StandardErrorPath</key>
    <string>/usr/local/var/log/phantomvault-error.log</string>
    
    <key>WorkingDirectory</key>
    <string>/usr/local/var/phantomvault</string>
    
    <key>UserName</key>
    <string>root</string>
    
    <key>GroupName</key>
    <string>wheel</string>
    
    <key>ThrottleInterval</key>
    <integer>10</integer>
    
    <key>ProcessType</key>
    <string>Background</string>
    
    <key>LowPriorityIO</key>
    <true/>
    
    <key>Nice</key>
    <integer>1</integer>
</dict>
</plist>
EOF
    
    print_success "LaunchDaemon configuration created"
}

# Create CLI tool
create_cli_tool() {
    print_status "Creating CLI tool..."
    
    cat > "$PACKAGE_DIR/staging/phantomvault" << 'EOF'
#!/bin/bash
# PhantomVault CLI for macOS

SERVICE_BIN="/usr/local/bin/phantomvault-service"
SERVICE_LABEL="com.phantomvault.service"

# Function to check if service is running
is_service_running() {
    launchctl list | grep -q "$SERVICE_LABEL"
}

# Handle commands
case "${1:-status}" in
    status)
        echo "PhantomVault - Invisible Folder Security"
        echo "========================================"
        if is_service_running; then
            echo "Status: ✅ Running"
            local pid=$(launchctl list | grep "$SERVICE_LABEL" | awk '{print $1}')
            if [[ "$pid" != "-" ]]; then
                local memory=$(ps -o rss= -p "$pid" 2>/dev/null | awk '{print $1/1024 " MB"}')
                echo "Memory: ${memory:-"Unknown"}"
            fi
        else
            echo "Status: ❌ Not running"
        fi
        ;;
    --gui)
        open -a "PhantomVault"
        ;;
    --start)
        echo "Starting PhantomVault service..."
        sudo launchctl load /Library/LaunchDaemons/com.phantomvault.service.plist
        ;;
    --stop)
        echo "Stopping PhantomVault service..."
        sudo launchctl unload /Library/LaunchDaemons/com.phantomvault.service.plist
        ;;
    --restart)
        echo "Restarting PhantomVault service..."
        sudo launchctl unload /Library/LaunchDaemons/com.phantomvault.service.plist
        sleep 1
        sudo launchctl load /Library/LaunchDaemons/com.phantomvault.service.plist
        ;;
    --status)
        launchctl list | grep "$SERVICE_LABEL" || echo "Service not running"
        ;;
    --logs)
        echo "PhantomVault Service Logs:"
        echo "========================="
        tail -f /usr/local/var/log/phantomvault.log
        ;;
    --help|-h)
        echo "PhantomVault CLI for macOS"
        echo "Usage: phantomvault [OPTION]"
        echo ""
        echo "Options:"
        echo "  status        Show service status (default)"
        echo "  --gui         Open GUI application"
        echo "  --start       Start service"
        echo "  --stop        Stop service"
        echo "  --restart     Restart service"
        echo "  --status      Detailed service status"
        echo "  --logs        Show service logs"
        echo "  --help        Show this help"
        ;;
    *)
        # Pass other arguments to service
        exec "$SERVICE_BIN" "$@"
        ;;
esac
EOF
    chmod +x "$PACKAGE_DIR/staging/phantomvault"
    
    print_success "CLI tool created"
}

# Create installer scripts
create_installer_scripts() {
    print_status "Creating installer scripts..."
    
    # Create preinstall script
    cat > "$PACKAGE_DIR/staging/preinstall" << 'EOF'
#!/bin/bash
# PhantomVault preinstall script

# Stop service if running
if launchctl list | grep -q "com.phantomvault.service"; then
    launchctl unload /Library/LaunchDaemons/com.phantomvault.service.plist 2>/dev/null || true
fi

# Create necessary directories
mkdir -p /usr/local/bin
mkdir -p /usr/local/var/{phantomvault,log}
mkdir -p /Library/LaunchDaemons

exit 0
EOF
    chmod +x "$PACKAGE_DIR/staging/preinstall"
    
    # Create postinstall script
    cat > "$PACKAGE_DIR/staging/postinstall" << 'EOF'
#!/bin/bash
# PhantomVault postinstall script

# Set proper permissions
chown root:wheel /usr/local/bin/phantomvault-service
chmod 755 /usr/local/bin/phantomvault-service

chown root:wheel /usr/local/bin/phantomvault
chmod 755 /usr/local/bin/phantomvault

chown root:wheel /Library/LaunchDaemons/com.phantomvault.service.plist
chmod 644 /Library/LaunchDaemons/com.phantomvault.service.plist

# Set up data directories
chown -R root:wheel /usr/local/var/phantomvault
chmod -R 755 /usr/local/var/phantomvault

# Load and start service
launchctl load /Library/LaunchDaemons/com.phantomvault.service.plist

echo "PhantomVault installed successfully!"
echo "The service will start automatically on boot."
echo "Use 'phantomvault --help' for CLI options or open PhantomVault.app"

exit 0
EOF
    chmod +x "$PACKAGE_DIR/staging/postinstall"
    
    print_success "Installer scripts created"
}

# Code sign files
code_sign_files() {
    if [[ -z "$DEVELOPER_ID" ]]; then
        print_warning "Skipping code signing - no DEVELOPER_ID set"
        return
    fi
    
    print_status "Code signing files..."
    
    # Sign the service binary
    codesign --force --sign "$DEVELOPER_ID" \
        --options runtime \
        --timestamp \
        "$PACKAGE_DIR/staging/phantomvault-service"
    
    # Sign the app bundle
    codesign --force --sign "$DEVELOPER_ID" \
        --options runtime \
        --timestamp \
        --deep \
        "$PACKAGE_DIR/app/$APP_NAME.app"
    
    print_success "Files code signed"
}

# Create PKG installer
create_pkg_installer() {
    print_status "Creating PKG installer..."
    
    local component_pkg="$PACKAGE_DIR/pkg/PhantomVault-Service.pkg"
    local product_pkg="$PACKAGE_DIR/pkg/PhantomVault-$VERSION.pkg"
    
    # Create component package
    pkgbuild --root "$PACKAGE_DIR/staging" \
        --identifier "$BUNDLE_ID.service" \
        --version "$VERSION" \
        --scripts "$PACKAGE_DIR/staging" \
        --install-location "/" \
        "$component_pkg"
    
    # Create distribution XML
    cat > "$PACKAGE_DIR/pkg/distribution.xml" << EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>PhantomVault $VERSION</title>
    <organization>com.phantomvault</organization>
    <domains enable_localSystem="true"/>
    <options customize="never" require-scripts="true" rootVolumeOnly="true"/>
    
    <welcome file="welcome.html"/>
    <license file="license.html"/>
    <conclusion file="conclusion.html"/>
    
    <pkg-ref id="$BUNDLE_ID.service"/>
    
    <choices-outline>
        <line choice="default">
            <line choice="$BUNDLE_ID.service"/>
        </line>
    </choices-outline>
    
    <choice id="default"/>
    <choice id="$BUNDLE_ID.service" visible="false">
        <pkg-ref id="$BUNDLE_ID.service"/>
    </choice>
    
    <pkg-ref id="$BUNDLE_ID.service" version="$VERSION" onConclusion="none">$component_pkg</pkg-ref>
</installer-gui-script>
EOF
    
    # Create welcome HTML
    cat > "$PACKAGE_DIR/pkg/welcome.html" << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Welcome to PhantomVault</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, sans-serif; margin: 20px; }
        h1 { color: #2563eb; }
    </style>
</head>
<body>
    <h1>Welcome to PhantomVault</h1>
    <p>This installer will install PhantomVault - Invisible Folder Security with Profile-Based Management.</p>
    <p><strong>Features:</strong></p>
    <ul>
        <li>Military-grade folder security</li>
        <li>Invisible folder management</li>
        <li>Profile-based access control</li>
        <li>Real-time integrity monitoring</li>
        <li>Cross-platform compatibility</li>
    </ul>
    <p>Click Continue to proceed with the installation.</p>
</body>
</html>
EOF
    
    # Create license HTML
    cat > "$PACKAGE_DIR/pkg/license.html" << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>License Agreement</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, sans-serif; margin: 20px; }
        pre { background: #f5f5f5; padding: 15px; border-radius: 5px; overflow: auto; }
    </style>
</head>
<body>
    <h1>License Agreement</h1>
    <pre>
MIT License

Copyright (c) 2024 PhantomVault Team

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
    </pre>
</body>
</html>
EOF
    
    # Create conclusion HTML
    cat > "$PACKAGE_DIR/pkg/conclusion.html" << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>Installation Complete</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, sans-serif; margin: 20px; }
        h1 { color: #16a34a; }
        .command { background: #f5f5f5; padding: 10px; border-radius: 5px; font-family: monospace; }
    </style>
</head>
<body>
    <h1>Installation Complete</h1>
    <p>PhantomVault has been successfully installed!</p>
    
    <h2>Getting Started</h2>
    <p><strong>GUI Application:</strong> Open PhantomVault from Applications or Launchpad</p>
    <p><strong>Command Line:</strong> Use the <code>phantomvault</code> command in Terminal</p>
    
    <h2>Quick Commands</h2>
    <div class="command">
        phantomvault status      # Check service status<br>
        phantomvault --gui       # Open GUI application<br>
        phantomvault --help      # Show all options
    </div>
    
    <p>The PhantomVault service is now running and will start automatically on boot.</p>
    <p>Press <strong>Ctrl+Alt+V</strong> anywhere to access your protected folders!</p>
</body>
</html>
EOF
    
    # Build product package
    productbuild --distribution "$PACKAGE_DIR/pkg/distribution.xml" \
        --package-path "$PACKAGE_DIR/pkg" \
        --resources "$PACKAGE_DIR/pkg" \
        "$product_pkg"
    
    # Sign the package if developer ID is available
    if [[ -n "$DEVELOPER_ID" ]]; then
        print_status "Signing PKG installer..."
        productsign --sign "$DEVELOPER_ID" "$product_pkg" "$product_pkg.signed"
        mv "$product_pkg.signed" "$product_pkg"
    fi
    
    print_success "PKG installer created: $product_pkg"
}

# Create DMG disk image
create_dmg_installer() {
    print_status "Creating DMG installer..."
    
    local dmg_staging="$PACKAGE_DIR/dmg/staging"
    local dmg_file="$PACKAGE_DIR/dmg/PhantomVault-$VERSION.dmg"
    
    # Create staging directory
    mkdir -p "$dmg_staging"
    
    # Copy app bundle
    cp -R "$PACKAGE_DIR/app/$APP_NAME.app" "$dmg_staging/"
    
    # Copy PKG installer
    cp "$PACKAGE_DIR/pkg/PhantomVault-$VERSION.pkg" "$dmg_staging/Install PhantomVault.pkg"
    
    # Create Applications symlink
    ln -s /Applications "$dmg_staging/Applications"
    
    # Create README
    cat > "$dmg_staging/README.txt" << 'EOF'
PhantomVault - Invisible Folder Security
========================================

Installation Options:

1. RECOMMENDED: Double-click "Install PhantomVault.pkg" for full system installation
   - Installs service that starts automatically on boot
   - Adds command line tools
   - Proper system integration

2. PORTABLE: Drag PhantomVault.app to Applications folder
   - Manual installation
   - Service must be started manually
   - Limited functionality

For support: https://github.com/ishaq2321/phantomVault

Usage:
• Press Ctrl+Alt+V anywhere to access your folders
• Use 'phantomvault --help' for command line options
• Open PhantomVault.app for GUI interface
EOF
    
    # Create temporary DMG
    local temp_dmg="$PACKAGE_DIR/dmg/temp.dmg"
    hdiutil create -srcfolder "$dmg_staging" \
        -volname "PhantomVault $VERSION" \
        -fs HFS+ \
        -fsargs "-c c=64,a=16,e=16" \
        -format UDRW \
        "$temp_dmg"
    
    # Mount and customize DMG
    local mount_point="/Volumes/PhantomVault $VERSION"
    hdiutil attach "$temp_dmg" -readwrite -noverify -noautoopen
    
    # Set custom icon and background (if available)
    if [[ -f "$INSTALLER_DIR/assets/dmg-background.png" ]]; then
        cp "$INSTALLER_DIR/assets/dmg-background.png" "$mount_point/.background.png"
    fi
    
    # Set Finder view options using AppleScript
    osascript << EOF
tell application "Finder"
    tell disk "PhantomVault $VERSION"
        open
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false
        set the bounds of container window to {400, 100, 900, 400}
        set viewOptions to the icon view options of container window
        set arrangement of viewOptions to not arranged
        set icon size of viewOptions to 72
        set background picture of viewOptions to file ".background.png"
        set position of item "PhantomVault.app" of container window to {150, 200}
        set position of item "Applications" of container window to {350, 200}
        set position of item "Install PhantomVault.pkg" of container window to {250, 100}
        set position of item "README.txt" of container window to {250, 300}
        close
        open
        update without registering applications
        delay 2
    end tell
end tell
EOF
    
    # Unmount and convert to read-only
    hdiutil detach "$mount_point"
    hdiutil convert "$temp_dmg" -format UDZO -imagekey zlib-level=9 -o "$dmg_file"
    rm "$temp_dmg"
    
    print_success "DMG installer created: $dmg_file"
}

# Main function
main() {
    print_status "Building PhantomVault macOS installer..."
    
    check_macos_env
    prepare_macos_build
    create_app_bundle
    create_launch_daemon
    create_cli_tool
    create_installer_scripts
    code_sign_files
    create_pkg_installer
    create_dmg_installer
    
    print_success "macOS installer created successfully!"
    print_status "Installers available in: $PACKAGE_DIR"
    
    # List created installers
    echo ""
    echo "Created installers:"
    find "$PACKAGE_DIR" -name "*.pkg" -o -name "*.dmg" | while read -r installer; do
        echo "  • $(basename "$installer")"
    done
    
    echo ""
    echo "Installation methods:"
    echo "1. DMG: Mount and run installer package"
    echo "2. PKG: Direct installer package"
    echo "3. App Bundle: Drag to Applications (manual setup required)"
}

# Run main function
main "$@"