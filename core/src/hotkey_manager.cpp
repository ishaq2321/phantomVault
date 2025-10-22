#include "phantom_vault/hotkey_manager.hpp"
#include "phantom_vault/keyboard_hook.hpp"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XInput2.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <cstring>
#include <cstdlib>

namespace phantom_vault {
namespace service {

// HotkeyCombo implementation
std::string HotkeyCombo::toString() const {
    std::string result;
    
    if (ctrl) result += "Ctrl+";
    if (alt) result += "Alt+";
    if (shift) result += "Shift+";
    if (super) result += "Super+";
    
    result += key;
    return result;
}

bool HotkeyCombo::matches(const std::string& key_name, unsigned int modifiers) const {
    // Check key match (case insensitive)
    if (key_name != key && 
        key_name != std::string(1, std::toupper(key[0])) &&
        key_name != std::string(1, std::tolower(key[0]))) {
        return false;
    }
    
    // Check modifiers
    bool has_ctrl = (modifiers & ControlMask) != 0;
    bool has_alt = (modifiers & Mod1Mask) != 0;
    bool has_shift = (modifiers & ShiftMask) != 0;
    bool has_super = (modifiers & Mod4Mask) != 0;
    
    return (ctrl == has_ctrl) && 
           (alt == has_alt) && 
           (shift == has_shift) && 
           (super == has_super);
}

// HotkeyManager implementation
class HotkeyManager::Implementation {
public:
    Implementation()
        : keyboard_hook_()
        , unlock_callback_()
        , recovery_callback_()
        , hotkeys_registered_(false)
        , last_error_()
        , mutex_()
    {
        // Define fixed hotkey combinations
        unlock_hotkey_ = {
            .key = "v",
            .ctrl = true,
            .alt = true,
            .shift = false,
            .super = false
        };
        
        recovery_hotkey_ = {
            .key = "r", 
            .ctrl = true,
            .alt = true,
            .shift = false,
            .super = false
        };
    }

    ~Implementation() {
        unregisterHotkeys();
    }

    bool initialize() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Initialize keyboard hook
        keyboard_hook_ = std::make_unique<KeyboardHook>();
        if (!keyboard_hook_->initialize()) {
            last_error_ = "Failed to initialize keyboard hook: " + keyboard_hook_->getLastError();
            return false;
        }
        
        std::cout << "[HotkeyManager] Initialized successfully" << std::endl;
        return true;
    }

    bool registerGlobalHotkeys() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!keyboard_hook_) {
            last_error_ = "Keyboard hook not initialized";
            return false;
        }
        
        if (hotkeys_registered_) {
            std::cout << "[HotkeyManager] Hotkeys already registered" << std::endl;
            return true;
        }
        
        // Start monitoring with our callback
        if (!keyboard_hook_->startMonitoring([this](const std::string& key, bool pressed, unsigned int modifiers) {
            handleKeyEvent(key, pressed, modifiers);
        })) {
            last_error_ = "Failed to start keyboard monitoring: " + keyboard_hook_->getLastError();
            return false;
        }
        
        hotkeys_registered_ = true;
        
        std::cout << "[HotkeyManager] Global hotkeys registered:" << std::endl;
        std::cout << "  Unlock: " << unlock_hotkey_.toString() << std::endl;
        std::cout << "  Recovery: " << recovery_hotkey_.toString() << std::endl;
        
        return true;
    }

    void unregisterHotkeys() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (keyboard_hook_ && hotkeys_registered_) {
            keyboard_hook_->stopMonitoring();
            hotkeys_registered_ = false;
            std::cout << "[HotkeyManager] Hotkeys unregistered" << std::endl;
        }
    }

    void setUnlockCallback(HotkeyCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        unlock_callback_ = std::move(callback);
    }

    void setRecoveryCallback(HotkeyCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        recovery_callback_ = std::move(callback);
    }

    bool areHotkeysRegistered() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return hotkeys_registered_;
    }

    std::vector<HotkeyCombo> getSupportedHotkeys() const {
        return {unlock_hotkey_, recovery_hotkey_};
    }

    bool isHotkeyAvailable(const HotkeyCombo& combo) const {
        // For now, assume all combinations are available
        // In a full implementation, we would check with X11
        return true;
    }

    std::string getCurrentPlatform() const {
        // Check if we're running under X11 or Wayland
        const char* display = getenv("DISPLAY");
        const char* wayland = getenv("WAYLAND_DISPLAY");
        
        if (wayland && strlen(wayland) > 0) {
            return "Wayland";
        } else if (display && strlen(display) > 0) {
            return "X11";
        } else {
            return "Unknown";
        }
    }

    std::string getLastError() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return last_error_;
    }

private:
    void handleKeyEvent(const std::string& key, bool pressed, unsigned int modifiers) {
        if (!pressed) {
            return; // Only handle key press events
        }
        
        // Check for unlock hotkey
        if (unlock_hotkey_.matches(key, modifiers)) {
            std::cout << "[HotkeyManager] Unlock hotkey detected: " << unlock_hotkey_.toString() << std::endl;
            
            // Call unlock callback in a separate thread to avoid blocking
            if (unlock_callback_) {
                std::thread([this]() {
                    try {
                        unlock_callback_();
                    } catch (const std::exception& e) {
                        std::cerr << "[HotkeyManager] Unlock callback error: " << e.what() << std::endl;
                    }
                }).detach();
            }
            return;
        }
        
        // Check for recovery hotkey
        if (recovery_hotkey_.matches(key, modifiers)) {
            std::cout << "[HotkeyManager] Recovery hotkey detected: " << recovery_hotkey_.toString() << std::endl;
            
            // Call recovery callback in a separate thread
            if (recovery_callback_) {
                std::thread([this]() {
                    try {
                        recovery_callback_();
                    } catch (const std::exception& e) {
                        std::cerr << "[HotkeyManager] Recovery callback error: " << e.what() << std::endl;
                    }
                }).detach();
            }
            return;
        }
        
        // Debug: Log other key combinations for development
        if ((modifiers & ControlMask) && (modifiers & Mod1Mask)) {
            std::cout << "[HotkeyManager] Debug: Ctrl+Alt+" << key << " pressed" << std::endl;
        }
    }

    std::unique_ptr<KeyboardHook> keyboard_hook_;
    HotkeyCallback unlock_callback_;
    HotkeyCallback recovery_callback_;
    std::atomic<bool> hotkeys_registered_;
    std::string last_error_;
    mutable std::mutex mutex_;
    
    // Fixed hotkey combinations
    HotkeyCombo unlock_hotkey_;
    HotkeyCombo recovery_hotkey_;
};

// HotkeyManager public interface
HotkeyManager::HotkeyManager() : pimpl(std::make_unique<Implementation>()) {}
HotkeyManager::~HotkeyManager() = default;

bool HotkeyManager::initialize() {
    return pimpl->initialize();
}

bool HotkeyManager::registerGlobalHotkeys() {
    return pimpl->registerGlobalHotkeys();
}

void HotkeyManager::unregisterHotkeys() {
    pimpl->unregisterHotkeys();
}

void HotkeyManager::setUnlockCallback(HotkeyCallback callback) {
    pimpl->setUnlockCallback(std::move(callback));
}

void HotkeyManager::setRecoveryCallback(HotkeyCallback callback) {
    pimpl->setRecoveryCallback(std::move(callback));
}

bool HotkeyManager::areHotkeysRegistered() const {
    return pimpl->areHotkeysRegistered();
}

std::vector<HotkeyCombo> HotkeyManager::getSupportedHotkeys() const {
    return pimpl->getSupportedHotkeys();
}

bool HotkeyManager::isHotkeyAvailable(const HotkeyCombo& combo) const {
    return pimpl->isHotkeyAvailable(combo);
}

std::string HotkeyManager::getCurrentPlatform() const {
    return pimpl->getCurrentPlatform();
}

std::string HotkeyManager::getLastError() const {
    return pimpl->getLastError();
}

} // namespace service
} // namespace phantom_vault