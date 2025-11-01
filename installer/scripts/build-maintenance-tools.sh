#!/bin/bash

# PhantomVault Maintenance Tools Builder
# Creates diagnostic and maintenance utilities with update mechanism

set -e

echo "Building maintenance and diagnostic tools..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() {
    echo -e "${BLUE}[TOOLS]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "../../README.md" ]; then
    print_error "Please run from installer/scripts directory"
    exit 1
fi

# Create tools directory
mkdir -p ../build/tools

print_status "Creating comprehensive diagnostic tool..."

# Create comprehensive diagnostic tool
cat > ../build/tools/phantomvault-diagnostic.sh << 'EOF'
#!/bin/bash

# PhantomVault Diagnostic Tool
# Comprehensive system analysis and troubleshooting

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_header() {
    echo -e "${BLUE}=== $1 ===${NC}"
}

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[OK]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

echo "PhantomVault Diagnostic Tool v1.0.0"
echo "===================================="
echo "WARNING: This software is in continuous development."
echo "Not yet tested on Windows and Mac. Use at your own risk."
echo

# System Information
print_header "System Information"
print_status "OS: $(uname -s) $(uname -r)"
print_status "Architecture: $(uname -m)"
print_status "User: $(whoami)"
print_status "Date: $(date)"
print_status "Hostname: $(hostname)"
echo

# Check PhantomVault Installation
print_header "PhantomVault Installation"

if [ -f "/usr/bin/phantomvault" ]; then
    print_success "PhantomVault executable found: /usr/bin/phantomvault"
    
    # Check version
    if /usr/bin/phantomvault --version &>/dev/null; then
        VERSION=$(/usr/bin/phantomvault --version 2>/dev/null | head -1)
        print_success "Version: $VERSION"
    else
        print_error "Cannot get version information"
    fi
else
    print_error "PhantomVault executable not found"
fi

if [ -f "/opt/phantomvault/bin/phantomvault" ]; then
    print_success "Service binary found: /opt/phantomvault/bin/phantomvault"
    
    # Check binary integrity
    if file "/opt/phantomvault/bin/phantomvault" | grep -q "ELF"; then
        print_success "Binary is valid ELF executable"
    else
        print_error "Binary appears corrupted"
    fi
else
    print_warning "Service binary not found in /opt/phantomvault/"
fi

# Check uninstaller
if [ -f "/opt/phantomvault/uninstall.sh" ]; then
    print_success "Uninstaller found: /opt/phantomvault/uninstall.sh"
else
    print_warning "Uninstaller not found"
fi

echo

# Check Service Status
print_header "Service Status"

if systemctl is-active --quiet phantomvault.service 2>/dev/null; then
    print_success "PhantomVault service is running"
    
    # Get service PID and memory usage
    PID=$(systemctl show phantomvault.service -p MainPID --value 2>/dev/null)
    if [ -n "$PID" ] && [ "$PID" != "0" ]; then
        MEMORY=$(ps -p "$PID" -o rss= 2>/dev/null | awk '{print int($1/1024)"MB"}')
        print_status "Service PID: $PID, Memory: $MEMORY"
    fi
else
    print_warning "PhantomVault service is not running"
fi

if systemctl is-enabled --quiet phantomvault.service 2>/dev/null; then
    print_success "PhantomVault service is enabled"
else
    print_warning "PhantomVault service is not enabled"
fi

# Show service logs
print_status "Recent service logs (last 5 entries):"
journalctl -u phantomvault.service --no-pager -n 5 --output=short 2>/dev/null | sed 's/^/  /' || print_warning "Cannot access service logs"
echo

# Check Dependencies
print_header "Dependencies"

check_dependency() {
    if command -v "$1" &> /dev/null; then
        VERSION_INFO=$($1 --version 2>/dev/null | head -1 || echo "unknown version")
        print_success "$1 is available ($VERSION_INFO)"
    else
        print_error "$1 is missing"
    fi
}

check_library() {
    if ldconfig -p 2>/dev/null | grep -q "$1"; then
        print_success "Library $1 is available"
    else
        print_error "Library $1 is missing"
    fi
}

check_dependency "systemctl"
check_library "libssl"
check_library "libX11"
check_library "libXtst"
check_library "libargon2"

# Check X11 display
if [ -n "$DISPLAY" ]; then
    print_success "X11 display available: $DISPLAY"
else
    print_warning "No X11 display detected (GUI may not work)"
fi

echo

# Check User Data
print_header "User Data"

USER_DATA_DIR="$HOME/.phantomvault"
if [ -d "$USER_DATA_DIR" ]; then
    print_success "User data directory exists: $USER_DATA_DIR"
    
    # Check directory size
    SIZE=$(du -sh "$USER_DATA_DIR" 2>/dev/null | cut -f1)
    print_status "Directory size: $SIZE"
    
    # Check directory contents
    if [ "$(ls -A "$USER_DATA_DIR" 2>/dev/null)" ]; then
        print_status "Directory contents:"
        ls -la "$USER_DATA_DIR" 2>/dev/null | sed 's/^/  /'
        
        # Check for profiles
        if [ -d "$USER_DATA_DIR/profiles" ]; then
            PROFILE_COUNT=$(find "$USER_DATA_DIR/profiles" -maxdepth 1 -type d | wc -l)
            print_status "Profiles found: $((PROFILE_COUNT - 1))"
        fi
        
        # Check for vaults
        if [ -d "$USER_DATA_DIR/vaults" ]; then
            VAULT_COUNT=$(find "$USER_DATA_DIR/vaults" -maxdepth 1 -type d | wc -l)
            print_status "Vaults found: $((VAULT_COUNT - 1))"
        fi
    else
        print_warning "User data directory is empty"
    fi
else
    print_warning "User data directory does not exist: $USER_DATA_DIR"
fi
echo

# Check Permissions
print_header "Permissions"

if [ -f "/opt/phantomvault/bin/phantomvault" ]; then
    PERMS=$(stat -c "%a" "/opt/phantomvault/bin/phantomvault" 2>/dev/null)
    OWNER=$(stat -c "%U:%G" "/opt/phantomvault/bin/phantomvault" 2>/dev/null)
    if [ "$PERMS" = "4755" ]; then
        print_success "Service binary has correct permissions (4755, $OWNER)"
    else
        print_warning "Service binary permissions: $PERMS ($OWNER) - expected: 4755"
    fi
fi

# Check if user is in phantomvault group
if groups 2>/dev/null | grep -q phantomvault; then
    print_success "Current user is in phantomvault group"
else
    print_warning "Current user is not in phantomvault group"
fi

# Check sudo access
if sudo -n true 2>/dev/null; then
    print_success "User has sudo access"
else
    print_warning "User does not have sudo access (may be required for some operations)"
fi

echo

# Network Connectivity
print_header "Network Connectivity"

# Check IPC port
if netstat -tlnp 2>/dev/null | grep -q ":9876"; then
    print_success "PhantomVault IPC server is listening on port 9876"
else
    print_warning "PhantomVault IPC server is not listening on port 9876"
fi

# Check internet connectivity for updates
if ping -c 1 8.8.8.8 &>/dev/null; then
    print_success "Internet connectivity available"
else
    print_warning "No internet connectivity (updates may not work)"
fi

echo

# Performance Check
print_header "Performance"

print_status "Memory usage:"
if command -v free &> /dev/null; then
    free -h | sed 's/^/  /'
else
    print_warning "free command not available"
fi

print_status "Disk usage (root filesystem):"
df -h / 2>/dev/null | sed 's/^/  /' || print_warning "Cannot check disk usage"

print_status "System load:"
if [ -f /proc/loadavg ]; then
    LOAD=$(cat /proc/loadavg | cut -d' ' -f1-3)
    print_status "Load average: $LOAD"
else
    uptime | sed 's/^/  /'
fi

# Check CPU info
if [ -f /proc/cpuinfo ]; then
    CPU_MODEL=$(grep "model name" /proc/cpuinfo | head -1 | cut -d: -f2 | xargs)
    CPU_CORES=$(grep -c "^processor" /proc/cpuinfo)
    print_status "CPU: $CPU_MODEL ($CPU_CORES cores)"
fi

echo

# Configuration Files
print_header "Configuration"

CONFIG_FILES=(
    "/etc/systemd/system/phantomvault.service"
    "/usr/share/applications/phantomvault.desktop"
    "$HOME/.phantomvault/config.json"
    "/opt/phantomvault/uninstall.sh"
)

for config in "${CONFIG_FILES[@]}"; do
    if [ -f "$config" ]; then
        SIZE=$(stat -c%s "$config" 2>/dev/null)
        print_success "Configuration file exists: $config ($SIZE bytes)"
    else
        print_warning "Configuration file missing: $config"
    fi
done
echo

# Security Check
print_header "Security"

# Check for running as root
if [ "$EUID" -eq 0 ]; then
    print_warning "Running diagnostic as root (not recommended for regular use)"
else
    print_success "Running diagnostic as regular user"
fi

# Check file permissions on sensitive directories
if [ -d "$USER_DATA_DIR" ]; then
    PERMS=$(stat -c "%a" "$USER_DATA_DIR" 2>/dev/null)
    if [ "$PERMS" = "700" ] || [ "$PERMS" = "750" ]; then
        print_success "User data directory has secure permissions ($PERMS)"
    else
        print_warning "User data directory permissions: $PERMS (consider 700 for better security)"
    fi
fi

echo

# Recommendations
print_header "Recommendations"

RECOMMENDATIONS=()

if ! systemctl is-active --quiet phantomvault.service 2>/dev/null; then
    RECOMMENDATIONS+=("Start the service: sudo systemctl start phantomvault")
fi

if ! systemctl is-enabled --quiet phantomvault.service 2>/dev/null; then
    RECOMMENDATIONS+=("Enable auto-start: sudo systemctl enable phantomvault")
fi

if [ ! -d "$USER_DATA_DIR" ]; then
    RECOMMENDATIONS+=("Initialize user data: phantomvault --cli init")
fi

if ! groups 2>/dev/null | grep -q phantomvault; then
    RECOMMENDATIONS+=("Add user to phantomvault group: sudo usermod -a -G phantomvault \$(whoami)")
fi

if [ ${#RECOMMENDATIONS[@]} -eq 0 ]; then
    print_success "No immediate recommendations - system appears healthy"
else
    for rec in "${RECOMMENDATIONS[@]}"; do
        print_status "$rec"
    done
fi

echo
print_header "Diagnostic Complete"
print_status "PhantomVault v1.0.0 - MIT License"
print_status "Contact: ishaq2321@proton.me"
print_status "Contributions welcome at: https://github.com/phantomvault/phantomvault"
print_warning "Remember: This software is in continuous development"
print_warning "Not yet tested on Windows and Mac - use at your own risk"
print_status "Include this diagnostic output with any support requests"

EOF

chmod +x ../build/tools/phantomvault-diagnostic.sh

print_success "Diagnostic tool created: ../build/tools/phantomvault-diagnostic.sh"

# Create system cleanup tool
print_status "Creating cleanup tool..."

cat > ../build/tools/phantomvault-cleanup.sh << 'EOF'
#!/bin/bash

# PhantomVault System Cleanup Tool
# Safely cleans temporary files and optimizes performance

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() {
    echo -e "${BLUE}[CLEANUP]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

echo "PhantomVault System Cleanup Tool v1.0.0"
echo "========================================"
echo "WARNING: This software is in continuous development."
echo "Not yet tested on Windows and Mac. Use at your own risk."
echo

# Check if running as root for system-wide cleanup
if [ "$EUID" -eq 0 ]; then
    SYSTEM_CLEANUP=true
    print_status "Running as root - performing system-wide cleanup"
else
    SYSTEM_CLEANUP=false
    print_status "Running as user - performing user-specific cleanup"
fi

CLEANED_SIZE=0

# Function to calculate size and add to total
add_cleaned_size() {
    if [ -n "$1" ] && [ "$1" -gt 0 ]; then
        CLEANED_SIZE=$((CLEANED_SIZE + $1))
    fi
}

# Clean user temporary files
print_status "Cleaning user temporary files..."

USER_DATA_DIR="$HOME/.phantomvault"
if [ -d "$USER_DATA_DIR" ]; then
    # Clean temporary files
    TEMP_SIZE=$(find "$USER_DATA_DIR" -name "*.tmp" -type f -exec du -cb {} + 2>/dev/null | tail -1 | cut -f1 || echo 0)
    find "$USER_DATA_DIR" -name "*.tmp" -type f -delete 2>/dev/null || true
    add_cleaned_size "$TEMP_SIZE"
    
    # Clean old log files (keep last 7 days)
    LOG_SIZE=$(find "$USER_DATA_DIR" -name "*.log.*" -type f -mtime +7 -exec du -cb {} + 2>/dev/null | tail -1 | cut -f1 || echo 0)
    find "$USER_DATA_DIR" -name "*.log.*" -type f -mtime +7 -delete 2>/dev/null || true
    add_cleaned_size "$LOG_SIZE"
    
    # Clean old analytics data (keep last 30 days)
    if [ -d "$USER_DATA_DIR/analytics" ]; then
        ANALYTICS_SIZE=$(find "$USER_DATA_DIR/analytics" -name "*.dat" -type f -mtime +30 -exec du -cb {} + 2>/dev/null | tail -1 | cut -f1 || echo 0)
        find "$USER_DATA_DIR/analytics" -name "*.dat" -type f -mtime +30 -delete 2>/dev/null || true
        add_cleaned_size "$ANALYTICS_SIZE"
    fi
    
    # Clean backup files older than 30 days
    if [ -d "$USER_DATA_DIR/backups" ]; then
        BACKUP_SIZE=$(find "$USER_DATA_DIR/backups" -name "*.bak" -type f -mtime +30 -exec du -cb {} + 2>/dev/null | tail -1 | cut -f1 || echo 0)
        find "$USER_DATA_DIR/backups" -name "*.bak" -type f -mtime +30 -delete 2>/dev/null || true
        add_cleaned_size "$BACKUP_SIZE"
    fi
    
    print_success "User temporary files cleaned"
else
    print_warning "User data directory not found"
fi

# Clean system logs (if running as root)
if [ "$SYSTEM_CLEANUP" = true ]; then
    print_status "Cleaning system logs..."
    
    # Clean journal logs older than 7 days
    JOURNAL_SIZE_BEFORE=$(journalctl --disk-usage 2>/dev/null | grep -o '[0-9.]*[KMGT]B' | head -1 || echo "0B")
    journalctl --vacuum-time=7d 2>/dev/null || true
    JOURNAL_SIZE_AFTER=$(journalctl --disk-usage 2>/dev/null | grep -o '[0-9.]*[KMGT]B' | head -1 || echo "0B")
    
    # Clean PhantomVault specific logs
    if [ -d "/var/log/phantomvault" ]; then
        SYSTEM_LOG_SIZE=$(find "/var/log/phantomvault" -name "*.log" -type f -mtime +7 -exec du -cb {} + 2>/dev/null | tail -1 | cut -f1 || echo 0)
        find "/var/log/phantomvault" -name "*.log" -type f -mtime +7 -delete 2>/dev/null || true
        add_cleaned_size "$SYSTEM_LOG_SIZE"
    fi
    
    print_success "System logs cleaned (journal: $JOURNAL_SIZE_BEFORE -> $JOURNAL_SIZE_AFTER)"
fi

# Optimize database files
print_status "Optimizing database files..."

if [ -d "$USER_DATA_DIR/profiles" ]; then
    # Compact profile databases
    DB_COUNT=0
    for db in "$USER_DATA_DIR/profiles"/*.db; do
        if [ -f "$db" ]; then
            sqlite3 "$db" "VACUUM;" 2>/dev/null || true
            DB_COUNT=$((DB_COUNT + 1))
        fi
    done
    
    if [ $DB_COUNT -gt 0 ]; then
        print_success "Optimized $DB_COUNT profile databases"
    else
        print_status "No profile databases found to optimize"
    fi
fi

# Clean package manager cache (if running as root)
if [ "$SYSTEM_CLEANUP" = true ]; then
    print_status "Cleaning package manager cache..."
    
    if command -v apt-get &> /dev/null; then
        APT_SIZE_BEFORE=$(du -sh /var/cache/apt/archives 2>/dev/null | cut -f1 || echo "0")
        apt-get clean 2>/dev/null || true
        apt-get autoclean 2>/dev/null || true
        APT_SIZE_AFTER=$(du -sh /var/cache/apt/archives 2>/dev/null | cut -f1 || echo "0")
        print_success "APT cache cleaned ($APT_SIZE_BEFORE -> $APT_SIZE_AFTER)"
    fi
    
    if command -v yum &> /dev/null; then
        yum clean all 2>/dev/null || true
        print_success "YUM cache cleaned"
    fi
    
    if command -v dnf &> /dev/null; then
        dnf clean all 2>/dev/null || true
        print_success "DNF cache cleaned"
    fi
fi

# Memory optimization
print_status "Performing memory optimization..."

# Clear page cache (if running as root)
if [ "$SYSTEM_CLEANUP" = true ]; then
    MEMORY_BEFORE=$(free -m | awk 'NR==2{printf "%.1fGB", $3/1024}')
    sync
    echo 1 > /proc/sys/vm/drop_caches 2>/dev/null || true
    sleep 1
    MEMORY_AFTER=$(free -m | awk 'NR==2{printf "%.1fGB", $3/1024}')
    print_success "System memory cache cleared ($MEMORY_BEFORE -> $MEMORY_AFTER used)"
fi

# Restart PhantomVault service for optimization
if [ "$SYSTEM_CLEANUP" = true ] && systemctl is-active --quiet phantomvault.service 2>/dev/null; then
    print_status "Restarting PhantomVault service for optimization..."
    systemctl restart phantomvault.service
    sleep 3
    
    if systemctl is-active --quiet phantomvault.service 2>/dev/null; then
        print_success "PhantomVault service restarted successfully"
    else
        print_error "Failed to restart PhantomVault service"
    fi
fi

# Clean thumbnail cache
print_status "Cleaning thumbnail cache..."
if [ -d "$HOME/.cache/thumbnails" ]; then
    THUMB_SIZE=$(du -sh "$HOME/.cache/thumbnails" 2>/dev/null | cut -f1 || echo "0")
    find "$HOME/.cache/thumbnails" -type f -mtime +30 -delete 2>/dev/null || true
    print_success "Thumbnail cache cleaned (was $THUMB_SIZE)"
fi

# Clean browser cache (optional)
if [ "$1" = "--deep" ]; then
    print_status "Performing deep cleanup (browser caches)..."
    
    # Firefox cache
    if [ -d "$HOME/.mozilla/firefox" ]; then
        find "$HOME/.mozilla/firefox" -name "Cache*" -type d -exec rm -rf {} + 2>/dev/null || true
        print_success "Firefox cache cleaned"
    fi
    
    # Chrome cache
    if [ -d "$HOME/.cache/google-chrome" ]; then
        rm -rf "$HOME/.cache/google-chrome/Default/Cache" 2>/dev/null || true
        print_success "Chrome cache cleaned"
    fi
fi

echo
print_success "System cleanup completed successfully!"

# Show summary
if [ $CLEANED_SIZE -gt 0 ]; then
    CLEANED_MB=$((CLEANED_SIZE / 1024 / 1024))
    if [ $CLEANED_MB -gt 0 ]; then
        print_status "Total space cleaned: ${CLEANED_MB}MB"
    else
        CLEANED_KB=$((CLEANED_SIZE / 1024))
        print_status "Total space cleaned: ${CLEANED_KB}KB"
    fi
fi

# Show current disk usage
print_status "Current disk usage:"
df -h / | grep -v Filesystem | awk '{print "  Root filesystem: " $3 " used, " $4 " available (" $5 " full)"}'

print_status "Cleanup options:"
print_status "  --deep    Include browser caches and more aggressive cleanup"
print_status "Contact: ishaq2321@proton.me | License: MIT"

EOF

chmod +x ../build/tools/phantomvault-cleanup.sh

print_success "Cleanup tool created: ../build/tools/phantomvault-cleanup.sh"

# Create update checker tool with automatic update mechanism
print_status "Creating update checker with automatic update mechanism..."

cat > ../build/tools/phantomvault-update-checker.sh << 'EOF'
#!/bin/bash

# PhantomVault Update Checker
# Checks for available updates and provides automatic update mechanism

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_status() {
    echo -e "${BLUE}[UPDATE]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

echo "PhantomVault Update Checker v1.0.0"
echo "==================================="
echo "WARNING: This software is in continuous development."
echo "Not yet tested on Windows and Mac. Use at your own risk."
echo

# Configuration
GITHUB_REPO="phantomvault/phantomvault"
GITHUB_API="https://api.github.com/repos/$GITHUB_REPO/releases/latest"
UPDATE_CHECK_FILE="$HOME/.phantomvault/last_update_check"

# Get current version
if command -v phantomvault &> /dev/null; then
    CURRENT_VERSION=$(phantomvault --version 2>/dev/null | head -1 | grep -o 'v[0-9]\+\.[0-9]\+\.[0-9]\+' || echo "v1.0.0")
    print_status "Current version: $CURRENT_VERSION"
else
    print_error "PhantomVault not found"
    exit 1
fi

# Check internet connectivity
print_status "Checking internet connectivity..."
if ! ping -c 1 8.8.8.8 &>/dev/null; then
    print_error "No internet connection available"
    print_status "Cannot check for updates without internet connectivity"
    exit 1
fi

print_success "Internet connectivity confirmed"

# Check for updates
print_status "Checking for updates from GitHub..."

# Create update check directory
mkdir -p "$(dirname "$UPDATE_CHECK_FILE")"

# Check if we should skip update check (last check was recent)
if [ -f "$UPDATE_CHECK_FILE" ]; then
    LAST_CHECK=$(cat "$UPDATE_CHECK_FILE" 2>/dev/null || echo "0")
    CURRENT_TIME=$(date +%s)
    TIME_DIFF=$((CURRENT_TIME - LAST_CHECK))
    
    # Skip if checked within last hour (unless forced)
    if [ $TIME_DIFF -lt 3600 ] && [ "$1" != "--force" ]; then
        print_status "Update check performed recently. Use --force to check again."
        exit 0
    fi
fi

# Fetch latest release info from GitHub API
LATEST_INFO=$(curl -s "$GITHUB_API" 2>/dev/null || echo "")

if [ -z "$LATEST_INFO" ]; then
    print_warning "Unable to fetch update information from GitHub"
    print_status "This may be due to rate limiting or network issues"
    print_status "Manual check: https://github.com/$GITHUB_REPO/releases"
    exit 1
fi

# Parse latest version
LATEST_VERSION=$(echo "$LATEST_INFO" | grep '"tag_name"' | cut -d'"' -f4 || echo "")

if [ -z "$LATEST_VERSION" ]; then
    print_warning "Unable to parse version information"
    exit 1
fi

print_status "Latest version: $LATEST_VERSION"

# Record this check
echo "$(date +%s)" > "$UPDATE_CHECK_FILE"

# Compare versions
if [ "$CURRENT_VERSION" = "$LATEST_VERSION" ]; then
    print_success "PhantomVault is up to date!"
    
    # Show additional info
    print_status "Release information:"
    RELEASE_NAME=$(echo "$LATEST_INFO" | grep '"name"' | head -1 | cut -d'"' -f4 || echo "")
    RELEASE_DATE=$(echo "$LATEST_INFO" | grep '"published_at"' | cut -d'"' -f4 | cut -d'T' -f1 || echo "")
    
    if [ -n "$RELEASE_NAME" ]; then
        print_status "  Release: $RELEASE_NAME"
    fi
    if [ -n "$RELEASE_DATE" ]; then
        print_status "  Published: $RELEASE_DATE"
    fi
    
else
    print_warning "Update available: $CURRENT_VERSION -> $LATEST_VERSION"
    echo
    
    # Show release notes
    RELEASE_BODY=$(echo "$LATEST_INFO" | grep '"body"' | cut -d'"' -f4 | sed 's/\\n/\n/g' | head -10)
    if [ -n "$RELEASE_BODY" ]; then
        print_status "Release notes:"
        echo "$RELEASE_BODY" | sed 's/^/  /'
        echo
    fi
    
    # Get download URL for Linux DEB package
    DOWNLOAD_URL=$(echo "$LATEST_INFO" | grep '"browser_download_url"' | grep '\.deb"' | head -1 | cut -d'"' -f4 || echo "")
    
    if [ -n "$DOWNLOAD_URL" ]; then
        print_status "Update package available: $(basename "$DOWNLOAD_URL")"
        
        # Offer automatic update
        if [ "$1" = "--auto" ] || [ "$1" = "--install" ]; then
            perform_update "$DOWNLOAD_URL" "$LATEST_VERSION"
        else
            echo
            print_status "To update PhantomVault:"
            print_status "1. Automatic update: $0 --install"
            print_status "2. Manual download: $DOWNLOAD_URL"
            print_status "3. GitHub releases: https://github.com/$GITHUB_REPO/releases"
            echo
            print_warning "Your data and settings will be preserved during update"
        fi
    else
        print_warning "No Linux package found in latest release"
        print_status "Visit: https://github.com/$GITHUB_REPO/releases"
    fi
fi

# Function to perform automatic update
perform_update() {
    local download_url="$1"
    local new_version="$2"
    
    print_status "Starting automatic update to $new_version..."
    
    # Check if running as root
    if [ "$EUID" -ne 0 ]; then
        print_error "Automatic update requires root privileges"
        print_status "Please run: sudo $0 --install"
        exit 1
    fi
    
    # Create temporary directory
    TEMP_DIR=$(mktemp -d)
    trap "rm -rf $TEMP_DIR" EXIT
    
    # Download the package
    print_status "Downloading update package..."
    PACKAGE_FILE="$TEMP_DIR/$(basename "$download_url")"
    
    if curl -L -o "$PACKAGE_FILE" "$download_url"; then
        print_success "Package downloaded successfully"
    else
        print_error "Failed to download update package"
        exit 1
    fi
    
    # Verify package
    if file "$PACKAGE_FILE" | grep -q "Debian binary package"; then
        print_success "Package verification passed"
    else
        print_error "Downloaded file is not a valid Debian package"
        exit 1
    fi
    
    # Stop service before update
    print_status "Stopping PhantomVault service..."
    systemctl stop phantomvault.service 2>/dev/null || true
    
    # Install the package
    print_status "Installing update..."
    if dpkg -i "$PACKAGE_FILE"; then
        print_success "Update installed successfully!"
        
        # Start service
        print_status "Starting PhantomVault service..."
        systemctl start phantomvault.service
        
        # Verify installation
        sleep 2
        NEW_INSTALLED_VERSION=$(phantomvault --version 2>/dev/null | head -1 | grep -o 'v[0-9]\+\.[0-9]\+\.[0-9]\+' || echo "unknown")
        
        if [ "$NEW_INSTALLED_VERSION" = "$new_version" ]; then
            print_success "Update completed successfully!"
            print_status "PhantomVault updated from $CURRENT_VERSION to $NEW_INSTALLED_VERSION"
        else
            print_warning "Update may not have completed correctly"
            print_status "Expected: $new_version, Got: $NEW_INSTALLED_VERSION"
        fi
        
    else
        print_error "Failed to install update package"
        print_status "You may need to fix dependency issues:"
        print_status "  sudo apt-get install -f"
        exit 1
    fi
}

# Check system dependencies for updates
print_status "Checking system dependencies..."

DEPS_OK=true

check_dependency() {
    if command -v "$1" &> /dev/null; then
        print_success "$1 is available"
    else
        print_warning "$1 is missing or outdated"
        DEPS_OK=false
    fi
}

check_dependency "systemctl"
check_dependency "curl"
check_dependency "dpkg"

# Check libraries
if ldconfig -p 2>/dev/null | grep -q "libssl"; then
    print_success "OpenSSL library is available"
else
    print_warning "OpenSSL library is missing"
    DEPS_OK=false
fi

if [ "$DEPS_OK" = true ]; then
    print_success "All dependencies are satisfied"
else
    print_warning "Some dependencies need attention"
    print_status "Update system packages: sudo apt update && sudo apt upgrade"
fi

echo
print_status "Update check completed"
print_status "Options:"
print_status "  --force     Force update check (ignore recent check)"
print_status "  --install   Automatically download and install updates"
print_status "  --auto      Same as --install"
echo
print_status "PhantomVault v1.0.0 - MIT License"
print_status "Contact: ishaq2321@proton.me"
print_status "GitHub: https://github.com/$GITHUB_REPO"

EOF

chmod +x ../build/tools/phantomvault-update-checker.sh

print_success "Update checker created: ../build/tools/phantomvault-update-checker.sh"

# Create a comprehensive maintenance script that runs all tools
print_status "Creating comprehensive maintenance script..."

cat > ../build/tools/phantomvault-maintenance.sh << 'EOF'
#!/bin/bash

# PhantomVault Comprehensive Maintenance Tool
# Runs all maintenance tasks in sequence

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_header() {
    echo -e "${BLUE}=== $1 ===${NC}"
}

print_status() {
    echo -e "${BLUE}[MAINTENANCE]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

echo "PhantomVault Comprehensive Maintenance Tool v1.0.0"
echo "=================================================="
echo "WARNING: This software is in continuous development."
echo "Not yet tested on Windows and Mac. Use at your own risk."
echo

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Check if other maintenance tools exist
DIAGNOSTIC_TOOL="$SCRIPT_DIR/phantomvault-diagnostic.sh"
CLEANUP_TOOL="$SCRIPT_DIR/phantomvault-cleanup.sh"
UPDATE_TOOL="$SCRIPT_DIR/phantomvault-update-checker.sh"

print_status "Starting comprehensive maintenance routine..."
echo

# 1. Run diagnostic
if [ -f "$DIAGNOSTIC_TOOL" ]; then
    print_header "System Diagnostic"
    bash "$DIAGNOSTIC_TOOL"
    echo
else
    print_warning "Diagnostic tool not found: $DIAGNOSTIC_TOOL"
fi

# 2. Check for updates
if [ -f "$UPDATE_TOOL" ]; then
    print_header "Update Check"
    bash "$UPDATE_TOOL"
    echo
else
    print_warning "Update tool not found: $UPDATE_TOOL"
fi

# 3. Run cleanup
if [ -f "$CLEANUP_TOOL" ]; then
    print_header "System Cleanup"
    
    # Ask for confirmation for cleanup
    if [ "$1" != "--auto" ]; then
        read -p "Perform system cleanup? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            bash "$CLEANUP_TOOL"
        else
            print_status "Cleanup skipped"
        fi
    else
        bash "$CLEANUP_TOOL"
    fi
    echo
else
    print_warning "Cleanup tool not found: $CLEANUP_TOOL"
fi

# 4. Final system check
print_header "Final System Check"

# Check service status
if systemctl is-active --quiet phantomvault.service 2>/dev/null; then
    print_success "PhantomVault service is running"
else
    print_warning "PhantomVault service is not running"
    print_status "Start with: sudo systemctl start phantomvault"
fi

# Check disk space
DISK_USAGE=$(df / | awk 'NR==2 {print $5}' | sed 's/%//')
if [ "$DISK_USAGE" -lt 90 ]; then
    print_success "Disk usage is healthy ($DISK_USAGE%)"
else
    print_warning "Disk usage is high ($DISK_USAGE%)"
fi

# Check memory usage
MEMORY_USAGE=$(free | awk 'NR==2{printf "%.0f", $3*100/$2}')
if [ "$MEMORY_USAGE" -lt 80 ]; then
    print_success "Memory usage is normal ($MEMORY_USAGE%)"
else
    print_warning "Memory usage is high ($MEMORY_USAGE%)"
fi

echo
print_header "Maintenance Complete"
print_success "PhantomVault maintenance routine completed"
print_status "Next recommended maintenance: $(date -d '+1 week' '+%Y-%m-%d')"
print_status "Run with --auto for unattended maintenance"
echo
print_status "PhantomVault v1.0.0 - MIT License"
print_status "Contact: ishaq2321@proton.me"
print_status "Contributions welcome!"

EOF

chmod +x ../build/tools/phantomvault-maintenance.sh

print_success "Comprehensive maintenance tool created: ../build/tools/phantomvault-maintenance.sh"

print_success "All maintenance tools created successfully!"
print_status "Available tools:"
print_status "  - phantomvault-diagnostic.sh (System diagnosis and troubleshooting)"
print_status "  - phantomvault-cleanup.sh (System cleanup and optimization)"
print_status "  - phantomvault-update-checker.sh (Update checking with auto-install)"
print_status "  - phantomvault-maintenance.sh (Comprehensive maintenance routine)"
print_status ""
print_status "All tools include proper warnings about development status"
print_status "Contact: ishaq2321@proton.me | License: MIT"