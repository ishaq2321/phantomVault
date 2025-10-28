#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <optional>
#include <unordered_map>

namespace phantomvault {

/**
 * @brief Metadata preservation structure for folder restoration
 */
struct FolderMetadata {
    std::string original_path;
    std::string owner;
    std::string group;
    uint32_t permissions;
    std::chrono::system_clock::time_point created_time;
    std::chrono::system_clock::time_point modified_time;
    std::chrono::system_clock::time_point accessed_time;
    std::unordered_map<std::string, std::string> extended_attributes;
    bool was_hidden;
    std::string original_location;
};

/**
 * @brief Vault organization structure
 */
struct VaultStructure {
    std::string vault_id;
    std::string profile_id;
    std::string vault_path;
    std::string hidden_folders_path;
    std::string metadata_path;
    std::string temp_path;
    std::string backup_path;
    size_t total_folders;
    size_t total_size;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_modified;
};

/**
 * @brief Folder hiding result
 */
struct HidingResult {
    bool success = false;
    std::string message;
    std::string error_details;
    std::string backup_location;
    FolderMetadata preserved_metadata;
};

/**
 * @brief Folder restoration result
 */
struct RestorationResult {
    bool success = false;
    std::string message;
    std::string error_details;
    std::string restored_path;
    bool metadata_restored = false;
};

/**
 * @brief Vault cleanup result
 */
struct CleanupResult {
    bool success = false;
    std::string message;
    std::string error_details;
    size_t folders_cleaned = 0;
    size_t bytes_freed = 0;
};

/**
 * @brief Advanced Vault Handler for PhantomVault
 * 
 * Implements platform-specific folder hiding mechanisms requiring elevated privileges,
 * complete folder restoration with metadata preservation, vault structure management,
 * and secure deletion capabilities.
 */
class VaultHandler {
public:
    VaultHandler();
    ~VaultHandler();
    
    // Initialization
    bool initialize(const std::string& vault_root_path);
    bool requiresElevatedPrivileges() const;
    
    // Platform-specific folder hiding with elevated privileges
    HidingResult hideFolder(const std::string& folder_path, const std::string& vault_id);
    RestorationResult restoreFolder(const std::string& vault_id, const std::string& folder_identifier);
    
    // Metadata preservation and restoration
    bool preserveFolderMetadata(const std::string& folder_path, FolderMetadata& metadata);
    bool restoreFolderMetadata(const std::string& folder_path, const FolderMetadata& metadata);
    
    // Vault structure management and organization
    bool createVaultStructure(const std::string& vault_id, const std::string& profile_id);
    bool organizeVaultContents(const std::string& vault_id);
    VaultStructure getVaultStructure(const std::string& vault_id) const;
    std::vector<std::string> listVaultFolders(const std::string& vault_id) const;
    
    // Secure folder deletion from vault
    CleanupResult secureDeleteFromVault(const std::string& vault_id, const std::string& folder_identifier);
    CleanupResult cleanupVault(const std::string& vault_id);
    bool secureWipeVaultData(const std::string& vault_path);
    
    // Vault integrity and maintenance
    bool validateVaultIntegrity(const std::string& vault_id);
    bool repairVaultStructure(const std::string& vault_id);
    bool compactVault(const std::string& vault_id);
    
    // Platform-specific operations
    bool setFolderHidden(const std::string& folder_path, bool hidden);
    bool setFolderSystemProtected(const std::string& folder_path, bool protected_mode);
    bool createSystemJunction(const std::string& source, const std::string& target);
    
    // Error handling
    std::string getLastError() const;
    std::vector<std::string> getOperationLog() const;
    
private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

} // namespace phantomvault