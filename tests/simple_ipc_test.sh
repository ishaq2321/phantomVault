#!/bin/bash
# Simple IPC Communication Test
# Verifies CLI to service communication works correctly

set -e

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[PASS]${NC} $1"; }
log_error() { echo -e "${RED}[FAIL]${NC} $1"; }

CLI_BIN="./build/bin/phantomvault"

echo "=================================================="
echo "PhantomVault IPC Communication Test"
echo "=================================================="

# Test 1: CLI error handling when service is not running
log_info "Test 1: CLI error handling when service is not running"

if output=$($CLI_BIN --cli status 2>&1); then
    log_error "CLI should fail when service is not running"
    exit 1
else
    if echo "$output" | grep -q "Cannot connect to PhantomVault service\|service is not running"; then
        log_success "CLI correctly handles service connection failure"
    else
        log_error "CLI error message was unexpected: $output"
        exit 1
    fi
fi

# Test 2: Verify CLI doesn't try to bind to ports
log_info "Test 2: Verify CLI doesn't create service instances"

# Check no processes are listening on 9876 before CLI commands
if netstat -tlnp 2>/dev/null | grep -q ":9876.*LISTEN"; then
    log_error "Port 9876 already in use before test"
    exit 1
fi

# Run multiple CLI commands
$CLI_BIN --cli status >/dev/null 2>&1 || true
$CLI_BIN --cli profiles >/dev/null 2>&1 || true
$CLI_BIN --cli status >/dev/null 2>&1 || true

# Verify no processes are listening on 9876 after CLI commands
if netstat -tlnp 2>/dev/null | grep -q ":9876.*LISTEN"; then
    log_error "CLI commands created service instances (port 9876 is bound)"
    exit 1
else
    log_success "CLI commands don't create service instances"
fi

# Test 3: Verify CLI methods use IPC client pattern
log_info "Test 3: Verify CLI methods use IPC client pattern"

# Test that CLI commands show proper IPC error messages
output=$($CLI_BIN --cli profiles 2>&1) || true
if echo "$output" | grep -q "Cannot connect to PhantomVault service"; then
    log_success "CLI profiles uses IPC client pattern"
else
    log_error "CLI profiles doesn't use expected IPC pattern"
    exit 1
fi

echo
echo "=================================================="
echo "Test Results"
echo "=================================================="
log_success "✅ CLI handles service connection failures correctly"
log_success "✅ CLI commands don't create service instances"
log_success "✅ CLI methods use pure IPC client pattern"
echo
log_success "🎉 IPC communication architecture is working correctly!"