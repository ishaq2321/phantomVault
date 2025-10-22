#pragma once

#include "phantom_vault_export.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace phantom_vault {
namespace service {

/**
 * @brief Hotkey combination structure
 */
struct PHANTOM_VAULT_EXPORT HotkeyCombo {
    std::string key;           // Main key (e.g., "v", "r")
    bool ctrl = false;         // Ctrl modifier
    bool alt = false;          // Alt modifier  
    bool shift = false;        // Shift modifier
    bool super = false;        // Super/Windows modifier
    
    std::string toString() const;
    bool matches(const std::string& key_name, unsigned int modifiers) const;
};

/**
 * @brief Global hotkey manager for PhantomVault service
 * 
 * Manages fixed global hotkeys for unlock and recovery operations.
 * Uses X11 for hotkey registration and provides invisible operation.
 */
class PHANTOM_VAULT_EXPORT HotkeyManager {
public:
    /**
     * @brief Callback type for hotkey events
     */
    using HotkeyCallback = std::function<void()>;

    /**
     * @brief Constructor
     */
    HotkeyManager();

    /**
     * @brief Destructor
     */
    ~HotkeyManager();

    /**
     * @brief Initialize the hotkey manager
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Register global hotkeys
     * @return true if registration successful, false otherwise
     */
    bool registerGlobalHotkeys();

    /**
     * @brief Unregister all hotkeys
     */
    void unregisterHotkeys();

    /**
     * @brief Set callback for unlock hotkey (Ctrl+Alt+V)
     * @param callback Function to call when unlock hotkey is pressed
     */
    void setUnlockCallback(HotkeyCallback callback);

    /**
     * @brief Set callback for recovery hotkey (Ctrl+Alt+R)
     * @param callback Function to call when recovery hotkey is pressed
     */
    void setRecoveryCallback(HotkeyCallback callback);

    /**
     * @brief Check if hotkeys are registered
     * @return true if hotkeys are active, false otherwise
     */
    bool areHotkeysRegistered() const;

    /**
     * @brief Get supported hotkey combinations
     * @return Vector of supported hotkey combinations
     */
    std::vector<HotkeyCombo> getSupportedHotkeys() const;

    /**
     * @brief Check if a hotkey combination is available
     * @param combo Hotkey combination to check
     * @return true if available, false if in use
     */
    bool isHotkeyAvailable(const HotkeyCombo& combo) const;

    /**
     * @brief Get current platform (X11, Wayland, etc.)
     * @return Platform name string
     */
    std::string getCurrentPlatform() const;

    /**
     * @brief Get last error message
     * @return Error message string
     */
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class HotkeyManager

} // namespace service
} // namespace phantom_vault