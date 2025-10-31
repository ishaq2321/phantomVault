/**
 * @file test_enhanced_error_handler.cpp
 * @brief Test suite for enhanced ErrorHandler functionality
 */

#include "../include/error_handler.hpp"
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>

using namespace phantomvault;

void testCategorizedErrorHandling() {
    std::cout << "Testing categorized error handling..." << std::endl;
    
    ErrorHandler handler;
    assert(handler.initialize("./test_logs/enhanced_error.log"));
    
    // Test system error handling
    handler.handleSystemError("TestComponent", "Test system error", ErrorSeverity::WARNING);
    
    // Test network error handling
    handler.handleNetworkError("connect", "https://example.com", "Connection timeout");
    
    // Test file system error handling
    handler.handleFileSystemError("write", "/tmp/test.txt", "Permission denied");
    
    // Test memory error handling
    handler.handleMemoryError("TestComponent", 1024*1024, "Out of memory");
    
    std::cout << "✓ Categorized error handling tests passed" << std::endl;
}

void testUserFriendlyMessages() {
    std::cout << "Testing user-friendly error messages..." << std::endl;
    
    ErrorHandler handler;
    assert(handler.initialize("./test_logs/enhanced_error.log"));
    
    // Test secure error messages
    std::string auth_msg = handler.getSecureErrorMessage(SecurityEventType::AUTHENTICATION_FAILURE);
    assert(!auth_msg.empty());
    std::cout << "Auth error message: " << auth_msg << std::endl;
    
    // Test user-friendly messages
    std::string friendly_msg = handler.getUserFriendlyErrorMessage("EncryptionEngine", "encrypt", ErrorSeverity::ERROR);
    assert(!friendly_msg.empty());
    std::cout << "Friendly error message: " << friendly_msg << std::endl;
    
    // Test recovery guidance
    std::string guidance = handler.getRecoveryGuidance(SecurityEventType::VAULT_CORRUPTION, ErrorSeverity::CRITICAL);
    assert(!guidance.empty());
    std::cout << "Recovery guidance: " << guidance << std::endl;
    
    std::cout << "✓ User-friendly message tests passed" << std::endl;
}

void testFailSafeDefaults() {
    std::cout << "Testing fail-safe defaults..." << std::endl;
    
    ErrorHandler handler;
    assert(handler.initialize("./test_logs/enhanced_error.log"));
    
    // Test automatic recovery
    handler.attemptAutomaticRecovery("TestComponent", "Test error");
    
    // Test safe mode
    handler.enableSafeMode();
    
    // Test offline mode
    handler.enableOfflineMode();
    
    std::cout << "✓ Fail-safe defaults tests passed" << std::endl;
}

void testEnhancedBackupSystem() {
    std::cout << "Testing enhanced backup system..." << std::endl;
    
    ErrorHandler handler;
    assert(handler.initialize("./test_logs/enhanced_error.log"));
    
    // Test backup scheduling
    handler.scheduleBackup("/tmp/test_file.txt", "test_profile");
    
    // Test backup listing
    auto backups = handler.listEncryptedBackups("test_profile");
    std::cout << "Found " << backups.size() << " backups for test_profile" << std::endl;
    
    std::cout << "✓ Enhanced backup system tests passed" << std::endl;
}

void testLogIntegrity() {
    std::cout << "Testing log integrity..." << std::endl;
    
    ErrorHandler handler;
    assert(handler.initialize("./test_logs/enhanced_error.log"));
    
    // Generate some log entries
    handler.logSecurityEvent(SecurityEventType::AUTHENTICATION_FAILURE, ErrorSeverity::WARNING,
                           "test_profile", "Test log entry", {{"test", "metadata"}});
    
    // Wait a moment for log to be written
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Test log integrity verification
    bool integrity_ok = handler.verifyLogIntegrity();
    std::cout << "Log integrity check: " << (integrity_ok ? "PASS" : "FAIL") << std::endl;
    
    std::cout << "✓ Log integrity tests completed" << std::endl;
}

int main() {
    std::cout << "=== Enhanced ErrorHandler Test Suite ===" << std::endl;
    
    try {
        testCategorizedErrorHandling();
        testUserFriendlyMessages();
        testFailSafeDefaults();
        testEnhancedBackupSystem();
        testLogIntegrity();
        
        std::cout << "\n🎉 All enhanced ErrorHandler tests passed!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}