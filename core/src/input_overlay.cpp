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
        
        // NO X11 DISPLAY NEEDED - completely terminal-based
        display_ = nullptr;
        
        std::cout << "[InputOverlay] Initialized for terminal input (no X11)" << std::endl;
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
        // NO WINDOW CREATION AT ALL - completely invisible operation
        std::cout << "[InputOverlay] Using invisible terminal input (no X11 interaction)" << std::endl;
        window_ = 0;  // No window needed
        return true;
    }

    void destroyOverlayWindow() {
        // No X11 resources to clean up - using terminal input
        std::cout << "[InputOverlay] Input capture cleaned up" << std::endl;
        window_ = 0;
    }

    std::string captureInput(int timeout_seconds) {
        std::cout << "\n🔐 Enter password (T+password for temporary, P+password for permanent): " << std::flush;
        
        // Use simple terminal input - no X11 interaction at all
        std::string input;
        std::getline(std::cin, input);
        
        if (input.empty()) {
            std::cout << "[InputOverlay] No input provided" << std::endl;
            return "";
        }
        
        std::cout << "[InputOverlay] Password captured (length: " << input.length() << ")" << std::endl;
        return input;
    }

    // X11 event handling methods removed - using terminal input instead

    void cleanup() {
        if (is_active_) { 
            cancel();
        }
        
        destroyOverlayWindow();
        
        // No X11 display to close - terminal-based only
        display_ = nullptr;
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