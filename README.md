# PhantomVault

**Invisible Folder Security with Profile-Based Management**

PhantomVault is a comprehensive folder security application that provides invisible folder locking, profile-based management, and keyboard sequence detection for seamless folder access. The system ensures complete folder security by hiding and encrypting folders while maintaining zero traces of their original existence.

## 🔒 Key Features

- **Invisible Folder Security**: Folders are completely hidden and encrypted with no traces left behind
- **Profile-Based Management**: Multiple profiles with separate master keys and recovery keys
- **Keyboard Sequence Detection**: Invisible unlock via Ctrl+Alt+V with pattern matching
- **Cross-Platform Support**: Linux, macOS, and Windows with platform-specific optimizations
- **Beautiful Modern GUI**: Dashboard, analytics, and settings with light/dark themes
- **Ultra-Lightweight**: < 10MB RAM usage with minimal battery impact
- **Secure Backups**: Multiple backup layers with sudo-protected storage

## 🚀 Quick Start

### Installation

Download the installer for your platform:
- **Linux**: `phantomvault-linux.deb` or `phantomvault-linux.rpm`
- **macOS**: `phantomvault-macos.dmg`
- **Windows**: `phantomvault-windows.msi`

### Usage

1. **Install**: Run the installer - service starts automatically
2. **Launch**: Click desktop icon or run `phantomvault` in terminal
3. **Create Profile**: First run requires admin privileges to create profiles
4. **Add Folders**: Select profile, authenticate, and add folders to secure
5. **Invisible Unlock**: Press Ctrl+Alt+V and type your password anywhere

## 🏗️ Development Status

**Current Status**: Under active development - building from ground up with modern architecture

### Architecture

- **Core Service**: Lightweight C++ service (< 10MB RAM)
- **GUI Application**: Modern Electron + React + TypeScript interface
- **Cross-Platform**: CMake build system with platform-specific optimizations
- **Security-First**: AES-256 encryption, secure key management, trace removal

## 📋 Development Roadmap

- [x] Project specification and design
- [ ] Core infrastructure and build system
- [ ] Profile management system
- [ ] Folder security and encryption
- [ ] Keyboard sequence detection
- [ ] Analytics engine
- [ ] Modern GUI interface
- [ ] Service integration
- [ ] Performance optimization
- [ ] Cross-platform installer

## 🛠️ Building from Source

### Prerequisites

- **C++17** compiler (GCC 8+, Clang 10+, MSVC 2019+)
- **CMake** 3.16+
- **Node.js** 18+ and npm
- **OpenSSL** development libraries

### Build Instructions

```bash
# Clone repository
git clone https://github.com/ishaq2321/phantomVault.git
cd phantomVault

# Build C++ service
cd core
mkdir build && cd build
cmake ..
make -j$(nproc)

# Build GUI application
cd ../../gui
npm install
npm run build

# Create installer
npm run dist
```

## 📖 Documentation

- [Requirements Specification](.kiro/specs/phantomvault-complete-system/requirements.md)
- [Design Document](.kiro/specs/phantomvault-complete-system/design.md)
- [Implementation Tasks](.kiro/specs/phantomvault-complete-system/tasks.md)

## 🔐 Security

PhantomVault is designed with security as the top priority:

- **AES-256 encryption** for all folder content
- **Bcrypt password hashing** with salt
- **Secure key derivation** (PBKDF2)
- **Complete trace removal** from original locations
- **Forensic resistance** and privacy protection

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🤝 Contributing

We welcome contributions! Please read our contributing guidelines and code of conduct.

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/ishaq2321/phantomVault/issues)
- **Discussions**: [GitHub Discussions](https://github.com/ishaq2321/phantomVault/discussions)
- **Security**: Report security issues privately to security@phantomvault.dev

---

**⚠️ Important**: This software is under active development. Do not use in production environments until stable release.