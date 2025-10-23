#include "phantom_vault/vault_storage_manager.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>

namespace phantom_vault {
namespace service {

namespace fs = std::filesystem;

class VaultStorageManager::Implementation {
public:
    Implementation()
        : username_()
        , vault_base_path_()
        , user_vault_path_()
        , backup_path_()
        , in_transaction_(false)
        , transaction_operations_()
        , last_error_()
    {}

    ~Implementation() {
        if (in_transaction_) {
            rollbackTransaction();
        }
    }

    bool initialize(const std::string& username) {
        username_ = username;
        
        // Get home directory
        const char* home = getenv("HOME");
        if (!home) {
            struct passwd* pw = getpwuid(getuid());
            if (!pw) {
                last_error_ = "Failed to get user home directory";
                return false;
            }
            home = pw->pw_dir;
        }

        // Set up paths
        vault_base_path_ = fs::path(home) / ".phantom_vault_storage";
        user_vault_path_ = vault_base_path_ / username;
        backup_path_ = user_vault_path_ / "backups";

        // Ensure directories exist
        if (!ensureDirectories()) {
            return false;
        }

        std::cout << "[VaultStorageManager] Initialized for user: " << username << std::endl;
        std::cout << "  Vault base path: " << vault_base_path_ << std::endl;
        std::cout << "  User vault path: " << user_vault_path_ << std::endl;
        std::cout << "  Backup path: " << backup_path_ << std::endl;

        return true;
    }

    StorageResult moveToVault(const fs::path& source_path, 
                             const fs::path& vault_path,
                             StorageProgressCallback progress_callback) {
        std::cout << "[VaultStorageManager] Moving to vault: " << source_path << " -> " << vault_path << std::endl;

        if (!fs::exists(source_path)) {
            return StorageResult(false, "Source path does not exist: " + source_path.string());
        }

        if (fs::exists(vault_path)) {
            return StorageResult(false, "Vault path already exists: " + vault_path.string());
        }

        try {
            // Ensure vault directory exists
            fs::create_directories(vault_path.parent_path());

            // Calculate total size for progress tracking
            size_t total_size = 0;
            if (progress_callback) {
                total_size = getFolderSize(source_path);
            }

            // Add to transaction if active
            if (in_transaction_) {
                StorageOperation op(StorageOperationType::MOVE_TO_VAULT, source_path, vault_path,
                                  "Move folder to vault storage");
                transaction_operations_.push_back(op);
            }

            // Perform the move operation
            StorageResult result = copyRecursive(source_path, vault_path, progress_callback, total_size);
            if (!result.success) {
                return result;
            }

            // Remove original after successful copy
            fs::remove_all(source_path);

            // Mark transaction operation as completed
            if (in_transaction_ && !transaction_operations_.empty()) {
                transaction_operations_.back().completed = true;
            }

            std::cout << "[VaultStorageManager] ✅ Successfully moved to vault" << std::endl;
            return StorageResult(true);

        } catch (const std::exception& e) {
            last_error_ = "Move to vault failed: " + std::string(e.what());
            return StorageResult(false, last_error_);
        }
    }

    StorageResult moveFromVault(const fs::path& vault_path, 
                               const fs::path& original_path,
                               StorageProgressCallback progress_callback) {
        std::cout << "[VaultStorageManager] Moving from vault: " << vault_path << " -> " << original_path << std::endl;

        if (!fs::exists(vault_path)) {
            return StorageResult(false, "Vault path does not exist: " + vault_path.string());
        }

        if (fs::exists(original_path)) {
            return StorageResult(false, "Original path already exists: " + original_path.string());
        }

        try {
            // Ensure original directory parent exists
            fs::create_directories(original_path.parent_path());

            // Calculate total size for progress tracking
            size_t total_size = 0;
            if (progress_callback) {
                total_size = getFolderSize(vault_path);
            }

            // Add to transaction if active
            if (in_transaction_) {
                StorageOperation op(StorageOperationType::MOVE_FROM_VAULT, vault_path, original_path,
                                  "Move folder from vault to original location");
                transaction_operations_.push_back(op);
            }

            // Perform the move operation
            StorageResult result = copyRecursive(vault_path, original_path, progress_callback, total_size);
            if (!result.success) {
                return result;
            }

            // Remove vault copy after successful restore
            fs::remove_all(vault_path);

            // Mark transaction operation as completed
            if (in_transaction_ && !transaction_operations_.empty()) {
                transaction_operations_.back().completed = true;
            }

            std::cout << "[VaultStorageManager] ✅ Successfully moved from vault" << std::endl;
            return StorageResult(true);

        } catch (const std::exception& e) {
            last_error_ = "Move from vault failed: " + std::string(e.what());
            return StorageResult(false, last_error_);
        }
    }

    StorageResult createBackup(const fs::path& source_path, 
                              const fs::path& backup_path,
                              const std::string& operation_type) {
        std::cout << "[VaultStorageManager] Creating backup: " << source_path << " -> " << backup_path << std::endl;

        if (!fs::exists(source_path)) {
            return StorageResult(false, "Source path does not exist: " + source_path.string());
        }

        try {
            // Ensure backup directory exists
            fs::create_directories(backup_path.parent_path());

            // Add to transaction if active
            if (in_transaction_) {
                StorageOperation op(StorageOperationType::CREATE_BACKUP, source_path, backup_path,
                                  "Create backup for " + operation_type);
                transaction_operations_.push_back(op);
            }

            // Perform backup copy
            StorageResult result = copyRecursive(source_path, backup_path, nullptr, 0);
            if (!result.success) {
                return result;
            }

            // Mark transaction operation as completed
            if (in_transaction_ && !transaction_operations_.empty()) {
                transaction_operations_.back().completed = true;
            }

            std::cout << "[VaultStorageManager] ✅ Backup created successfully" << std::endl;
            return StorageResult(true);

        } catch (const std::exception& e) {
            last_error_ = "Backup creation failed: " + std::string(e.what());
            return StorageResult(false, last_error_);
        }
    }

    StorageResult restoreFromBackup(const fs::path& backup_path, 
                                   const fs::path& target_path) {
        std::cout << "[VaultStorageManager] Restoring from backup: " << backup_path << " -> " << target_path << std::endl;

        if (!fs::exists(backup_path)) {
            return StorageResult(false, "Backup path does not exist: " + backup_path.string());
        }

        try {
            // Remove target if it exists
            if (fs::exists(target_path)) {
                fs::remove_all(target_path);
            }

            // Ensure target directory parent exists
            fs::create_directories(target_path.parent_path());

            // Add to transaction if active
            if (in_transaction_) {
                StorageOperation op(StorageOperationType::RESTORE_BACKUP, backup_path, target_path,
                                  "Restore from backup");
                transaction_operations_.push_back(op);
            }

            // Perform restore copy
            StorageResult result = copyRecursive(backup_path, target_path, nullptr, 0);
            if (!result.success) {
                return result;
            }

            // Mark transaction operation as completed
            if (in_transaction_ && !transaction_operations_.empty()) {
                transaction_operations_.back().completed = true;
            }

            std::cout << "[VaultStorageManager] ✅ Restored from backup successfully" << std::endl;
            return StorageResult(true);

        } catch (const std::exception& e) {
            last_error_ = "Backup restoration failed: " + std::string(e.what());
            return StorageResult(false, last_error_);
        }
    }

    int cleanOldBackups(const std::string& folder_name, int keep_count) {
        std::cout << "[VaultStorageManager] Cleaning old backups for: " << folder_name 
                  << " (keeping " << keep_count << ")" << std::endl;

        try {
            if (!fs::exists(backup_path_)) {
                return 0;
            }

            // Find all backup directories for this folder
            std::vector<std::pair<fs::path, fs::file_time_type>> backups;
            
            for (const auto& entry : fs::directory_iterator(backup_path_)) {
                if (entry.is_directory()) {
                    std::string dir_name = entry.path().filename().string();
                    if (dir_name.find(folder_name + "_backup_") == 0) {
                        backups.emplace_back(entry.path(), entry.last_write_time());
                    }
                }
            }

            // Sort by modification time (newest first)
            std::sort(backups.begin(), backups.end(),
                     [](const auto& a, const auto& b) {
                         return a.second > b.second;
                     });

            // Remove old backups beyond keep_count
            int cleaned = 0;
            for (size_t i = keep_count; i < backups.size(); ++i) {
                try {
                    fs::remove_all(backups[i].first);
                    cleaned++;
                    std::cout << "  Removed old backup: " << backups[i].first.filename() << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "  Failed to remove backup " << backups[i].first << ": " << e.what() << std::endl;
                }
            }

            std::cout << "[VaultStorageManager] Cleaned " << cleaned << " old backups" << std::endl;
            return cleaned;

        } catch (const std::exception& e) {
            last_error_ = "Backup cleanup failed: " + std::string(e.what());
            return 0;
        }
    }

    fs::path generateVaultPath(const std::string& folder_name, const std::string& folder_id) {
        // Generate path like: ~/.phantom_vault_storage/username/vaults/FolderName_vault_1234567890_abc123
        std::string vault_dir_name = folder_name + "_vault_" + folder_id;
        return user_vault_path_ / "vaults" / vault_dir_name;
    }

    fs::path generateBackupPath(const std::string& folder_name, const std::string& operation_type) {
        // Generate timestamp
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        
        // Generate path like: ~/.phantom_vault_storage/username/backups/FolderName_backup_pre-lock_1234567890
        std::string backup_dir_name = folder_name + "_backup_" + operation_type + "_" + std::to_string(timestamp);
        return backup_path_ / backup_dir_name;
    }

    bool pathExists(const fs::path& path) {
        return fs::exists(path);
    }

    size_t getFolderSize(const fs::path& path) {
        if (!fs::exists(path)) {
            return 0;
        }

        try {
            size_t size = 0;
            for (const auto& entry : fs::recursive_directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    size += entry.file_size();
                }
            }
            return size;
        } catch (const std::exception& e) {
            std::cerr << "Error calculating folder size: " << e.what() << std::endl;
            return 0;
        }
    }

    bool verifyFolderIntegrity(const fs::path& path) {
        if (!fs::exists(path)) {
            return false;
        }

        try {
            // Check if we can iterate through all files
            for (const auto& entry : fs::recursive_directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    // Try to get file size (this will fail if file is corrupted/inaccessible)
                    entry.file_size();
                }
            }
            return true;
        } catch (const std::exception& e) {
            last_error_ = "Folder integrity check failed: " + std::string(e.what());
            return false;
        }
    }

    // Transaction support
    bool beginTransaction() {
        if (in_transaction_) {
            last_error_ = "Transaction already in progress";
            return false;
        }

        in_transaction_ = true;
        transaction_operations_.clear();
        std::cout << "[VaultStorageManager] Transaction started" << std::endl;
        return true;
    }

    bool commitTransaction() {
        if (!in_transaction_) {
            last_error_ = "No transaction in progress";
            return false;
        }

        // All operations should already be completed
        // Just clean up transaction state
        in_transaction_ = false;
        transaction_operations_.clear();
        std::cout << "[VaultStorageManager] Transaction committed" << std::endl;
        return true;
    }

    bool rollbackTransaction() {
        if (!in_transaction_) {
            last_error_ = "No transaction in progress";
            return false;
        }

        std::cout << "[VaultStorageManager] Rolling back transaction..." << std::endl;

        // Rollback operations in reverse order
        for (auto it = transaction_operations_.rbegin(); it != transaction_operations_.rend(); ++it) {
            if (it->completed) {
                rollbackOperation(*it);
            }
        }

        in_transaction_ = false;
        transaction_operations_.clear();
        std::cout << "[VaultStorageManager] Transaction rolled back" << std::endl;
        return true;
    }

    bool isInTransaction() const {
        return in_transaction_;
    }

    std::vector<StorageOperation> getTransactionOperations() const {
        return transaction_operations_;
    }

    fs::path getVaultBasePath() const {
        return vault_base_path_;
    }

    fs::path getUserVaultPath() const {
        return user_vault_path_;
    }

    fs::path getBackupPath() const {
        return backup_path_;
    }

    std::string getLastError() const {
        return last_error_;
    }

private:
    bool ensureDirectories() {
        try {
            // Create base vault directory
            if (!fs::exists(vault_base_path_)) {
                fs::create_directories(vault_base_path_);
                fs::permissions(vault_base_path_, fs::perms::owner_all);
            }

            // Create user vault directory
            if (!fs::exists(user_vault_path_)) {
                fs::create_directories(user_vault_path_);
                fs::permissions(user_vault_path_, fs::perms::owner_all);
            }

            // Create subdirectories
            std::vector<std::string> subdirs = {"vaults", "backups", "metadata", "logs"};
            for (const auto& subdir : subdirs) {
                fs::path dir_path = user_vault_path_ / subdir;
                if (!fs::exists(dir_path)) {
                    fs::create_directories(dir_path);
                    fs::permissions(dir_path, fs::perms::owner_all);
                }
            }

            return true;
        } catch (const std::exception& e) {
            last_error_ = "Failed to create directories: " + std::string(e.what());
            return false;
        }
    }

    StorageResult copyRecursive(const fs::path& source, const fs::path& destination,
                               StorageProgressCallback progress_callback, size_t total_size) {
        try {
            size_t processed_bytes = 0;
            std::vector<std::string> processed_paths;

            // Create destination directory
            fs::create_directories(destination);

            // Copy all files and subdirectories
            for (const auto& entry : fs::recursive_directory_iterator(source)) {
                const auto& source_path = entry.path();
                auto relative_path = fs::relative(source_path, source);
                auto dest_path = destination / relative_path;

                if (entry.is_directory()) {
                    fs::create_directories(dest_path);
                } else if (entry.is_regular_file()) {
                    // Create parent directory if needed
                    fs::create_directories(dest_path.parent_path());
                    
                    // Copy file
                    fs::copy_file(source_path, dest_path);
                    
                    // Update progress
                    size_t file_size = entry.file_size();
                    processed_bytes += file_size;
                    processed_paths.push_back(dest_path.string());

                    if (progress_callback) {
                        progress_callback(source_path.string(), processed_bytes, total_size);
                    }
                }
            }

            StorageResult result(true);
            result.processed_paths = processed_paths;
            result.bytes_processed = processed_bytes;
            return result;

        } catch (const std::exception& e) {
            return StorageResult(false, "Copy operation failed: " + std::string(e.what()));
        }
    }

    void rollbackOperation(const StorageOperation& op) {
        try {
            switch (op.type) {
                case StorageOperationType::MOVE_TO_VAULT:
                    // Restore from vault back to original location
                    if (fs::exists(op.destination)) {
                        if (fs::exists(op.source)) {
                            fs::remove_all(op.source);
                        }
                        fs::rename(op.destination, op.source);
                        std::cout << "  Rolled back move to vault: " << op.destination << " -> " << op.source << std::endl;
                    }
                    break;

                case StorageOperationType::MOVE_FROM_VAULT:
                    // Move back to vault from original location
                    if (fs::exists(op.destination)) {
                        if (fs::exists(op.source)) {
                            fs::remove_all(op.source);
                        }
                        fs::rename(op.destination, op.source);
                        std::cout << "  Rolled back move from vault: " << op.destination << " -> " << op.source << std::endl;
                    }
                    break;

                case StorageOperationType::CREATE_BACKUP:
                    // Remove created backup
                    if (fs::exists(op.destination)) {
                        fs::remove_all(op.destination);
                        std::cout << "  Rolled back backup creation: " << op.destination << std::endl;
                    }
                    break;

                case StorageOperationType::RESTORE_BACKUP:
                    // Remove restored files
                    if (fs::exists(op.destination)) {
                        fs::remove_all(op.destination);
                        std::cout << "  Rolled back backup restoration: " << op.destination << std::endl;
                    }
                    break;

                default:
                    std::cout << "  Unknown operation type for rollback" << std::endl;
                    break;
            }
        } catch (const std::exception& e) {
            std::cerr << "  Rollback failed for operation: " << e.what() << std::endl;
        }
    }

    std::string username_;
    fs::path vault_base_path_;
    fs::path user_vault_path_;
    fs::path backup_path_;
    
    bool in_transaction_;
    std::vector<StorageOperation> transaction_operations_;
    std::string last_error_;
};

// VaultStorageManager public interface
VaultStorageManager::VaultStorageManager() : pimpl(std::make_unique<Implementation>()) {}
VaultStorageManager::~VaultStorageManager() = default;

bool VaultStorageManager::initialize(const std::string& username) {
    return pimpl->initialize(username);
}

StorageResult VaultStorageManager::moveToVault(const std::filesystem::path& source_path, 
                                              const std::filesystem::path& vault_path,
                                              StorageProgressCallback progress_callback) {
    return pimpl->moveToVault(source_path, vault_path, progress_callback);
}

StorageResult VaultStorageManager::moveFromVault(const std::filesystem::path& vault_path, 
                                                const std::filesystem::path& original_path,
                                                StorageProgressCallback progress_callback) {
    return pimpl->moveFromVault(vault_path, original_path, progress_callback);
}

StorageResult VaultStorageManager::createBackup(const std::filesystem::path& source_path, 
                                               const std::filesystem::path& backup_path,
                                               const std::string& operation_type) {
    return pimpl->createBackup(source_path, backup_path, operation_type);
}

StorageResult VaultStorageManager::restoreFromBackup(const std::filesystem::path& backup_path, 
                                                    const std::filesystem::path& target_path) {
    return pimpl->restoreFromBackup(backup_path, target_path);
}

int VaultStorageManager::cleanOldBackups(const std::string& folder_name, int keep_count) {
    return pimpl->cleanOldBackups(folder_name, keep_count);
}

std::filesystem::path VaultStorageManager::generateVaultPath(const std::string& folder_name, 
                                                            const std::string& folder_id) {
    return pimpl->generateVaultPath(folder_name, folder_id);
}

std::filesystem::path VaultStorageManager::generateBackupPath(const std::string& folder_name, 
                                                             const std::string& operation_type) {
    return pimpl->generateBackupPath(folder_name, operation_type);
}

bool VaultStorageManager::pathExists(const std::filesystem::path& path) {
    return pimpl->pathExists(path);
}

size_t VaultStorageManager::getFolderSize(const std::filesystem::path& path) {
    return pimpl->getFolderSize(path);
}

bool VaultStorageManager::verifyFolderIntegrity(const std::filesystem::path& path) {
    return pimpl->verifyFolderIntegrity(path);
}

bool VaultStorageManager::beginTransaction() {
    return pimpl->beginTransaction();
}

bool VaultStorageManager::commitTransaction() {
    return pimpl->commitTransaction();
}

bool VaultStorageManager::rollbackTransaction() {
    return pimpl->rollbackTransaction();
}

bool VaultStorageManager::isInTransaction() const {
    return pimpl->isInTransaction();
}

std::vector<StorageOperation> VaultStorageManager::getTransactionOperations() const {
    return pimpl->getTransactionOperations();
}

std::filesystem::path VaultStorageManager::getVaultBasePath() const {
    return pimpl->getVaultBasePath();
}

std::filesystem::path VaultStorageManager::getUserVaultPath() const {
    return pimpl->getUserVaultPath();
}

std::filesystem::path VaultStorageManager::getBackupPath() const {
    return pimpl->getBackupPath();
}

std::string VaultStorageManager::getLastError() const {
    return pimpl->getLastError();
}

} // namespace service
} // namespace phantom_vault