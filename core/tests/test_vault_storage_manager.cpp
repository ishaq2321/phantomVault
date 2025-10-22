#include <gtest/gtest.h>
#include "phantom_vault/vault_storage_manager.hpp"
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <thread>
#include <chrono>

using namespace phantom_vault::service;
namespace fs = std::filesystem;

class VaultStorageManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for testing
        char temp_template[] = "/tmp/phantom_vault_storage_test_XXXXXX";
        temp_dir_ = mkdtemp(temp_template);
        ASSERT_FALSE(temp_dir_.empty());
        
        // Set HOME to our temp directory for testing
        original_home_ = getenv("HOME");
        setenv("HOME", temp_dir_.c_str(), 1);
        
        // Initialize storage manager
        storage_manager_ = std::make_unique<VaultStorageManager>();
        ASSERT_TRUE(storage_manager_->initialize("testuser"));
        
        // Create test folders
        createTestFolder();
    }

    void TearDown() override {
        storage_manager_.reset();
        
        // Restore original HOME
        if (!original_home_.empty()) {
            setenv("HOME", original_home_.c_str(), 1);
        }
        
        // Clean up temp directory
        if (!temp_dir_.empty() && fs::exists(temp_dir_)) {
            fs::remove_all(temp_dir_);
        }
    }

    void createTestFolder() {
        test_folder_path_ = fs::path(temp_dir_) / "test_folder";
        fs::create_directories(test_folder_path_);
        
        // Create some test files
        std::ofstream file1(test_folder_path_ / "file1.txt");
        file1 << "Test content 1";
        file1.close();
        
        std::ofstream file2(test_folder_path_ / "file2.txt");
        file2 << "Test content 2 with more data";
        file2.close();
        
        // Create subdirectory with file
        fs::create_directories(test_folder_path_ / "subdir");
        std::ofstream file3(test_folder_path_ / "subdir" / "file3.txt");
        file3 << "Test content 3 in subdirectory";
        file3.close();
    }

    std::string temp_dir_;
    std::string original_home_;
    fs::path test_folder_path_;
    std::unique_ptr<VaultStorageManager> storage_manager_;
};

TEST_F(VaultStorageManagerTest, InitializationCreatesDirectories) {
    // Verify that initialization created the required directories
    auto vault_base = storage_manager_->getVaultBasePath();
    auto user_vault = storage_manager_->getUserVaultPath();
    auto backup_path = storage_manager_->getBackupPath();
    
    EXPECT_TRUE(fs::exists(vault_base));
    EXPECT_TRUE(fs::exists(user_vault));
    EXPECT_TRUE(fs::exists(backup_path));
    EXPECT_TRUE(fs::exists(user_vault / "vaults"));
    EXPECT_TRUE(fs::exists(user_vault / "metadata"));
}

TEST_F(VaultStorageManagerTest, PathGeneration) {
    auto vault_path = storage_manager_->generateVaultPath("TestFolder", "12345");
    auto backup_path = storage_manager_->generateBackupPath("TestFolder", "pre-lock");
    
    EXPECT_TRUE(vault_path.string().find("TestFolder_vault_12345") != std::string::npos);
    EXPECT_TRUE(backup_path.string().find("TestFolder_backup_pre-lock") != std::string::npos);
}

TEST_F(VaultStorageManagerTest, FolderSizeCalculation) {
    size_t folder_size = storage_manager_->getFolderSize(test_folder_path_);
    EXPECT_GT(folder_size, 0);
    
    // Should be sum of all file contents
    size_t expected_size = 13 + 27 + 32; // Approximate content sizes
    EXPECT_GE(folder_size, expected_size);
}

TEST_F(VaultStorageManagerTest, FolderIntegrityCheck) {
    EXPECT_TRUE(storage_manager_->verifyFolderIntegrity(test_folder_path_));
    
    // Test with non-existent folder
    EXPECT_FALSE(storage_manager_->verifyFolderIntegrity("/non/existent/path"));
}

TEST_F(VaultStorageManagerTest, BackupCreation) {
    auto backup_path = storage_manager_->generateBackupPath("TestFolder", "pre-lock");
    
    auto result = storage_manager_->createBackup(test_folder_path_, backup_path, "pre-lock");
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(fs::exists(backup_path));
    EXPECT_TRUE(fs::exists(backup_path / "file1.txt"));
    EXPECT_TRUE(fs::exists(backup_path / "file2.txt"));
    EXPECT_TRUE(fs::exists(backup_path / "subdir" / "file3.txt"));
}

TEST_F(VaultStorageManagerTest, BackupRestoration) {
    // Create backup first
    auto backup_path = storage_manager_->generateBackupPath("TestFolder", "pre-lock");
    auto result = storage_manager_->createBackup(test_folder_path_, backup_path, "pre-lock");
    ASSERT_TRUE(result.success);
    
    // Remove original folder
    fs::remove_all(test_folder_path_);
    EXPECT_FALSE(fs::exists(test_folder_path_));
    
    // Restore from backup
    result = storage_manager_->restoreFromBackup(backup_path, test_folder_path_);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(fs::exists(test_folder_path_));
    EXPECT_TRUE(fs::exists(test_folder_path_ / "file1.txt"));
    EXPECT_TRUE(fs::exists(test_folder_path_ / "file2.txt"));
    EXPECT_TRUE(fs::exists(test_folder_path_ / "subdir" / "file3.txt"));
}

TEST_F(VaultStorageManagerTest, MoveToVault) {
    auto vault_path = storage_manager_->generateVaultPath("TestFolder", "12345");
    
    auto result = storage_manager_->moveToVault(test_folder_path_, vault_path);
    
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(fs::exists(test_folder_path_)); // Original should be gone
    EXPECT_TRUE(fs::exists(vault_path));         // Should exist in vault
    EXPECT_TRUE(fs::exists(vault_path / "file1.txt"));
    EXPECT_TRUE(fs::exists(vault_path / "file2.txt"));
    EXPECT_TRUE(fs::exists(vault_path / "subdir" / "file3.txt"));
}

TEST_F(VaultStorageManagerTest, MoveFromVault) {
    // First move to vault
    auto vault_path = storage_manager_->generateVaultPath("TestFolder", "12345");
    auto result = storage_manager_->moveToVault(test_folder_path_, vault_path);
    ASSERT_TRUE(result.success);
    
    // Then move back from vault
    result = storage_manager_->moveFromVault(vault_path, test_folder_path_);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(fs::exists(test_folder_path_));   // Should be restored
    EXPECT_FALSE(fs::exists(vault_path));         // Vault copy should be gone
    EXPECT_TRUE(fs::exists(test_folder_path_ / "file1.txt"));
    EXPECT_TRUE(fs::exists(test_folder_path_ / "file2.txt"));
    EXPECT_TRUE(fs::exists(test_folder_path_ / "subdir" / "file3.txt"));
}

TEST_F(VaultStorageManagerTest, TransactionSupport) {
    EXPECT_FALSE(storage_manager_->isInTransaction());
    
    // Begin transaction
    EXPECT_TRUE(storage_manager_->beginTransaction());
    EXPECT_TRUE(storage_manager_->isInTransaction());
    
    // Perform operations within transaction
    auto vault_path = storage_manager_->generateVaultPath("TestFolder", "12345");
    auto result = storage_manager_->moveToVault(test_folder_path_, vault_path);
    EXPECT_TRUE(result.success);
    
    // Check transaction operations
    auto operations = storage_manager_->getTransactionOperations();
    EXPECT_EQ(operations.size(), 1);
    EXPECT_EQ(operations[0].type, StorageOperationType::MOVE_TO_VAULT);
    
    // Commit transaction
    EXPECT_TRUE(storage_manager_->commitTransaction());
    EXPECT_FALSE(storage_manager_->isInTransaction());
}

TEST_F(VaultStorageManagerTest, TransactionRollback) {
    // Begin transaction
    EXPECT_TRUE(storage_manager_->beginTransaction());
    
    // Perform operation
    auto vault_path = storage_manager_->generateVaultPath("TestFolder", "12345");
    auto result = storage_manager_->moveToVault(test_folder_path_, vault_path);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(fs::exists(vault_path));
    EXPECT_FALSE(fs::exists(test_folder_path_));
    
    // Rollback transaction
    EXPECT_TRUE(storage_manager_->rollbackTransaction());
    EXPECT_FALSE(storage_manager_->isInTransaction());
    
    // Verify rollback restored original state
    EXPECT_TRUE(fs::exists(test_folder_path_));
    EXPECT_FALSE(fs::exists(vault_path));
}

TEST_F(VaultStorageManagerTest, BackupCleanup) {
    // Create multiple backups
    for (int i = 0; i < 5; ++i) {
        auto backup_path = storage_manager_->generateBackupPath("TestFolder", "test-" + std::to_string(i));
        auto result = storage_manager_->createBackup(test_folder_path_, backup_path, "test");
        EXPECT_TRUE(result.success);
        
        // Add small delay to ensure different timestamps
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Clean old backups, keeping only 2
    int cleaned = storage_manager_->cleanOldBackups("TestFolder", 2);
    EXPECT_EQ(cleaned, 3); // Should have removed 3 old backups
}

TEST_F(VaultStorageManagerTest, ErrorHandling) {
    // Test moving non-existent folder
    auto vault_path = storage_manager_->generateVaultPath("NonExistent", "12345");
    auto result = storage_manager_->moveToVault("/non/existent/path", vault_path);
    
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
    
    // Test moving to existing destination
    fs::create_directories(vault_path);
    result = storage_manager_->moveToVault(test_folder_path_, vault_path);
    
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(VaultStorageManagerTest, ProgressCallback) {
    bool callback_called = false;
    size_t total_bytes_reported = 0;
    
    auto progress_callback = [&](const std::string& current_path, size_t processed_bytes, size_t total_bytes) {
        callback_called = true;
        total_bytes_reported = total_bytes;
    };
    
    auto vault_path = storage_manager_->generateVaultPath("TestFolder", "12345");
    auto result = storage_manager_->moveToVault(test_folder_path_, vault_path, progress_callback);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(callback_called);
    EXPECT_GT(total_bytes_reported, 0);
}