# GitHub Release Creation Instructions

## Files to Upload to v1.1.0 Release

The following files are ready in `installer/build/` and need to be uploaded to GitHub:

### 1. DEB Package
- **File**: `installer/build/deb/phantomvault_1.1.0_amd64.deb`
- **Upload as**: `phantomvault_1.1.0_amd64.deb`

### 2. RPM Package  
- **File**: `installer/build/rpm/rpmbuild/RPMS/x86_64/phantomvault-1.1.0-1.x86_64.rpm`
- **Upload as**: `phantomvault-1.1.0-1.x86_64.rpm`

### 3. Service Binary
- **File**: `installer/build/phantomvault-service-linux`
- **Upload as**: `phantomvault-service-linux`

### 4. Standalone Installer
- **File**: `installer/build/phantomvault-linux-installer.sh`
- **Upload as**: `phantomvault-linux-installer.sh`

## Release Information

**Tag**: `v1.1.0`
**Title**: `PhantomVault v1.1.0 - Architecture Refactor Complete`

**Description**:
```markdown
# PhantomVault v1.1.0 - Architecture Refactor Complete

🚀 **MAJOR ARCHITECTURE IMPROVEMENTS**

This release completely fixes the CLI architecture issues and implements a pure client-server pattern.

## ✅ What's Fixed

- **CLI Port Conflicts Resolved**: No more "Failed to bind socket to port 9876" errors
- **Pure Client Architecture**: CLI commands are now lightweight clients that don't create service instances
- **Single Service Daemon**: Only one phantomvault process runs on port 9876
- **Clean IPC Communication**: All CLI/GUI communication via HTTP/JSON API
- **Organized Installation**: Structured /opt/phantomvault/ directory layout

## 📥 Installation

**Quick Install (Recommended)**:
```bash
curl -fsSL https://raw.githubusercontent.com/ishaq2321/phantomVault/main/installer/install-linux.sh | sudo bash
```

**Manual Install**:
```bash
# Download DEB package
wget https://github.com/ishaq2321/phantomVault/releases/download/v1.1.0/phantomvault_1.1.0_amd64.deb

# Install
sudo dpkg -i phantomvault_1.1.0_amd64.deb
sudo apt-get install -f

# Start service
sudo systemctl start phantomvault
sudo systemctl enable phantomvault
```

## 🧪 Testing

- All CLI commands now work without conflicts
- Multiple concurrent CLI commands supported
- Service management improved
- GUI integration maintained

## 🔧 Technical Changes

- Refactored CLI to pure IPC client implementation
- Fixed IPC client endpoints to use correct server routes
- Eliminated ServiceManager initialization in CLI methods
- Added missing IPC server endpoints
- Organized installer directory structure

## 📋 Files

- `phantomvault_1.1.0_amd64.deb` - Debian/Ubuntu package
- `phantomvault-1.1.0-1.x86_64.rpm` - RedHat/Fedora package  
- `phantomvault-service-linux` - Standalone service binary
- `phantomvault-linux-installer.sh` - Standalone installer script

## ⚠️ Important Notes

- **Linux Only**: Thoroughly tested on Ubuntu/Debian systems
- **Admin Required**: All operations require admin privileges (sudo)
- **Clean Install**: Uninstall previous versions before installing v1.1.0

## 🆘 Support

- **Issues**: [GitHub Issues](https://github.com/ishaq2321/phantomVault/issues)
- **Documentation**: [README.md](https://github.com/ishaq2321/phantomVault/blob/main/README.md)
- **Contact**: ishaq2321@proton.me

---

**Full Changelog**: https://github.com/ishaq2321/phantomVault/compare/v1.0.0...v1.1.0
```

## Steps to Create Release

1. Go to: https://github.com/ishaq2321/phantomVault/releases
2. Click "Create a new release"
3. Set tag: `v1.1.0`
4. Set title: `PhantomVault v1.1.0 - Architecture Refactor Complete`
5. Paste the description above
6. Upload the 4 files listed above
7. Check "Set as the latest release"
8. Click "Publish release"