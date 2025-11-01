#!/bin/bash

# PhantomVault Windows Installer Builder
# Creates MSI installer with complete uninstaller using WiX Toolset

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() { echo -e "${BLUE}[INSTALLER]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Configuration
PACKAGE_NAME="PhantomVault"
VERSION="1.0.0"
MANUFACTURER="PhantomVault Team"
DESCRIPTION="Invisible Folder Security with Profile-Based Management"
UPGRADE_CODE="12345678-1234-1234-1234-123456789012"

# Directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/installer/build"
WIX_DIR="$BUILD_DIR/wix"

print_status "Building Windows MSI installer..."
print_status "Project root: $PROJECT_ROOT"
print_status "Build directory: $BUILD_DIR"

# Clean and create build directories
rm -rf "$WIX_DIR"
mkdir -p "$WIX_DIR"/{source,output}

# Check for WiX Toolset (if running on Windows/WSL)
check_wix_toolset() {
    if command -v candle.exe &> /dev/null && command -v light.exe &> /dev/null; then
        print_status "WiX Toolset found"
        return 0
    elif [ -d "/mnt/c/Program Files (x86)/WiX Toolset v3.11/bin" ]; then
        export PATH="/mnt/c/Program Files (x86)/WiX Toolset v3.11/bin:$PATH"
        print_status "WiX Toolset found in Program Files"
        return 0
    else
        print_warning "WiX Toolset not found"
        print_status "Please install WiX Toolset v3.11 or later"
        print_status "Download from: https://wixtoolset.org/releases/"
        return 1
    fi
}

# Create WiX source files
create_wix_source() {
    print_status "Creating WiX installer source..."
    
    # Copy application files to source directory
    local source_dir="$WIX_DIR/source"
    mkdir -p "$source_dir"/{bin,gui,docs}
    
    # Copy service executable
    if [ -f "$PROJECT_ROOT/bin/phantomvault.exe" ]; then
        cp "$PROJECT_ROOT/bin/phantomvault.exe" "$source_dir/bin/"
    elif [ -f "$PROJECT_ROOT/core/build/bin/phantomvault-service.exe" ]; then
        cp "$PROJECT_ROOT/core/build/bin/phantomvault-service.exe" "$source_dir/bin/phantomvault.exe"
    else
        print_warning "No Windows executable found, creating placeholder"
        echo "Placeholder for phantomvault.exe" > "$source_dir/bin/phantomvault.exe"
    fi
    
    # Copy GUI files if they exist
    if [ -d "$PROJECT_ROOT/gui/dist" ]; then
        cp -r "$PROJECT_ROOT/gui/dist"/* "$source_dir/gui/"
    fi
    
    # Copy documentation
    if [ -f "$PROJECT_ROOT/README.md" ]; then
        cp "$PROJECT_ROOT/README.md" "$source_dir/docs/"
    fi
    
    # Create main WiX source file
    cat > "$WIX_DIR/phantomvault.wxs" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">
  <Product Id="*" 
           Name="$PACKAGE_NAME" 
           Language="1033" 
           Version="$VERSION" 
           Manufacturer="$MANUFACTURER" 
           UpgradeCode="$UPGRADE_CODE">
    
    <Package InstallerVersion="200" 
             Compressed="yes" 
             InstallScope="perMachine" 
             Description="$DESCRIPTION"
             Comments="PhantomVault Security System Installer" />
    
    <MajorUpgrade DowngradeErrorMessage="A newer version of [ProductName] is already installed." />
    <MediaTemplate EmbedCab="yes" />
    
    <!-- Installation directory structure -->
    <Feature Id="ProductFeature" Title="PhantomVault" Level="1">
      <ComponentGroupRef Id="ProductComponents" />
      <ComponentGroupRef Id="ServiceComponents" />
      <ComponentGroupRef Id="ShortcutComponents" />
    </Feature>
    
    <!-- Directory structure -->
    <Directory Id="TARGETDIR" Name="SourceDir">
      <Directory Id="ProgramFilesFolder">
        <Directory Id="INSTALLFOLDER" Name="PhantomVault">
          <Directory Id="BinFolder" Name="bin" />
          <Directory Id="GuiFolder" Name="gui" />
          <Directory Id="DocsFolder" Name="docs" />
        </Directory>
      </Directory>
      
      <!-- Start Menu -->
      <Directory Id="ProgramMenuFolder">
        <Directory Id="ApplicationProgramsFolder" Name="PhantomVault" />
      </Directory>
      
      <!-- Desktop -->
      <Directory Id="DesktopFolder" Name="Desktop" />
    </Directory>
    
    <!-- Service installation -->
    <ComponentGroup Id="ServiceComponents" Directory="BinFolder">
      <Component Id="ServiceExecutable" Guid="*">
        <File Id="PhantomVaultExe" 
              Source="source/bin/phantomvault.exe" 
              KeyPath="yes" />
        
        <!-- Windows Service -->
        <ServiceInstall Id="PhantomVaultService"
                        Type="ownProcess"
                        Vital="yes"
                        Name="PhantomVault"
                        DisplayName="PhantomVault Security Service"
                        Description="Invisible folder security with profile-based management"
                        Start="auto"
                        Account="LocalSystem"
                        ErrorControl="ignore"
                        Interactive="no"
                        Arguments="--service --daemon">
          <util:ServiceConfig xmlns:util="http://schemas.microsoft.com/wix/UtilExtension"
                              FirstFailureActionType="restart"
                              SecondFailureActionType="restart"
                              ThirdFailureActionType="restart"
                              RestartServiceDelayInSeconds="60" />
        </ServiceInstall>
        
        <ServiceControl Id="StartService" 
                        Start="install" 
                        Stop="both" 
                        Remove="uninstall" 
                        Name="PhantomVault" 
                        Wait="yes" />
      </Component>
    </ComponentGroup>
    
    <!-- Application files -->
    <ComponentGroup Id="ProductComponents" Directory="INSTALLFOLDER">
      <Component Id="MainExecutable" Guid="*">
        <File Source="source/bin/phantomvault.exe" />
      </Component>
      
      <!-- GUI files -->
      <Component Id="GuiFiles" Guid="*" Directory="GuiFolder">
        <File Source="source/gui/*" />
      </Component>
      
      <!-- Documentation -->
      <Component Id="Documentation" Guid="*" Directory="DocsFolder">
        <File Source="source/docs/README.md" />
      </Component>
      
      <!-- Registry entries -->
      <Component Id="RegistryEntries" Guid="*">
        <RegistryKey Root="HKLM" Key="Software\\PhantomVault">
          <RegistryValue Type="string" Name="InstallPath" Value="[INSTALLFOLDER]" KeyPath="yes" />
          <RegistryValue Type="string" Name="Version" Value="$VERSION" />
        </RegistryKey>
        
        <!-- Protocol handler -->
        <RegistryKey Root="HKLM" Key="Software\\Classes\\phantomvault">
          <RegistryValue Type="string" Value="PhantomVault Protocol" />
          <RegistryValue Type="string" Name="URL Protocol" Value="" />
          <RegistryKey Key="shell\\open\\command">
            <RegistryValue Type="string" Value="&quot;[INSTALLFOLDER]bin\\phantomvault.exe&quot; &quot;%1&quot;" />
          </RegistryKey>
        </RegistryKey>
      </Component>
      
      <!-- Uninstaller -->
      <Component Id="UninstallerComponent" Guid="*">
        <File Id="UninstallerScript" 
              Source="uninstaller.bat" 
              Name="uninstall.bat" />
      </Component>
    </ComponentGroup>
    
    <!-- Shortcuts -->
    <ComponentGroup Id="ShortcutComponents">
      <!-- Start Menu shortcuts -->
      <Component Id="ApplicationShortcut" Guid="*" Directory="ApplicationProgramsFolder">
        <Shortcut Id="ApplicationStartMenuShortcut"
                  Name="PhantomVault"
                  Description="Invisible Folder Security"
                  Target="[INSTALLFOLDER]bin\\phantomvault.exe"
                  Arguments="--gui"
                  WorkingDirectory="INSTALLFOLDER" />
        
        <Shortcut Id="UninstallShortcut"
                  Name="Uninstall PhantomVault"
                  Description="Remove PhantomVault from your computer"
                  Target="[System64Folder]msiexec.exe"
                  Arguments="/x [ProductCode]" />
        
        <RemoveFolder Id="ApplicationProgramsFolder" On="uninstall" />
        <RegistryValue Root="HKCU" 
                       Key="Software\\PhantomVault\\Shortcuts" 
                       Name="installed" 
                       Type="integer" 
                       Value="1" 
                       KeyPath="yes" />
      </Component>
      
      <!-- Desktop shortcut -->
      <Component Id="DesktopShortcut" Guid="*" Directory="DesktopFolder">
        <Shortcut Id="DesktopShortcut"
                  Name="PhantomVault"
                  Description="Invisible Folder Security"
                  Target="[INSTALLFOLDER]bin\\phantomvault.exe"
                  Arguments="--gui"
                  WorkingDirectory="INSTALLFOLDER" />
        
        <RegistryValue Root="HKCU" 
                       Key="Software\\PhantomVault\\Shortcuts" 
                       Name="desktop" 
                       Type="integer" 
                       Value="1" 
                       KeyPath="yes" />
      </Component>
    </ComponentGroup>
    
    <!-- Custom actions for cleanup -->
    <CustomAction Id="CleanupUserData" 
                  BinaryKey="WixCA" 
                  DllEntry="WixQuietExec" 
                  Execute="deferred" 
                  Return="ignore" 
                  Impersonate="no" />
    
    <!-- Installation sequence -->
    <InstallExecuteSequence>
      <Custom Action="CleanupUserData" Before="RemoveFiles">REMOVE="ALL"</Custom>
    </InstallExecuteSequence>
    
    <!-- UI -->
    <UIRef Id="WixUI_InstallDir" />
    <Property Id="WIXUI_INSTALLDIR" Value="INSTALLFOLDER" />
    
    <!-- License -->
    <WixVariable Id="WixUILicenseRtf" Value="license.rtf" />
    
  </Product>
</Wix>
EOF
    
    # Create uninstaller batch file
    cat > "$WIX_DIR/uninstaller.bat" << 'EOF'
@echo off
echo PhantomVault Uninstaller
echo This will completely remove PhantomVault from your system.
echo User data will be preserved.
pause

echo Stopping PhantomVault service...
net stop PhantomVault 2>nul

echo Removing PhantomVault...
msiexec /x {ProductCode} /quiet

echo PhantomVault has been removed.
echo User data preserved in %USERPROFILE%\.phantomvault\
pause
EOF
    
    # Create license file
    cat > "$WIX_DIR/license.rtf" << 'EOF'
{\rtf1\ansi\deff0 {\fonttbl {\f0 Times New Roman;}}
\f0\fs24
PhantomVault License Agreement\par
\par
Copyright (c) 2025 PhantomVault Team\par
\par
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:\par
\par
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\par
\par
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\par
}
EOF
    
    print_success "WiX source files created"
}

# Build MSI installer
build_msi_installer() {
    print_status "Building MSI installer..."
    
    cd "$WIX_DIR"
    
    # Compile WiX source
    print_status "Compiling WiX source..."
    candle.exe -ext WixUtilExtension phantomvault.wxs -o output/
    
    if [ $? -ne 0 ]; then
        print_error "Failed to compile WiX source"
        return 1
    fi
    
    # Link and create MSI
    print_status "Linking MSI package..."
    light.exe -ext WixUIExtension -ext WixUtilExtension output/phantomvault.wixobj -o "output/PhantomVault-$VERSION.msi"
    
    if [ $? -eq 0 ]; then
        print_success "MSI installer created: output/PhantomVault-$VERSION.msi"
        
        # Copy to main build directory
        cp "output/PhantomVault-$VERSION.msi" "$BUILD_DIR/"
        print_success "MSI installer available at: $BUILD_DIR/PhantomVault-$VERSION.msi"
    else
        print_error "Failed to create MSI installer"
        return 1
    fi
}

# Create standalone installer
create_standalone_installer() {
    print_status "Creating standalone Windows installer..."
    
    cat > "$BUILD_DIR/phantomvault-windows-installer.bat" << 'EOF'
@echo off
title PhantomVault Windows Installer

echo ========================================
echo PhantomVault Windows Installer v1.0.0
echo Invisible Folder Security System
echo ========================================
echo.

:: Check for administrator privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: This installer must be run as Administrator
    echo Right-click and select "Run as administrator"
    pause
    exit /b 1
)

echo Installing PhantomVault...

:: Create installation directory
set INSTALL_DIR=%ProgramFiles%\PhantomVault
mkdir "%INSTALL_DIR%" 2>nul
mkdir "%INSTALL_DIR%\bin" 2>nul
mkdir "%INSTALL_DIR%\gui" 2>nul
mkdir "%INSTALL_DIR%\docs" 2>nul

:: Copy files (in real installer, files would be embedded)
echo Copying application files...
if exist "phantomvault.exe" (
    copy "phantomvault.exe" "%INSTALL_DIR%\bin\" >nul
) else (
    echo ERROR: phantomvault.exe not found in installer
    pause
    exit /b 1
)

:: Install Windows service
echo Installing PhantomVault service...
sc create PhantomVault binPath= "\"%INSTALL_DIR%\bin\phantomvault.exe\" --service --daemon" DisplayName= "PhantomVault Security Service" start= auto

:: Create desktop shortcut
echo Creating desktop shortcut...
set DESKTOP=%USERPROFILE%\Desktop
echo [InternetShortcut] > "%DESKTOP%\PhantomVault.url"
echo URL=file:///%INSTALL_DIR%\bin\phantomvault.exe >> "%DESKTOP%\PhantomVault.url"
echo IconFile=%INSTALL_DIR%\bin\phantomvault.exe >> "%DESKTOP%\PhantomVault.url"
echo IconIndex=0 >> "%DESKTOP%\PhantomVault.url"

:: Create start menu shortcut
set STARTMENU=%ProgramData%\Microsoft\Windows\Start Menu\Programs
mkdir "%STARTMENU%\PhantomVault" 2>nul
echo [InternetShortcut] > "%STARTMENU%\PhantomVault\PhantomVault.url"
echo URL=file:///%INSTALL_DIR%\bin\phantomvault.exe >> "%STARTMENU%\PhantomVault\PhantomVault.url"

:: Register protocol handler
echo Registering protocol handler...
reg add "HKLM\SOFTWARE\Classes\phantomvault" /ve /d "PhantomVault Protocol" /f >nul
reg add "HKLM\SOFTWARE\Classes\phantomvault" /v "URL Protocol" /d "" /f >nul
reg add "HKLM\SOFTWARE\Classes\phantomvault\shell\open\command" /ve /d "\"%INSTALL_DIR%\bin\phantomvault.exe\" \"%%1\"" /f >nul

:: Create uninstaller
echo Creating uninstaller...
(
echo @echo off
echo title PhantomVault Uninstaller
echo echo Removing PhantomVault...
echo.
echo :: Stop and remove service
echo net stop PhantomVault 2^>nul
echo sc delete PhantomVault 2^>nul
echo.
echo :: Remove files
echo rmdir /s /q "%INSTALL_DIR%" 2^>nul
echo.
echo :: Remove shortcuts
echo del "%USERPROFILE%\Desktop\PhantomVault.url" 2^>nul
echo rmdir /s /q "%ProgramData%\Microsoft\Windows\Start Menu\Programs\PhantomVault" 2^>nul
echo.
echo :: Remove registry entries
echo reg delete "HKLM\SOFTWARE\Classes\phantomvault" /f 2^>nul
echo.
echo echo PhantomVault has been completely removed.
echo echo User data preserved in %%USERPROFILE%%\.phantomvault\
echo pause
) > "%INSTALL_DIR%\uninstall.bat"

:: Start service
echo Starting PhantomVault service...
net start PhantomVault

echo.
echo ========================================
echo PhantomVault installed successfully!
echo ========================================
echo.
echo Service: PhantomVault (started automatically)
echo GUI: Run PhantomVault from Start Menu or Desktop
echo Uninstall: Run %INSTALL_DIR%\uninstall.bat as Administrator
echo.
pause
EOF
    
    print_success "Standalone Windows installer created: $BUILD_DIR/phantomvault-windows-installer.bat"
}

# Main execution
main() {
    print_status "Starting Windows installer build process..."
    
    create_wix_source
    
    if check_wix_toolset; then
        build_msi_installer
    else
        print_warning "Skipping MSI creation due to missing WiX Toolset"
    fi
    
    create_standalone_installer
    
    print_success "Windows installer packages created successfully!"
    print_status "Available installers:"
    find "$BUILD_DIR" -name "*.msi" -o -name "*windows*installer*" | while read file; do
        print_status "  - $(basename "$file")"
    done
}

# Run main function
main "$@"