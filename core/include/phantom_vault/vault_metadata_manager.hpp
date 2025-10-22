#pragma once

#include "phantom_vault_export.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <optional>

namespace phantom_vault {
namespace service {

/**
 * @brief Backup entry structure for folder metadata
 */
struct PHANTOM_VAULT_EXPORT BackupEntry {
    int64_t timestamp;
    std::string path;
    std::string operation;  // "pre-lock", "pre-unlock", etc.
    
    BackupEntry() : timestamp(0) {}
    BackupEntry(int64_t ts, const std::string& p, const std::string& op)
        : timestamp(ts), path(p), operation(op) {}
};

/**
 * @brief Folder metadata structure compatible with VaultFolderManager.js
 */
struct PHANTOM_VAULT_EXPORT FolderMetadata {
    std::string id;                    // "vault_1728388800000_abc123"
    std::string folderPath;            // Current path (original when unlocked, vault when locked)
    std::string folderName;            // "MyFolder"
    bool isLocked;                     // true/false
    bool usesMasterPassword;           // true/false
    int64_t createdAt;                 // Unix timestamp
    std::optional<std::string> unlockMode; // "temporary", "permanent", or null
    
    // Phase 4.2 vault storage fields
    std::string originalPath;          // "/home/user/Desktop/MyFolder"
    std::optional<std::string> vaultPath; // "~/.phantom_vault_storage/.../vaults/..." or null
    std::vector<BackupEntry> backups;  // Backup history
    
    // Custom password fields (optional)
    std::optional<std::string> customPasswordHash;      // "salt:hash"
    std::optional<std::string> customRecoveryKeyHash;   // "salt:hash"
    std::optional<std::string> encryptedCustomRecoveryKey; // "iv:encrypted"
    
    FolderMetadata() : isLocked(false), usesMasterPassword(true), createdAt(0) {}
};

/**
 * @brief Profile metadata structure compatible with VaultProfileManager.js
 */
struct PHANTOM_VAULT_EXPORT ProfileMetadata {
    std::string id;                    // "profile_1728388800000_def456"
    std::string name;                  // "User Profile"
    std::string hashedPassword;        // PBKDF2 hash
    std::string encryptedRecoveryKey;  // Encrypted recovery key
    int64_t createdAt;                 // Unix timestamp
    
    ProfileMetadata() : createdAt(0) {}
};

/**
 * @brief Folders metadata container structure
 */
struct PHANTOM_VAULT_EXPORT FoldersMetadata {
    std::string profileId;
    std::vector<FolderMetadata> folders;
    int64_t lastModified;
    std::optional<std::string> hmac;   // HMAC for integrity protection
    
    FoldersMetadata() : lastModified(0) {}
};

/**
 * @brief Profiles metadata container structure
 */
struct PHANTOM_VAULT_EXPORT ProfilesMetadata {
    std::vector<ProfileMetadata> profiles;
    std::string activeProfileId;
    int64_t lastModified;
    std::optional<std::string> hmac;   // HMAC for integrity protection
    
    ProfilesMetadata() : lastModified(0) {}
};

/**
 * @brief Vault metadata manager for native service
 * 
 * Provides read/write access to the same JSON metadata files used by the Electron app,
 * maintaining full compatibility with the existing VaultFolderManager and VaultProfileManager.
 */
class PHANTOM_VAULT_EXPORT VaultMetadataManager {
public:
    /**
     * @brief Constructor
     */
    VaultMetadataManager();

    /**
     * @brief Destructor
     */
    ~VaultMetadataManager();

    /**
     * @brief Initialize the metadata manager for a specific user
     * @param username OS username for vault storage path
     * @return true if initialization successful, false otherwise
     */
    bool initialize(const std::string& username);

    /**
     * @brief Load folders metadata for a profile
     * @param profileId Profile identifier
     * @return Folders metadata structure, empty if not found
     */
    FoldersMetadata loadFoldersMetadata(const std::string& profileId);

    /**
     * @brief Save folders metadata for a profile
     * @param profileId Profile identifier
     * @param metadata Folders metadata to save
     * @return true if save successful, false otherwise
     */
    bool saveFoldersMetadata(const std::string& profileId, const FoldersMetadata& metadata);

    /**
     * @brief Load profiles metadata
     * @return Profiles metadata structure, empty if not found
     */
    ProfilesMetadata loadProfilesMetadata();

    /**
     * @brief Save profiles metadata
     * @param metadata Profiles metadata to save
     * @return true if save successful, false otherwise
     */
    bool saveProfilesMetadata(const ProfilesMetadata& metadata);

    /**
     * @brief Get all folders for a profile
     * @param profileId Profile identifier
     * @return Vector of folder metadata
     */
    std::vector<FolderMetadata> getFolders(const std::string& profileId);

    /**
     * @brief Get specific folder by ID
     * @param profileId Profile identifier
     * @param folderId Folder identifier
     * @return Folder metadata if found, empty optional otherwise
     */
    std::optional<FolderMetadata> getFolder(const std::string& profileId, const std::string& folderId);

    /**
     * @brief Update folder lock state and paths
     * @param profileId Profile identifier
     * @param folderId Folder identifier
     * @param isLocked New lock state
     * @param vaultPath Vault path when locked (optional)
     * @param unlockMode Unlock mode when unlocked (optional)
     * @return true if update successful, false otherwise
     */
    bool updateFolderState(const std::string& profileId, const std::string& folderId, 
                          bool isLocked, const std::optional<std::string>& vaultPath = std::nullopt,
                          const std::optional<std::string>& unlockMode = std::nullopt);

    /**
     * @brief Add backup entry to folder metadata
     * @param profileId Profile identifier
     * @param folderId Folder identifier
     * @param backupPath Path to backup
     * @param operation Operation type ("pre-lock", "pre-unlock", etc.)
     * @return true if backup entry added successfully, false otherwise
     */
    bool addBackupEntry(const std::string& profileId, const std::string& folderId,
                       const std::string& backupPath, const std::string& operation);

    /**
     * @brief Get profile by ID
     * @param profileId Profile identifier
     * @return Profile metadata if found, empty optional otherwise
     */
    std::optional<ProfileMetadata> getProfile(const std::string& profileId);

    /**
     * @brief Get active profile
     * @return Active profile metadata if found, empty optional otherwise
     */
    std::optional<ProfileMetadata> getActiveProfile();

    /**
     * @brief Validate metadata integrity using HMAC
     * @param profileId Profile identifier (for folders metadata)
     * @return true if integrity is valid, false otherwise
     */
    bool validateMetadataIntegrity(const std::string& profileId = "");

    /**
     * @brief Get current username
     * @return Username string
     */
    std::string getUsername() const;

    /**
     * @brief Get vault storage base path
     * @return Base path string
     */
    std::string getVaultStoragePath() const;

    /**
     * @brief Get last error message
     * @return Error message string
     */
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class VaultMetadataManager

} // namespace service
} // namespace phantom_vault