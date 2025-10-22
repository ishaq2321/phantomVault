#include <gtest/gtest.h>
#include "phantom_vault/storage.hpp"
#include "phantom_vault/encryption.hpp"
#include <filesystem>
#include <thread>

using namespace phantom_vault;
using namespace phantom_vault::storage;
using namespace std::chrono_literals;

class StorageTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Generate a test key
        EncryptionEngine engine;
        engine.initialize();
        key_ = engine.generateKey();
        
        storage_.initialize(key_);
    }

    void TearDown() override {
        std::filesystem::remove_all(".phantom_vault");
    }

    std::vector<uint8_t> key_;
    SecureStorage storage_;
};

TEST_F(StorageTest, SaveLoadMetadata) {
    VaultMetadata metadata;
    metadata.vault_id = "test-vault";
    metadata.name = "Test Vault";
    metadata.description = "Test vault description";
    metadata.location = "/path/to/vault";
    metadata.created_time = std::chrono::system_clock::now();
    metadata.modified_time = metadata.created_time;
    metadata.key_verification = {1, 2, 3, 4};
    metadata.salt = {5, 6, 7, 8};
    metadata.iterations = 1000;

    ASSERT_TRUE(storage_.saveVaultMetadata(metadata));

    auto loaded = storage_.loadVaultMetadata("test-vault");
    ASSERT_TRUE(loaded.has_value());

    EXPECT_EQ(loaded->vault_id, metadata.vault_id);
    EXPECT_EQ(loaded->name, metadata.name);
    EXPECT_EQ(loaded->description, metadata.description);
    EXPECT_EQ(loaded->location, metadata.location);
    EXPECT_EQ(loaded->key_verification, metadata.key_verification);
    EXPECT_EQ(loaded->salt, metadata.salt);
    EXPECT_EQ(loaded->iterations, metadata.iterations);
}

TEST_F(StorageTest, SaveLoadConfig) {
    VaultConfig config;
    config.auto_lock = true;
    config.lock_timeout = 300s;
    config.clear_clipboard = true;
    config.clipboard_timeout = 30s;
    config.hide_vault_dir = true;
    config.secure_delete = true;
    config.secure_delete_passes = 3;

    ASSERT_TRUE(storage_.saveVaultConfig("test-vault", config));

    auto loaded = storage_.loadVaultConfig("test-vault");
    ASSERT_TRUE(loaded.has_value());

    EXPECT_EQ(loaded->auto_lock, config.auto_lock);
    EXPECT_EQ(loaded->lock_timeout, config.lock_timeout);
    EXPECT_EQ(loaded->clear_clipboard, config.clear_clipboard);
    EXPECT_EQ(loaded->clipboard_timeout, config.clipboard_timeout);
    EXPECT_EQ(loaded->hide_vault_dir, config.hide_vault_dir);
    EXPECT_EQ(loaded->secure_delete, config.secure_delete);
    EXPECT_EQ(loaded->secure_delete_passes, config.secure_delete_passes);
}

TEST_F(StorageTest, ListVaults) {
    VaultMetadata metadata1;
    metadata1.vault_id = "vault1";
    metadata1.name = "Vault 1";
    
    VaultMetadata metadata2;
    metadata2.vault_id = "vault2";
    metadata2.name = "Vault 2";

    ASSERT_TRUE(storage_.saveVaultMetadata(metadata1));
    ASSERT_TRUE(storage_.saveVaultMetadata(metadata2));

    auto vaults = storage_.listVaults();
    ASSERT_EQ(vaults.size(), 2);
    EXPECT_TRUE(std::find(vaults.begin(), vaults.end(), "vault1") != vaults.end());
    EXPECT_TRUE(std::find(vaults.begin(), vaults.end(), "vault2") != vaults.end());
}

TEST_F(StorageTest, DeleteVault) {
    VaultMetadata metadata;
    metadata.vault_id = "test-vault";
    metadata.name = "Test Vault";

    ASSERT_TRUE(storage_.saveVaultMetadata(metadata));
    ASSERT_TRUE(storage_.deleteVaultMetadata("test-vault"));
    ASSERT_FALSE(storage_.loadVaultMetadata("test-vault").has_value());
}

TEST_F(StorageTest, NonexistentVault) {
    EXPECT_FALSE(storage_.loadVaultMetadata("nonexistent").has_value());
    EXPECT_FALSE(storage_.loadVaultConfig("nonexistent").has_value());
    EXPECT_FALSE(storage_.deleteVaultMetadata("nonexistent"));
}

TEST_F(StorageTest, ErrorHandling) {
    // Try to load without initialization
    SecureStorage uninitialized_storage;
    auto result = uninitialized_storage.loadVaultMetadata("test-vault");
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(uninitialized_storage.getLastError().empty());
}

TEST_F(StorageTest, MetadataEncryption) {
    VaultMetadata metadata;
    metadata.vault_id = "test-vault";
    metadata.name = "Test Vault";
    
    ASSERT_TRUE(storage_.saveVaultMetadata(metadata));

    // Try to load with wrong key
    SecureStorage other_storage;
    EncryptionEngine engine;
    engine.initialize();
    std::vector<uint8_t> wrong_key = engine.generateKey();
    other_storage.initialize(wrong_key);
    
    auto result = other_storage.loadVaultMetadata("test-vault");
    EXPECT_FALSE(result.has_value());
} 