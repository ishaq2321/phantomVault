#pragma once

#include "phantom_vault_export.h"
#include <string>
#include <memory>
#include <chrono>

namespace phantom_vault {
namespace service {

/**
 * @brief Password input modes
 */
enum class UnlockMode {
    TEMPORARY,   // T prefix - locks when system locks
    PERMANENT    // P prefix - removes from vault permanently
};

/**
 * @brief Parsed password input structure
 */
struct PHANTOM_VAULT_EXPORT PasswordInput {
    std::string password;           // The actual password
    UnlockMode mode;               // Unlock mode (T/P)
    bool is_recovery_key;          // True if this is a recovery key
    bool is_relock_mode;           // True if this is for re-locking temporary folders
    
    PasswordInput() 
        : password()
        , mode(UnlockMode::TEMPORARY)
        , is_recovery_key(false)
        , is_relock_mode(false) 
    {}
};

/**
 * @brief Invisible input overlay for password capture
 * 
 * Creates a completely transparent overlay window that captures keyboard input
 * without providing any visual feedback. Used for stealth password entry.
 */
class PHANTOM_VAULT_EXPORT InputOverlay {
public:
    /**
     * @brief Constructor
     */
    InputOverlay();

    /**
     * @brief Destructor
     */
    ~InputOverlay();

    /**
     * @brief Initialize the input overlay
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Capture password input invisibly
     * @param timeout_seconds Timeout in seconds (default 10)
     * @return Parsed password input, empty if cancelled/timeout
     */
    PasswordInput capturePassword(int timeout_seconds = 10);

    /**
     * @brief Capture recovery key input
     * @param timeout_seconds Timeout in seconds (default 30)
     * @return Recovery key string, empty if cancelled/timeout
     */
    std::string captureRecoveryKey(int timeout_seconds = 30);

    /**
     * @brief Check if overlay is currently active
     * @return true if capturing input, false otherwise
     */
    bool isActive() const;

    /**
     * @brief Cancel current input capture
     */
    void cancel();

    /**
     * @brief Get last error message
     * @return Error message string
     */
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class InputOverlay

/**
 * @brief Password input parser utilities
 */
class PHANTOM_VAULT_EXPORT PasswordParser {
public:
    /**
     * @brief Parse password input with T/P mode detection
     * @param raw_input Raw input string from user
     * @return Parsed password input structure
     */
    static PasswordInput parseInput(const std::string& raw_input);

    /**
     * @brief Validate recovery key format
     * @param key Recovery key string to validate
     * @return true if valid XXXX-XXXX-XXXX-XXXX format
     */
    static bool isValidRecoveryKey(const std::string& key);

    /**
     * @brief Clean and normalize input
     * @param input Raw input string
     * @return Cleaned input string
     */
    static std::string cleanInput(const std::string& input);
};

} // namespace service
} // namespace phantom_vault