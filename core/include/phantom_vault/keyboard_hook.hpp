#pragma once

#include "phantom_vault_export.h"
#include <functional>
#include <memory>
#include <string>

namespace phantom_vault {

/**
 * @brief Class for handling global keyboard events
 */
class PHANTOM_VAULT_EXPORT KeyboardHook {
public:
    /**
     * @brief Callback type for keyboard events
     * @param key_name The name of the key (e.g., "a", "Control_L", "Return")
     * @param is_pressed True if key is pressed, false if released
     * @param modifiers Modifier keys that were active (Shift, Control, Alt, etc.)
     */
    using KeyCallback = std::function<void(const std::string& key_name, bool is_pressed, unsigned int modifiers)>;

    /**
     * @brief Constructor
     */
    KeyboardHook();

    /**
     * @brief Destructor
     */
    ~KeyboardHook();

    /**
     * @brief Initialize the keyboard hook
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Start monitoring keyboard events
     * @param callback Function to call when keyboard events occur
     * @return true if monitoring started successfully, false otherwise
     */
    bool startMonitoring(KeyCallback callback);

    /**
     * @brief Stop monitoring keyboard events
     */
    void stopMonitoring();

    /**
     * @brief Check if monitoring is active
     * @return true if monitoring is active, false otherwise
     */
    bool isMonitoring() const;

    /**
     * @brief Get the last error message
     * @return The last error message
     */
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class KeyboardHook

} // namespace phantom_vault 