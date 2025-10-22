#include <gtest/gtest.h>
#include "phantom_vault/filesystem.hpp"
#include <fstream>
#include <thread>
#include <chrono>

using namespace phantom_vault::fs;
using namespace std::chrono_literals;

class FilesystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        fs.createDirectories("test_dir");
        std::ofstream test_file("test_dir/test.txt");
        test_file << "test content" << std::endl;
        test_file.close();
    }

    void TearDown() override {
        fs.remove("test_dir", true);
    }

    FileSystem fs;
};

TEST_F(FilesystemTest, HideUnhideFile) {
    ASSERT_TRUE(fs.exists("test_dir/test.txt"));
    ASSERT_FALSE(fs.isHidden("test_dir/test.txt"));
    
    ASSERT_TRUE(fs.hide("test_dir/test.txt"));
    ASSERT_TRUE(fs.exists("test_dir/.test.txt"));
    ASSERT_TRUE(fs.isHidden("test_dir/.test.txt"));
    
    ASSERT_TRUE(fs.unhide("test_dir/.test.txt"));
    ASSERT_TRUE(fs.exists("test_dir/test.txt"));
    ASSERT_FALSE(fs.isHidden("test_dir/test.txt"));
}

TEST_F(FilesystemTest, FileAttributes) {
    FileAttributes attrs;
    ASSERT_TRUE(fs.getAttributes("test_dir/test.txt", attrs));
    
    attrs.readonly = true;
    ASSERT_TRUE(fs.setAttributes("test_dir/test.txt", attrs));
    
    FileAttributes new_attrs;
    ASSERT_TRUE(fs.getAttributes("test_dir/test.txt", new_attrs));
    ASSERT_TRUE(new_attrs.readonly);
}

TEST_F(FilesystemTest, FileTimestamps) {
    auto now = std::chrono::system_clock::now();
    auto future = now + 24h;
    
    ASSERT_TRUE(fs.setTimestamps("test_dir/test.txt", now, future, future));
    
    FileAttributes attrs;
    ASSERT_TRUE(fs.getAttributes("test_dir/test.txt", attrs));
    
    // Note: filesystem timestamps might not have nanosecond precision
    // so we check if they're within 2 seconds
    auto diff_modified = std::chrono::duration_cast<std::chrono::seconds>(
        attrs.modified_time - future).count();
    ASSERT_LE(std::abs(diff_modified), 2);
    
    auto diff_accessed = std::chrono::duration_cast<std::chrono::seconds>(
        attrs.accessed_time - future).count();
    ASSERT_LE(std::abs(diff_accessed), 2);
}

TEST_F(FilesystemTest, DirectoryOperations) {
    ASSERT_TRUE(fs.createDirectories("test_dir/nested/deep"));
    ASSERT_TRUE(fs.exists("test_dir/nested/deep"));
    
    std::ofstream test_file("test_dir/nested/test2.txt");
    test_file << "test content 2" << std::endl;
    test_file.close();
    
    ASSERT_TRUE(fs.copy("test_dir/nested", "test_dir/nested_copy", true));
    ASSERT_TRUE(fs.exists("test_dir/nested_copy/test2.txt"));
    ASSERT_TRUE(fs.exists("test_dir/nested_copy/deep"));
    
    ASSERT_TRUE(fs.move("test_dir/nested_copy", "test_dir/nested_moved"));
    ASSERT_TRUE(fs.exists("test_dir/nested_moved/test2.txt"));
    ASSERT_FALSE(fs.exists("test_dir/nested_copy"));
    
    ASSERT_TRUE(fs.remove("test_dir/nested", true));
    ASSERT_FALSE(fs.exists("test_dir/nested"));
}

TEST_F(FilesystemTest, ErrorHandling) {
    ASSERT_FALSE(fs.hide("nonexistent.txt"));
    ASSERT_FALSE(fs.getLastError().value() == 0);
    
    ASSERT_FALSE(fs.copy("nonexistent.txt", "dest.txt", false));
    ASSERT_FALSE(fs.getLastError().value() == 0);
    
    ASSERT_FALSE(fs.move("nonexistent.txt", "dest.txt"));
    ASSERT_FALSE(fs.getLastError().value() == 0);
} 