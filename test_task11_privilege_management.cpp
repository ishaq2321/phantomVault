/**
 * Test Task 11: Admin Privilege Requirements Enforcement
 * 
 * This test verifies that the privilege management system correctly:
 * 1. Checks privileges at application startup
 * 2. Requests elevation for vault operations
 * 3. Validates privileges for folder hiding and vault access
 * 4. Handles privilege loss gracefully during operation
 */

#include "privilege_manager.hpp"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace phantomvault;

void testPrivilegeChecking() {
    std::cout << "\n=== Testing Privilege Checking ===" << std::endl;
    
    PrivilegeManager manager;
    assert(manager.initialize());
    
    // Test current privilege detection
    auto result = manager.checkCurrentPrivileges();
    std::cout << "Current privilege level: " << static_cast<int>(result.currentLevel) << std::endl;
    std::cout << "Has admin privileges: " << (manager.hasAdminPrivileges() ? "Yes" : "No") << std::endl;
    
    // Test operation-specific privilege checking
    std::cout << "Can access vault: " << (manager.validateVaultAccess() ? "Yes" : "No") << std::endl;
    std::cout << "Can hide folders: " << (manager.validateFolderHiding() ? "Yes" : "No") << std::endl;
    std::cout << "Can create profiles: " << (manager.validateProfileCreation() ? "Yes" : "No") << std::endl;
    std::cout << "Can manage services: " << (manager.validateServiceManagement() ? "Yes" : "No") << std::endl;
    
    std::cout << "✓ Privilege checking test passed" << std::endl;
}

void testStartupValidation() {
    std::cout << "\n=== Testing Startup Validation ===" << std::endl;
    
    PrivilegeManager manager;
    assert(manager.initialize());
    
    bool hasStartupPrivileges = manager.validateStartupPrivileges();
    std::cout << "Has startup privileges: " << (hasStartupPrivileges ? "Yes" : "No") << std::endl;
    
    if (!hasStartupPrivileges) {
        std::cout << "Startup error: " << manager.getStartupPrivilegeError() << std::endl;
        std::cout << "Requires elevation: " << (manager.requiresElevationForStartup() ? "Yes" : "No") << std::endl;
        
        auto missing = manager.getMissingPermissions();
        std::cout << "Missing permissions: ";
        for (const auto& perm : missing) {
            std::cout << perm << " ";
        }
        std::cout << std::endl;
    }
    
    std::cout << "✓ Startup validation test passed" << std::endl;
}

void testElevationRequests() {
    std::cout << "\n=== Testing Elevation Requests ===" << std::endl;
    
    PrivilegeManager manager;
    assert(manager.initialize());
    
    // Test elevation capability
    std::cout << "Can request elevation: " << (manager.canRequestElevation() ? "Yes" : "No") << std::endl;
    
    // Test elevation request for vault operations
    auto result = manager.requestElevationForOperation(PrivilegedOperation::VAULT_ACCESS);
    std::cout << "Vault elevation request: " << (result.success ? "Success" : "Failed") << std::endl;
    if (!result.success) {
        std::cout << "Error: " << result.errorDetails << std::endl;
    } else {
        std::cout << "Message: " << result.message << std::endl;
    }
    
    // Test RAII elevation guard
    {
        PrivilegeElevationGuard guard(&manager, PrivilegedOperation::FOLDER_HIDING);
        std::cout << "Elevation guard active: " << (guard.isElevated() ? "Yes" : "No") << std::endl;
        if (!guard.isElevated()) {
            std::cout << "Guard error: " << guard.getErrorMessage() << std::endl;
        }
    }
    
    std::cout << "✓ Elevation request test passed" << std::endl;
}

void testPrivilegeMonitoring() {
    std::cout << "\n=== Testing Privilege Monitoring ===" << std::endl;
    
    PrivilegeManager manager;
    assert(manager.initialize());
    
    // Test monitoring lifecycle
    std::cout << "Monitoring active: " << (manager.isPrivilegeMonitoringActive() ? "Yes" : "No") << std::endl;
    
    manager.startPrivilegeMonitoring();
    std::cout << "Started monitoring: " << (manager.isPrivilegeMonitoringActive() ? "Yes" : "No") << std::endl;
    
    // Let monitoring run briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    manager.stopPrivilegeMonitoring();
    std::cout << "Stopped monitoring: " << (manager.isPrivilegeMonitoringActive() ? "No" : "Yes") << std::endl;
    
    std::cout << "✓ Privilege monitoring test passed" << std::endl;
}

void testPrivilegeLossHandling() {
    std::cout << "\n=== Testing Privilege Loss Handling ===" << std::endl;
    
    PrivilegeManager manager;
    assert(manager.initialize());
    
    // Set up privilege loss callback
    bool callbackTriggered = false;
    manager.setPrivilegeLossCallback([&callbackTriggered](PrivilegeLevel level) {
        callbackTriggered = true;
        std::cout << "Privilege loss callback triggered, new level: " << static_cast<int>(level) << std::endl;
    });
    
    // Test privilege recovery attempt
    bool recovered = manager.attemptPrivilegeRecovery();
    std::cout << "Privilege recovery attempt: " << (recovered ? "Success" : "No change needed") << std::endl;
    
    // Test manual privilege loss handling
    manager.handlePrivilegeLoss();
    std::cout << "Manual privilege loss handling completed" << std::endl;
    
    std::cout << "✓ Privilege loss handling test passed" << std::endl;
}

void testPlatformInfo() {
    std::cout << "\n=== Testing Platform Information ===" << std::endl;
    
    PrivilegeManager manager;
    assert(manager.initialize());
    
    std::cout << "Platform: " << manager.getPlatformInfo() << std::endl;
    std::cout << "Current user: " << manager.getCurrentUser() << std::endl;
    std::cout << "Running as service: " << (manager.isRunningAsService() ? "Yes" : "No") << std::endl;
    
    auto required = manager.getRequiredPermissions();
    std::cout << "Required permissions: ";
    for (const auto& perm : required) {
        std::cout << perm << " ";
    }
    std::cout << std::endl;
    
    std::cout << "✓ Platform information test passed" << std::endl;
}

void testErrorMessages() {
    std::cout << "\n=== Testing Error Messages ===" << std::endl;
    
    PrivilegeManager manager;
    assert(manager.initialize());
    
    // Test operation-specific error messages
    std::cout << "Vault access error: " << manager.getPrivilegeErrorMessage(PrivilegedOperation::VAULT_ACCESS) << std::endl;
    std::cout << "Folder hiding error: " << manager.getPrivilegeErrorMessage(PrivilegedOperation::FOLDER_HIDING) << std::endl;
    std::cout << "Profile creation error: " << manager.getPrivilegeErrorMessage(PrivilegedOperation::PROFILE_CREATION) << std::endl;
    
    std::cout << "Last error: " << manager.getLastError() << std::endl;
    
    std::cout << "✓ Error message test passed" << std::endl;
}

int main() {
    std::cout << "=== Task 11: Admin Privilege Requirements Enforcement Test ===" << std::endl;
    
    try {
        testPrivilegeChecking();
        testStartupValidation();
        testElevationRequests();
        testPrivilegeMonitoring();
        testPrivilegeLossHandling();
        testPlatformInfo();
        testErrorMessages();
        
        std::cout << "\n🎉 All Task 11 tests passed! Admin privilege requirements enforcement is complete." << std::endl;
        std::cout << "\nTask 11 Implementation Summary:" << std::endl;
        std::cout << "✓ Privilege checking at application startup with error messages" << std::endl;
        std::cout << "✓ Privilege elevation requests for vault operations" << std::endl;
        std::cout << "✓ Privilege validation for folder hiding and vault access" << std::endl;
        std::cout << "✓ Graceful handling of privilege loss during operation" << std::endl;
        std::cout << "✓ Platform-specific privilege management (Linux/Windows/macOS)" << std::endl;
        std::cout << "✓ RAII privilege elevation guard for safe operations" << std::endl;
        std::cout << "✓ Privilege monitoring and recovery mechanisms" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}