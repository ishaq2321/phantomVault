# PhantomVault

**🚀 PRODUCTION READY - UNIFIED ARCHITECTURE COMPLETE 🚀**

**Invisible Folder Security with Profile-Based Management**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20%7C%20Windows-blue)](https://github.com/ishaq2321/phantomVault)
[![Security](https://img.shields.io/badge/Encryption-AES--256-green)](https://github.com/ishaq2321/phantomVault)
[![Memory](https://img.shields.io/badge/RAM%20Usage-%3C%2010MB-brightgreen)](https://github.com/ishaq2321/phantomVault)

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

🌐 **Cross-Platform Excellence**
- Native support for Linux, macOS, and Windows
- Professional installers (DEB, DMG, MSI, AppImage)
- Consistent experience across all platforms

## 🚀 Quick Start

### Installation

**Linux (One Command Install)**
```bash
wget -qO- https://github.com/ishaq2321/phantomVault/releases/download/v1.0.0/phantomvault-linux-installer.sh | sudo bash
```

**macOS (One Command Install)**
```bash
curl -fsSL https://github.com/ishaq2321/phantomVault/releases/download/v1.0.0/phantomvault-macos-installer.sh | bash
```

**Windows**
```powershell
# Download and run as Administrator:
# https://github.com/ishaq2321/phantomVault/releases/download/v1.0.0/phantomvault-windows-installer.exe
```

**Manual Installation**
```bash
# Download installer for your platform
wget https://github.com/ishaq2321/phantomVault/releases/download/v1.0.0/phantomvault-linux-installer.sh
chmod +x phantomvault-linux-installer.sh
sudo ./phantomvault-linux-installer.sh
```

### First Use

1. **Create Profile**: Launch PhantomVault → "Create New Profile"
2. **Set Master Password**: Choose a strong password (12+ characters)
3. **Save Recovery Key**: Write down and store safely
4. **Add Folders**: Select folders to protect with encryption
5. **Access Anytime**: Press `Ctrl+Alt+V` and type your password

## 🏗️ Architecture

PhantomVault uses a modern, secure architecture:

```
┌─────────────────┐    HTTP/JSON    ┌──────────────────┐
│   Electron GUI  │ ←──────────────→ │   C++ Service    │
│                 │   (localhost)    │                  │
│ • React + MUI   │                  │ • Profile Mgmt   │
│ • TypeScript    │                  │ • Encryption     │
│ • Modern UI     │                  │ • Folder Ops     │
└─────────────────┘                  │ • Analytics      │
                                     │ • Performance    │
                                     └──────────────────┘
```

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
- 🔒 **Security Issues**: Email security@phantomvault.dev

**Community:**
- ⭐ Star this repository if you find it useful
- 🍴 Fork and contribute to the project
- 📢 Share with others who need folder security

## 🏆 Recognition

PhantomVault has been designed with security best practices and has undergone comprehensive security auditing. It implements industry-standard encryption and follows OWASP security guidelines.

---

<div align="center">

**PhantomVault** - *Your files, invisible until you need them.*

[📥 Download v1.0.0](https://github.com/ishaq2321/phantomVault/releases/tag/v1.0.0) • [📖 Documentation](docs/) • [🐛 Report Issues](https://github.com/ishaq2321/phantomVault/issues)

Made with ❤️ for privacy and security

</div>