#include <gtest/gtest.h>
#include "phantom_vault/vault_metadata_manager.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace phantom_vault::service;
using json = nlohmann::json;

class VaultMetadataManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory
        test_dir_ = "/tmp/phantom_vault_test_" + std::to_string(getpid());
        std::filesystem::create_directories(test_dir_);
        
        // Set up test environment
        setenv("HOME", test_dir_.c_str(), 1);
        
        manager_ = std::make_unique<VaultMetadataManager>();
        ASSERT_TRUE(manager_->initialize("testuser"));
    }
    
    void TearDown() override {
        manager_.reset();
        std::filesystem::remove_all(test_dir_);
    }
    
    void createTestProfilesMetadata() {
        json profiles_json = {
            {"activeProfileId", "profile_123"},
            {"lastModified", 1728388800000},
            {"profiles", json::array({
                {
                    {"id", "profile_123"},
                    {"name", "Test Profile"},
                    {"hashedPassword", "salt123:hash456"},
                    {"encryptedRecoveryKey", "encrypted_key_data"},
                    {"createdAt", 1728388800000}
                }
            })}
        };
        
        std::string profiles_path = test_dir_ + "/.phantom_vault_storage/testuser/metadata/profiles.json";
        std::filesystem::create_directories(std::filesystem::path(profiles_path).parent_path());
        
        std::ofstream file(profiles_path);
        file << profiles_json.dump(2);
        file.close();
    }
    
    void createTestFoldersMetadata() {
        json folders_json = {
            {"profileId", "profile_123"},
            {"lastModified", 1728388800000},
            {"folders", json::array({
                {
                    {"id", "vault_123_abc"},
                    {"folderPath", "/home/testuser/Desktop/TestFolder"},
                    {"folderName", "TestFolder"},
                    {"isLocked", true},
                    {"usesMasterPassword", true},
                    {"createdAt", 1728388800000},
                    {"unlockMode", nullptr},
                    {"originalPath", "/home/testuser/Desktop/TestFolder"},
                    {"vaultPath", "/home/testuser/.phantom_vault_storage/testuser/vaults/TestFolder_vault_123"},
                    {"backups", json::array({
                        {
                            {"timestamp", 1728388800000},
                            {"path", "/home/testuser/.phantom_vault_storage/testuser/backups/TestFolder_backup_123"},
                            {"operation", "pre-lock"}
                        }
                    })}
                }
            })}
        };
        
        std::string folders_path = test_dir_ + "/.phantom_vault_storage/testuser/metadata/profile_123/folders_metadata.json";
        std::filesystem::create_directories(std::filesystem::path(folders_path).parent_path());
        
        std::ofstream file(folders_path);
        file << folders_json.dump(2);
        file.close();
    }
    
    std::string test_dir_;
    std::unique_ptr<VaultMetadataManager> manager_;
};

TEST_F(VaultMetadataManagerTest, InitializationCreatesDirectories) {
    std::string vault_path = manager_->getVaultStoragePath();
    
    EXPECT_TRUE(std::filesystem::exists(vault_path));
    EXPECT_TRUE(std::filesystem::exists(vault_path + "/metadata"));
    EXPECT_TRUE(std::filesystem::exists(vault_path + "/vaults"));
    EXPECT_TRUE(std::filesystem::exists(vault_path + "/backups"));
}

TEST_F(VaultMetadataManagerTest, LoadEmptyProfilesMetadata) {
    ProfilesMetadata metadata = manager_->loadProfilesMetadata();
    
    EXPECT_TRUE(metadata.profiles.empty());
    EXPECT_TRUE(metadata.activeProfileId.empty());
    EXPECT_GT(metadata.lastModified, 0);
}

TEST_F(VaultMetadataManagerTest, LoadExistingProfilesMetadata) {
    createTestProfilesMetadata();
    
    ProfilesMetadata metadata = manager_->loadProfilesMetadata();
    
    EXPECT_EQ(metadata.activeProfileId, "profile_123");
    EXPECT_EQ(metadata.profiles.size(), 1);
    
    const auto& profile = metadata.profiles[0];
    EXPECT_EQ(profile.id, "profile_123");
    EXPECT_EQ(profile.name, "Test Profile");
    EXPECT_EQ(profile.hashedPassword, "salt123:hash456");
    EXPECT_EQ(profile.encryptedRecoveryKey, "encrypted_key_data");
}

TEST_F(VaultMetadataManagerTest, GetActiveProfile) {
    createTestProfilesMetadata();
    
    auto profile = manager_->getActiveProfile();
    
    ASSERT_TRUE(profile.has_value());
    EXPECT_EQ(profile->id, "profile_123");
    EXPECT_EQ(profile->name, "Test Profile");
}

TEST_F(VaultMetadataManagerTest, LoadEmptyFoldersMetadata) {
    FoldersMetadata metadata = manager_->loadFoldersMetadata("profile_123");
    
    EXPECT_EQ(metadata.profileId, "profile_123");
    EXPECT_TRUE(metadata.folders.empty());
    EXPECT_GT(metadata.lastModified, 0);
}

TEST_F(VaultMetadataManagerTest, LoadExistingFoldersMetadata) {
    createTestFoldersMetadata();
    
    FoldersMetadata metadata = manager_->loadFoldersMetadata("profile_123");
    
    EXPECT_EQ(metadata.profileId, "profile_123");
    EXPECT_EQ(metadata.folders.size(), 1);
    
    const auto& folder = metadata.folders[0];
    EXPECT_EQ(folder.id, "vault_123_abc");
    EXPECT_EQ(folder.folderName, "TestFolder");
    EXPECT_EQ(folder.originalPath, "/home/testuser/Desktop/TestFolder");
    EXPECT_TRUE(folder.vaultPath.has_value());
    EXPECT_EQ(folder.vaultPath.value(), "/home/testuser/.phantom_vault_storage/testuser/vaults/TestFolder_vault_123");
    EXPECT_TRUE(folder.isLocked);
    EXPECT_EQ(folder.backups.size(), 1);
    EXPECT_EQ(folder.backups[0].operation, "pre-lock");
}

TEST_F(VaultMetadataManagerTest, GetFolderById) {
    createTestFoldersMetadata();
    
    auto folder = manager_->getFolder("profile_123", "vault_123_abc");
    
    ASSERT_TRUE(folder.has_value());
    EXPECT_EQ(folder->id, "vault_123_abc");
    EXPECT_EQ(folder->folderName, "TestFolder");
}

TEST_F(VaultMetadataManagerTest, UpdateFolderState) {
    createTestFoldersMetadata();
    
    // Update folder to unlocked state
    bool success = manager_->updateFolderState("profile_123", "vault_123_abc", 
                                              false, std::nullopt, "temporary");
    
    EXPECT_TRUE(success);
    
    // Verify the update
    auto folder = manager_->getFolder("profile_123", "vault_123_abc");
    ASSERT_TRUE(folder.has_value());
    EXPECT_FALSE(folder->isLocked);
    EXPECT_FALSE(folder->vaultPath.has_value());
    EXPECT_TRUE(folder->unlockMode.has_value());
    EXPECT_EQ(folder->unlockMode.value(), "temporary");
}

TEST_F(VaultMetadataManagerTest, AddBackupEntry) {
    createTestFoldersMetadata();
    
    bool success = manager_->addBackupEntry("profile_123", "vault_123_abc",
                                           "/test/backup/path", "pre-unlock");
    
    EXPECT_TRUE(success);
    
    // Verify the backup was added
    auto folder = manager_->getFolder("profile_123", "vault_123_abc");
    ASSERT_TRUE(folder.has_value());
    EXPECT_EQ(folder->backups.size(), 2);
    EXPECT_EQ(folder->backups[1].path, "/test/backup/path");
    EXPECT_EQ(folder->backups[1].operation, "pre-unlock");
}

TEST_F(VaultMetadataManagerTest, SaveAndLoadRoundTrip) {
    // Create test data
    FoldersMetadata original;
    original.profileId = "test_profile";
    
    FolderMetadata folder;
    folder.id = "test_folder";
    folder.folderName = "TestFolder";
    folder.originalPath = "/test/path";
    folder.isLocked = true;
    folder.vaultPath = "/vault/path";
    
    BackupEntry backup(1728388800000, "/backup/path", "test-operation");
    folder.backups.push_back(backup);
    
    original.folders.push_back(folder);
    
    // Save and reload
    EXPECT_TRUE(manager_->saveFoldersMetadata("test_profile", original));
    FoldersMetadata loaded = manager_->loadFoldersMetadata("test_profile");
    
    // Verify data integrity
    EXPECT_EQ(loaded.profileId, original.profileId);
    EXPECT_EQ(loaded.folders.size(), 1);
    
    const auto& loadedFolder = loaded.folders[0];
    EXPECT_EQ(loadedFolder.id, folder.id);
    EXPECT_EQ(loadedFolder.folderName, folder.folderName);
    EXPECT_EQ(loadedFolder.originalPath, folder.originalPath);
    EXPECT_EQ(loadedFolder.isLocked, folder.isLocked);
    EXPECT_EQ(loadedFolder.vaultPath.value(), folder.vaultPath.value());
    EXPECT_EQ(loadedFolder.backups.size(), 1);
    EXPECT_EQ(loadedFolder.backups[0].path, backup.path);
    EXPECT_EQ(loadedFolder.backups[0].operation, backup.operation);
}