#!/bin/bash
# PhantomVault Linux Package Builder
# Creates DEB and RPM packages with systemd service integration

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
INSTALLER_DIR="$PROJECT_ROOT/installer"
BUILD_DIR="$PROJECT_ROOT/build"
PACKAGE_DIR="$BUILD_DIR/packages"
VERSION="1.0.0"
ARCHITECTURE="amd64"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() { echo -e "${BLUE}[PACKAGE]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check dependencies
check_dependencies() {
    print_status "Checking packaging dependencies..."
    
    local missing_deps=()
    
    # Check for DEB packaging tools
    if ! command -v dpkg-deb &> /dev/null; then
        missing_deps+=("dpkg-dev")
    fi
    
    # Check for RPM packaging tools
    if ! command -v rpmbuild &> /dev/null; then
        missing_deps+=("rpm-build")
    fi
    
    # Check for fakeroot
    if ! command -v fakeroot &> /dev/null; then
        missing_deps+=("fakeroot")
    fi
    
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        print_warning "Missing packaging dependencies: ${missing_deps[*]}"
        print_status "Installing dependencies..."
        
        if command -v apt-get &> /dev/null; then
            sudo apt-get update -qq
            sudo apt-get install -y -qq dpkg-dev rpm fakeroot
        elif command -v dnf &> /dev/null; then
            sudo dnf install -y rpm-build rpm-devel dpkg fakeroot
        elif command -v yum &> /dev/null; then
            sudo yum install -y rpm-build rpm-devel dpkg fakeroot
        else
            print_error "Cannot install packaging dependencies automatically"
            print_error "Please install: dpkg-dev, rpm-build, fakeroot"
            exit 1
        fi
        
        print_success "Dependencies installed"
    fi
}

# Prepare build environment
prepare_build_env() {
    print_status "Preparing build environment..."
    
    # Clean and create package directory
    rm -rf "$PACKAGE_DIR"
    mkdir -p "$PACKAGE_DIR"/{deb,rpm,staging}
    
    # Ensure service is built
    if [[ ! -f "$BUILD_DIR/phantomvault" ]]; then
        print_status "Building PhantomVault service..."
        cd "$PROJECT_ROOT"
        ./build.sh
    fi
    
    print_success "Build environment prepared"
}

# Create DEB package
create_deb_package() {
    print_status "Creating DEB package..."
    
    local deb_dir="$PACKAGE_DIR/deb"
    local staging_dir="$deb_dir/phantomvault_${VERSION}_${ARCHITECTURE}"
    
    # Create package structure
    mkdir -p "$staging_dir"/{DEBIAN,opt/phantomvault/{bin,share,var,logs},etc/systemd/system,usr/{share/applications,local/bin},etc/polkit-1/rules.d}
    
    # Copy application files
    cp "$BUILD_DIR/phantomvault" "$staging_dir/opt/phantomvault/bin/phantomvault-service"
    chmod +x "$staging_dir/opt/phantomvault/bin/phantomvault-service"
    
    # Create GUI wrapper
    cat > "$staging_dir/opt/phantomvault/bin/phantomvault-gui" << 'EOF'
#!/bin/bash
# PhantomVault GUI Launcher
INSTALL_DIR="/opt/phantomvault"
SERVICE_BIN="$INSTALL_DIR/bin/phantomvault-service"

# Check if service is running
if ! systemctl is-active --quiet phantomvault 2>/dev/null; then
    echo "Starting PhantomVault service..."
    if command -v pkexec &> /dev/null; then
        pkexec systemctl start phantomvault
    else
        echo "Please start the service manually: sudo systemctl start phantomvault"
        exit 1
    fi
fi

# Show status
echo "PhantomVault - Invisible Folder Security"
echo "========================================"
if systemctl is-active --quiet phantomvault; then
    echo "✅ Service is running"
    echo "🔒 Your folders are protected"
    echo ""
    echo "Usage:"
    echo "• Press Ctrl+Alt+V anywhere to access your folders"
    echo "• Run 'phantomvault --help' for more options"
else
    echo "❌ Service is not running"
    echo "Please check: systemctl status phantomvault"
fi

# Keep terminal open
if [[ -t 1 ]]; then
    echo ""
    echo "Press Enter to continue..."
    read -r
fi
EOF
    chmod +x "$staging_dir/opt/phantomvault/bin/phantomvault-gui"
    
    # Create application icon
    mkdir -p "$staging_dir/opt/phantomvault/share/pixmaps"
    cat > "$staging_dir/opt/phantomvault/share/pixmaps/phantomvault.svg" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<svg width="64" height="64" viewBox="0 0 64 64" xmlns="http://www.w3.org/2000/svg">
  <rect width="64" height="64" rx="8" fill="#2563eb"/>
  <rect x="12" y="20" width="40" height="24" rx="4" fill="none" stroke="#ffffff" stroke-width="2"/>
  <circle cx="32" cy="32" r="6" fill="none" stroke="#ffffff" stroke-width="2"/>
  <circle cx="32" cy="32" r="2" fill="#ffffff"/>
  <text x="32" y="52" text-anchor="middle" fill="#ffffff" font-family="Arial" font-size="8">PV</text>
</svg>
EOF
    
    # Create systemd service file
    cat > "$staging_dir/etc/systemd/system/phantomvault.service" << 'EOF'
[Unit]
Description=PhantomVault - Invisible Folder Security Service
Documentation=https://github.com/ishaq2321/phantomVault
After=network.target graphical-session.target
Wants=network.target

[Service]
Type=notify
NotifyAccess=main
User=root
Group=root
WorkingDirectory=/opt/phantomvault
ExecStart=/opt/phantomvault/bin/phantomvault-service --daemon --log-level INFO --port 9876
ExecReload=/bin/kill -HUP $MAINPID
Restart=on-failure
RestartSec=10
StartLimitInterval=300s
StartLimitBurst=5
TimeoutStartSec=60

# Logging
StandardOutput=append:/opt/phantomvault/logs/phantomvault.log
StandardError=append:/opt/phantomvault/logs/phantomvault-error.log

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
MemoryMax=100M
CPUQuota=20%

# Environment
Environment=PHANTOMVAULT_DATA_DIR=/home
Environment=PHANTOMVAULT_LOG_LEVEL=INFO

[Install]
WantedBy=multi-user.target
EOF
    
    # Create desktop entry
    cat > "$staging_dir/usr/share/applications/phantomvault.desktop" << 'EOF'
[Desktop Entry]
Version=1.0
Type=Application
Name=PhantomVault
Comment=Invisible Folder Security with Profile-Based Management
Exec=/opt/phantomvault/bin/phantomvault-gui
Icon=/opt/phantomvault/share/pixmaps/phantomvault.svg
Terminal=true
Categories=Security;Utility;FileManager;
Keywords=security;encryption;privacy;folders;vault;
StartupWMClass=PhantomVault
MimeType=application/x-phantomvault-profile;
StartupNotify=true
EOF
    
    # Create CLI tool
    cat > "$staging_dir/usr/local/bin/phantomvault" << 'EOF'
#!/bin/bash
# PhantomVault CLI interface
SERVICE_BIN="/opt/phantomvault/bin/phantomvault-service"

case "${1:-status}" in
    status)
        echo "PhantomVault - Invisible Folder Security"
        echo "========================================"
        if systemctl is-active --quiet phantomvault 2>/dev/null; then
            echo "Status: ✅ Running"
            echo "Memory: $(systemctl show phantomvault --property=MemoryCurrent --value | numfmt --to=iec-i --suffix=B 2>/dev/null || echo "Unknown")"
        else
            echo "Status: ❌ Not running"
        fi
        ;;
    --gui)
        exec /opt/phantomvault/bin/phantomvault-gui
        ;;
    --start)
        echo "Starting PhantomVault..."
        pkexec systemctl start phantomvault
        ;;
    --stop)
        echo "Stopping PhantomVault..."
        pkexec systemctl stop phantomvault
        ;;
    --restart)
        echo "Restarting PhantomVault..."
        pkexec systemctl restart phantomvault
        ;;
    --status)
        systemctl status phantomvault --no-pager
        ;;
    --logs)
        journalctl -u phantomvault -f --no-pager
        ;;
    --help|-h)
        echo "PhantomVault CLI"
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
        exec "$SERVICE_BIN" "$@"
        ;;
esac
EOF
    chmod +x "$staging_dir/usr/local/bin/phantomvault"
    
    # Create polkit rule
    cat > "$staging_dir/etc/polkit-1/rules.d/50-phantomvault.rules" << 'EOF'
// Allow users to manage PhantomVault service
polkit.addRule(function(action, subject) {
    if (action.id == "org.freedesktop.systemd1.manage-units" &&
        action.lookup("unit") == "phantomvault.service" &&
        subject.isInGroup("users")) {
        return polkit.Result.YES;
    }
});
EOF
    
    # Create DEBIAN control files
    cat > "$staging_dir/DEBIAN/control" << EOF
Package: phantomvault
Version: $VERSION
Section: utils
Priority: optional
Architecture: $ARCHITECTURE
Depends: libssl3, libx11-6, libxtst6, libgtk-3-0, systemd, polkitd
Maintainer: PhantomVault Team <support@phantomvault.com>
Description: Invisible Folder Security with Profile-Based Management
 PhantomVault provides military-grade folder security with invisible
 folder management, profile-based access control, and real-time
 monitoring. Features include:
 .
 • Invisible folder hiding with Ctrl+Alt+V access
 • Profile-based security management
 • Real-time integrity monitoring
 • Cross-platform compatibility
 • Systemd service integration
Homepage: https://github.com/ishaq2321/phantomVault
EOF
    
    # Create postinst script
    cat > "$staging_dir/DEBIAN/postinst" << 'EOF'
#!/bin/bash
set -e

# Create phantomvault user if it doesn't exist
if ! id phantomvault &>/dev/null; then
    useradd --system --no-create-home --shell /bin/false phantomvault
fi

# Set proper permissions
chown -R root:root /opt/phantomvault
chown -R phantomvault:phantomvault /opt/phantomvault/var /opt/phantomvault/logs
chmod 755 /opt/phantomvault/bin/*

# Reload systemd and enable service
systemctl daemon-reload
systemctl enable phantomvault

# Update desktop database
if command -v update-desktop-database &> /dev/null; then
    update-desktop-database /usr/share/applications
fi

# Reload polkit rules
if command -v systemctl &> /dev/null; then
    systemctl reload polkit 2>/dev/null || true
fi

echo "PhantomVault installed successfully!"
echo "Start with: sudo systemctl start phantomvault"
echo "Or use: phantomvault --help"
EOF
    chmod +x "$staging_dir/DEBIAN/postinst"
    
    # Create prerm script
    cat > "$staging_dir/DEBIAN/prerm" << 'EOF'
#!/bin/bash
set -e

# Stop and disable service
if systemctl is-active --quiet phantomvault 2>/dev/null; then
    systemctl stop phantomvault
fi
systemctl disable phantomvault 2>/dev/null || true
EOF
    chmod +x "$staging_dir/DEBIAN/prerm"
    
    # Create postrm script
    cat > "$staging_dir/DEBIAN/postrm" << 'EOF'
#!/bin/bash
set -e

if [[ "$1" == "purge" ]]; then
    # Remove user
    if id phantomvault &>/dev/null; then
        userdel phantomvault 2>/dev/null || true
    fi
    
    # Remove logs and data (but preserve user data)
    rm -rf /opt/phantomvault/logs
    rm -rf /opt/phantomvault/var
    
    # Update desktop database
    if command -v update-desktop-database &> /dev/null; then
        update-desktop-database /usr/share/applications
    fi
fi

systemctl daemon-reload
EOF
    chmod +x "$staging_dir/DEBIAN/postrm"
    
    # Build DEB package
    cd "$deb_dir"
    fakeroot dpkg-deb --build "phantomvault_${VERSION}_${ARCHITECTURE}"
    
    print_success "DEB package created: $deb_dir/phantomvault_${VERSION}_${ARCHITECTURE}.deb"
}

# Create RPM package
create_rpm_package() {
    # Check if rpmbuild is available
    if ! command -v rpmbuild &> /dev/null; then
        print_warning "rpmbuild not found - skipping RPM package creation"
        print_status "Install rpm-build to create RPM packages"
        return 0
    fi
    
    print_status "Creating RPM package..."
    
    local rpm_dir="$PACKAGE_DIR/rpm"
    local spec_file="$rpm_dir/SPECS/phantomvault.spec"
    
    # Create RPM build structure
    mkdir -p "$rpm_dir"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
    mkdir -p "$rpm_dir/SOURCES/phantomvault-$VERSION"
    
    # Copy files to SOURCES
    cp -r "$BUILD_DIR/phantomvault" "$rpm_dir/SOURCES/phantomvault-$VERSION/"
    
    # Create spec file
    cat > "$spec_file" << EOF
Name:           phantomvault
Version:        $VERSION
Release:        1%{?dist}
Summary:        Invisible Folder Security with Profile-Based Management
License:        MIT
URL:            https://github.com/ishaq2321/phantomVault
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  systemd
Requires:       openssl, libX11, libXtst, gtk3, systemd, polkit

%description
PhantomVault provides military-grade folder security with invisible
folder management, profile-based access control, and real-time
monitoring.

%prep
%setup -q

%build
# Nothing to build - pre-compiled binary

%install
rm -rf %{buildroot}

# Create directories
mkdir -p %{buildroot}/opt/phantomvault/{bin,share/pixmaps,var,logs}
mkdir -p %{buildroot}%{_unitdir}
mkdir -p %{buildroot}%{_datadir}/applications
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}/etc/polkit-1/rules.d

# Install binary
install -m 755 phantomvault %{buildroot}/opt/phantomvault/bin/phantomvault-service

# Install GUI wrapper
cat > %{buildroot}/opt/phantomvault/bin/phantomvault-gui << 'WRAPPER_EOF'
#!/bin/bash
# PhantomVault GUI Launcher
if ! systemctl is-active --quiet phantomvault 2>/dev/null; then
    echo "Starting PhantomVault service..."
    pkexec systemctl start phantomvault
fi

echo "PhantomVault - Invisible Folder Security"
echo "========================================"
if systemctl is-active --quiet phantomvault; then
    echo "✅ Service is running"
    echo "🔒 Your folders are protected"
else
    echo "❌ Service is not running"
fi
WRAPPER_EOF
chmod +x %{buildroot}/opt/phantomvault/bin/phantomvault-gui

# Install systemd service
cat > %{buildroot}%{_unitdir}/phantomvault.service << 'SERVICE_EOF'
[Unit]
Description=PhantomVault - Invisible Folder Security Service
After=network.target

[Service]
Type=notify
NotifyAccess=main
User=root
ExecStart=/opt/phantomvault/bin/phantomvault-service --daemon
Restart=on-failure
TimeoutStartSec=60

[Install]
WantedBy=multi-user.target
SERVICE_EOF

# Install desktop entry
cat > %{buildroot}%{_datadir}/applications/phantomvault.desktop << 'DESKTOP_EOF'
[Desktop Entry]
Name=PhantomVault
Comment=Invisible Folder Security
Exec=/opt/phantomvault/bin/phantomvault-gui
Icon=/opt/phantomvault/share/pixmaps/phantomvault.svg
Terminal=true
Categories=Security;Utility;
DESKTOP_EOF

# Install CLI tool
cat > %{buildroot}%{_bindir}/phantomvault << 'CLI_EOF'
#!/bin/bash
exec /opt/phantomvault/bin/phantomvault-service "\$@"
CLI_EOF
chmod +x %{buildroot}%{_bindir}/phantomvault

%files
/opt/phantomvault/
%{_unitdir}/phantomvault.service
%{_datadir}/applications/phantomvault.desktop
%{_bindir}/phantomvault

%post
systemctl daemon-reload
systemctl enable phantomvault
if ! id phantomvault &>/dev/null; then
    useradd --system --no-create-home --shell /bin/false phantomvault
fi
chown -R phantomvault:phantomvault /opt/phantomvault/var /opt/phantomvault/logs

%preun
if [ \$1 -eq 0 ]; then
    systemctl stop phantomvault 2>/dev/null || true
    systemctl disable phantomvault 2>/dev/null || true
fi

%postun
systemctl daemon-reload
if [ \$1 -eq 0 ]; then
    if id phantomvault &>/dev/null; then
        userdel phantomvault 2>/dev/null || true
    fi
fi

%changelog
* $(date +'%a %b %d %Y') PhantomVault Team <support@phantomvault.com> - $VERSION-1
- Initial RPM package
EOF
    
    # Create tarball
    cd "$rpm_dir/SOURCES"
    tar -czf "phantomvault-$VERSION.tar.gz" "phantomvault-$VERSION"
    
    # Build RPM
    cd "$rpm_dir"
    if rpmbuild --define "_topdir $rpm_dir" -ba SPECS/phantomvault.spec 2>/dev/null; then
        print_success "RPM package created: $rpm_dir/RPMS/*/phantomvault-$VERSION-1.*.rpm"
    else
        print_warning "RPM build failed - likely missing build dependencies"
        print_status "On RHEL/Fedora systems, install: systemd-rpm-macros"
        print_status "RPM spec file created at: $spec_file"
    fi
}

# Create AppImage (universal Linux package)
create_appimage() {
    print_status "Creating AppImage..."
    
    local appimage_dir="$PACKAGE_DIR/appimage"
    local appdir="$appimage_dir/PhantomVault.AppDir"
    
    # Download AppImageTool if not present
    if [[ ! -f "$appimage_dir/appimagetool" ]]; then
        print_status "Downloading AppImageTool..."
        mkdir -p "$appimage_dir"
        wget -q -O "$appimage_dir/appimagetool" \
            "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
        chmod +x "$appimage_dir/appimagetool"
    fi
    
    # Create AppDir structure
    mkdir -p "$appdir"/{usr/bin,usr/share/{applications,pixmaps}}
    
    # Copy application
    cp "$BUILD_DIR/phantomvault" "$appdir/usr/bin/phantomvault-service"
    
    # Create AppRun
    cat > "$appdir/AppRun" << 'EOF'
#!/bin/bash
# PhantomVault AppImage launcher
HERE="$(dirname "$(readlink -f "${0}")")"
export PATH="${HERE}/usr/bin:${PATH}"

# Check if service is running
if ! pgrep -f phantomvault-service > /dev/null; then
    echo "Starting PhantomVault service..."
    "${HERE}/usr/bin/phantomvault-service" --daemon &
    sleep 2
fi

echo "PhantomVault - Portable Folder Security"
echo "======================================"
if pgrep -f phantomvault-service > /dev/null; then
    echo "✅ Service is running"
    echo "🔒 Your folders are protected"
    echo ""
    echo "Usage:"
    echo "• Press Ctrl+Alt+V to access folders"
    echo "• This AppImage is portable - no installation needed"
else
    echo "❌ Failed to start service"
fi

# Keep terminal open
if [[ -t 1 ]]; then
    echo ""
    echo "Press Enter to continue..."
    read -r
fi
EOF
    chmod +x "$appdir/AppRun"
    
    # Create desktop entry
    cat > "$appdir/phantomvault.desktop" << 'EOF'
[Desktop Entry]
Type=Application
Name=PhantomVault
Comment=Invisible Folder Security (Portable)
Exec=AppRun
Icon=phantomvault
Terminal=true
Categories=Security;Utility;System;
EOF
    
    # Create icon (copy from previous)
    cp "$PACKAGE_DIR/deb/phantomvault_${VERSION}_${ARCHITECTURE}/opt/phantomvault/share/pixmaps/phantomvault.svg" \
       "$appdir/phantomvault.svg"
    
    # Copy desktop entry to standard location
    cp "$appdir/phantomvault.desktop" "$appdir/usr/share/applications/"
    cp "$appdir/phantomvault.svg" "$appdir/usr/share/pixmaps/"
    
    # Build AppImage
    cd "$appimage_dir"
    ./appimagetool "$appdir" "PhantomVault-$VERSION-x86_64.AppImage"
    
    print_success "AppImage created: $appimage_dir/PhantomVault-$VERSION-x86_64.AppImage"
}

# Main function
main() {
    print_status "Building PhantomVault Linux packages..."
    
    check_dependencies
    prepare_build_env
    
    # Create packages
    create_deb_package
    create_rpm_package
    create_appimage
    
    print_success "All Linux packages created successfully!"
    print_status "Packages available in: $PACKAGE_DIR"
    
    # List created packages
    echo ""
    echo "Created packages:"
    find "$PACKAGE_DIR" -name "*.deb" -o -name "*.rpm" -o -name "*.AppImage" | while read -r package; do
        echo "  • $(basename "$package")"
    done
}

# Run main function
main "$@"