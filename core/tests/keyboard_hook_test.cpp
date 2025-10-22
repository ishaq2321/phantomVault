#include <gtest/gtest.h>
#include "phantom_vault/keyboard_hook.hpp"
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysymdef.h>
#include <X11/XKBlib.h>
#include <X11/Xutil.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <mutex>

using namespace phantom_vault;
using namespace std::chrono_literals;

class KeyboardHookTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Open display for sending test events
        display_ = XOpenDisplay(nullptr);
        ASSERT_TRUE(display_ != nullptr) << "Failed to open X display";
    }

    void TearDown() override {
        if (display_) {
            XCloseDisplay(display_);
        }
    }

    void simulateKeyEvent(KeySym keysym, bool press) {
        KeyCode keycode = XKeysymToKeycode(display_, keysym);
        XTestFakeKeyEvent(display_, keycode, press, CurrentTime);
        XFlush(display_);
        std::this_thread::sleep_for(50ms);  // Give time for event processing
    }

    Display* display_;
};

TEST_F(KeyboardHookTest, InitializeTest) {
    KeyboardHook hook;
    EXPECT_TRUE(hook.initialize());
}

TEST_F(KeyboardHookTest, StartStopTest) {
    KeyboardHook hook;
    ASSERT_TRUE(hook.initialize());

    bool callback_called = false;
    auto callback = [&](const std::string&, bool, unsigned int) {
        callback_called = true;
    };

    EXPECT_TRUE(hook.startMonitoring(callback));
    EXPECT_TRUE(hook.isMonitoring());
    
    hook.stopMonitoring();
    EXPECT_FALSE(hook.isMonitoring());
}

TEST_F(KeyboardHookTest, KeyPressTest) {
    KeyboardHook hook;
    ASSERT_TRUE(hook.initialize());

    std::mutex mtx;
    std::vector<std::tuple<std::string, bool, unsigned int>> events;
    auto callback = [&](const std::string& key_name, bool is_pressed, unsigned int modifiers) {
        std::lock_guard<std::mutex> lock(mtx);
        events.emplace_back(key_name, is_pressed, modifiers);
    };

    ASSERT_TRUE(hook.startMonitoring(callback));

    // Simulate 'a' key press and release
    simulateKeyEvent(XStringToKeysym("a"), true);
    simulateKeyEvent(XStringToKeysym("a"), false);

    // Wait for events to be processed
    std::this_thread::sleep_for(100ms);

    hook.stopMonitoring();

    // Verify events
    ASSERT_EQ(events.size(), 2);
    
    // Check press event
    auto [key1, pressed1, mods1] = events[0];
    EXPECT_EQ(key1, "a");
    EXPECT_TRUE(pressed1);
    EXPECT_EQ(mods1, 0);

    // Check release event
    auto [key2, pressed2, mods2] = events[1];
    EXPECT_EQ(key2, "a");
    EXPECT_FALSE(pressed2);
    EXPECT_EQ(mods2, 0);
}

TEST_F(KeyboardHookTest, ModifierTest) {
    KeyboardHook hook;
    ASSERT_TRUE(hook.initialize());

    std::mutex mtx;
    std::vector<std::tuple<std::string, bool, unsigned int>> events;
    auto callback = [&](const std::string& key_name, bool is_pressed, unsigned int modifiers) {
        std::lock_guard<std::mutex> lock(mtx);
        events.emplace_back(key_name, is_pressed, modifiers);
    };

    ASSERT_TRUE(hook.startMonitoring(callback));

    // Simulate Ctrl+A
    simulateKeyEvent(XStringToKeysym("Control_L"), true);
    simulateKeyEvent(XStringToKeysym("a"), true);
    simulateKeyEvent(XStringToKeysym("a"), false);
    simulateKeyEvent(XStringToKeysym("Control_L"), false);

    // Wait for events to be processed
    std::this_thread::sleep_for(100ms);

    hook.stopMonitoring();

    // Find the Ctrl+A press event
    bool found_ctrl_a = false;
    for (const auto& [key, pressed, mods] : events) {
        if (key == "a" && pressed && (mods & ControlMask)) {
            found_ctrl_a = true;
            break;
        }
    }

    EXPECT_TRUE(found_ctrl_a) << "Failed to detect Ctrl+A combination";
}

TEST_F(KeyboardHookTest, MultipleStartTest) {
    KeyboardHook hook;
    ASSERT_TRUE(hook.initialize());

    auto callback = [](const std::string&, bool, unsigned int) {};

    EXPECT_TRUE(hook.startMonitoring(callback));
    EXPECT_FALSE(hook.startMonitoring(callback)) << "Should not allow multiple start calls";
    
    hook.stopMonitoring();
} 