#!/bin/bash
# PhantomVault Windows Installer Builder
# Creates MSI installer using WiX Toolset

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
INSTALLER_DIR="$PROJECT_ROOT/installer"
BUILD_DIR="$PROJECT_ROOT/build"
PACKAGE_DIR="$BUILD_DIR/packages/windows"
VERSION="1.0.0"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() { echo -e "${BLUE}[WINDOWS]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check if running on Windows or with Wine
check_windows_env() {
    print_status "Checking Windows build environment..."
    
    if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
        print_status "Running on Windows (MSYS/Cygwin)"
        WINDOWS_ENV="native"
    elif command -v wine &> /dev/null; then
        print_status "Using Wine for Windows builds"
        WINDOWS_ENV="wine"
    else
        print_error "Windows installer requires Windows environment or Wine"
        print_error "Install Wine: sudo apt-get install wine"
        exit 1
    fi
}

# Install WiX Toolset
install_wix_toolset() {
    print_status "Setting up WiX Toolset..."
    
    local wix_dir="$PACKAGE_DIR/wix"
    mkdir -p "$wix_dir"
    
    if [[ ! -f "$wix_dir/candle.exe" ]]; then
        print_status "Downloading WiX Toolset..."
        
        # Download WiX binaries
        local wix_url="https://github.com/wixtoolset/wix3/releases/download/wix3112rtm/wix311-binaries.zip"
        
        if command -v wget &> /dev/null; then
            wget -q -O "$wix_dir/wix-binaries.zip" "$wix_url"
        elif command -v curl &> /dev/null; then
            curl -fsSL -o "$wix_dir/wix-binaries.zip" "$wix_url"
        else
            print_error "Need wget or curl to download WiX Toolset"
            exit 1
        fi
        
        # Extract WiX
        cd "$wix_dir"
        unzip -q wix-binaries.zip
        rm wix-binaries.zip
        
        print_success "WiX Toolset installed"
    fi
    
    # Set WiX paths
    WIX_CANDLE="$wix_dir/candle.exe"
    WIX_LIGHT="$wix_dir/light.exe"
    
    if [[ "$WINDOWS_ENV" == "wine" ]]; then
        WIX_CANDLE="wine $WIX_CANDLE"
        WIX_LIGHT="wine $WIX_LIGHT"
    fi
}

# Prepare Windows build
prepare_windows_build() {
    print_status "Preparing Windows build..."
    
    # Clean and create directories
    rm -rf "$PACKAGE_DIR"
    mkdir -p "$PACKAGE_DIR"/{staging,wix,output}
    
    # Check if Windows binary exists
    local windows_binary="$BUILD_DIR/phantomvault.exe"
    if [[ ! -f "$windows_binary" ]]; then
        print_warning "Windows binary not found, attempting cross-compilation..."
        
        # Try to cross-compile for Windows
        cd "$PROJECT_ROOT"
        if command -v x86_64-w64-mingw32-g++ &> /dev/null; then
            print_status "Cross-compiling for Windows..."
            mkdir -p "$BUILD_DIR/windows"
            cd "$BUILD_DIR/windows"
            
            # Configure for Windows cross-compilation
            cmake "$PROJECT_ROOT" \
                -DCMAKE_TOOLCHAIN_FILE="$PROJECT_ROOT/cmake/mingw-w64-x86_64.cmake" \
                -DCMAKE_BUILD_TYPE=Release
            
            make -j$(nproc)
            
            if [[ -f "phantomvault.exe" ]]; then
                cp "phantomvault.exe" "$windows_binary"
                print_success "Windows binary cross-compiled successfully"
            else
                print_error "Failed to cross-compile Windows binary"
                exit 1
            fi
        else
            print_error "Windows binary not found and cross-compilation not available"
            print_error "Please build on Windows or install mingw-w64"
            exit 1
        fi
    fi
    
    # Copy binary to staging
    cp "$windows_binary" "$PACKAGE_DIR/staging/phantomvault-service.exe"
}

# Create WiX source file
create_wix_source() {
    print_status "Creating WiX installer source..."
    
    local wix_file="$PACKAGE_DIR/wix/phantomvault.wxs"
    
    cat > "$wix_file" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">
  <Product Id="*" 
           Name="PhantomVault" 
           Language="1033" 
           Version="1.0.0" 
           Manufacturer="PhantomVault Team" 
           UpgradeCode="12345678-1234-1234-1234-123456789012">
    
    <Package InstallerVersion="200" 
             Compressed="yes" 
             InstallScope="perMachine"
             Description="PhantomVault - Invisible Folder Security"
             Comments="Military-grade folder security with profile-based management" />
    
    <MajorUpgrade DowngradeErrorMessage="A newer version of [ProductName] is already installed." />
    <MediaTemplate EmbedCab="yes" />
    
    <!-- Installation directory structure -->
    <Feature Id="ProductFeature" Title="PhantomVault" Level="1">
      <ComponentGroupRef Id="ProductComponents" />
      <ComponentGroupRef Id="ServiceComponents" />
      <ComponentGroupRef Id="ShortcutComponents" />
    </Feature>
    
    <!-- Installation directory -->
    <Directory Id="TARGETDIR" Name="SourceDir">
      <Directory Id="ProgramFilesFolder">
        <Directory Id="INSTALLFOLDER" Name="PhantomVault">
          <Directory Id="BinFolder" Name="bin" />
          <Directory Id="LogsFolder" Name="logs" />
          <Directory Id="DataFolder" Name="data" />
        </Directory>
      </Directory>
      
      <!-- Start Menu -->
      <Directory Id="ProgramMenuFolder">
        <Directory Id="ApplicationProgramsFolder" Name="PhantomVault" />
      </Directory>
      
      <!-- Desktop -->
      <Directory Id="DesktopFolder" Name="Desktop" />
    </Directory>
    
    <!-- Main application components -->
    <ComponentGroup Id="ProductComponents" Directory="BinFolder">
      <Component Id="MainExecutable" Guid="*">
        <File Id="PhantomVaultService" 
              Source="$(var.SourceDir)\phantomvault-service.exe" 
              KeyPath="yes" />
        
        <!-- Windows Service registration -->
        <ServiceInstall Id="PhantomVaultService"
                       Type="ownProcess"
                       Vital="yes"
                       Name="PhantomVault"
                       DisplayName="PhantomVault - Invisible Folder Security"
                       Description="Provides invisible folder security with profile-based management"
                       Start="auto"
                       Account="LocalSystem"
                       ErrorControl="ignore"
                       Interactive="no">
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
      
      <!-- GUI Wrapper Script -->
      <Component Id="GUIWrapper" Guid="*">
        <File Id="PhantomVaultGUI" 
              Source="$(var.SourceDir)\phantomvault-gui.bat" 
              KeyPath="yes" />
      </Component>
      
      <!-- CLI Tool -->
      <Component Id="CLITool" Guid="*">
        <File Id="PhantomVaultCLI" 
              Source="$(var.SourceDir)\phantomvault.bat" 
              KeyPath="yes" />
      </Component>
    </ComponentGroup>
    
    <!-- Service configuration components -->
    <ComponentGroup Id="ServiceComponents" Directory="DataFolder">
      <Component Id="ServiceConfig" Guid="*">
        <CreateFolder />
        
        <!-- Registry entries for service configuration -->
        <RegistryKey Root="HKLM" Key="SOFTWARE\PhantomVault">
          <RegistryValue Type="string" Name="InstallPath" Value="[INSTALLFOLDER]" />
          <RegistryValue Type="string" Name="Version" Value="1.0.0" />
          <RegistryValue Type="string" Name="DataPath" Value="[DataFolder]" />
        </RegistryKey>
        
        <!-- Firewall exception -->
        <fire:FirewallException xmlns:fire="http://schemas.microsoft.com/wix/FirewallExtension"
                               Id="PhantomVaultFirewall"
                               Name="PhantomVault Service"
                               Port="9876"
                               Protocol="tcp"
                               Scope="localSubnet" />
      </Component>
    </ComponentGroup>
    
    <!-- Shortcuts -->
    <ComponentGroup Id="ShortcutComponents" Directory="ApplicationProgramsFolder">
      <Component Id="ApplicationShortcut" Guid="*">
        <Shortcut Id="ApplicationStartMenuShortcut"
                 Name="PhantomVault"
                 Description="Invisible Folder Security"
                 Target="[BinFolder]phantomvault-gui.bat"
                 WorkingDirectory="BinFolder" />
        
        <RemoveFolder Id="ApplicationProgramsFolder" On="uninstall" />
        <RegistryValue Root="HKCU" 
                      Key="Software\PhantomVault" 
                      Name="installed" 
                      Type="integer" 
                      Value="1" 
                      KeyPath="yes" />
      </Component>
      
      <Component Id="DesktopShortcut" Guid="*" Directory="DesktopFolder">
        <Shortcut Id="ApplicationDesktopShortcut"
                 Name="PhantomVault"
                 Description="Invisible Folder Security"
                 Target="[BinFolder]phantomvault-gui.bat"
                 WorkingDirectory="BinFolder" />
        
        <RegistryValue Root="HKCU" 
                      Key="Software\PhantomVault" 
                      Name="desktop" 
                      Type="integer" 
                      Value="1" 
                      KeyPath="yes" />
      </Component>
    </ComponentGroup>
    
    <!-- UI customization -->
    <WixVariable Id="WixUILicenseRtf" Value="$(var.SourceDir)\license.rtf" />
    <WixVariable Id="WixUIBannerBmp" Value="$(var.SourceDir)\banner.bmp" />
    <WixVariable Id="WixUIDialogBmp" Value="$(var.SourceDir)\dialog.bmp" />
    
    <UIRef Id="WixUI_InstallDir" />
    <Property Id="WIXUI_INSTALLDIR" Value="INSTALLFOLDER" />
    
    <!-- Launch condition -->
    <Condition Message="This application requires Windows 7 or higher.">
      <![CDATA[Installed OR (VersionNT >= 601)]]>
    </Condition>
    
  </Product>
</Wix>
EOF
    
    print_success "WiX source file created"
}

# Create Windows batch files
create_windows_scripts() {
    print_status "Creating Windows scripts..."
    
    # Create GUI wrapper batch file
    cat > "$PACKAGE_DIR/staging/phantomvault-gui.bat" << 'EOF'
@echo off
REM PhantomVault GUI Launcher for Windows

set INSTALL_DIR=%~dp0
set SERVICE_NAME=PhantomVault

echo PhantomVault - Invisible Folder Security
echo ========================================

REM Check if service is running
sc query "%SERVICE_NAME%" | find "RUNNING" >nul
if %errorlevel% == 0 (
    echo Status: Running
    echo Your folders are protected
    echo.
    echo Usage:
    echo • Press Ctrl+Alt+V anywhere to access your folders
    echo • Use 'phantomvault --help' for more options
) else (
    echo Status: Not running
    echo.
    echo Starting PhantomVault service...
    net start "%SERVICE_NAME%"
    if %errorlevel% == 0 (
        echo Service started successfully
    ) else (
        echo Failed to start service - please run as administrator
    )
)

echo.
echo Press any key to continue...
pause >nul
EOF
    
    # Create CLI batch file
    cat > "$PACKAGE_DIR/staging/phantomvault.bat" << 'EOF'
@echo off
REM PhantomVault CLI for Windows

set INSTALL_DIR=%~dp0
set SERVICE_NAME=PhantomVault
set SERVICE_EXE=%INSTALL_DIR%phantomvault-service.exe

if "%1"=="" goto status
if "%1"=="--help" goto help
if "%1"=="-h" goto help
if "%1"=="--gui" goto gui
if "%1"=="--start" goto start
if "%1"=="--stop" goto stop
if "%1"=="--restart" goto restart
if "%1"=="--status" goto detailed_status

REM Pass other arguments to service executable
"%SERVICE_EXE%" %*
goto end

:status
echo PhantomVault - Invisible Folder Security
echo ========================================
sc query "%SERVICE_NAME%" | find "RUNNING" >nul
if %errorlevel% == 0 (
    echo Status: Running
) else (
    echo Status: Not running
)
goto end

:help
echo PhantomVault CLI for Windows
echo Usage: phantomvault [OPTION]
echo.
echo Options:
echo   (no args)     Show service status
echo   --gui         Open GUI application
echo   --start       Start service
echo   --stop        Stop service
echo   --restart     Restart service
echo   --status      Detailed service status
echo   --help        Show this help
goto end

:gui
start "" "%INSTALL_DIR%phantomvault-gui.bat"
goto end

:start
echo Starting PhantomVault service...
net start "%SERVICE_NAME%"
goto end

:stop
echo Stopping PhantomVault service...
net stop "%SERVICE_NAME%"
goto end

:restart
echo Restarting PhantomVault service...
net stop "%SERVICE_NAME%"
net start "%SERVICE_NAME%"
goto end

:detailed_status
sc query "%SERVICE_NAME%"
goto end

:end
EOF
    
    # Create license file (RTF format for WiX)
    cat > "$PACKAGE_DIR/staging/license.rtf" << 'EOF'
{\rtf1\ansi\deff0 {\fonttbl {\f0 Times New Roman;}}
\f0\fs24
PhantomVault License Agreement\par
\par
Copyright (c) 2024 PhantomVault Team\par
\par
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:\par
\par
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\par
\par
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\par
}
EOF
    
    print_success "Windows scripts created"
}

# Create installer images (placeholder)
create_installer_images() {
    print_status "Creating installer images..."
    
    # Create simple banner (BMP format required by WiX)
    # This is a placeholder - in production, use proper graphics tools
    cat > "$PACKAGE_DIR/staging/banner.bmp" << 'EOF'
BM6...  # Placeholder BMP header - replace with actual banner image
EOF
    
    cat > "$PACKAGE_DIR/staging/dialog.bmp" << 'EOF'
BM6...  # Placeholder BMP header - replace with actual dialog image
EOF
    
    print_warning "Using placeholder images - replace with proper graphics in production"
}

# Build MSI installer
build_msi_installer() {
    print_status "Building MSI installer..."
    
    local wix_file="$PACKAGE_DIR/wix/phantomvault.wxs"
    local obj_file="$PACKAGE_DIR/wix/phantomvault.wixobj"
    local msi_file="$PACKAGE_DIR/output/PhantomVault-$VERSION.msi"
    
    # Compile WiX source
    print_status "Compiling WiX source..."
    $WIX_CANDLE -out "$obj_file" "$wix_file" \
        -dSourceDir="$PACKAGE_DIR/staging" \
        -ext WixUtilExtension \
        -ext WixFirewallExtension
    
    if [[ $? -ne 0 ]]; then
        print_error "Failed to compile WiX source"
        exit 1
    fi
    
    # Link and create MSI
    print_status "Creating MSI installer..."
    $WIX_LIGHT -out "$msi_file" "$obj_file" \
        -ext WixUIExtension \
        -ext WixUtilExtension \
        -ext WixFirewallExtension
    
    if [[ $? -ne 0 ]]; then
        print_error "Failed to create MSI installer"
        exit 1
    fi
    
    print_success "MSI installer created: $msi_file"
}

# Create NSIS installer (alternative)
create_nsis_installer() {
    print_status "Creating NSIS installer..."
    
    # Check if NSIS is available
    if ! command -v makensis &> /dev/null; then
        print_warning "NSIS not found - skipping NSIS installer"
        return
    fi
    
    local nsis_file="$PACKAGE_DIR/nsis/phantomvault.nsi"
    mkdir -p "$PACKAGE_DIR/nsis"
    
    cat > "$nsis_file" << 'EOF'
; PhantomVault NSIS Installer Script
!define APPNAME "PhantomVault"
!define COMPANYNAME "PhantomVault Team"
!define DESCRIPTION "Invisible Folder Security"
!define VERSIONMAJOR 1
!define VERSIONMINOR 0
!define VERSIONBUILD 0

RequestExecutionLevel admin

InstallDir "$PROGRAMFILES\${APPNAME}"
Name "${APPNAME}"
OutFile "PhantomVault-Setup.exe"

Page directory
Page instfiles

Section "install"
    SetOutPath $INSTDIR
    
    ; Copy files
    File "..\staging\phantomvault-service.exe"
    File "..\staging\phantomvault-gui.bat"
    File "..\staging\phantomvault.bat"
    
    ; Create shortcuts
    CreateDirectory "$SMPROGRAMS\${APPNAME}"
    CreateShortCut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\phantomvault-gui.bat"
    CreateShortCut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\phantomvault-gui.bat"
    
    ; Install service
    ExecWait '"$INSTDIR\phantomvault-service.exe" --install-service'
    
    ; Start service
    ExecWait 'net start PhantomVault'
    
    ; Registry entries
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayName" "${APPNAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString" "$INSTDIR\uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "Publisher" "${COMPANYNAME}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "VersionMajor" ${VERSIONMAJOR}
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "VersionMinor" ${VERSIONMINOR}
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoRepair" 1
    
    ; Create uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

Section "uninstall"
    ; Stop and remove service
    ExecWait 'net stop PhantomVault'
    ExecWait '"$INSTDIR\phantomvault-service.exe" --remove-service'
    
    ; Remove files
    Delete "$INSTDIR\phantomvault-service.exe"
    Delete "$INSTDIR\phantomvault-gui.bat"
    Delete "$INSTDIR\phantomvault.bat"
    Delete "$INSTDIR\uninstall.exe"
    RMDir "$INSTDIR"
    
    ; Remove shortcuts
    Delete "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk"
    RMDir "$SMPROGRAMS\${APPNAME}"
    Delete "$DESKTOP\${APPNAME}.lnk"
    
    ; Remove registry entries
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
SectionEnd
EOF
    
    # Build NSIS installer
    cd "$PACKAGE_DIR/nsis"
    makensis phantomvault.nsi
    
    if [[ -f "PhantomVault-Setup.exe" ]]; then
        mv "PhantomVault-Setup.exe" "$PACKAGE_DIR/output/"
        print_success "NSIS installer created: $PACKAGE_DIR/output/PhantomVault-Setup.exe"
    fi
}

# Main function
main() {
    print_status "Building PhantomVault Windows installer..."
    
    check_windows_env
    install_wix_toolset
    prepare_windows_build
    create_wix_source
    create_windows_scripts
    create_installer_images
    build_msi_installer
    create_nsis_installer
    
    print_success "Windows installer created successfully!"
    print_status "Installers available in: $PACKAGE_DIR/output"
    
    # List created installers
    echo ""
    echo "Created installers:"
    find "$PACKAGE_DIR/output" -name "*.msi" -o -name "*.exe" | while read -r installer; do
        echo "  • $(basename "$installer")"
    done
}

# Run main function
main "$@"