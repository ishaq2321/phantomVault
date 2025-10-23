#include <gtest/gtest.h>
#include "phantom_vault/startup_manager.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>

using namespace phantom_vault;

class StartupManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for testing
        test_dir_ = std::filesystem::temp_directory_path() / "phantom_vault_test";
        std::filesystem::create_directories(test_dir_);

        // Set XDG_CONFIG_HOME to our test directory
        original_xdg_config_home_ = std::getenv("XDG_CONFIG_HOME");
        setenv("XDG_CONFIG_HOME", test_dir_.string().c_str(), 1);

        // Create test executable and icon files
        test_exec_ = test_dir_ / "phantom_vault";
        test_icon_ = test_dir_ / "phantom_vault.png";
        std::ofstream(test_exec_).close();
        std::ofstream(test_icon_).close();
    }

    void TearDown() override {
        // Restore original XDG_CONFIG_HOME
        if (original_xdg_config_home_) {
            setenv("XDG_CONFIG_HOME", original_xdg_config_home_, 1);
        } else {
            unsetenv("XDG_CONFIG_HOME");
        }

        // Clean up test directory
        std::filesystem::remove_all(test_dir_);
    }

    std::string readDesktopFile(const std::filesystem::path& path) {
        std::ifstream file(path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::filesystem::path test_dir_;
    std::filesystem::path test_exec_;
    std::filesystem::path test_icon_;
    const char* original_xdg_config_home_;
};

TEST_F(StartupManagerTest, InitializeTest) {
    StartupManager manager;
    EXPECT_TRUE(manager.initialize("phantom_vault",
                                 test_exec_.string(),
                                 test_icon_.string()));
}

TEST_F(StartupManagerTest, EnableDisableTest) {
    StartupManager manager;
    ASSERT_TRUE(manager.initialize("phantom_vault",
                                 test_exec_.string(),
                                 test_icon_.string()));

    // Test enabling autostart
    EXPECT_TRUE(manager.setAutostart(true));
    EXPECT_TRUE(manager.isAutostartEnabled());

    // Verify desktop entry file exists and has correct content
    auto desktop_file = std::filesystem::path(test_dir_) / "autostart" / "phantom_vault.desktop";
    EXPECT_TRUE(std::filesystem::exists(desktop_file));

    std::string content = readDesktopFile(desktop_file);
    EXPECT_TRUE(content.find("Exec=" + test_exec_.string()) != std::string::npos);
    EXPECT_TRUE(content.find("Icon=" + test_icon_.string()) != std::string::npos);

    // Test disabling autostart
    EXPECT_TRUE(manager.setAutostart(false));
    EXPECT_FALSE(manager.isAutostartEnabled());
    EXPECT_FALSE(std::filesystem::exists(desktop_file));
}

TEST_F(StartupManagerTest, UpdateCommandTest) {
    StartupManager manager;
    ASSERT_TRUE(manager.initialize("phantom_vault",
                                 test_exec_.string(),
                                 test_icon_.string()));

    // Enable autostart
    ASSERT_TRUE(manager.setAutostart(true));

    // Update command with arguments
    std::string new_exec = (test_dir_ / "new_exec").string();
    std::string args = "--minimize --hidden";
    EXPECT_TRUE(manager.updateCommand(new_exec, args));

    // Verify the update
    auto desktop_file = std::filesystem::path(test_dir_) / "autostart" / "phantom_vault.desktop";
    std::string content = readDesktopFile(desktop_file);
    EXPECT_TRUE(content.find("Exec=" + new_exec + " " + args) != std::string::npos);
}

TEST_F(StartupManagerTest, ErrorHandlingTest) {
    StartupManager manager;

    // Test operations before initialization
    EXPECT_FALSE(manager.setAutostart(true));
    EXPECT_FALSE(manager.isAutostartEnabled());
    EXPECT_FALSE(manager.updateCommand("/path/to/exec"));
    EXPECT_FALSE(manager.getLastError().empty());

    // Test with invalid paths
    EXPECT_FALSE(manager.initialize("phantom_vault",
                                  "/nonexistent/path",
                                  "/nonexistent/icon"));
    EXPECT_FALSE(manager.getLastError().empty());
}

TEST_F(StartupManagerTest, MultipleInstancesTest) {
    StartupManager manager1;
    StartupManager manager2;

    ASSERT_TRUE(manager1.initialize("phantom_vault",
                                  test_exec_.string(),
                                  test_icon_.string()));
    ASSERT_TRUE(manager2.initialize("phantom_vault",
                                  test_exec_.string(),
                                  test_icon_.string()));

    // Both instances should see the same state
    EXPECT_TRUE(manager1.setAutostart(true));
    EXPECT_TRUE(manager2.isAutostartEnabled());

    EXPECT_TRUE(manager2.setAutostart(false));
    EXPECT_FALSE(manager1.isAutostartEnabled());
} 