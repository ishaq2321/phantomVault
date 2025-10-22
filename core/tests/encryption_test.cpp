#include <gtest/gtest.h>
#include "phantom_vault/encryption.hpp"
#include <fstream>
#include <string>
#include <filesystem>

using namespace phantom_vault;
namespace fs = std::filesystem;

class EncryptionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory if it doesn't exist
        fs::create_directories("test_files");
        
        // Create a test file
        std::ofstream test_file("test_files/test.txt");
        test_file << "This is test content for encryption testing.";
        test_file.close();
    }

    void TearDown() override {
        // Clean up test files
        fs::remove_all("test_files");
    }

    EncryptionEngine engine;
};

TEST_F(EncryptionTest, Initialization) {
    EXPECT_TRUE(engine.initialize());
}

TEST_F(EncryptionTest, KeyGeneration) {
    auto key = engine.generateKey();
    EXPECT_EQ(key.size(), 32); // 256 bits = 32 bytes
    
    // Keys should be different each time
    auto another_key = engine.generateKey();
    EXPECT_NE(key, another_key);
}

TEST_F(EncryptionTest, IVGeneration) {
    auto iv = engine.generateIV();
    EXPECT_EQ(iv.size(), 12); // 96 bits = 12 bytes (GCM mode)
    
    // IVs should be different each time
    auto another_iv = engine.generateIV();
    EXPECT_NE(iv, another_iv);
}

TEST_F(EncryptionTest, SaltGeneration) {
    auto salt = engine.generateSalt();
    EXPECT_EQ(salt.size(), 32); // 256 bits = 32 bytes
    
    // Salts should be different each time
    auto another_salt = engine.generateSalt();
    EXPECT_NE(salt, another_salt);
}

TEST_F(EncryptionTest, KeyDerivation) {
    std::string password = "test_password";
    auto salt = engine.generateSalt();
    
    // Same password and salt should produce same key
    auto key1 = engine.deriveKeyFromPassword(password, salt);
    auto key2 = engine.deriveKeyFromPassword(password, salt);
    EXPECT_EQ(key1, key2);
    
    // Different passwords should produce different keys
    auto key3 = engine.deriveKeyFromPassword("different_password", salt);
    EXPECT_NE(key1, key3);
    
    // Different salts should produce different keys
    auto different_salt = engine.generateSalt();
    auto key4 = engine.deriveKeyFromPassword(password, different_salt);
    EXPECT_NE(key1, key4);
}

TEST_F(EncryptionTest, FileEncryptionDecryption) {
    // Generate key and IV
    auto key = engine.generateKey();
    auto iv = engine.generateIV();
    
    // Encrypt the test file
    EXPECT_TRUE(engine.encryptFile(
        "test_files/test.txt",
        "test_files/encrypted.bin",
        key,
        iv
    ));
    
    // Verify encrypted file exists and is different from source
    EXPECT_TRUE(fs::exists("test_files/encrypted.bin"));
    EXPECT_NE(fs::file_size("test_files/test.txt"), fs::file_size("test_files/encrypted.bin"));
    
    // Decrypt the file
    EXPECT_TRUE(engine.decryptFile(
        "test_files/encrypted.bin",
        "test_files/decrypted.txt",
        key,
        iv
    ));
    
    // Compare original and decrypted content
    std::ifstream original("test_files/test.txt");
    std::ifstream decrypted("test_files/decrypted.txt");
    std::string original_content((std::istreambuf_iterator<char>(original)),
                                std::istreambuf_iterator<char>());
    std::string decrypted_content((std::istreambuf_iterator<char>(decrypted)),
                                 std::istreambuf_iterator<char>());
    EXPECT_EQ(original_content, decrypted_content);
}

TEST_F(EncryptionTest, EncryptionWithWrongKey) {
    auto key = engine.generateKey();
    auto wrong_key = engine.generateKey();
    auto iv = engine.generateIV();
    
    // Encrypt with correct key
    EXPECT_TRUE(engine.encryptFile(
        "test_files/test.txt",
        "test_files/encrypted.bin",
        key,
        iv
    ));
    
    // Try to decrypt with wrong key
    EXPECT_FALSE(engine.decryptFile(
        "test_files/encrypted.bin",
        "test_files/decrypted.txt",
        wrong_key,
        iv
    ));
}

TEST_F(EncryptionTest, EncryptionWithWrongIV) {
    auto key = engine.generateKey();
    auto iv = engine.generateIV();
    auto wrong_iv = engine.generateIV();
    
    // Encrypt with correct IV
    EXPECT_TRUE(engine.encryptFile(
        "test_files/test.txt",
        "test_files/encrypted.bin",
        key,
        iv
    ));
    
    // Try to decrypt with wrong IV
    EXPECT_FALSE(engine.decryptFile(
        "test_files/encrypted.bin",
        "test_files/decrypted.txt",
        key,
        wrong_iv
    ));
} 