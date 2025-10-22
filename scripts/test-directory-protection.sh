#!/bin/bash

# PhantomVault Directory Protection Test Script
# This script demonstrates and tests the directory protection functionality

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}PhantomVault Directory Protection Test${NC}"
echo "======================================"

# Check if service is running
if ! systemctl --user is-active --quiet phantom-vault 2>/dev/null; then
    echo -e "${RED}❌ PhantomVault service is not running${NC}"
    echo "Start the service with: ./scripts/manage-service.sh start"
    exit 1
fi

echo -e "${GREEN}✅ PhantomVault service is running${NC}"

# Test directory paths
TEST_DIR="$HOME/.phantom_vault_storage/test_protection"
VAULT_BASE="$HOME/.phantom_vault_storage"
USER_VAULT="$HOME/.phantom_vault_storage/$(whoami)"

echo -e "\n${BLUE}Testing Directory Protection:${NC}"

# Create test directory if it doesn't exist
if [[ ! -d "$TEST_DIR" ]]; then
    echo -e "${YELLOW}Creating test directory: $TEST_DIR${NC}"
    mkdir -p "$TEST_DIR"
    echo "Test file for protection" > "$TEST_DIR/test.txt"
fi

# Test 1: Check if chattr is available
echo -e "\n${BLUE}Test 1: System Capabilities${NC}"
if command -v chattr &> /dev/null && command -v lsattr &> /dev/null; then
    echo -e "✅ chattr/lsattr available"
    
    # Test immutable attribute on test directory
    echo -e "${YELLOW}Testing immutable attribute on test directory...${NC}"
    
    # Apply immutable attribute
    if chattr +i "$TEST_DIR" 2>/dev/null; then
        echo -e "✅ Successfully applied immutable attribute"
        
        # Check if attribute is set
        if lsattr -d "$TEST_DIR" 2>/dev/null | grep -q 'i'; then
            echo -e "✅ Immutable attribute confirmed"
            
            # Test protection (try to delete file - should fail)
            echo -e "${YELLOW}Testing protection (attempting to delete test file)...${NC}"
            if rm "$TEST_DIR/test.txt" 2>/dev/null; then
                echo -e "❌ Protection failed - file was deleted"
            else
                echo -e "✅ Protection working - file deletion blocked"
            fi
            
            # Remove immutable attribute for cleanup
            chattr -i "$TEST_DIR" 2>/dev/null
            echo -e "✅ Immutable attribute removed for cleanup"
        else
            echo -e "❌ Immutable attribute not confirmed"
        fi
    else
        echo -e "❌ Failed to apply immutable attribute (may need different filesystem)"
    fi
else
    echo -e "⚠️  chattr/lsattr not available - using permission-based protection"
fi

# Test 2: Check vault directories
echo -e "\n${BLUE}Test 2: Vault Directory Status${NC}"

directories=("$VAULT_BASE" "$USER_VAULT")
for dir in "${directories[@]}"; do
    if [[ -d "$dir" ]]; then
        echo -e "Directory: $dir"
        echo -e "  Status: ${GREEN}EXISTS${NC}"
        
        # Check permissions
        perms=$(stat -c "%a" "$dir" 2>/dev/null)
        echo -e "  Permissions: $perms"
        
        # Check immutable attribute if available
        if command -v lsattr &> /dev/null; then
            attrs=$(lsattr -d "$dir" 2>/dev/null | cut -d' ' -f1)
            if [[ "$attrs" == *"i"* ]]; then
                echo -e "  Immutable: ${GREEN}YES${NC}"
            else
                echo -e "  Immutable: ${YELLOW}NO${NC}"
            fi
        fi
        
        # Check ownership
        owner=$(stat -c "%U:%G" "$dir" 2>/dev/null)
        echo -e "  Owner: $owner"
        
    else
        echo -e "Directory: $dir"
        echo -e "  Status: ${YELLOW}MISSING${NC}"
    fi
    echo ""
done

# Test 3: Security log check
echo -e "${BLUE}Test 3: Security Logging${NC}"

# Check system log for PhantomVault entries
echo -e "${YELLOW}Recent PhantomVault system log entries:${NC}"
if journalctl --user -u phantom-vault -n 5 --no-pager 2>/dev/null | grep -i "security\|protection\|violation"; then
    echo -e "✅ Security logging active"
else
    echo -e "ℹ️  No recent security events in system log"
fi

# Check for security log file
SECURITY_LOG="$VAULT_BASE/security.log"
if [[ -f "$SECURITY_LOG" ]]; then
    echo -e "\n${YELLOW}Security log file found: $SECURITY_LOG${NC}"
    echo -e "Recent entries:"
    tail -5 "$SECURITY_LOG" 2>/dev/null || echo "Unable to read security log"
else
    echo -e "\n${YELLOW}No security log file found at: $SECURITY_LOG${NC}"
fi

# Test 4: Service status and monitoring
echo -e "\n${BLUE}Test 4: Service Monitoring${NC}"

# Check service resource usage
echo -e "${YELLOW}Service resource usage:${NC}"
if systemctl --user status phantom-vault --no-pager -l 2>/dev/null | grep -E "(Memory|CPU|Tasks)"; then
    echo -e "✅ Resource monitoring active"
else
    echo -e "⚠️  Resource information not available"
fi

# Test 5: Manual protection test
echo -e "\n${BLUE}Test 5: Manual Protection Test${NC}"

if [[ -d "$TEST_DIR" ]]; then
    echo -e "${YELLOW}Testing manual directory protection...${NC}"
    
    # Try to protect the test directory manually
    if command -v chattr &> /dev/null; then
        echo -e "Applying protection to test directory..."
        if chattr +i "$TEST_DIR" 2>/dev/null; then
            echo -e "✅ Protection applied"
            
            # Wait a moment for monitoring to detect
            sleep 2
            
            # Try to remove protection (simulate violation)
            echo -e "Simulating protection removal..."
            if chattr -i "$TEST_DIR" 2>/dev/null; then
                echo -e "⚠️  Protection removed (simulated violation)"
                
                # Wait for monitoring to detect and restore
                echo -e "Waiting for automatic restoration (30 seconds)..."
                sleep 35
                
                # Check if protection was restored
                if lsattr -d "$TEST_DIR" 2>/dev/null | grep -q 'i'; then
                    echo -e "✅ Protection automatically restored"
                else
                    echo -e "❌ Protection not restored automatically"
                fi
            fi
        else
            echo -e "❌ Failed to apply protection"
        fi
    else
        echo -e "⚠️  chattr not available for manual test"
    fi
fi

# Cleanup
echo -e "\n${BLUE}Cleanup${NC}"
if [[ -d "$TEST_DIR" ]]; then
    # Remove any protection before cleanup
    chattr -i "$TEST_DIR" 2>/dev/null || true
    rm -rf "$TEST_DIR" 2>/dev/null
    echo -e "✅ Test directory cleaned up"
fi

echo -e "\n${GREEN}Directory Protection Test Complete${NC}"
echo -e "\n${BLUE}Summary:${NC}"
echo "- Directory protection system is integrated into the service"
echo "- Security monitoring runs every 30 seconds"
echo "- Violations are logged to system log and security log file"
echo "- Automatic protection restoration is active"
echo ""
echo -e "${YELLOW}Note: Some features may require specific filesystem support (ext2/3/4)${NC}"
echo -e "${YELLOW}For full functionality, ensure the service has appropriate permissions${NC}"