# 🔐 PhantomVault

**Invisible Folder Encryption with Global Hotkey Access - Desktop Application**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Linux-blue.svg)]()
[![Version](https://img.shields.io/badge/version-1.0.3-brightgreen.svg)](https://github.com/ishaq2321/phantomvault/releases/latest)
[![Release](https://img.shields.io/badge/release-ready-success.svg)]()

## 🚀 Overview

PhantomVault is a **desktop application** that provides invisible folder encryption with instant global hotkey access. Lock and unlock your folders anywhere, anytime with simple keyboard shortcuts - no visible interface needed! The application runs completely in the background, securing your sensitive folders with military-grade encryption.

### ✨ Key Features

- **🔐 Military-Grade Encryption**: AES-256-GCM encryption with PBKDF2 key derivation (100,000 iterations)
- **⚡ Global Hotkeys**: `Ctrl+Alt+V` for instant lock/unlock, `Ctrl+Alt+R` for recovery access
- **👻 Invisible Operation**: No black screens, popups, or interface disruption
- **🖥️ Desktop Application**: Install once, use from Applications menu or hotkeys
- **🔄 Smart Modes**: Temporary unlock (auto-locks) or permanent unlock (removes from vault)
- **🔑 Recovery System**: Emergency access with recovery keys when passwords are forgotten
- **👥 Multi-User Support**: Isolated vaults per user with secure permissions
- **🛡️ Auto-Start Service**: Background service starts automatically on login
- **📦 One-Click Install**: Professional installer handles everything automatically

## 🏗️ Architecture

PhantomVault uses a hybrid architecture combining a native C++ background service with an optional Electron GUI:

```
┌─────────────────────────────────────────────────────────────┐
│                    User Interaction                        │
│  Ctrl+Alt+V → T/Ppassword → Enter                          │
└─────────────────┬───────────────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────────────┐
│              Native C++ Service                             │
│  ┌─────────────┬─────────────┬─────────────┬─────────────┐  │
│  │   Hotkey    │   Input     │   Vault     │  Recovery   │  │
│  │  Manager    │  Overlay    │ Operations  │  Manager    │  │
│  └─────────────┴─────────────┴─────────────┴─────────────┘  │
└─────────────────┬───────────────────────────────────────────┘
                  │
┌─────────────────▼───────────────────────────────────────────┐
│           Encrypted Vault Storage                           │
│  ~/.phantom_vault_storage/{username}/                      │
│  ├── vaults/     (encrypted folders)                       │
│  ├── backups/    (automatic backups)                       │
│  └── metadata/   (JSON configuration)                      │
└─────────────────────────────────────────────────────────────┘
```

### Core Components

1. **BackgroundService**: Main service orchestrator with lifecycle management
2. **VaultMetadataManager**: JSON metadata compatibility with existing systems
3. **VaultEncryptionManager**: AES-256-GCM folder encryption/decryption
4. **VaultStorageManager**: File system operations with transaction support
5. **ServiceVaultManager**: High-level vault operations coordinator
6. **HotkeyManager**: Global hotkey registration and handling
7. **InputOverlay**: Invisible password capture system
8. **RecoveryManager**: Emergency recovery key operations

## 📥 Installation

### 🚀 Quick Install (Recommended)

**Download and install PhantomVault as a desktop application:**

1. **Download the latest release:**
   ```bash
   # Download from GitHub releases
   wget https://github.com/ishaq2321/phantomvault/releases/latest/download/phantomvault-v1.0.3-linux.tar.gz
   
   # Extract
   tar -xzf phantomvault-v1.0.3-linux.tar.gz
   cd phantomvault-v1.0.3-linux
   ```

2. **Install with one command:**
   ```bash
   sudo ./install-phantomvault.sh
   ```

3. **Launch PhantomVault:**
   - Open **Applications** → **PhantomVault**
   - Or run `phantomvault` in terminal
   - Or use **Ctrl+Alt+V** hotkey anywhere!

### 📋 System Requirements

- **Linux** with systemd (Ubuntu/Debian/Fedora/Arch Linux)
- **X11 or Wayland** display server
- **Root access** for installation (sudo)
- **Internet connection** for dependencies

> **✅ Latest Updates (v1.0.3):**
> - Fixed installer file copying logic and directory handling
> - Resolved installer compatibility with NodeSource Node.js installations
> - Enhanced dependency detection and path resolution
> - Improved error messages for better troubleshooting

### 🗑️ Uninstallation

```bash
sudo ./uninstall-phantomvault.sh
```

*Your vault data remains safe in `~/.phantom_vault_storage`*

## 🎯 Usage

### 🚀 First Time Setup

1. **Open PhantomVault** from Applications menu
2. **Create master password** - This encrypts all your folders
3. **Get recovery key** - Save this safely for emergency access
4. **Add folders** - Select folders you want to protect
5. **Done!** - Folders are now encrypted and hidden

### ⌨️ Global Hotkeys (Works Anywhere!)

#### 🔓 Lock/Unlock Folders (`Ctrl+Alt+V`)
1. Press **Ctrl+Alt+V** anywhere on your system
2. Enter password with mode:
   - **`Tmypassword`** - Temporary unlock (auto-locks when PC locks)
   - **`Pmypassword`** - Permanent unlock (removes from vault)
3. Press **Enter**
4. Folders instantly appear/disappear!

#### 🔑 Emergency Recovery (`Ctrl+Alt+R`)
1. Press **Ctrl+Alt+R**
2. Enter recovery key: **`XXXX-XXXX-XXXX-XXXX`**
3. Press **Enter**
4. Emergency access to all folders

### 🖥️ GUI Management

- **Add Folders**: Use the GUI to select and add new folders to protect
- **View Status**: See which folders are locked/unlocked
- **Change Passwords**: Update master password or folder-specific passwords
- **Manage Recovery**: Generate new recovery keys

### 🔧 Service Management

```bash
# Check if service is running
systemctl --user status phantom-vault.service

# View logs
journalctl --user -u phantom-vault.service -f

# Restart service
systemctl --user restart phantom-vault.service
```

## 📁 How It Works

### 🔒 Folder Protection Process

1. **Add Folder** → PhantomVault encrypts it with AES-256-GCM
2. **Hide Folder** → Moves encrypted data to secure vault storage
3. **Original Gone** → Folder disappears from original location
4. **Unlock Anytime** → Use hotkeys to restore folders instantly

### 🗂️ Vault Storage Structure

```
~/.phantom_vault_storage/{username}/
├── vaults/                    # Your encrypted folders
│   └── FolderName_vault_id/   # Individual encrypted folders
├── backups/                   # Automatic safety backups
├── metadata/                  # Configuration and profiles
└── logs/                      # Security audit logs
```

### 🔐 Security Features

- **AES-256-GCM Encryption** - Military-grade security
- **PBKDF2 Key Derivation** - 100,000 iterations against brute force
- **Secure Memory Handling** - Passwords cleared after use
- **Automatic Backups** - Data protection during operations
- **Recovery Keys** - Emergency access system
- **Audit Logging** - Security event tracking

## 🛠️ Development

### 🔨 Building from Source

If you want to build PhantomVault yourself:

```bash
# Clone repository
git clone https://github.com/ishaq2321/phantomvault.git
cd phantomvault

# Build everything
./build-all.sh

# Test the build
./test-installer.sh
```

### 🧪 Running Tests

```bash
# Build and run tests
cd core/build
make
./core_tests
```

### 📦 Creating Release Package

```bash
# Prepare release for distribution
./prepare-release.sh
```

## 🛡️ Security & Privacy

### 🔐 Encryption Standards

- **AES-256-GCM** - Military-grade encryption with authentication
- **PBKDF2** - 100,000 iterations against brute force attacks
- **Random Salts** - Unique 32-byte salt per password
- **Secure Memory** - Passwords cleared after use

### 🔒 Privacy Protection

- **Local Storage Only** - No cloud, no network, no tracking
- **User Isolation** - Each user has separate encrypted vaults
- **Process Concealment** - Service runs invisibly in background
- **Audit Logging** - Security events logged for monitoring

### 💡 Security Best Practices

- **Strong Passwords** - Use 12+ character master passwords
- **Safe Recovery Keys** - Store recovery keys offline securely
- **Regular Backups** - PhantomVault creates automatic backups
- **Monitor Logs** - Check service logs for suspicious activity

## 🤝 Contributing

We welcome contributions to PhantomVault!

### 🔧 Development Setup

```bash
git clone https://github.com/ishaq2321/phantomvault.git
cd phantomvault
./build-all.sh
```

### 📝 Areas for Contribution

- **Cross-Platform Support** - Windows and macOS versions
- **GUI Enhancements** - Improved user interface
- **Security Audits** - Code review and vulnerability testing
- **Documentation** - User guides and tutorials
- **Testing** - Additional test coverage
- **Performance** - Optimization and profiling

### 🐛 Bug Reports

Found a bug? Please open an issue with:
- System information (OS, version)
- Steps to reproduce
- Expected vs actual behavior
- Log files if available

## 📄 License

PhantomVault is released under the MIT License. See the [LICENSE](LICENSE) file for details.

```
MIT License

Copyright (c) 2025 PhantomVault Contributors

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
```

## 🆘 Troubleshooting

### 🔧 Common Issues

#### Installation Issues

**Node.js/npm conflicts:**
```bash
# If you see npm dependency conflicts, use the latest installer (v1.0.3+)
# The installer now handles NodeSource Node.js installations automatically
wget https://github.com/ishaq2321/phantomvault/releases/latest/download/phantomvault-v1.0.3-linux.tar.gz
```

**Source files not found:**
```bash
# Ensure you're running the installer from the extracted directory
cd phantomvault-v1.0.3-linux
sudo ./install-phantomvault.sh
```

#### Service Not Starting
```bash
# Check service status
systemctl --user status phantom-vault.service

# View logs
journalctl --user -u phantom-vault.service -f

# Restart service
systemctl --user restart phantom-vault.service
```

#### Hotkeys Not Working
```bash
# Verify service is running
ps aux | grep phantom_vault_service

# Check display environment
echo $DISPLAY

# Test manually
/opt/phantomvault/core/build/phantom_vault_service
```

#### Can't Access Folders
```bash
# Check vault permissions
ls -la ~/.phantom_vault_storage/

# Verify encryption
openssl version
```

### 📞 Getting Help

- **GitHub Issues** - Bug reports and feature requests
- **Documentation** - Check this README for detailed info
- **Logs** - Service logs available via `journalctl`
- **Security Issues** - Report privately to ishaq2321@proton.me

## 🔮 Roadmap

### 🚀 Upcoming Features

- **Windows & macOS Support** - Cross-platform desktop applications
- **Mobile Companion Apps** - Android/iOS remote management
- **Cloud Backup Integration** - Secure encrypted cloud sync
- **Hardware Security Keys** - YubiKey and FIDO2 support
- **Network Vault Sharing** - Secure folder sharing between devices
- **Advanced GUI Features** - Enhanced management interface

### 📈 Version History

- **v1.0.3** - Fixed installer file copying logic (current)
- **v1.0.2** - Fixed installer path detection and Node.js conflicts
- **v1.0.1** - Fixed Node.js/npm installation conflicts
- **v1.0.0** - Desktop application with installer
- **v0.9.0** - Native C++ service architecture
- **v0.8.0** - Multi-user support and recovery system
- **v0.7.0** - Initial release with core functionality

## 📄 License

PhantomVault is released under the **MIT License**. See [LICENSE](LICENSE) file for details.

---

## 🎉 Ready to Secure Your Folders?

**[📥 Download PhantomVault](https://github.com/ishaq2321/phantomvault/releases/latest)**

**PhantomVault** - *Invisible Security, Instant Access* 👻🔐

*Protect your sensitive folders with military-grade encryption and global hotkey access!*