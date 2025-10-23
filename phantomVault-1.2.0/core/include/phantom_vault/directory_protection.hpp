#pragma once

#include "phantom_vault_export.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>

namespace phantom_vault {
namespace service {

/**
 * @brief Protection status for a directory
 */
enum class ProtectionStatus {
    PROTECTED,      // Directory is properly protected
    UNPROTECTED,    // Directory exists but not protected
    MISSING,        // Directory doesn't exist
    ERROR          // Error checking protection status
};

/**
 * @brief Protection method used for directory
 */
enum class ProtectionMethod {
    IMMUTABLE_ATTR, // Using chattr +i (immutable attribute)
    PERMISSIONS,    // Using file permissions (chmod)
    BOTH           // Using both methods
};

/**
 * @brief Security violation types
 */
enum class ViolationType {
    PROTECTION_REMOVED,    // Immutable attribute removed
    PERMISSIONS_CHANGED,   // File permissions changed
    DIRECTORY_DELETED,     // Directory was deleted
    UNAUTHORIZED_ACCESS,   // Unauthorized access attempt
    CONTENT_MODIFIED      // Directory contents modified
};

/**
 * @brief Security violation event
 */
struct PHANTOM_VAULT_EXPORT SecurityViolation {
    ViolationType type;
    std::string directory_path;
    std::string description;
    std::chrono::system_clock::time_point timestamp;
    std::string user_context;
    
    SecurityViolation(ViolationType t, const std::string& path, const std::string& desc)
        : type(t), directory_path(path), description(desc)
        , timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Protection result for operations
 */
struct PHANTOM_VAULT_EXPORT ProtectionResult {
    bool success = false;
    std::string error_message;
    ProtectionStatus status = ProtectionStatus::ERROR;
    ProtectionMethod method_used = ProtectionMethod::IMMUTABLE_ATTR;
    
    ProtectionResult() = default;
    ProtectionResult(bool success, const std::string& error = "")
        : success(success), error_message(error) {}
};

/**
 * @brief Directory protection manager for PhantomVault
 * 
 * Provides security hardening for vault directories including:
 * - Immutable file attributes (chattr +i)
 * - Permission-based protection
 * - Security violation monitoring
 * - Automatic protection restoration
 */
class PHANTOM_VAULT_EXPORT DirectoryProtection {
public:
    /**
     * @brief Callback type for security violations
     */
    using ViolationCallback = std::function<void(const SecurityViolation&)>;

    /**
     * @brief Constructor
     */
    DirectoryProtection();

    /**
     * @brief Destructor
     */
    ~DirectoryProtection();

    /**
     * @brief Initialize the directory protection system
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Protect a directory with immutable attributes
     * @param directory_path Path to directory to protect
     * @param method Protection method to use
     * @return Protection result
     */
    ProtectionResult protectDirectory(const std::string& directory_path, 
                                    ProtectionMethod method = ProtectionMethod::IMMUTABLE_ATTR);

    /**
     * @brief Remove protection from a directory
     * @param directory_path Path to directory to unprotect
     * @return Protection result
     */
    ProtectionResult unprotectDirectory(const std::string& directory_path);

    /**
     * @brief Check protection status of a directory
     * @param directory_path Path to directory to check
     * @return Current protection status
     */
    ProtectionStatus checkProtectionStatus(const std::string& directory_path);

    /**
     * @brief Verify and restore protection for all monitored directories
     * @return Number of directories that had protection restored
     */
    int verifyAndRestoreProtection();

    /**
     * @brief Add a directory to the monitoring list
     * @param directory_path Path to directory to monitor
     * @param method Protection method to use
     * @return true if added successfully
     */
    bool addMonitoredDirectory(const std::string& directory_path, 
                              ProtectionMethod method = ProtectionMethod::IMMUTABLE_ATTR);

    /**
     * @brief Remove a directory from monitoring
     * @param directory_path Path to directory to stop monitoring
     * @return true if removed successfully
     */
    bool removeMonitoredDirectory(const std::string& directory_path);

    /**
     * @brief Get list of all monitored directories
     * @return Vector of monitored directory paths
     */
    std::vector<std::string> getMonitoredDirectories() const;

    /**
     * @brief Set callback for security violations
     * @param callback Callback function to call on violations
     */
    void setViolationCallback(ViolationCallback callback);

    /**
     * @brief Check if immutable attributes are supported on this system
     * @return true if chattr +i is available and working
     */
    bool isImmutableAttributeSupported();

    /**
     * @brief Get detailed protection information for a directory
     * @param directory_path Path to directory
     * @return Detailed protection information
     */
    std::string getProtectionInfo(const std::string& directory_path);

    /**
     * @brief Enable or disable automatic protection restoration
     * @param enabled true to enable automatic restoration
     */
    void setAutoRestoreEnabled(bool enabled);

    /**
     * @brief Check if automatic restoration is enabled
     * @return true if auto-restore is enabled
     */
    bool isAutoRestoreEnabled() const;

    /**
     * @brief Get last error message
     * @return Error message string
     */
    std::string getLastError() const;

    /**
     * @brief Get security violation history
     * @param max_entries Maximum number of entries to return (0 = all)
     * @return Vector of security violations
     */
    std::vector<SecurityViolation> getViolationHistory(size_t max_entries = 100) const;

    /**
     * @brief Clear violation history
     */
    void clearViolationHistory();

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class DirectoryProtection

} // namespace service
} // namespace phantom_vault