/**
 * Unit Tests for Encryption Engine
 * 
 * Tests for AES-256-CBC encryption, PBKDF2 key derivation,
 * and cryptographic security compliance.
 */

#include "test_framework.hpp"
#include "../include/encryption_engine.hpp"
#include <filesystem>
#include <fstream>
#include <random>

using namespace phantomvault;
using namespace phantomvault::testing;

namespace fs = std::filesystem;

class EncryptionEngineTests {
public:
    static void registerTests(TestFramework& framework) {
        // Basic functionality tests
        REGISTER_TEST(framework, "EncryptionEngine", "initialization", testInitialization);
        REGISTER_TEST(framework, "EncryptionEngine", "self_test", testSelfTest);
        REGISTER_TEST(framework, "EncryptionEngine", "key_derivation", testKeyDerivation);
        
        // Encryption/Decryption tests
        REGISTER_TEST(framework, "EncryptionEngine", "basic_encryption", testBasicEncryption);
        REGISTER_TEST(framework, "EncryptionEngine", "large_data_encryption", testLargeDataEncryption);
        REGISTER_TEST(framework, "EncryptionEngine", "file_encryption", testFileEncryption);
        REGISTER_TEST(framework, "EncryptionEngine", "chunked_processing", testChunkedProcessing);
        
        // Security tests
        REGISTER_TEST(framework, "EncryptionEngine", "iv_uniqueness", testIVUniqueness);
        REGISTER_TEST(framework, "EncryptionEngine", "salt_uniqueness", testSaltUniqueness);
        REGISTER_TEST(framework, "EncryptionEngine", "key_derivation_consistency", testKeyDerivationConsistency);
        REGISTER_TEST(framework, "EncryptionEngine", "encryption_determinism", testEncryptionDeterminism);
        
        // Error handling tests
        REGISTER_TEST(framework, "EncryptionEngine", "invalid_key_handling", testInvalidKeyHandling);
        REGISTER_TEST(framework, "EncryptionEngine", "corrupted_data_handling", testCorruptedDataHandling);
        REGISTER_TEST(framework, "EncryptionEngine", "empty_data_handling", testEmptyDataHandling);
        
        // Performance tests
        REGISTER_TEST(framework, "EncryptionEngine", "encryption_performance", testEncryptionPerformance);
        REGISTER_TEST(framework, "EncryptionEngine", "key_derivation_performance", testKeyDerivationPerformance);
    }

private:
    static void testInitialization() {
        EncryptionEngine engine;
        ASSERT_TRUE(engine.selfTest());
    }
    
    static void testSelfTest() {
        EncryptionEngine engine;
        
        // Self test should pass multiple times
        ASSERT_TRUE(engine.selfTest());
        ASSERT_TRUE(engine.selfTest());
        ASSERT_TRUE(engine.selfTest());
    }
    
    static void testKeyDerivation() {
        EncryptionEngine engine;
        
        std::string password = "test_password_123";
        std::vector<uint8_t> salt = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        
        auto key1 = engine.deriveKey(password, salt, 100000);
        auto key2 = engine.deriveKey(password, salt, 100000);
        
        // Same password and salt should produce same key
        ASSERT_EQ(key1.size(), 32); // AES-256 key size
        ASSERT_EQ(key1, key2);
        
        // Different salt should produce different key
        std::vector<uint8_t> different_salt = {16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
        auto key3 = engine.deriveKey(password, different_salt, 100000);
        ASSERT_NE(key1, key3);
        
        // Different password should produce different key
        auto key4 = engine.deriveKey("different_password", salt, 100000);
        ASSERT_NE(key1, key4);
    }
    
    static void testBasicEncryption() {
        EncryptionEngine engine;
        
        std::string plaintext = "Hello, PhantomVault! This is a test message for encryption.";
        std::string password = "secure_password_123";
        
        // Encrypt
        auto encrypted_result = engine.encryptData(
            std::vector<uint8_t>(plaintext.begin(), plaintext.end()), 
            password
        );
        
        ASSERT_TRUE(encrypted_result.success);
        ASSERT_FALSE(encrypted_result.encrypted_data.empty());
        ASSERT_FALSE(encrypted_result.salt.empty());
        ASSERT_FALSE(encrypted_result.iv.empty());
        
        // Decrypt
        auto decrypted_result = engine.decryptData(
            encrypted_result.encrypted_data,
            password,
            encrypted_result.salt,
            encrypted_result.iv
        );
        
        ASSERT_TRUE(decrypted_result.success);
        
        std::string decrypted_text(decrypted_result.decrypted_data.begin(), 
                                 decrypted_result.decrypted_data.end());
        ASSERT_EQ(plaintext, decrypted_text);
    }
    
    static void testLargeDataEncryption() {
        EncryptionEngine engine;
        
        // Generate 1MB of test data
        std::vector<uint8_t> large_data(1024 * 1024);
        std::iota(large_data.begin(), large_data.end(), 0);
        
        std::string password = "large_data_password";
        
        PerformanceTimer timer;
        
        // Encrypt large data
        auto encrypted_result = engine.encryptData(large_data, password);
        auto encrypt_time = timer.elapsed();
        
        ASSERT_TRUE(encrypted_result.success);
        ASSERT_FALSE(encrypted_result.encrypted_data.empty());
        
        timer.reset();
        
        // Decrypt large data
        auto decrypted_result = engine.decryptData(
            encrypted_result.encrypted_data,
            password,
            encrypted_result.salt,
            encrypted_result.iv
        );
        auto decrypt_time = timer.elapsed();
        
        ASSERT_TRUE(decrypted_result.success);
        ASSERT_EQ(large_data, decrypted_result.decrypted_data);
        
        // Performance check - should complete within reasonable time
        ASSERT_TRUE(encrypt_time.count() < 5000); // Less than 5 seconds
        ASSERT_TRUE(decrypt_time.count() < 5000); // Less than 5 seconds
    }
    
    static void testFileEncryption() {
        EncryptionEngine engine;
        
        // Create test file
        std::string test_file = "test_encryption_file.txt";
        std::string test_content = "This is test file content for encryption testing.\n";
        test_content += "It contains multiple lines and various characters: !@#$%^&*()_+\n";
        test_content += "Testing file encryption and decryption functionality.\n";
        
        {
            std::ofstream file(test_file);
            file << test_content;
        }
        
        std::string password = "file_encryption_password";
        std::string encrypted_file = test_file + ".encrypted";
        std::string decrypted_file = test_file + ".decrypted";
        
        // Encrypt file
        auto encrypt_result = engine.encryptFile(test_file, encrypted_file, password);
        ASSERT_TRUE(encrypt_result.success);
        ASSERT_TRUE(fs::exists(encrypted_file));
        
        // Decrypt file
        auto decrypt_result = engine.decryptFile(encrypted_file, decrypted_file, password);
        ASSERT_TRUE(decrypt_result.success);
        ASSERT_TRUE(fs::exists(decrypted_file));
        
        // Verify content
        std::ifstream decrypted_stream(decrypted_file);
        std::string decrypted_content((std::istreambuf_iterator<char>(decrypted_stream)),
                                    std::istreambuf_iterator<char>());
        
        ASSERT_EQ(test_content, decrypted_content);
        
        // Cleanup
        fs::remove(test_file);
        fs::remove(encrypted_file);
        fs::remove(decrypted_file);
    }
    
    static void testChunkedProcessing() {
        EncryptionEngine engine;
        
        // Create large test data (5MB)
        std::vector<uint8_t> large_data(5 * 1024 * 1024);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        for (auto& byte : large_data) {
            byte = static_cast<uint8_t>(dis(gen));
        }
        
        std::string password = "chunked_processing_password";
        
        // Test chunked encryption
        auto encrypted_result = engine.encryptData(large_data, password);
        ASSERT_TRUE(encrypted_result.success);
        
        // Test chunked decryption
        auto decrypted_result = engine.decryptData(
            encrypted_result.encrypted_data,
            password,
            encrypted_result.salt,
            encrypted_result.iv
        );
        
        ASSERT_TRUE(decrypted_result.success);
        ASSERT_EQ(large_data, decrypted_result.decrypted_data);
    }
    
    static void testIVUniqueness() {
        EncryptionEngine engine;
        
        std::string plaintext = "Test message for IV uniqueness";
        std::string password = "test_password";
        
        std::set<std::vector<uint8_t>> ivs;
        
        // Generate multiple encryptions and check IV uniqueness
        for (int i = 0; i < 100; ++i) {
            auto result = engine.encryptData(
                std::vector<uint8_t>(plaintext.begin(), plaintext.end()),
                password
            );
            
            ASSERT_TRUE(result.success);
            ASSERT_EQ(result.iv.size(), 16); // AES block size
            
            // IV should be unique
            ASSERT_TRUE(ivs.find(result.iv) == ivs.end());
            ivs.insert(result.iv);
        }
        
        ASSERT_EQ(ivs.size(), 100);
    }
    
    static void testSaltUniqueness() {
        EncryptionEngine engine;
        
        std::string password = "test_password";
        std::set<std::vector<uint8_t>> salts;
        
        // Generate multiple key derivations and check salt uniqueness
        for (int i = 0; i < 100; ++i) {
            auto salt = engine.generateSalt();
            
            ASSERT_EQ(salt.size(), 16); // Standard salt size
            
            // Salt should be unique
            ASSERT_TRUE(salts.find(salt) == salts.end());
            salts.insert(salt);
            
            // Salt should have proper entropy
            ASSERT_TRUE(SecurityTestUtils::hasProperEntropy(salt));
        }
        
        ASSERT_EQ(salts.size(), 100);
    }
    
    static void testKeyDerivationConsistency() {
        EncryptionEngine engine;
        
        std::string password = "consistency_test_password";
        std::vector<uint8_t> salt = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        uint32_t iterations = 100000;
        
        // Derive key multiple times with same parameters
        auto key1 = engine.deriveKey(password, salt, iterations);
        auto key2 = engine.deriveKey(password, salt, iterations);
        auto key3 = engine.deriveKey(password, salt, iterations);
        
        // All keys should be identical
        ASSERT_EQ(key1, key2);
        ASSERT_EQ(key2, key3);
        ASSERT_EQ(key1.size(), 32); // AES-256 key size
        
        // Key should have proper entropy
        ASSERT_TRUE(SecurityTestUtils::hasProperEntropy(key1));
    }
    
    static void testEncryptionDeterminism() {
        EncryptionEngine engine;
        
        std::string plaintext = "Determinism test message";
        std::string password = "determinism_password";
        
        // Same plaintext should produce different ciphertext (due to random IV)
        auto result1 = engine.encryptData(
            std::vector<uint8_t>(plaintext.begin(), plaintext.end()),
            password
        );
        auto result2 = engine.encryptData(
            std::vector<uint8_t>(plaintext.begin(), plaintext.end()),
            password
        );
        
        ASSERT_TRUE(result1.success);
        ASSERT_TRUE(result2.success);
        
        // Encrypted data should be different (due to different IVs)
        ASSERT_NE(result1.encrypted_data, result2.encrypted_data);
        ASSERT_NE(result1.iv, result2.iv);
        
        // But both should decrypt to same plaintext
        auto decrypt1 = engine.decryptData(result1.encrypted_data, password, result1.salt, result1.iv);
        auto decrypt2 = engine.decryptData(result2.encrypted_data, password, result2.salt, result2.iv);
        
        ASSERT_TRUE(decrypt1.success);
        ASSERT_TRUE(decrypt2.success);
        ASSERT_EQ(decrypt1.decrypted_data, decrypt2.decrypted_data);
    }
    
    static void testInvalidKeyHandling() {
        EncryptionEngine engine;
        
        std::string plaintext = "Test message";
        std::string correct_password = "correct_password";
        std::string wrong_password = "wrong_password";
        
        // Encrypt with correct password
        auto encrypted_result = engine.encryptData(
            std::vector<uint8_t>(plaintext.begin(), plaintext.end()),
            correct_password
        );
        
        ASSERT_TRUE(encrypted_result.success);
        
        // Try to decrypt with wrong password
        auto decrypt_result = engine.decryptData(
            encrypted_result.encrypted_data,
            wrong_password,
            encrypted_result.salt,
            encrypted_result.iv
        );
        
        // Should fail gracefully
        ASSERT_FALSE(decrypt_result.success);
        ASSERT_FALSE(decrypt_result.error_message.empty());
    }
    
    static void testCorruptedDataHandling() {
        EncryptionEngine engine;
        
        std::string plaintext = "Test message for corruption test";
        std::string password = "corruption_test_password";
        
        // Encrypt data
        auto encrypted_result = engine.encryptData(
            std::vector<uint8_t>(plaintext.begin(), plaintext.end()),
            password
        );
        
        ASSERT_TRUE(encrypted_result.success);
        
        // Corrupt the encrypted data
        auto corrupted_data = encrypted_result.encrypted_data;
        if (!corrupted_data.empty()) {
            corrupted_data[corrupted_data.size() / 2] ^= 0xFF; // Flip bits
        }
        
        // Try to decrypt corrupted data
        auto decrypt_result = engine.decryptData(
            corrupted_data,
            password,
            encrypted_result.salt,
            encrypted_result.iv
        );
        
        // Should fail gracefully
        ASSERT_FALSE(decrypt_result.success);
        ASSERT_FALSE(decrypt_result.error_message.empty());
    }
    
    static void testEmptyDataHandling() {
        EncryptionEngine engine;
        
        std::vector<uint8_t> empty_data;
        std::string password = "empty_data_password";
        
        // Encrypt empty data
        auto encrypted_result = engine.encryptData(empty_data, password);
        
        // Should handle empty data gracefully
        ASSERT_TRUE(encrypted_result.success);
        
        // Decrypt empty data
        auto decrypted_result = engine.decryptData(
            encrypted_result.encrypted_data,
            password,
            encrypted_result.salt,
            encrypted_result.iv
        );
        
        ASSERT_TRUE(decrypted_result.success);
        ASSERT_EQ(decrypted_result.decrypted_data, empty_data);
    }
    
    static void testEncryptionPerformance() {
        EncryptionEngine engine;
        
        // Test different data sizes
        std::vector<size_t> test_sizes = {1024, 10240, 102400, 1048576}; // 1KB, 10KB, 100KB, 1MB
        
        for (size_t size : test_sizes) {
            std::vector<uint8_t> test_data(size, 0x42);
            std::string password = "performance_test_password";
            
            PerformanceTimer timer;
            
            auto encrypted_result = engine.encryptData(test_data, password);
            auto encrypt_time = timer.elapsed();
            
            ASSERT_TRUE(encrypted_result.success);
            
            timer.reset();
            
            auto decrypted_result = engine.decryptData(
                encrypted_result.encrypted_data,
                password,
                encrypted_result.salt,
                encrypted_result.iv
            );
            auto decrypt_time = timer.elapsed();
            
            ASSERT_TRUE(decrypted_result.success);
            
            // Performance benchmarks (adjust based on requirements)
            double encrypt_mbps = (double)size / (1024 * 1024) / (encrypt_time.count() / 1000.0);
            double decrypt_mbps = (double)size / (1024 * 1024) / (decrypt_time.count() / 1000.0);
            
            // Should achieve reasonable throughput (at least 10 MB/s)
            ASSERT_TRUE(encrypt_mbps > 10.0);
            ASSERT_TRUE(decrypt_mbps > 10.0);
        }
    }
    
    static void testKeyDerivationPerformance() {
        EncryptionEngine engine;
        
        std::string password = "performance_test_password";
        std::vector<uint8_t> salt = engine.generateSalt();
        
        // Test different iteration counts
        std::vector<uint32_t> iteration_counts = {10000, 50000, 100000, 200000};
        
        for (uint32_t iterations : iteration_counts) {
            PerformanceTimer timer;
            
            auto key = engine.deriveKey(password, salt, iterations);
            auto derivation_time = timer.elapsed();
            
            ASSERT_EQ(key.size(), 32);
            
            // Key derivation should complete within reasonable time
            // Higher iteration counts should take longer but not excessively
            double time_per_iteration = (double)derivation_time.count() / iterations;
            ASSERT_TRUE(time_per_iteration < 0.1); // Less than 0.1ms per iteration
        }
    }
};

// Test registration function
void registerEncryptionEngineTests(TestFramework& framework) {
    EncryptionEngineTests::registerTests(framework);
}