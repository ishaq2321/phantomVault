# PhantomVault Installer

This directory contains the installer configuration and scripts for PhantomVault across different platforms.

## Structure

- `scripts/` - Installation and setup scripts
- `assets/` - Installer assets (icons, images, etc.)
- `config/` - Platform-specific installer configurations
- `templates/` - Template files for service configuration

## Platform Support

### Linux
- **Debian/Ubuntu**: `.deb` packages
- **Red Hat/Fedora**: `.rpm` packages
- **AppImage**: Universal Linux application
- **Systemd service**: Automatic service installation

### macOS
- **DMG**: Disk image installer
- **LaunchDaemon**: Automatic service installation

### Windows
- **NSIS**: Traditional installer
- **MSI**: Windows Installer package
- **Windows Service**: Automatic service installation

## Features

- Automatic service installation and startup
- Desktop integration (icons, shortcuts, file associations)
- Terminal command registration (`phantomvault`)
- Uninstaller with complete cleanup
- Update mechanism support
- Digital signing (production builds)

## Development

The installer is built using `electron-builder` with platform-specific configurations defined in the main `package.json` file.

### Building Installers

```bash
# Build for current platform
npm run dist

# Build for specific platforms
npm run dist:linux
npm run dist:mac
npm run dist:win
```

### Testing

Test installers in virtual machines or containers to ensure proper installation and functionality across different environments.