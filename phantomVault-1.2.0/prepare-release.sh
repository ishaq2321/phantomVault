#!/bin/bash

# PhantomVault Release Preparation Script
# Creates a distributable package for GitHub releases

VERSION="1.2.0"
RELEASE_NAME="phantomvault-v${VERSION}-linux"
RELEASE_DIR="releases/${RELEASE_NAME}"

echo "📦 PhantomVault Release Preparation"
echo "=================================="
echo "Version: $VERSION"
echo "Package: $RELEASE_NAME"
echo ""

# Create release directory
mkdir -p "$RELEASE_DIR"

echo "📁 Copying files to release package..."

# Copy essential files
cp install-phantomvault.sh "$RELEASE_DIR/"
cp uninstall-phantomvault.sh "$RELEASE_DIR/"
cp phantomvault-app.sh "$RELEASE_DIR/"
cp INSTALLER_README.md "$RELEASE_DIR/README.md"
cp README.md "$RELEASE_DIR/PROJECT_README.md"

# Copy source code
cp -r core "$RELEASE_DIR/"
cp -r ui "$RELEASE_DIR/"
cp -r systemd "$RELEASE_DIR/"
cp -r assets "$RELEASE_DIR/"

# Remove build artifacts and node_modules to reduce size
rm -rf "$RELEASE_DIR/core/build"
rm -rf "$RELEASE_DIR/ui/node_modules"
rm -rf "$RELEASE_DIR/ui/dist"
rm -rf "$RELEASE_DIR/ui/dist-electron"

# Create installation instructions
cat > "$RELEASE_DIR/INSTALL.txt" << 'EOF'
PhantomVault v1.0.0 - Installation Instructions
===============================================

QUICK INSTALL:
1. Extract this package
2. Open terminal in the extracted folder
3. Run: sudo ./install-phantomvault.sh
4. Follow the prompts
5. Open PhantomVault from Applications menu

REQUIREMENTS:
- Linux with systemd
- Root access (sudo)
- Internet connection for dependencies

SUPPORTED SYSTEMS:
- Ubuntu/Debian (apt)
- Fedora/RHEL (dnf)
- Arch Linux (pacman)

For detailed instructions, see README.md

UNINSTALL:
sudo ./uninstall-phantomvault.sh
EOF

# Create version info
cat > "$RELEASE_DIR/VERSION" << EOF
PhantomVault v${VERSION}
Build Date: $(date)
Platform: Linux
Architecture: x86_64
EOF

# Make scripts executable
chmod +x "$RELEASE_DIR"/*.sh

echo "✅ Files copied successfully"

echo ""
echo "📝 Creating checksums..."

# Create checksums
cd releases
find "$RELEASE_NAME" -type f -exec sha256sum {} \; > "${RELEASE_NAME}.sha256"

echo "✅ Checksums created: ${RELEASE_NAME}.sha256"

echo ""
echo "🗜️  Creating archive..."

# Create tar.gz archive
tar -czf "${RELEASE_NAME}.tar.gz" "$RELEASE_NAME"

# Create zip archive for Windows users
if command -v zip &> /dev/null; then
    zip -r "${RELEASE_NAME}.zip" "$RELEASE_NAME"
    echo "✅ Created: ${RELEASE_NAME}.zip"
fi

echo "✅ Created: ${RELEASE_NAME}.tar.gz"

# Calculate final sizes
TAR_SIZE=$(du -h "${RELEASE_NAME}.tar.gz" | cut -f1)
DIR_SIZE=$(du -sh "$RELEASE_NAME" | cut -f1)

cd ..

echo ""
echo "🎉 Release Package Ready!"
echo "========================"
echo ""
echo "📦 Package Details:"
echo "   Name: $RELEASE_NAME"
echo "   Version: $VERSION"
echo "   Directory Size: $DIR_SIZE"
echo "   Archive Size: $TAR_SIZE"
echo ""
echo "📁 Files created:"
echo "   releases/${RELEASE_NAME}/"
echo "   releases/${RELEASE_NAME}.tar.gz"
if [ -f "releases/${RELEASE_NAME}.zip" ]; then
echo "   releases/${RELEASE_NAME}.zip"
fi
echo "   releases/${RELEASE_NAME}.sha256"
echo ""
echo "🚀 Ready for GitHub Release!"
echo ""
echo "📋 GitHub Release Instructions:"
echo "1. Go to GitHub → Releases → Create new release"
echo "2. Tag: v${VERSION}"
echo "3. Title: PhantomVault v${VERSION} - Desktop Application"
echo "4. Upload: ${RELEASE_NAME}.tar.gz and ${RELEASE_NAME}.zip"
echo "5. Include: ${RELEASE_NAME}.sha256 for verification"
echo ""
echo "📝 Release Notes Template:"
echo "---"
echo "## PhantomVault v${VERSION} - Desktop Application"
echo ""
echo "### 🎉 New Features"
echo "- Complete desktop application with installer"
echo "- One-click installation from GitHub"
echo "- Desktop integration with Applications menu"
echo "- Auto-starting background service"
echo "- Fixed black screen issue completely"
echo ""
echo "### 📥 Installation"
echo "1. Download \`${RELEASE_NAME}.tar.gz\`"
echo "2. Extract: \`tar -xzf ${RELEASE_NAME}.tar.gz\`"
echo "3. Install: \`cd ${RELEASE_NAME} && sudo ./install-phantomvault.sh\`"
echo "4. Launch from Applications menu or run \`phantomvault\`"
echo ""
echo "### 🔧 System Requirements"
echo "- Linux (Ubuntu/Debian/Fedora/Arch)"
echo "- systemd"
echo "- Root access for installation"
echo ""
echo "### 🗑️ Uninstallation"
echo "\`sudo ./uninstall-phantomvault.sh\`"
echo "---"
echo ""