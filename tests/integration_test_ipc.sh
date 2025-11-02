#!/bin/bash
# PhantomVault IPC Integration Tests
# Tests CLI to service communication and error handling

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
SERVICE_BIN="./core/build/bin/phantomvault-service"
CLI_BIN="./build/bin/phantomvault"
TEST_PORT=9876
SERVICE_PID=""

# Logging functions
log_info() {
    echo -e "${BLUE}[TEST INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[TEST PASS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[TEST WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[TEST FAIL]${NC} $1"
}

# Cleanup function
cleanup() {
    log_info "Cleaning up test environment..."
    if [[ -n "$SERVICE_PID" ]]; then
        sudo kill -TERM "$SERVICE_PID" 2>/dev/null || true
        sleep 2
        sudo kill -KILL "$SERVICE_PID" 2>/dev/null || true
    fi
    
    # Kill any remaining phantomvault processes
    sudo pkill -f phantomvault 2>/dev/null || true
    
    # Wait for port to be free
    sleep 2
    
    log_info "Cleanup completed"
}

# Set up cleanup trap
trap cleanup EXIT

# Test 1: Verify service can start and bind to port
test_service_startup() {
    log_info "Test 1: Service startup and port binding"
    
    # Start service in background
    sudo $SERVICE_BIN --service --daemon &
    SERVICE_PID=$!
    
    # Wait for service to start
    sleep 5
    
    # Check if service is running
    if ! kill -0 "$SERVICE_PID" 2>/dev/null; then
        log_error "Service failed to start"
        return 1
    fi
    
    # Check if port is bound
    if ! netstat -tlnp | grep -q ":$TEST_PORT.*LISTEN"; then
        log_error "Service not listening on port $TEST_PORT"
        return 1
    fi
    
    # Verify only one process is listening
    local listener_count=$(sudo lsof -i :$TEST_PORT | grep LISTEN | wc -l)
    if [[ "$listener_count" -ne 1 ]]; then
        log_error "Expected 1 listener on port $TEST_PORT, found $listener_count"
        return 1
    fi
    
    log_success "Service started successfully and bound to port $TEST_PORT"
    return 0
}

# Test 2: CLI status command when service is running
test_cli_status_running() {
    log_info "Test 2: CLI status command with running service"
    
    local output
    if output=$($CLI_BIN --cli status 2>&1); then
        if echo "$output" | grep -q "PhantomVault service is running"; then
            log_success "CLI status correctly detected running service"
            return 0
        else
            log_error "CLI status did not detect running service"
            echo "$output"
            return 1
        fi
    else
        log_error "CLI status command failed"
        echo "$output"
        return 1
    fi
}

# Test 3: CLI profiles command
test_cli_profiles() {
    log_info "Test 3: CLI profiles command"
    
    local output
    if output=$($CLI_BIN --cli profiles 2>&1); then
        if echo "$output" | grep -q "No profiles found\|Available profiles:"; then
            log_success "CLI profiles command worked correctly"
            return 0
        else
            log_error "CLI profiles command gave unexpected output"
            echo "$output"
            return 1
        fi
    else
        log_error "CLI profiles command failed"
        echo "$output"
        return 1
    fi
}

# Test 4: Multiple concurrent CLI commands
test_concurrent_cli_commands() {
    log_info "Test 4: Multiple concurrent CLI commands"
    
    # Run multiple CLI commands in parallel
    $CLI_BIN --cli status &
    local pid1=$!
    
    $CLI_BIN --cli profiles &
    local pid2=$!
    
    $CLI_BIN --cli status &
    local pid3=$!
    
    # Wait for all commands to complete
    wait $pid1 $pid2 $pid3
    
    # Verify still only one process listening on port
    local listener_count=$(sudo lsof -i :$TEST_PORT | grep LISTEN | wc -l)
    if [[ "$listener_count" -ne 1 ]]; then
        log_error "Expected 1 listener after concurrent commands, found $listener_count"
        return 1
    fi
    
    log_success "Concurrent CLI commands worked without creating additional service instances"
    return 0
}

# Test 5: Stop service and test CLI error handling
test_cli_error_handling() {
    log_info "Test 5: CLI error handling when service is stopped"
    
    # Stop the service
    if [[ -n "$SERVICE_PID" ]]; then
        sudo kill -TERM "$SERVICE_PID"
        wait "$SERVICE_PID" 2>/dev/null || true
        SERVICE_PID=""
    fi
    
    # Wait for service to stop
    sleep 3
    
    # Verify port is no longer bound
    if netstat -tlnp | grep -q ":$TEST_PORT.*LISTEN"; then
        log_error "Port $TEST_PORT still bound after stopping service"
        return 1
    fi
    
    # Test CLI status with stopped service
    local output
    if output=$($CLI_BIN --cli status 2>&1); then
        log_error "CLI status should fail when service is stopped"
        return 1
    else
        if echo "$output" | grep -q "Cannot connect to PhantomVault service\|service is not running"; then
            log_success "CLI correctly detected stopped service"
            return 0
        else
            log_error "CLI error message was not as expected"
            echo "$output"
            return 1
        fi
    fi
}

# Test 6: Service restart capability
test_service_restart() {
    log_info "Test 6: Service restart capability"
    
    # Start service again
    sudo $SERVICE_BIN --service --daemon &
    SERVICE_PID=$!
    
    # Wait for service to start
    sleep 5
    
    # Verify service is running
    if ! kill -0 "$SERVICE_PID" 2>/dev/null; then
        log_error "Service failed to restart"
        return 1
    fi
    
    # Test CLI works after restart
    local output
    if output=$($CLI_BIN --cli status 2>&1); then
        if echo "$output" | grep -q "PhantomVault service is running"; then
            log_success "Service restarted successfully and CLI works"
            return 0
        else
            log_error "CLI status failed after service restart"
            echo "$output"
            return 1
        fi
    else
        log_error "CLI status command failed after restart"
        echo "$output"
        return 1
    fi
}

# Main test execution
main() {
    echo "=================================================="
    echo "PhantomVault IPC Integration Tests"
    echo "Testing CLI to Service Communication"
    echo "=================================================="
    echo
    
    local failed_tests=0
    local total_tests=6
    
    # Check if binaries exist
    if [[ ! -f "$SERVICE_BIN" ]]; then
        log_error "Service binary not found: $SERVICE_BIN"
        log_info "Please build the service first: cd core/build && make phantomvault-service"
        exit 1
    fi
    
    if [[ ! -f "$CLI_BIN" ]]; then
        log_error "CLI binary not found: $CLI_BIN"
        log_info "Please build the application first: cd build && make phantomvault"
        exit 1
    fi
    
    # Run tests
    test_service_startup || ((failed_tests++))
    echo
    
    test_cli_status_running || ((failed_tests++))
    echo
    
    test_cli_profiles || ((failed_tests++))
    echo
    
    test_concurrent_cli_commands || ((failed_tests++))
    echo
    
    test_cli_error_handling || ((failed_tests++))
    echo
    
    test_service_restart || ((failed_tests++))
    echo
    
    # Test summary
    echo "=================================================="
    echo "Test Results Summary"
    echo "=================================================="
    
    local passed_tests=$((total_tests - failed_tests))
    
    if [[ $failed_tests -eq 0 ]]; then
        log_success "All $total_tests tests passed!"
        echo
        log_success "✅ Service runs as single process on port $TEST_PORT"
        log_success "✅ CLI commands work as pure clients"
        log_success "✅ No port conflicts with multiple CLI commands"
        log_success "✅ Proper error handling when service is not running"
        log_success "✅ Service can restart successfully"
        echo
        echo "🎉 PhantomVault architecture refactor is working perfectly!"
        exit 0
    else
        log_error "$failed_tests out of $total_tests tests failed"
        log_error "❌ Architecture refactor needs attention"
        exit 1
    fi
}

# Run main function
main "$@"