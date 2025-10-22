#include <gtest/gtest.h>
#include "phantom_vault/system_tray.hpp"
#include <QApplication>
#include <QSignalSpy>
#include <QTest>
#include <thread>
#include <chrono>
#include <filesystem>

using namespace phantom_vault;
using namespace std::chrono_literals;

class SystemTrayTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test icon file
        std::filesystem::path test_icon = std::filesystem::temp_directory_path() / "test_icon.png";
        icon_path_ = test_icon.string();
        
        // Create a simple 16x16 PNG file
        QImage img(16, 16, QImage::Format_ARGB32);
        img.fill(Qt::red);
        img.save(QString::fromStdString(icon_path_));
    }

    void TearDown() override {
        // Remove test icon file
        std::filesystem::remove(icon_path_);
    }

    std::string icon_path_;
};

TEST_F(SystemTrayTest, InitializeTest) {
    SystemTray tray;
    EXPECT_TRUE(tray.initialize(icon_path_, "Test Tooltip"));
}

TEST_F(SystemTrayTest, MenuTest) {
    SystemTray tray;
    ASSERT_TRUE(tray.initialize(icon_path_, "Test Tooltip"));

    bool callback_called = false;
    std::vector<SystemTray::MenuItem> menu_items = {
        {"Item 1", [&]() { callback_called = true; }},
        {}, // separator
        {"Item 2", nullptr, true}, // separator
        {"Item 3", nullptr, false, true, false, true} // checkable item
    };

    EXPECT_TRUE(tray.setMenu(menu_items));
}

TEST_F(SystemTrayTest, VisibilityTest) {
    SystemTray tray;
    ASSERT_TRUE(tray.initialize(icon_path_, "Test Tooltip"));

    EXPECT_FALSE(tray.isVisible());
    tray.setVisible(true);
    EXPECT_TRUE(tray.isVisible());
    tray.setVisible(false);
    EXPECT_FALSE(tray.isVisible());
}

TEST_F(SystemTrayTest, IconTest) {
    SystemTray tray;
    ASSERT_TRUE(tray.initialize(icon_path_, "Test Tooltip"));

    // Test with invalid icon
    EXPECT_FALSE(tray.setIcon("/nonexistent/path.png"));
    EXPECT_FALSE(tray.getLastError().empty());

    // Test with valid icon
    EXPECT_TRUE(tray.setIcon(icon_path_));
}

TEST_F(SystemTrayTest, TooltipTest) {
    SystemTray tray;
    ASSERT_TRUE(tray.initialize(icon_path_, "Initial Tooltip"));

    tray.setTooltip("Updated Tooltip");
    // Note: We can't actually verify the tooltip text as Qt doesn't provide a way to read it back
}

TEST_F(SystemTrayTest, NotificationTest) {
    SystemTray tray;
    ASSERT_TRUE(tray.initialize(icon_path_, "Test Tooltip"));
    tray.setVisible(true);

    // Test different notification types
    tray.showNotification("Info", "Info message", 1);
    tray.showNotification("Warning", "Warning message", 2);
    tray.showNotification("Critical", "Critical message", 3);
    
    // Note: We can't verify if notifications were actually shown as it depends on the system
    // But we can verify that the calls don't crash
} 