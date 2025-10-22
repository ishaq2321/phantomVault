#include <gtest/gtest.h>
#include "phantom_vault/vault_encryption_manager.hpp"
#include <filesystem>
#include <fstream>
#include <string>

using namespace phantom_vault::service;

class VaultEncryptionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory
        test_dir_ = "/tmp/phantom_vault_encryption_test_" + std::to_string(getpid());
        std::filesystem::create_directories(test_dir_);
        
        manager_ = std::make_unique<VaultEncryptionManager>();
        ASSERT_TRUE(manager_->initialize());
    }
    
    void TearDown() override {
        manager_.reset();
        std::filesystem::remove_all(test_dir_);
    }
    
    void createTestFolder() {
        test_folder_ = test_dir_ + "/test_folder";
        std::filesystem::create_directories(test_folder_);
        
        // Create test files
        std::ofstream file1(test_folder_ + "/file1.txt");
        file1 << "This is test file 1 content";
        file1.close();
        
        std::ofstream file2(test_folder_ + "/file2.txt");
        file2 << "This is test file 2 content with more data";
        file2.close();
        
        // Create subdirectory with file
        std::filesystem::create_directories(test_folder_ + "/subdir");
        std::ofstream file3(test_folder_ + "/subdir/file3.txt");
        file3 << "This is test file 3 in subdirectory";
        file3.close();
    }
    
    std::string test_dir_;
    std::string test_folder_;
    std::unique_ptr<VaultEncryptionManager> manager_;
    const std::string test_password_ = "test_password_123";
};

TEST_F(VaultEncryptionManagerTest, InitializationSucceeds) {
    // Manager should be initialized in SetUp
    EXPECT_TRUE(manager_ != nullptr);
}

TEST_F(VaultEncryptionManagerTest, PasswordHashing) {
    std::string hashed = manager_->hashPassword(test_password_);
    
    // Should be in format "salt:hash"
    EXPECT_NE(hashed.find(':'), std::string::npos);
    
    // Should be able to verify the password
    auto result = manager_->verifyPassword(test_password_, hashed);
    EXPECT_TRUE(result.is_valid);
    
    // Wrong password should fail
    auto wrong_result = manager_->verifyPassword("wrong_password", hashed);
    EXPECT_FALSE(wrong_result.is_valid);
}

TEST_F(VaultEncryptionManagerTest, PasswordHashingWithSalt) {
    std::string salt = "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef";
    std::string hashed = manager_->hashPassword(test_password_, salt);
    
    // Should start with the provided salt
    EXPECT_TRUE(hashed.starts_with(salt + ":"));
    
    // Should be reproducible with same salt
    std::string hashed2 = manager_->hashPassword(test_password_, salt);
    EXPECT_EQ(hashed, hashed2);
}

TEST_F(VaultEncryptionManagerTest, FolderFileCount) {
    createTestFolder();
    
    size_t count = manager_->getFolderFileCount(test_folder_);
    EXPECT_EQ(count, 3); // file1.txt, file2.txt, subdir/file3.txt
}

TEST_F(VaultEncryptionManagerTest, FolderEncryptionDetection) {
    createTestFolder();
    
    // Initially not encrypted
    EXPECT_FALSE(manager_->isFolderEncrypted(test_folder_));
    
    // After encryption should be detected as encrypted
    auto result = manager_->encryptFolder(test_folder_, test_password_);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(manager_->isFolderEncrypted(test_folder_));
}

TEST_F(VaultEncryptionManagerTest, FolderEncryptionBasic) {
    createTestFolder();
    
    // Encrypt the folder
    auto encrypt_result = manager_->encryptFolder(test_folder_, test_password_);
    
    EXPECT_TRUE(encrypt_result.success);
    EXPECT_EQ(encrypt_result.total_files, 3);
    EXPECT_EQ(encrypt_result.failed_files, 0);
    EXPECT_EQ(encrypt_result.processed_files.size(), 3);
    
    // Original files should be gone, .enc files should exist
    EXPECT_FALSE(std::filesystem::exists(test_folder_ + "/file1.txt"));
    EXPECT_FALSE(std::filesystem::exists(test_folder_ + "/file2.txt"));
    EXPECT_FALSE(std::filesystem::exists(test_folder_ + "/subdir/file3.txt"));
    
    EXPECT_TRUE(std::filesystem::exists(test_folder_ + "/file1.txt.enc"));
    EXPECT_TRUE(std::filesystem::exists(test_folder_ + "/file2.txt.enc"));
    EXPECT_TRUE(std::filesystem::exists(test_folder_ + "/subdir/file3.txt.enc"));
    
    // Metadata file should exist
    EXPECT_TRUE(std::filesystem::exists(test_folder_ + "/.phantom_vault/encryption.meta"));
}

TEST_F(VaultEncryptionManagerTest, FolderDecryptionBasic) {
    createTestFolder();
    
    // Store original content for comparison
    std::ifstream orig_file1(test_folder_ + "/file1.txt");
    std::string orig_content1((std::istreambuf_iterator<char>(orig_file1)),
                             std::istreambuf_iterator<char>());
    orig_file1.close();
    
    // Encrypt the folder
    auto encrypt_result = manager_->encryptFolder(test_folder_, test_password_);
    ASSERT_TRUE(encrypt_result.success);
    
    // Decrypt the folder
    auto decrypt_result = manager_->decryptFolder(test_folder_, test_password_);
    
    EXPECT_TRUE(decrypt_result.success);
    EXPECT_EQ(decrypt_result.total_files, 3);
    EXPECT_EQ(decrypt_result.failed_files, 0);
    EXPECT_EQ(decrypt_result.processed_files.size(), 3);
    
    // Original files should be restored
    EXPECT_TRUE(std::filesystem::exists(test_folder_ + "/file1.txt"));
    EXPECT_TRUE(std::filesystem::exists(test_folder_ + "/file2.txt"));
    EXPECT_TRUE(std::filesystem::exists(test_folder_ + "/subdir/file3.txt"));
    
    // .enc files should be gone
    EXPECT_FALSE(std::filesystem::exists(test_folder_ + "/file1.txt.enc"));
    EXPECT_FALSE(std::filesystem::exists(test_folder_ + "/file2.txt.enc"));
    EXPECT_FALSE(std::filesystem::exists(test_folder_ + "/subdir/file3.txt.enc"));
    
    // Metadata file should be gone
    EXPECT_FALSE(std::filesystem::exists(test_folder_ + "/.phantom_vault/encryption.meta"));
    
    // Content should be preserved
    std::ifstream restored_file1(test_folder_ + "/file1.txt");
    std::string restored_content1((std::istreambuf_iterator<char>(restored_file1)),
                                 std::istreambuf_iterator<char>());
    restored_file1.close();
    
    EXPECT_EQ(orig_content1, restored_content1);
}

TEST_F(VaultEncryptionManagerTest, DecryptionWithWrongPassword) {
    createTestFolder();
    
    // Encrypt the folder
    auto encrypt_result = manager_->encryptFolder(test_folder_, test_password_);
    ASSERT_TRUE(encrypt_result.success);
    
    // Try to decrypt with wrong password
    auto decrypt_result = manager_->decryptFolder(test_folder_, "wrong_password");
    
    EXPECT_FALSE(decrypt_result.success);
    EXPECT_GT(decrypt_result.failed_files, 0);
    
    // Files should still be encrypted (decryption failed)
    EXPECT_TRUE(std::filesystem::exists(test_folder_ + "/file1.txt.enc"));
    // Note: Failed decryption may leave partial files, so we don't check for absence
}

TEST_F(VaultEncryptionManagerTest, EncryptionWithProgressCallback) {
    createTestFolder();
    
    std::vector<std::string> progress_files;
    std::vector<size_t> progress_counts;
    
    auto progress_callback = [&](const std::string& file, size_t processed, size_t total) {
        progress_files.push_back(file);
        progress_counts.push_back(processed);
    };
    
    auto result = manager_->encryptFolder(test_folder_, test_password_, progress_callback);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(progress_files.size(), 3);
    EXPECT_EQ(progress_counts.size(), 3);
    
    // Progress should be sequential
    EXPECT_EQ(progress_counts[0], 0);
    EXPECT_EQ(progress_counts[1], 1);
    EXPECT_EQ(progress_counts[2], 2);
}

TEST_F(VaultEncryptionManagerTest, EmptyFolderEncryption) {
    std::string empty_folder = test_dir_ + "/empty_folder";
    std::filesystem::create_directories(empty_folder);
    
    auto result = manager_->encryptFolder(empty_folder, test_password_);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.total_files, 0);
    EXPECT_EQ(result.failed_files, 0);
}

TEST_F(VaultEncryptionManagerTest, NonExistentFolderEncryption) {
    std::string non_existent = test_dir_ + "/non_existent";
    
    auto result = manager_->encryptFolder(non_existent, test_password_);
    
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(VaultEncryptionManagerTest, KeyDerivation) {
    std::vector<uint8_t> salt = manager_->generateSalt();
    EXPECT_EQ(salt.size(), 32); // Standard salt size
    
    std::vector<uint8_t> key1 = manager_->deriveKey(test_password_, salt);
    std::vector<uint8_t> key2 = manager_->deriveKey(test_password_, salt);
    
    // Same password and salt should produce same key
    EXPECT_EQ(key1, key2);
    
    // Different password should produce different key
    std::vector<uint8_t> key3 = manager_->deriveKey("different_password", salt);
    EXPECT_NE(key1, key3);
}