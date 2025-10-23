#include <gtest/gtest.h>
#include "phantom_vault/sequence_detector.hpp"
#include <thread>
#include <chrono>

using namespace phantom_vault::service;

class SequenceDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        detector = std::make_unique<SequenceDetector>();
        ASSERT_TRUE(detector->initialize());
    }

    void TearDown() override {
        if (detector && detector->isActive()) {
            detector->stopDetection();
        }
        detector.reset();
    }

    std::unique_ptr<SequenceDetector> detector;
};

TEST_F(SequenceDetectorTest, Initialization) {
    EXPECT_FALSE(detector->isActive());
    EXPECT_TRUE(detector->getLastError().empty());
}

TEST_F(SequenceDetectorTest, PasswordHashing) {
    std::string password = "test123";
    std::string hash1 = PasswordUtils::hashPassword(password);
    std::string hash2 = PasswordUtils::hashPassword(password);
    
    // Same password should produce same hash
    EXPECT_EQ(hash1, hash2);
    
    // Different passwords should produce different hashes
    std::string different_hash = PasswordUtils::hashPassword("different");
    EXPECT_NE(hash1, different_hash);
    
    // Verify password against hash
    EXPECT_TRUE(PasswordUtils::verifyPassword(password, hash1));
    EXPECT_FALSE(PasswordUtils::verifyPassword("wrong", hash1));
}

TEST_F(SequenceDetectorTest, ModeExtraction) {
    std::string password = "1234";
    
    // Test temporary mode
    EXPECT_EQ(UnlockMode::TEMPORARY, PasswordUtils::extractMode("t1234", password));
    EXPECT_EQ(UnlockMode::TEMPORARY, PasswordUtils::extractMode("T1234", password));
    EXPECT_EQ(UnlockMode::TEMPORARY, PasswordUtils::extractMode("abct1234def", password));
    
    // Test permanent mode
    EXPECT_EQ(UnlockMode::PERMANENT, PasswordUtils::extractMode("p1234", password));
    EXPECT_EQ(UnlockMode::PERMANENT, PasswordUtils::extractMode("P1234", password));
    EXPECT_EQ(UnlockMode::PERMANENT, PasswordUtils::extractMode("xyzp1234ghi", password));
    
    // Test default (temporary) mode
    EXPECT_EQ(UnlockMode::TEMPORARY, PasswordUtils::extractMode("1234", password));
    EXPECT_EQ(UnlockMode::TEMPORARY, PasswordUtils::extractMode("abc1234def", password));
}

TEST_F(SequenceDetectorTest, FolderPasswordManagement) {
    // Add folder passwords
    FolderPassword folder1("id1", "Folder1", PasswordUtils::hashPassword("pass1"), "/path1", true);
    FolderPassword folder2("id2", "Folder2", PasswordUtils::hashPassword("pass2"), "/path2", false);
    
    detector->addFolderPassword(folder1);
    detector->addFolderPassword(folder2);
    
    // Remove one folder
    detector->removeFolderPassword("id1");
    
    // Clear all folders
    detector->clearFolderPasswords();
    
    // Should not crash
    EXPECT_FALSE(detector->isActive());
}

TEST_F(SequenceDetectorTest, DetectionLifecycle) {
    bool callback_called = false;
    PasswordDetectionResult callback_result;
    
    // Set up callback
    detector->setDetectionCallback([&](const PasswordDetectionResult& result) {
        callback_called = true;
        callback_result = result;
    });
    
    // Add test folder
    FolderPassword test_folder("test_id", "TestFolder", PasswordUtils::hashPassword("1234"), "/test/path", true);
    detector->addFolderPassword(test_folder);
    
    // Start detection
    EXPECT_TRUE(detector->startDetection(2)); // 2 second timeout
    EXPECT_TRUE(detector->isActive());
    
    // Simulate keystrokes that contain the password
    std::string test_sequence = "abct1234def";
    for (char c : test_sequence) {
        detector->processKeystroke(c);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Wait a bit for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Detection should stop after finding password
    EXPECT_FALSE(detector->isActive());
}

TEST_F(SequenceDetectorTest, TimeoutHandling) {
    // Start detection with short timeout
    EXPECT_TRUE(detector->startDetection(1)); // 1 second timeout
    EXPECT_TRUE(detector->isActive());
    
    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    
    // Should have timed out
    EXPECT_FALSE(detector->isActive());
}

TEST_F(SequenceDetectorTest, BufferSizeLimit) {
    detector->setMaxBufferSize(10); // Very small buffer for testing
    
    // Start detection
    EXPECT_TRUE(detector->startDetection(5));
    
    // Send more characters than buffer size
    for (int i = 0; i < 20; i++) {
        detector->processKeystroke('a' + (i % 26));
    }
    
    // Should still be active (not crashed)
    EXPECT_TRUE(detector->isActive());
    
    detector->stopDetection();
}

TEST_F(SequenceDetectorTest, CaseSensitivity) {
    // Test case insensitive (default)
    detector->setCaseSensitive(false);
    
    FolderPassword test_folder("test_id", "TestFolder", PasswordUtils::hashPassword("Test"), "/test/path", true);
    detector->addFolderPassword(test_folder);
    
    EXPECT_TRUE(detector->startDetection(2));
    
    // Should detect both upper and lower case
    detector->processKeystroke('T');
    detector->processKeystroke('E');
    detector->processKeystroke('S');
    detector->processKeystroke('T');
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    detector->stopDetection();
}

TEST_F(SequenceDetectorTest, StatsReporting) {
    std::string stats = detector->getStats();
    EXPECT_FALSE(stats.empty());
    
    // Should be valid JSON format
    EXPECT_NE(stats.find("is_active"), std::string::npos);
    EXPECT_NE(stats.find("folder_count"), std::string::npos);
}

TEST_F(SequenceDetectorTest, SecureCleanup) {
    // Start and stop detection multiple times
    for (int i = 0; i < 3; i++) {
        EXPECT_TRUE(detector->startDetection(1));
        
        // Add some keystrokes
        detector->processKeystroke('a');
        detector->processKeystroke('b');
        detector->processKeystroke('c');
        
        detector->stopDetection();
        EXPECT_FALSE(detector->isActive());
    }
    
    // Should not crash or leak memory
    EXPECT_TRUE(detector->getLastError().empty() || detector->getLastError().find("timeout") != std::string::npos);
}