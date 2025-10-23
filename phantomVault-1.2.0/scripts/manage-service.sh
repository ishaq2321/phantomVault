#!/bin/bash

# PhantomVault Service Management Script
# This script provides easy management of the PhantomVault background service

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SERVICE_NAME="phantom-vault"

# Function to show usage
show_usage() {
    echo -e "${BLUE}PhantomVault Service Management${NC}"
    echo "==============================="
    echo ""
    echo "Usage: $0 [COMMAND]"
    echo ""
    echo "Commands:"
    echo "  status    - Show service status"
    echo "  start     - Start the service"
    echo "  stop      - Stop the service"
    echo "  restart   - Restart the service"
    echo "  enable    - Enable auto-start on login"
    echo "  disable   - Disable auto-start on login"
    echo "  logs      - Show service logs (follow mode)"
    echo "  test      - Test service functionality"
    echo "  help      - Show this help message"
    echo ""
}

# Function to check if systemd is available
check_systemd() {
    if ! command -v systemctl &> /dev/null; then
        echo -e "${RED}Error: systemd not found${NC}"
        exit 1
    fi
}

# Function to show service status
show_status() {
    echo -e "${BLUE}PhantomVault Service Status${NC}"
    echo "==========================="
    
    if systemctl --user is-active --quiet "$SERVICE_NAME" 2>/dev/null; then
        echo -e "Status: ${GREEN}RUNNING${NC}"
    else
        echo -e "Status: ${RED}STOPPED${NC}"
    fi
    
    if systemctl --user is-enabled --quiet "$SERVICE_NAME" 2>/dev/null; then
        echo -e "Auto-start: ${GREEN}ENABLED${NC}"
    else
        echo -e "Auto-start: ${RED}DISABLED${NC}"
    fi
    
    echo ""
    systemctl --user status "$SERVICE_NAME" --no-pager -l 2>/dev/null || echo "Service not found"
}

# Function to start service
start_service() {
    echo -e "${YELLOW}Starting PhantomVault service...${NC}"
    if systemctl --user start "$SERVICE_NAME"; then
        echo -e "${GREEN}✅ Service started successfully${NC}"
    else
        echo -e "${RED}❌ Failed to start service${NC}"
        echo "Check logs with: $0 logs"
        exit 1
    fi
}

# Function to stop service
stop_service() {
    echo -e "${YELLOW}Stopping PhantomVault service...${NC}"
    if systemctl --user stop "$SERVICE_NAME"; then
        echo -e "${GREEN}✅ Service stopped successfully${NC}"
    else
        echo -e "${RED}❌ Failed to stop service${NC}"
        exit 1
    fi
}

# Function to restart service
restart_service() {
    echo -e "${YELLOW}Restarting PhantomVault service...${NC}"
    if systemctl --user restart "$SERVICE_NAME"; then
        echo -e "${GREEN}✅ Service restarted successfully${NC}"
    else
        echo -e "${RED}❌ Failed to restart service${NC}"
        echo "Check logs with: $0 logs"
        exit 1
    fi
}

# Function to enable auto-start
enable_service() {
    echo -e "${YELLOW}Enabling auto-start...${NC}"
    if systemctl --user enable "$SERVICE_NAME"; then
        echo -e "${GREEN}✅ Auto-start enabled${NC}"
        echo "Service will start automatically when you log in"
    else
        echo -e "${RED}❌ Failed to enable auto-start${NC}"
        exit 1
    fi
}

# Function to disable auto-start
disable_service() {
    echo -e "${YELLOW}Disabling auto-start...${NC}"
    if systemctl --user disable "$SERVICE_NAME"; then
        echo -e "${GREEN}✅ Auto-start disabled${NC}"
        echo "Service will not start automatically (but can still be started manually)"
    else
        echo -e "${RED}❌ Failed to disable auto-start${NC}"
        exit 1
    fi
}

# Function to show logs
show_logs() {
    echo -e "${BLUE}PhantomVault Service Logs${NC}"
    echo "========================="
    echo "Press Ctrl+C to exit log view"
    echo ""
    journalctl --user -u "$SERVICE_NAME" -f
}

# Function to test service functionality
test_service() {
    echo -e "${BLUE}PhantomVault Service Test${NC}"
    echo "========================="
    
    # Check if service is running
    if ! systemctl --user is-active --quiet "$SERVICE_NAME" 2>/dev/null; then
        echo -e "${RED}❌ Service is not running${NC}"
        echo "Start the service with: $0 start"
        exit 1
    fi
    
    echo -e "${GREEN}✅ Service is running${NC}"
    
    # Check if binary exists
    if [[ -f "$HOME/.local/bin/phantom_vault_service" ]]; then
        echo -e "${GREEN}✅ Service binary found${NC}"
    else
        echo -e "${RED}❌ Service binary not found${NC}"
    fi
    
    # Check directories
    if [[ -d "$HOME/.phantom_vault_storage" ]]; then
        echo -e "${GREEN}✅ Vault storage directory exists${NC}"
    else
        echo -e "${YELLOW}⚠️  Vault storage directory not found${NC}"
    fi
    
    # Show recent logs
    echo -e "\n${BLUE}Recent logs (last 10 lines):${NC}"
    journalctl --user -u "$SERVICE_NAME" -n 10 --no-pager 2>/dev/null || echo "No logs available"
    
    echo -e "\n${BLUE}Global Hotkeys:${NC}"
    echo "  Ctrl+Alt+V - Unlock/Lock folders"
    echo "  Ctrl+Alt+R - Recovery key input"
    echo ""
    echo -e "${YELLOW}Test the hotkeys to verify functionality${NC}"
}

# Main script logic
check_systemd

case "${1:-}" in
    "status")
        show_status
        ;;
    "start")
        start_service
        ;;
    "stop")
        stop_service
        ;;
    "restart")
        restart_service
        ;;
    "enable")
        enable_service
        ;;
    "disable")
        disable_service
        ;;
    "logs")
        show_logs
        ;;
    "test")
        test_service
        ;;
    "help"|"--help"|"-h")
        show_usage
        ;;
    "")
        show_status
        ;;
    *)
        echo -e "${RED}Error: Unknown command '$1'${NC}"
        echo ""
        show_usage
        exit 1
        ;;
esac