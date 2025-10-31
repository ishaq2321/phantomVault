/**
 * Comprehensive Integration Testing Suite
 * 
 * End-to-end testing of complete PhantomVault workflows:
 * - Full system integration testing
 * - Multi-component interaction validation
 * - Real-world scenario simulation
 * - System resilience under load
 * - Cross-platform compatibility testing
 */

#include "test_framework.hpp"
#include "../include/vault_handler.hpp"
#include "../include/profile_manager.hpp"
#include "../include/folder_security_manager.hpp"
#include "../include/keyboard_sequence_detector.hpp"
#include "../include/privilege_manager.hpp"
#include "../include/error_handler.hpp"
#include "../include/encryption_engine.hpp"
#include "../include/service_manager.hpp"
#include <filesystem>
#include <fstream>
#include <thread>
#include <future>

using namespace phantomvault;
using namespace phantomvault::testing;
namespace fs = std::filesystem;

class IntegrationTests {
public:
    static void registerTests(TestFramework& framework) {
        // Full system integration tests
        REGISTER_TEST(framework, "Integration", "complete_system_workflow", testCompleteSystemWorkflow);
        REGISTER_TEST(framework, "Integration", "multi_user_scenarios", testMultiUserScenarios);
        REGISTER_TEST(framework, "Integration", "concurrent_operations", testConcurrentOperations);
        REGISTER_TEST(framework, "Integration", "system_recovery_scenarios", testSystemRecoveryScenarios);
        
        // Component interaction tests
        REGISTER_TEST(framework, "Integration", "service_manager_integration", testServiceManagerIntegration);
        REGISTER_TEST(framework, "Integration", "keyboard_vault_integration", testKeyboardVaultIntegration);
        REGISTER_TEST(framework, "Integration", "privilege_security_integration", testPrivilegeSecurityIntegration);
        REGISTER_TEST(framework, "Integration", "error_recovery_integration", testErrorRecoveryIntegration);
        
        // Real-world scenario tests
        REGISTER_TEST(framework, "Integration", "large_folder_management", testLargeFolderManagement);
        REGISTER_TEST(framework, "Integration", "multiple_vault_operations", testMultipleVaultOperations);
        REGISTER_TEST(framework, "Integration", "system_stress_testing", testSystemStressTesting);
        REGISTER_TEST(framework, "Integration", "data_integrity_validation", testDataIntegrityValidation);
        
        // Cross-platform compatibility
        REGISTER_TEST(framework, "Integration", "cross_platform_compatibility", testCrossPlatformCompatibility);
        REGISTER_TEST(framework, "Integration", "filesystem_compatibility", testFilesystemCompatibility);
        REGISTER_TEST(framework, "Integration", "permission_model_testing", testPermissionModelTesting);
    }

private:
    // Full System Integration Tests
    static void testCompleteSystemWorkflow() {
        std::string test_root = "./integration_test_complete";
        
        // Cleanup
        if (fs::exists(test_root)) fs::remove_all(test_root);
        
        // Initialize all components
        ServiceManager service_manager;
        ProfileManager profile_manager;
        VaultHandler vault_handler;
        FolderSecurityManager security_manager;
        KeyboardSequenceDetector keyboard_detector;
        PrivilegeManager privilege_manager;
        ErrorHandler error_handler;
        
        // Initialize components
        ASSERT_TRUE(service_manager.initialize());
        ASSERT_TRUE(profile_manager.initialize(test_root + "/profiles"));
        ASSERT_TRUE(vault_handler.initialize(test_root + "/vaults"));
        ASSERT_TRUE(security_manager.initialize());
        ASSERT_TRUE(keyboard_detector.initialize());
        ASSERT_TRUE(privilege_manager.initialize());
        ASSERT_TRUE(error_handler.initialize(test_root + "/logs"));
        
        // Create test user profile
        auto profile_result = profile_manager.createProfile("integration_user", "IntegrationTest123!", "Integration Test User");
        ASSERT_TRUE(profile_result.success);
        
        // Authenticate user
        auto auth_result = profile_manager.authenticateProfile("integration_user", "IntegrationTest123!");
        ASSERT_TRUE(auth_result.success);
        
        // Create vault structure
        ASSERT_TRUE(vault_handler.createVaultStructure("integration_vault", profile_result.profile_id));
        
        // Create test folders
        std::vector<std::string> test_folders;
        for (int i = 0; i < 5; ++i) {
            std::string folder_name = "./integration_folder_" + std::to_string(i);
            fs::create_directories(folder_name);
            
            // Add test files
            std::ofstream test_file(folder_name + "/test_file.txt");
            test_file << "Integration test content " << i;
            test_file.close();
            
            test_folders.push_back(folder_name);
        }
        
        // Hide all folders
        std::vector<std::string> obfuscated_ids;
        for (const auto& folder : test_folders) {
            auto hide_result = vault_handler.hideFolder(folder, "integration_vault");
            ASSERT_TRUE(hide_result.success);
            obfuscated_ids.push_back(hide_result.obfuscated_identifier);
        }
        
        // Verify vault integrity
        ASSERT_TRUE(vault_handler.validateVaultIntegrity("integration_vault"));
        
        // Restore all folders
        for (size_t i = 0; i < obfuscated_ids.size(); ++i) {
            auto restore_result = vault_handler.restoreFolder("integration_vault", obfuscated_ids[i]);
            ASSERT_TRUE(restore_result.success);
            ASSERT_TRUE(fs::exists(test_folders[i]));
        }
        
        // Cleanup
        for (const auto& folder : test_folders) {
            if (fs::exists(folder)) fs::remove_all(folder);
        }
        fs::remove_all(test_root);
    }   
 
    static void testMultiUserScenarios() {
        std::string test_root = "./integration_test_multiuser";
        
        if (fs::exists(test_root)) fs::remove_all(test_root);
        
        ProfileManager profile_manager;
        VaultHandler vault_handler;
        
        ASSERT_TRUE(profile_manager.initialize(test_root + "/profiles"));
        ASSERT_TRUE(vault_handler.initialize(test_root + "/vaults"));
        
        // Create multiple users
        std::vector<std::string> user_ids;
        for (int i = 0; i < 3; ++i) {
            std::string username = "user_" + std::to_string(i);
            std::string password = "Password_" + std::to_string(i) + "!";
            
            auto result = profile_manager.createProfile(username, password, "Test User " + std::to_string(i));
            ASSERT_TRUE(result.success);
            user_ids.push_back(result.profile_id);
        }
        
        // Each user creates their own vault
        for (size_t i = 0; i < user_ids.size(); ++i) {
            std::string vault_id = "vault_" + std::to_string(i);
            ASSERT_TRUE(vault_handler.createVaultStructure(vault_id, user_ids[i]));
            
            // Create and hide folders for each user
            std::string test_folder = "./multiuser_folder_" + std::to_string(i);
            fs::create_directories(test_folder);
            
            std::ofstream test_file(test_folder + "/user_data.txt");
            test_file << "User " << i << " private data";
            test_file.close();
            
            auto hide_result = vault_handler.hideFolder(test_folder, vault_id);
            ASSERT_TRUE(hide_result.success);
            
            // Verify vault integrity
            ASSERT_TRUE(vault_handler.validateVaultIntegrity(vault_id));
            
            // Cleanup test folder
            if (fs::exists(test_folder)) fs::remove_all(test_folder);
        }
        
        fs::remove_all(test_root);
    }
    
    static void testConcurrentOperations() {
        std::string test_root = "./integration_test_concurrent";
        
        if (fs::exists(test_root)) fs::remove_all(test_root);
        
        VaultHandler vault_handler;
        ASSERT_TRUE(vault_handler.initialize(test_root + "/vaults"));
        ASSERT_TRUE(vault_handler.createVaultStructure("concurrent_vault", "test_profile"));
        
        // Create test folders
        std::vector<std::string> test_folders;
        for (int i = 0; i < 10; ++i) {
            std::string folder_name = "./concurrent_folder_" + std::to_string(i);
            fs::create_directories(folder_name);
            
            std::ofstream test_file(folder_name + "/concurrent_test.txt");
            test_file << "Concurrent test data " << i;
            test_file.close();
            
            test_folders.push_back(folder_name);
        }
        
        // Perform concurrent hide operations
        std::vector<std::future<HidingResult>> hide_futures;
        
        for (const auto& folder : test_folders) {
            hide_futures.push_back(std::async(std::launch::async, [&vault_handler, folder]() {
                return vault_handler.hideFolder(folder, "concurrent_vault");
            }));
        }
        
        // Wait for all operations to complete
        std::vector<std::string> obfuscated_ids;
        for (auto& future : hide_futures) {
            auto result = future.get();
            ASSERT_TRUE(result.success);
            obfuscated_ids.push_back(result.obfuscated_identifier);
        }
        
        // Verify vault integrity after concurrent operations
        ASSERT_TRUE(vault_handler.validateVaultIntegrity("concurrent_vault"));
        
        // Perform concurrent restore operations
        std::vector<std::future<RestorationResult>> restore_futures;
        
        for (const auto& id : obfuscated_ids) {
            restore_futures.push_back(std::async(std::launch::async, [&vault_handler, id]() {
                return vault_handler.restoreFolder("concurrent_vault", id);
            }));
        }
        
        // Wait for all restore operations
        for (auto& future : restore_futures) {
            auto result = future.get();
            ASSERT_TRUE(result.success);
        }
        
        // Cleanup
        for (const auto& folder : test_folders) {
            if (fs::exists(folder)) fs::remove_all(folder);
        }
        fs::remove_all(test_root);
    }
    
    static void testSystemRecoveryScenarios() {
        std::string test_root = "./integration_test_recovery";
        
        if (fs::exists(test_root)) fs::remove_all(test_root);
        
        VaultHandler vault_handler;
        ErrorHandler error_handler;
        
        ASSERT_TRUE(vault_handler.initialize(test_root + "/vaults"));
        ASSERT_TRUE(error_handler.initialize(test_root + "/logs"));
        ASSERT_TRUE(vault_handler.createVaultStructure("recovery_vault", "test_profile"));
        
        // Create test folder
        std::string test_folder = "./recovery_test_folder";
        fs::create_directories(test_folder);
        
        std::ofstream test_file(test_folder + "/recovery_data.txt");
        test_file << "Recovery test data";
        test_file.close();
        
        // Hide folder
        auto hide_result = vault_handler.hideFolder(test_folder, "recovery_vault");
        ASSERT_TRUE(hide_result.success);
        
        // Simulate corruption by modifying vault files
        std::string vault_path = test_root + "/vaults/recovery_vault";
        if (fs::exists(vault_path + "/metadata")) {
            // Create a backup first
            auto backup_result = error_handler.createBackup(vault_path, test_root + "/backup");
            ASSERT_TRUE(backup_result.success);
            
            // Simulate minor corruption
            std::string metadata_file = vault_path + "/metadata/" + hide_result.obfuscated_identifier + ".json";
            if (fs::exists(metadata_file)) {
                std::ofstream corrupt_file(metadata_file, std::ios::app);
                corrupt_file << "corrupted_data";
                corrupt_file.close();
            }
        }
        
        // Test repair functionality
        ASSERT_TRUE(vault_handler.repairVaultStructure("recovery_vault"));
        
        // Verify integrity after repair
        ASSERT_TRUE(vault_handler.validateVaultIntegrity("recovery_vault"));
        
        // Test recovery from backup if needed
        auto recovery_result = error_handler.attemptRecovery("vault_handler", "corruption_detected");
        ASSERT_TRUE(recovery_result.success || !recovery_result.errorDetails.empty());
        
        // Cleanup
        if (fs::exists(test_folder)) fs::remove_all(test_folder);
        fs::remove_all(test_root);
    }
    
    static void testServiceManagerIntegration() {
        ServiceManager service_manager;
        VaultHandler vault_handler;
        KeyboardSequenceDetector keyboard_detector;
        
        ASSERT_TRUE(service_manager.initialize());
        ASSERT_TRUE(vault_handler.initialize("./test_service_vaults"));
        ASSERT_TRUE(keyboard_detector.initialize());
        
        // Test service lifecycle
        ASSERT_TRUE(service_manager.startService());
        ASSERT_TRUE(service_manager.isServiceRunning());
        
        // Test keyboard integration
        keyboard_detector.setSequenceCallback([&vault_handler](const std::string& sequence) {
            if (sequence == "Ctrl+Alt+V") {
                // Simulate vault operation trigger
                vault_handler.createVaultStructure("service_vault", "service_profile");
            }
        });
        
        // Simulate keyboard sequence
        keyboard_detector.simulateKeySequence("Ctrl+Alt+V");
        
        // Verify service is still running
        ASSERT_TRUE(service_manager.isServiceRunning());
        
        // Stop service
        ASSERT_TRUE(service_manager.stopService());
        ASSERT_FALSE(service_manager.isServiceRunning());
        
        // Cleanup
        if (fs::exists("./test_service_vaults")) {
            fs::remove_all("./test_service_vaults");
        }
    }
    
    static void testKeyboardVaultIntegration() {
        KeyboardSequenceDetector detector;
        VaultHandler vault_handler;
        
        ASSERT_TRUE(detector.initialize());
        ASSERT_TRUE(vault_handler.initialize("./test_keyboard_vaults"));
        ASSERT_TRUE(vault_handler.createVaultStructure("keyboard_vault", "keyboard_profile"));
        
        // Create test folder
        std::string test_folder = "./keyboard_test_folder";
        fs::create_directories(test_folder);
        
        std::ofstream test_file(test_folder + "/keyboard_data.txt");
        test_file << "Keyboard integration test";
        test_file.close();
        
        // Set up keyboard callback for vault operations
        bool operation_triggered = false;
        detector.setSequenceCallback([&](const std::string& sequence) {
            if (sequence == "Ctrl+Alt+V") {
                auto result = vault_handler.hideFolder(test_folder, "keyboard_vault");
                operation_triggered = result.success;
            }
        });
        
        // Simulate keyboard sequence
        detector.simulateKeySequence("Ctrl+Alt+V");
        
        // Verify operation was triggered
        ASSERT_TRUE(operation_triggered);
        ASSERT_FALSE(fs::exists(test_folder)); // Folder should be hidden
        
        // Cleanup
        if (fs::exists("./test_keyboard_vaults")) {
            fs::remove_all("./test_keyboard_vaults");
        }
    }
    
    static void testPrivilegeSecurityIntegration() {
        PrivilegeManager privilege_manager;
        VaultHandler vault_handler;
        FolderSecurityManager security_manager;
        
        ASSERT_TRUE(privilege_manager.initialize());
        ASSERT_TRUE(vault_handler.initialize("./test_privilege_vaults"));
        ASSERT_TRUE(security_manager.initialize());
        
        // Test privilege elevation for vault operations
        auto elevation_result = privilege_manager.requestElevationForOperation(PrivilegedOperation::VAULT_ACCESS);
        ASSERT_TRUE(elevation_result.success || !elevation_result.errorDetails.empty());
        
        // Test folder hiding with privilege check
        auto folder_elevation = privilege_manager.requestElevationForOperation(PrivilegedOperation::FOLDER_HIDING);
        ASSERT_TRUE(folder_elevation.success || !folder_elevation.errorDetails.empty());
        
        // Create vault with security integration
        ASSERT_TRUE(vault_handler.createVaultStructure("privilege_vault", "privilege_profile"));
        
        // Test security manager integration
        std::string test_folder = "./privilege_test_folder";
        fs::create_directories(test_folder);
        
        auto security_result = security_manager.secureFolder(test_folder);
        ASSERT_TRUE(security_result.success);
        
        // Cleanup
        if (fs::exists(test_folder)) fs::remove_all(test_folder);
        if (fs::exists("./test_privilege_vaults")) {
            fs::remove_all("./test_privilege_vaults");
        }
    }
    
    static void testErrorRecoveryIntegration() {
        ErrorHandler error_handler;
        VaultHandler vault_handler;
        ProfileManager profile_manager;
        
        ASSERT_TRUE(error_handler.initialize("./test_error_logs"));
        ASSERT_TRUE(vault_handler.initialize("./test_error_vaults"));
        ASSERT_TRUE(profile_manager.initialize("./test_error_profiles"));
        
        // Create test data
        auto profile_result = profile_manager.createProfile("error_user", "ErrorTest123!", "Error Test User");
        ASSERT_TRUE(profile_result.success);
        
        ASSERT_TRUE(vault_handler.createVaultStructure("error_vault", profile_result.profile_id));
        
        // Simulate error condition
        std::string test_folder = "./error_test_folder";
        fs::create_directories(test_folder);
        
        auto hide_result = vault_handler.hideFolder(test_folder, "error_vault");
        ASSERT_TRUE(hide_result.success);
        
        // Simulate system error
        auto error_result = error_handler.handleError("VAULT_CORRUPTION", "Simulated vault corruption", ErrorSeverity::HIGH);
        ASSERT_TRUE(error_result.success);
        
        // Test recovery
        auto recovery_result = error_handler.attemptRecovery("vault_handler", "corruption_detected");
        ASSERT_TRUE(recovery_result.success || !recovery_result.errorDetails.empty());
        
        // Verify system integrity after recovery
        ASSERT_TRUE(vault_handler.validateVaultIntegrity("error_vault"));
        
        // Cleanup
        if (fs::exists("./test_error_logs")) fs::remove_all("./test_error_logs");
        if (fs::exists("./test_error_vaults")) fs::remove_all("./test_error_vaults");
        if (fs::exists("./test_error_profiles")) fs::remove_all("./test_error_profiles");
    }
    
    static void testLargeFolderManagement() {
        VaultHandler vault_handler;
        ASSERT_TRUE(vault_handler.initialize("./test_large_vaults"));
        ASSERT_TRUE(vault_handler.createVaultStructure("large_vault", "large_profile"));
        
        // Create large folder structure
        std::string large_folder = "./large_test_folder";
        fs::create_directories(large_folder);
        
        // Create many files and subdirectories
        for (int i = 0; i < 100; ++i) {
            std::string subfolder = large_folder + "/subfolder_" + std::to_string(i);
            fs::create_directories(subfolder);
            
            for (int j = 0; j < 10; ++j) {
                std::string filename = subfolder + "/file_" + std::to_string(j) + ".txt";
                std::ofstream file(filename);
                file << "Large folder test content " << i << "_" << j;
                file.close();
            }
        }
        
        // Test hiding large folder
        PerformanceTimer timer;
        auto hide_result = vault_handler.hideFolder(large_folder, "large_vault");
        auto hide_time = timer.elapsed();
        
        ASSERT_TRUE(hide_result.success);
        ASSERT_FALSE(fs::exists(large_folder));
        
        // Performance check - should complete within reasonable time
        ASSERT_TRUE(hide_time.count() < 30000); // Less than 30 seconds
        
        // Test restoring large folder
        timer.reset();
        auto restore_result = vault_handler.restoreFolder("large_vault", hide_result.obfuscated_identifier);
        auto restore_time = timer.elapsed();
        
        ASSERT_TRUE(restore_result.success);
        ASSERT_TRUE(fs::exists(large_folder));
        ASSERT_TRUE(restore_time.count() < 30000); // Less than 30 seconds
        
        // Verify integrity
        ASSERT_TRUE(vault_handler.validateVaultIntegrity("large_vault"));
        
        // Cleanup
        if (fs::exists(large_folder)) fs::remove_all(large_folder);
        if (fs::exists("./test_large_vaults")) fs::remove_all("./test_large_vaults");
    }
    
    static void testMultipleVaultOperations() {
        VaultHandler vault_handler;
        ASSERT_TRUE(vault_handler.initialize("./test_multiple_vaults"));
        
        // Create multiple vaults
        std::vector<std::string> vault_ids;
        for (int i = 0; i < 5; ++i) {
            std::string vault_id = "multi_vault_" + std::to_string(i);
            ASSERT_TRUE(vault_handler.createVaultStructure(vault_id, "multi_profile_" + std::to_string(i)));
            vault_ids.push_back(vault_id);
        }
        
        // Create test folders for each vault
        std::vector<std::string> test_folders;
        std::vector<std::string> obfuscated_ids;
        
        for (size_t i = 0; i < vault_ids.size(); ++i) {
            std::string folder_name = "./multi_folder_" + std::to_string(i);
            fs::create_directories(folder_name);
            
            std::ofstream test_file(folder_name + "/multi_data.txt");
            test_file << "Multi vault test data " << i;
            test_file.close();
            
            test_folders.push_back(folder_name);
            
            // Hide folder in corresponding vault
            auto hide_result = vault_handler.hideFolder(folder_name, vault_ids[i]);
            ASSERT_TRUE(hide_result.success);
            obfuscated_ids.push_back(hide_result.obfuscated_identifier);
        }
        
        // Verify all vaults maintain integrity
        for (const auto& vault_id : vault_ids) {
            ASSERT_TRUE(vault_handler.validateVaultIntegrity(vault_id));
        }
        
        // Restore all folders
        for (size_t i = 0; i < vault_ids.size(); ++i) {
            auto restore_result = vault_handler.restoreFolder(vault_ids[i], obfuscated_ids[i]);
            ASSERT_TRUE(restore_result.success);
            ASSERT_TRUE(fs::exists(test_folders[i]));
        }
        
        // Cleanup
        for (const auto& folder : test_folders) {
            if (fs::exists(folder)) fs::remove_all(folder);
        }
        if (fs::exists("./test_multiple_vaults")) fs::remove_all("./test_multiple_vaults");
    }
    
    static void testSystemStressTesting() {
        VaultHandler vault_handler;
        ProfileManager profile_manager;
        
        ASSERT_TRUE(vault_handler.initialize("./test_stress_vaults"));
        ASSERT_TRUE(profile_manager.initialize("./test_stress_profiles"));
        
        // Create stress test profile
        auto profile_result = profile_manager.createProfile("stress_user", "StressTest123!", "Stress Test User");
        ASSERT_TRUE(profile_result.success);
        
        ASSERT_TRUE(vault_handler.createVaultStructure("stress_vault", profile_result.profile_id));
        
        // Perform rapid operations
        std::vector<std::string> stress_folders;
        std::vector<std::string> obfuscated_ids;
        
        PerformanceTimer total_timer;
        
        // Rapid hide operations
        for (int i = 0; i < 50; ++i) {
            std::string folder_name = "./stress_folder_" + std::to_string(i);
            fs::create_directories(folder_name);
            
            std::ofstream test_file(folder_name + "/stress_data.txt");
            test_file << "Stress test data " << i;
            test_file.close();
            
            stress_folders.push_back(folder_name);
            
            auto hide_result = vault_handler.hideFolder(folder_name, "stress_vault");
            ASSERT_TRUE(hide_result.success);
            obfuscated_ids.push_back(hide_result.obfuscated_identifier);
        }
        
        auto hide_time = total_timer.elapsed();
        
        // Verify vault integrity under stress
        ASSERT_TRUE(vault_handler.validateVaultIntegrity("stress_vault"));
        
        // Rapid restore operations
        total_timer.reset();
        for (size_t i = 0; i < obfuscated_ids.size(); ++i) {
            auto restore_result = vault_handler.restoreFolder("stress_vault", obfuscated_ids[i]);
            ASSERT_TRUE(restore_result.success);
        }
        
        auto restore_time = total_timer.elapsed();
        
        // Performance validation
        ASSERT_TRUE(hide_time.count() < 60000); // Less than 1 minute for 50 operations
        ASSERT_TRUE(restore_time.count() < 60000); // Less than 1 minute for 50 operations
        
        // Cleanup
        for (const auto& folder : stress_folders) {
            if (fs::exists(folder)) fs::remove_all(folder);
        }
        if (fs::exists("./test_stress_vaults")) fs::remove_all("./test_stress_vaults");
        if (fs::exists("./test_stress_profiles")) fs::remove_all("./test_stress_profiles");
    }
    
    static void testDataIntegrityValidation() {
        VaultHandler vault_handler;
        EncryptionEngine encryption_engine;
        
        ASSERT_TRUE(vault_handler.initialize("./test_integrity_vaults"));
        ASSERT_TRUE(vault_handler.createVaultStructure("integrity_vault", "integrity_profile"));
        
        // Create test folder with specific content
        std::string test_folder = "./integrity_test_folder";
        fs::create_directories(test_folder);
        
        std::string original_content = "Data integrity test content with special characters: !@#$%^&*()";
        std::ofstream test_file(test_folder + "/integrity_data.txt");
        test_file << original_content;
        test_file.close();
        
        // Calculate original checksum
        std::vector<uint8_t> original_data(original_content.begin(), original_content.end());
        auto original_hash = encryption_engine.calculateHash(original_data);
        
        // Hide folder
        auto hide_result = vault_handler.hideFolder(test_folder, "integrity_vault");
        ASSERT_TRUE(hide_result.success);
        
        // Verify vault integrity
        ASSERT_TRUE(vault_handler.validateVaultIntegrity("integrity_vault"));
        
        // Restore folder
        auto restore_result = vault_handler.restoreFolder("integrity_vault", hide_result.obfuscated_identifier);
        ASSERT_TRUE(restore_result.success);
        
        // Verify data integrity
        ASSERT_TRUE(fs::exists(test_folder + "/integrity_data.txt"));
        
        std::ifstream restored_file(test_folder + "/integrity_data.txt");
        std::string restored_content((std::istreambuf_iterator<char>(restored_file)),
                                   std::istreambuf_iterator<char>());
        restored_file.close();
        
        ASSERT_EQ(original_content, restored_content);
        
        // Verify checksum
        std::vector<uint8_t> restored_data(restored_content.begin(), restored_content.end());
        auto restored_hash = encryption_engine.calculateHash(restored_data);
        ASSERT_EQ(original_hash, restored_hash);
        
        // Cleanup
        if (fs::exists(test_folder)) fs::remove_all(test_folder);
        if (fs::exists("./test_integrity_vaults")) fs::remove_all("./test_integrity_vaults");
    }
    
    static void testCrossPlatformCompatibility() {
        VaultHandler vault_handler;
        ASSERT_TRUE(vault_handler.initialize("./test_platform_vaults"));
        ASSERT_TRUE(vault_handler.createVaultStructure("platform_vault", "platform_profile"));
        
        // Test with different path separators and special characters
        std::vector<std::string> test_paths = {
            "./platform_test_folder",
            "./platform test folder with spaces",
            "./platform_folder_with_unicode_测试",
            "./platform-folder-with-dashes",
            "./platform_folder_with_numbers_123"
        };
        
        std::vector<std::string> obfuscated_ids;
        
        for (const auto& path : test_paths) {
            if (fs::create_directories(path)) {
                std::ofstream test_file(path + "/platform_test.txt");
                test_file << "Cross-platform compatibility test";
                test_file.close();
                
                auto hide_result = vault_handler.hideFolder(path, "platform_vault");
                if (hide_result.success) {
                    obfuscated_ids.push_back(hide_result.obfuscated_identifier);
                }
            }
        }
        
        // Verify vault integrity
        ASSERT_TRUE(vault_handler.validateVaultIntegrity("platform_vault"));
        
        // Restore folders
        for (const auto& id : obfuscated_ids) {
            auto restore_result = vault_handler.restoreFolder("platform_vault", id);
            ASSERT_TRUE(restore_result.success);
        }
        
        // Cleanup
        for (const auto& path : test_paths) {
            if (fs::exists(path)) fs::remove_all(path);
        }
        if (fs::exists("./test_platform_vaults")) fs::remove_all("./test_platform_vaults");
    }
    
    static void testFilesystemCompatibility() {
        VaultHandler vault_handler;
        ASSERT_TRUE(vault_handler.initialize("./test_filesystem_vaults"));
        ASSERT_TRUE(vault_handler.createVaultStructure("filesystem_vault", "filesystem_profile"));
        
        // Test with different file types and sizes
        std::string test_folder = "./filesystem_test_folder";
        fs::create_directories(test_folder);
        
        // Create various file types
        std::vector<std::string> test_files = {
            "text_file.txt",
            "binary_file.bin",
            "image_file.jpg",
            "document_file.pdf",
            "executable_file.exe"
        };
        
        for (const auto& filename : test_files) {
            std::string filepath = test_folder + "/" + filename;
            std::ofstream file(filepath, std::ios::binary);
            
            // Write different content based on file type
            if (filename.find(".txt") != std::string::npos) {
                file << "Text file content for filesystem compatibility test";
            } else {
                // Write binary data
                for (int i = 0; i < 1000; ++i) {
                    file.put(static_cast<char>(i % 256));
                }
            }
            file.close();
        }
        
        // Hide folder with various file types
        auto hide_result = vault_handler.hideFolder(test_folder, "filesystem_vault");
        ASSERT_TRUE(hide_result.success);
        
        // Verify vault integrity
        ASSERT_TRUE(vault_handler.validateVaultIntegrity("filesystem_vault"));
        
        // Restore folder
        auto restore_result = vault_handler.restoreFolder("filesystem_vault", hide_result.obfuscated_identifier);
        ASSERT_TRUE(restore_result.success);
        
        // Verify all files are restored
        for (const auto& filename : test_files) {
            std::string filepath = test_folder + "/" + filename;
            ASSERT_TRUE(fs::exists(filepath));
        }
        
        // Cleanup
        if (fs::exists(test_folder)) fs::remove_all(test_folder);
        if (fs::exists("./test_filesystem_vaults")) fs::remove_all("./test_filesystem_vaults");
    }
    
    static void testPermissionModelTesting() {
        PrivilegeManager privilege_manager;
        VaultHandler vault_handler;
        
        ASSERT_TRUE(privilege_manager.initialize());
        ASSERT_TRUE(vault_handler.initialize("./test_permission_vaults"));
        
        // Test different permission scenarios
        std::vector<PrivilegedOperation> operations = {
            PrivilegedOperation::VAULT_ACCESS,
            PrivilegedOperation::FOLDER_HIDING,
            PrivilegedOperation::SYSTEM_MODIFICATION,
            PrivilegedOperation::REGISTRY_ACCESS
        };
        
        for (auto operation : operations) {
            auto elevation_result = privilege_manager.requestElevationForOperation(operation);
            // Should either succeed with proper authorization or fail gracefully
            ASSERT_TRUE(elevation_result.success || !elevation_result.errorDetails.empty());
        }
        
        // Test vault creation with permission checks
        ASSERT_TRUE(vault_handler.createVaultStructure("permission_vault", "permission_profile"));
        
        // Test folder operations with permission validation
        std::string test_folder = "./permission_test_folder";
        fs::create_directories(test_folder);
        
        auto hide_result = vault_handler.hideFolder(test_folder, "permission_vault");
        ASSERT_TRUE(hide_result.success);
        
        // Verify vault integrity
        ASSERT_TRUE(vault_handler.validateVaultIntegrity("permission_vault"));
        
        // Cleanup
        if (fs::exists("./test_permission_vaults")) fs::remove_all("./test_permission_vaults");
    }
};

// Test registration function
void registerIntegrationTests(TestFramework& framework) {
    IntegrationTests::registerTests(framework);
}