#include "profile_vault.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <openssl/sha.h>

#ifdef PLATFORM_LINUX
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#else
#include <sys/stat.h>
#endif

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace PhantomVault {

// ProfileVault Implementation

ProfileVault::ProfileVault(const std::string& profile_id, const std::string& vault_root_path)
    : profile_id_(profile_id)
    , vault_root_path_(vault_root_path)
    , vault_path_(vault_root_path + "/" + profile_id)
    , metadata_file_(vault_path_ + "/vault_metadata.json")
    , temp_unlock_file_(vault_path_ + "/temp_unlock.json")
    , encryption_engine_(std::make_unique<EncryptionEngine>()) {
    clearError();
}

ProfileVault::~ProfileVault() = default;

bool ProfileVault::initialize() {
    clearError();
    
    try {
        // Ensure vault directory exists
        if (!fs::exists(vault_path_)) {
            if (!createVaultStructure()) {
                return false;
            }
        }
        
        // Load existing metadata
        if (fs::exists(metadata_file_)) {
            if (!loadVaultMetadata()) {
                setError("Failed to load vault metadata");
                return false;
            }
        } else {
            // Initialize new vault metadata
            vault_metadata_.profile_id = profile_id_;
            vault_metadata_.vault_version = "1.0";
            vault_metadata_.created_at = std::chrono::system_clock::now();
            vault_metadata_.last_modified = vault_metadata_.created_at;
            
            if (!saveVaultMetadata()) {
                return false;
            }
        }
        
        // Load temporary unlock state if exists
        if (fs::exists(temp_unlock_file_)) {
            loadTemporaryUnlockState();
        }
        
        // Validate encryption engine
        if (!encryption_engine_->selfTest()) {
            setError("Encryption engine self-test failed: " + encryption_engine_->getLastError());
            return false;
        }
        
        std::cout << "[ProfileVault] Initialized vault for profile: " << profile_id_ << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        setError("Failed to initialize vault: " + std::string(e.what()));
        return false;
    }
}

bool ProfileVault::createVaultStructure() {
    clearError();
    
    try {
        // Create main vault directory
        fs::create_directories(vault_path_);
        fs::permissions(vault_path_, fs::perms::owner_all, fs::perm_options::replace);
        
        // Create subdirectories
        fs::create_directories(vault_path_ + "/folders");
        fs::create_directories(vault_path_ + "/metadata");
        fs::create_directories(vault_path_ + "/temp");
        
        // Set secure permissions
        fs::permissions(vault_path_ + "/folders", fs::perms::owner_all, fs::perm_options::replace);
        fs::permissions(vault_path_ + "/metadata", fs::perms::owner_all, fs::perm_options::replace);
        fs::permissions(vault_path_ + "/temp", fs::perms::owner_all, fs::perm_options::replace);
        
        std::cout << "[ProfileVault] Created vault structure: " << vault_path_ << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        setError("Failed to create vault structure: " + std::string(e.what()));
        return false;
    }
}

VaultOperationResult ProfileVault::lockFolder(const std::string& folder_path, const std::string& master_key) {
    clearError();
    VaultOperationResult result;
    
    try {
        // Validate inputs
        if (folder_path.empty()) {
            result.error_details = "Folder path cannot be empty";
            return result;
        }
        
        if (!fs::exists(folder_path)) {
            result.error_details = "Folder does not exist: " + folder_path;
            return result;
        }
        
        if (!fs::is_directory(folder_path)) {
            result.error_details = "Path is not a directory: " + folder_path;
            return result;
        }
        
        // Check if folder is already locked
        if (isFolderLocked(folder_path)) {
            result.error_details = "Folder is already locked";
            return result;
        }
        
        // Encrypt and store the folder
        result = encryptAndStoreFolder(folder_path, master_key);
        
        if (result.success) {
            // Hide the original folder
            if (!hideOriginalFolder(folder_path)) {
                // If hiding fails, we should clean up the vault entry
                setError("Failed to hide original folder, cleaning up vault entry");
                
                // Cleanup: Remove the vault files we just created
                std::string vault_location = generateVaultLocation(folder_path);
                std::string vault_folder_path = getVaultFolderPath(vault_location);
                std::string metadata_path = getFolderMetadataPath(vault_location);
                
                if (fs::exists(vault_folder_path)) {
                    fs::remove_all(vault_folder_path);
                }
                if (fs::exists(metadata_path)) {
                    fs::remove(metadata_path);
                }
                
                // Remove from vault metadata
                auto it = std::find(vault_metadata_.locked_folders.begin(),
                                   vault_metadata_.locked_folders.end(), folder_path);
                if (it != vault_metadata_.locked_folders.end()) {
                    vault_metadata_.locked_folders.erase(it);
                    vault_metadata_.total_folders--;
                    saveVaultMetadata();
                }
                
                result.success = false;
                result.error_details = last_error_;
                return result;
            }
            
            result.message = "Folder successfully locked and encrypted";
            std::cout << "[ProfileVault] Locked folder: " << folder_path << std::endl;
        }
        
        return result;
        
    } catch (const std::exception& e) {
        result.error_details = "Failed to lock folder: " + std::string(e.what());
        return result;
    }
}

VaultOperationResult ProfileVault::unlockFolder(const std::string& folder_path, const std::string& master_key, UnlockMode mode) {
    clearError();
    VaultOperationResult result;
    
    try {
        // Check if folder is locked
        if (!isFolderLocked(folder_path)) {
            result.error_details = "Folder is not locked: " + folder_path;
            return result;
        }
        
        // Get folder info
        auto folder_info = getFolderInfo(folder_path);
        if (!folder_info) {
            result.error_details = "Failed to get folder information";
            return result;
        }
        
        // Decrypt and restore the folder
        result = decryptAndRestoreFolder(folder_info->vault_location, folder_path, master_key, mode);
        
        if (result.success) {
            if (mode == UnlockMode::TEMPORARY) {
                // Add to temporary unlock tracking
                temp_unlock_state_.unlocked_folders.push_back(folder_path);
                temp_unlock_state_.unlock_timestamp = std::chrono::system_clock::now();
                saveTemporaryUnlockState();
                
                result.message = "Folder temporarily unlocked (will auto-lock on system events)";
            } else {
                // Remove from vault tracking for permanent unlock
                auto it = std::find(vault_metadata_.locked_folders.begin(), 
                                  vault_metadata_.locked_folders.end(), folder_path);
                if (it != vault_metadata_.locked_folders.end()) {
                    vault_metadata_.locked_folders.erase(it);
                    vault_metadata_.total_folders--;
                    saveVaultMetadata();
                }
                
                // Clean up vault files
                std::string vault_folder_path = getVaultFolderPath(folder_info->vault_location);
                if (fs::exists(vault_folder_path)) {
                    fs::remove_all(vault_folder_path);
                }
                
                result.message = "Folder permanently unlocked and removed from vault";
            }
            
            std::cout << "[ProfileVault] Unlocked folder: " << folder_path 
                     << " (mode: " << (mode == UnlockMode::TEMPORARY ? "temporary" : "permanent") << ")" << std::endl;
        }
        
        return result;
        
    } catch (const std::exception& e) {
        result.error_details = "Failed to unlock folder: " + std::string(e.what());
        return result;
    }
}

std::vector<LockedFolderInfo> ProfileVault::getLockedFolders() const {
    std::vector<LockedFolderInfo> folders;
    
    try {
        for (const auto& folder_path : vault_metadata_.locked_folders) {
            auto info = getFolderInfo(folder_path);
            if (info) {
                folders.push_back(*info);
            }
        }
    } catch (const std::exception& e) {
        setError("Failed to get locked folders: " + std::string(e.what()));
    }
    
    return folders;
}

std::optional<LockedFolderInfo> ProfileVault::getFolderInfo(const std::string& folder_path) const {
    clearError();
    
    try {
        std::string vault_location = generateVaultLocation(folder_path);
        return loadFolderMetadata(vault_location);
    } catch (const std::exception& e) {
        setError("Failed to get folder info: " + std::string(e.what()));
        return std::nullopt;
    }
}

bool ProfileVault::isFolderLocked(const std::string& folder_path) const {
    auto it = std::find(vault_metadata_.locked_folders.begin(), 
                       vault_metadata_.locked_folders.end(), folder_path);
    return it != vault_metadata_.locked_folders.end();
}

bool ProfileVault::isFolderTemporarilyUnlocked(const std::string& folder_path) const {
    auto it = std::find(temp_unlock_state_.unlocked_folders.begin(),
                       temp_unlock_state_.unlocked_folders.end(), folder_path);
    return it != temp_unlock_state_.unlocked_folders.end();
}

bool ProfileVault::isValidMasterKey(const std::string& master_key) const {
    // Test the master key by trying to decrypt a small test file
    // For now, we'll assume it's valid if it's not empty
    // In a real implementation, we'd store a test encrypted value
    return !master_key.empty() && master_key.length() >= 4;
}

VaultOperationResult ProfileVault::relockTemporaryFolders() {
    clearError();
    VaultOperationResult result;
    
    try {
        std::vector<std::string> failed_folders;
        
        for (const auto& folder_path : temp_unlock_state_.unlocked_folders) {
            if (fs::exists(folder_path)) {
                // Hide the folder again
                if (!hideOriginalFolder(folder_path)) {
                    failed_folders.push_back(folder_path);
                }
            }
        }
        
        if (failed_folders.empty()) {
            clearTemporaryUnlockState();
            result.success = true;
            result.message = "All temporary folders re-locked successfully";
        } else {
            result.error_details = "Failed to re-lock some folders";
            result.processed_files = failed_folders;
        }
        
        return result;
        
    } catch (const std::exception& e) {
        result.error_details = "Failed to re-lock temporary folders: " + std::string(e.what());
        return result;
    }
}

std::vector<std::string> ProfileVault::getTemporarilyUnlockedFolders() const {
    return temp_unlock_state_.unlocked_folders;
}

bool ProfileVault::validateVaultIntegrity() const {
    clearError();
    
    try {
        // Check if vault structure exists
        if (!fs::exists(vault_path_)) {
            setError("Vault directory does not exist");
            return false;
        }
        
        // Check required subdirectories
        if (!fs::exists(vault_path_ + "/folders") ||
            !fs::exists(vault_path_ + "/metadata")) {
            setError("Vault structure is incomplete");
            return false;
        }
        
        // Check metadata file
        if (!fs::exists(metadata_file_)) {
            setError("Vault metadata file missing");
            return false;
        }
        
        // Validate each locked folder
        for (const auto& folder_path : vault_metadata_.locked_folders) {
            if (!verifyFolderIntegrity(generateVaultLocation(folder_path))) {
                setError("Folder integrity check failed: " + folder_path);
                return false;
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        setError("Vault integrity validation failed: " + std::string(e.what()));
        return false;
    }
}

bool ProfileVault::cleanupCorruptedEntries() {
    clearError();
    
    try {
        std::vector<std::string> corrupted_folders;
        
        // Check each locked folder for corruption
        for (const auto& folder_path : vault_metadata_.locked_folders) {
            std::string vault_location = generateVaultLocation(folder_path);
            
            if (!verifyFolderIntegrity(vault_location)) {
                corrupted_folders.push_back(folder_path);
                
                // Remove corrupted vault files
                std::string vault_folder_path = getVaultFolderPath(vault_location);
                std::string metadata_path = getFolderMetadataPath(vault_location);
                
                if (fs::exists(vault_folder_path)) {
                    fs::remove_all(vault_folder_path);
                }
                
                if (fs::exists(metadata_path)) {
                    fs::remove(metadata_path);
                }
                
                std::cout << "[ProfileVault] Cleaned up corrupted entry: " << folder_path << std::endl;
            }
        }
        
        // Remove corrupted entries from metadata
        for (const auto& corrupted_path : corrupted_folders) {
            auto it = std::find(vault_metadata_.locked_folders.begin(),
                               vault_metadata_.locked_folders.end(), corrupted_path);
            if (it != vault_metadata_.locked_folders.end()) {
                vault_metadata_.locked_folders.erase(it);
                vault_metadata_.total_folders--;
            }
        }
        
        // Update metadata if changes were made
        if (!corrupted_folders.empty()) {
            vault_metadata_.last_modified = std::chrono::system_clock::now();
            saveVaultMetadata();
        }
        
        return true;
        
    } catch (const std::exception& e) {
        setError("Failed to cleanup corrupted entries: " + std::string(e.what()));
        return false;
    }
}

size_t ProfileVault::getVaultSize() const {
    size_t total_size = 0;
    
    try {
        if (fs::exists(vault_path_)) {
            for (const auto& entry : fs::recursive_directory_iterator(vault_path_)) {
                if (entry.is_regular_file()) {
                    total_size += fs::file_size(entry.path());
                }
            }
        }
    } catch (const std::exception& e) {
        setError("Failed to calculate vault size: " + std::string(e.what()));
    }
    
    return total_size;
}

// Private implementation methods

VaultOperationResult ProfileVault::encryptAndStoreFolder(const std::string& folder_path, const std::string& master_key) {
    VaultOperationResult result;
    
    try {
        std::string vault_location = generateVaultLocation(folder_path);
        std::string vault_folder_path = getVaultFolderPath(vault_location);
        
        // Create vault folder
        fs::create_directories(vault_folder_path);
        
        // Initialize folder info
        LockedFolderInfo folder_info;
        folder_info.original_path = folder_path;
        folder_info.vault_location = vault_location;
        folder_info.lock_timestamp = std::chrono::system_clock::now();
        
        // Recursively encrypt all files
        size_t file_count = 0;
        size_t total_size = 0;
        
        for (const auto& entry : fs::recursive_directory_iterator(folder_path)) {
            if (entry.is_regular_file()) {
                std::string relative_path = fs::relative(entry.path(), folder_path);
                std::string vault_file_path = vault_folder_path + "/" + relative_path + ".enc";
                
                // Create directory structure in vault
                fs::create_directories(fs::path(vault_file_path).parent_path());
                
                if (encryptFile(entry.path().string(), vault_file_path, master_key)) {
                    file_count++;
                    total_size += fs::file_size(entry.path());
                    result.processed_files.push_back(entry.path().string());
                } else {
                    result.error_details = "Failed to encrypt file: " + entry.path().string();
                    return result;
                }
            }
        }
        
        folder_info.file_count = file_count;
        folder_info.total_size = total_size;
        
        // Save folder metadata
        if (!saveFolderMetadata(vault_location, folder_info)) {
            result.error_details = "Failed to save folder metadata";
            return result;
        }
        
        // Update vault metadata
        vault_metadata_.locked_folders.push_back(folder_path);
        vault_metadata_.total_folders++;
        vault_metadata_.total_files += file_count;
        vault_metadata_.last_modified = std::chrono::system_clock::now();
        
        if (!saveVaultMetadata()) {
            result.error_details = "Failed to update vault metadata";
            return result;
        }
        
        result.success = true;
        result.message = "Folder encrypted and stored successfully";
        
        return result;
        
    } catch (const std::exception& e) {
        result.error_details = "Failed to encrypt and store folder: " + std::string(e.what());
        return result;
    }
}

VaultOperationResult ProfileVault::decryptAndRestoreFolder(const std::string& vault_location, 
                                                          const std::string& original_path, 
                                                          const std::string& master_key,
                                                          UnlockMode /* mode */) {
    VaultOperationResult result;
    
    try {
        std::string vault_folder_path = getVaultFolderPath(vault_location);
        
        if (!fs::exists(vault_folder_path)) {
            result.error_details = "Vault folder not found: " + vault_folder_path;
            return result;
        }
        
        // Create original directory if it doesn't exist
        if (!fs::exists(original_path)) {
            fs::create_directories(original_path);
        }
        
        // Recursively decrypt all files
        for (const auto& entry : fs::recursive_directory_iterator(vault_folder_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".enc") {
                std::string relative_path = fs::relative(entry.path(), vault_folder_path);
                
                // Remove .enc extension
                std::string relative_path_str = relative_path;
                if (relative_path_str.size() >= 4 && 
                    relative_path_str.substr(relative_path_str.size() - 4) == ".enc") {
                    relative_path_str = relative_path_str.substr(0, relative_path_str.length() - 4);
                }
                
                std::string output_path = original_path + "/" + relative_path_str;
                
                // Create directory structure
                fs::create_directories(fs::path(output_path).parent_path());
                
                if (decryptFile(entry.path().string(), output_path, master_key)) {
                    result.processed_files.push_back(output_path);
                } else {
                    result.error_details = "Failed to decrypt file: " + entry.path().string();
                    return result;
                }
            }
        }
        
        result.success = true;
        result.message = "Folder decrypted and restored successfully";
        
        return result;
        
    } catch (const std::exception& e) {
        result.error_details = "Failed to decrypt and restore folder: " + std::string(e.what());
        return result;
    }
}

bool ProfileVault::encryptFile(const std::string& file_path, const std::string& vault_file_path, const std::string& master_key) {
    try {
        auto result = encryption_engine_->encryptFile(file_path, master_key);
        if (!result.success) {
            setError("Encryption failed: " + result.error_message);
            return false;
        }
        
        // Save encrypted data and metadata
        json file_data;
        file_data["encrypted_data"] = result.encrypted_data;
        file_data["iv"] = result.iv;
        file_data["salt"] = result.salt;
        file_data["algorithm"] = result.algorithm;
        
        // Add file metadata
        auto metadata = encryption_engine_->getFileMetadata(file_path);
        file_data["metadata"] = {
            {"original_path", metadata.original_path},
            {"original_permissions", metadata.original_permissions},
            {"original_size", metadata.original_size},
            {"created_timestamp", metadata.created_timestamp},
            {"modified_timestamp", metadata.modified_timestamp},
            {"accessed_timestamp", metadata.accessed_timestamp},
            {"checksum_sha256", metadata.checksum_sha256}
        };
        
        std::ofstream file(vault_file_path, std::ios::binary);
        if (!file) {
            setError("Failed to create vault file: " + vault_file_path);
            return false;
        }
        
        file << file_data.dump();
        file.close();
        
        // Set secure permissions
        fs::permissions(vault_file_path, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace);
        
        return true;
        
    } catch (const std::exception& e) {
        setError("Failed to encrypt file: " + std::string(e.what()));
        return false;
    }
}

bool ProfileVault::decryptFile(const std::string& vault_file_path, const std::string& output_path, const std::string& master_key) {
    try {
        std::ifstream file(vault_file_path, std::ios::binary);
        if (!file) {
            setError("Failed to open vault file: " + vault_file_path);
            return false;
        }
        
        json file_data;
        file >> file_data;
        file.close();
        
        // Extract encryption data
        std::vector<uint8_t> encrypted_data = file_data["encrypted_data"];
        std::vector<uint8_t> iv = file_data["iv"];
        std::vector<uint8_t> salt = file_data["salt"];
        
        // Decrypt the file
        auto decrypted_data = encryption_engine_->decryptFile(encrypted_data, master_key, iv, salt);
        if (decrypted_data.empty()) {
            setError("Decryption failed: " + encryption_engine_->getLastError());
            return false;
        }
        
        // Write decrypted data
        std::ofstream output_file(output_path, std::ios::binary);
        if (!output_file) {
            setError("Failed to create output file: " + output_path);
            return false;
        }
        
        output_file.write(reinterpret_cast<const char*>(decrypted_data.data()), decrypted_data.size());
        output_file.close();
        
        // Restore file metadata if available
        if (file_data.contains("metadata")) {
            auto metadata = file_data["metadata"];
            
            // Restore permissions
            if (metadata.contains("original_permissions")) {
                std::string perm_str = metadata["original_permissions"];
                mode_t mode = std::stoi(perm_str, nullptr, 8);
                chmod(output_path.c_str(), mode);
            }
            
            // TODO: Restore timestamps using utimes() or similar
        }
        
        return true;
        
    } catch (const std::exception& e) {
        setError("Failed to decrypt file: " + std::string(e.what()));
        return false;
    }
}

bool ProfileVault::saveVaultMetadata() {
    try {
        json metadata;
        metadata["profile_id"] = vault_metadata_.profile_id;
        metadata["vault_version"] = vault_metadata_.vault_version;
        metadata["created_at"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            vault_metadata_.created_at.time_since_epoch()).count();
        metadata["last_modified"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            vault_metadata_.last_modified.time_since_epoch()).count();
        metadata["locked_folders"] = vault_metadata_.locked_folders;
        metadata["total_folders"] = vault_metadata_.total_folders;
        metadata["total_files"] = vault_metadata_.total_files;
        
        std::ofstream file(metadata_file_);
        if (!file) {
            setError("Failed to create metadata file");
            return false;
        }
        
        file << metadata.dump(2);
        file.close();
        
        fs::permissions(metadata_file_, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace);
        
        return true;
        
    } catch (const std::exception& e) {
        setError("Failed to save vault metadata: " + std::string(e.what()));
        return false;
    }
}

bool ProfileVault::loadVaultMetadata() {
    try {
        if (!fs::exists(metadata_file_)) {
            return false;
        }
        
        std::ifstream file(metadata_file_);
        json metadata;
        file >> metadata;
        file.close();
        
        vault_metadata_.profile_id = metadata["profile_id"];
        vault_metadata_.vault_version = metadata["vault_version"];
        
        int64_t created_ms = metadata["created_at"];
        int64_t modified_ms = metadata["last_modified"];
        
        vault_metadata_.created_at = std::chrono::system_clock::from_time_t(created_ms / 1000);
        vault_metadata_.last_modified = std::chrono::system_clock::from_time_t(modified_ms / 1000);
        vault_metadata_.locked_folders = metadata["locked_folders"].get<std::vector<std::string>>();
        vault_metadata_.total_folders = metadata["total_folders"];
        vault_metadata_.total_files = metadata["total_files"];
        
        return true;
        
    } catch (const std::exception& e) {
        setError("Failed to load vault metadata: " + std::string(e.what()));
        return false;
    }
}

bool ProfileVault::saveFolderMetadata(const std::string& vault_location, const LockedFolderInfo& info) {
    try {
        std::string metadata_path = getFolderMetadataPath(vault_location);
        
        json folder_metadata;
        folder_metadata["original_path"] = info.original_path;
        folder_metadata["vault_location"] = info.vault_location;
        folder_metadata["lock_timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            info.lock_timestamp.time_since_epoch()).count();
        folder_metadata["file_count"] = info.file_count;
        folder_metadata["total_size"] = info.total_size;
        folder_metadata["is_temporarily_unlocked"] = info.is_temporarily_unlocked;
        
        fs::create_directories(fs::path(metadata_path).parent_path());
        
        std::ofstream file(metadata_path);
        if (!file) {
            setError("Failed to create folder metadata file");
            return false;
        }
        
        file << folder_metadata.dump(2);
        file.close();
        
        fs::permissions(metadata_path, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace);
        
        return true;
        
    } catch (const std::exception& e) {
        setError("Failed to save folder metadata: " + std::string(e.what()));
        return false;
    }
}

std::optional<LockedFolderInfo> ProfileVault::loadFolderMetadata(const std::string& vault_location) const {
    try {
        std::string metadata_path = getFolderMetadataPath(vault_location);
        
        if (!fs::exists(metadata_path)) {
            return std::nullopt;
        }
        
        std::ifstream file(metadata_path);
        json folder_metadata;
        file >> folder_metadata;
        file.close();
        
        LockedFolderInfo info;
        info.original_path = folder_metadata["original_path"];
        info.vault_location = folder_metadata["vault_location"];
        
        int64_t lock_ms = folder_metadata["lock_timestamp"];
        info.lock_timestamp = std::chrono::system_clock::from_time_t(lock_ms / 1000);
        
        info.file_count = folder_metadata["file_count"];
        info.total_size = folder_metadata["total_size"];
        info.is_temporarily_unlocked = folder_metadata["is_temporarily_unlocked"];
        
        return info;
        
    } catch (const std::exception& e) {
        setError("Failed to load folder metadata: " + std::string(e.what()));
        return std::nullopt;
    }
}

bool ProfileVault::saveTemporaryUnlockState() {
    try {
        json temp_state;
        temp_state["unlocked_folders"] = temp_unlock_state_.unlocked_folders;
        temp_state["unlock_timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            temp_unlock_state_.unlock_timestamp.time_since_epoch()).count();
        
        std::ofstream file(temp_unlock_file_);
        if (!file) {
            setError("Failed to create temporary unlock file");
            return false;
        }
        
        file << temp_state.dump(2);
        file.close();
        
        fs::permissions(temp_unlock_file_, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace);
        
        return true;
        
    } catch (const std::exception& e) {
        setError("Failed to save temporary unlock state: " + std::string(e.what()));
        return false;
    }
}

bool ProfileVault::loadTemporaryUnlockState() {
    try {
        if (!fs::exists(temp_unlock_file_)) {
            return false;
        }
        
        std::ifstream file(temp_unlock_file_);
        json temp_state;
        file >> temp_state;
        file.close();
        
        temp_unlock_state_.unlocked_folders = temp_state["unlocked_folders"].get<std::vector<std::string>>();
        
        int64_t unlock_ms = temp_state["unlock_timestamp"];
        temp_unlock_state_.unlock_timestamp = std::chrono::system_clock::from_time_t(unlock_ms / 1000);
        
        return true;
        
    } catch (const std::exception& e) {
        setError("Failed to load temporary unlock state: " + std::string(e.what()));
        return false;
    }
}

bool ProfileVault::clearTemporaryUnlockState() {
    try {
        temp_unlock_state_.unlocked_folders.clear();
        
        if (fs::exists(temp_unlock_file_)) {
            fs::remove(temp_unlock_file_);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        setError("Failed to clear temporary unlock state: " + std::string(e.what()));
        return false;
    }
}

std::string ProfileVault::generateVaultLocation(const std::string& folder_path) const {
    return hashFolderPath(folder_path);
}

std::string ProfileVault::getVaultFolderPath(const std::string& vault_location) const {
    return vault_path_ + "/folders/" + vault_location;
}

std::string ProfileVault::getFolderMetadataPath(const std::string& vault_location) const {
    return vault_path_ + "/metadata/" + vault_location + ".json";
}

std::string ProfileVault::hashFolderPath(const std::string& folder_path) const {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(folder_path.c_str()), folder_path.length(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

bool ProfileVault::verifyFolderIntegrity(const std::string& vault_location) const {
    try {
        // Check if vault folder exists
        std::string vault_folder_path = getVaultFolderPath(vault_location);
        if (!fs::exists(vault_folder_path)) {
            return false;
        }
        
        // Check if metadata exists
        std::string metadata_path = getFolderMetadataPath(vault_location);
        if (!fs::exists(metadata_path)) {
            return false;
        }
        
        // Load and validate metadata
        auto folder_info = loadFolderMetadata(vault_location);
        if (!folder_info) {
            return false;
        }
        
        // Count files in vault folder
        size_t actual_file_count = 0;
        for (const auto& entry : fs::recursive_directory_iterator(vault_folder_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".enc") {
                actual_file_count++;
            }
        }
        
        // Verify file count matches metadata
        if (actual_file_count != folder_info->file_count) {
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        return false;
    }
}

bool ProfileVault::hideOriginalFolder(const std::string& folder_path) {
    try {
        // For now, we'll rename the folder to a hidden name
        // In a production system, this would use platform-specific hiding mechanisms
        std::string hidden_path = folder_path + ".phantomvault_hidden";
        
        if (fs::exists(hidden_path)) {
            fs::remove_all(hidden_path);
        }
        
        fs::rename(folder_path, hidden_path);
        
        // Set hidden attributes on supported platforms
        #ifdef PLATFORM_LINUX
        // On Linux, files starting with . are hidden
        // We could also use extended attributes
        #endif
        
        return true;
        
    } catch (const std::exception& e) {
        setError("Failed to hide folder: " + std::string(e.what()));
        return false;
    }
}

bool ProfileVault::restoreOriginalFolder(const std::string& folder_path, const std::string& /* vault_location */) {
    try {
        std::string hidden_path = folder_path + ".phantomvault_hidden";
        
        if (fs::exists(hidden_path)) {
            if (fs::exists(folder_path)) {
                fs::remove_all(folder_path);
            }
            fs::rename(hidden_path, folder_path);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        setError("Failed to restore folder: " + std::string(e.what()));
        return false;
    }
}

void ProfileVault::setError(const std::string& error) const {
    last_error_ = error;
}

void ProfileVault::clearError() const {
    last_error_.clear();
}

// VaultManager Implementation

VaultManager::VaultManager(const std::string& vault_root_path)
    : vault_root_path_(vault_root_path) {
    clearError();
}

VaultManager::~VaultManager() = default;

bool VaultManager::initializeVaultSystem() {
    clearError();
    
    try {
        // Create root vault directory
        if (!fs::exists(vault_root_path_)) {
            fs::create_directories(vault_root_path_);
            fs::permissions(vault_root_path_, fs::perms::owner_all, fs::perm_options::replace);
        }
        
        std::cout << "[VaultManager] Initialized vault system: " << vault_root_path_ << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        setError("Failed to initialize vault system: " + std::string(e.what()));
        return false;
    }
}

std::unique_ptr<ProfileVault> VaultManager::getProfileVault(const std::string& profile_id) {
    clearError();
    
    try {
        auto vault = std::make_unique<ProfileVault>(profile_id, vault_root_path_);
        
        if (!vault->initialize()) {
            setError("Failed to initialize profile vault: " + vault->getLastError());
            return nullptr;
        }
        
        return vault;
        
    } catch (const std::exception& e) {
        setError("Failed to get profile vault: " + std::string(e.what()));
        return nullptr;
    }
}

bool VaultManager::createProfileVault(const std::string& profile_id) {
    clearError();
    
    try {
        std::string profile_vault_path = getProfileVaultPath(profile_id);
        
        if (fs::exists(profile_vault_path)) {
            setError("Profile vault already exists");
            return false;
        }
        
        auto vault = std::make_unique<ProfileVault>(profile_id, vault_root_path_);
        
        if (!vault->createVaultStructure()) {
            setError("Failed to create vault structure: " + vault->getLastError());
            return false;
        }
        
        if (!vault->initialize()) {
            setError("Failed to initialize new vault: " + vault->getLastError());
            return false;
        }
        
        std::cout << "[VaultManager] Created profile vault: " << profile_id << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        setError("Failed to create profile vault: " + std::string(e.what()));
        return false;
    }
}

std::vector<std::string> VaultManager::getAllProfileVaults() const {
    std::vector<std::string> profile_ids;
    
    try {
        if (!fs::exists(vault_root_path_)) {
            return profile_ids;
        }
        
        for (const auto& entry : fs::directory_iterator(vault_root_path_)) {
            if (entry.is_directory()) {
                profile_ids.push_back(entry.path().filename().string());
            }
        }
        
    } catch (const std::exception& e) {
        setError("Failed to get profile vaults: " + std::string(e.what()));
    }
    
    return profile_ids;
}

bool VaultManager::deleteProfileVault(const std::string& profile_id, const std::string& master_key) {
    clearError();
    
    try {
        std::string profile_vault_path = getProfileVaultPath(profile_id);
        
        if (!fs::exists(profile_vault_path)) {
            setError("Profile vault does not exist");
            return false;
        }
        
        // Verify master key before deletion by trying to load the vault
        auto vault = getProfileVault(profile_id);
        if (!vault) {
            setError("Failed to load vault for verification");
            return false;
        }
        
        if (!vault->isValidMasterKey(master_key)) {
            setError("Invalid master key for vault deletion");
            return false;
        }
        
        // Remove the entire profile vault directory
        fs::remove_all(profile_vault_path);
        
        std::cout << "[VaultManager] Deleted profile vault: " << profile_id << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        setError("Failed to delete profile vault: " + std::string(e.what()));
        return false;
    }
}

bool VaultManager::relockAllTemporaryFolders() {
    clearError();
    
    try {
        auto profile_ids = getAllProfileVaults();
        bool all_success = true;
        
        for (const auto& profile_id : profile_ids) {
            auto vault = getProfileVault(profile_id);
            if (vault) {
                auto result = vault->relockTemporaryFolders();
                if (!result.success) {
                    all_success = false;
                    setError("Failed to relock folders for profile: " + profile_id);
                }
            }
        }
        
        return all_success;
        
    } catch (const std::exception& e) {
        setError("Failed to relock temporary folders: " + std::string(e.what()));
        return false;
    }
}

size_t VaultManager::getTotalVaultSize() const {
    size_t total_size = 0;
    
    try {
        if (!fs::exists(vault_root_path_)) {
            return 0;
        }
        
        for (const auto& entry : fs::recursive_directory_iterator(vault_root_path_)) {
            if (entry.is_regular_file()) {
                total_size += fs::file_size(entry.path());
            }
        }
        
    } catch (const std::exception& e) {
        setError("Failed to calculate vault size: " + std::string(e.what()));
    }
    
    return total_size;
}

bool VaultManager::performVaultMaintenance() {
    clearError();
    
    try {
        auto profile_ids = getAllProfileVaults();
        bool all_success = true;
        
        for (const auto& profile_id : profile_ids) {
            auto vault = getProfileVault(profile_id);
            if (vault) {
                // Perform basic maintenance tasks
                if (!vault->validateVaultIntegrity()) {
                    std::cout << "[VaultManager] Integrity issues found in vault: " << profile_id << std::endl;
                    
                    // Attempt cleanup
                    if (!vault->cleanupCorruptedEntries()) {
                        all_success = false;
                        setError("Failed to cleanup vault: " + profile_id);
                    }
                }
            }
        }
        
        return all_success;
        
    } catch (const std::exception& e) {
        setError("Failed to perform vault maintenance: " + std::string(e.what()));
        return false;
    }
}

bool VaultManager::validateAllVaults() const {
    clearError();
    
    try {
        auto profile_ids = getAllProfileVaults();
        
        for (const auto& profile_id : profile_ids) {
            // Create a temporary non-const instance for validation
            VaultManager* non_const_this = const_cast<VaultManager*>(this);
            auto vault = non_const_this->getProfileVault(profile_id);
            if (!vault) {
                setError("Failed to load vault: " + profile_id);
                return false;
            }
            
            if (!vault->validateVaultIntegrity()) {
                setError("Vault integrity validation failed: " + profile_id);
                return false;
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        setError("Failed to validate vaults: " + std::string(e.what()));
        return false;
    }
}

std::string VaultManager::getProfileVaultPath(const std::string& profile_id) const {
    return vault_root_path_ + "/" + profile_id;
}

void VaultManager::setError(const std::string& error) const {
    last_error_ = error;
}

void VaultManager::clearError() const {
    last_error_.clear();
}

} // namespace PhantomVault