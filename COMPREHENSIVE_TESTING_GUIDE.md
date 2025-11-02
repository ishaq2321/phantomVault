# PhantomVault v1.1.0 - Comprehensive Testing Guide

**⚠️ COMPLETE SYSTEM TEST - Architecture Refactor Validation**

This document provides a complete testing procedure to validate that PhantomVault v1.1.0 works correctly with the new pure client-server architecture. Check each box as you complete the test.

---

## 🧹 PRE-TEST CLEANUP (COMPLETED)

- [x] **System Cleanup Verified**: PhantomVault completely uninstalled from system
- [x] **No Service Running**: `systemctl status phantomvault` shows "not found"
- [x] **No Files Remaining**: No PhantomVault files in `/opt`, `/usr`, `/etc`
- [x] **No Command Available**: `which phantomvault` returns "not found"

---

## 📥 INSTALLATION TESTING

### Phase 1: Download and Install

- [ ] **1.1 Download Latest Release**
  ```bash
  # Test the quick install method
  curl -fsSL https://raw.githubusercontent.com/ishaq2321/phantomVault/main/installer/install-linux.sh | sudo bash
  ```
  - [ ] Download completes without errors
  - [ ] Script executes without errors
  - [ ] No permission denied errors
  - [ ] Installation completes successfully

- [ ] **1.2 Verify Installation Directory Structure**
  ```bash
  ls -la /opt/phantomvault/
  ```
  - [ ] `/opt/phantomvault/` directory exists
  - [ ] `bin/` subdirectory exists with executables
  - [ ] `gui/` subdirectory exists with GUI files
  - [ ] `docs/` subdirectory exists with documentation
  - [ ] `logs/` subdirectory exists for log files
  - [ ] Proper file permissions set (executable files are executable)

- [ ] **1.3 Verify System Integration**
  ```bash
  which phantomvault
  ls -la /usr/local/bin/phantomvault
  ls -la /usr/share/applications/phantomvault.desktop
  systemctl status phantomvault
  ```
  - [ ] `phantomvault` command available in PATH
  - [ ] Symbolic link to main installation exists
  - [ ] Desktop entry created
  - [ ] Systemd service installed and enabled

---

## 🚀 SERVICE TESTING

### Phase 2: Service Management

- [ ] **2.1 Service Startup**
  ```bash
  sudo systemctl start phantomvault
  systemctl status phantomvault
  ```
  - [ ] Service starts without errors
  - [ ] Status shows "active (running)"
  - [ ] No error messages in status output
  - [ ] Service PID is shown

- [ ] **2.2 Port Binding Verification**
  ```bash
  netstat -tlnp | grep :9876
  ps aux | grep phantomvault
  ```
  - [ ] Exactly ONE process listening on port 9876
  - [ ] Only ONE phantomvault process running
  - [ ] Process is running as phantomvault user
  - [ ] Memory usage is reasonable (<50MB)

- [ ] **2.3 Service Logs**
  ```bash
  journalctl -u phantomvault -n 20
  ```
  - [ ] Service initialization messages present
  - [ ] No error messages in logs
  - [ ] IPC server startup message present
  - [ ] "Service started successfully" message present

---

## 💻 CLI TESTING (ARCHITECTURE FIX VALIDATION)

### Phase 3: CLI Client-Server Communication

- [ ] **3.1 Basic CLI Commands**
  ```bash
  phantomvault --help
  phantomvault --version
  phantomvault
  ```
  - [ ] Help message displays correctly
  - [ ] Version shows v1.1.0 or later
  - [ ] Default command shows service status
  - [ ] No errors or crashes

- [ ] **3.2 Service Status Check**
  ```bash
  phantomvault --cli status
  ```
  - [ ] Shows "✅ PhantomVault service is running"
  - [ ] Displays service information
  - [ ] No connection errors
  - [ ] Response time is fast (<1 second)

- [ ] **3.3 Profile Management**
  ```bash
  phantomvault --cli profiles
  ```
  - [ ] Command executes without errors
  - [ ] Shows "No profiles found" or lists existing profiles
  - [ ] No "Failed to bind socket to port 9876" errors
  - [ ] No additional phantomvault processes created

- [ ] **3.4 Multiple Concurrent CLI Commands (CRITICAL TEST)**
  ```bash
  # Run these commands simultaneously in different terminals
  phantomvault --cli status &
  phantomvault --cli profiles &
  phantomvault --cli status &
  wait
  ```
  - [ ] All commands complete successfully
  - [ ] No port conflict errors
  - [ ] No "address already in use" errors
  - [ ] Only ONE service process remains running
  - [ ] No zombie processes created

- [ ] **3.5 Service Control Commands**
  ```bash
  phantomvault --cli stop
  phantomvault --cli status
  phantomvault --cli restart
  phantomvault --cli status
  ```
  - [ ] Stop command works (service stops gracefully)
  - [ ] Status shows service stopped
  - [ ] Restart command works
  - [ ] Status shows service running again

---

## 👤 PROFILE TESTING (ADMIN PRIVILEGES)

### Phase 4: Profile Management

- [ ] **4.1 Profile Creation (Admin Required)**
  ```bash
  # This should fail without sudo
  phantomvault --cli create-profile testprofile password123
  
  # This should work with sudo
  sudo phantomvault --cli create-profile testprofile password123
  ```
  - [ ] Non-sudo command shows admin privilege error
  - [ ] Error message suggests using sudo
  - [ ] Sudo command creates profile successfully
  - [ ] Profile creation confirmation message shown

- [ ] **4.2 Profile Listing**
  ```bash
  phantomvault --cli profiles
  ```
  - [ ] Shows the created "testprofile"
  - [ ] Displays profile information (folder count, last access)
  - [ ] No errors in profile listing

- [ ] **4.3 Profile Authentication Test**
  ```bash
  # Test folder locking (should require authentication)
  sudo phantomvault --cli lock testprofile
  ```
  - [ ] Command executes (may show authentication required message)
  - [ ] No crashes or connection errors
  - [ ] Appropriate response for authentication state

---

## 🖥️ GUI TESTING

### Phase 5: Desktop Application

- [ ] **5.1 GUI Launch**
  ```bash
  phantomvault --gui
  # Or launch from applications menu
  ```
  - [ ] GUI application starts without errors
  - [ ] Electron window opens
  - [ ] No console errors visible
  - [ ] Interface loads completely

- [ ] **5.2 Service Connection**
  - [ ] GUI shows service status as "Running"
  - [ ] No connection error messages
  - [ ] Service information displayed correctly
  - [ ] Real-time status updates work

- [ ] **5.3 Profile Management in GUI**
  - [ ] Existing profiles are listed
  - [ ] Profile creation dialog works
  - [ ] Admin privilege elevation works (if needed)
  - [ ] Profile authentication dialog works

- [ ] **5.4 GUI Service Independence**
  ```bash
  # While GUI is running, check processes
  ps aux | grep phantomvault
  netstat -tlnp | grep :9876
  ```
  - [ ] Still only ONE service process running
  - [ ] GUI doesn't create additional service instances
  - [ ] Port 9876 still has only one listener

---

## 🔧 SYSTEM INTEGRATION TESTING

### Phase 6: System Integration

- [ ] **6.1 Systemd Integration**
  ```bash
  sudo systemctl restart phantomvault
  sudo systemctl status phantomvault
  systemctl is-enabled phantomvault
  ```
  - [ ] Service restarts cleanly
  - [ ] Status shows healthy after restart
  - [ ] Service is enabled for auto-start
  - [ ] No systemd errors

- [ ] **6.2 User Permissions**
  ```bash
  # Test as regular user
  phantomvault --cli status
  
  # Test service management (should require privileges)
  phantomvault --cli restart
  ```
  - [ ] Regular user can check status
  - [ ] Service management requires appropriate privileges
  - [ ] Clear error messages for permission issues

- [ ] **6.3 Log File Management**
  ```bash
  ls -la /opt/phantomvault/logs/
  tail -f /opt/phantomvault/logs/phantomvault.log
  ```
  - [ ] Log directory exists and is writable
  - [ ] Log files are being created
  - [ ] Logs contain useful information
  - [ ] No permission errors in logging

---

## 🔒 SECURITY TESTING

### Phase 7: Security Validation

- [ ] **7.1 Privilege Validation**
  ```bash
  # Test without admin privileges
  phantomvault --cli create-profile test2 pass123
  
  # Test with admin privileges
  sudo phantomvault --cli create-profile test2 pass123
  ```
  - [ ] Non-admin command properly rejected
  - [ ] Clear error message about admin requirements
  - [ ] Admin command works correctly
  - [ ] No privilege escalation vulnerabilities

- [ ] **7.2 Service Security**
  ```bash
  # Check service user
  ps aux | grep phantomvault | grep -v grep
  
  # Check listening ports
  netstat -tlnp | grep phantomvault
  ```
  - [ ] Service runs as dedicated user (not root)
  - [ ] Only listens on localhost (127.0.0.1:9876)
  - [ ] No unnecessary network exposure
  - [ ] Proper process isolation

---

## 📊 PERFORMANCE TESTING

### Phase 8: Performance Validation

- [ ] **8.1 Resource Usage**
  ```bash
  # Monitor resource usage
  top -p $(pgrep phantomvault)
  ps -o pid,ppid,cmd,%mem,%cpu -p $(pgrep phantomvault)
  ```
  - [ ] Memory usage under 50MB
  - [ ] CPU usage under 5% when idle
  - [ ] No memory leaks over time
  - [ ] Reasonable startup time (<5 seconds)

- [ ] **8.2 Response Time**
  ```bash
  # Test CLI response times
  time phantomvault --cli status
  time phantomvault --cli profiles
  ```
  - [ ] CLI commands respond in under 1 second
  - [ ] No noticeable delays
  - [ ] Consistent performance across commands

---

## 🧪 STRESS TESTING

### Phase 9: Stress and Edge Cases

- [ ] **9.1 Concurrent CLI Stress Test**
  ```bash
  # Run 10 concurrent CLI commands
  for i in {1..10}; do
    phantomvault --cli status &
  done
  wait
  
  # Check system state
  ps aux | grep phantomvault
  netstat -tlnp | grep :9876
  ```
  - [ ] All commands complete successfully
  - [ ] No port conflicts or binding errors
  - [ ] Still only one service process running
  - [ ] No resource exhaustion

- [ ] **9.2 Service Recovery Test**
  ```bash
  # Kill service process directly
  sudo pkill -f phantomvault-service
  
  # Try CLI command (should fail)
  phantomvault --cli status
  
  # Restart service
  sudo systemctl start phantomvault
  
  # Try CLI command again (should work)
  phantomvault --cli status
  ```
  - [ ] CLI shows clear error when service is down
  - [ ] Error message suggests how to start service
  - [ ] Service restarts cleanly
  - [ ] CLI works again after service restart

---

## 🗑️ UNINSTALLATION TESTING

### Phase 10: Clean Removal

- [ ] **10.1 Service Cleanup**
  ```bash
  sudo systemctl stop phantomvault
  sudo systemctl disable phantomvault
  ```
  - [ ] Service stops cleanly
  - [ ] Service disabled from auto-start
  - [ ] No errors during service cleanup

- [ ] **10.2 File Removal**
  ```bash
  # If uninstaller exists
  sudo /opt/phantomvault/uninstall.sh
  
  # Or manual removal
  sudo rm -rf /opt/phantomvault
  sudo rm -f /usr/local/bin/phantomvault
  sudo rm -f /usr/share/applications/phantomvault.desktop
  sudo rm -f /etc/systemd/system/phantomvault.service
  ```
  - [ ] All PhantomVault files removed
  - [ ] No orphaned files left behind
  - [ ] System restored to clean state

- [ ] **10.3 Verification of Complete Removal**
  ```bash
  which phantomvault
  find /opt -name "*phantom*" 2>/dev/null
  find /usr -name "*phantom*" 2>/dev/null | grep -v linux
  systemctl status phantomvault
  ```
  - [ ] Command not found
  - [ ] No PhantomVault files remain
  - [ ] Service not found
  - [ ] System completely clean

---

## 📋 TEST RESULTS SUMMARY

### Critical Architecture Tests (Must Pass)
- [ ] **Single Service Process**: Only one phantomvault process runs
- [ ] **No Port Conflicts**: Multiple CLI commands don't cause port binding errors
- [ ] **Pure Client Architecture**: CLI commands don't create service instances
- [ ] **Clean IPC Communication**: All CLI/GUI communication works via IPC

### Security Tests (Must Pass)
- [ ] **Admin Privilege Enforcement**: Profile/folder operations require admin privileges
- [ ] **Service Isolation**: Service runs as dedicated user, not root
- [ ] **Network Security**: Service only listens on localhost

### Functionality Tests (Must Pass)
- [ ] **Service Management**: Start, stop, restart work correctly
- [ ] **Profile Management**: Create, list, authenticate profiles work
- [ ] **CLI Interface**: All CLI commands work without errors
- [ ] **GUI Interface**: Desktop application launches and functions

### Performance Tests (Should Pass)
- [ ] **Resource Usage**: Memory <50MB, CPU <5% when idle
- [ ] **Response Time**: CLI commands respond in <1 second
- [ ] **Stability**: No crashes or memory leaks during testing

---

## 🚨 FAILURE HANDLING

If any test fails:

1. **Document the failure**: Note exactly what went wrong
2. **Check logs**: `journalctl -u phantomvault -n 50`
3. **Check processes**: `ps aux | grep phantomvault`
4. **Check ports**: `netstat -tlnp | grep :9876`
5. **Report the issue**: Include all error messages and system state

---

## ✅ FINAL VALIDATION

After completing all tests:

- [ ] **All Critical Tests Passed**: Architecture, security, functionality
- [ ] **No Regressions Found**: All existing features work as expected
- [ ] **Performance Acceptable**: Resource usage and response times good
- [ ] **Installation/Uninstallation Clean**: No issues with setup/removal

**Test Completion Date**: _______________

**Tester**: _______________

**Overall Result**: [ ] PASS [ ] FAIL

**Notes**:
_________________________________________________________________
_________________________________________________________________
_________________________________________________________________

---

**🎯 SUCCESS CRITERIA**: All critical tests must pass for v1.1.0 to be considered production-ready.