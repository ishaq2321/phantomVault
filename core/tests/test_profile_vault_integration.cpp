/**
 * Integration Tests for Profile Vault System
 * 
 * Tests for profile isolation, access control, vault management,
 * and cross-profile security.
 */

#include "test_framework.hpp"
#include "../include/profile_vault.hpp"
#include "../include/profile_manager.hpp"
#include "../include/folder_security_manager.hpp"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace PhantomVault;
using namespace phantomvault::testing;

namespace fs = std::filesystem;

class ProfileVaultIntegrationTests {
public:
    static void registerTests(TestFramework& framework) {
        // Profile isolation tests
        REGISTER_TEST(framework, "ProfileVault", "profile_isolation", testProfileIsolation);
        REGISTER_TEST(framework, "ProfileVault", "cross_profile_access_denied", testCrossProfileAccessDenied);
        REGISTER_TEST(framework, "ProfileVault", "profile_vault_separation", testProfileVaultSeparation);
        
        // Access control tests
        REGISTER_TEST(framework, "ProfileVault", "authentication_required", testAuthenticationRequired);
        REGISTER_TEST(framework, "ProfileVault", "master_key_validation", testMasterKeyValidation);
        REGISTER_TEST(framework, "ProfileVault", "session_management", testSessionManagement);
        
        // Vault management tests
        REGISTER_TEST(framework, "ProfileVault", "vault_creation_cleanup", testVaultCreationCleanup);
        REGISTER_TEST(framework, "ProfileVault", "concurrent_vault_access", testConcurrentVaultAccess);
        REGISTER_TEST(framework, "ProfileVault", "vault_integrity_checks", testVaultIntegrityChecks);
        
        // Folder operations tests
        REGISTER_TEST(framework, "ProfileVault", "folder_encryption_isolation", testFolderEncryptionIsolation);
        REGISTER_TEST(framework, "ProfileVault", "temporary_unlock_isolation", testTemporaryUnlockIsolation);
        REGISTER_TEST(framework, "ProfileVault", "permanent_unlock_cleanup", testPermanentUnlockCleanup);
        
        // Security tests
        REGISTER_TEST(framework, "ProfileVault", "vault_metadata_protection", testVaultMetadataProtection);
        REGISTER_TEST(framework, "ProfileVault", "encrypted_storage_verification", testEncryptedStorageVerification);
        REGISTER_TEST(framework, "ProfileVault", "recovery_key_isolation", testRecoveryKeyIsolation);
    }

private:
    static std::string createTestFolder(const std::string& name, const std::string& content = "test content") {
        std::string folder_path = "./test_" + name + "_folder";
        fs::create_directories(folder_path);
        
        std::ofstream file(folder_path + "/test_file.txt");
        file << content;
        file.close();
        
        std::ofstream file2(folder_path + "/test_file2.txt");
        file2 << content + " - file 2";
        file2.close();
        
        return folder_path;
    }
    
    static void cleanupTestFolder(const std::string& folder_path) {
        if (fs::exists(folder_path)) {
            fs::remove_all(folder_path);
        }
    }
    
    static void testProfileIsolation() {
        std::string vault_root = "./test_vault_isolation";
        
        // Clean up any existing test data
        if (fs::exists(vault_root)) {
            fs::remove_all(vault_root);
        }
        
        // Create two separate profiles
        ProfileVault vault1("profile1", vault_root);
        ProfileVault vault2("profile2", vault_root);
        
        ASSERT_TRUE(vault1.initialize());
        ASSERT_TRUE(vault2.initialize());
        
        // Create test folders
        std::string folder1 = createTestFolder("profile1", "Profile 1 content");
        std::string folder2 = createTestFolder("profile2", "Profile 2 content");
        
        std::string master_key1 = "master_key_profile1";
        std::string master_key2 = "master_key_profile2";
        
        // Lock folders in separate profiles
        auto result1 = vault1.lockFolder(folder1, master_key1);
        auto result2 = vault2.lockFolder(folder2, master_key2);
        
        ASSERT_TRUE(result1.success);
        ASSERT_TRUE(result2.success);
        
        // Verify profile isolation - each profile should only see its own folders
        auto folders1 = vault1.getLockedFolders();
        auto folders2 = vault2.getLockedFolders();
        
        ASSERT_EQ(folders1.size(), 1);
        ASSERT_EQ(folders2.size(), 1);
        
        ASSERT_EQ(folders1[0].original_path, folder1);
        ASSERT_EQ(folders2[0].original_path, folder2);
        
        // Verify vault paths are separate
        ASSERT_NE(folders1[0].vault_location, folders2[0].vault_location);
        
        // Cleanup
        cleanupTestFolder(folder1);
        cleanupTestFolder(folder2);
        fs::remove_all(vault_root);
    }
    
    static void testCrossProfileAccessDenied() {
        std::string vault_root = "./test_cross_profile_access";
        
        if (fs::exists(vault_root)) {
            fs::remove_all(vault_root);
        }
        
        ProfileVault vault1("profile1", vault_root);
        ProfileVault vault2("profile2", vault_root);
        
        ASSERT_TRUE(vault1.initialize());
        ASSERT_TRUE(vault2.initialize());
        
        std::string test_folder = createTestFolder("cross_access", "Cross access test content");
        std::string master_key1 = "master_key_profile1";
        std::string master_key2 = "master_key_profile2";
        
        // Lock folder in profile1
        auto lock_result = vault1.lockFolder(test_folder, master_key1);
        ASSERT_TRUE(lock_result.success);
        
        // Try to unlock from profile2 with profile2's key - should fail
        auto unlock_result = vault2.unlockFolder(test_folder, master_key2, UnlockMode::TEMPORARY);
        ASSERT_FALSE(unlock_result.success);
        
        // Try to unlock from profile2 with profile1's key - should still fail (different profile)
        unlock_result = vault2.unlockFolder(test_folder, master_key1, UnlockMode::TEMPORARY);
        ASSERT_FALSE(unlock_result.success);
        
        // Verify profile1 can still unlock its own folder
        unlock_result = vault1.unlockFolder(test_folder, master_key1, UnlockMode::TEMPORARY);
        ASSERT_TRUE(unlock_result.success);
        
        // Cleanup
        cleanupTestFolder(test_folder);
        fs::remove_all(vault_root);
    }
    
    static void testProfileVaultSeparation() {
        std::string vault_root = "./test_vault_separation";
        
        if (fs::exists(vault_root)) {
            fs::remove_all(vault_root);
        }
        
        ProfileVault vault1("profile1", vault_root);
        ProfileVault vault2("profile2", vault_root);
        
        ASSERT_TRUE(vault1.initialize());
        ASSERT_TRUE(vault2.initialize());
        
        // Verify separate vault directories exist
        ASSERT_TRUE(fs::exists(vault_root + "/profile1"));
        ASSERT_TRUE(fs::exists(vault_root + "/profile2"));
        
        // Verify vault directories have proper structure
        ASSERT_TRUE(fs::exists(vault_root + "/profile1/folders"));
        ASSERT_TRUE(fs::exists(vault_root + "/profile1/metadata"));
        ASSERT_TRUE(fs::exists(vault_root + "/profile2/folders"));
        ASSERT_TRUE(fs::exists(vault_root + "/profile2/metadata"));
        
        // Verify vault paths are different
        ASSERT_NE(vault1.getVaultPath(), vault2.getVaultPath());
        
        // Cleanup
        fs::remove_all(vault_root);
    }
    
    static void testAuthenticationRequired() {
        std::string vault_root = "./test_authentication";
        
        if (fs::exists(vault_root)) {
            fs::remove_all(vault_root);
        }
        
        ProfileVault vault("test_profile", vault_root);
        ASSERT_TRUE(vault.initialize());
        
        std::string test_folder = createTestFolder("auth_test", "Authentication test content");
        std::string correct_key = "correct_master_key";
        std::string wrong_key = "wrong_master_key";
        
        // Lock folder with correct key
        auto lock_result = vault.lockFolder(test_folder, correct_key);
        ASSERT_TRUE(lock_result.success);
        
        // Try to unlock with wrong key - should fail
        auto unlock_result = vault.unlockFolder(test_folder, wrong_key, UnlockMode::TEMPORARY);
        ASSERT_FALSE(unlock_result.success);
        ASSERT_FALSE(unlock_result.error_details.empty());
        
        // Unlock with correct key - should succeed
        unlock_result = vault.unlockFolder(test_folder, correct_key, UnlockMode::TEMPORARY);
        ASSERT_TRUE(unlock_result.success);
        
        // Cleanup
        cleanupTestFolder(test_folder);
        fs::remove_all(vault_root);
    }
    
    static void testMasterKeyValidation() {
        std::string vault_root = "./test_key_validation";
        
        if (fs::exists(vault_root)) {
            fs::remove_all(vault_root);
        }
        
        ProfileVault vault("test_profile", vault_root);
        ASSERT_TRUE(vault.initialize());
        
        // Test various invalid keys
        std::vector<std::string> invalid_keys = {
            "",           // Empty key
            "a",          // Too short
            "12345",      // Numeric only
            "     ",      // Whitespace only
        };
        
        std::string test_folder = createTestFolder("key_validation", "Key validation test");
        
        for (const auto& invalid_key : invalid_keys) {
            auto result = vault.lockFolder(test_folder, invalid_key);
            // Should either fail or handle gracefully
            if (result.success) {
                // If it succeeds, it should still work for unlock
                auto unlock_result = vault.unlockFolder(test_folder, invalid_key, UnlockMode::TEMPORARY);
                ASSERT_TRUE(unlock_result.success);
            }
        }
        
        // Test valid key
        std::string valid_key = "valid_master_key_123!@#";
        auto result = vault.lockFolder(test_folder, valid_key);
        ASSERT_TRUE(result.success);
        
        auto unlock_result = vault.unlockFolder(test_folder, valid_key, UnlockMode::TEMPORARY);
        ASSERT_TRUE(unlock_result.success);
        
        // Cleanup
        cleanupTestFolder(test_folder);
        fs::remove_all(vault_root);
    }
    
    static void testSessionManagement() {
        std::string vault_root = "./test_session_management";
        
        if (fs::exists(vault_root)) {
            fs::remove_all(vault_root);
        }
        
        ProfileVault vault("test_profile", vault_root);
        ASSERT_TRUE(vault.initialize());
        
        std::string test_folder = createTestFolder("session_test", "Session management test");
        std::string master_key = "session_master_key";
        
        // Lock folder
        auto lock_result = vault.lockFolder(test_folder, master_key);
        ASSERT_TRUE(lock_result.success);
        
        // Unlock temporarily
        auto unlock_result = vault.unlockFolder(test_folder, master_key, UnlockMode::TEMPORARY);
        ASSERT_TRUE(unlock_result.success);
        
        // Verify folder is temporarily unlocked
        auto folder_info = vault.getFolderInfo(test_folder);
        ASSERT_TRUE(folder_info.has_value());
        ASSERT_TRUE(folder_info->is_temporarily_unlocked);
        
        // Re-lock temporary folders
        auto relock_result = vault.relockTemporaryFolders();
        ASSERT_TRUE(relock_result.success);
        
        // Verify folder is locked again
        folder_info = vault.getFolderInfo(test_folder);
        ASSERT_TRUE(folder_info.has_value());
        ASSERT_FALSE(folder_info->is_temporarily_unlocked);
        
        // Cleanup
        cleanupTestFolder(test_folder);
        fs::remove_all(vault_root);
    }
    
    static void testVaultCreationCleanup() {
        std::string vault_root = "./test_vault_cleanup";
        
        if (fs::exists(vault_root)) {
            fs::remove_all(vault_root);
        }
        
        {
            ProfileVault vault("cleanup_test", vault_root);
            ASSERT_TRUE(vault.initialize());
            
            // Verify vault structure is created
            ASSERT_TRUE(fs::exists(vault_root + "/cleanup_test"));
            ASSERT_TRUE(fs::exists(vault_root + "/cleanup_test/folders"));
            ASSERT_TRUE(fs::exists(vault_root + "/cleanup_test/metadata"));
        }
        
        // Vault object destroyed, but files should remain
        ASSERT_TRUE(fs::exists(vault_root + "/cleanup_test"));
        
        // Manual cleanup
        fs::remove_all(vault_root);
        ASSERT_FALSE(fs::exists(vault_root));
    }
    
    static void testConcurrentVaultAccess() {
        std::string vault_root = "./test_concurrent_access";
        
        if (fs::exists(vault_root)) {
            fs::remove_all(vault_root);
        }
        
        ProfileVault vault("concurrent_test", vault_root);
        ASSERT_TRUE(vault.initialize());
        
        std::string test_folder1 = createTestFolder("concurrent1", "Concurrent test 1");
        std::string test_folder2 = createTestFolder("concurrent2", "Concurrent test 2");
        std::string master_key = "concurrent_master_key";
        
        std::vector<std::thread> threads;
        std::vector<bool> results(4, false);
        
        // Launch concurrent operations
        threads.emplace_back([&vault, &test_folder1, &master_key, &results]() {
            auto result = vault.lockFolder(test_folder1, master_key);
            results[0] = result.success;
        });
        
        threads.emplace_back([&vault, &test_folder2, &master_key, &results]() {
            auto result = vault.lockFolder(test_folder2, master_key);
            results[1] = result.success;
        });
        
        threads.emplace_back([&vault, &test_folder1, &master_key, &results]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            auto result = vault.unlockFolder(test_folder1, master_key, UnlockMode::TEMPORARY);
            results[2] = result.success;
        });
        
        threads.emplace_back([&vault, &test_folder2, &master_key, &results]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            auto result = vault.unlockFolder(test_folder2, master_key, UnlockMode::TEMPORARY);
            results[3] = result.success;
        });
        
        // Wait for all threads
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Verify all operations succeeded
        for (bool result : results) {
            ASSERT_TRUE(result);
        }
        
        // Cleanup
        cleanupTestFolder(test_folder1);
        cleanupTestFolder(test_folder2);
        fs::remove_all(vault_root);
    }
    
    static void testVaultIntegrityChecks() {
        std::string vault_root = "./test_vault_integrity";
        
        if (fs::exists(vault_root)) {
            fs::remove_all(vault_root);
        }
        
        ProfileVault vault("integrity_test", vault_root);
        ASSERT_TRUE(vault.initialize());
        
        std::string test_folder = createTestFolder("integrity", "Integrity test content");
        std::string master_key = "integrity_master_key";
        
        // Lock folder
        auto lock_result = vault.lockFolder(test_folder, master_key);
        ASSERT_TRUE(lock_result.success);
        
        // Verify vault integrity
        ASSERT_TRUE(vault.verifyIntegrity());
        
        // Get folder info to find vault location
        auto folder_info = vault.getFolderInfo(test_folder);
        ASSERT_TRUE(folder_info.has_value());
        
        // Corrupt vault metadata (simulate corruption)
        std::string metadata_file = vault_root + "/integrity_test/vault_metadata.json";
        if (fs::exists(metadata_file)) {
            std::ofstream corrupt_file(metadata_file, std::ios::app);
            corrupt_file << "CORRUPTED_DATA";
            corrupt_file.close();
        }
        
        // Integrity check should detect corruption
        ASSERT_FALSE(vault.verifyIntegrity());
        
        // Cleanup
        cleanupTestFolder(test_folder);
        fs::remove_all(vault_root);
    }
    
    static void testFolderEncryptionIsolation() {
        std::string vault_root = "./test_encryption_isolation";
        
        if (fs::exists(vault_root)) {
            fs::remove_all(vault_root);
        }
        
        ProfileVault vault1("profile1", vault_root);
        ProfileVault vault2("profile2", vault_root);
        
        ASSERT_TRUE(vault1.initialize());
        ASSERT_TRUE(vault2.initialize());
        
        std::string folder1 = createTestFolder("encrypt1", "Profile 1 sensitive data");
        std::string folder2 = createTestFolder("encrypt2", "Profile 2 sensitive data");
        
        std::string key1 = "profile1_encryption_key";
        std::string key2 = "profile2_encryption_key";
        
        // Lock folders with different keys
        auto result1 = vault1.lockFolder(folder1, key1);
        auto result2 = vault2.lockFolder(folder2, key2);
        
        ASSERT_TRUE(result1.success);
        ASSERT_TRUE(result2.success);
        
        // Verify encrypted data is different and isolated
        auto info1 = vault1.getFolderInfo(folder1);
        auto info2 = vault2.getFolderInfo(folder2);
        
        ASSERT_TRUE(info1.has_value());
        ASSERT_TRUE(info2.has_value());
        
        // Vault locations should be different
        ASSERT_NE(info1->vault_location, info2->vault_location);
        
        // Verify cross-profile decryption fails
        auto cross_unlock1 = vault2.unlockFolder(folder1, key2, UnlockMode::TEMPORARY);
        auto cross_unlock2 = vault1.unlockFolder(folder2, key1, UnlockMode::TEMPORARY);
        
        ASSERT_FALSE(cross_unlock1.success);
        ASSERT_FALSE(cross_unlock2.success);
        
        // Cleanup
        cleanupTestFolder(folder1);
        cleanupTestFolder(folder2);
        fs::remove_all(vault_root);
    }
    
    static void testTemporaryUnlockIsolation() {
        std::string vault_root = "./test_temp_unlock_isolation";
        
        if (fs::exists(vault_root)) {
            fs::remove_all(vault_root);
        }
        
        ProfileVault vault1("profile1", vault_root);
        ProfileVault vault2("profile2", vault_root);
        
        ASSERT_TRUE(vault1.initialize());
        ASSERT_TRUE(vault2.initialize());
        
        std::string folder1 = createTestFolder("temp1", "Temporary unlock test 1");
        std::string folder2 = createTestFolder("temp2", "Temporary unlock test 2");
        
        std::string key1 = "temp_key1";
        std::string key2 = "temp_key2";
        
        // Lock and temporarily unlock folders
        vault1.lockFolder(folder1, key1);
        vault2.lockFolder(folder2, key2);
        
        vault1.unlockFolder(folder1, key1, UnlockMode::TEMPORARY);
        vault2.unlockFolder(folder2, key2, UnlockMode::TEMPORARY);
        
        // Verify temporary unlock state is isolated
        auto info1 = vault1.getFolderInfo(folder1);
        auto info2 = vault2.getFolderInfo(folder2);
        
        ASSERT_TRUE(info1.has_value());
        ASSERT_TRUE(info2.has_value());
        ASSERT_TRUE(info1->is_temporarily_unlocked);
        ASSERT_TRUE(info2->is_temporarily_unlocked);
        
        // Re-lock profile1's temporary folders
        auto relock1 = vault1.relockTemporaryFolders();
        ASSERT_TRUE(relock1.success);
        
        // Verify only profile1's folder is re-locked
        info1 = vault1.getFolderInfo(folder1);
        info2 = vault2.getFolderInfo(folder2);
        
        ASSERT_TRUE(info1.has_value());
        ASSERT_TRUE(info2.has_value());
        ASSERT_FALSE(info1->is_temporarily_unlocked);
        ASSERT_TRUE(info2->is_temporarily_unlocked); // Still unlocked
        
        // Cleanup
        cleanupTestFolder(folder1);
        cleanupTestFolder(folder2);
        fs::remove_all(vault_root);
    }
    
    static void testPermanentUnlockCleanup() {
        std::string vault_root = "./test_permanent_unlock";
        
        if (fs::exists(vault_root)) {
            fs::remove_all(vault_root);
        }
        
        ProfileVault vault("permanent_test", vault_root);
        ASSERT_TRUE(vault.initialize());
        
        std::string test_folder = createTestFolder("permanent", "Permanent unlock test");
        std::string master_key = "permanent_master_key";
        
        // Lock folder
        auto lock_result = vault.lockFolder(test_folder, master_key);
        ASSERT_TRUE(lock_result.success);
        
        // Verify folder is in vault
        auto folders_before = vault.getLockedFolders();
        ASSERT_EQ(folders_before.size(), 1);
        
        auto folder_info = vault.getFolderInfo(test_folder);
        ASSERT_TRUE(folder_info.has_value());
        std::string vault_location = folder_info->vault_location;
        
        // Permanently unlock folder
        auto unlock_result = vault.unlockFolder(test_folder, master_key, UnlockMode::PERMANENT);
        ASSERT_TRUE(unlock_result.success);
        
        // Verify folder is removed from vault tracking
        auto folders_after = vault.getLockedFolders();
        ASSERT_EQ(folders_after.size(), 0);
        
        // Verify vault files are cleaned up
        folder_info = vault.getFolderInfo(test_folder);
        ASSERT_FALSE(folder_info.has_value());
        
        // Cleanup
        cleanupTestFolder(test_folder);
        fs::remove_all(vault_root);
    }
    
    static void testVaultMetadataProtection() {
        std::string vault_root = "./test_metadata_protection";
        
        if (fs::exists(vault_root)) {
            fs::remove_all(vault_root);
        }
        
        ProfileVault vault("metadata_test", vault_root);
        ASSERT_TRUE(vault.initialize());
        
        std::string test_folder = createTestFolder("metadata", "Metadata protection test");
        std::string master_key = "metadata_master_key";
        
        // Lock folder
        auto lock_result = vault.lockFolder(test_folder, master_key);
        ASSERT_TRUE(lock_result.success);
        
        // Verify metadata files have proper permissions
        std::string metadata_file = vault_root + "/metadata_test/vault_metadata.json";
        ASSERT_TRUE(fs::exists(metadata_file));
        
        // Check file permissions (owner-only access)
        auto perms = fs::status(metadata_file).permissions();
        ASSERT_TRUE((perms & fs::perms::others_read) == fs::perms::none);
        ASSERT_TRUE((perms & fs::perms::others_write) == fs::perms::none);
        ASSERT_TRUE((perms & fs::perms::group_read) == fs::perms::none);
        ASSERT_TRUE((perms & fs::perms::group_write) == fs::perms::none);
        
        // Cleanup
        cleanupTestFolder(test_folder);
        fs::remove_all(vault_root);
    }
    
    static void testEncryptedStorageVerification() {
        std::string vault_root = "./test_encrypted_storage";
        
        if (fs::exists(vault_root)) {
            fs::remove_all(vault_root);
        }
        
        ProfileVault vault("storage_test", vault_root);
        ASSERT_TRUE(vault.initialize());
        
        std::string test_folder = createTestFolder("storage", "Encrypted storage verification");
        std::string master_key = "storage_master_key";
        
        // Read original content
        std::ifstream original_file(test_folder + "/test_file.txt");
        std::string original_content((std::istreambuf_iterator<char>(original_file)),
                                   std::istreambuf_iterator<char>());
        
        // Lock folder (encrypt and store)
        auto lock_result = vault.lockFolder(test_folder, master_key);
        ASSERT_TRUE(lock_result.success);
        
        // Verify original folder is hidden/moved
        ASSERT_FALSE(fs::exists(test_folder + "/test_file.txt"));
        
        // Find encrypted storage location
        auto folder_info = vault.getFolderInfo(test_folder);
        ASSERT_TRUE(folder_info.has_value());
        
        std::string vault_folder = vault_root + "/storage_test/folders/" + folder_info->vault_location;
        ASSERT_TRUE(fs::exists(vault_folder));
        
        // Verify stored data is encrypted (not readable as plaintext)
        for (const auto& entry : fs::recursive_directory_iterator(vault_folder)) {
            if (entry.is_regular_file()) {
                std::ifstream encrypted_file(entry.path(), std::ios::binary);
                std::string encrypted_content((std::istreambuf_iterator<char>(encrypted_file)),
                                            std::istreambuf_iterator<char>());
                
                // Encrypted content should not contain original plaintext
                ASSERT_TRUE(encrypted_content.find(original_content) == std::string::npos);
            }
        }
        
        // Unlock and verify content is restored
        auto unlock_result = vault.unlockFolder(test_folder, master_key, UnlockMode::TEMPORARY);
        ASSERT_TRUE(unlock_result.success);
        
        std::ifstream restored_file(test_folder + "/test_file.txt");
        std::string restored_content((std::istreambuf_iterator<char>(restored_file)),
                                   std::istreambuf_iterator<char>());
        
        ASSERT_EQ(original_content, restored_content);
        
        // Cleanup
        cleanupTestFolder(test_folder);
        fs::remove_all(vault_root);
    }
    
    static void testRecoveryKeyIsolation() {
        std::string vault_root = "./test_recovery_isolation";
        
        if (fs::exists(vault_root)) {
            fs::remove_all(vault_root);
        }
        
        // This test would require ProfileManager integration
        // For now, we'll test the concept with separate vault instances
        
        ProfileVault vault1("recovery1", vault_root);
        ProfileVault vault2("recovery2", vault_root);
        
        ASSERT_TRUE(vault1.initialize());
        ASSERT_TRUE(vault2.initialize());
        
        // Verify vaults are isolated
        ASSERT_NE(vault1.getVaultPath(), vault2.getVaultPath());
        
        // Each vault should have separate metadata
        std::string metadata1 = vault_root + "/recovery1/vault_metadata.json";
        std::string metadata2 = vault_root + "/recovery2/vault_metadata.json";
        
        ASSERT_TRUE(fs::exists(metadata1));
        ASSERT_TRUE(fs::exists(metadata2));
        
        // Metadata files should be different
        std::ifstream file1(metadata1);
        std::ifstream file2(metadata2);
        
        std::string content1((std::istreambuf_iterator<char>(file1)),
                           std::istreambuf_iterator<char>());
        std::string content2((std::istreambuf_iterator<char>(file2)),
                           std::istreambuf_iterator<char>());
        
        ASSERT_NE(content1, content2);
        
        // Cleanup
        fs::remove_all(vault_root);
    }
};

// Test registration function
void registerProfileVaultIntegrationTests(TestFramework& framework) {
    ProfileVaultIntegrationTests::registerTests(framework);
}