#!/bin/bash

# PhantomVault Linux Installer Builder
# Creates DEB and RPM packages with complete uninstaller functionality

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${BLUE}[INSTALLER]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Get version from package.json
VERSION=$(grep '"version"' gui/package.json | sed 's/.*"version": "\(.*\)".*/\1/')
PACKAGE_NAME="phantomvault"
MAINTAINER="PhantomVault Team <team@phantomvault.dev>"
DESCRIPTION="Invisible Folder Security with Profile-Based Management"

print_status "Building Linux installer packages for PhantomVault v$VERSION"

# Create build directories
mkdir -p installer/build/linux/{deb,rpm}
mkdir -p installer/build/linux/deb/DEBIAN
mkdir -p installer/build/linux/deb/opt/phantomvault/{bin,gui,docs}
mkdir -p installer/build/linux/deb/usr/share/{applications,pixmaps}
mkdir -p installer/build/linux/deb/etc/systemd/system
mkdir -p installer/build/linux/deb/usr/local/bin

# Copy application files
print_status "Copying application files..."

# Copy unified executable
cp bin/phantomvault installer/build/linux/deb/opt/phantomvault/bin/
chmod +x installer/build/linux/deb/opt/phantomvault/bin/phantomvault

# Copy GUI files
cp -r gui/dist/* installer/build/linux/deb/opt/phantomvault/gui/
cp gui/package.json installer/build/linux/deb/opt/phantomvault/gui/

# Copy documentation
cp README.md installer/build/linux/deb/opt/phantomvault/docs/
cp LICENSE installer/build/linux/deb/opt/phantomvault/docs/
cp -r docs/* installer/build/linux/deb/opt/phantomvault/docs/ 2>/dev/null || true

# Create desktop entry
cat > installer/build/linux/deb/usr/share/applications/phantomvault.desktop << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=PhantomVault
Comment=Invisible Folder Security with Profile-Based Management
Exec=/opt/phantomvault/bin/phantomvault --gui
Icon=phantomvault
Terminal=false
StartupNotify=true
Categories=Security;Utility;FileManager;
MimeType=application/x-phantomvault-profile;
Keywords=security;encryption;privacy;folders;vault;
StartupWMClass=PhantomVault
EOF

# Create icon (placeholder - in production would use actual icon)
cp gui/assets/icon.png installer/build/linux/deb/usr/share/pixmaps/phantomvault.png 2>/dev/null || \
echo "# PhantomVault Icon Placeholder" > installer/build/linux/deb/usr/share/pixmaps/phantomvault.png

# Create systemd service file
cat > installer/build/linux/deb/etc/systemd/system/phantomvault.service << EOF
[Unit]
Description=PhantomVault Security Service
After=network.target
Wants=network.target

[Service]
Type=simple
ExecStart=/opt/phantomvault/bin/phantomvault --service
Restart=always
RestartSec=5
User=root
Group=root
StandardOutput=journal
StandardError=journal
SyslogIdentifier=phantomvault

# Security settings
NoNewPrivileges=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/opt/phantomvault /var/lib/phantomvault
PrivateTmp=true
ProtectKernelTunables=true
ProtectKernelModules=true
ProtectControlGroups=true

[Install]
WantedBy=multi-user.target
EOF

# Create command-line wrapper
cat > installer/build/linux/deb/usr/local/bin/phantomvault << 'EOF'
#!/bin/bash
# PhantomVault Command Line Wrapper
exec /opt/phantomvault/bin/phantomvault "$@"
EOF
chmod +x installer/build/linux/deb/usr/local/bin/phantomvault

# Create uninstaller script
cat > installer/build/linux/deb/opt/phantomvault/bin/uninstall.sh << 'EOF'
#!/bin/bash

# PhantomVault Uninstaller
# Safely removes PhantomVault while preserving user data

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() {
    echo -e "${BLUE}[UNINSTALL]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    print_error "Please run uninstaller as root (sudo)"
    exit 1
fi

print_status "PhantomVault Uninstaller"
echo "This will remove PhantomVault from your system."
echo "User data and profiles will be preserved in ~/.phantomvault/"
echo ""

# Confirm uninstallation
read -p "Are you sure you want to uninstall PhantomVault? (y/N): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    print_status "Uninstallation cancelled"
    exit 0
fi

print_status "Stopping PhantomVault service..."
systemctl stop phantomvault 2>/dev/null || true
systemctl disable phantomvault 2>/dev/null || true

print_status "Removing application files..."
rm -rf /opt/phantomvault
rm -f /usr/local/bin/phantomvault
rm -f /usr/share/applications/phantomvault.desktop
rm -f /usr/share/pixmaps/phantomvault.png
rm -f /etc/systemd/system/phantomvault.service

print_status "Reloading systemd..."
systemctl daemon-reload

print_status "Removing package manager entries..."
# Remove from dpkg if installed via DEB
if dpkg -l | grep -q phantomvault; then
    dpkg --remove phantomvault 2>/dev/null || true
fi

# Remove from rpm if installed via RPM
if rpm -qa | grep -q phantomvault; then
    rpm -e phantomvault 2>/dev/null || true
fi

print_success "PhantomVault has been successfully uninstalled"
print_warning "User data preserved in ~/.phantomvault/ directories"
print_status "To completely remove all data, manually delete ~/.phantomvault/ directories"

EOF
chmod +x installer/build/linux/deb/opt/phantomvault/bin/uninstall.sh

# Create DEB control file
cat > installer/build/linux/deb/DEBIAN/control << EOF
Package: $PACKAGE_NAME
Version: $VERSION
Section: utils
Priority: optional
Architecture: amd64
Depends: libc6, libssl3, libstdc++6, systemd
Maintainer: $MAINTAINER
Description: $DESCRIPTION
 PhantomVault provides invisible folder security with AES-256 encryption,
 profile-based management, and cross-platform compatibility. Features include:
 .
  * Military-grade AES-256 encryption
  * Profile-based folder management
  * Invisible keyboard sequence detection (Ctrl+Alt+V)
  * System tray integration
  * Cross-platform support (Linux, macOS, Windows)
  * Complete folder obfuscation and trace removal
Homepage: https://github.com/ishaq2321/phantomVault
EOF

# Create post-installation script
cat > installer/build/linux/deb/DEBIAN/postinst << 'EOF'
#!/bin/bash
set -e

# Create data directory
mkdir -p /var/lib/phantomvault
chmod 700 /var/lib/phantomvault

# Enable and start service
systemctl daemon-reload
systemctl enable phantomvault
systemctl start phantomvault

# Update desktop database
update-desktop-database /usr/share/applications/ 2>/dev/null || true

# Register protocol handler
if command -v xdg-mime >/dev/null 2>&1; then
    xdg-mime default phantomvault.desktop x-scheme-handler/phantomvault 2>/dev/null || true
fi

echo "PhantomVault has been installed successfully!"
echo "You can start it from the applications menu or run 'phantomvault --gui'"
echo "Service is running in the background for global hotkey detection (Ctrl+Alt+V)"

EOF
chmod +x installer/build/linux/deb/DEBIAN/postinst

# Create pre-removal script
cat > installer/build/linux/deb/DEBIAN/prerm << 'EOF'
#!/bin/bash
set -e

# Stop service before removal
systemctl stop phantomvault 2>/dev/null || true
systemctl disable phantomvault 2>/dev/null || true

EOF
chmod +x installer/build/linux/deb/DEBIAN/prerm

# Create post-removal script
cat > installer/build/linux/deb/DEBIAN/postrm << 'EOF'
#!/bin/bash
set -e

# Clean up systemd
systemctl daemon-reload

# Update desktop database
update-desktop-database /usr/share/applications/ 2>/dev/null || true

# Note: User data is preserved in ~/.phantomvault/

EOF
chmod +x installer/build/linux/deb/DEBIAN/postrm

# Build DEB package
print_status "Building DEB package..."
cd installer/build/linux
dpkg-deb --build deb "${PACKAGE_NAME}_${VERSION}_amd64.deb"

if [ $? -eq 0 ]; then
    print_success "DEB package created: installer/build/linux/${PACKAGE_NAME}_${VERSION}_amd64.deb"
else
    print_error "Failed to create DEB package"
    exit 1
fi

# Create RPM package (if rpmbuild is available)
if command -v rpmbuild >/dev/null 2>&1; then
    print_status "Building RPM package..."
    
    # Create RPM spec file
    mkdir -p rpm/SPECS rpm/SOURCES rpm/BUILD rpm/RPMS rpm/SRPMS
    
    cat > rpm/SPECS/phantomvault.spec << EOF
Name:           $PACKAGE_NAME
Version:        $VERSION
Release:        1%{?dist}
Summary:        $DESCRIPTION
License:        MIT
URL:            https://github.com/ishaq2321/phantomVault
Source0:        %{name}-%{version}.tar.gz
BuildArch:      x86_64

Requires:       glibc, openssl-libs, libstdc++, systemd

%description
PhantomVault provides invisible folder security with AES-256 encryption,
profile-based management, and cross-platform compatibility.

%prep
%setup -q

%build
# Nothing to build - pre-compiled binaries

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/opt/phantomvault/{bin,gui,docs}
mkdir -p %{buildroot}/usr/share/{applications,pixmaps}
mkdir -p %{buildroot}/etc/systemd/system
mkdir -p %{buildroot}/usr/local/bin

# Copy files from DEB structure
cp -r ../deb/opt/phantomvault/* %{buildroot}/opt/phantomvault/
cp ../deb/usr/share/applications/phantomvault.desktop %{buildroot}/usr/share/applications/
cp ../deb/usr/share/pixmaps/phantomvault.png %{buildroot}/usr/share/pixmaps/
cp ../deb/etc/systemd/system/phantomvault.service %{buildroot}/etc/systemd/system/
cp ../deb/usr/local/bin/phantomvault %{buildroot}/usr/local/bin/

%files
/opt/phantomvault/
/usr/share/applications/phantomvault.desktop
/usr/share/pixmaps/phantomvault.png
/etc/systemd/system/phantomvault.service
/usr/local/bin/phantomvault

%post
mkdir -p /var/lib/phantomvault
chmod 700 /var/lib/phantomvault
systemctl daemon-reload
systemctl enable phantomvault
systemctl start phantomvault
update-desktop-database /usr/share/applications/ 2>/dev/null || true

%preun
systemctl stop phantomvault 2>/dev/null || true
systemctl disable phantomvault 2>/dev/null || true

%postun
systemctl daemon-reload
update-desktop-database /usr/share/applications/ 2>/dev/null || true

%changelog
* $(date '+%a %b %d %Y') PhantomVault Team <team@phantomvault.dev> - $VERSION-1
- Initial RPM package release

EOF

    # Create source tarball (simplified)
    tar -czf rpm/SOURCES/${PACKAGE_NAME}-${VERSION}.tar.gz -C ../deb opt usr etc
    
    # Build RPM
    rpmbuild --define "_topdir $(pwd)/rpm" -ba rpm/SPECS/phantomvault.spec
    
    if [ $? -eq 0 ]; then
        cp rpm/RPMS/x86_64/${PACKAGE_NAME}-${VERSION}-1.*.rpm .
        print_success "RPM package created: installer/build/linux/${PACKAGE_NAME}-${VERSION}-1.*.rpm"
    else
        print_warning "RPM build failed, but DEB package is available"
    fi
else
    print_warning "rpmbuild not found, skipping RPM package creation"
fi

cd ../../..

print_success "Linux installer packages created successfully!"
print_status "DEB package: installer/build/linux/${PACKAGE_NAME}_${VERSION}_amd64.deb"
print_status "Install with: sudo dpkg -i installer/build/linux/${PACKAGE_NAME}_${VERSION}_amd64.deb"
print_status "Uninstall with: sudo /opt/phantomvault/bin/uninstall.sh"