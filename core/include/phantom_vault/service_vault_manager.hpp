#pragma once

#include "phantom_vault_export.h"
#include "phantom_vault/input_overlay.hpp"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <unordered_map>

namespace phantom_vault {
namespace service {

/**
 * @brief Vault profile information
 */
struct PHANTOM_VAULT_EXPORT VaultProfile {
    std::string id;                    // Profile ID
    std::string name;                  // Profile name
    std::string os_user;               // OS username
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_access;
    
    // Password verification data (hashed)
    std::string master_password_hash;  // Salt:Hash format
    std::string recovery_key_hash;     // Salt:Hash format
    std::string encrypted_recovery_key; // Encrypted with master password
};

/**
 * @brief Vault folder information
 */
struct PHANTOM_VAULT_EXPORT VaultFolder {
    std::string id;                    // Folder ID
    std::string profile_id;            // Owner profile ID
    std::string folder_name;           // Display name
    std::string original_path;         // Original location (e.g., /home/user/Desktop/MyFolder)
    std::string vault_path;            // Vault storage location (when locked)
    bool is_locked;                    // Current lock state
    bool uses_master_password;        // True if uses profile master password
    UnlockMode unlock_mode;            // Current unlock mode (if unlocked)
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_unlocked;
    
    // Custom password data (if not using master password)
    std::string custom_password_hash;     // Salt:Hash format
    std::string custom_recovery_key_hash; // Salt:Hash format
    std::string encrypted_custom_recovery_key;
    
    // Backup tracking
    struct Backup {
        std::chrono::system_clock::time_point timestamp;
        std::string path;
        std::string operation; // "pre-lock", "pre-unlock"
    };
    std::vector<Backup> backups;
};

/**
 * @brief Unlock operation result
 */
struct PHANTOM_VAULT_EXPORT UnlockResult {
    int success_count = 0;
    int failed_count = 0;
    std::vector<std::string> failed_folder_ids;
    std::vector<std::string> error_messages;
};

/**
 * @brief Service vault manager for native C++ operations
 * 
 * Provides vault management functionality for the background service,
 * integrating with existing PhantomVault core components.
 */
class PHANTOM_VAULT_EXPORT ServiceVaultManager {
public:
    /**
     * @brief Constructor
     */
    ServiceVaultManager();

    /**
     * @brief Destructor
     */
    ~ServiceVaultManager();

    /**
     * @brief Initialize the vault manager
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Get active profile for current OS user
     * @return Active profile, or nullptr if none exists
     */
    std::shared_ptr<VaultProfile> getActiveProfile();

    /**
     * @brief Create a new profile
     * @param name Profile name
     * @param master_password Master password
     * @param recovery_key Recovery key
     * @return Created profile, or nullptr if failed
     */
    std::shared_ptr<VaultProfile> createProfile(
        const std::string& name,
        const std::string& master_password,
        const std::string& recovery_key
    );

    /**
     * @brief Verify master password for profile
     * @param profile_id Profile ID
     * @param password Password to verify
     * @return true if password is correct, false otherwise
     */
    bool verifyMasterPassword(const std::string& profile_id, const std::string& password);

    /**
     * @brief Get all folders for a profile
     * @param profile_id Profile ID
     * @return Vector of folders
     */
    std::vector<VaultFolder> getFolders(const std::string& profile_id);

    /**
     * @brief Get folder by ID
     * @param profile_id Profile ID
     * @param folder_id Folder ID
     * @return Folder pointer, or nullptr if not found
     */
    std::shared_ptr<VaultFolder> getFolder(const std::string& profile_id, const std::string& folder_id);

    /**
     * @brief Unlock folders with password and mode
     * @param profile_id Profile ID
     * @param password Password (master or custom)
     * @param mode Unlock mode (temporary or permanent)
     * @return Unlock operation result
     */
    UnlockResult unlockFolders(
        const std::string& profile_id,
        const std::string& password,
        UnlockMode mode
    );

    /**
     * @brief Unlock folders with recovery key
     * @param profile_id Profile ID
     * @param recovery_key Recovery key
     * @return Unlock operation result
     */
    UnlockResult unlockWithRecoveryKey(
        const std::string& profile_id,
        const std::string& recovery_key
    );

    /**
     * @brief Lock all temporary folders
     * @param profile_id Profile ID
     * @param password Master password for locking
     * @return Number of folders locked
     */
    int lockAllTemporaryFolders(const std::string& profile_id, const std::string& password);

    /**
     * @brief Get temporarily unlocked folders
     * @param profile_id Profile ID
     * @return Vector of temporarily unlocked folders
     */
    std::vector<VaultFolder> getTemporaryUnlockedFolders(const std::string& profile_id);

    /**
     * @brief Check if there are any temporarily unlocked folders
     * @param profile_id Profile ID
     * @return true if temporary folders exist, false otherwise
     */
    bool hasTemporaryUnlockedFolders(const std::string& profile_id);

    /**
     * @brief Get vault storage base path
     * @return Base path for vault storage
     */
    std::string getVaultBasePath() const;

    /**
     * @brief Get user-specific vault path
     * @return User vault path
     */
    std::string getUserVaultPath() const;

    /**
     * @brief Get last error message
     * @return Error message string
     */
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class ServiceVaultManager

} // namespace service
} // namespace phantom_vault