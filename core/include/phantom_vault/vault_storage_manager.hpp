#pragma once

#include "phantom_vault_export.h"
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <functional>

namespace phantom_vault {
namespace service {

/**
 * @brief Storage operation types for transaction tracking
 */
enum class PHANTOM_VAULT_EXPORT StorageOperationType {
    MOVE_TO_VAULT,      // Move folder from original location to vault
    MOVE_FROM_VAULT,    // Move folder from vault to original location
    CREATE_BACKUP,      // Create backup before operation
    RESTORE_BACKUP,     // Restore from backup on failure
    DELETE_BACKUP,      // Clean up old backups
    UPDATE_METADATA     // Update metadata files
};

/**
 * @brief Storage operation for transaction tracking
 */
struct PHANTOM_VAULT_EXPORT StorageOperation {
    StorageOperationType type;
    std::filesystem::path source;
    std::filesystem::path destination;
    std::string description;
    bool completed;
    std::string error_message;
    
    StorageOperation() : type(StorageOperationType::MOVE_TO_VAULT), completed(false) {}
    StorageOperation(StorageOperationType t, const std::filesystem::path& src, 
                    const std::filesystem::path& dest, const std::string& desc)
        : type(t), source(src), destination(dest), description(desc), completed(false) {}
};

/**
 * @brief Storage operation result
 */
struct PHANTOM_VAULT_EXPORT StorageResult {
    bool success;
    std::string error_message;
    std::vector<std::string> processed_paths;
    size_t bytes_processed;
    
    StorageResult() : success(false), bytes_processed(0) {}
    StorageResult(bool s) : success(s), bytes_processed(0) {}
    StorageResult(bool s, const std::string& error) : success(s), error_message(error), bytes_processed(0) {}
};

/**
 * @brief Progress callback for storage operations
 */
using StorageProgressCallback = std::function<void(const std::string& current_path, 
                                                  size_t processed_bytes, 
                                                  size_t total_bytes)>;

/**
 * @brief Vault storage manager for file system operations
 * 
 * Handles moving folders between vault storage and original locations,
 * creating and managing backups, and providing transaction support
 * for atomic operations with rollback capability.
 */
class PHANTOM_VAULT_EXPORT VaultStorageManager {
public:
    /**
     * @brief Constructor
     */
    VaultStorageManager();

    /**
     * @brief Destructor
     */
    ~VaultStorageManager();

    /**
     * @brief Initialize the storage manager
     * @param username OS username for vault paths
     * @return true if initialization successful, false otherwise
     */
    bool initialize(const std::string& username);

    /**
     * @brief Move folder from original location to vault storage
     * @param source_path Original folder location
     * @param vault_path Destination path in vault storage
     * @param progress_callback Optional progress callback
     * @return Storage operation result
     */
    StorageResult moveToVault(const std::filesystem::path& source_path, 
                             const std::filesystem::path& vault_path,
                             StorageProgressCallback progress_callback = nullptr);

    /**
     * @brief Move folder from vault storage to original location
     * @param vault_path Source path in vault storage
     * @param original_path Destination original location
     * @param progress_callback Optional progress callback
     * @return Storage operation result
     */
    StorageResult moveFromVault(const std::filesystem::path& vault_path, 
                               const std::filesystem::path& original_path,
                               StorageProgressCallback progress_callback = nullptr);

    /**
     * @brief Create backup of folder before operation
     * @param source_path Folder to backup
     * @param backup_path Backup destination path
     * @param operation_type Type of operation backup is for
     * @return Storage operation result
     */
    StorageResult createBackup(const std::filesystem::path& source_path, 
                              const std::filesystem::path& backup_path,
                              const std::string& operation_type);

    /**
     * @brief Restore folder from backup
     * @param backup_path Backup source path
     * @param target_path Restoration destination
     * @return Storage operation result
     */
    StorageResult restoreFromBackup(const std::filesystem::path& backup_path, 
                                   const std::filesystem::path& target_path);

    /**
     * @brief Clean old backups, keeping only the most recent ones
     * @param folder_name Folder name for backup cleanup
     * @param keep_count Number of backups to keep (default: 3)
     * @return Number of backups cleaned up
     */
    int cleanOldBackups(const std::string& folder_name, int keep_count = 3);

    /**
     * @brief Generate vault storage path for a folder
     * @param folder_name Name of the folder
     * @param folder_id Unique folder identifier
     * @return Generated vault path
     */
    std::filesystem::path generateVaultPath(const std::string& folder_name, 
                                           const std::string& folder_id);

    /**
     * @brief Generate backup path for a folder
     * @param folder_name Name of the folder
     * @param operation_type Type of operation (e.g., "pre-lock", "pre-unlock")
     * @return Generated backup path
     */
    std::filesystem::path generateBackupPath(const std::string& folder_name, 
                                            const std::string& operation_type);

    /**
     * @brief Check if path exists and is accessible
     * @param path Path to check
     * @return true if path exists and is accessible, false otherwise
     */
    bool pathExists(const std::filesystem::path& path);

    /**
     * @brief Get folder size in bytes
     * @param path Folder path
     * @return Size in bytes, 0 if error
     */
    size_t getFolderSize(const std::filesystem::path& path);

    /**
     * @brief Verify folder integrity (check if all files are accessible)
     * @param path Folder path
     * @return true if folder is intact, false if corrupted/missing files
     */
    bool verifyFolderIntegrity(const std::filesystem::path& path);

    // Transaction support for atomic operations
    
    /**
     * @brief Begin a new transaction
     * @return true if transaction started successfully, false otherwise
     */
    bool beginTransaction();

    /**
     * @brief Commit current transaction (make all operations permanent)
     * @return true if commit successful, false otherwise
     */
    bool commitTransaction();

    /**
     * @brief Rollback current transaction (undo all operations)
     * @return true if rollback successful, false otherwise
     */
    bool rollbackTransaction();

    /**
     * @brief Check if currently in a transaction
     * @return true if transaction is active, false otherwise
     */
    bool isInTransaction() const;

    /**
     * @brief Get current transaction operations
     * @return Vector of operations in current transaction
     */
    std::vector<StorageOperation> getTransactionOperations() const;

    /**
     * @brief Get vault storage base path
     * @return Base vault storage path
     */
    std::filesystem::path getVaultBasePath() const;

    /**
     * @brief Get user vault storage path
     * @return User-specific vault storage path
     */
    std::filesystem::path getUserVaultPath() const;

    /**
     * @brief Get backup storage path
     * @return Backup storage path
     */
    std::filesystem::path getBackupPath() const;

    /**
     * @brief Get last error message
     * @return Error message string
     */
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class VaultStorageManager

} // namespace service
} // namespace phantom_vault