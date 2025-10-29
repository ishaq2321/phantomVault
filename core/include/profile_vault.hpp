#pragma once

#include "encryption_engine.hpp"
#include "error_handler.hpp"
#include "vault_handler.hpp"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <filesystem>

namespace PhantomVault {

/**
 * @brief Unlock modes for encrypted folders
 */
enum class UnlockMode {
    TEMPORARY,  // Auto-lock on system events (reboot, lock, manual re-lock)
    PERMANENT   // Remove from vault tracking permanently
};

/**
 * @brief Information about a locked folder
 */
struct LockedFolderInfo {
    std::string original_path;
    std::string vault_location;
    std::chrono::system_clock::time_point lock_timestamp;
    size_t file_count;
    size_t total_size;
    bool is_temporarily_unlocked;
    
    LockedFolderInfo() : file_count(0), total_size(0), is_temporarily_unlocked(false) {}
};

/**
 * @brief Result of vault operations
 */
struct VaultOperationResult {
    bool success;
    std::string message;
    std::string error_details;
    std::vector<std::string> processed_files;
    
    VaultOperationResult() : success(false) {}
};

/**
 * @brief Profile-specific encrypted vault for secure folder storage
 * 
 * This class manages encrypted storage for a single profile, providing:
 * - Complete folder encryption and hiding
 * - Profile isolation (no cross-profile access)
 * - Temporary and permanent unlock modes
 * - Metadata preservation and integrity verification
 * - Secure cleanup and recovery mechanisms
 */
class ProfileVault {
public:
    ProfileVault(const std::string& profile_id, const std::string& vault_root_path);
    ~ProfileVault();

    // Initialization and setup
    bool initialize();
    bool createVaultStructure();
    
    // Folder locking operations
    VaultOperationResult lockFolder(const std::string& folder_path, const std::string& master_key);
    VaultOperationResult unlockFolder(const std::string& folder_path, const std::string& master_key, UnlockMode mode);
    
    // Folder management
    std::vector<LockedFolderInfo> getLockedFolders() const;
    std::optional<LockedFolderInfo> getFolderInfo(const std::string& folder_path) const;
    bool isFolderLocked(const std::string& folder_path) const;
    bool isFolderTemporarilyUnlocked(const std::string& folder_path) const;
    
    // Authentication and security
    bool isValidMasterKey(const std::string& master_key) const;
    bool validateVaultIntegrity() const;
    
    // Temporary unlock management
    VaultOperationResult relockTemporaryFolders();
    std::vector<std::string> getTemporarilyUnlockedFolders() const;
    
    // Vault maintenance
    bool cleanupCorruptedEntries();
    size_t getVaultSize() const;
    std::string getVaultPath() const { return vault_path_; }
    
    // Error handling
    std::string getLastError() const { return last_error_; }

private:
    std::string profile_id_;
    std::string vault_root_path_;
    std::string vault_path_;
    std::string metadata_file_;
    std::string temp_unlock_file_;
    mutable std::string last_error_;
    
    std::unique_ptr<EncryptionEngine> encryption_engine_;
    std::unique_ptr<phantomvault::ErrorHandler> error_handler_;
    std::unique_ptr<phantomvault::VaultHandler> vault_handler_;
    
    // Internal folder operations
    VaultOperationResult encryptAndStoreFolder(const std::string& folder_path, const std::string& master_key);
    VaultOperationResult decryptAndRestoreFolder(const std::string& vault_location, 
                                                const std::string& original_path, 
                                                const std::string& master_key,
                                                UnlockMode mode);
    
    // File processing
    bool encryptFile(const std::string& file_path, const std::string& vault_file_path, const std::string& master_key);
    bool decryptFile(const std::string& vault_file_path, const std::string& output_path, const std::string& master_key);
    
    // Metadata management
    bool saveVaultMetadata();
    bool loadVaultMetadata();
    bool saveFolderMetadata(const std::string& vault_location, const LockedFolderInfo& info);
    std::optional<LockedFolderInfo> loadFolderMetadata(const std::string& vault_location) const;
    
    // Temporary unlock tracking
    bool saveTemporaryUnlockState();
    bool loadTemporaryUnlockState();
    bool clearTemporaryUnlockState();
    
    // Path utilities
    std::string generateVaultLocation(const std::string& folder_path) const;
    std::string getVaultFolderPath(const std::string& vault_location) const;
    std::string getFolderMetadataPath(const std::string& vault_location) const;
    bool isPathSecure(const std::string& path) const;
    
    // Security utilities
    std::string hashFolderPath(const std::string& folder_path) const;
    bool verifyFolderIntegrity(const std::string& vault_location) const;
    
    // File system operations
    bool hideOriginalFolder(const std::string& folder_path);
    bool restoreOriginalFolder(const std::string& folder_path, const std::string& vault_location);
    bool secureDeleteFolder(const std::string& folder_path);
    
    // Error handling
    void setError(const std::string& error) const;
    void clearError() const;
    
    // Vault data structures
    struct VaultMetadata {
        std::string profile_id;
        std::string vault_version;
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point last_modified;
        std::vector<std::string> locked_folders;
        size_t total_folders;
        size_t total_files;
        
        VaultMetadata() : total_folders(0), total_files(0) {}
    };
    
    struct TemporaryUnlockState {
        std::vector<std::string> unlocked_folders;
        std::chrono::system_clock::time_point unlock_timestamp;
        
        TemporaryUnlockState() = default;
    };
    
    VaultMetadata vault_metadata_;
    TemporaryUnlockState temp_unlock_state_;
};

/**
 * @brief Vault manager for handling multiple profile vaults
 * 
 * This class coordinates vault operations across multiple profiles and provides
 * system-wide vault management functionality.
 */
class VaultManager {
public:
    VaultManager(const std::string& vault_root_path);
    ~VaultManager();
    
    // Vault lifecycle
    bool initializeVaultSystem();
    std::unique_ptr<ProfileVault> getProfileVault(const std::string& profile_id);
    bool createProfileVault(const std::string& profile_id);
    bool deleteProfileVault(const std::string& profile_id, const std::string& master_key);
    
    // System-wide operations
    std::vector<std::string> getAllProfileVaults() const;
    bool relockAllTemporaryFolders();
    size_t getTotalVaultSize() const;
    
    // Maintenance
    bool performVaultMaintenance();
    bool validateAllVaults() const;
    
    // Error handling
    std::string getLastError() const { return last_error_; }

private:
    std::string vault_root_path_;
    mutable std::string last_error_;
    
    // Path utilities
    std::string getProfileVaultPath(const std::string& profile_id) const;
    
    // Error handling
    void setError(const std::string& error) const;
    void clearError() const;
};

} // namespace PhantomVault