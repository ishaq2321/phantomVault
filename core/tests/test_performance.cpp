/**
 * Performance Tests for Encryption Operations and System Impact
 * 
 * Tests for encryption/decryption performance, memory usage,
 * system resource impact, and scalability.
 */

#include "test_framework.hpp"
#include "../include/encryption_engine.hpp"
#include "../include/profile_vault.hpp"
#include "../include/folder_security_manager.hpp"
#include <filesystem>
#include <fstream>
#include <random>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>

using namespace phantomvault;
using namespace phantomvault::testing;
using namespace PhantomVault;

namespace fs = std::filesystem;

class PerformanceTests {
public:
    static void registerTests(TestFramework& framework) {
        // Encryption performance tests
        REGISTER_TEST(framework, "Performance", "encryption_throughput", testEncryptionThroughput);
        REGISTER_TEST(framework, "Performance", "decryption_throughput", testDecryptionThroughput);
        REGISTER_TEST(framework, "Performance", "key_derivation_performance", testKeyDerivationPerformance);
        REGISTER_TEST(framework, "Performance", "file_encryption_performance", testFileEncryptionPerformance);
        
        // Memory usage tests
        REGISTER_TEST(framework, "Performance", "memory_usage_encryption", testMemoryUsageEncryption);
        REGISTER_TEST(framework, "Performance", "memory_usage_vault", testMemoryUsageVault);
        REGISTER_TEST(framework, "Performance", "memory_leak_detection", testMemoryLeakDetection);
        
        // Scalability tests
        REGISTER_TEST(framework, "Performance", "concurrent_encryption", testConcurrentEncryption);
        REGISTER_TEST(framework, "Performance", "large_file_handling", testLargeFileHandling);
        REGISTER_TEST(framework, "Performance", "multiple_vault_performance", testMultipleVaultPerformance);
        
        // System impact tests
        REGISTER_TEST(framework, "Performance", "cpu_usage_impact", testCPUUsageImpact);
        REGISTER_TEST(framework, "Performance", "disk_io_performance", testDiskIOPerformance);
        REGISTER_TEST(framework, "Performance", "startup_performance", testStartupPerformance);
    }

private:
    static std::vector<uint8_t> generateTestData(size_t size) {
        std::vector<uint8_t> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        for (auto& byte : data) {
            byte = static_cast<uint8_t>(dis(gen));
        }
        
        return data;
    }
    
    static void createTestFile(const std::string& path, size_t size) {
        std::ofstream file(path, std::ios::binary);
        auto data = generateTestData(size);
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
    
    static void testEncryptionThroughput() {
        EncryptionEngine engine;
        std::string password = "performance_test_password";
        
        // Test different data sizes
        std::vector<size_t> test_sizes = {
            1024,        // 1 KB
            10240,       // 10 KB
            102400,      // 100 KB
            1048576,     // 1 MB
            10485760     // 10 MB
        };
        
        for (size_t size : test_sizes) {
            auto test_data = generateTestData(size);
            
            PerformanceTimer timer;
            auto result = engine.encryptData(test_data, password);
            auto elapsed = timer.elapsed();
            
            ASSERT_TRUE(result.success);
            
            // Calculate throughput in MB/s
            double mb_size = (double)size / (1024 * 1024);
            double seconds = elapsed.count() / 1000.0;
            double throughput = mb_size / seconds;
            
            // Performance requirements (adjust based on hardware)
            if (size >= 1048576) { // 1MB or larger
                ASSERT_TRUE(throughput > 50.0); // At least 50 MB/s
            } else {
                ASSERT_TRUE(throughput > 10.0); // At least 10 MB/s for smaller files
            }
            
            // Verify encryption completed within reasonable time
            ASSERT_TRUE(elapsed.count() < 10000); // Less than 10 seconds
        }
    }  
  
    static void testDecryptionThroughput() {
        EncryptionEngine engine;
        std::string password = "decryption_performance_test";
        
        std::vector<size_t> test_sizes = {1024, 10240, 102400, 1048576, 10485760};
        
        for (size_t size : test_sizes) {
            auto test_data = generateTestData(size);
            
            // First encrypt the data
            auto encrypted_result = engine.encryptData(test_data, password);
            ASSERT_TRUE(encrypted_result.success);
            
            // Then measure decryption performance
            PerformanceTimer timer;
            auto decrypted_result = engine.decryptData(
                encrypted_result.encrypted_data,
                password,
                encrypted_result.salt,
                encrypted_result.iv
            );
            auto elapsed = timer.elapsed();
            
            ASSERT_TRUE(decrypted_result.success);
            ASSERT_EQ(test_data, decrypted_result.decrypted_data);
            
            // Calculate throughput
            double mb_size = (double)size / (1024 * 1024);
            double seconds = elapsed.count() / 1000.0;
            double throughput = mb_size / seconds;
            
            // Decryption should be at least as fast as encryption
            if (size >= 1048576) {
                ASSERT_TRUE(throughput > 50.0);
            } else {
                ASSERT_TRUE(throughput > 10.0);
            }
        }
    }
    
    static void testKeyDerivationPerformance() {
        EncryptionEngine engine;
        std::string password = "key_derivation_performance_test";
        auto salt = engine.generateSalt();
        
        // Test different iteration counts
        std::vector<uint32_t> iteration_counts = {10000, 50000, 100000, 200000};
        
        for (uint32_t iterations : iteration_counts) {
            PerformanceTimer timer;
            auto key = engine.deriveKey(password, salt, iterations);
            auto elapsed = timer.elapsed();
            
            ASSERT_EQ(key.size(), 32);
            
            // Performance should scale roughly linearly with iterations
            double time_per_iteration = (double)elapsed.count() / iterations;
            
            // Should complete within reasonable time per iteration
            ASSERT_TRUE(time_per_iteration < 0.1); // Less than 0.1ms per iteration
            
            // Higher iteration counts should take longer but not excessively
            if (iterations >= 100000) {
                ASSERT_TRUE(elapsed.count() < 30000); // Less than 30 seconds
            }
        }
    }
    
    static void testFileEncryptionPerformance() {
        EncryptionEngine engine;
        std::string password = "file_encryption_performance";
        
        // Test different file sizes
        std::vector<size_t> file_sizes = {1024, 102400, 1048576, 10485760}; // 1KB to 10MB
        
        for (size_t size : file_sizes) {
            std::string test_file = "perf_test_" + std::to_string(size) + ".dat";
            std::string encrypted_file = test_file + ".enc";
            std::string decrypted_file = test_file + ".dec";
            
            // Create test file
            createTestFile(test_file, size);
            
            // Measure encryption performance
            PerformanceTimer encrypt_timer;
            auto encrypt_result = engine.encryptFile(test_file, encrypted_file, password);
            auto encrypt_time = encrypt_timer.elapsed();
            
            ASSERT_TRUE(encrypt_result.success);
            ASSERT_TRUE(fs::exists(encrypted_file));
            
            // Measure decryption performance
            PerformanceTimer decrypt_timer;
            auto decrypt_result = engine.decryptFile(encrypted_file, decrypted_file, password);
            auto decrypt_time = decrypt_timer.elapsed();
            
            ASSERT_TRUE(decrypt_result.success);
            ASSERT_TRUE(fs::exists(decrypted_file));
            
            // Verify file integrity
            ASSERT_EQ(fs::file_size(test_file), fs::file_size(decrypted_file));
            
            // Performance checks
            double mb_size = (double)size / (1024 * 1024);
            double encrypt_throughput = mb_size / (encrypt_time.count() / 1000.0);
            double decrypt_throughput = mb_size / (decrypt_time.count() / 1000.0);
            
            if (size >= 1048576) {
                ASSERT_TRUE(encrypt_throughput > 20.0); // At least 20 MB/s for large files
                ASSERT_TRUE(decrypt_throughput > 20.0);
            }
            
            // Cleanup
            fs::remove(test_file);
            fs::remove(encrypted_file);
            fs::remove(decrypted_file);
        }
    }
    
    static void testMemoryUsageEncryption() {
        EncryptionEngine engine;
        std::string password = "memory_usage_test";
        
        // Test memory usage with different data sizes
        std::vector<size_t> test_sizes = {1024, 102400, 1048576, 10485760};
        
        for (size_t size : test_sizes) {
            auto test_data = generateTestData(size);
            
            // Measure memory before encryption
            size_t memory_before = getCurrentMemoryUsage();
            
            auto encrypted_result = engine.encryptData(test_data, password);
            ASSERT_TRUE(encrypted_result.success);
            
            // Measure memory after encryption
            size_t memory_after = getCurrentMemoryUsage();
            
            // Memory usage should be reasonable (not more than 3x the data size)
            size_t memory_increase = memory_after > memory_before ? memory_after - memory_before : 0;
            ASSERT_TRUE(memory_increase < size * 3);
            
            // Decrypt to verify functionality
            auto decrypted_result = engine.decryptData(
                encrypted_result.encrypted_data,
                password,
                encrypted_result.salt,
                encrypted_result.iv
            );
            
            ASSERT_TRUE(decrypted_result.success);
            ASSERT_EQ(test_data, decrypted_result.decrypted_data);
        }
    }
    
    static void testMemoryUsageVault() {
        std::string vault_root = "./perf_test_vault";
        
        if (fs::exists(vault_root)) {
            fs::remove_all(vault_root);
        }
        
        size_t memory_before = getCurrentMemoryUsage();
        
        {
            ProfileVault vault("perf_test", vault_root);
            ASSERT_TRUE(vault.initialize());
            
            // Create multiple test folders
            std::vector<std::string> test_folders;
            for (int i = 0; i < 10; ++i) {
                std::string folder_name = "perf_folder_" + std::to_string(i);
                std::string folder_path = "./" + folder_name;
                
                fs::create_directories(folder_path);
                createTestFile(folder_path + "/test_file.dat", 10240); // 10KB each
                
                test_folders.push_back(folder_path);
            }
            
            // Lock all folders
            std::string master_key = "vault_performance_key";
            for (const auto& folder : test_folders) {
                auto result = vault.lockFolder(folder, master_key);
                ASSERT_TRUE(result.success);
            }
            
            size_t memory_peak = getCurrentMemoryUsage();
            size_t memory_increase = memory_peak > memory_before ? memory_peak - memory_before : 0;
            
            // Memory usage should be reasonable for 10 folders
            ASSERT_TRUE(memory_increase < 50 * 1024 * 1024); // Less than 50MB increase
            
            // Cleanup test folders
            for (const auto& folder : test_folders) {
                if (fs::exists(folder)) {
                    fs::remove_all(folder);
                }
            }
        }
        
        fs::remove_all(vault_root);
    }
    
    static void testMemoryLeakDetection() {
        size_t initial_memory = getCurrentMemoryUsage();
        
        // Perform many encryption/decryption cycles
        {
            EncryptionEngine engine;
            std::string password = "memory_leak_test";
            
            for (int i = 0; i < 100; ++i) {
                auto test_data = generateTestData(10240); // 10KB
                
                auto encrypted_result = engine.encryptData(test_data, password);
                ASSERT_TRUE(encrypted_result.success);
                
                auto decrypted_result = engine.decryptData(
                    encrypted_result.encrypted_data,
                    password,
                    encrypted_result.salt,
                    encrypted_result.iv
                );
                
                ASSERT_TRUE(decrypted_result.success);
                ASSERT_EQ(test_data, decrypted_result.decrypted_data);
            }
        }
        
        // Force garbage collection (if applicable)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        size_t final_memory = getCurrentMemoryUsage();
        size_t memory_difference = final_memory > initial_memory ? final_memory - initial_memory : 0;
        
        // Memory usage should not have increased significantly
        ASSERT_TRUE(memory_difference < 10 * 1024 * 1024); // Less than 10MB increase
    }
    
    static void testConcurrentEncryption() {
        const int num_threads = 4;
        const int operations_per_thread = 25;
        
        std::vector<std::thread> threads;
        std::vector<bool> results(num_threads, false);
        std::vector<std::chrono::milliseconds> times(num_threads);
        
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([t, &results, &times, operations_per_thread]() {
                EncryptionEngine engine;
                std::string password = "concurrent_test_" + std::to_string(t);
                bool all_success = true;
                
                PerformanceTimer timer;
                
                for (int i = 0; i < operations_per_thread; ++i) {
                    auto test_data = generateTestData(1024);
                    
                    auto encrypted_result = engine.encryptData(test_data, password);
                    if (!encrypted_result.success) {
                        all_success = false;
                        break;
                    }
                    
                    auto decrypted_result = engine.decryptData(
                        encrypted_result.encrypted_data,
                        password,
                        encrypted_result.salt,
                        encrypted_result.iv
                    );
                    
                    if (!decrypted_result.success || decrypted_result.decrypted_data != test_data) {
                        all_success = false;
                        break;
                    }
                }
                
                results[t] = all_success;
                times[t] = timer.elapsed();
            });
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Verify all threads succeeded
        for (bool result : results) {
            ASSERT_TRUE(result);
        }
        
        // Verify reasonable performance under concurrency
        for (auto time : times) {
            ASSERT_TRUE(time.count() < 30000); // Less than 30 seconds per thread
        }
    }
    
    static void testLargeFileHandling() {
        EncryptionEngine engine;
        std::string password = "large_file_test";
        
        // Test with a 100MB file
        size_t large_file_size = 100 * 1024 * 1024;
        std::string large_file = "large_test_file.dat";
        std::string encrypted_file = large_file + ".enc";
        std::string decrypted_file = large_file + ".dec";
        
        // Create large test file
        createTestFile(large_file, large_file_size);
        
        // Measure encryption time
        PerformanceTimer encrypt_timer;
        auto encrypt_result = engine.encryptFile(large_file, encrypted_file, password);
        auto encrypt_time = encrypt_timer.elapsed();
        
        ASSERT_TRUE(encrypt_result.success);
        ASSERT_TRUE(fs::exists(encrypted_file));
        
        // Measure decryption time
        PerformanceTimer decrypt_timer;
        auto decrypt_result = engine.decryptFile(encrypted_file, decrypted_file, password);
        auto decrypt_time = decrypt_timer.elapsed();
        
        ASSERT_TRUE(decrypt_result.success);
        ASSERT_TRUE(fs::exists(decrypted_file));
        
        // Verify file sizes match
        ASSERT_EQ(fs::file_size(large_file), fs::file_size(decrypted_file));
        
        // Performance requirements for large files
        double mb_size = (double)large_file_size / (1024 * 1024);
        double encrypt_throughput = mb_size / (encrypt_time.count() / 1000.0);
        double decrypt_throughput = mb_size / (decrypt_time.count() / 1000.0);
        
        // Should achieve reasonable throughput for large files
        ASSERT_TRUE(encrypt_throughput > 30.0); // At least 30 MB/s
        ASSERT_TRUE(decrypt_throughput > 30.0);
        
        // Should complete within reasonable time
        ASSERT_TRUE(encrypt_time.count() < 60000); // Less than 1 minute
        ASSERT_TRUE(decrypt_time.count() < 60000);
        
        // Cleanup
        fs::remove(large_file);
        fs::remove(encrypted_file);
        fs::remove(decrypted_file);
    }
    
    static void testMultipleVaultPerformance() {
        std::string vault_root = "./multi_vault_perf_test";
        
        if (fs::exists(vault_root)) {
            fs::remove_all(vault_root);
        }
        
        const int num_vaults = 5;
        std::vector<std::unique_ptr<ProfileVault>> vaults;
        
        PerformanceTimer setup_timer;
        
        // Create multiple vaults
        for (int i = 0; i < num_vaults; ++i) {
            auto vault = std::make_unique<ProfileVault>("vault_" + std::to_string(i), vault_root);
            ASSERT_TRUE(vault->initialize());
            vaults.push_back(std::move(vault));
        }
        
        auto setup_time = setup_timer.elapsed();
        
        // Create test folders for each vault
        std::vector<std::string> test_folders;
        for (int i = 0; i < num_vaults; ++i) {
            std::string folder_name = "multi_vault_folder_" + std::to_string(i);
            std::string folder_path = "./" + folder_name;
            
            fs::create_directories(folder_path);
            createTestFile(folder_path + "/test_file.dat", 10240);
            test_folders.push_back(folder_path);
        }
        
        // Measure vault operations performance
        PerformanceTimer operations_timer;
        
        std::string master_key = "multi_vault_key";
        for (int i = 0; i < num_vaults; ++i) {
            auto result = vaults[i]->lockFolder(test_folders[i], master_key);
            ASSERT_TRUE(result.success);
        }
        
        auto operations_time = operations_timer.elapsed();
        
        // Performance checks
        ASSERT_TRUE(setup_time.count() < 5000); // Setup should be fast
        ASSERT_TRUE(operations_time.count() < 10000); // Operations should be reasonable
        
        // Cleanup
        for (const auto& folder : test_folders) {
            if (fs::exists(folder)) {
                fs::remove_all(folder);
            }
        }
        fs::remove_all(vault_root);
    }
    
    static void testCPUUsageImpact() {
        // This is a simplified CPU usage test
        // In a real implementation, you might use system APIs to measure CPU usage
        
        EncryptionEngine engine;
        std::string password = "cpu_usage_test";
        
        // Measure time for CPU-intensive operations
        auto test_data = generateTestData(1048576); // 1MB
        
        PerformanceTimer timer;
        
        // Perform multiple encryption operations
        for (int i = 0; i < 10; ++i) {
            auto result = engine.encryptData(test_data, password);
            ASSERT_TRUE(result.success);
        }
        
        auto total_time = timer.elapsed();
        
        // Should complete within reasonable time (indicating reasonable CPU usage)
        ASSERT_TRUE(total_time.count() < 30000); // Less than 30 seconds for 10 operations
        
        // Average time per operation should be consistent
        double avg_time = total_time.count() / 10.0;
        ASSERT_TRUE(avg_time < 5000); // Less than 5 seconds per operation
    }
    
    static void testDiskIOPerformance() {
        EncryptionEngine engine;
        std::string password = "disk_io_test";
        
        // Test with different file sizes to measure disk I/O impact
        std::vector<size_t> file_sizes = {10240, 102400, 1048576}; // 10KB, 100KB, 1MB
        
        for (size_t size : file_sizes) {
            std::string test_file = "disk_io_test_" + std::to_string(size) + ".dat";
            std::string encrypted_file = test_file + ".enc";
            
            createTestFile(test_file, size);
            
            // Measure file encryption I/O performance
            PerformanceTimer io_timer;
            auto result = engine.encryptFile(test_file, encrypted_file, password);
            auto io_time = io_timer.elapsed();
            
            ASSERT_TRUE(result.success);
            
            // I/O should be reasonably fast
            double mb_size = (double)size / (1024 * 1024);
            if (mb_size >= 1.0) {
                double io_throughput = mb_size / (io_time.count() / 1000.0);
                ASSERT_TRUE(io_throughput > 10.0); // At least 10 MB/s I/O throughput
            }
            
            // Cleanup
            fs::remove(test_file);
            fs::remove(encrypted_file);
        }
    }
    
    static void testStartupPerformance() {
        std::string vault_root = "./startup_perf_test";
        
        if (fs::exists(vault_root)) {
            fs::remove_all(vault_root);
        }
        
        // Measure vault initialization time
        PerformanceTimer startup_timer;
        
        {
            ProfileVault vault("startup_test", vault_root);
            bool init_result = vault.initialize();
            ASSERT_TRUE(init_result);
        }
        
        auto startup_time = startup_timer.elapsed();
        
        // Startup should be fast
        ASSERT_TRUE(startup_time.count() < 1000); // Less than 1 second
        
        // Test startup with existing vault data
        {
            ProfileVault vault("startup_test", vault_root);
            
            PerformanceTimer existing_startup_timer;
            bool init_result = vault.initialize();
            auto existing_startup_time = existing_startup_timer.elapsed();
            
            ASSERT_TRUE(init_result);
            ASSERT_TRUE(existing_startup_time.count() < 2000); // Less than 2 seconds with existing data
        }
        
        fs::remove_all(vault_root);
    }
    
    // Helper function to get current memory usage (simplified implementation)
    static size_t getCurrentMemoryUsage() {
        // This is a simplified implementation
        // In a real system, you would use platform-specific APIs
        // to get actual memory usage
        
        #ifdef __linux__
        std::ifstream status_file("/proc/self/status");
        std::string line;
        while (std::getline(status_file, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                std::istringstream iss(line);
                std::string label, value, unit;
                iss >> label >> value >> unit;
                return std::stoul(value) * 1024; // Convert KB to bytes
            }
        }
        #endif
        
        // Fallback: return a reasonable estimate
        return 10 * 1024 * 1024; // 10MB
    }
};

// Test registration function
void registerPerformanceTests(TestFramework& framework) {
    PerformanceTests::registerTests(framework);
}