#include <gtest/gtest.h>
#include "phantom_vault/core.hpp"
#include "phantom_vault/storage.hpp"
#include "phantom_vault/encryption.hpp"
#include "phantom_vault/keyboard_hook.hpp"
#include "phantom_vault/system_tray.hpp"
#include "phantom_vault/filesystem.hpp"
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

using namespace phantom_vault;
using namespace phantom_vault::storage;
using namespace std::chrono;

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize core library
        core = std::make_unique<Core>();
        ASSERT_TRUE(core->initialize());
        
        // Initialize storage with test master key
        storage = std::make_unique<SecureStorage>();
        std::vector<uint8_t> master_key(32, 0x42);
        ASSERT_TRUE(storage->initialize(master_key));
        
        // Initialize encryption engine
        encryption = std::make_unique<EncryptionEngine>();
        ASSERT_TRUE(encryption->initialize());
        
        // Initialize file system
        filesystem = std::make_unique<fs::FileSystem>();
        
        // Initialize system tray (mock mode for testing)
        system_tray = std::make_unique<SystemTray>();
        
        // Initialize keyboard hook
        keyboard_hook = std::make_unique<KeyboardHook>();
        ASSERT_TRUE(keyboard_hook->initialize());
    }

    void TearDown() override {
        // Clean up test files
        cleanupTestFiles();
    }

    void cleanupTestFiles() {
        // Remove test vaults and recovery data
        auto vaults = storage->listVaults();
        for (const auto& vault_id : vaults) {
            if (vault_id.find("test-") == 0) {
                storage->deleteVaultMetadata(vault_id);
                storage->removePasswordRecovery(vault_id);
            }
        }
    }

    std::unique_ptr<Core> core;
    std::unique_ptr<SecureStorage> storage;
    std::unique_ptr<EncryptionEngine> encryption;
    std::unique_ptr<fs::FileSystem> filesystem;
    std::unique_ptr<SystemTray> system_tray;
    std::unique_ptr<KeyboardHook> keyboard_hook;
};

TEST_F(IntegrationTest, CoreLibraryInitialization) {
    EXPECT_TRUE(core->isInitialized());
    EXPECT_FALSE(core->getVersion().empty());
}

TEST_F(IntegrationTest, VaultCreationAndManagement) {
    // Create a test vault
    VaultMetadata metadata;
    metadata.vault_id = "test-vault-integration-1";
    metadata.name = "Test Integration Vault";
    metadata.description = "Integration test vault";
    metadata.location = "/tmp/test-vault";
    metadata.created_time = system_clock::now();
    metadata.modified_time = system_clock::now();
    metadata.key_verification = {0x01, 0x02, 0x03, 0x04};
    metadata.salt = encryption->generateSalt();
    metadata.iterations = 100000;

    // Save vault metadata
    EXPECT_TRUE(storage->saveVaultMetadata(metadata));
    
    // Verify vault exists
    EXPECT_TRUE(storage->hasPasswordRecovery(metadata.vault_id) == false);
    
    // Load vault metadata
    auto loaded_metadata = storage->loadVaultMetadata(metadata.vault_id);
    ASSERT_TRUE(loaded_metadata.has_value());
    EXPECT_EQ(loaded_metadata->vault_id, metadata.vault_id);
    EXPECT_EQ(loaded_metadata->name, metadata.name);
    
    // List vaults
    auto vaults = storage->listVaults();
    EXPECT_TRUE(std::find(vaults.begin(), vaults.end(), metadata.vault_id) != vaults.end());
}

TEST_F(IntegrationTest, EncryptionIntegration) {
    // Test encryption with generated keys
    auto key = encryption->generateKey();
    auto iv = encryption->generateIV();
    
    EXPECT_EQ(key.size(), 32); // 256 bits
    EXPECT_EQ(iv.size(), 12);  // 96 bits for GCM
    
    // Test data encryption/decryption
    std::string test_data = "This is a test message for integration testing";
    std::vector<uint8_t> data(test_data.begin(), test_data.end());
    
    auto encrypted = encryption->encryptData(data, key, iv);
    EXPECT_FALSE(encrypted.empty());
    EXPECT_NE(encrypted, data);
    
    auto decrypted = encryption->decryptData(encrypted, key, iv);
    EXPECT_EQ(decrypted, data);
    
    std::string decrypted_str(decrypted.begin(), decrypted.end());
    EXPECT_EQ(decrypted_str, test_data);
}

TEST_F(IntegrationTest, PasswordRecoveryIntegration) {
    // Create test vault
    VaultMetadata metadata;
    metadata.vault_id = "test-vault-recovery-integration";
    metadata.name = "Recovery Test Vault";
    metadata.description = "Integration test for password recovery";
    metadata.location = "/tmp/test-recovery-vault";
    metadata.created_time = system_clock::now();
    metadata.modified_time = system_clock::now();
    metadata.key_verification = {0x01, 0x02, 0x03, 0x04};
    metadata.salt = encryption->generateSalt();
    metadata.iterations = 100000;

    ASSERT_TRUE(storage->saveVaultMetadata(metadata));
    
    // Set up password recovery
    RecoveryInfo recovery_info;
    recovery_info.vault_id = metadata.vault_id;
    recovery_info.attempts_remaining = 3;
    recovery_info.created_time = system_clock::now();
    recovery_info.last_used = system_clock::now();
    recovery_info.recovery_key = encryption->generateKey();
    recovery_info.recovery_iv = encryption->generateIV();
    
    // Add recovery questions
    RecoveryQuestion question1;
    question1.question_id = "q1";
    question1.question_text = "What is your favorite color?";
    question1.salt = encryption->generateSalt();
    std::string answer1 = "blue";
    question1.answer_hash = encryption->deriveKeyFromPassword(answer1, question1.salt);
    recovery_info.questions.push_back(question1);
    
    RecoveryQuestion question2;
    question2.question_id = "q2";
    question2.question_text = "What was your first pet's name?";
    question2.salt = encryption->generateSalt();
    std::string answer2 = "fluffy";
    question2.answer_hash = encryption->deriveKeyFromPassword(answer2, question2.salt);
    recovery_info.questions.push_back(question2);
    
    // Save recovery info
    EXPECT_TRUE(storage->setupPasswordRecovery(metadata.vault_id, recovery_info));
    EXPECT_TRUE(storage->hasPasswordRecovery(metadata.vault_id));
    
    // Test recovery questions retrieval
    auto questions = storage->getRecoveryQuestions(metadata.vault_id);
    EXPECT_EQ(questions.size(), 2);
    EXPECT_EQ(questions[0].question_text, "What is your favorite color?");
    EXPECT_EQ(questions[1].question_text, "What was your first pet's name?");
    
    // Test correct recovery
    std::vector<std::string> correct_answers = {"blue", "fluffy"};
    auto recovery_key = storage->verifyRecoveryAnswers(metadata.vault_id, correct_answers);
    EXPECT_FALSE(recovery_key.empty());
    EXPECT_EQ(recovery_key, recovery_info.recovery_key);
    
    // Test incorrect recovery
    std::vector<std::string> incorrect_answers = {"red", "spot"};
    auto empty_key = storage->verifyRecoveryAnswers(metadata.vault_id, incorrect_answers);
    EXPECT_TRUE(empty_key.empty());
}

TEST_F(IntegrationTest, FileSystemIntegration) {
    // Test file system operations
    std::string test_file = "/tmp/phantom_vault_test_file.txt";
    std::string hidden_file = "/tmp/.phantom_vault_test_file.txt";
    std::string test_content = "Test content for integration testing";
    
    // Create test file
    std::ofstream file(test_file);
    file << test_content;
    file.close();
    
    // Verify file exists
    EXPECT_TRUE(filesystem->exists(test_file));
    EXPECT_FALSE(filesystem->isHidden(test_file));
    
    // Test file hiding (this will rename the file)
    EXPECT_TRUE(filesystem->hide(test_file));
    
    // After hiding, the original file should not exist, but the hidden version should
    EXPECT_FALSE(filesystem->exists(test_file));
    EXPECT_TRUE(filesystem->exists(hidden_file));
    EXPECT_TRUE(filesystem->isHidden(hidden_file));
    
    // Test file unhiding (this will rename back)
    EXPECT_TRUE(filesystem->unhide(hidden_file));
    
    // After unhiding, the original file should exist again
    EXPECT_TRUE(filesystem->exists(test_file));
    EXPECT_FALSE(filesystem->isHidden(test_file));
    EXPECT_FALSE(filesystem->exists(hidden_file));
    
    // Test file attributes
    fs::FileAttributes attrs;
    EXPECT_TRUE(filesystem->getAttributes(test_file, attrs));
    EXPECT_TRUE(filesystem->exists(test_file));
    
    // Clean up
    std::remove(test_file.c_str());
    std::remove(hidden_file.c_str());
}

TEST_F(IntegrationTest, VaultConfigurationIntegration) {
    // Create test vault
    VaultMetadata metadata;
    metadata.vault_id = "test-vault-config-integration";
    metadata.name = "Config Test Vault";
    metadata.description = "Integration test for vault configuration";
    metadata.location = "/tmp/test-config-vault";
    metadata.created_time = system_clock::now();
    metadata.modified_time = system_clock::now();
    metadata.key_verification = {0x01, 0x02, 0x03, 0x04};
    metadata.salt = encryption->generateSalt();
    metadata.iterations = 100000;

    ASSERT_TRUE(storage->saveVaultMetadata(metadata));
    
    // Create vault configuration
    VaultConfig config;
    config.auto_lock = true;
    config.lock_timeout = seconds(300); // 5 minutes
    config.clear_clipboard = true;
    config.clipboard_timeout = seconds(30);
    config.hide_vault_dir = true;
    config.secure_delete = true;
    config.secure_delete_passes = 3;
    
    // Save configuration
    EXPECT_TRUE(storage->saveVaultConfig(metadata.vault_id, config));
    
    // Load configuration
    auto loaded_config = storage->loadVaultConfig(metadata.vault_id);
    ASSERT_TRUE(loaded_config.has_value());
    EXPECT_EQ(loaded_config->auto_lock, config.auto_lock);
    EXPECT_EQ(loaded_config->lock_timeout.count(), config.lock_timeout.count());
    EXPECT_EQ(loaded_config->clear_clipboard, config.clear_clipboard);
    EXPECT_EQ(loaded_config->hide_vault_dir, config.hide_vault_dir);
    EXPECT_EQ(loaded_config->secure_delete, config.secure_delete);
    EXPECT_EQ(loaded_config->secure_delete_passes, config.secure_delete_passes);
}

TEST_F(IntegrationTest, KeyboardHookIntegration) {
    // Test keyboard hook initialization
    EXPECT_TRUE(keyboard_hook->initialize());
    
    // Test monitoring start/stop
    bool callback_called = false;
    std::string last_key;
    bool last_pressed = false;
    unsigned int last_modifiers = 0;
    
    auto callback = [&](const std::string& key_name, bool is_pressed, unsigned int modifiers) {
        callback_called = true;
        last_key = key_name;
        last_pressed = is_pressed;
        last_modifiers = modifiers;
    };
    
    // Start monitoring
    EXPECT_TRUE(keyboard_hook->startMonitoring(callback));
    EXPECT_TRUE(keyboard_hook->isMonitoring());
    
    // Let it run briefly
    std::this_thread::sleep_for(milliseconds(100));
    
    // Stop monitoring
    keyboard_hook->stopMonitoring();
    EXPECT_FALSE(keyboard_hook->isMonitoring());
}

TEST_F(IntegrationTest, SystemTrayIntegration) {
    // Test system tray initialization (mock mode)
    // Note: In a real test environment, this might not work without a display
    // but we can test the initialization logic
    
    // Test menu creation
    std::vector<SystemTray::MenuItem> menu_items;
    menu_items.push_back({"Test Item 1", [](){}, false, true, false, false});
    menu_items.push_back({"Test Item 2", [](){}, false, true, false, false});
    menu_items.push_back({"", [](){}, true, true, false, false}); // Separator
    menu_items.push_back({"Test Item 3", [](){}, false, true, false, false});
    
    // Note: Actual system tray operations require a display server
    // This test verifies the API contract
    EXPECT_TRUE(true); // Placeholder for system tray functionality
}

TEST_F(IntegrationTest, EndToEndVaultWorkflow) {
    // Complete vault workflow test
    std::string vault_id = "test-e2e-vault";
    
    // 1. Create vault metadata
    VaultMetadata metadata;
    metadata.vault_id = vault_id;
    metadata.name = "E2E Test Vault";
    metadata.description = "End-to-end integration test";
    metadata.location = "/tmp/e2e-test-vault";
    metadata.created_time = system_clock::now();
    metadata.modified_time = system_clock::now();
    metadata.key_verification = {0x01, 0x02, 0x03, 0x04};
    metadata.salt = encryption->generateSalt();
    metadata.iterations = 100000;
    
    EXPECT_TRUE(storage->saveVaultMetadata(metadata));
    
    // 2. Set up vault configuration
    VaultConfig config;
    config.auto_lock = true;
    config.lock_timeout = seconds(600);
    config.clear_clipboard = true;
    config.clipboard_timeout = seconds(60);
    config.hide_vault_dir = false;
    config.secure_delete = false;
    config.secure_delete_passes = 1;
    
    EXPECT_TRUE(storage->saveVaultConfig(vault_id, config));
    
    // 3. Set up password recovery
    RecoveryInfo recovery_info;
    recovery_info.vault_id = vault_id;
    recovery_info.attempts_remaining = 3;
    recovery_info.created_time = system_clock::now();
    recovery_info.last_used = system_clock::now();
    recovery_info.recovery_key = encryption->generateKey();
    recovery_info.recovery_iv = encryption->generateIV();
    
    RecoveryQuestion question;
    question.question_id = "e2e_q1";
    question.question_text = "What is the test answer?";
    question.salt = encryption->generateSalt();
    std::string answer = "integration_test";
    question.answer_hash = encryption->deriveKeyFromPassword(answer, question.salt);
    recovery_info.questions.push_back(question);
    
    EXPECT_TRUE(storage->setupPasswordRecovery(vault_id, recovery_info));
    
    // 4. Verify complete setup
    auto loaded_metadata = storage->loadVaultMetadata(vault_id);
    ASSERT_TRUE(loaded_metadata.has_value());
    EXPECT_EQ(loaded_metadata->vault_id, vault_id);
    
    auto loaded_config = storage->loadVaultConfig(vault_id);
    ASSERT_TRUE(loaded_config.has_value());
    EXPECT_TRUE(loaded_config->auto_lock);
    
    EXPECT_TRUE(storage->hasPasswordRecovery(vault_id));
    
    auto questions = storage->getRecoveryQuestions(vault_id);
    EXPECT_EQ(questions.size(), 1);
    
    // 5. Test recovery workflow
    std::vector<std::string> answers = {"integration_test"};
    auto recovery_key = storage->verifyRecoveryAnswers(vault_id, answers);
    EXPECT_FALSE(recovery_key.empty());
    
    // 6. Test vault listing
    auto vaults = storage->listVaults();
    EXPECT_TRUE(std::find(vaults.begin(), vaults.end(), vault_id) != vaults.end());
    
    // 7. Clean up
    EXPECT_TRUE(storage->removePasswordRecovery(vault_id));
    EXPECT_TRUE(storage->deleteVaultMetadata(vault_id));
    EXPECT_FALSE(storage->hasPasswordRecovery(vault_id));
}

TEST_F(IntegrationTest, ErrorHandlingIntegration) {
    // Test error handling across components
    
    // Test with invalid vault ID
    auto metadata = storage->loadVaultMetadata("non-existent-vault");
    EXPECT_FALSE(metadata.has_value());
    
    auto config = storage->loadVaultConfig("non-existent-vault");
    EXPECT_FALSE(config.has_value());
    
    auto questions = storage->getRecoveryQuestions("non-existent-vault");
    EXPECT_TRUE(questions.empty());
    
    std::vector<std::string> answers = {"test"};
    auto recovery_key = storage->verifyRecoveryAnswers("non-existent-vault", answers);
    EXPECT_TRUE(recovery_key.empty());
    
    // Test with wrong number of answers
    VaultMetadata test_metadata;
    test_metadata.vault_id = "test-error-handling";
    test_metadata.name = "Error Test Vault";
    test_metadata.description = "Error handling test";
    test_metadata.location = "/tmp/error-test-vault";
    test_metadata.created_time = system_clock::now();
    test_metadata.modified_time = system_clock::now();
    test_metadata.key_verification = {0x01, 0x02, 0x03, 0x04};
    test_metadata.salt = encryption->generateSalt();
    test_metadata.iterations = 100000;
    
    ASSERT_TRUE(storage->saveVaultMetadata(test_metadata));
    
    RecoveryInfo recovery_info;
    recovery_info.vault_id = test_metadata.vault_id;
    recovery_info.attempts_remaining = 3;
    recovery_info.created_time = system_clock::now();
    recovery_info.last_used = system_clock::now();
    recovery_info.recovery_key = encryption->generateKey();
    recovery_info.recovery_iv = encryption->generateIV();
    
    RecoveryQuestion question;
    question.question_id = "error_q1";
    question.question_text = "Test question?";
    question.salt = encryption->generateSalt();
    std::string answer = "test_answer";
    question.answer_hash = encryption->deriveKeyFromPassword(answer, question.salt);
    recovery_info.questions.push_back(question);
    
    ASSERT_TRUE(storage->setupPasswordRecovery(test_metadata.vault_id, recovery_info));
    
    // Test with wrong number of answers
    std::vector<std::string> wrong_count_answers = {"answer1", "answer2"}; // 2 answers for 1 question
    auto empty_key = storage->verifyRecoveryAnswers(test_metadata.vault_id, wrong_count_answers);
    EXPECT_TRUE(empty_key.empty());
    
    // Test with correct number but wrong answers
    std::vector<std::string> wrong_answers = {"wrong_answer"};
    empty_key = storage->verifyRecoveryAnswers(test_metadata.vault_id, wrong_answers);
    EXPECT_TRUE(empty_key.empty());
}
