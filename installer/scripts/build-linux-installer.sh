#!/bin/bash

# PhantomVault Linux Installer Builder
# Creates DEB and RPM packages with complete uninstaller

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
PACKAGE_NAME="phantomvault"
VERSION="1.1.0"
MAINTAINER="PhantomVault Team <team@phantomvault.dev>"
DESCRIPTION="Invisible Folder Security with Profile-Based Management"
HOMEPAGE="https://github.com/ishaq2321/phantomVault"

# Directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/installer/build"
DEB_DIR="$BUILD_DIR/deb"
RPM_DIR="$BUILD_DIR/rpm"

print_status "Building Linux installer packages..."
print_status "Project root: $PROJECT_ROOT"
print_status "Build directory: $BUILD_DIR"

# Clean and create build directories
rm -rf "$DEB_DIR" "$RPM_DIR"
mkdir -p "$DEB_DIR" "$RPM_DIR"

# Create DEB package structure
create_deb_package() {
    print_status "Creating DEB package..."
    
    local deb_root="$DEB_DIR/phantomvault_${VERSION}_amd64"
    mkdir -p "$deb_root"/{DEBIAN,opt/phantomvault/{bin,gui,docs},usr/{bin,share/{applications,pixmaps}},etc/systemd/system}
    
    # Copy binaries
    if [ -f "$PROJECT_ROOT/bin/phantomvault" ]; then
        cp "$PROJECT_ROOT/bin/phantomvault" "$deb_root/opt/phantomvault/bin/"
        chmod +x "$deb_root/opt/phantomvault/bin/phantomvault"
    elif [ -f "$PROJECT_ROOT/core/build/bin/phantomvault-service" ]; then
        cp "$PROJECT_ROOT/core/build/bin/phantomvault-service" "$deb_root/opt/phantomvault/bin/phantomvault"
        chmod +x "$deb_root/opt/phantomvault/bin/phantomvault"
    else
        print_error "No service executable found!"
        return 1
    fi
    
    # Copy GUI files if they exist
    if [ -d "$PROJECT_ROOT/gui/dist" ]; then
        cp -r "$PROJECT_ROOT/gui/dist"/* "$deb_root/opt/phantomvault/gui/"
    fi
    
    # Copy documentation
    if [ -f "$PROJECT_ROOT/README.md" ]; then
        cp "$PROJECT_ROOT/README.md" "$deb_root/opt/phantomvault/docs/"
    fi
    
    # Create symlink in /usr/bin
    ln -sf "/opt/phantomvault/bin/phantomvault" "$deb_root/usr/bin/phantomvault"
    
    # Create desktop entry
    cat > "$deb_root/usr/share/applications/phantomvault.desktop" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=PhantomVault
Comment=Invisible Folder Security with Profile-Based Management
Exec=/opt/phantomvault/bin/phantomvault --gui
Icon=phantomvault
Terminal=false
Categories=Security;Utility;
StartupWMClass=PhantomVault
EOF
    
    # Create systemd service
    cat > "$deb_root/etc/systemd/system/phantomvault.service" << EOF
[Unit]
Description=PhantomVault Security Service
Documentation=https://github.com/phantomvault/phantomvault
After=network.target graphical-session.target
Wants=network.target

[Service]
Type=simple
User=root
Group=root
ExecStart=/opt/phantomvault/bin/phantomvault --service
ExecStop=/bin/kill -TERM \$MAINPID
Restart=on-failure
RestartSec=5
StandardOutput=journal
StandardError=journal
Environment=DISPLAY=:0
Environment=XAUTHORITY=/home/%i/.Xauthority

[Install]
WantedBy=multi-user.target
EOF
    
    # Create control file
    cat > "$deb_root/DEBIAN/control" << EOF
Package: $PACKAGE_NAME
Version: $VERSION
Section: utils
Priority: optional
Architecture: amd64
Depends: libc6, libssl3, libstdc++6
Maintainer: $MAINTAINER
Description: $DESCRIPTION
 PhantomVault provides invisible folder security with AES-256 encryption,
 profile-based management, and cross-platform compatibility. Secure your
 sensitive files with advanced encryption and invisible keyboard sequence
 detection.
Homepage: $HOMEPAGE
EOF
    
    # Create postinst script
    cat > "$deb_root/DEBIAN/postinst" << 'EOF'
#!/bin/bash
set -e

# Enable and start systemd service
systemctl daemon-reload
systemctl enable phantomvault.service

# Create phantomvault group and add current user
groupadd -f phantomvault
if [ -n "$SUDO_USER" ]; then
    usermod -a -G phantomvault "$SUDO_USER"
fi

# Set proper permissions
chown -R root:phantomvault /opt/phantomvault
chmod -R 755 /opt/phantomvault
chmod 4755 /opt/phantomvault/bin/phantomvault

# Update desktop database
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database /usr/share/applications
fi

echo "PhantomVault installed successfully!"
echo "Start with: sudo systemctl start phantomvault"
echo "Or run GUI: phantomvault --gui"
EOF
    
    # Create prerm script (uninstaller preparation)
    cat > "$deb_root/DEBIAN/prerm" << 'EOF'
#!/bin/bash
set -e

# Stop and disable service
systemctl stop phantomvault.service || true
systemctl disable phantomvault.service || true
EOF
    
    # Create postrm script (complete uninstaller)
    cat > "$deb_root/DEBIAN/postrm" << 'EOF'
#!/bin/bash
set -e

if [ "$1" = "purge" ]; then
    # Remove systemd service file
    rm -f /etc/systemd/system/phantomvault.service
    systemctl daemon-reload
    
    # Remove application directory (preserve user data)
    rm -rf /opt/phantomvault
    
    # Remove desktop entry
    rm -f /usr/share/applications/phantomvault.desktop
    
    # Update desktop database
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database /usr/share/applications
    fi
    
    # Remove group (but keep user data)
    groupdel phantomvault 2>/dev/null || true
    
    echo "PhantomVault completely removed."
    echo "User data preserved in ~/.phantomvault/"
fi
EOF
    
    # Make scripts executable
    chmod 755 "$deb_root/DEBIAN"/{postinst,prerm,postrm}
    
    # Build DEB package
    dpkg-deb --build "$deb_root"
    
    if [ $? -eq 0 ]; then
        print_success "DEB package created: $deb_root.deb"
    else
        print_error "Failed to create DEB package"
        return 1
    fi
}

# Create RPM package structure
create_rpm_package() {
    print_status "Creating RPM package..."
    
    # Check if rpmbuild is available
    if ! command -v rpmbuild &> /dev/null; then
        print_warning "rpmbuild not found, skipping RPM package creation"
        print_status "Install with: sudo apt-get install rpm"
        return 0
    fi
    
    local rpm_root="$RPM_DIR/rpmbuild"
    mkdir -p "$rpm_root"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
    
    # Create source tarball
    local source_dir="$rpm_root/SOURCES/phantomvault-$VERSION"
    mkdir -p "$source_dir"
    
    # Copy files to source directory
    if [ -f "$PROJECT_ROOT/bin/phantomvault" ]; then
        mkdir -p "$source_dir/bin"
        cp "$PROJECT_ROOT/bin/phantomvault" "$source_dir/bin/"
    elif [ -f "$PROJECT_ROOT/core/build/bin/phantomvault-service" ]; then
        mkdir -p "$source_dir/bin"
        cp "$PROJECT_ROOT/core/build/bin/phantomvault-service" "$source_dir/bin/phantomvault"
    fi
    
    if [ -d "$PROJECT_ROOT/gui/dist" ]; then
        cp -r "$PROJECT_ROOT/gui/dist" "$source_dir/gui"
    fi
    
    if [ -f "$PROJECT_ROOT/README.md" ]; then
        mkdir -p "$source_dir/docs"
        cp "$PROJECT_ROOT/README.md" "$source_dir/docs/"
    fi
    
    # Create tarball
    cd "$rpm_root/SOURCES"
    tar -czf "phantomvault-$VERSION.tar.gz" "phantomvault-$VERSION"
    cd - > /dev/null
    
    # Create RPM spec file
    cat > "$rpm_root/SPECS/phantomvault.spec" << EOF
Name:           phantomvault
Version:        $VERSION
Release:        1%{?dist}
Summary:        $DESCRIPTION
License:        MIT
URL:            $HOMEPAGE
Source0:        %{name}-%{version}.tar.gz
BuildArch:      x86_64

Requires:       glibc, openssl-libs, libstdc++

%description
PhantomVault provides invisible folder security with AES-256 encryption,
profile-based management, and cross-platform compatibility. Secure your
sensitive files with advanced encryption and invisible keyboard sequence
detection.

%prep
%setup -q

%build
# Nothing to build, binaries are pre-compiled

%install
rm -rf \$RPM_BUILD_ROOT
mkdir -p \$RPM_BUILD_ROOT/opt/phantomvault/bin
mkdir -p \$RPM_BUILD_ROOT/opt/phantomvault/gui
mkdir -p \$RPM_BUILD_ROOT/opt/phantomvault/docs
mkdir -p \$RPM_BUILD_ROOT/usr/bin
mkdir -p \$RPM_BUILD_ROOT/usr/share/applications
mkdir -p \$RPM_BUILD_ROOT/usr/share/pixmaps
mkdir -p \$RPM_BUILD_ROOT/etc/systemd/system

# Install binaries
cp bin/phantomvault \$RPM_BUILD_ROOT/opt/phantomvault/bin/
chmod +x \$RPM_BUILD_ROOT/opt/phantomvault/bin/phantomvault

# Install GUI if available
if [ -d gui ]; then
    cp -r gui/* \$RPM_BUILD_ROOT/opt/phantomvault/gui/
fi

# Install documentation
if [ -d docs ]; then
    cp -r docs/* \$RPM_BUILD_ROOT/opt/phantomvault/docs/
fi

# Create symlink
ln -sf /opt/phantomvault/bin/phantomvault \$RPM_BUILD_ROOT/usr/bin/phantomvault

# Install desktop entry
cat > \$RPM_BUILD_ROOT/usr/share/applications/phantomvault.desktop << 'DESKTOP_EOF'
[Desktop Entry]
Version=1.0
Type=Application
Name=PhantomVault
Comment=Invisible Folder Security with Profile-Based Management
Exec=/opt/phantomvault/bin/phantomvault --gui
Icon=phantomvault
Terminal=false
Categories=Security;Utility;
StartupWMClass=PhantomVault
DESKTOP_EOF

# Install systemd service
cat > \$RPM_BUILD_ROOT/etc/systemd/system/phantomvault.service << 'SERVICE_EOF'
[Unit]
Description=PhantomVault Security Service
After=network.target

[Service]
Type=simple
User=root
ExecStart=/opt/phantomvault/bin/phantomvault --service
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
SERVICE_EOF

%files
/opt/phantomvault/
/usr/bin/phantomvault
/usr/share/applications/phantomvault.desktop
/etc/systemd/system/phantomvault.service

%post
systemctl daemon-reload
systemctl enable phantomvault.service
groupadd -f phantomvault
chown -R root:phantomvault /opt/phantomvault
chmod -R 755 /opt/phantomvault
chmod 4755 /opt/phantomvault/bin/phantomvault
echo "PhantomVault installed successfully!"

%preun
systemctl stop phantomvault.service || true
systemctl disable phantomvault.service || true

%postun
if [ \$1 -eq 0 ]; then
    rm -rf /opt/phantomvault
    systemctl daemon-reload
    groupdel phantomvault 2>/dev/null || true
    echo "PhantomVault completely removed."
fi

%changelog
* $(date '+%a %b %d %Y') PhantomVault Team <team@phantomvault.dev> - $VERSION-1
- Initial RPM package release
- Complete installer with uninstaller
- Systemd service integration
EOF
    
    # Build RPM package
    rpmbuild --define "_topdir $rpm_root" -ba "$rpm_root/SPECS/phantomvault.spec"
    
    if [ $? -eq 0 ]; then
        print_success "RPM package created in: $rpm_root/RPMS/"
        find "$rpm_root/RPMS" -name "*.rpm" -exec ls -la {} \;
    else
        print_error "Failed to create RPM package"
        return 1
    fi
}

# Create standalone installer script
create_standalone_installer() {
    print_status "Creating standalone installer script..."
    
    cat > "$BUILD_DIR/phantomvault-linux-installer.sh" << 'EOF'
#!/bin/bash

# PhantomVault Linux Standalone Installer
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

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    print_error "This installer must be run as root (use sudo)"
    exit 1
fi

print_status "PhantomVault Linux Installer v1.0.0"
print_status "Installing invisible folder security system..."

# Create installation directory
INSTALL_DIR="/opt/phantomvault"
mkdir -p "$INSTALL_DIR"/{bin,gui,docs}

# Extract and install files (this would contain the actual binaries)
print_status "Installing PhantomVault service..."

# Copy service executable (placeholder - would be embedded in real installer)
if [ -f "./phantomvault" ]; then
    cp "./phantomvault" "$INSTALL_DIR/bin/"
    chmod +x "$INSTALL_DIR/bin/phantomvault"
else
    print_error "Service executable not found in installer"
    exit 1
fi

# Create symlink
ln -sf "$INSTALL_DIR/bin/phantomvault" "/usr/bin/phantomvault"

# Create systemd service
cat > "/etc/systemd/system/phantomvault.service" << 'SERVICE_EOF'
[Unit]
Description=PhantomVault Security Service
After=network.target

[Service]
Type=simple
User=root
ExecStart=/opt/phantomvault/bin/phantomvault --service
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
SERVICE_EOF

# Create desktop entry
mkdir -p "/usr/share/applications"
cat > "/usr/share/applications/phantomvault.desktop" << 'DESKTOP_EOF'
[Desktop Entry]
Version=1.0
Type=Application
Name=PhantomVault
Comment=Invisible Folder Security with Profile-Based Management
Exec=/opt/phantomvault/bin/phantomvault --gui
Icon=phantomvault
Terminal=false
Categories=Security;Utility;
StartupWMClass=PhantomVault
DESKTOP_EOF

# Set up permissions
groupadd -f phantomvault
chown -R root:phantomvault "$INSTALL_DIR"
chmod -R 755 "$INSTALL_DIR"
chmod 4755 "$INSTALL_DIR/bin/phantomvault"

# Enable systemd service
systemctl daemon-reload
systemctl enable phantomvault.service

# Create uninstaller
cat > "$INSTALL_DIR/uninstall.sh" << 'UNINSTALL_EOF'
#!/bin/bash

# PhantomVault Uninstaller
# Completely removes PhantomVault while preserving user data

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_status() { echo -e "${YELLOW}[UNINSTALL]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

if [ "$EUID" -ne 0 ]; then
    print_error "Uninstaller must be run as root (use sudo)"
    exit 1
fi

print_status "PhantomVault Uninstaller"
echo "This will completely remove PhantomVault from your system."
echo "User data in ~/.phantomvault/ will be preserved."
read -p "Continue? (y/N): " -n 1 -r
echo

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Uninstall cancelled."
    exit 0
fi

print_status "Stopping PhantomVault service..."
systemctl stop phantomvault.service || true
systemctl disable phantomvault.service || true

print_status "Removing system files..."
rm -f /etc/systemd/system/phantomvault.service
rm -f /usr/bin/phantomvault
rm -f /usr/share/applications/phantomvault.desktop
rm -rf /opt/phantomvault

print_status "Cleaning up system..."
systemctl daemon-reload
groupdel phantomvault 2>/dev/null || true

# Update desktop database
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database /usr/share/applications
fi

print_success "PhantomVault has been completely removed."
print_status "User data preserved in ~/.phantomvault/"
UNINSTALL_EOF

chmod +x "$INSTALL_DIR/uninstall.sh"

print_success "PhantomVault installed successfully!"
print_status "Service: sudo systemctl start phantomvault"
print_status "GUI: phantomvault --gui"
print_status "Uninstall: sudo $INSTALL_DIR/uninstall.sh"

EOF
    
    chmod +x "$BUILD_DIR/phantomvault-linux-installer.sh"
    print_success "Standalone installer created: $BUILD_DIR/phantomvault-linux-installer.sh"
}

# Main execution
main() {
    print_status "Starting Linux installer build process..."
    
    # Check dependencies
    if ! command -v dpkg-deb &> /dev/null; then
        print_warning "dpkg-deb not found, skipping DEB package creation"
    else
        create_deb_package
    fi
    
    create_rpm_package
    create_standalone_installer
    
    print_success "Linux installer packages created successfully!"
    print_status "Available packages:"
    find "$BUILD_DIR" -name "*.deb" -o -name "*.rpm" -o -name "*installer*.sh" | while read file; do
        print_status "  - $(basename "$file")"
    done
}

# Run main function
main "$@"