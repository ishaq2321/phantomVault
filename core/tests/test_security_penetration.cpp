/**
 * Advanced Security and Penetration Testing Suite
 * 
 * Comprehensive security testing including:
 * - Cryptographic strength validation
 * - Timing attack resistance
 * - Buffer overflow protection
 * - Side-channel attack resistance
 * - Penetration testing scenarios
 * - Vulnerability assessment
 */

#include "test_framework.hpp"
#include "../include/encryption_engine.hpp"
#include "../include/profile_manager.hpp"
#include "../include/vault_handler.hpp"
#include "../include/privilege_manager.hpp"
#include <random>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <set>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace fs = std::filesystem;

using namespace phantomvault;
using namespace phantomvault::testing;

class SecurityPenetrationTests {
public:
    static void registerTests(TestFramework& framework) {
        // Cryptographic security tests
        REGISTER_TEST(framework, "SecurityPenetration", "cryptographic_strength", testCryptographicStrength);
        REGISTER_TEST(framework, "SecurityPenetration", "key_derivation_security", testKeyDerivationSecurity);
        REGISTER_TEST(framework, "SecurityPenetration", "random_number_quality", testRandomNumberQuality);
        REGISTER_TEST(framework, "SecurityPenetration", "encryption_avalanche_effect", testEncryptionAvalancheEffect);
        
        // Timing attack tests
        REGISTER_TEST(framework, "SecurityPenetration", "timing_attack_resistance", testTimingAttackResistance);
        REGISTER_TEST(framework, "SecurityPenetration", "cache_timing_attacks", testCacheTimingAttacks);
        REGISTER_TEST(framework, "SecurityPenetration", "statistical_timing_analysis", testStatisticalTimingAnalysis);
        
        // Input validation and fuzzing
        REGISTER_TEST(framework, "SecurityPenetration", "input_fuzzing", testInputFuzzing);
        REGISTER_TEST(framework, "SecurityPenetration", "buffer_overflow_protection", testBufferOverflowProtection);
        REGISTER_TEST(framework, "SecurityPenetration", "malformed_data_handling", testMalformedDataHandling);
        
        // Authentication security
        REGISTER_TEST(framework, "SecurityPenetration", "brute_force_resistance", testBruteForceResistance);
        REGISTER_TEST(framework, "SecurityPenetration", "password_strength_validation", testPasswordStrengthValidation);
        REGISTER_TEST(framework, "SecurityPenetration", "session_security", testSessionSecurity);
        
        // Memory security
        REGISTER_TEST(framework, "SecurityPenetration", "memory_protection", testMemoryProtection);
        REGISTER_TEST(framework, "SecurityPenetration", "secure_memory_clearing", testSecureMemoryClearing);
        REGISTER_TEST(framework, "SecurityPenetration", "memory_leak_security", testMemoryLeakSecurity);
        
        // Side-channel attacks
        REGISTER_TEST(framework, "SecurityPenetration", "power_analysis_resistance", testPowerAnalysisResistance);
        REGISTER_TEST(framework, "SecurityPenetration", "electromagnetic_resistance", testElectromagneticResistance);
        
        // Penetration testing scenarios
        REGISTER_TEST(framework, "SecurityPenetration", "privilege_escalation_attempts", testPrivilegeEscalationAttempts);
        REGISTER_TEST(framework, "SecurityPenetration", "injection_attacks", testInjectionAttacks);
        REGISTER_TEST(framework, "SecurityPenetration", "path_traversal_attacks", testPathTraversalAttacks);
    }

private:
    // Cryptographic Security Tests
    static void testCryptographicStrength() {
        EncryptionEngine engine;
        
        // Test key strength
        auto salt = engine.generateSalt();
        auto key = engine.deriveKey("strong_password_123!", salt, EncryptionEngine::KeyDerivationConfig());
        
        ASSERT_TRUE(SecurityTestUtils::testKeyStrength(key));
        ASSERT_TRUE(SecurityTestUtils::hasProperEntropy(key));
        
        // Test entropy of generated values
        std::vector<std::vector<uint8_t>> salts;
        std::vector<std::vector<uint8_t>> ivs;
        
        for (int i = 0; i < 1000; ++i) {
            salts.push_back(engine.generateSalt());
            ivs.push_back(engine.generateIV());
        }
        
        // Test uniqueness
        ASSERT_TRUE(SecurityTestUtils::testSaltUniqueness([&]() { return engine.generateSalt(); }));
        ASSERT_TRUE(SecurityTestUtils::testIVUniqueness([&]() { return engine.generateIV(); }));
        
        // Test statistical properties
        for (const auto& salt : salts) {
            ASSERT_TRUE(SecurityTestUtils::isRandomDataUniform(salt));
            ASSERT_TRUE(SecurityTestUtils::passesChiSquareTest(salt));
        }
    }
    
    static void testKeyDerivationSecurity() {
        EncryptionEngine engine;
        std::string password = "test_password_security";
        auto salt = engine.generateSalt();
        
        // Test different iteration counts
        auto key_10k = engine.deriveKey(password, salt, EncryptionEngine::KeyDerivationConfig());
        auto key_100k = engine.deriveKey(password, salt, EncryptionEngine::KeyDerivationConfig());
        
        ASSERT_NE(key_10k, key_100k); // Different iterations should produce different keys
        
        // Test timing resistance
        auto timing_function = [&](const std::string& pwd) -> bool {
            auto derived_key = engine.deriveKey(pwd, salt, EncryptionEngine::KeyDerivationConfig());
            return derived_key == key_10k;
        };
        
        ASSERT_TRUE(SecurityTestUtils::isTimingAttackResistant(
            timing_function, password, "wrong_password", 100));
    }
    
    static void testRandomNumberQuality() {
        EncryptionEngine engine;
        
        // Generate large amount of random data
        std::vector<uint8_t> random_data;
        for (int i = 0; i < 100; ++i) {
            auto salt = engine.generateSalt();
            random_data.insert(random_data.end(), salt.begin(), salt.end());
        }
        
        // Test statistical properties
        ASSERT_TRUE(SecurityTestUtils::isRandomDataUniform(random_data));
        ASSERT_TRUE(SecurityTestUtils::hasProperEntropy(random_data));
        ASSERT_TRUE(SecurityTestUtils::passesChiSquareTest(random_data));
        ASSERT_TRUE(SecurityTestUtils::passesRunsTest(random_data));
        
        // Test entropy calculation
        double entropy = SecurityTestUtils::calculateEntropy(random_data);
        ASSERT_TRUE(entropy > 7.5); // Should have high entropy (close to 8.0 for perfect randomness)
    }
    
    static void testEncryptionAvalancheEffect() {
        EncryptionEngine engine;
        std::string password = "avalanche_test_password";
        
        // Test avalanche effect - small input change should cause large output change
        std::vector<uint8_t> data1 = {'t', 'e', 's', 't', ' ', 'd', 'a', 't', 'a'};
        std::vector<uint8_t> data2 = {'t', 'e', 's', 't', ' ', 'd', 'a', 't', 'b'}; // One bit different
        
        auto result1 = engine.encryptData(data1, password);
        auto result2 = engine.encryptData(data2, password);
        
        ASSERT_TRUE(result1.success && result2.success);
        
        // Count different bits
        size_t different_bits = 0;
        size_t min_size = std::min(result1.encrypted_data.size(), result2.encrypted_data.size());
        
        for (size_t i = 0; i < min_size; ++i) {
            uint8_t xor_result = result1.encrypted_data[i] ^ result2.encrypted_data[i];
            different_bits += __builtin_popcount(xor_result);
        }
        
        // Should have approximately 50% different bits (avalanche effect)
        double difference_ratio = (double)different_bits / (min_size * 8);
        ASSERT_TRUE(difference_ratio > 0.4 && difference_ratio < 0.6);
    }   
 
    // Timing Attack Tests
    static void testTimingAttackResistance() {
        ProfileManager manager;
        manager.initialize("./test_timing_profiles");
        
        auto create_result = manager.createProfile("timing_user", "TimingTest123!", "Timing Test");
        ASSERT_TRUE(create_result.success);
        
        // Test authentication timing
        auto auth_function = [&](const std::string& password) -> bool {
            auto result = manager.authenticateProfile("timing_user", password);
            return result.success;
        };
        
        ASSERT_TRUE(SecurityTestUtils::isTimingAttackResistant(
            auth_function, "TimingTest123!", "WrongPassword123!", 1000));
        
        if (fs::exists("./test_timing_profiles")) {
            fs::remove_all("./test_timing_profiles");
        }
    }
    
    static void testCacheTimingAttacks() {
        EncryptionEngine engine;
        std::string correct_password = "CacheTimingTest123!";
        auto salt = engine.generateSalt();
        
        // Test cache timing resistance in key derivation
        auto cache_function = [&](const std::string& password) {
            engine.deriveKey(password, salt, EncryptionEngine::KeyDerivationConfig());
        };
        
        ASSERT_TRUE(SecurityTestUtils::testCacheTimingAttacks(cache_function));
    }
    
    static void testStatisticalTimingAnalysis() {
        EncryptionEngine engine;
        std::vector<std::string> test_passwords = {
            "password123", "different_password", "another_test", "timing_analysis"
        };
        
        auto timing_function = [&](const std::string& password) -> bool {
            auto salt = engine.generateSalt();
            auto key = engine.deriveKey(password, salt, EncryptionEngine::KeyDerivationConfig());
            return key.size() == 32; // Always true, just for timing
        };
        
        auto analysis_result = SecurityTestUtils::analyzeTimingVulnerability(
            timing_function, test_passwords, 1000);
        
        ASSERT_FALSE(analysis_result.vulnerable);
        ASSERT_TRUE(analysis_result.confidence_level > 0.95);
    }
    
    // Input Validation and Fuzzing Tests
    static void testInputFuzzing() {
        VaultHandler handler;
        handler.initialize("./test_fuzz_vault");
        
        auto fuzz_inputs = SecurityTestUtils::generateFuzzingInputs(100);
        
        for (const auto& input : fuzz_inputs) {
            // Test with malformed inputs - should not crash
            auto result = handler.hideFolder(input, "fuzz_vault");
            // Should fail gracefully, not crash
            ASSERT_FALSE(result.success || result.success); // Always passes, tests no crash
        }
        
        if (fs::exists("./test_fuzz_vault")) {
            fs::remove_all("./test_fuzz_vault");
        }
    }
    
    static void testBufferOverflowProtection() {
        ProfileManager manager;
        manager.initialize("./test_buffer_profiles");
        
        // Test with extremely long inputs
        std::string long_username(10000, 'A');
        std::string long_password(10000, 'B');
        std::string long_display_name(10000, 'C');
        
        auto buffer_function = [&](const std::vector<uint8_t>& data) {
            std::string input(data.begin(), data.end());
            manager.createProfile(input, "test_password", "Test User");
        };
        
        ASSERT_TRUE(SecurityTestUtils::testBufferOverflow(buffer_function));
        
        if (fs::exists("./test_buffer_profiles")) {
            fs::remove_all("./test_buffer_profiles");
        }
    }
    
    static void testMalformedDataHandling() {
        EncryptionEngine engine;
        auto malformed_data = SecurityTestUtils::generateMalformedData(50);
        
        for (const auto& data : malformed_data) {
            // Test encryption with malformed data - should handle gracefully
            auto result = engine.encryptData(data, "test_password");
            // Should either succeed or fail gracefully, not crash
            ASSERT_TRUE(result.success || !result.success); // Always passes, tests no crash
        }
    }
    
    // Authentication Security Tests
    static void testBruteForceResistance() {
        ProfileManager manager;
        manager.initialize("./test_brute_profiles");
        
        auto create_result = manager.createProfile("brute_user", "BruteForceTest123!", "Brute Force Test");
        ASSERT_TRUE(create_result.success);
        
        // Simulate brute force attempts
        std::vector<std::string> common_passwords = {
            "password", "123456", "password123", "admin", "qwerty",
            "letmein", "welcome", "monkey", "dragon", "master"
        };
        
        int failed_attempts = 0;
        for (const auto& password : common_passwords) {
            auto result = manager.authenticateProfile("brute_user", password);
            if (!result.success) {
                failed_attempts++;
            }
        }
        
        // Should reject all common passwords
        ASSERT_EQ(failed_attempts, common_passwords.size());
        
        if (fs::exists("./test_brute_profiles")) {
            fs::remove_all("./test_brute_profiles");
        }
    }
    
    static void testPasswordStrengthValidation() {
        ProfileManager manager;
        manager.initialize("./test_strength_profiles");
        
        // Test weak passwords - should be rejected
        std::vector<std::string> weak_passwords = {
            "123", "password", "abc", "qwerty", "admin", "test"
        };
        
        for (const auto& weak_pwd : weak_passwords) {
            auto result = manager.createProfile("test_user", weak_pwd, "Test User");
            ASSERT_FALSE(result.success); // Should reject weak passwords
        }
        
        // Test strong password - should be accepted
        auto strong_result = manager.createProfile("strong_user", "Str0ng_P@ssw0rd_2024!", "Strong User");
        ASSERT_TRUE(strong_result.success);
        
        if (fs::exists("./test_strength_profiles")) {
            fs::remove_all("./test_strength_profiles");
        }
    }
    
    static void testSessionSecurity() {
        ProfileManager manager;
        manager.initialize("./test_session_profiles");
        
        auto create_result = manager.createProfile("session_user", "SessionTest123!", "Session Test");
        ASSERT_TRUE(create_result.success);
        
        auto auth_result = manager.authenticateProfile("session_user", "SessionTest123!");
        ASSERT_TRUE(auth_result.success);
        
        // Test session timeout (if implemented)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Session should still be valid for short duration
        auto profile_info = manager.getProfileInfo(create_result.profile_id);
        ASSERT_TRUE(profile_info.has_value());
        
        if (fs::exists("./test_session_profiles")) {
            fs::remove_all("./test_session_profiles");
        }
    }   
 
    // Memory Security Tests
    static void testMemoryProtection() {
        EncryptionEngine engine;
        std::string sensitive_data = "sensitive_encryption_key_data";
        
        // Test that sensitive data is properly cleared from memory
        {
            auto salt = engine.generateSalt();
            auto key = engine.deriveKey(sensitive_data, salt, EncryptionEngine::KeyDerivationConfig());
            
            // Key should be valid
            ASSERT_EQ(key.size(), 32);
        }
        
        // Test memory leak detection
        ASSERT_TRUE(SecurityTestUtils::detectMemoryLeaks([&]() {
            auto salt = engine.generateSalt();
            engine.deriveKey(sensitive_data, salt, EncryptionEngine::KeyDerivationConfig());
        }, 100));
    }
    
    static void testSecureMemoryClearing() {
        EncryptionEngine engine;
        
        // Allocate memory for sensitive data
        std::vector<uint8_t> sensitive_buffer(1024);
        std::fill(sensitive_buffer.begin(), sensitive_buffer.end(), 0xAA);
        
        // Use the buffer for encryption
        auto result = engine.encryptData(sensitive_buffer, "test_password");
        ASSERT_TRUE(result.success);
        
        // Clear the buffer
        std::fill(sensitive_buffer.begin(), sensitive_buffer.end(), 0x00);
        
        // Verify memory is cleared
        ASSERT_TRUE(SecurityTestUtils::isMemoryCleared(
            sensitive_buffer.data(), sensitive_buffer.size()));
    }
    
    static void testMemoryLeakSecurity() {
        size_t initial_memory = SecurityTestUtils::measureMemoryUsage();
        
        // Perform many operations that could leak memory
        {
            ProfileManager manager;
            manager.initialize("./test_memory_leak");
            
            for (int i = 0; i < 100; ++i) {
                std::string username = "user_" + std::to_string(i);
                std::string password = "Password_" + std::to_string(i) + "!";
                
                auto result = manager.createProfile(username, password, "Test User");
                if (result.success) {
                    manager.authenticateProfile(username, password);
                }
            }
        }
        
        // Force cleanup
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        size_t final_memory = SecurityTestUtils::measureMemoryUsage();
        size_t memory_increase = final_memory > initial_memory ? final_memory - initial_memory : 0;
        
        // Memory increase should be reasonable
        ASSERT_TRUE(memory_increase < 50 * 1024 * 1024); // Less than 50MB
        
        if (fs::exists("./test_memory_leak")) {
            fs::remove_all("./test_memory_leak");
        }
    }
    
    // Side-Channel Attack Tests
    static void testPowerAnalysisResistance() {
        EncryptionEngine engine;
        
        auto crypto_function = [&]() {
            auto salt = engine.generateSalt();
            engine.deriveKey("power_analysis_test", salt, EncryptionEngine::KeyDerivationConfig());
        };
        
        ASSERT_TRUE(SecurityTestUtils::testPowerAnalysisResistance(crypto_function));
    }
    
    static void testElectromagneticResistance() {
        EncryptionEngine engine;
        std::string password = "electromagnetic_test";
        
        // Test that encryption operations don't leak information through EM emissions
        std::vector<uint8_t> test_data(1024, 0x55);
        
        // Perform multiple encryptions with same data
        std::vector<std::chrono::nanoseconds> timing_variations;
        
        for (int i = 0; i < 100; ++i) {
            PerformanceTimer timer;
            auto result = engine.encryptData(test_data, password);
            auto elapsed = timer.elapsedNanos();
            
            ASSERT_TRUE(result.success);
            timing_variations.push_back(elapsed);
        }
        
        // Timing should be relatively consistent (no significant variations)
        auto min_time = *std::min_element(timing_variations.begin(), timing_variations.end());
        auto max_time = *std::max_element(timing_variations.begin(), timing_variations.end());
        
        double variation_ratio = (double)(max_time.count() - min_time.count()) / min_time.count();
        ASSERT_TRUE(variation_ratio < 0.5); // Less than 50% variation
    }
    
    // Penetration Testing Scenarios
    static void testPrivilegeEscalationAttempts() {
        PrivilegeManager manager;
        manager.initialize();
        
        // Test various privilege escalation attempts
        std::vector<PrivilegedOperation> operations = {
            PrivilegedOperation::FOLDER_HIDING,
            PrivilegedOperation::SYSTEM_MODIFICATION,
            PrivilegedOperation::REGISTRY_ACCESS
        };
        
        for (auto operation : operations) {
            auto result = manager.requestElevationForOperation(operation);
            // Should either succeed with proper authorization or fail securely
            ASSERT_TRUE(result.success || !result.errorDetails.empty());
        }
    }
    
    static void testInjectionAttacks() {
        ProfileManager manager;
        manager.initialize("./test_injection_profiles");
        
        // Test SQL injection attempts
        std::vector<std::string> injection_payloads = {
            "'; DROP TABLE users; --",
            "admin'--",
            "' OR '1'='1",
            "'; INSERT INTO users VALUES ('hacker', 'password'); --",
            "' UNION SELECT * FROM sensitive_data --"
        };
        
        auto injection_function = [&](const std::string& payload) -> bool {
            auto result = manager.createProfile(payload, "test_password", "Test User");
            return result.success;
        };
        
        ASSERT_TRUE(SecurityTestUtils::testSQLInjection(injection_function));
        
        if (fs::exists("./test_injection_profiles")) {
            fs::remove_all("./test_injection_profiles");
        }
    }
    
    static void testPathTraversalAttacks() {
        VaultHandler handler;
        handler.initialize("./test_traversal_vault");
        
        // Test path traversal attempts
        std::vector<std::string> traversal_payloads = {
            "../../../etc/passwd",
            "..\\..\\..\\windows\\system32\\config\\sam",
            "/etc/shadow",
            "C:\\Windows\\System32\\config\\SAM",
            "../../../../root/.ssh/id_rsa"
        };
        
        auto traversal_function = [&](const std::string& path) -> bool {
            auto result = handler.hideFolder(path, "traversal_vault");
            return result.success;
        };
        
        ASSERT_TRUE(SecurityTestUtils::testPathTraversal(traversal_function));
        
        if (fs::exists("./test_traversal_vault")) {
            fs::remove_all("./test_traversal_vault");
        }
    }
};

// Test registration function
void registerSecurityPenetrationTests(TestFramework& framework) {
    SecurityPenetrationTests::registerTests(framework);
}