#!/bin/bash

# PhantomVault Universal Installation Script
# Automatically detects the best installation method for your system

set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}PhantomVault Universal Installer${NC}"
echo "================================="

# Check if running as root
if [[ $EUID -eq 0 ]]; then
   echo -e "${RED}Error: This script should not be run as root${NC}"
   echo "PhantomVault runs as a user service for security reasons"
   exit 1
fi

# Function to show installation options
show_options() {
    echo -e "\n${BLUE}Installation Options:${NC}"
    echo "1. Automatic (recommended) - Detect best method"
    echo "2. systemd service - For systemd-based systems"
    echo "3. Fallback method - For non-systemd or compatibility issues"
    echo "4. System detection only - Check compatibility without installing"
    echo "5. Exit"
    echo ""
}

# Function to run system detection
run_detection() {
    if [[ -f "$SCRIPT_DIR/detect-system.sh" ]]; then
        echo -e "${YELLOW}Running system detection...${NC}"
        source "$SCRIPT_DIR/detect-system.sh"
        return $?
    else
        echo -e "${RED}Error: System detection script not found${NC}"
        return 1
    fi
}

# Function to install with systemd
install_systemd() {
    if [[ -f "$SCRIPT_DIR/install-service.sh" ]]; then
        echo -e "${YELLOW}Installing with systemd...${NC}"
        exec "$SCRIPT_DIR/install-service.sh"
    else
        echo -e "${RED}Error: systemd installation script not found${NC}"
        return 1
    fi
}

# Function to install with fallback method
install_fallback() {
    if [[ -f "$SCRIPT_DIR/install-fallback.sh" ]]; then
        echo -e "${YELLOW}Installing with fallback method...${NC}"
        exec "$SCRIPT_DIR/install-fallback.sh"
    else
        echo -e "${RED}Error: Fallback installation script not found${NC}"
        return 1
    fi
}

# Function for automatic installation
install_automatic() {
    echo -e "${YELLOW}Detecting best installation method...${NC}"
    
    # Run system detection
    if run_detection; then
        # System is compatible, check for systemd
        if [[ "$INIT_SYSTEM" == "systemd" ]]; then
            echo -e "${GREEN}✅ systemd detected - using systemd installation${NC}"
            install_systemd
        else
            echo -e "${YELLOW}⚠️  Non-systemd system detected - using fallback installation${NC}"
            install_fallback
        fi
    else
        # System has compatibility issues
        echo -e "${YELLOW}⚠️  Compatibility issues detected${NC}"
        echo ""
        read -p "Try fallback installation? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            install_fallback
        else
            echo "Installation cancelled"
            exit 1
        fi
    fi
}

# Main installation loop
main() {
    while true; do
        show_options
        read -p "Choose an option (1-5): " -n 1 -r
        echo
        
        case $REPLY in
            1)
                install_automatic
                break
                ;;
            2)
                install_systemd
                break
                ;;
            3)
                install_fallback
                break
                ;;
            4)
                run_detection
                echo ""
                read -p "Press Enter to continue..." -r
                ;;
            5)
                echo "Installation cancelled"
                exit 0
                ;;
            *)
                echo -e "${RED}Invalid option. Please choose 1-5.${NC}"
                echo ""
                ;;
        esac
    done
}

# Check for command line arguments
case "${1:-}" in
    "--auto"|"-a")
        install_automatic
        ;;
    "--systemd"|"-s")
        install_systemd
        ;;
    "--fallback"|"-f")
        install_fallback
        ;;
    "--detect"|"-d")
        run_detection
        ;;
    "--help"|"-h")
        echo "PhantomVault Universal Installer"
        echo ""
        echo "Usage: $0 [OPTION]"
        echo ""
        echo "Options:"
        echo "  -a, --auto      Automatic installation (detect best method)"
        echo "  -s, --systemd   Force systemd installation"
        echo "  -f, --fallback  Force fallback installation"
        echo "  -d, --detect    Run system detection only"
        echo "  -h, --help      Show this help message"
        echo ""
        echo "If no option is provided, interactive mode will be used."
        ;;
    "")
        main
        ;;
    *)
        echo -e "${RED}Unknown option: $1${NC}"
        echo "Use --help for usage information"
        exit 1
        ;;
esac