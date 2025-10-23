#include <gtest/gtest.h>
#include "phantom_vault/encryption.hpp"
#include "phantom_vault/storage.hpp"
#include "phantom_vault/filesystem.hpp"
#include <chrono>
#include <vector>
#include <random>
#include <fstream>

using namespace phantom_vault;
using namespace phantom_vault::storage;
using namespace std::chrono;

class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        encryption = std::make_unique<EncryptionEngine>();
        ASSERT_TRUE(encryption->initialize());
        
        storage = std::make_unique<SecureStorage>();
        std::vector<uint8_t> master_key(32, 0x42);
        ASSERT_TRUE(storage->initialize(master_key));
        
        filesystem = std::make_unique<fs::FileSystem>();
    }

    void TearDown() override {
        // Clean up test files
        cleanupTestFiles();
    }

    void cleanupTestFiles() {
        // Remove test files
        std::vector<std::string> test_files = {
            "/tmp/phantom_vault_perf_test_1.txt",
            "/tmp/phantom_vault_perf_test_2.txt",
            "/tmp/phantom_vault_perf_test_3.txt",
            "/tmp/phantom_vault_perf_test_large.txt"
        };
        
        for (const auto& file : test_files) {
            std::remove(file.c_str());
        }
        
        // Clean up test vaults
        auto vaults = storage->listVaults();
        for (const auto& vault_id : vaults) {
            if (vault_id.find("perf-test-") == 0) {
                storage->deleteVaultMetadata(vault_id);
                storage->removePasswordRecovery(vault_id);
            }
        }
    }

    std::vector<uint8_t> generateRandomData(size_t size) {
        std::vector<uint8_t> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        for (auto& byte : data) {
            byte = static_cast<uint8_t>(dis(gen));
        }
        return data;
    }

    std::unique_ptr<EncryptionEngine> encryption;
    std::unique_ptr<SecureStorage> storage;
    std::unique_ptr<fs::FileSystem> filesystem;
};

TEST_F(PerformanceTest, EncryptionPerformance) {
    // Test encryption performance with different data sizes
    std::vector<size_t> test_sizes = {1024, 10240, 102400, 1048576}; // 1KB, 10KB, 100KB, 1MB
    
    for (size_t size : test_sizes) {
        auto data = generateRandomData(size);
        auto key = encryption->generateKey();
        auto iv = encryption->generateIV();
        
        // Measure encryption time
        auto start = high_resolution_clock::now();
        auto encrypted = encryption->encryptData(data, key, iv);
        auto end = high_resolution_clock::now();
        
        auto encryption_time = duration_cast<microseconds>(end - start);
        
        // Measure decryption time
        start = high_resolution_clock::now();
        auto decrypted = encryption->decryptData(encrypted, key, iv);
        end = high_resolution_clock::now();
        
        auto decryption_time = duration_cast<microseconds>(end - start);
        
        // Verify correctness
        EXPECT_EQ(decrypted, data);
        
        // Performance assertions (adjust thresholds as needed)
        EXPECT_LT(encryption_time.count(), size * 10); // Should be less than 10 microseconds per byte
        EXPECT_LT(decryption_time.count(), size * 10);
        
        std::cout << "Size: " << size << " bytes, "
                  << "Encryption: " << encryption_time.count() << " μs, "
                  << "Decryption: " << decryption_time.count() << " μs, "
                  << "Throughput: " << (size * 1000000.0 / encryption_time.count()) << " bytes/sec" << std::endl;
    }
}

TEST_F(PerformanceTest, FileEncryptionPerformance) {
    // Test file encryption performance
    std::string test_file = "/tmp/phantom_vault_perf_test_large.txt";
    std::string encrypted_file = test_file + ".enc";
    std::string decrypted_file = test_file + ".dec";
    
    // Create a large test file (1MB)
    size_t file_size = 1024 * 1024;
    auto data = generateRandomData(file_size);
    
    std::ofstream file(test_file, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    file.close();
    
    auto key = encryption->generateKey();
    auto iv = encryption->generateIV();
    
    // Measure file encryption time
    auto start = high_resolution_clock::now();
    bool encrypt_success = encryption->encryptFile(test_file, encrypted_file, key, iv);
    auto end = high_resolution_clock::now();
    
    EXPECT_TRUE(encrypt_success);
    auto encryption_time = duration_cast<milliseconds>(end - start);
    
    // Measure file decryption time
    start = high_resolution_clock::now();
    bool decrypt_success = encryption->decryptFile(encrypted_file, decrypted_file, key, iv);
    end = high_resolution_clock::now();
    
    EXPECT_TRUE(decrypt_success);
    auto decryption_time = duration_cast<milliseconds>(end - start);
    
    // Verify file integrity
    std::ifstream original(test_file, std::ios::binary | std::ios::ate);
    std::ifstream decrypted(decrypted_file, std::ios::binary | std::ios::ate);
    
    EXPECT_EQ(original.tellg(), decrypted.tellg());
    
    std::cout << "File size: " << file_size << " bytes, "
              << "Encryption: " << encryption_time.count() << " ms, "
              << "Decryption: " << decryption_time.count() << " ms, "
              << "Encryption throughput: " << (file_size / 1024.0 / encryption_time.count()) << " MB/s, "
              << "Decryption throughput: " << (file_size / 1024.0 / decryption_time.count()) << " MB/s" << std::endl;
}

TEST_F(PerformanceTest, KeyDerivationPerformance) {
    // Test key derivation performance with different iteration counts
    std::vector<uint32_t> iterations = {1000, 10000, 100000, 1000000};
    std::string password = "test_password_123";
    auto salt = encryption->generateSalt();
    
    for (uint32_t iter : iterations) {
        auto start = high_resolution_clock::now();
        auto key = encryption->deriveKeyFromPassword(password, salt);
        auto end = high_resolution_clock::now();
        
        auto derivation_time = duration_cast<milliseconds>(end - start);
        
        EXPECT_EQ(key.size(), 32); // 256 bits
        
        std::cout << "Iterations: " << iter << ", "
                  << "Time: " << derivation_time.count() << " ms" << std::endl;
    }
}

TEST_F(PerformanceTest, StoragePerformance) {
    // Test storage operations performance
    const int num_vaults = 100;
    
    // Create multiple vaults
    auto start = high_resolution_clock::now();
    
    for (int i = 0; i < num_vaults; ++i) {
        VaultMetadata metadata;
        metadata.vault_id = "perf-test-vault-" + std::to_string(i);
        metadata.name = "Performance Test Vault " + std::to_string(i);
        metadata.description = "Performance testing vault";
        metadata.location = "/tmp/perf-test-vault-" + std::to_string(i);
        metadata.created_time = system_clock::now();
        metadata.modified_time = system_clock::now();
        metadata.key_verification = {0x01, 0x02, 0x03, 0x04};
        metadata.salt = encryption->generateSalt();
        metadata.iterations = 100000;
        
        EXPECT_TRUE(storage->saveVaultMetadata(metadata));
    }
    
    auto end = high_resolution_clock::now();
    auto creation_time = duration_cast<milliseconds>(end - start);
    
    // Test vault listing performance
    start = high_resolution_clock::now();
    auto vaults = storage->listVaults();
    end = high_resolution_clock::now();
    auto listing_time = duration_cast<microseconds>(end - start);
    
    EXPECT_EQ(vaults.size(), num_vaults);
    
    // Test vault loading performance
    start = high_resolution_clock::now();
    for (const auto& vault_id : vaults) {
        if (vault_id.find("perf-test-") == 0) {
            auto metadata = storage->loadVaultMetadata(vault_id);
            EXPECT_TRUE(metadata.has_value());
        }
    }
    end = high_resolution_clock::now();
    auto loading_time = duration_cast<milliseconds>(end - start);
    
    std::cout << "Created " << num_vaults << " vaults in " << creation_time.count() << " ms, "
              << "Listed vaults in " << listing_time.count() << " μs, "
              << "Loaded all vaults in " << loading_time.count() << " ms" << std::endl;
}

TEST_F(PerformanceTest, RecoveryPerformance) {
    // Test password recovery setup and verification performance
    const int num_questions = 5;
    
    // Set up recovery
    RecoveryInfo recovery_info;
    recovery_info.vault_id = "perf-test-recovery";
    recovery_info.attempts_remaining = 3;
    recovery_info.created_time = system_clock::now();
    recovery_info.last_used = system_clock::now();
    recovery_info.recovery_key = encryption->generateKey();
    recovery_info.recovery_iv = encryption->generateIV();
    
    // Add multiple questions
    for (int i = 0; i < num_questions; ++i) {
        RecoveryQuestion question;
        question.question_id = "q" + std::to_string(i + 1);
        question.question_text = "Test question " + std::to_string(i + 1) + "?";
        question.salt = encryption->generateSalt();
        std::string answer = "answer" + std::to_string(i + 1);
        question.answer_hash = encryption->deriveKeyFromPassword(answer, question.salt);
        recovery_info.questions.push_back(question);
    }
    
    // Measure setup time
    auto start = high_resolution_clock::now();
    EXPECT_TRUE(storage->setupPasswordRecovery(recovery_info.vault_id, recovery_info));
    auto end = high_resolution_clock::now();
    auto setup_time = duration_cast<milliseconds>(end - start);
    
    // Measure verification time
    std::vector<std::string> answers;
    for (int i = 0; i < num_questions; ++i) {
        answers.push_back("answer" + std::to_string(i + 1));
    }
    
    start = high_resolution_clock::now();
    auto recovery_key = storage->verifyRecoveryAnswers(recovery_info.vault_id, answers);
    end = high_resolution_clock::now();
    auto verification_time = duration_cast<milliseconds>(end - start);
    
    EXPECT_FALSE(recovery_key.empty());
    EXPECT_EQ(recovery_key, recovery_info.recovery_key);
    
    std::cout << "Recovery setup with " << num_questions << " questions: " << setup_time.count() << " ms, "
              << "Verification: " << verification_time.count() << " ms" << std::endl;
}

TEST_F(PerformanceTest, FileSystemPerformance) {
    // Test file system operations performance
    const int num_files = 50;
    std::vector<std::string> test_files;
    
    // Create test files
    for (int i = 0; i < num_files; ++i) {
        std::string filename = "/tmp/phantom_vault_perf_test_" + std::to_string(i) + ".txt";
        test_files.push_back(filename);
        
        std::ofstream file(filename);
        file << "Test content for file " << i;
        file.close();
    }
    
    // Test hiding performance
    auto start = high_resolution_clock::now();
    for (const auto& filename : test_files) {
        EXPECT_TRUE(filesystem->hide(filename));
    }
    auto end = high_resolution_clock::now();
    auto hiding_time = duration_cast<milliseconds>(end - start);
    
    // Test unhiding performance (files are now hidden with . prefix)
    start = high_resolution_clock::now();
    for (const auto& filename : test_files) {
        std::string hidden_filename = "/tmp/.phantom_vault_perf_test_" + 
            filename.substr(filename.find_last_of('_') + 1);
        EXPECT_TRUE(filesystem->unhide(hidden_filename));
    }
    end = high_resolution_clock::now();
    auto unhiding_time = duration_cast<milliseconds>(end - start);
    
    // Test attribute checking performance
    start = high_resolution_clock::now();
    for (const auto& filename : test_files) {
        EXPECT_TRUE(filesystem->exists(filename));
        EXPECT_FALSE(filesystem->isHidden(filename));
    }
    end = high_resolution_clock::now();
    auto checking_time = duration_cast<milliseconds>(end - start);
    
    std::cout << "Hid " << num_files << " files in " << hiding_time.count() << " ms, "
              << "Unhid " << num_files << " files in " << unhiding_time.count() << " ms, "
              << "Checked " << num_files << " files in " << checking_time.count() << " ms" << std::endl;
}

TEST_F(PerformanceTest, MemoryUsage) {
    // Test memory usage during operations
    const size_t large_data_size = 10 * 1024 * 1024; // 10MB
    auto large_data = generateRandomData(large_data_size);
    auto key = encryption->generateKey();
    auto iv = encryption->generateIV();
    
    // Measure memory usage during encryption
    auto start = high_resolution_clock::now();
    auto encrypted = encryption->encryptData(large_data, key, iv);
    auto end = high_resolution_clock::now();
    
    auto encryption_time = duration_cast<milliseconds>(end - start);
    
    // Verify the operation worked
    EXPECT_FALSE(encrypted.empty());
    EXPECT_GT(encrypted.size(), large_data.size());
    
    // Measure memory usage during decryption
    start = high_resolution_clock::now();
    auto decrypted = encryption->decryptData(encrypted, key, iv);
    end = high_resolution_clock::now();
    
    auto decryption_time = duration_cast<milliseconds>(end - start);
    
    // Verify correctness
    EXPECT_EQ(decrypted, large_data);
    
    std::cout << "Large data (" << large_data_size / 1024 / 1024 << " MB) - "
              << "Encryption: " << encryption_time.count() << " ms, "
              << "Decryption: " << decryption_time.count() << " ms" << std::endl;
}
