#pragma once

#include "phantom_vault_export.h"
#include "phantom_vault/input_overlay.hpp"
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <chrono>
#include <map>

namespace phantom_vault {
namespace service {

/**
 * @brief Password detection result
 */
struct PHANTOM_VAULT_EXPORT PasswordDetectionResult {
    bool found = false;
    std::string password;
    UnlockMode mode = UnlockMode::TEMPORARY;
    std::string folder_id;  // Which folder this password belongs to
    
    PasswordDetectionResult() = default;
    PasswordDetectionResult(bool f, const std::string& pwd, UnlockMode m, const std::string& fid = "")
        : found(f), password(pwd), mode(m), folder_id(fid) {}
};

/**
 * @brief Folder password information
 */
struct PHANTOM_VAULT_EXPORT FolderPassword {
    std::string folder_id;
    std::string folder_name;
    std::string password_hash;  // For security, we store hashes
    std::string original_path;
    bool is_locked;
    
    FolderPassword() = default;
    FolderPassword(const std::string& id, const std::string& name, const std::string& hash, 
                   const std::string& path, bool locked)
        : folder_id(id), folder_name(name), password_hash(hash), original_path(path), is_locked(locked) {}
};

/**
 * @brief Keyboard sequence detector for invisible password input
 * 
 * Captures keystrokes for a limited time after Ctrl+Alt+V is pressed,
 * looking for password patterns (T+password, P+password, or just password).
 * Provides truly invisible operation by detecting passwords within normal typing.
 */
class PHANTOM_VAULT_EXPORT SequenceDetector {
public:
    /**
     * @brief Callback type for password detection
     */
    using DetectionCallback = std::function<void(const PasswordDetectionResult&)>;

    /**
     * @brief Constructor
     */
    SequenceDetector();

    /**
     * @brief Destructor
     */
    ~SequenceDetector();

    /**
     * @brief Initialize the sequence detector
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Start keyboard sequence detection
     * @param timeout_seconds How long to monitor keystrokes (default 10 seconds)
     * @return true if detection started successfully
     */
    bool startDetection(int timeout_seconds = 10);

    /**
     * @brief Stop keyboard sequence detection
     */
    void stopDetection();

    /**
     * @brief Check if detection is currently active
     * @return true if actively monitoring keystrokes
     */
    bool isActive() const;

    /**
     * @brief Set callback for password detection
     * @param callback Function to call when password is detected
     */
    void setDetectionCallback(DetectionCallback callback);

    /**
     * @brief Update folder passwords for detection
     * @param folders Vector of folder password information
     */
    void updateFolderPasswords(const std::vector<FolderPassword>& folders);

    /**
     * @brief Add a single folder password
     * @param folder Folder password information
     */
    void addFolderPassword(const FolderPassword& folder);

    /**
     * @brief Remove folder password
     * @param folder_id Folder ID to remove
     */
    void removeFolderPassword(const std::string& folder_id);

    /**
     * @brief Clear all folder passwords
     */
    void clearFolderPasswords();

    /**
     * @brief Get detection statistics
     * @return Statistics as JSON string
     */
    std::string getStats() const;

    /**
     * @brief Get last error message
     * @return Error message string
     */
    std::string getLastError() const;

    /**
     * @brief Process a keystroke (called by keyboard hook)
     * @param key_char Character that was typed
     */
    void processKeystroke(char key_char);

    /**
     * @brief Set maximum buffer size (for security)
     * @param max_size Maximum number of characters to keep in buffer
     */
    void setMaxBufferSize(size_t max_size);

    /**
     * @brief Enable/disable case-sensitive matching
     * @param case_sensitive Whether to match case exactly
     */
    void setCaseSensitive(bool case_sensitive);

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class SequenceDetector

/**
 * @brief Utility functions for password hashing and validation
 */
class PHANTOM_VAULT_EXPORT PasswordUtils {
public:
    /**
     * @brief Hash a password for secure storage
     * @param password Plain text password
     * @return SHA-256 hash of the password
     */
    static std::string hashPassword(const std::string& password);

    /**
     * @brief Verify a password against a hash
     * @param password Plain text password to verify
     * @param hash Stored password hash
     * @return true if password matches hash
     */
    static bool verifyPassword(const std::string& password, const std::string& hash);

    /**
     * @brief Extract mode prefix from keystroke sequence
     * @param sequence Keystroke sequence to analyze
     * @param password Password to look for
     * @return Detected unlock mode
     */
    static UnlockMode extractMode(const std::string& sequence, const std::string& password);

    /**
     * @brief Secure memory wipe (DOD 5220.22-M standard)
     * @param data Pointer to data to wipe
     * @param size Size of data in bytes
     */
    static void secureWipe(void* data, size_t size);
};

} // namespace service
} // namespace phantom_vault