#include "phantom_vault/recovery_manager.hpp"
#include "phantom_vault/input_overlay.hpp"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <regex>
#include <random>
#include <iomanip>
#include <sstream>

namespace phantom_vault {
namespace service {

class RecoveryManager::Implementation {
public:
    Implementation()
        : display_(nullptr)
        , window_(0)
        , is_active_(false)
        , should_cancel_(false)
        , last_error_()
        , mutex_()
        , condition_()
        , recovery_thread_()
    {}

    ~Implementation() {
        cleanup();
    }

    bool initialize() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Open X11 display
        display_ = XOpenDisplay(nullptr);
        if (!display_) {
            last_error_ = "Failed to open X11 display for recovery manager";
            return false;
        }
        
        std::cout << "[RecoveryManager] Initialized with X11 display" << std::endl;
        return true;
    }

    void showRecoveryKey(const std::string& recovery_key, RecoveryCallback callback) {
        std::cout << "[RecoveryManager] Showing recovery key window (5 seconds)" << std::endl;
        
        if (is_active_) {
            std::cout << "[RecoveryManager] Recovery window already active" << std::endl;
            if (callback) callback(false, "Recovery window already active");
            return;
        }
        
        // Start recovery display thread
        recovery_thread_ = std::thread([this, recovery_key, callback]() {
            displayRecoveryKeyWindow(recovery_key, callback);
        });
        recovery_thread_.detach();
    }

    void showRecoveryInput(RecoveryCallback callback) {
        std::cout << "[RecoveryManager] Showing recovery input window (30 seconds)" << std::endl;
        
        if (is_active_) {
            std::cout << "[RecoveryManager] Recovery window already active" << std::endl;
            if (callback) callback(false, "Recovery window already active");
            return;
        }
        
        // Start recovery input thread
        recovery_thread_ = std::thread([this, callback]() {
            inputRecoveryKeyWindow(callback);
        });
        recovery_thread_.detach();
    }

    bool isRecoveryWindowActive() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return is_active_;
    }

    void cancelRecovery() {
        std::lock_guard<std::mutex> lock(mutex_);
        should_cancel_ = true;
        condition_.notify_all();
    }

    int getDisplayTimeout() const {
        return 5; // 5 seconds for recovery key display
    }

    int getInputTimeout() const {
        return 30; // 30 seconds for recovery key input
    }

    std::string getLastError() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return last_error_;
    }

private:
    void displayRecoveryKeyWindow(const std::string& recovery_key, RecoveryCallback callback) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            is_active_ = true;
            should_cancel_ = false;
        }
        
        if (!createRecoveryWindow("PhantomVault Recovery Key", 600, 200)) {
            if (callback) callback(false, "Failed to create recovery window");
            {
                std::lock_guard<std::mutex> lock(mutex_);
                is_active_ = false;
            }
            return;
        }
        
        // Display recovery key for 5 seconds
        displayRecoveryKeyContent(recovery_key);
        
        // Wait for timeout or cancellation
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait_for(lock, std::chrono::seconds(5), [this]() {
                return should_cancel_.load();
            });
        }
        
        destroyRecoveryWindow();
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            is_active_ = false;
        }
        
        std::cout << "[RecoveryManager] Recovery key window closed" << std::endl;
        if (callback) callback(true, "Recovery key displayed successfully");
    }

    void inputRecoveryKeyWindow(RecoveryCallback callback) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            is_active_ = true;
            should_cancel_ = false;
        }
        
        if (!createRecoveryWindow("PhantomVault Recovery Input", 500, 150)) {
            if (callback) callback(false, "Failed to create recovery input window");
            {
                std::lock_guard<std::mutex> lock(mutex_);
                is_active_ = false;
            }
            return;
        }
        
        // Capture recovery key input
        std::string input = captureRecoveryInput();
        
        destroyRecoveryWindow();
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            is_active_ = false;
        }
        
        if (input.empty()) {
            std::cout << "[RecoveryManager] Recovery input cancelled or timed out" << std::endl;
            if (callback) callback(false, "Recovery input cancelled");
        } else if (validateRecoveryKeyFormat(input)) {
            std::cout << "[RecoveryManager] Valid recovery key entered" << std::endl;
            if (callback) callback(true, input);
        } else {
            std::cout << "[RecoveryManager] Invalid recovery key format" << std::endl;
            if (callback) callback(false, "Invalid recovery key format");
        }
    }

    bool createRecoveryWindow(const std::string& title, int width, int height) {
        if (!display_) {
            return false;
        }
        
        int screen = DefaultScreen(display_);
        Window root = RootWindow(display_, screen);
        
        // Center window on screen
        int screen_width = DisplayWidth(display_, screen);
        int screen_height = DisplayHeight(display_, screen);
        int x = (screen_width - width) / 2;
        int y = (screen_height - height) / 2;
        
        // Create window
        XSetWindowAttributes attrs;
        attrs.background_pixel = WhitePixel(display_, screen);
        attrs.border_pixel = BlackPixel(display_, screen);
        attrs.event_mask = ExposureMask | KeyPressMask | ButtonPressMask;
        
        window_ = XCreateWindow(
            display_, root,
            x, y, width, height,
            2, // Border width
            CopyFromParent,
            InputOutput,
            CopyFromParent,
            CWBackPixel | CWBorderPixel | CWEventMask,
            &attrs
        );
        
        if (!window_) {
            return false;
        }
        
        // Set window title
        XStoreName(display_, window_, title.c_str());
        
        // Set window properties
        XSizeHints* size_hints = XAllocSizeHints();
        if (size_hints) {
            size_hints->flags = PMinSize | PMaxSize;
            size_hints->min_width = size_hints->max_width = width;
            size_hints->min_height = size_hints->max_height = height;
            XSetWMNormalHints(display_, window_, size_hints);
            XFree(size_hints);
        }
        
        // Make window stay on top (simplified)
        // Note: Advanced window manager hints removed for compatibility
        
        // Map window
        XMapWindow(display_, window_);
        XRaiseWindow(display_, window_);
        XFlush(display_);
        
        return true;
    }

    void destroyRecoveryWindow() {
        if (display_ && window_) {
            XDestroyWindow(display_, window_);
            XFlush(display_);
            window_ = 0;
        }
    }

    void displayRecoveryKeyContent(const std::string& recovery_key) {
        if (!display_ || !window_) {
            return;
        }
        
        // Wait for window to be mapped
        XEvent event;
        while (true) {
            XNextEvent(display_, &event);
            if (event.type == Expose) {
                break;
            }
        }
        
        // Get graphics context
        GC gc = XCreateGC(display_, window_, 0, nullptr);
        
        // Set font and colors
        XSetForeground(display_, gc, BlackPixel(display_, DefaultScreen(display_)));
        XSetBackground(display_, gc, WhitePixel(display_, DefaultScreen(display_)));
        
        // Draw title
        std::string title = "Your Recovery Key:";
        XDrawString(display_, window_, gc, 50, 50, title.c_str(), title.length());
        
        // Draw recovery key in larger font
        XDrawString(display_, window_, gc, 50, 100, recovery_key.c_str(), recovery_key.length());
        
        // Draw instructions
        std::string instruction = "Copy this key and store it safely!";
        XDrawString(display_, window_, gc, 50, 130, instruction.c_str(), instruction.length());
        
        std::string timeout_msg = "Window will close in 5 seconds...";
        XDrawString(display_, window_, gc, 50, 160, timeout_msg.c_str(), timeout_msg.length());
        
        XFreeGC(display_, gc);
        XFlush(display_);
    }

    std::string captureRecoveryInput() {
        if (!display_ || !window_) {
            return "";
        }
        
        // Wait for window to be mapped and draw initial content
        XEvent event;
        while (true) {
            XNextEvent(display_, &event);
            if (event.type == Expose) {
                drawInputWindow("");
                break;
            }
        }
        
        std::string input;
        auto start_time = std::chrono::steady_clock::now();
        
        while (true) {
            // Check for timeout
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
            if (elapsed.count() >= 30) {
                break; // Timeout
            }
            
            // Check for cancellation
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (should_cancel_) {
                    break;
                }
            }
            
            // Process events
            if (XPending(display_) > 0) {
                XNextEvent(display_, &event);
                
                if (event.type == KeyPress) {
                    KeySym keysym;
                    char buffer[32];
                    int len = XLookupString(&event.xkey, buffer, sizeof(buffer) - 1, &keysym, nullptr);
                    buffer[len] = '\0';
                    
                    if (keysym == XK_Return || keysym == XK_KP_Enter) {
                        // Enter pressed - complete input
                        break;
                    } else if (keysym == XK_Escape) {
                        // Escape pressed - cancel
                        input.clear();
                        break;
                    } else if (keysym == XK_BackSpace) {
                        // Backspace - remove last character
                        if (!input.empty()) {
                            input.pop_back();
                            drawInputWindow(input);
                        }
                    } else if (len > 0 && (std::isalnum(buffer[0]) || buffer[0] == '-')) {
                        // Add valid characters (alphanumeric and dash)
                        input += buffer[0];
                        drawInputWindow(input);
                    }
                }
            } else {
                // Small sleep to avoid busy waiting
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        
        return input;
    }

    void drawInputWindow(const std::string& current_input) {
        if (!display_ || !window_) {
            return;
        }
        
        // Clear window
        XClearWindow(display_, window_);
        
        // Get graphics context
        GC gc = XCreateGC(display_, window_, 0, nullptr);
        XSetForeground(display_, gc, BlackPixel(display_, DefaultScreen(display_)));
        
        // Draw title
        std::string title = "Enter Recovery Key:";
        XDrawString(display_, window_, gc, 50, 40, title.c_str(), title.length());
        
        // Draw input field
        std::string display_input = current_input;
        if (display_input.length() > 20) {
            display_input = display_input.substr(0, 20) + "...";
        }
        
        XDrawString(display_, window_, gc, 50, 80, display_input.c_str(), display_input.length());
        
        // Draw cursor
        int cursor_x = 50 + display_input.length() * 8; // Approximate character width
        XDrawLine(display_, window_, gc, cursor_x, 65, cursor_x, 85);
        
        // Draw instructions
        std::string instruction = "Format: XXXX-XXXX-XXXX-XXXX";
        XDrawString(display_, window_, gc, 50, 110, instruction.c_str(), instruction.length());
        
        std::string controls = "Press Enter to confirm, Esc to cancel";
        XDrawString(display_, window_, gc, 50, 130, controls.c_str(), controls.length());
        
        XFreeGC(display_, gc);
        XFlush(display_);
    }

    void cleanup() {
        cancelRecovery();
        
        if (recovery_thread_.joinable()) {
            recovery_thread_.join();
        }
        
        destroyRecoveryWindow();
        
        if (display_) {
            XCloseDisplay(display_);
            display_ = nullptr;
        }
    }

    Display* display_;
    Window window_;
    std::atomic<bool> is_active_;
    std::atomic<bool> should_cancel_;
    std::string last_error_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::thread recovery_thread_;
};

// Static utility functions
bool RecoveryManager::validateRecoveryKeyFormat(const std::string& key) {
    // Check for XXXX-XXXX-XXXX-XXXX format
    std::regex recovery_pattern(R"([A-Fa-f0-9]{4}-[A-Fa-f0-9]{4}-[A-Fa-f0-9]{4}-[A-Fa-f0-9]{4})");
    return std::regex_match(key, recovery_pattern);
}

std::string RecoveryManager::generateRecoveryKey() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    for (int segment = 0; segment < 4; ++segment) {
        if (segment > 0) ss << "-";
        for (int i = 0; i < 4; ++i) {
            ss << std::hex << std::uppercase << dis(gen);
        }
    }
    
    return ss.str();
}

// RecoveryManager public interface
RecoveryManager::RecoveryManager() : pimpl(std::make_unique<Implementation>()) {}
RecoveryManager::~RecoveryManager() = default;

bool RecoveryManager::initialize() {
    return pimpl->initialize();
}

void RecoveryManager::showRecoveryKey(const std::string& recovery_key, RecoveryCallback callback) {
    pimpl->showRecoveryKey(recovery_key, callback);
}

void RecoveryManager::showRecoveryInput(RecoveryCallback callback) {
    pimpl->showRecoveryInput(callback);
}

bool RecoveryManager::isRecoveryWindowActive() const {
    return pimpl->isRecoveryWindowActive();
}

void RecoveryManager::cancelRecovery() {
    pimpl->cancelRecovery();
}

int RecoveryManager::getDisplayTimeout() const {
    return pimpl->getDisplayTimeout();
}

int RecoveryManager::getInputTimeout() const {
    return pimpl->getInputTimeout();
}

std::string RecoveryManager::getLastError() const {
    return pimpl->getLastError();
}

} // namespace service
} // namespace phantom_vault