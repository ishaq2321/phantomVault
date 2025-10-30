#!/bin/bash
# PhantomVault Master Installer Builder
# Builds installers for all supported platforms

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
INSTALLER_DIR="$PROJECT_ROOT/installer"
BUILD_DIR="$PROJECT_ROOT/build"
PACKAGE_DIR="$BUILD_DIR/packages"
VERSION="1.0.0"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

print_header() { echo -e "${CYAN}[MASTER]${NC} $1"; }
print_status() { echo -e "${BLUE}[BUILD]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Print banner
print_banner() {
    echo ""
    echo "=================================================="
    echo "PhantomVault Professional Installer Builder"
    echo "Building installers for all supported platforms"
    echo "=================================================="
    echo ""
}

# Check build environment
check_environment() {
    print_status "Checking build environment..."
    
    # Check if we're in the right directory
    if [[ ! -f "$PROJECT_ROOT/README.md" ]] || [[ ! -d "$PROJECT_ROOT/core" ]]; then
        print_error "Please run this script from the PhantomVault root directory"
        exit 1
    fi
    
    # Check if main application is built
    if [[ ! -f "$BUILD_DIR/phantomvault" ]]; then
        print_status "Building PhantomVault application..."
        cd "$PROJECT_ROOT"
        ./build.sh
        
        if [[ ! -f "$BUILD_DIR/phantomvault" ]]; then
            print_error "Failed to build PhantomVault application"
            exit 1
        fi
    fi
    
    # Create package directory
    mkdir -p "$PACKAGE_DIR"
    
    print_success "Build environment ready"
}

# Build Linux packages
build_linux_packages() {
    print_header "Building Linux packages..."
    
    if [[ -f "$SCRIPT_DIR/build-linux-packages.sh" ]]; then
        "$SCRIPT_DIR/build-linux-packages.sh"
        print_success "Linux packages built successfully"
    else
        print_warning "Linux package builder not found - skipping"
    fi
}

# Build Windows installer
build_windows_installer() {
    print_header "Building Windows installer..."
    
    # Check if we can build Windows installer
    if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]] || command -v wine &> /dev/null; then
        if [[ -f "$SCRIPT_DIR/build-windows-installer.sh" ]]; then
            "$SCRIPT_DIR/build-windows-installer.sh"
            print_success "Windows installer built successfully"
        else
            print_warning "Windows installer builder not found - skipping"
        fi
    else
        print_warning "Windows build environment not available - skipping Windows installer"
        print_status "To build Windows installer:"
        print_status "  • Run on Windows with WiX Toolset"
        print_status "  • Or install Wine: sudo apt-get install wine"
    fi
}

# Build macOS installer
build_macos_installer() {
    print_header "Building macOS installer..."
    
    if [[ "$(uname -s)" == "Darwin" ]]; then
        if [[ -f "$SCRIPT_DIR/build-macos-installer.sh" ]]; then
            "$SCRIPT_DIR/build-macos-installer.sh"
            print_success "macOS installer built successfully"
        else
            print_warning "macOS installer builder not found - skipping"
        fi
    else
        print_warning "macOS build environment not available - skipping macOS installer"
        print_status "To build macOS installer, run this script on macOS"
    fi
}

# Create universal installer script
create_universal_installer() {
    print_header "Creating universal installer script..."
    
    local universal_script="$PACKAGE_DIR/install-phantomvault.sh"
    
    cat > "$universal_script" << 'EOF'
#!/bin/bash
# PhantomVault Universal Installer
# Automatically detects platform and installs appropriate package

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() { echo -e "${BLUE}[INSTALL]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Detect platform
detect_platform() {
    case "$(uname -s)" in
        Linux*)
            PLATFORM="linux"
            if command -v apt-get &> /dev/null; then
                DISTRO="debian"
            elif command -v dnf &> /dev/null; then
                DISTRO="fedora"
            elif command -v yum &> /dev/null; then
                DISTRO="rhel"
            elif command -v pacman &> /dev/null; then
                DISTRO="arch"
            elif command -v zypper &> /dev/null; then
                DISTRO="opensuse"
            else
                DISTRO="unknown"
            fi
            ;;
        Darwin*)
            PLATFORM="macos"
            DISTRO="macos"
            ;;
        CYGWIN*|MINGW32*|MSYS*|MINGW*)
            PLATFORM="windows"
            DISTRO="windows"
            ;;
        *)
            PLATFORM="unknown"
            DISTRO="unknown"
            ;;
    esac
}

# Install for Linux
install_linux() {
    print_status "Installing PhantomVault for Linux ($DISTRO)..."
    
    # Check for available packages
    local deb_package="phantomvault_1.0.0_amd64.deb"
    local rpm_package="phantomvault-1.0.0-1.x86_64.rpm"
    local appimage="PhantomVault-1.0.0-x86_64.AppImage"
    
    if [[ "$DISTRO" == "debian" && -f "$deb_package" ]]; then
        print_status "Installing DEB package..."
        sudo dpkg -i "$deb_package" || {
            print_status "Fixing dependencies..."
            sudo apt-get install -f -y
        }
    elif [[ "$DISTRO" =~ ^(fedora|rhel|opensuse)$ && -f "$rpm_package" ]]; then
        print_status "Installing RPM package..."
        if command -v dnf &> /dev/null; then
            sudo dnf install -y "$rpm_package"
        elif command -v yum &> /dev/null; then
            sudo yum install -y "$rpm_package"
        elif command -v zypper &> /dev/null; then
            sudo zypper install -y "$rpm_package"
        fi
    elif [[ -f "$appimage" ]]; then
        print_status "Installing AppImage..."
        chmod +x "$appimage"
        mkdir -p "$HOME/.local/bin"
        cp "$appimage" "$HOME/.local/bin/phantomvault"
        
        # Create desktop entry
        mkdir -p "$HOME/.local/share/applications"
        cat > "$HOME/.local/share/applications/phantomvault.desktop" << 'DESKTOP_EOF'
[Desktop Entry]
Name=PhantomVault
Comment=Invisible Folder Security
Exec=$HOME/.local/bin/phantomvault
Icon=security
Terminal=true
Categories=Security;Utility;
DESKTOP_EOF
        
        print_success "AppImage installed to ~/.local/bin/phantomvault"
    else
        print_error "No suitable package found for $DISTRO"
        print_status "Available installation methods:"
        print_status "1. Download and install manually from GitHub releases"
        print_status "2. Build from source: git clone https://github.com/ishaq2321/phantomVault"
        exit 1
    fi
}

# Install for macOS
install_macos() {
    print_status "Installing PhantomVault for macOS..."
    
    local pkg_installer="PhantomVault-1.0.0.pkg"
    local dmg_installer="PhantomVault-1.0.0.dmg"
    
    if [[ -f "$pkg_installer" ]]; then
        print_status "Installing PKG package..."
        sudo installer -pkg "$pkg_installer" -target /
    elif [[ -f "$dmg_installer" ]]; then
        print_status "Mounting DMG installer..."
        hdiutil attach "$dmg_installer"
        
        # Find the mounted volume
        local mount_point="/Volumes/PhantomVault 1.0.0"
        if [[ -d "$mount_point" ]]; then
            if [[ -f "$mount_point/Install PhantomVault.pkg" ]]; then
                sudo installer -pkg "$mount_point/Install PhantomVault.pkg" -target /
            fi
            hdiutil detach "$mount_point"
        fi
    else
        print_error "No macOS installer found"
        print_status "Please download the installer from GitHub releases"
        exit 1
    fi
}

# Install for Windows
install_windows() {
    print_status "Installing PhantomVault for Windows..."
    
    local msi_installer="PhantomVault-1.0.0.msi"
    local exe_installer="PhantomVault-Setup.exe"
    
    if [[ -f "$msi_installer" ]]; then
        print_status "Running MSI installer..."
        msiexec /i "$msi_installer" /quiet
    elif [[ -f "$exe_installer" ]]; then
        print_status "Running EXE installer..."
        "./$exe_installer" /S
    else
        print_error "No Windows installer found"
        print_status "Please download the installer from GitHub releases"
        exit 1
    fi
}

# Main installation function
main() {
    echo "PhantomVault Universal Installer"
    echo "==============================="
    echo ""
    
    detect_platform
    print_status "Detected platform: $PLATFORM ($DISTRO)"
    
    case "$PLATFORM" in
        linux)
            install_linux
            ;;
        macos)
            install_macos
            ;;
        windows)
            install_windows
            ;;
        *)
            print_error "Unsupported platform: $PLATFORM"
            print_status "Supported platforms: Linux, macOS, Windows"
            exit 1
            ;;
    esac
    
    print_success "PhantomVault installed successfully!"
    echo ""
    echo "Getting Started:"
    echo "• Run 'phantomvault --help' for command line options"
    echo "• Press Ctrl+Alt+V anywhere to access your folders"
    echo "• Open PhantomVault from your applications menu"
    echo ""
    echo "For support: https://github.com/ishaq2321/phantomVault"
}

# Run main function
main "$@"
EOF
    
    chmod +x "$universal_script"
    print_success "Universal installer script created: $universal_script"
}

# Create installer documentation
create_documentation() {
    print_header "Creating installer documentation..."
    
    cat > "$PACKAGE_DIR/INSTALLATION.md" << 'EOF'
# PhantomVault Installation Guide

## Quick Installation

### Universal Installer (Recommended)
```bash
curl -fsSL https://raw.githubusercontent.com/ishaq2321/phantomVault/main/installer/install.sh | bash
```

### Platform-Specific Installers

#### Linux
- **Debian/Ubuntu**: `sudo dpkg -i phantomvault_1.0.0_amd64.deb`
- **Red Hat/Fedora**: `sudo dnf install phantomvault-1.0.0-1.x86_64.rpm`
- **Universal**: `chmod +x PhantomVault-1.0.0-x86_64.AppImage && ./PhantomVault-1.0.0-x86_64.AppImage`

#### macOS
- **PKG Installer**: `sudo installer -pkg PhantomVault-1.0.0.pkg -target /`
- **DMG**: Mount the DMG and run the installer package

#### Windows
- **MSI**: `msiexec /i PhantomVault-1.0.0.msi`
- **EXE**: Run `PhantomVault-Setup.exe` as administrator

## System Requirements

### Linux
- **OS**: Ubuntu 18.04+, Debian 10+, Fedora 30+, CentOS 8+, Arch Linux
- **Dependencies**: OpenSSL, X11, XTest, GTK3, systemd
- **Memory**: 50MB RAM
- **Disk**: 100MB free space

### macOS
- **OS**: macOS 10.12 (Sierra) or later
- **Architecture**: Intel x64 or Apple Silicon (M1/M2)
- **Memory**: 50MB RAM
- **Disk**: 100MB free space

### Windows
- **OS**: Windows 7 SP1 or later (Windows 10/11 recommended)
- **Architecture**: x64
- **Dependencies**: Visual C++ Redistributable, .NET Framework 4.7.2
- **Memory**: 50MB RAM
- **Disk**: 100MB free space

## Installation Features

### Service Integration
- **Linux**: systemd service with automatic startup
- **macOS**: LaunchDaemon with automatic startup
- **Windows**: Windows Service with automatic startup

### Desktop Integration
- Application shortcuts in start menu/applications folder
- Desktop shortcuts (optional)
- File associations for .phantomvault profiles
- System tray integration

### Command Line Tools
- `phantomvault` command available system-wide
- Tab completion support
- Man pages (Linux/macOS)

## Post-Installation

### First Run
1. The service starts automatically after installation
2. Press **Ctrl+Alt+V** anywhere to test folder access
3. Run `phantomvault --help` for command options
4. Open PhantomVault GUI from applications menu

### Configuration
- Service configuration: `/etc/phantomvault/` (Linux), `/usr/local/etc/phantomvault/` (macOS), `%PROGRAMDATA%\PhantomVault\` (Windows)
- User data: `~/.phantomvault/`
- Logs: `/var/log/phantomvault/` (Linux), `/usr/local/var/log/` (macOS), `%PROGRAMDATA%\PhantomVault\logs\` (Windows)

### Verification
```bash
# Check service status
phantomvault status

# View service logs
phantomvault --logs

# Test functionality
phantomvault --gui
```

## Troubleshooting

### Service Not Starting
```bash
# Check service status
systemctl status phantomvault  # Linux
launchctl list | grep phantomvault  # macOS
sc query PhantomVault  # Windows

# Restart service
sudo systemctl restart phantomvault  # Linux
sudo launchctl unload/load /Library/LaunchDaemons/com.phantomvault.service.plist  # macOS
net stop PhantomVault && net start PhantomVault  # Windows
```

### Permission Issues
- Ensure you ran the installer with administrator privileges
- Check that your user is in the appropriate groups
- Verify firewall settings allow the service

### Dependencies Missing
```bash
# Linux - install missing dependencies
sudo apt-get install libssl3 libx11-6 libxtst6 libgtk-3-0  # Debian/Ubuntu
sudo dnf install openssl libX11 libXtst gtk3  # Fedora

# Windows - install Visual C++ Redistributable and .NET Framework
# macOS - update to supported macOS version
```

## Uninstallation

### Linux
```bash
# DEB package
sudo dpkg -r phantomvault

# RPM package
sudo dnf remove phantomvault  # or yum remove

# AppImage
rm ~/.local/bin/phantomvault
rm ~/.local/share/applications/phantomvault.desktop
```

### macOS
```bash
# Remove service
sudo launchctl unload /Library/LaunchDaemons/com.phantomvault.service.plist
sudo rm /Library/LaunchDaemons/com.phantomvault.service.plist

# Remove application files
sudo rm -rf /usr/local/bin/phantomvault*
sudo rm -rf /usr/local/var/phantomvault
rm -rf /Applications/PhantomVault.app
```

### Windows
- Use "Add or Remove Programs" in Windows Settings
- Or run the uninstaller from the installation directory

## Support

- **Documentation**: https://github.com/ishaq2321/phantomVault/wiki
- **Issues**: https://github.com/ishaq2321/phantomVault/issues
- **Discussions**: https://github.com/ishaq2321/phantomVault/discussions
- **Email**: support@phantomvault.com

## Security Notes

- PhantomVault requires administrator privileges for system-level folder operations
- The service runs with elevated permissions to access protected system areas
- All data is encrypted using military-grade AES-256-XTS encryption
- No telemetry or data collection is performed
EOF
    
    print_success "Installation documentation created"
}

# Generate checksums and signatures
generate_checksums() {
    print_header "Generating checksums and signatures..."
    
    cd "$PACKAGE_DIR"
    
    # Generate SHA256 checksums
    find . -name "*.deb" -o -name "*.rpm" -o -name "*.AppImage" -o -name "*.pkg" -o -name "*.dmg" -o -name "*.msi" -o -name "*.exe" | while read -r file; do
        if [[ -f "$file" ]]; then
            sha256sum "$file" >> checksums.sha256
        fi
    done
    
    # Generate MD5 checksums (for compatibility)
    find . -name "*.deb" -o -name "*.rpm" -o -name "*.AppImage" -o -name "*.pkg" -o -name "*.dmg" -o -name "*.msi" -o -name "*.exe" | while read -r file; do
        if [[ -f "$file" ]]; then
            md5sum "$file" >> checksums.md5
        fi
    done
    
    print_success "Checksums generated"
}

# Create release summary
create_release_summary() {
    print_header "Creating release summary..."
    
    cat > "$PACKAGE_DIR/RELEASE_SUMMARY.md" << EOF
# PhantomVault v$VERSION Release

## Package Summary

### Linux Packages
$(find "$PACKAGE_DIR" -name "*.deb" -o -name "*.rpm" -o -name "*.AppImage" | while read -r file; do
    if [[ -f "$file" ]]; then
        echo "- $(basename "$file") ($(du -h "$file" | cut -f1))"
    fi
done)

### macOS Packages
$(find "$PACKAGE_DIR" -name "*.pkg" -o -name "*.dmg" | while read -r file; do
    if [[ -f "$file" ]]; then
        echo "- $(basename "$file") ($(du -h "$file" | cut -f1))"
    fi
done)

### Windows Packages
$(find "$PACKAGE_DIR" -name "*.msi" -o -name "*.exe" | while read -r file; do
    if [[ -f "$file" ]]; then
        echo "- $(basename "$file") ($(du -h "$file" | cut -f1))"
    fi
done)

## Installation

### Quick Install (Linux/macOS)
\`\`\`bash
curl -fsSL https://raw.githubusercontent.com/ishaq2321/phantomVault/main/installer/install.sh | bash
\`\`\`

### Manual Installation
1. Download the appropriate package for your platform
2. Verify checksums against checksums.sha256
3. Install using platform-specific method (see INSTALLATION.md)

## What's New in v$VERSION

- Enhanced integrity verification systems with SHA-3 hashing
- Merkle tree verification for large folder structures
- Digital signature verification for tamper detection
- Real-time integrity monitoring with automatic corruption detection
- Professional installer packages for all platforms
- Comprehensive systemd/LaunchDaemon/Windows Service integration

## System Requirements

- **Linux**: Ubuntu 18.04+, Debian 10+, Fedora 30+, CentOS 8+
- **macOS**: 10.12 (Sierra) or later
- **Windows**: Windows 7 SP1 or later

## Support

- Documentation: https://github.com/ishaq2321/phantomVault/wiki
- Issues: https://github.com/ishaq2321/phantomVault/issues
- Email: support@phantomvault.com

---
Built on $(date) by PhantomVault Build System
EOF
    
    print_success "Release summary created"
}

# Main function
main() {
    print_banner
    
    # Parse command line arguments
    local build_linux=true
    local build_windows=true
    local build_macos=true
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --linux-only)
                build_windows=false
                build_macos=false
                shift
                ;;
            --windows-only)
                build_linux=false
                build_macos=false
                shift
                ;;
            --macos-only)
                build_linux=false
                build_windows=false
                shift
                ;;
            --no-linux)
                build_linux=false
                shift
                ;;
            --no-windows)
                build_windows=false
                shift
                ;;
            --no-macos)
                build_macos=false
                shift
                ;;
            --help|-h)
                echo "Usage: $0 [OPTIONS]"
                echo ""
                echo "Options:"
                echo "  --linux-only     Build only Linux packages"
                echo "  --windows-only   Build only Windows installer"
                echo "  --macos-only     Build only macOS installer"
                echo "  --no-linux       Skip Linux packages"
                echo "  --no-windows     Skip Windows installer"
                echo "  --no-macos       Skip macOS installer"
                echo "  --help           Show this help"
                exit 0
                ;;
            *)
                print_warning "Unknown option: $1"
                shift
                ;;
        esac
    done
    
    check_environment
    
    # Build platform-specific installers
    if [[ "$build_linux" == true ]]; then
        build_linux_packages
    fi
    
    if [[ "$build_windows" == true ]]; then
        build_windows_installer
    fi
    
    if [[ "$build_macos" == true ]]; then
        build_macos_installer
    fi
    
    # Create universal components
    create_universal_installer
    create_documentation
    generate_checksums
    create_release_summary
    
    print_success "All installers built successfully!"
    print_status "Packages available in: $PACKAGE_DIR"
    
    # Show summary
    echo ""
    echo "Build Summary:"
    echo "=============="
    
    local total_packages=0
    
    if [[ -d "$PACKAGE_DIR/linux" ]]; then
        local linux_count=$(find "$PACKAGE_DIR/linux" -name "*.deb" -o -name "*.rpm" -o -name "*.AppImage" | wc -l)
        echo "Linux packages: $linux_count"
        total_packages=$((total_packages + linux_count))
    fi
    
    if [[ -d "$PACKAGE_DIR/windows" ]]; then
        local windows_count=$(find "$PACKAGE_DIR/windows" -name "*.msi" -o -name "*.exe" | wc -l)
        echo "Windows packages: $windows_count"
        total_packages=$((total_packages + windows_count))
    fi
    
    if [[ -d "$PACKAGE_DIR/macos" ]]; then
        local macos_count=$(find "$PACKAGE_DIR/macos" -name "*.pkg" -o -name "*.dmg" | wc -l)
        echo "macOS packages: $macos_count"
        total_packages=$((total_packages + macos_count))
    fi
    
    echo "Total packages: $total_packages"
    echo ""
    echo "Next steps:"
    echo "1. Test installers on target platforms"
    echo "2. Upload to GitHub releases"
    echo "3. Update documentation"
    echo "4. Announce release"
}

# Run main function
main "$@"