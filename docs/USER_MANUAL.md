# PhantomVault User Manual

## Welcome to PhantomVault

PhantomVault is an invisible folder security application that provides military-grade encryption and profile-based management for your sensitive files. With PhantomVault, you can hide and encrypt folders with AES-256 encryption, making them completely invisible until you need them.

## Table of Contents

1. [Getting Started](#getting-started)
2. [Installation](#installation)
3. [Creating Your First Profile](#creating-your-first-profile)
4. [Managing Folders](#managing-folders)
5. [Security Features](#security-features)
6. [Analytics and Monitoring](#analytics-and-monitoring)
7. [Settings and Configuration](#settings-and-configuration)
8. [Troubleshooting](#troubleshooting)
9. [FAQ](#faq)

## Getting Started

### What is PhantomVault?

PhantomVault is a security application that:
- **Hides folders completely** from your file system
- **Encrypts folder contents** with AES-256 encryption
- **Manages multiple profiles** for different security contexts
- **Detects invisible keyboard sequences** for quick access
- **Provides secure backups** with multiple recovery options
- **Works across platforms** (Linux, macOS, Windows)

### Key Features

- 🔒 **Military-Grade Encryption**: AES-256 encryption with secure key derivation
- 👤 **Profile-Based Security**: Multiple isolated security profiles
- 🫥 **Complete Invisibility**: Folders disappear completely from file system
- ⌨️ **Invisible Access**: Ctrl+Alt+V hotkey for quick folder access
- 📊 **Usage Analytics**: Track access patterns and security events
- 🔄 **Secure Backups**: Multiple backup layers with recovery keys
- 🌐 **Cross-Platform**: Works on Linux, macOS, and Windows

## Installation

### System Requirements

**Minimum Requirements:**
- **RAM**: 512MB available memory
- **Storage**: 100MB free disk space
- **OS**: Linux (Ubuntu 18.04+), macOS (10.15+), Windows (10+)

**Recommended Requirements:**
- **RAM**: 1GB available memory
- **Storage**: 500MB free disk space
- **Admin Access**: Required for initial installation

### Linux Installation

1. **Download** the PhantomVault installer package:
   ```bash
   # For Debian/Ubuntu
   wget https://github.com/ishaq2321/phantomVault/releases/latest/phantomvault_1.0.0_amd64.deb
   
   # For AppImage (universal)
   wget https://github.com/ishaq2321/phantomVault/releases/latest/PhantomVault-1.0.0.AppImage
   ```

2. **Install** the package:
   ```bash
   # Debian/Ubuntu
   sudo dpkg -i phantomvault_1.0.0_amd64.deb
   sudo apt-get install -f  # Fix dependencies if needed
   
   # AppImage
   chmod +x PhantomVault-1.0.0.AppImage
   ./PhantomVault-1.0.0.AppImage
   ```

3. **Launch** PhantomVault from your applications menu or run:
   ```bash
   phantomvault
   ```

### macOS Installation

1. **Download** the PhantomVault DMG file
2. **Open** the DMG and drag PhantomVault to Applications
3. **Launch** PhantomVault from Applications folder
4. **Grant permissions** when prompted (Accessibility, Input Monitoring)

### Windows Installation

1. **Download** the PhantomVault installer (EXE)
2. **Run** the installer as Administrator
3. **Follow** the installation wizard
4. **Launch** PhantomVault from Start Menu or Desktop

## Creating Your First Profile

### What are Profiles?

Profiles in PhantomVault are isolated security contexts that allow you to:
- Separate personal and work folders
- Use different master passwords
- Maintain independent security settings
- Track usage separately

### Creating a Profile

1. **Launch PhantomVault** and click "Create New Profile"
2. **Enter Profile Details**:
   - **Profile Name**: Choose a descriptive name (e.g., "Personal", "Work")
   - **Master Password**: Create a strong password (12+ characters recommended)
   - **Confirm Password**: Re-enter your master password
3. **Save Recovery Key**: Write down the recovery key and store it safely
4. **Click "Create Profile"** to complete setup

### Profile Security Tips

- ✅ **Use strong passwords**: Mix letters, numbers, and symbols
- ✅ **Keep recovery keys safe**: Store in a secure location
- ✅ **Use different passwords**: Don't reuse passwords from other services
- ✅ **Regular backups**: Export profile data periodically

## Managing Folders

### Adding Folders to Protection

1. **Select your profile** and enter your master password
2. **Click "Add Folder"** in the dashboard
3. **Browse and select** the folder you want to protect
4. **Confirm the operation** - the folder will be encrypted and hidden

### Understanding Folder States

- **🔒 Locked**: Folder is encrypted and completely hidden
- **🔓 Temporarily Unlocked**: Folder is accessible but will auto-lock
- **🔐 Permanently Unlocked**: Folder is restored and removed from protection

### Unlocking Folders

#### Method 1: Invisible Keyboard Sequence
1. **Press Ctrl+Alt+V** anywhere on your system
2. **Type your master password** within 10 seconds
3. **Folders unlock automatically** for temporary access

#### Method 2: GUI Interface
1. **Open PhantomVault** application
2. **Select your profile** and authenticate
3. **Choose unlock option**:
   - **Temporary**: Folders unlock until system lock/restart
   - **Permanent**: Folders restored permanently (removes protection)

### Folder Operations

#### Temporary Unlock
- Folders become accessible in their original locations
- Auto-locks when you lock your screen or restart
- Maintains encryption and protection

#### Permanent Unlock
- Folders are restored to original locations
- Removes all encryption and protection
- Cannot be undone - use with caution

#### Lock All Temporary Folders
- Immediately locks all temporarily unlocked folders
- Useful for quick security when leaving computer

## Security Features

### Encryption Details

- **Algorithm**: AES-256-CBC with PKCS7 padding
- **Key Derivation**: PBKDF2 with SHA-256, 100,000 iterations
- **IV Generation**: Cryptographically secure random IV per file
- **Salt**: 32-byte random salt per password

### Trace Removal

PhantomVault completely removes traces of protected folders:
- **File System**: Folders disappear from file listings
- **Search Indexes**: Removed from system search databases
- **Recent Files**: Cleared from recent file lists
- **Thumbnails**: Thumbnail caches cleared
- **Registry**: Windows registry entries cleaned (Windows only)

### Backup System

Multiple backup layers protect your data:
- **Primary Vault**: Encrypted storage in ~/.phantomvault/vault
- **Secondary Backup**: Additional backup in ~/.phantomvault/backups
- **Recovery Keys**: Allow access even if master password is forgotten

### Security Monitoring

PhantomVault monitors security events:
- **Access Attempts**: Tracks successful and failed unlock attempts
- **Keyboard Detection**: Logs invisible sequence usage
- **Security Violations**: Alerts for suspicious activity
- **System Events**: Monitors for security-relevant system changes

## Analytics and Monitoring

### Usage Statistics

View detailed analytics about your PhantomVault usage:
- **Total Profiles**: Number of security profiles
- **Protected Folders**: Count of folders under protection
- **Access History**: Timeline of folder access events
- **Security Events**: Log of security-related activities

### Performance Monitoring

Monitor system performance impact:
- **Memory Usage**: Current and peak memory consumption
- **CPU Usage**: Processing overhead monitoring
- **Battery Impact**: Power consumption tracking (laptops)
- **Response Times**: System responsiveness metrics

### Security Alerts

Receive notifications for:
- **Failed Authentication**: Multiple incorrect password attempts
- **Unusual Access Patterns**: Unexpected usage patterns
- **System Security Events**: Security-relevant system changes
- **Maintenance Reminders**: Backup and security maintenance

## Settings and Configuration

### General Settings

- **Theme**: Choose between light and dark themes
- **Language**: Select your preferred language
- **Startup**: Configure auto-start behavior
- **Updates**: Enable/disable automatic updates

### Security Settings

- **Auto-Lock Timeout**: Set automatic lock timeout
- **Password Policy**: Configure password requirements
- **Recovery Options**: Manage recovery key settings
- **Audit Logging**: Enable/disable security audit logs

### Performance Settings

- **Memory Limit**: Set maximum memory usage
- **CPU Limit**: Configure CPU usage limits
- **Battery Mode**: Enable power-saving features
- **Background Activity**: Control background operations

### Platform-Specific Settings

#### Linux
- **Desktop Environment**: Optimize for your DE (GNOME, KDE, etc.)
- **File Manager**: Configure file manager integration
- **Permissions**: Manage required system permissions

#### macOS
- **Accessibility**: Configure accessibility permissions
- **Input Monitoring**: Manage input monitoring permissions
- **Gatekeeper**: Handle security warnings

#### Windows
- **UAC Integration**: Configure User Account Control
- **Windows Defender**: Manage antivirus exclusions
- **Registry Access**: Handle registry permission requirements

## Troubleshooting

### Common Issues

#### "Failed to Create Profile"
**Cause**: Insufficient permissions or disk space  
**Solution**: 
1. Run PhantomVault as administrator
2. Check available disk space (need 100MB minimum)
3. Verify write permissions to home directory

#### "Cannot Access Folders After Unlock"
**Cause**: File permissions or antivirus interference  
**Solution**:
1. Check folder permissions in original location
2. Add PhantomVault to antivirus exclusions
3. Restart PhantomVault service

#### "Keyboard Sequence Not Working"
**Cause**: Permission issues or conflicting hotkeys  
**Solution**:
1. Grant accessibility permissions (macOS/Linux)
2. Check for conflicting keyboard shortcuts
3. Try alternative unlock method through GUI

#### "High Memory Usage"
**Cause**: Large folders or memory leak  
**Solution**:
1. Check folder sizes in analytics
2. Restart PhantomVault service
3. Adjust memory limits in settings

### Getting Help

1. **Check Logs**: View application logs in settings
2. **Run Diagnostics**: Use built-in diagnostic tools
3. **Contact Support**: Visit GitHub issues page
4. **Community**: Join user community discussions

### Log Locations

- **Linux**: `~/.phantomvault/logs/`
- **macOS**: `~/Library/Logs/PhantomVault/`
- **Windows**: `%APPDATA%\PhantomVault\logs\`

## FAQ

### General Questions

**Q: Is PhantomVault free to use?**  
A: Yes, PhantomVault is open-source and completely free.

**Q: How secure is PhantomVault?**  
A: PhantomVault uses military-grade AES-256 encryption and has undergone security audits.

**Q: Can I use PhantomVault on multiple computers?**  
A: Yes, you can install PhantomVault on multiple devices. Profiles are device-specific.

**Q: What happens if I forget my master password?**  
A: Use your recovery key to regain access. Without the recovery key, data cannot be recovered.

### Technical Questions

**Q: How much system resources does PhantomVault use?**  
A: PhantomVault uses less than 10MB RAM and minimal CPU resources.

**Q: Can antivirus software interfere with PhantomVault?**  
A: Some antivirus software may flag folder operations. Add PhantomVault to exclusions.

**Q: Is my data safe if PhantomVault is uninstalled?**  
A: Yes, encrypted data remains in the vault directory and can be recovered.

**Q: Can I backup my PhantomVault profiles?**  
A: Yes, backup the ~/.phantomvault directory to preserve all profiles and data.

### Security Questions

**Q: Can someone access my folders if they have physical access to my computer?**  
A: No, folders remain encrypted and hidden even with physical access.

**Q: Does PhantomVault log my keystrokes?**  
A: Only during the 10-second window after Ctrl+Alt+V, and only to detect your password.

**Q: What happens to my data if PhantomVault has a bug?**  
A: Multiple backup layers protect your data. Recovery is always possible.

**Q: Can law enforcement access my protected folders?**  
A: PhantomVault uses strong encryption. Without your password/recovery key, access is not possible.

## Support and Community

### Getting Support

- **GitHub Issues**: Report bugs and request features
- **Documentation**: Comprehensive guides and tutorials
- **Community Forum**: User discussions and help
- **Email Support**: Direct support for critical issues

### Contributing

PhantomVault is open-source! Contribute by:
- Reporting bugs and issues
- Suggesting new features
- Contributing code improvements
- Helping with documentation
- Supporting other users

### Stay Updated

- **GitHub Releases**: Latest version announcements
- **Security Advisories**: Important security updates
- **Feature Updates**: New feature announcements
- **Community News**: Project updates and news

---

**Thank you for using PhantomVault!** Your privacy and security are our top priorities.

For the latest updates and support, visit: https://github.com/ishaq2321/phantomVault