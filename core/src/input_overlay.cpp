#include "phantom_vault/input_overlay.hpp"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xutil.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <regex>
#include <cctype>
#include <algorithm>
#include <chrono>

namespace phantom_vault {
namespace service {

// PasswordParser implementation
PasswordInput PasswordParser::parseInput(const std::string& raw_input) {
    PasswordInput result;
    
    if (raw_input.empty()) {
        return result;
    }
    
    std::string cleaned = cleanInput(raw_input);
    
    // Check if it's a recovery key format
    if (isValidRecoveryKey(cleaned)) {
        result.password = cleaned;
        result.is_recovery_key = true;
        result.mode = UnlockMode::TEMPORARY; // Recovery keys default to temporary
        return result;
    }
    
    // Check for T/P prefix
    if (cleaned.length() >= 2) {
        char first_char = std::tolower(cleaned[0]);
        
        if (first_char == 't') {
            // Temporary mode - T+password format
            result.password = cleaned.substr(1); // Remove T prefix
            result.mode = UnlockMode::TEMPORARY;
        } else if (first_char == 'p') {
            // Permanent mode - P+password format
            result.password = cleaned.substr(1); // Remove P prefix
            result.mode = UnlockMode::PERMANENT;
        } else {
            // No prefix - default to temporary
            result.password = cleaned;
            result.mode = UnlockMode::TEMPORARY;
        }
    } else {
        // Short input - use as-is, default to temporary
        result.password = cleaned;
        result.mode = UnlockMode::TEMPORARY;
    }
    
    return result;
}

bool PasswordParser::isValidRecoveryKey(const std::string& key) {
    // Check for XXXX-XXXX-XXXX-XXXX format
    std::regex recovery_pattern(R"([A-Fa-f0-9]{4}-[A-Fa-f0-9]{4}-[A-Fa-f0-9]{4}-[A-Fa-f0-9]{4})");
    return std::regex_match(key, recovery_pattern);
}

std::string PasswordParser::cleanInput(const std::string& input) {
    std::string cleaned = input;
    
    // Remove leading/trailing whitespace
    cleaned.erase(cleaned.begin(), std::find_if(cleaned.begin(), cleaned.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    cleaned.erase(std::find_if(cleaned.rbegin(), cleaned.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), cleaned.end());
    
    return cleaned;
}

// InputOverlay implementation
class InputOverlay::Implementation {
public:
    Implementation()
        : display_(nullptr)
        , window_(0)
        , is_active_(false)
        , should_cancel_(false)
        , last_error_()
        , mutex_()
        , condition_()
        , input_buffer_()
    {}

    ~Implementation() {
        cleanup();
    }

    bool initialize() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Open X11 display for invisible window input capture
        display_ = XOpenDisplay(nullptr);
        if (!display_) {
            last_error_ = "Failed to open X display";
            std::cout << "[InputOverlay] ERROR: " << last_error_ << std::endl;
            return false;
        }
        
        std::cout << "[InputOverlay] Initialized for invisible X11 input capture" << std::endl;
        return true;
    }

    PasswordInput capturePassword(int timeout_seconds) {
        std::cout << "[InputOverlay] Starting password capture (timeout: " << timeout_seconds << "s)" << std::endl;
        
        if (!createOverlayWindow()) {
            return PasswordInput{};
        }
        
        std::string raw_input = captureInput(timeout_seconds);
        destroyOverlayWindow();
        
        if (raw_input.empty()) {
            std::cout << "[InputOverlay] Password capture cancelled or timed out" << std::endl;
            return PasswordInput{};
        }
        
        PasswordInput result = PasswordParser::parseInput(raw_input);
        
        std::cout << "[InputOverlay] Password captured successfully" << std::endl;
        std::cout << "  Mode: " << (result.mode == UnlockMode::TEMPORARY ? "Temporary" : "Permanent") << std::endl;
        std::cout << "  Is recovery key: " << (result.is_recovery_key ? "Yes" : "No") << std::endl;
        
        return result;
    }

    std::string captureRecoveryKey(int timeout_seconds) {
        std::cout << "[InputOverlay] Starting recovery key capture (timeout: " << timeout_seconds << "s)" << std::endl;
        
        if (!createOverlayWindow()) {
            return "";
        }
        
        std::string raw_input = captureInput(timeout_seconds);
        destroyOverlayWindow();
        
        if (raw_input.empty()) {
            std::cout << "[InputOverlay] Recovery key capture cancelled or timed out" << std::endl;
            return "";
        }
        
        std::string cleaned = PasswordParser::cleanInput(raw_input);
        
        if (!PasswordParser::isValidRecoveryKey(cleaned)) {
            std::cout << "[InputOverlay] Invalid recovery key format" << std::endl;
            last_error_ = "Invalid recovery key format (expected XXXX-XXXX-XXXX-XXXX)";
            return "";
        }
        
        std::cout << "[InputOverlay] Recovery key captured successfully" << std::endl;
        return cleaned;
    }

    bool isActive() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return is_active_;
    }

    void cancel() {
        std::lock_guard<std::mutex> lock(mutex_);
        should_cancel_ = true;
        condition_.notify_all();
    }

    std::string getLastError() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return last_error_;
    }

private:
    bool createOverlayWindow() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Open X11 display if not already open
        if (!display_) {
            display_ = XOpenDisplay(nullptr);
            if (!display_) {
                last_error_ = "Failed to open X display";
                std::cout << "[InputOverlay] ERROR: " << last_error_ << std::endl;
                return false;
            }
        }
        
        // Create invisible window that captures all input
        int screen = DefaultScreen(display_);
        Window root = RootWindow(display_, screen);
        
        // Create a 1x1 invisible window positioned off-screen
        XSetWindowAttributes attrs;
        attrs.override_redirect = True;  // Bypass window manager
        attrs.background_pixel = BlackPixel(display_, screen);
        attrs.border_pixel = BlackPixel(display_, screen);
        attrs.event_mask = KeyPressMask | KeyReleaseMask | FocusChangeMask;
        
        window_ = XCreateWindow(display_, root,
                               -10, -10, 1, 1, 0,  // Off-screen, 1x1 size
                               CopyFromParent, InputOutput, CopyFromParent,
                               CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWEventMask,
                               &attrs);
        
        if (!window_) {
            last_error_ = "Failed to create overlay window";
            std::cout << "[InputOverlay] ERROR: " << last_error_ << std::endl;
            return false;
        }
        
        // Map the window (make it exist, but invisible)
        XMapWindow(display_, window_);
        
        // Set input focus to our invisible window
        XSetInputFocus(display_, window_, RevertToParent, CurrentTime);
        
        // Flush all X requests
        XFlush(display_);
        
        std::cout << "[InputOverlay] Created invisible input capture window" << std::endl;
        return true;
    }

    void destroyOverlayWindow() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (window_ && display_) {
            // Restore focus to the previously focused window
            XSetInputFocus(display_, PointerRoot, RevertToPointerRoot, CurrentTime);
            
            // Destroy our invisible window
            XDestroyWindow(display_, window_);
            XFlush(display_);
            
            std::cout << "[InputOverlay] Destroyed input capture window" << std::endl;
        }
        
        window_ = 0;
    }

    std::string captureInput(int timeout_seconds) {
        if (!window_ || !display_) {
            last_error_ = "Overlay window not created";
            return "";
        }
        
        std::cout << "[InputOverlay] Capturing invisible input (timeout: " << timeout_seconds << "s)" << std::endl;
        std::cout << "[InputOverlay] Type T+password (temporary) or P+password (permanent), then press Enter" << std::endl;
        
        input_buffer_.clear();
        is_active_ = true;
        should_cancel_ = false;
        
        // Set up timeout
        auto start_time = std::chrono::steady_clock::now();
        auto timeout_duration = std::chrono::seconds(timeout_seconds);
        
        XEvent event;
        while (is_active_ && !should_cancel_) {
            // Check for timeout
            if (std::chrono::steady_clock::now() - start_time > timeout_duration) {
                std::cout << "[InputOverlay] Input capture timed out" << std::endl;
                break;
            }
            
            // Check for X11 events with timeout
            if (XPending(display_) > 0) {
                XNextEvent(display_, &event);
                
                if (event.type == KeyPress && event.xkey.window == window_) {
                    if (processKeyPress(&event.xkey)) {
                        // Enter key pressed - input complete
                        break;
                    }
                }
            } else {
                // No events pending, sleep briefly
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        
        is_active_ = false;
        
        if (should_cancel_) {
            std::cout << "[InputOverlay] Input capture cancelled" << std::endl;
            return "";
        }
        
        if (input_buffer_.empty()) {
            std::cout << "[InputOverlay] No input captured" << std::endl;
            return "";
        }
        
        std::cout << "[InputOverlay] Input captured successfully (length: " << input_buffer_.length() << ")" << std::endl;
        return input_buffer_;
    }
    
    bool processKeyPress(XKeyEvent* key_event) {
        KeySym keysym = XkbKeycodeToKeysym(display_, key_event->keycode, 0, 0);
        
        if (keysym == XK_Return || keysym == XK_KP_Enter) {
            // Enter key - complete input
            return true;
        } else if (keysym == XK_Escape) {
            // Escape key - cancel input
            should_cancel_ = true;
            return true;
        } else if (keysym == XK_BackSpace) {
            // Backspace - remove last character
            if (!input_buffer_.empty()) {
                input_buffer_.pop_back();
            }
        } else {
            // Regular character input
            char buffer[32];
            int len = XLookupString(key_event, buffer, sizeof(buffer), &keysym, nullptr);
            
            if (len > 0) {
                // Add printable characters to input buffer
                for (int i = 0; i < len; i++) {
                    if (std::isprint(buffer[i])) {
                        input_buffer_ += buffer[i];
                    }
                }
            }
        }
        
        return false; // Continue capturing
    }

    // X11 event handling methods removed - using terminal input instead

    void cleanup() {
        if (is_active_) { 
            cancel();
        }
        
        destroyOverlayWindow();
        
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
    std::string input_buffer_;
};

// InputOverlay public interface
InputOverlay::InputOverlay() : pimpl(std::make_unique<Implementation>()) {}
InputOverlay::~InputOverlay() = default;

bool InputOverlay::initialize() {
    return pimpl->initialize();
}

PasswordInput InputOverlay::capturePassword(int timeout_seconds) {
    return pimpl->capturePassword(timeout_seconds);
}

std::string InputOverlay::captureRecoveryKey(int timeout_seconds) {
    return pimpl->captureRecoveryKey(timeout_seconds);
}

bool InputOverlay::isActive() const {
    return pimpl->isActive();
}

void InputOverlay::cancel() {
    pimpl->cancel();
}

std::string InputOverlay::getLastError() const {
    return pimpl->getLastError();
}

} // namespace service
} // namespace phantom_vault