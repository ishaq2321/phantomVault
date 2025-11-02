# PhantomVault

**⚠️ DEVELOPMENT VERSION - USE AT YOUR OWN RISK ⚠️**

**Invisible Folder Security with Profile-Based Management**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-Linux%20(Tested)%20%7C%20Windows%20(Untested)%20%7C%20macOS%20(Untested)-orange)](https://github.com/ishaq2321/phantomVault)
[![Security](https://img.shields.io/badge/Encryption-AES--256-green)](https://github.com/ishaq2321/phantomVault)
[![Status](https://img.shields.io/badge/Status-Continuous%20Development-yellow)](https://github.com/ishaq2321/phantomVault)

## ⚠️ Important Notice

**This software is in continuous development and has not been fully tested on Windows and Mac platforms. Use at your own risk.**

PhantomVault is a professional-grade security application that makes your sensitive folders completely invisible with military-grade encryption. Perfect for protecting confidential documents, personal files, and sensitive data across all major platforms.

## ✨ Key Features

🔒 **Military-Grade Security**
- AES-256-CBC encryption with secure key derivation
- Complete folder invisibility and trace removal
- Forensic-resistant data protection

👤 **Profile-Based Management**
- Multiple isolated security profiles
- Individual master passwords and recovery keys
- Separate analytics and access controls

⌨️ **Invisible Access**
- Ctrl+Alt+V hotkey for instant folder access
- 10-second invisible keyboard sequence detection
- Auto-lock on screen lock or system restart

📊 **Smart Monitoring**
- Real-time usage analytics and security events
- Performance monitoring with <10MB RAM usage
- Adaptive resource management and battery optimization

🌐 **Cross-Platform Development**
- ✅ Linux: Fully tested and supported
- ⚠️ Windows: Code complete but not yet tested
- ⚠️ macOS: Code complete but not yet tested
- Professional Linux installer (DEB package available)

## 🚀 Quick Start

### Installation

**Linux (Recommended - Tested Platform)**

**LATEST RELEASE v1.1.0 - Architecture Refactor Complete**
```bash
# Quick Install (Recommended)
curl -fsSL https://raw.githubusercontent.com/ishaq2321/phantomVault/main/installer/install-linux.sh | sudo bash

# Or Manual Install
wget https://github.com/ishaq2321/phantomVault/releases/download/v1.1.0/phantomvault_1.1.0_amd64.deb
sudo dpkg -i phantomvault_1.1.0_amd64.deb
sudo apt-get install -f

# Start the service
sudo systemctl start phantomvault
sudo systemctl enable phantomvault

# Test CLI (Architecture Fixed!)
phantomvault --cli status
phantomvault --cli profiles
phantomvault --help
```

**Windows (⚠️ Untested - Use at Your Own Risk)**
```powershell
# Build from source (Windows support not yet tested)
git clone https://github.com/ishaq2321/phantomVault.git
cd phantomvault
# Follow build instructions below
```

**macOS (⚠️ Untested - Use at Your Own Risk)**
```bash
# Build from source (macOS support not yet tested)
git clone https://github.com/ishaq2321/phantomVault.git
cd phantomvault
# Follow build instructions below
```

**Build from Source (All Platforms)**
```bash
# Clone the repository
git clone https://github.com/ishaq2321/phantomVault.git
cd phantomvault

# Build the project
./build.sh

# Create installer packages (Linux)
cd installer/scripts
./build-linux-installer.sh

# Create maintenance tools
./build-maintenance-tools.sh
```

### First Use

1. **Create Profile**: Launch PhantomVault → "Create New Profile"
2. **Set Master Password**: Choose a strong password (12+ characters)
3. **Save Recovery Key**: Write down and store safely
4. **Add Folders**: Select folders to protect with encryption
5. **Access Anytime**: Press `Ctrl+Alt+V` and type your password

### CLI Usage (NEW - Fixed in v1.0.0!)

```bash
# Check service status
phantomvault --cli status

# List profiles
phantomvault --cli profiles

# Lock profile folders
phantomvault --cli lock profile-name

# Service management
phantomvault --cli stop      # Graceful shutdown
phantomvault --cli restart   # Service restart

# Help and version
phantomvault --help
phantomvault --version
```

## 🏗️ Architecture - REFACTORED v1.1.0

PhantomVault uses a **Pure Client-Server Architecture** with complete CLI fix:

```
┌─────────────────┐    IPC/HTTP     ┌──────────────────┐
│   CLI Client    │ ←──────────────→ │   Service Daemon │
│                 │   (localhost)    │                  │
│ • Pure client   │                  │ • Single instance│
│ • No service    │                  │ • Port 9876      │
│ • IPC calls only│                  │ • All operations │
└─────────────────┘                  └──────────────────┘

┌─────────────────┐    IPC/HTTP     ┌──────────────────┐
│   Electron GUI  │ ←──────────────→ │   Service Daemon │
│                 │   (localhost)    │                  │
│ • React + MUI   │                  │ • Profile Mgmt   │
│ • TypeScript    │                  │ • Encryption     │
│ • Modern UI     │                  │ • Folder Ops     │
└─────────────────┘                  │ • Analytics      │
                                     │ • Performance    │
                                     └──────────────────┘
```

**ARCHITECTURE IMPROVEMENTS v1.1.0:**
- **✅ CLI Completely Fixed**: Zero port conflicts, pure client implementation
- **✅ Single Service Daemon**: One process handles all operations on port 9876
- **✅ Clean IPC Communication**: All CLI/GUI communication via HTTP/JSON API
- **✅ No Resource Conflicts**: Eliminated duplicate service instances
- **✅ Organized Installation**: Structured /opt/phantomvault/ directory layout
- **✅ Professional Installer**: Complete installation and uninstallation system

**Service Components:**
- **Profile Manager**: Secure authentication and profile isolation
- **Folder Security**: AES-256 encryption and complete trace removal
- **Keyboard Detector**: Invisible sequence detection (Ctrl+Alt+V)
- **Analytics Engine**: Usage tracking and security monitoring
- **Performance Monitor**: Resource optimization and battery management

## 🔐 Security Features

### Encryption & Protection
- **AES-256-CBC** encryption with PKCS7 padding
- **PBKDF2** key derivation (SHA-256, 100K iterations)
- **bcrypt** password hashing (cost factor 12)
- **Cryptographically secure** random IV and salt generation

### Trace Removal
- **Complete folder invisibility** from file system
- **Search index cleanup** (Windows, macOS, Linux)
- **Recent files removal** from system lists
- **Registry cleanup** (Windows) and metadata removal
- **Thumbnail cache clearing** across all platforms

### Access Control
- **Profile-based isolation** with individual master passwords
- **Recovery key system** for password recovery
- **Session management** with automatic timeouts
- **Audit logging** of all security events

## 📊 Performance

PhantomVault is designed for efficiency:

- **Memory Usage**: <10MB RAM (typically 6-8MB)
- **CPU Impact**: <5% average usage with adaptive limiting
- **Battery Optimized**: Power-aware scheduling and resource management
- **Startup Time**: <2 seconds to full functionality
- **Response Time**: Instant folder operations

## 🛠️ Development

### Building from Source

**Prerequisites:**
- C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.16+
- OpenSSL 3.0+
- Node.js 18+
- Platform-specific libraries (X11, Cocoa, Win32)

**Build Process:**
```bash
git clone https://github.com/ishaq2321/phantomVault.git
cd phantomVault

# Build everything (service + GUI)
./build.sh

# Development mode
cd gui && npm run dev
```

### Project Structure
```
phantomVault/
├── core/                 # C++ service
│   ├── src/             # Source files
│   ├── include/         # Header files
│   └── CMakeLists.txt   # Build configuration
├── gui/                 # Electron GUI
│   ├── src/             # React components
│   ├── electron/        # Electron main process
│   └── package.json     # Dependencies
├── installer/           # Installation scripts
├── docs/               # Documentation
└── build.sh           # Build script
```

## 📚 Documentation

- **[Quick Start Guide](docs/QUICK_START.md)** - Get started in 5 minutes
- **[User Manual](docs/USER_MANUAL.md)** - Complete user documentation
- **[Security Audit](docs/SECURITY_AUDIT.md)** - Security analysis and compliance
- **[API Documentation](docs/API.md)** - Service API reference
- **[Contributing Guide](CONTRIBUTING.md)** - Development guidelines

## 🤝 Contributing

We welcome contributions! Please read our [Contributing Guide](CONTRIBUTING.md) and [Code of Conduct](CODE_OF_CONDUCT.md).

**Ways to Contribute:**
- 🐛 Report bugs and issues
- 💡 Suggest new features
- 🔧 Submit code improvements
- 📖 Improve documentation
- 🌍 Help with translations

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🆘 Support

**Get Help:**
- 📖 **Documentation**: Check docs/ directory
- 🐛 **Bug Reports**: [GitHub Issues](https://github.com/ishaq2321/phantomVault/issues)
- 💬 **Discussions**: [GitHub Discussions](https://github.com/ishaq2321/phantomVault/discussions)
- 📧 **Contact**: ishaq2321@proton.me
- 📄 **License**: MIT
- 🤝 **Contributions**: Welcome!

**Community:**
- ⭐ Star this repository if you find it useful
- 🍴 Fork and contribute to the project
- 📢 Share with others who need folder security

## ⚠️ Important Disclaimers

### Development Status
- **Continuous Development**: This software is actively being developed and improved
- **Platform Testing**: Only Linux (Ubuntu/Debian) has been thoroughly tested
- **Windows & macOS**: Code is complete but has not been tested on these platforms
- **Use at Own Risk**: While designed with security in mind, use this software at your own risk

### Security Notice
- This software has not undergone professional security audit
- Always maintain backups of important files before using
- Test thoroughly in a safe environment before production use
- The developers are not responsible for any data loss or security issues

### Legal Compliance
- Users are responsible for compliance with local laws and regulations
- This software is designed for legitimate security purposes only
- The developers are not responsible for any misuse of this software

### Support Limitations
- Support is primarily provided for Linux platforms
- Windows and macOS support is experimental
- Community support is available through GitHub issues

---

<div align="center">

**PhantomVault** - *Your files, invisible until you need them.*

[📥 Download v1.1.0](https://github.com/ishaq2321/phantomVault/releases/tag/v1.1.0) • [📖 Documentation](docs/) • [🐛 Report Issues](https://github.com/ishaq2321/phantomVault/issues) • [📧 Contact](mailto:ishaq2321@proton.me)

Made with ❤️ for privacy and security

</div>