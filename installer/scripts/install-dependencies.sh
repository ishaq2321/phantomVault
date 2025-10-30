#!/bin/bash
# PhantomVault Dependency Installer
# Automatically installs required dependencies for all platforms

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() { echo -e "${BLUE}[DEPS]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Detect platform and distribution
detect_platform() {
    case "$(uname -s)" in
        Linux*)
            PLATFORM="linux"
            if [[ -f /etc/os-release ]]; then
                source /etc/os-release
                case "$ID" in
                    ubuntu|debian)
                        DISTRO="debian"
                        PACKAGE_MANAGER="apt"
                        ;;
                    fedora)
                        DISTRO="fedora"
                        PACKAGE_MANAGER="dnf"
                        ;;
                    rhel|centos|rocky|almalinux)
                        DISTRO="rhel"
                        PACKAGE_MANAGER="yum"
                        if command -v dnf &> /dev/null; then
                            PACKAGE_MANAGER="dnf"
                        fi
                        ;;
                    arch|manjaro)
                        DISTRO="arch"
                        PACKAGE_MANAGER="pacman"
                        ;;
                    opensuse*|sles)
                        DISTRO="opensuse"
                        PACKAGE_MANAGER="zypper"
                        ;;
                    *)
                        DISTRO="unknown"
                        PACKAGE_MANAGER="unknown"
                        ;;
                esac
            else
                DISTRO="unknown"
                PACKAGE_MANAGER="unknown"
            fi
            ;;
        Darwin*)
            PLATFORM="macos"
            DISTRO="macos"
            if command -v brew &> /dev/null; then
                PACKAGE_MANAGER="brew"
            else
                PACKAGE_MANAGER="none"
            fi
            ;;
        CYGWIN*|MINGW32*|MSYS*|MINGW*)
            PLATFORM="windows"
            DISTRO="windows"
            if command -v choco &> /dev/null; then
                PACKAGE_MANAGER="choco"
            elif command -v winget &> /dev/null; then
                PACKAGE_MANAGER="winget"
            else
                PACKAGE_MANAGER="none"
            fi
            ;;
        *)
            PLATFORM="unknown"
            DISTRO="unknown"
            PACKAGE_MANAGER="unknown"
            ;;
    esac
    
    print_status "Detected: $PLATFORM ($DISTRO) with $PACKAGE_MANAGER"
}

# Install build dependencies
install_build_dependencies() {
    print_status "Installing build dependencies..."
    
    case "$DISTRO" in
        debian)
            print_status "Installing dependencies for Debian/Ubuntu..."
            sudo apt-get update -qq
            sudo apt-get install -y -qq \
                build-essential \
                cmake \
                pkg-config \
                libssl-dev \
                libx11-dev \
                libxtst-dev \
                libgtk-3-dev \
                libnlohmann-json3-dev \
                git \
                curl \
                wget \
                unzip
            
            # Install Node.js and npm for GUI
            if ! command -v node &> /dev/null || ! command -v npm &> /dev/null; then
                print_status "Installing Node.js and npm..."
                curl -fsSL https://deb.nodesource.com/setup_lts.x | sudo -E bash -
                sudo apt-get install -y nodejs
            fi
            ;;
            
        fedora)
            print_status "Installing dependencies for Fedora..."
            sudo dnf install -y \
                gcc-c++ \
                cmake \
                pkgconfig \
                openssl-devel \
                libX11-devel \
                libXtst-devel \
                gtk3-devel \
                json-devel \
                git \
                curl \
                wget \
                unzip
            
            # Install Node.js and npm
            if ! command -v node &> /dev/null || ! command -v npm &> /dev/null; then
                sudo dnf install -y nodejs npm
            fi
            ;;
            
        rhel)
            print_status "Installing dependencies for RHEL/CentOS..."
            
            # Enable EPEL repository
            if ! rpm -q epel-release &> /dev/null; then
                sudo $PACKAGE_MANAGER install -y epel-release
            fi
            
            sudo $PACKAGE_MANAGER groupinstall -y "Development Tools"
            sudo $PACKAGE_MANAGER install -y \
                cmake \
                pkgconfig \
                openssl-devel \
                libX11-devel \
                libXtst-devel \
                gtk3-devel \
                git \
                curl \
                wget \
                unzip
            
            # Install Node.js
            if ! command -v node &> /dev/null; then
                curl -fsSL https://rpm.nodesource.com/setup_lts.x | sudo bash -
                sudo $PACKAGE_MANAGER install -y nodejs
            fi
            ;;
            
        arch)
            print_status "Installing dependencies for Arch Linux..."
            sudo pacman -Sy --noconfirm \
                base-devel \
                cmake \
                pkgconf \
                openssl \
                libx11 \
                libxtst \
                gtk3 \
                nlohmann-json \
                git \
                curl \
                wget \
                unzip
            
            # Install Node.js and npm
            if ! command -v node &> /dev/null; then
                sudo pacman -S --noconfirm nodejs npm
            fi
            ;;
            
        opensuse)
            print_status "Installing dependencies for openSUSE..."
            sudo zypper install -y \
                gcc-c++ \
                cmake \
                pkg-config \
                libopenssl-devel \
                libX11-devel \
                libXtst-devel \
                gtk3-devel \
                nlohmann_json-devel \
                git \
                curl \
                wget \
                unzip
            
            # Install Node.js and npm
            if ! command -v node &> /dev/null; then
                sudo zypper install -y nodejs18 npm18
            fi
            ;;
            
        *)
            print_warning "Unknown Linux distribution: $DISTRO"
            print_status "Please install these dependencies manually:"
            print_status "- C++ compiler (g++ or clang++)"
            print_status "- CMake 3.10+"
            print_status "- OpenSSL development libraries"
            print_status "- X11 development libraries"
            print_status "- XTest development libraries"
            print_status "- GTK3 development libraries"
            print_status "- nlohmann/json library"
            print_status "- Node.js and npm"
            return 1
            ;;
    esac
    
    print_success "Build dependencies installed"
}

# Install packaging dependencies
install_packaging_dependencies() {
    print_status "Installing packaging dependencies..."
    
    case "$DISTRO" in
        debian)
            sudo apt-get install -y -qq \
                dpkg-dev \
                fakeroot \
                rpm \
                alien
            ;;
            
        fedora|rhel)
            sudo $PACKAGE_MANAGER install -y \
                rpm-build \
                rpm-devel \
                rpmdevtools \
                dpkg \
                fakeroot
            ;;
            
        arch)
            sudo pacman -S --noconfirm \
                base-devel \
                fakeroot \
                dpkg
            
            # Install rpm-tools from AUR if available
            if command -v yay &> /dev/null; then
                yay -S --noconfirm rpm-tools
            elif command -v paru &> /dev/null; then
                paru -S --noconfirm rpm-tools
            fi
            ;;
            
        opensuse)
            sudo zypper install -y \
                rpm-build \
                dpkg \
                fakeroot
            ;;
    esac
    
    print_success "Packaging dependencies installed"
}

# Install macOS dependencies
install_macos_dependencies() {
    print_status "Installing macOS dependencies..."
    
    # Check for Xcode Command Line Tools
    if ! xcode-select -p &> /dev/null; then
        print_status "Installing Xcode Command Line Tools..."
        xcode-select --install
        
        print_warning "Please complete Xcode Command Line Tools installation and run this script again"
        exit 1
    fi
    
    # Install Homebrew if not present
    if [[ "$PACKAGE_MANAGER" == "none" ]]; then
        print_status "Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
        
        # Add Homebrew to PATH
        if [[ -f "/opt/homebrew/bin/brew" ]]; then
            eval "$(/opt/homebrew/bin/brew shellenv)"
        elif [[ -f "/usr/local/bin/brew" ]]; then
            eval "$(/usr/local/bin/brew shellenv)"
        fi
        
        PACKAGE_MANAGER="brew"
    fi
    
    # Install dependencies via Homebrew
    print_status "Installing dependencies via Homebrew..."
    brew install \
        cmake \
        openssl \
        nlohmann-json \
        node
    
    print_success "macOS dependencies installed"
}

# Install Windows dependencies
install_windows_dependencies() {
    print_status "Installing Windows dependencies..."
    
    case "$PACKAGE_MANAGER" in
        choco)
            print_status "Installing dependencies via Chocolatey..."
            choco install -y \
                cmake \
                mingw \
                nodejs \
                git \
                curl \
                wget
            ;;
            
        winget)
            print_status "Installing dependencies via winget..."
            winget install --id Kitware.CMake
            winget install --id OpenJS.NodeJS
            winget install --id Git.Git
            winget install --id Microsoft.VisualStudio.2022.BuildTools
            ;;
            
        *)
            print_warning "No package manager found for Windows"
            print_status "Please install these dependencies manually:"
            print_status "1. Visual Studio Build Tools or MinGW-w64"
            print_status "2. CMake 3.10+"
            print_status "3. Node.js and npm"
            print_status "4. Git"
            print_status ""
            print_status "Recommended: Install Chocolatey (https://chocolatey.org/)"
            print_status "Then run: choco install cmake mingw nodejs git"
            return 1
            ;;
    esac
    
    print_success "Windows dependencies installed"
}

# Install development tools
install_development_tools() {
    print_status "Installing development tools..."
    
    # Install global npm packages for GUI development
    if command -v npm &> /dev/null; then
        print_status "Installing Node.js development tools..."
        npm install -g \
            electron \
            electron-builder \
            typescript \
            @types/node
    fi
    
    # Install additional tools based on platform
    case "$PLATFORM" in
        linux)
            # Install AppImage tools
            if [[ ! -f "/usr/local/bin/appimagetool" ]]; then
                print_status "Installing AppImageTool..."
                wget -q -O /tmp/appimagetool \
                    "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
                chmod +x /tmp/appimagetool
                sudo mv /tmp/appimagetool /usr/local/bin/
            fi
            ;;
            
        windows)
            # Install WiX Toolset for MSI creation
            if [[ "$PACKAGE_MANAGER" == "choco" ]]; then
                choco install -y wixtoolset
            fi
            ;;
    esac
    
    print_success "Development tools installed"
}

# Verify installation
verify_installation() {
    print_status "Verifying installation..."
    
    local missing_tools=()
    
    # Check essential build tools
    if ! command -v cmake &> /dev/null; then
        missing_tools+=("cmake")
    fi
    
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        missing_tools+=("C++ compiler")
    fi
    
    if ! command -v node &> /dev/null; then
        missing_tools+=("node")
    fi
    
    if ! command -v npm &> /dev/null; then
        missing_tools+=("npm")
    fi
    
    # Platform-specific checks
    case "$PLATFORM" in
        linux)
            if ! pkg-config --exists openssl; then
                missing_tools+=("openssl-dev")
            fi
            
            if ! pkg-config --exists x11; then
                missing_tools+=("libx11-dev")
            fi
            
            if ! pkg-config --exists xtst; then
                missing_tools+=("libxtst-dev")
            fi
            ;;
            
        macos)
            if ! xcode-select -p &> /dev/null; then
                missing_tools+=("Xcode Command Line Tools")
            fi
            ;;
    esac
    
    if [[ ${#missing_tools[@]} -eq 0 ]]; then
        print_success "All dependencies verified successfully!"
        
        # Show versions
        echo ""
        echo "Installed versions:"
        echo "=================="
        echo "CMake: $(cmake --version | head -n1)"
        echo "Node.js: $(node --version)"
        echo "npm: $(npm --version)"
        
        if command -v g++ &> /dev/null; then
            echo "g++: $(g++ --version | head -n1)"
        elif command -v clang++ &> /dev/null; then
            echo "clang++: $(clang++ --version | head -n1)"
        fi
        
        return 0
    else
        print_error "Missing dependencies: ${missing_tools[*]}"
        return 1
    fi
}

# Main function
main() {
    echo "PhantomVault Dependency Installer"
    echo "================================="
    echo ""
    
    detect_platform
    
    case "$PLATFORM" in
        linux)
            install_build_dependencies
            install_packaging_dependencies
            ;;
        macos)
            install_macos_dependencies
            ;;
        windows)
            install_windows_dependencies
            ;;
        *)
            print_error "Unsupported platform: $PLATFORM"
            exit 1
            ;;
    esac
    
    install_development_tools
    
    if verify_installation; then
        echo ""
        print_success "PhantomVault build environment ready!"
        echo ""
        echo "Next steps:"
        echo "1. Build PhantomVault: ./build.sh"
        echo "2. Create installers: ./installer/scripts/build-all-installers.sh"
        echo "3. Test installation: ./build/packages/install-phantomvault.sh"
    else
        print_error "Dependency installation incomplete"
        echo ""
        echo "Please resolve missing dependencies and run this script again"
        exit 1
    fi
}

# Run main function
main "$@"