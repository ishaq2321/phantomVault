#!/bin/bash

# PhantomVault macOS Installer Builder
# Creates DMG installer with complete uninstaller

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() { echo -e "${BLUE}[INSTALLER]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Configuration
PACKAGE_NAME="PhantomVault"
VERSION="1.0.0"
BUNDLE_ID="dev.phantomvault.app"
DEVELOPER_ID="PhantomVault Team"
DESCRIPTION="Invisible Folder Security with Profile-Based Management"

# Directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/installer/build"
MACOS_DIR="$BUILD_DIR/macos"

print_status "Building macOS DMG installer..."
print_status "Project root: $PROJECT_ROOT"
print_status "Build directory: $BUILD_DIR"

# Clean and create build directories
rm -rf "$MACOS_DIR"
mkdir -p "$MACOS_DIR"/{app,dmg,pkg}

# Check if running on macOS
check_macos_tools() {
    if [[ "$OSTYPE" != "darwin"* ]]; then
        print_warning "Not running on macOS - creating installer template only"
        return 1
    fi
    
    if ! command -v hdiutil &> /dev/null; then
        print_error "hdiutil not found (required for DMG creation)"
        return 1
    fi
    
    if ! command -v pkgbuild &> /dev/null; then
        print_error "pkgbuild not found (required for PKG creation)"
        return 1
    fi
    
    return 0
}

# Create macOS app bundle
create_app_bundle() {
    print_status "Creating macOS app bundle..."
    
    local app_dir="$MACOS_DIR/app/PhantomVault.app"
    mkdir -p "$app_dir/Contents"/{MacOS,Resources,Frameworks}
    
    # Create Info.plist
    cat > "$app_dir/Contents/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>phantomvault</string>
    <key>CFBundleIdentifier</key>
    <string>$BUNDLE_ID</string>
    <key>CFBundleName</key>
    <string>$PACKAGE_NAME</string>
    <key>CFBundleDisplayName</key>
    <string>$PACKAGE_NAME</string>
    <key>CFBundleVersion</key>
    <string>$VERSION</string>
    <key>CFBundleShortVersionString</key>
    <string>$VERSION</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleSignature</key>
    <string>PVLT</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.14</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSSupportsAutomaticGraphicsSwitching</key>
    <true/>
    <key>LSUIElement</key>
    <false/>
    <key>CFBundleURLTypes</key>
    <array>
        <dict>
            <key>CFBundleURLName</key>
            <string>PhantomVault Protocol</string>
            <key>CFBundleURLSchemes</key>
            <array>
                <string>phantomvault</string>
            </array>
        </dict>
    </array>
    <key>NSAppleEventsUsageDescription</key>
    <string>PhantomVault needs access to control other applications for security operations.</string>
    <key>NSSystemAdministrationUsageDescription</key>
    <string>PhantomVault requires administrator privileges for folder security operations.</string>
</dict>
</plist>
EOF
    
    # Copy executable
    if [ -f "$PROJECT_ROOT/bin/phantomvault" ]; then
        cp "$PROJECT_ROOT/bin/phantomvault" "$app_dir/Contents/MacOS/"
        chmod +x "$app_dir/Contents/MacOS/phantomvault"
    elif [ -f "$PROJECT_ROOT/core/build/bin/phantomvault-service" ]; then
        cp "$PROJECT_ROOT/core/build/bin/phantomvault-service" "$app_dir/Contents/MacOS/phantomvault"
        chmod +x "$app_dir/Contents/MacOS/phantomvault"
    else
        print_warning "No macOS executable found, creating placeholder"
        echo '#!/bin/bash\necho "PhantomVault placeholder executable"' > "$app_dir/Contents/MacOS/phantomvault"
        chmod +x "$app_dir/Contents/MacOS/phantomvault"
    fi
    
    # Create app icon (placeholder)
    if [ -f "$PROJECT_ROOT/gui/assets/icon.icns" ]; then
        cp "$PROJECT_ROOT/gui/assets/icon.icns" "$app_dir/Contents/Resources/"
    else
        print_warning "App icon not found, using placeholder"
        # Create a simple icon placeholder
        mkdir -p "$app_dir/Contents/Resources/PhantomVault.iconset"
        # In real implementation, would create proper icon files
        touch "$app_dir/Contents/Resources/PhantomVault.icns"
    fi
    
    print_success "App bundle created: $app_dir"
}

# Create PKG installer
create_pkg_installer() {
    print_status "Creating PKG installer..."
    
    local pkg_root="$MACOS_DIR/pkg/root"
    local scripts_dir="$MACOS_DIR/pkg/scripts"
    mkdir -p "$pkg_root/Applications" "$scripts_dir"
    
    # Copy app bundle to pkg root
    cp -R "$MACOS_DIR/app/PhantomVault.app" "$pkg_root/Applications/"
    
    # Create preinstall script
    cat > "$scripts_dir/preinstall" << 'EOF'
#!/bin/bash

# Stop any running PhantomVault processes
pkill -f phantomvault || true

# Remove old installation if it exists
if [ -d "/Applications/PhantomVault.app" ]; then
    rm -rf "/Applications/PhantomVault.app"
fi

exit 0
EOF
    
    # Create postinstall script
    cat > "$scripts_dir/postinstall" << 'EOF'
#!/bin/bash

# Set proper permissions
chown -R root:admin "/Applications/PhantomVault.app"
chmod -R 755 "/Applications/PhantomVault.app"

# Create LaunchDaemon for service
cat > "/Library/LaunchDaemons/dev.phantomvault.service.plist" << 'PLIST_EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>dev.phantomvault.service</string>
    <key>ProgramArguments</key>
    <array>
        <string>/Applications/PhantomVault.app/Contents/MacOS/phantomvault</string>
        <string>--service</string>
        <string>--daemon</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <true/>
    <key>StandardOutPath</key>
    <string>/var/log/phantomvault.log</string>
    <key>StandardErrorPath</key>
    <string>/var/log/phantomvault.error.log</string>
</dict>
</plist>
PLIST_EOF

# Load the service
launchctl load "/Library/LaunchDaemons/dev.phantomvault.service.plist"

# Create uninstaller
cat > "/Applications/PhantomVault.app/Contents/MacOS/uninstall.sh" << 'UNINSTALL_EOF'
#!/bin/bash

# PhantomVault macOS Uninstaller

echo "PhantomVault Uninstaller"
echo "This will completely remove PhantomVault from your system."
echo "User data in ~/.phantomvault/ will be preserved."
read -p "Continue? (y/N): " -n 1 -r
echo

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Uninstall cancelled."
    exit 0
fi

echo "Stopping PhantomVault service..."
launchctl unload "/Library/LaunchDaemons/dev.phantomvault.service.plist" 2>/dev/null || true

echo "Removing PhantomVault..."
rm -f "/Library/LaunchDaemons/dev.phantomvault.service.plist"
rm -rf "/Applications/PhantomVault.app"

echo "PhantomVault has been completely removed."
echo "User data preserved in ~/.phantomvault/"
UNINSTALL_EOF

chmod +x "/Applications/PhantomVault.app/Contents/MacOS/uninstall.sh"

echo "PhantomVault installed successfully!"
echo "Launch from Applications folder or use: open -a PhantomVault"

exit 0
EOF
    
    # Make scripts executable
    chmod +x "$scripts_dir"/{preinstall,postinstall}
    
    # Build PKG
    if check_macos_tools; then
        pkgbuild --root "$pkg_root" \
                 --scripts "$scripts_dir" \
                 --identifier "$BUNDLE_ID" \
                 --version "$VERSION" \
                 --install-location "/" \
                 "$MACOS_DIR/PhantomVault-$VERSION.pkg"
        
        if [ $? -eq 0 ]; then
            print_success "PKG installer created: $MACOS_DIR/PhantomVault-$VERSION.pkg"
        else
            print_error "Failed to create PKG installer"
            return 1
        fi
    else
        print_warning "Skipping PKG creation (not on macOS)"
    fi
}

# Create DMG installer
create_dmg_installer() {
    print_status "Creating DMG installer..."
    
    local dmg_source="$MACOS_DIR/dmg/source"
    local dmg_temp="$MACOS_DIR/dmg/temp"
    mkdir -p "$dmg_source" "$dmg_temp"
    
    # Copy app bundle to DMG source
    cp -R "$MACOS_DIR/app/PhantomVault.app" "$dmg_source/"
    
    # Create Applications symlink
    ln -sf "/Applications" "$dmg_source/Applications"
    
    # Create README file
    cat > "$dmg_source/README.txt" << EOF
PhantomVault v$VERSION
Invisible Folder Security with Profile-Based Management

INSTALLATION:
1. Drag PhantomVault.app to the Applications folder
2. Launch PhantomVault from Applications
3. Grant necessary permissions when prompted

UNINSTALLATION:
Run: /Applications/PhantomVault.app/Contents/MacOS/uninstall.sh

SUPPORT:
Visit: https://github.com/ishaq2321/phantomVault
Email: team@phantomvault.dev

Copyright © 2025 PhantomVault Team
EOF
    
    # Create DMG background image (placeholder)
    if [ -f "$PROJECT_ROOT/gui/assets/dmg-background.png" ]; then
        cp "$PROJECT_ROOT/gui/assets/dmg-background.png" "$dmg_source/.background.png"
    fi
    
    # Create DS_Store for DMG layout (if on macOS)
    if check_macos_tools; then
        # Create temporary DMG
        hdiutil create -srcfolder "$dmg_source" \
                       -volname "PhantomVault $VERSION" \
                       -fs HFS+ \
                       -fsargs "-c c=64,a=16,e=16" \
                       -format UDRW \
                       -size 100m \
                       "$dmg_temp/temp.dmg"
        
        # Mount temporary DMG
        local mount_point="/Volumes/PhantomVault $VERSION"
        hdiutil attach "$dmg_temp/temp.dmg" -mountpoint "$mount_point"
        
        # Set DMG window properties using AppleScript
        osascript << EOF
tell application "Finder"
    tell disk "PhantomVault $VERSION"
        open
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false
        set the bounds of container window to {400, 100, 920, 420}
        set viewOptions to the icon view options of container window
        set arrangement of viewOptions to not arranged
        set icon size of viewOptions to 72
        set background picture of viewOptions to file ".background.png"
        set position of item "PhantomVault.app" of container window to {130, 220}
        set position of item "Applications" of container window to {410, 220}
        close
        open
        update without registering applications
        delay 2
    end tell
end tell
EOF
        
        # Unmount temporary DMG
        hdiutil detach "$mount_point"
        
        # Convert to compressed DMG
        hdiutil convert "$dmg_temp/temp.dmg" \
                        -format UDZO \
                        -imagekey zlib-level=9 \
                        -o "$MACOS_DIR/PhantomVault-$VERSION.dmg"
        
        # Clean up
        rm "$dmg_temp/temp.dmg"
        
        if [ -f "$MACOS_DIR/PhantomVault-$VERSION.dmg" ]; then
            print_success "DMG installer created: $MACOS_DIR/PhantomVault-$VERSION.dmg"
            
            # Copy to main build directory
            cp "$MACOS_DIR/PhantomVault-$VERSION.dmg" "$BUILD_DIR/"
        else
            print_error "Failed to create DMG installer"
            return 1
        fi
    else
        print_warning "Skipping DMG creation (not on macOS)"
        
        # Create a simple tar.gz archive instead
        cd "$dmg_source"
        tar -czf "$BUILD_DIR/PhantomVault-$VERSION-macos.tar.gz" *
        cd - > /dev/null
        print_success "macOS archive created: $BUILD_DIR/PhantomVault-$VERSION-macos.tar.gz"
    fi
}

# Create standalone installer script
create_standalone_installer() {
    print_status "Creating standalone macOS installer script..."
    
    cat > "$BUILD_DIR/phantomvault-macos-installer.sh" << 'EOF'
#!/bin/bash

# PhantomVault macOS Standalone Installer
# Self-extracting installer with complete uninstaller

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() { echo -e "${BLUE}[INSTALLER]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check if running on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    print_error "This installer is for macOS only"
    exit 1
fi

print_status "PhantomVault macOS Installer v1.0.0"
print_status "Installing invisible folder security system..."

# Check for admin privileges
if [ "$EUID" -ne 0 ]; then
    print_error "This installer must be run with sudo"
    exit 1
fi

# Create application directory
APP_DIR="/Applications/PhantomVault.app"
mkdir -p "$APP_DIR/Contents"/{MacOS,Resources}

print_status "Installing PhantomVault application..."

# Copy executable (placeholder - would be embedded in real installer)
if [ -f "./phantomvault" ]; then
    cp "./phantomvault" "$APP_DIR/Contents/MacOS/"
    chmod +x "$APP_DIR/Contents/MacOS/phantomvault"
else
    print_error "PhantomVault executable not found in installer"
    exit 1
fi

# Create Info.plist
cat > "$APP_DIR/Contents/Info.plist" << 'PLIST_EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>phantomvault</string>
    <key>CFBundleIdentifier</key>
    <string>dev.phantomvault.app</string>
    <key>CFBundleName</key>
    <string>PhantomVault</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
</dict>
</plist>
PLIST_EOF

# Set proper permissions
chown -R root:admin "$APP_DIR"
chmod -R 755 "$APP_DIR"

# Create LaunchDaemon for service
cat > "/Library/LaunchDaemons/dev.phantomvault.service.plist" << 'SERVICE_EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>Label</key>
    <string>dev.phantomvault.service</string>
    <key>ProgramArguments</key>
    <array>
        <string>/Applications/PhantomVault.app/Contents/MacOS/phantomvault</string>
        <string>--service</string>
        <string>--daemon</string>
    </array>
    <key>RunAtLoad</key>
    <true/>
    <key>KeepAlive</key>
    <true/>
</dict>
</plist>
SERVICE_EOF

# Load the service
launchctl load "/Library/LaunchDaemons/dev.phantomvault.service.plist"

# Create uninstaller
cat > "$APP_DIR/Contents/MacOS/uninstall.sh" << 'UNINSTALL_EOF'
#!/bin/bash

echo "PhantomVault macOS Uninstaller"
echo "This will completely remove PhantomVault from your system."
echo "User data in ~/.phantomvault/ will be preserved."
read -p "Continue? (y/N): " -n 1 -r
echo

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Uninstall cancelled."
    exit 0
fi

echo "Stopping PhantomVault service..."
sudo launchctl unload "/Library/LaunchDaemons/dev.phantomvault.service.plist" 2>/dev/null || true

echo "Removing PhantomVault..."
sudo rm -f "/Library/LaunchDaemons/dev.phantomvault.service.plist"
sudo rm -rf "/Applications/PhantomVault.app"

echo "PhantomVault has been completely removed."
echo "User data preserved in ~/.phantomvault/"
UNINSTALL_EOF

chmod +x "$APP_DIR/Contents/MacOS/uninstall.sh"

print_success "PhantomVault installed successfully!"
print_status "Launch: open -a PhantomVault"
print_status "Service: Automatically started"
print_status "Uninstall: $APP_DIR/Contents/MacOS/uninstall.sh"

EOF
    
    chmod +x "$BUILD_DIR/phantomvault-macos-installer.sh"
    print_success "Standalone macOS installer created: $BUILD_DIR/phantomvault-macos-installer.sh"
}

# Main execution
main() {
    print_status "Starting macOS installer build process..."
    
    create_app_bundle
    create_pkg_installer
    create_dmg_installer
    create_standalone_installer
    
    print_success "macOS installer packages created successfully!"
    print_status "Available packages:"
    find "$BUILD_DIR" -name "*.dmg" -o -name "*.pkg" -o -name "*macos*installer*" -o -name "*macos*.tar.gz" | while read file; do
        print_status "  - $(basename "$file")"
    done
}

# Run main function
main "$@"