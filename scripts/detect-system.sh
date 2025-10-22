#!/bin/bash

# PhantomVault System Detection Script
# Detects Linux distribution, desktop environment, and system capabilities

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Global variables
DISTRO=""
DISTRO_VERSION=""
DESKTOP_ENV=""
INIT_SYSTEM=""
PACKAGE_MANAGER=""
X11_AVAILABLE=false
WAYLAND_AVAILABLE=false

# Function to detect Linux distribution
detect_distribution() {
    if [[ -f /etc/os-release ]]; then
        source /etc/os-release
        DISTRO="$ID"
        DISTRO_VERSION="$VERSION_ID"
    elif [[ -f /etc/lsb-release ]]; then
        source /etc/lsb-release
        DISTRO="$DISTRIB_ID"
        DISTRO_VERSION="$DISTRIB_RELEASE"
    elif [[ -f /etc/debian_version ]]; then
        DISTRO="debian"
        DISTRO_VERSION=$(cat /etc/debian_version)
    elif [[ -f /etc/redhat-release ]]; then
        DISTRO="rhel"
        DISTRO_VERSION=$(grep -oE '[0-9]+\.[0-9]+' /etc/redhat-release | head -1)
    elif [[ -f /etc/arch-release ]]; then
        DISTRO="arch"
        DISTRO_VERSION="rolling"
    else
        DISTRO="unknown"
        DISTRO_VERSION="unknown"
    fi
    
    # Normalize distribution names
    case "$DISTRO" in
        "ubuntu"|"Ubuntu")
            DISTRO="ubuntu"
            ;;
        "debian"|"Debian")
            DISTRO="debian"
            ;;
        "fedora"|"Fedora")
            DISTRO="fedora"
            ;;
        "centos"|"CentOS"|"rhel"|"RHEL")
            DISTRO="rhel"
            ;;
        "arch"|"Arch"|"manjaro"|"Manjaro")
            DISTRO="arch"
            ;;
        "opensuse"|"openSUSE"|"suse"|"SUSE")
            DISTRO="opensuse"
            ;;
        "pop"|"Pop")
            DISTRO="ubuntu"  # Pop!_OS is Ubuntu-based
            ;;
    esac
}

# Function to detect desktop environment
detect_desktop_environment() {
    if [[ -n "$XDG_CURRENT_DESKTOP" ]]; then
        DESKTOP_ENV="$XDG_CURRENT_DESKTOP"
    elif [[ -n "$DESKTOP_SESSION" ]]; then
        DESKTOP_ENV="$DESKTOP_SESSION"
    elif [[ -n "$GDMSESSION" ]]; then
        DESKTOP_ENV="$GDMSESSION"
    elif pgrep -x "gnome-session" > /dev/null; then
        DESKTOP_ENV="GNOME"
    elif pgrep -x "kded5" > /dev/null || pgrep -x "kded4" > /dev/null; then
        DESKTOP_ENV="KDE"
    elif pgrep -x "xfce4-session" > /dev/null; then
        DESKTOP_ENV="XFCE"
    elif pgrep -x "lxsession" > /dev/null; then
        DESKTOP_ENV="LXDE"
    elif pgrep -x "mate-session" > /dev/null; then
        DESKTOP_ENV="MATE"
    elif pgrep -x "cinnamon-session" > /dev/null; then
        DESKTOP_ENV="Cinnamon"
    else
        DESKTOP_ENV="unknown"
    fi
    
    # Normalize desktop environment names
    DESKTOP_ENV=$(echo "$DESKTOP_ENV" | tr '[:lower:]' '[:upper:]')
}

# Function to detect init system
detect_init_system() {
    if [[ -d /run/systemd/system ]]; then
        INIT_SYSTEM="systemd"
    elif [[ -f /sbin/openrc ]]; then
        INIT_SYSTEM="openrc"
    elif [[ -d /etc/init ]]; then
        INIT_SYSTEM="upstart"
    elif [[ -f /etc/inittab ]]; then
        INIT_SYSTEM="sysvinit"
    else
        INIT_SYSTEM="unknown"
    fi
}

# Function to detect package manager
detect_package_manager() {
    if command -v apt &> /dev/null; then
        PACKAGE_MANAGER="apt"
    elif command -v dnf &> /dev/null; then
        PACKAGE_MANAGER="dnf"
    elif command -v yum &> /dev/null; then
        PACKAGE_MANAGER="yum"
    elif command -v pacman &> /dev/null; then
        PACKAGE_MANAGER="pacman"
    elif command -v zypper &> /dev/null; then
        PACKAGE_MANAGER="zypper"
    elif command -v emerge &> /dev/null; then
        PACKAGE_MANAGER="portage"
    else
        PACKAGE_MANAGER="unknown"
    fi
}

# Function to detect display server capabilities
detect_display_servers() {
    # Check for X11
    if [[ -n "$DISPLAY" ]] || command -v Xorg &> /dev/null || command -v X &> /dev/null; then
        X11_AVAILABLE=true
    fi
    
    # Check for Wayland
    if [[ -n "$WAYLAND_DISPLAY" ]] || command -v weston &> /dev/null || command -v sway &> /dev/null; then
        WAYLAND_AVAILABLE=true
    fi
}

# Function to check system requirements
check_requirements() {
    local requirements_met=true
    
    echo -e "${BLUE}System Requirements Check:${NC}"
    
    # Check for required libraries
    local required_libs=("libX11" "libXtst" "libXi")
    for lib in "${required_libs[@]}"; do
        if ldconfig -p | grep -q "$lib"; then
            echo -e "  ✅ $lib found"
        else
            echo -e "  ❌ $lib missing"
            requirements_met=false
        fi
    done
    
    # Check for development tools if building from source
    if [[ ! -f "$HOME/.local/bin/phantom_vault_service" ]]; then
        echo -e "\n${BLUE}Development Tools (for building):${NC}"
        local dev_tools=("gcc" "g++" "make" "cmake")
        for tool in "${dev_tools[@]}"; do
            if command -v "$tool" &> /dev/null; then
                echo -e "  ✅ $tool found"
            else
                echo -e "  ❌ $tool missing"
                requirements_met=false
            fi
        done
    fi
    
    if [[ "$requirements_met" == true ]]; then
        return 0
    else
        return 1
    fi
}

# Function to suggest installation commands
suggest_installation() {
    echo -e "\n${BLUE}Installation Suggestions:${NC}"
    
    case "$DISTRO" in
        "ubuntu"|"debian")
            echo "Install required packages:"
            echo "  sudo apt update"
            echo "  sudo apt install libx11-dev libxtst-dev libxi-dev"
            if [[ ! -f "$HOME/.local/bin/phantom_vault_service" ]]; then
                echo "  sudo apt install build-essential cmake"
            fi
            ;;
        "fedora")
            echo "Install required packages:"
            echo "  sudo dnf install libX11-devel libXtst-devel libXi-devel"
            if [[ ! -f "$HOME/.local/bin/phantom_vault_service" ]]; then
                echo "  sudo dnf install gcc-c++ make cmake"
            fi
            ;;
        "rhel")
            echo "Install required packages:"
            echo "  sudo yum install libX11-devel libXtst-devel libXi-devel"
            if [[ ! -f "$HOME/.local/bin/phantom_vault_service" ]]; then
                echo "  sudo yum groupinstall 'Development Tools'"
                echo "  sudo yum install cmake"
            fi
            ;;
        "arch")
            echo "Install required packages:"
            echo "  sudo pacman -S libx11 libxtst libxi"
            if [[ ! -f "$HOME/.local/bin/phantom_vault_service" ]]; then
                echo "  sudo pacman -S base-devel cmake"
            fi
            ;;
        "opensuse")
            echo "Install required packages:"
            echo "  sudo zypper install libX11-devel libXtst-devel libXi-devel"
            if [[ ! -f "$HOME/.local/bin/phantom_vault_service" ]]; then
                echo "  sudo zypper install gcc-c++ make cmake"
            fi
            ;;
        *)
            echo "Unknown distribution. Please install X11 development libraries manually."
            ;;
    esac
}

# Function to check compatibility
check_compatibility() {
    local compatible=true
    
    echo -e "\n${BLUE}Compatibility Check:${NC}"
    
    # Check init system
    if [[ "$INIT_SYSTEM" == "systemd" ]]; then
        echo -e "  ✅ systemd supported"
    else
        echo -e "  ⚠️  Non-systemd init system ($INIT_SYSTEM)"
        echo -e "     Auto-start may require manual configuration"
        compatible=false
    fi
    
    # Check display server
    if [[ "$X11_AVAILABLE" == true ]]; then
        echo -e "  ✅ X11 available"
    elif [[ "$WAYLAND_AVAILABLE" == true ]]; then
        echo -e "  ⚠️  Wayland detected (X11 compatibility layer may be needed)"
    else
        echo -e "  ❌ No display server detected"
        compatible=false
    fi
    
    # Check desktop environment
    case "$DESKTOP_ENV" in
        "GNOME"|"KDE"|"XFCE"|"MATE"|"CINNAMON"|"LXDE")
            echo -e "  ✅ Desktop environment supported ($DESKTOP_ENV)"
            ;;
        *)
            echo -e "  ⚠️  Unknown desktop environment ($DESKTOP_ENV)"
            echo -e "     Global hotkeys may not work properly"
            ;;
    esac
    
    if [[ "$compatible" == true ]]; then
        return 0
    else
        return 1
    fi
}

# Main function
main() {
    echo -e "${BLUE}PhantomVault System Detection${NC}"
    echo "============================="
    
    # Detect system information
    detect_distribution
    detect_desktop_environment
    detect_init_system
    detect_package_manager
    detect_display_servers
    
    # Display system information
    echo -e "\n${BLUE}System Information:${NC}"
    echo "  Distribution: $DISTRO $DISTRO_VERSION"
    echo "  Desktop Environment: $DESKTOP_ENV"
    echo "  Init System: $INIT_SYSTEM"
    echo "  Package Manager: $PACKAGE_MANAGER"
    echo "  X11 Available: $X11_AVAILABLE"
    echo "  Wayland Available: $WAYLAND_AVAILABLE"
    
    # Check requirements
    echo ""
    if check_requirements; then
        echo -e "${GREEN}✅ All requirements met${NC}"
    else
        echo -e "${RED}❌ Some requirements missing${NC}"
        suggest_installation
    fi
    
    # Check compatibility
    if check_compatibility; then
        echo -e "\n${GREEN}✅ System is compatible with PhantomVault${NC}"
        return 0
    else
        echo -e "\n${YELLOW}⚠️  System has compatibility issues${NC}"
        echo "PhantomVault may work with reduced functionality"
        return 1
    fi
}

# Export functions and variables for use by other scripts
export -f detect_distribution detect_desktop_environment detect_init_system
export -f detect_package_manager detect_display_servers
export DISTRO DISTRO_VERSION DESKTOP_ENV INIT_SYSTEM PACKAGE_MANAGER
export X11_AVAILABLE WAYLAND_AVAILABLE

# Run main function if script is executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi