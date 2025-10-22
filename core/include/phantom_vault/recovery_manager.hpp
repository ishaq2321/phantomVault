#pragma once

#include "phantom_vault_export.h"
#include <string>
#include <memory>
#include <chrono>
#include <functional>

namespace phantom_vault {
namespace service {

/**
 * @brief Recovery window display modes
 */
enum class RecoveryDisplayMode {
    SHOW_KEY,      // Display recovery key for user to copy
    INPUT_KEY      // Accept recovery key input for unlock
};

/**
 * @brief Recovery manager for emergency access
 * 
 * Handles recovery key display and input operations with secure timing.
 * Provides a 5-second window for recovery key display and 30-second input timeout.
 */
class PHANTOM_VAULT_EXPORT RecoveryManager {
public:
    /**
     * @brief Callback type for recovery operations
     */
    using RecoveryCallback = std::function<void(bool success, const std::string& message)>;

    /**
     * @brief Constructor
     */
    RecoveryManager();

    /**
     * @brief Destructor
     */
    ~RecoveryManager();

    /**
     * @brief Initialize the recovery manager
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Show recovery key window for 5 seconds
     * @param recovery_key The recovery key to display
     * @param callback Callback when window closes
     */
    void showRecoveryKey(const std::string& recovery_key, RecoveryCallback callback = nullptr);

    /**
     * @brief Show recovery input window for 30 seconds
     * @param callback Callback with input result
     */
    void showRecoveryInput(RecoveryCallback callback);

    /**
     * @brief Check if recovery window is currently active
     * @return true if window is showing, false otherwise
     */
    bool isRecoveryWindowActive() const;

    /**
     * @brief Cancel current recovery operation
     */
    void cancelRecovery();

    /**
     * @brief Get recovery key display timeout (5 seconds)
     * @return Display timeout in seconds
     */
    int getDisplayTimeout() const;

    /**
     * @brief Get recovery input timeout (30 seconds)
     * @return Input timeout in seconds
     */
    int getInputTimeout() const;

    /**
     * @brief Validate recovery key format
     * @param key Recovery key to validate
     * @return true if valid XXXX-XXXX-XXXX-XXXX format
     */
    static bool validateRecoveryKeyFormat(const std::string& key);

    /**
     * @brief Generate a new recovery key
     * @return Generated recovery key in XXXX-XXXX-XXXX-XXXX format
     */
    static std::string generateRecoveryKey();

    /**
     * @brief Get last error message
     * @return Error message string
     */
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class RecoveryManager

} // namespace service
} // namespace phantom_vault