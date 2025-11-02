#!/bin/bash
# Service Communication Test
# Verifies service runs correctly and CLI can communicate with it

set -e

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[PASS]${NC} $1"; }
log_error() { echo -e "${RED}[FAIL]${NC} $1"; }

SERVICE_BIN="./core/build/bin/phantomvault-service"
CLI_BIN="./build/bin/phantomvault"
SERVICE_PID=""

cleanup() {
    if [[ -n "$SERVICE_PID" ]]; then
        sudo kill -TERM "$SERVICE_PID" 2>/dev/null || true
        sleep 2
        sudo kill -KILL "$SERVICE_PID" 2>/dev/null || true
    fi
    sudo pkill -f phantomvault 2>/dev/null || true
}

trap cleanup EXIT

echo "=================================================="
echo "PhantomVault Service Communication Test"
echo "=================================================="

# Start service
log_info "Starting PhantomVault service..."
sudo $SERVICE_BIN --service --daemon &
SERVICE_PID=$!

# Wait for service to start
sleep 5

# Verify service is running
if ! kill -0 "$SERVICE_PID" 2>/dev/null; then
    log_error "Service failed to start"
    exit 1
fi

# Test CLI communication with running service
log_info "Testing CLI communication with running service..."

if output=$($CLI_BIN --cli status 2>&1); then
    if echo "$output" | grep -q "PhantomVault service is running"; then
        log_success "CLI successfully communicates with service"
    else
        log_error "CLI communication failed: $output"
        exit 1
    fi
else
    log_error "CLI status command failed: $output"
    exit 1
fi

# Test profiles command
if output=$($CLI_BIN --cli profiles 2>&1); then
    if echo "$output" | grep -q "No profiles found\|Available profiles:"; then
        log_success "CLI profiles command works correctly"
    else
        log_error "CLI profiles unexpected output: $output"
        exit 1
    fi
else
    log_error "CLI profiles command failed: $output"
    exit 1
fi

# Verify only one process on port 9876
listener_count=$(sudo lsof -i :9876 2>/dev/null | grep LISTEN | wc -l)
if [[ "$listener_count" -eq 1 ]]; then
    log_success "Only one process listening on port 9876"
else
    log_error "Expected 1 listener, found $listener_count"
    exit 1
fi

echo
echo "=================================================="
echo "Test Results"
echo "=================================================="
log_success "✅ Service starts successfully"
log_success "✅ CLI communicates with service correctly"
log_success "✅ Only one service instance running"
echo
log_success "🎉 Service communication is working perfectly!"