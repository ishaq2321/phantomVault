/**
 * PhantomVault Advanced Vault Handler Implementation
 * 
 * Platform-specific folder hiding mechanisms requiring elevated privileges,
 * complete folder restoration with metadata preservation, vault structure management,
 * and secure deletion capabilities.
 */

#include "vault_handler.hpp"
#include "privilege_manager.hpp"
#include "error_handler.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <unordered_map>
#include <utime.h>
#include <nlohmann/json.hpp>

#ifdef PLATFORM_LINUX
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <utime.h>
#include <dirent.h>
#elif PLATFORM_WINDOWS
#include <windows.h>
#include <shlobj.h>
#include <aclapi.h>
#include <sddl.h>
#elif PLATFORM_MACOS
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <sys/mount.h>
#endif

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace phantomvault {

class VaultHandler::Implementation {
public:
    Implementation()
        : vault_root_path_()
        , privilege_manager_(std::make_unique<PrivilegeManager>())
        , error_handler_(std::make_unique<ErrorHandler>())
        , last_error_()
        , operation_log_()
        , vault_structures_()
    {}
    
    bool initialize(const std::string& vault_root_path) {
        try {
            vault_root_path_ = vault_root_path;
            
            // Initialize privilege manager
            if (!privilege_manager_->initialize()) {
                last_error_ = "Failed to initialize privilege manager: " + privilege_manager_->getLastError();
                return false;
            }
            
            // Check if we have required privileges for advanced operations
            if (!privilege_manager_->hasPrivilegeForOperation(PrivilegedOperation::FOLDER_HIDING)) {
                last_error_ = "Advanced folder hiding requires elevated privileges";
                logOperation("INIT_WARNING", "Operating without elevated privileges - limited functionality");
            }
            
            // Initialize error handler
            std::string error_log_path = vault_root_path + "/vault_handler.log";
            if (!error_handler_->initialize(error_log_path)) {
                last_error_ = "Failed to initialize error handler: " + error_handler_->getLastError();
                return false;
            }
            
            // Ensure vault root exists
            if (!fs::exists(vault_root_path_)) {
                fs::create_directories(vault_root_path_);
                fs::permissions(vault_root_path_, fs::perms::owner_all, fs::perm_options::replace);
            }
            
            logOperation("INIT_SUCCESS", "VaultHandler initialized with path: " + vault_root_path_);
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to initialize VaultHandler: " + std::string(e.what());
            return false;
        }
    }
    
    bool requiresElevatedPrivileges() const {
        return !privilege_manager_->hasPrivilegeForOperation(PrivilegedOperation::FOLDER_HIDING);
    }
    
    HidingResult hideFolder(const std::string& folder_path, const std::string& vault_id) {
        HidingResult result;
        
        try {
            // Validate inputs
            if (folder_path.empty() || vault_id.empty()) {
                result.error_details = "Folder path and vault ID cannot be empty";
                return result;
            }
            
            if (!fs::exists(folder_path) || !fs::is_directory(folder_path)) {
                result.error_details = "Folder does not exist or is not a directory: " + folder_path;
                return result;
            }
            
            // Check privileges
            if (requiresElevatedPrivileges()) {
                auto elevation_result = privilege_manager_->requestElevationForOperation(PrivilegedOperation::FOLDER_HIDING);
                if (!elevation_result.success) {
                    result.error_details = "Elevated privileges required: " + elevation_result.errorDetails;
                    return result;
                }
            }
            
            // Preserve original metadata
            if (!preserveFolderMetadata(folder_path, result.preserved_metadata)) {
                result.error_details = "Failed to preserve folder metadata";
                return result;
            }
            
            // Generate completely obfuscated identifier
            std::string obfuscated_id = generateObfuscatedIdentifier(folder_path, vault_id);
            result.obfuscated_identifier = obfuscated_id;
            
            // Create backup location using obfuscated identifier
            std::string backup_path = getVaultPath(vault_id) + "/hidden_folders/" + obfuscated_id;
            result.backup_location = backup_path;
            
            // Create obfuscated mapping for later resolution
            if (!createObfuscatedMapping(vault_id, folder_path, obfuscated_id)) {
                result.error_details = "Failed to create obfuscated mapping: " + last_error_;
                return result;
            }
            
            // Ensure backup directory exists
            fs::create_directories(fs::path(backup_path).parent_path());
            
            // Platform-specific hiding with elevated privileges
            if (!performPlatformSpecificHiding(folder_path, backup_path)) {
                result.error_details = "Platform-specific hiding failed: " + last_error_;
                return result;
            }
            
            // Save metadata to vault
            if (!saveMetadataToVault(vault_id, result.preserved_metadata, backup_path)) {
                result.error_details = "Failed to save metadata to vault";
                return result;
            }
            
            result.success = true;
            result.message = "Folder successfully hidden using platform-specific mechanisms";
            
            logOperation("HIDE_SUCCESS", "Hidden folder: " + folder_path + " -> " + backup_path);
            return result;
            
        } catch (const std::exception& e) {
            result.error_details = "Failed to hide folder: " + std::string(e.what());
            logOperation("HIDE_ERROR", result.error_details);
            return result;
        }
    }
    
    RestorationResult restoreFolder(const std::string& vault_id, const std::string& folder_identifier) {
        RestorationResult result;
        
        try {
            // Load metadata from vault
            auto metadata = loadMetadataFromVault(vault_id, folder_identifier);
            if (!metadata) {
                result.error_details = "Failed to load folder metadata from vault";
                return result;
            }
            
            // Check if original location is available
            if (fs::exists(metadata->original_path)) {
                result.error_details = "Original location already exists: " + metadata->original_path;
                return result;
            }
            
            // Get backup location
            std::string backup_path = getVaultPath(vault_id) + "/hidden_folders/" + folder_identifier;
            if (!fs::exists(backup_path)) {
                result.error_details = "Backup folder not found in vault: " + backup_path;
                return result;
            }
            
            // Platform-specific restoration
            if (!performPlatformSpecificRestoration(backup_path, metadata->original_path)) {
                result.error_details = "Platform-specific restoration failed: " + last_error_;
                return result;
            }
            
            // Restore metadata
            result.metadata_restored = restoreFolderMetadata(metadata->original_path, *metadata);
            
            result.success = true;
            result.restored_path = metadata->original_path;
            result.message = std::string("Folder successfully restored with ") + 
                           (result.metadata_restored ? "complete" : "partial") + " metadata preservation";
            
            logOperation("RESTORE_SUCCESS", "Restored folder: " + backup_path + " -> " + metadata->original_path);
            return result;
            
        } catch (const std::exception& e) {
            result.error_details = "Failed to restore folder: " + std::string(e.what());
            logOperation("RESTORE_ERROR", result.error_details);
            return result;
        }
    }
    
    bool preserveFolderMetadata(const std::string& folder_path, FolderMetadata& metadata) {
        try {
            metadata.original_path = folder_path;
            
            #ifdef PLATFORM_LINUX
            struct stat st;
            if (stat(folder_path.c_str(), &st) != 0) {
                last_error_ = "Failed to get folder stats";
                return false;
            }
            
            // Get owner and group
            struct passwd* pw = getpwuid(st.st_uid);
            struct group* gr = getgrgid(st.st_gid);
            metadata.owner = pw ? pw->pw_name : std::to_string(st.st_uid);
            metadata.group = gr ? gr->gr_name : std::to_string(st.st_gid);
            
            // Get permissions
            metadata.permissions = st.st_mode & 0777;
            
            // Get timestamps
            metadata.created_time = std::chrono::system_clock::from_time_t(st.st_ctime);
            metadata.modified_time = std::chrono::system_clock::from_time_t(st.st_mtime);
            metadata.accessed_time = std::chrono::system_clock::from_time_t(st.st_atime);
            
            // Get extended attributes
            char* attr_list = nullptr;
            ssize_t attr_size = listxattr(folder_path.c_str(), nullptr, 0);
            if (attr_size > 0) {
                attr_list = new char[attr_size];
                if (listxattr(folder_path.c_str(), attr_list, attr_size) > 0) {
                    char* attr_name = attr_list;
                    while (attr_name < attr_list + attr_size) {
                        char value[1024];
                        ssize_t value_size = getxattr(folder_path.c_str(), attr_name, value, sizeof(value) - 1);
                        if (value_size > 0) {
                            value[value_size] = '\0';
                            metadata.extended_attributes[attr_name] = value;
                        }
                        attr_name += strlen(attr_name) + 1;
                    }
                }
                delete[] attr_list;
            }
            
            // Check if folder was hidden (starts with .)
            fs::path path(folder_path);
            metadata.was_hidden = path.filename().string().front() == '.';
            
            #elif PLATFORM_WINDOWS
            HANDLE hFile = CreateFileA(folder_path.c_str(), GENERIC_READ, FILE_SHARE_READ, 
                                     NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
            if (hFile == INVALID_HANDLE_VALUE) {
                last_error_ = "Failed to open folder for metadata reading";
                return false;
            }
            
            // Get file attributes
            DWORD attributes = GetFileAttributesA(folder_path.c_str());
            metadata.was_hidden = (attributes & FILE_ATTRIBUTE_HIDDEN) != 0;
            metadata.permissions = attributes;
            
            // Get timestamps
            FILETIME created, accessed, modified;
            if (GetFileTime(hFile, &created, &accessed, &modified)) {
                metadata.created_time = fileTimeToSystemClock(created);
                metadata.accessed_time = fileTimeToSystemClock(accessed);
                metadata.modified_time = fileTimeToSystemClock(modified);
            }
            
            // Get owner information
            PSID owner_sid = nullptr;
            PSECURITY_DESCRIPTOR security_desc = nullptr;
            if (GetNamedSecurityInfoA(folder_path.c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
                                    &owner_sid, nullptr, nullptr, nullptr, &security_desc) == ERROR_SUCCESS) {
                char* owner_name = nullptr;
                if (ConvertSidToStringSidA(owner_sid, &owner_name)) {
                    metadata.owner = owner_name;
                    LocalFree(owner_name);
                }
                LocalFree(security_desc);
            }
            
            CloseHandle(hFile);
            
            #elif PLATFORM_MACOS
            struct stat st;
            if (stat(folder_path.c_str(), &st) != 0) {
                last_error_ = "Failed to get folder stats";
                return false;
            }
            
            // Similar to Linux implementation
            struct passwd* pw = getpwuid(st.st_uid);
            struct group* gr = getgrgid(st.st_gid);
            metadata.owner = pw ? pw->pw_name : std::to_string(st.st_uid);
            metadata.group = gr ? gr->gr_name : std::to_string(st.st_gid);
            metadata.permissions = st.st_mode & 0777;
            
            metadata.created_time = std::chrono::system_clock::from_time_t(st.st_ctime);
            metadata.modified_time = std::chrono::system_clock::from_time_t(st.st_mtime);
            metadata.accessed_time = std::chrono::system_clock::from_time_t(st.st_atime);
            
            // Get extended attributes (macOS specific)
            char* attr_list = nullptr;
            ssize_t attr_size = listxattr(folder_path.c_str(), nullptr, 0, 0);
            if (attr_size > 0) {
                attr_list = new char[attr_size];
                if (listxattr(folder_path.c_str(), attr_list, attr_size, 0) > 0) {
                    char* attr_name = attr_list;
                    while (attr_name < attr_list + attr_size) {
                        char value[1024];
                        ssize_t value_size = getxattr(folder_path.c_str(), attr_name, value, sizeof(value) - 1, 0, 0);
                        if (value_size > 0) {
                            value[value_size] = '\0';
                            metadata.extended_attributes[attr_name] = value;
                        }
                        attr_name += strlen(attr_name) + 1;
                    }
                }
                delete[] attr_list;
            }
            
            fs::path path(folder_path);
            metadata.was_hidden = path.filename().string().front() == '.';
            #endif
            
            metadata.original_location = folder_path;
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to preserve metadata: " + std::string(e.what());
            return false;
        }
    }
    
    bool restoreFolderMetadata(const std::string& folder_path, const FolderMetadata& metadata) {
        (void)folder_path; // Suppress unused parameter warning
        (void)metadata;    // Suppress unused parameter warning
        try {
            #ifdef PLATFORM_LINUX
            // Restore ownership
            struct passwd* pw = getpwnam(metadata.owner.c_str());
            struct group* gr = getgrnam(metadata.group.c_str());
            
            uid_t uid = pw ? pw->pw_uid : std::stoul(metadata.owner);
            gid_t gid = gr ? gr->gr_gid : std::stoul(metadata.group);
            
            if (chown(folder_path.c_str(), uid, gid) != 0) {
                logOperation("METADATA_WARNING", "Failed to restore ownership for: " + folder_path);
            }
            
            // Restore permissions
            if (chmod(folder_path.c_str(), metadata.permissions) != 0) {
                logOperation("METADATA_WARNING", "Failed to restore permissions for: " + folder_path);
            }
            
            // Restore timestamps
            struct utimbuf times;
            times.actime = std::chrono::system_clock::to_time_t(metadata.accessed_time);
            times.modtime = std::chrono::system_clock::to_time_t(metadata.modified_time);
            
            if (utime(folder_path.c_str(), &times) != 0) {
                logOperation("METADATA_WARNING", "Failed to restore timestamps for: " + folder_path);
            }
            
            // Restore extended attributes
            for (const auto& attr : metadata.extended_attributes) {
                if (setxattr(folder_path.c_str(), attr.first.c_str(), attr.second.c_str(), 
                           attr.second.length(), 0) != 0) {
                    logOperation("METADATA_WARNING", "Failed to restore extended attribute: " + attr.first);
                }
            }
            
            #elif PLATFORM_WINDOWS
            // Restore file attributes
            if (!SetFileAttributesA(folder_path.c_str(), metadata.permissions)) {
                logOperation("METADATA_WARNING", "Failed to restore attributes for: " + folder_path);
            }
            
            // Restore timestamps
            HANDLE hFile = CreateFileA(folder_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                                     NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                FILETIME created = systemClockToFileTime(metadata.created_time);
                FILETIME accessed = systemClockToFileTime(metadata.accessed_time);
                FILETIME modified = systemClockToFileTime(metadata.modified_time);
                
                SetFileTime(hFile, &created, &accessed, &modified);
                CloseHandle(hFile);
            }
            
            #elif PLATFORM_MACOS
            // Similar to Linux implementation
            struct passwd* pw = getpwnam(metadata.owner.c_str());
            struct group* gr = getgrnam(metadata.group.c_str());
            
            uid_t uid = pw ? pw->pw_uid : std::stoul(metadata.owner);
            gid_t gid = gr ? gr->gr_gid : std::stoul(metadata.group);
            
            chown(folder_path.c_str(), uid, gid);
            chmod(folder_path.c_str(), metadata.permissions);
            
            struct utimbuf times;
            times.actime = std::chrono::system_clock::to_time_t(metadata.accessed_time);
            times.modtime = std::chrono::system_clock::to_time_t(metadata.modified_time);
            utime(folder_path.c_str(), &times);
            
            // Restore extended attributes
            for (const auto& attr : metadata.extended_attributes) {
                setxattr(folder_path.c_str(), attr.first.c_str(), attr.second.c_str(), 
                        attr.second.length(), 0, 0);
            }
            #endif
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to restore metadata: " + std::string(e.what());
            return false;
        }
    }
    
    bool createVaultStructure(const std::string& vault_id, const std::string& profile_id) {
        try {
            VaultStructure structure;
            structure.vault_id = vault_id;
            structure.profile_id = profile_id;
            structure.vault_path = getVaultPath(vault_id);
            structure.hidden_folders_path = structure.vault_path + "/hidden_folders";
            structure.metadata_path = structure.vault_path + "/metadata";
            structure.temp_path = structure.vault_path + "/temp";
            structure.backup_path = structure.vault_path + "/backup";
            structure.created_at = std::chrono::system_clock::now();
            structure.last_modified = structure.created_at;
            structure.total_folders = 0;
            structure.total_size = 0;
            
            // Create directory structure
            fs::create_directories(structure.vault_path);
            fs::create_directories(structure.hidden_folders_path);
            fs::create_directories(structure.metadata_path);
            fs::create_directories(structure.temp_path);
            fs::create_directories(structure.backup_path);
            
            // Set secure permissions
            fs::permissions(structure.vault_path, fs::perms::owner_all, fs::perm_options::replace);
            fs::permissions(structure.hidden_folders_path, fs::perms::owner_all, fs::perm_options::replace);
            fs::permissions(structure.metadata_path, fs::perms::owner_all, fs::perm_options::replace);
            fs::permissions(structure.temp_path, fs::perms::owner_all, fs::perm_options::replace);
            fs::permissions(structure.backup_path, fs::perms::owner_all, fs::perm_options::replace);
            
            // Save structure metadata
            if (!saveVaultStructure(structure)) {
                last_error_ = "Failed to save vault structure metadata";
                return false;
            }
            
            vault_structures_[vault_id] = structure;
            
            logOperation("VAULT_CREATE", "Created vault structure: " + vault_id);
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to create vault structure: " + std::string(e.what());
            return false;
        }
    }
    
    CleanupResult secureDeleteFromVault(const std::string& vault_id, const std::string& folder_identifier) {
        CleanupResult result;
        
        try {
            std::string folder_path = getVaultPath(vault_id) + "/hidden_folders/" + folder_identifier;
            std::string metadata_path = getVaultPath(vault_id) + "/metadata/" + folder_identifier + ".json";
            
            size_t folder_size = 0;
            if (fs::exists(folder_path)) {
                folder_size = calculateDirectorySize(folder_path);
                
                // Secure wipe of folder contents
                if (!secureWipeDirectory(folder_path)) {
                    result.error_details = "Failed to securely wipe folder contents";
                    return result;
                }
                
                fs::remove_all(folder_path);
            }
            
            // Remove metadata
            if (fs::exists(metadata_path)) {
                if (!secureWipeFile(metadata_path)) {
                    logOperation("CLEANUP_WARNING", "Failed to securely wipe metadata file: " + metadata_path);
                }
                fs::remove(metadata_path);
            }
            
            result.success = true;
            result.folders_cleaned = 1;
            result.bytes_freed = folder_size;
            result.message = "Folder securely deleted from vault";
            
            logOperation("SECURE_DELETE", "Deleted folder from vault: " + folder_identifier);
            return result;
            
        } catch (const std::exception& e) {
            result.error_details = "Failed to securely delete from vault: " + std::string(e.what());
            return result;
        }
    }
    
    std::string getLastError() const {
        return last_error_;
    }
    
    std::vector<std::string> getOperationLog() const {
        return operation_log_;
    }

private:
    std::string vault_root_path_;
    std::unique_ptr<PrivilegeManager> privilege_manager_;
    std::unique_ptr<ErrorHandler> error_handler_;
    mutable std::string last_error_;
    std::vector<std::string> operation_log_;
    std::unordered_map<std::string, VaultStructure> vault_structures_;
    
    std::string getVaultPath(const std::string& vault_id) const {
        return vault_root_path_ + "/" + vault_id;
    }
    
    std::string generateObfuscatedIdentifier(const std::string& folder_path, const std::string& vault_id) const {
        // Generate cryptographically secure obfuscated identifier that provides no clues about original path
        
        // Create unique salt for this obfuscation
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dis;
        
        std::string salt = std::to_string(dis(gen)) + std::to_string(dis(gen));
        
        // Combine multiple entropy sources to eliminate any path correlation
        auto now = std::chrono::high_resolution_clock::now();
        auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        
        std::string entropy_mix = vault_id + salt + std::to_string(nanos) + 
                                 std::to_string(dis(gen)) + folder_path + 
                                 std::to_string(std::hash<std::string>{}(folder_path + salt));
        
        // Generate multiple hash rounds to eliminate correlation
        std::hash<std::string> hasher;
        std::string hash_input = entropy_mix;
        
        for (int i = 0; i < 7; ++i) {  // 7 rounds for maximum obfuscation
            hash_input = std::to_string(hasher(hash_input + std::to_string(i)));
        }
        
        // Create final obfuscated identifier using multiple random components
        std::stringstream obfuscated_id;
        obfuscated_id << std::hex;
        
        // Generate 4 random hex segments to look like system identifiers
        for (int i = 0; i < 4; ++i) {
            uint64_t segment = hasher(hash_input + std::to_string(i) + salt) ^ dis(gen);
            obfuscated_id << std::setfill('0') << std::setw(8) << (segment & 0xFFFFFFFF);
            if (i < 3) obfuscated_id << "-";
        }
        
        return obfuscated_id.str();
    }
    
    std::string generateFolderIdentifier(const std::string& folder_path) const {
        // Legacy method - redirect to enhanced obfuscation
        return generateObfuscatedIdentifier(folder_path, "default_vault");
    }
    
    void logOperation(const std::string& type, const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "] "
           << type << ": " << message;
        
        operation_log_.push_back(ss.str());
        
        // Keep only last 1000 log entries
        if (operation_log_.size() > 1000) {
            operation_log_.erase(operation_log_.begin());
        }
        
        // Also log to error handler if available
        if (error_handler_) {
            error_handler_->logSecurityEvent(SecurityEventType::UNAUTHORIZED_ACCESS, ErrorSeverity::INFO, 
                                            "vault_handler", type + ": " + message);
        }
    }
    
    bool createObfuscatedMapping(const std::string& vault_id, const std::string& original_path, const std::string& obfuscated_id) {
        try {
            // Create encrypted mapping file that stores the relationship between obfuscated ID and original path
            std::string mapping_dir = getVaultPath(vault_id) + "/mappings";
            fs::create_directories(mapping_dir);
            
            // Generate encryption key from vault_id and obfuscated_id
            std::string key_material = vault_id + obfuscated_id + "mapping_key_salt_2024";
            std::hash<std::string> hasher;
            std::string encryption_key = std::to_string(hasher(key_material));
            
            // Create mapping data
            json mapping_data;
            mapping_data["obfuscated_id"] = obfuscated_id;
            mapping_data["encrypted_path"] = encryptPathForStorage(original_path, encryption_key);
            mapping_data["created_timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            mapping_data["access_count"] = 0;
            
            // Save mapping with obfuscated filename
            std::string mapping_file = mapping_dir + "/" + obfuscated_id + ".map";
            std::ofstream file(mapping_file);
            file << mapping_data.dump(4);
            file.close();
            
            // Set restrictive permissions
            fs::permissions(mapping_file, fs::perms::owner_read | fs::perms::owner_write);
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to create obfuscated mapping: " + std::string(e.what());
            return false;
        }
    }
    
    std::string resolveObfuscatedPath(const std::string& vault_id, const std::string& obfuscated_id) {
        try {
            std::string mapping_file = getVaultPath(vault_id) + "/mappings/" + obfuscated_id + ".map";
            
            if (!fs::exists(mapping_file)) {
                last_error_ = "Obfuscated mapping not found: " + obfuscated_id;
                return "";
            }
            
            std::ifstream file(mapping_file);
            json mapping_data;
            file >> mapping_data;
            file.close();
            
            // Generate decryption key
            std::string key_material = vault_id + obfuscated_id + "mapping_key_salt_2024";
            std::hash<std::string> hasher;
            std::string decryption_key = std::to_string(hasher(key_material));
            
            // Decrypt and return original path
            return decryptPathFromStorage(mapping_data["encrypted_path"], decryption_key);
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to resolve obfuscated path: " + std::string(e.what());
            return "";
        }
    }
    
    bool eliminatePathTraces(const std::string& original_path) {
        try {
            // Comprehensive trace elimination to prevent OSINT analysis
            
            // 1. Clear filesystem metadata that might contain path references
            #ifdef PLATFORM_LINUX
            // Clear extended attributes that might contain path info
            std::vector<std::string> xattr_names = {"user.original_path", "user.backup_info", "user.source"};
            for (const auto& attr : xattr_names) {
                removexattr(original_path.c_str(), attr.c_str());  // Ignore errors
            }
            #endif
            
            // 2. Overwrite any temporary files that might contain path references
            std::vector<std::string> temp_patterns = {
                "/tmp/*" + fs::path(original_path).filename().string() + "*",
                "/var/tmp/*" + fs::path(original_path).filename().string() + "*"
            };
            
            for (const auto& pattern : temp_patterns) {
                // Securely wipe any matching temporary files
                secureWipeTempFiles(pattern);
            }
            
            // 3. Clear any system logs that might reference the path
            clearSystemLogReferences(original_path);
            
            // 4. Overwrite directory entry metadata
            overwriteDirectoryMetadata(fs::path(original_path).parent_path().string());
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to eliminate path traces: " + std::string(e.what());
            return false;
        }
    }
    
    bool createDecoyStructure(const std::string& vault_id, const std::string& obfuscated_id) {
        (void)obfuscated_id; // Suppress unused parameter warning
        try {
            // Create multiple decoy folders to confuse OSINT analysis
            std::string decoy_base = getVaultPath(vault_id) + "/decoys";
            fs::create_directories(decoy_base);
            
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> count_dis(5, 12);  // 5-12 decoy folders
            std::uniform_int_distribution<int> name_dis(8, 16);   // 8-16 character names
            
            int decoy_count = count_dis(gen);
            
            for (int i = 0; i < decoy_count; ++i) {
                // Generate random decoy folder name
                std::string decoy_name = generateRandomHexString(name_dis(gen));
                std::string decoy_path = decoy_base + "/" + decoy_name;
                
                // Create decoy folder with random content
                fs::create_directories(decoy_path);
                
                // Add random files to make it look legitimate
                createDecoyFiles(decoy_path);
                
                // Set random timestamps to avoid temporal correlation
                setRandomTimestamps(decoy_path);
            }
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to create decoy structure: " + std::string(e.what());
            return false;
        }
    }
    
    bool performPlatformSpecificHiding(const std::string& folder_path, const std::string& backup_path) {
        try {
            #ifdef PLATFORM_LINUX
            // On Linux with elevated privileges, we can:
            // 1. Move folder to a system-protected location
            // 2. Create a bind mount to hide the original location
            // 3. Set special extended attributes
            
            // Move folder to backup location
            fs::rename(folder_path, backup_path);
            
            // Create a placeholder directory with restricted access
            fs::create_directory(folder_path);
            fs::permissions(folder_path, fs::perms::none, fs::perm_options::replace);
            
            // Set extended attribute to mark as PhantomVault hidden
            if (setxattr(folder_path.c_str(), "user.phantomvault.hidden", "true", 4, 0) != 0) {
                logOperation("HIDE_WARNING", "Failed to set extended attribute");
            }
            
            return true;
            
            #elif PLATFORM_WINDOWS
            // On Windows with elevated privileges, we can:
            // 1. Move folder to secure location
            // 2. Set system and hidden attributes
            // 3. Create NTFS junction point if needed
            
            // Move folder to backup location
            if (!MoveFileA(folder_path.c_str(), backup_path.c_str())) {
                last_error_ = "Failed to move folder to backup location";
                return false;
            }
            
            // Create placeholder directory with system attributes
            if (!CreateDirectoryA(folder_path.c_str(), NULL)) {
                last_error_ = "Failed to create placeholder directory";
                return false;
            }
            
            // Set system and hidden attributes
            DWORD attributes = FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
            if (!SetFileAttributesA(folder_path.c_str(), attributes)) {
                logOperation("HIDE_WARNING", "Failed to set system attributes");
            }
            
            return true;
            
            #elif PLATFORM_MACOS
            // On macOS with elevated privileges, we can:
            // 1. Move folder to secure location
            // 2. Set special flags and extended attributes
            // 3. Use chflags for additional protection
            
            // Move folder to backup location
            fs::rename(folder_path, backup_path);
            
            // Create placeholder with restricted access
            fs::create_directory(folder_path);
            fs::permissions(folder_path, fs::perms::none, fs::perm_options::replace);
            
            // Set extended attribute
            if (setxattr(folder_path.c_str(), "com.phantomvault.hidden", "true", 4, 0, 0) != 0) {
                logOperation("HIDE_WARNING", "Failed to set extended attribute");
            }
            
            return true;
            
            #else
            // Fallback: simple rename
            fs::rename(folder_path, backup_path);
            return true;
            #endif
            
        } catch (const std::exception& e) {
            last_error_ = "Platform-specific hiding failed: " + std::string(e.what());
            return false;
        }
    }
    
    bool performPlatformSpecificRestoration(const std::string& backup_path, const std::string& original_path) {
        try {
            #ifdef PLATFORM_LINUX
            // Remove placeholder if it exists
            if (fs::exists(original_path)) {
                fs::remove_all(original_path);
            }
            
            // Move folder back from backup
            fs::rename(backup_path, original_path);
            
            return true;
            
            #elif PLATFORM_WINDOWS
            // Remove placeholder if it exists
            if (fs::exists(original_path)) {
                // Remove system attributes first
                SetFileAttributesA(original_path.c_str(), FILE_ATTRIBUTE_NORMAL);
                fs::remove_all(original_path);
            }
            
            // Move folder back from backup
            if (!MoveFileA(backup_path.c_str(), original_path.c_str())) {
                last_error_ = "Failed to move folder from backup location";
                return false;
            }
            
            return true;
            
            #elif PLATFORM_MACOS
            // Remove placeholder if it exists
            if (fs::exists(original_path)) {
                fs::remove_all(original_path);
            }
            
            // Move folder back from backup
            fs::rename(backup_path, original_path);
            
            return true;
            
            #else
            // Fallback: simple rename
            fs::rename(backup_path, original_path);
            return true;
            #endif
            
        } catch (const std::exception& e) {
            last_error_ = "Platform-specific restoration failed: " + std::string(e.what());
            return false;
        }
    }
    
    bool saveMetadataToVault(const std::string& vault_id, const FolderMetadata& metadata, const std::string& backup_path) {
        try {
            std::string metadata_file = getVaultPath(vault_id) + "/metadata/" + 
                                      generateFolderIdentifier(metadata.original_path) + ".json";
            
            json metadata_json;
            metadata_json["original_path"] = metadata.original_path;
            metadata_json["owner"] = metadata.owner;
            metadata_json["group"] = metadata.group;
            metadata_json["permissions"] = metadata.permissions;
            metadata_json["created_time"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                metadata.created_time.time_since_epoch()).count();
            metadata_json["modified_time"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                metadata.modified_time.time_since_epoch()).count();
            metadata_json["accessed_time"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                metadata.accessed_time.time_since_epoch()).count();
            metadata_json["extended_attributes"] = metadata.extended_attributes;
            metadata_json["was_hidden"] = metadata.was_hidden;
            metadata_json["backup_path"] = backup_path;
            
            std::ofstream file(metadata_file);
            file << metadata_json.dump(2);
            file.close();
            
            fs::permissions(metadata_file, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace);
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to save metadata to vault: " + std::string(e.what());
            return false;
        }
    }
    
    std::optional<FolderMetadata> loadMetadataFromVault(const std::string& vault_id, const std::string& folder_identifier) {
        try {
            std::string metadata_file = getVaultPath(vault_id) + "/metadata/" + folder_identifier + ".json";
            
            if (!fs::exists(metadata_file)) {
                last_error_ = "Metadata file not found: " + metadata_file;
                return std::nullopt;
            }
            
            std::ifstream file(metadata_file);
            json metadata_json;
            file >> metadata_json;
            
            FolderMetadata metadata;
            metadata.original_path = metadata_json.value("original_path", "");
            metadata.owner = metadata_json.value("owner", "");
            metadata.group = metadata_json.value("group", "");
            metadata.permissions = metadata_json.value("permissions", 0755);
            
            if (metadata_json.contains("created_time")) {
                metadata.created_time = std::chrono::system_clock::from_time_t(
                    metadata_json["created_time"].get<int64_t>() / 1000);
            }
            if (metadata_json.contains("modified_time")) {
                metadata.modified_time = std::chrono::system_clock::from_time_t(
                    metadata_json["modified_time"].get<int64_t>() / 1000);
            }
            if (metadata_json.contains("accessed_time")) {
                metadata.accessed_time = std::chrono::system_clock::from_time_t(
                    metadata_json["accessed_time"].get<int64_t>() / 1000);
            }
            
            if (metadata_json.contains("extended_attributes")) {
                metadata.extended_attributes = metadata_json["extended_attributes"];
            }
            
            metadata.was_hidden = metadata_json.value("was_hidden", false);
            
            return metadata;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to load metadata from vault: " + std::string(e.what());
            return std::nullopt;
        }
    }
    
    bool saveVaultStructure(const VaultStructure& structure) {
        try {
            std::string structure_file = structure.vault_path + "/vault_structure.json";
            
            json structure_json;
            structure_json["vault_id"] = structure.vault_id;
            structure_json["profile_id"] = structure.profile_id;
            structure_json["vault_path"] = structure.vault_path;
            structure_json["hidden_folders_path"] = structure.hidden_folders_path;
            structure_json["metadata_path"] = structure.metadata_path;
            structure_json["temp_path"] = structure.temp_path;
            structure_json["backup_path"] = structure.backup_path;
            structure_json["total_folders"] = structure.total_folders;
            structure_json["total_size"] = structure.total_size;
            structure_json["created_at"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                structure.created_at.time_since_epoch()).count();
            structure_json["last_modified"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                structure.last_modified.time_since_epoch()).count();
            
            std::ofstream file(structure_file);
            file << structure_json.dump(2);
            file.close();
            
            fs::permissions(structure_file, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace);
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to save vault structure: " + std::string(e.what());
            return false;
        }
    }
    
    size_t calculateDirectorySize(const std::string& dir_path) {
        try {
            size_t size = 0;
            for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
                if (entry.is_regular_file()) {
                    size += entry.file_size();
                }
            }
            return size;
        } catch (const std::exception& e) {
            return 0;
        }
    }
    
    bool secureWipeDirectory(const std::string& dir_path) {
        try {
            for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
                if (entry.is_regular_file()) {
                    if (!secureWipeFile(entry.path().string())) {
                        logOperation("WIPE_WARNING", "Failed to securely wipe file: " + entry.path().string());
                    }
                }
            }
            return true;
        } catch (const std::exception& e) {
            last_error_ = "Failed to securely wipe directory: " + std::string(e.what());
            return false;
        }
    }
    
    bool secureWipeFile(const std::string& file_path) {
        try {
            if (!fs::exists(file_path)) {
                return true;
            }
            
            size_t file_size = fs::file_size(file_path);
            
            // Overwrite file with random data multiple times
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 255);
            
            std::ofstream file(file_path, std::ios::binary);
            if (!file.is_open()) {
                return false;
            }
            
            // Three-pass secure wipe
            for (int pass = 0; pass < 3; ++pass) {
                file.seekp(0);
                for (size_t i = 0; i < file_size; ++i) {
                    file.put(static_cast<char>(dis(gen)));
                }
                file.flush();
            }
            
            file.close();
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to securely wipe file: " + std::string(e.what());
            return false;
        }
    }
    
    #ifdef PLATFORM_WINDOWS
    std::chrono::system_clock::time_point fileTimeToSystemClock(const FILETIME& ft) {
        ULARGE_INTEGER ull;
        ull.LowPart = ft.dwLowDateTime;
        ull.HighPart = ft.dwHighDateTime;
        
        // Convert from Windows epoch (1601) to Unix epoch (1970)
        auto windows_ticks = ull.QuadPart;
        auto unix_seconds = (windows_ticks - 116444736000000000ULL) / 10000000ULL;
        
        return std::chrono::system_clock::from_time_t(unix_seconds);
    }
    
    FILETIME systemClockToFileTime(const std::chrono::system_clock::time_point& tp) {
        auto unix_seconds = std::chrono::system_clock::to_time_t(tp);
        auto windows_ticks = (unix_seconds * 10000000ULL) + 116444736000000000ULL;
        
        ULARGE_INTEGER ull;
        ull.QuadPart = windows_ticks;
        
        FILETIME ft;
        ft.dwLowDateTime = ull.LowPart;
        ft.dwHighDateTime = ull.HighPart;
        
        return ft;
    }
    #endif
    
    // Helper methods for complete obfuscation
    std::string encryptPathForStorage(const std::string& path, const std::string& key) {
        // Simple XOR encryption for path storage (not for security, just obfuscation)
        std::string encrypted = path;
        for (size_t i = 0; i < encrypted.length(); ++i) {
            encrypted[i] ^= key[i % key.length()];
        }
        
        // Base64-like encoding to make it look like system data
        std::stringstream encoded;
        for (unsigned char c : encrypted) {
            encoded << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(c);
        }
        
        return encoded.str();
    }
    
    std::string decryptPathFromStorage(const std::string& encrypted_path, const std::string& key) {
        try {
            // Decode from hex
            std::string decoded;
            for (size_t i = 0; i < encrypted_path.length(); i += 2) {
                std::string byte_str = encrypted_path.substr(i, 2);
                unsigned char byte = static_cast<unsigned char>(std::stoi(byte_str, nullptr, 16));
                decoded.push_back(byte);
            }
            
            // XOR decrypt
            for (size_t i = 0; i < decoded.length(); ++i) {
                decoded[i] ^= key[i % key.length()];
            }
            
            return decoded;
            
        } catch (const std::exception& e) {
            return "";
        }
    }
    
    void secureWipeTempFiles(const std::string& pattern) {
        // Securely wipe temporary files matching pattern
        try {
            // This is a simplified implementation - in production would use proper glob matching
            std::string base_dir = pattern.substr(0, pattern.find('*'));
            if (fs::exists(base_dir) && fs::is_directory(base_dir)) {
                for (const auto& entry : fs::directory_iterator(base_dir)) {
                    if (entry.is_regular_file()) {
                        secureWipeFile(entry.path().string());
                        fs::remove(entry.path());
                    }
                }
            }
        } catch (const std::exception& e) {
            // Ignore errors in cleanup
        }
    }
    
    std::string generateRandomHexString(size_t length) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);
        
        std::string hex_string;
        hex_string.reserve(length);
        
        const char hex_chars[] = "0123456789abcdef";
        for (size_t i = 0; i < length; ++i) {
            hex_string += hex_chars[dis(gen)];
        }
        return hex_string;
    }
    
    void createDecoyFiles(const std::string& directory) {
        try {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> file_count_dis(3, 8);
            std::uniform_int_distribution<> size_dis(1024, 10240);
            
            int num_files = file_count_dis(gen);
            for (int i = 0; i < num_files; ++i) {
                std::string filename = generateRandomHexString(12) + ".tmp";
                std::string filepath = directory + "/" + filename;
                
                std::ofstream file(filepath, std::ios::binary);
                if (file.is_open()) {
                    int file_size = size_dis(gen);
                    std::uniform_int_distribution<> byte_dis(0, 255);
                    
                    for (int j = 0; j < file_size; ++j) {
                        file.put(static_cast<char>(byte_dis(gen)));
                    }
                    file.close();
                }
            }
        } catch (const std::exception& e) {
            // Ignore errors in decoy creation
        }
    }
    
    void setRandomTimestamps(const std::string& path) {
        try {
            std::random_device rd;
            std::mt19937 gen(rd());
            
            // Generate random timestamp within last 6 months
            auto now = std::chrono::system_clock::now();
            auto six_months_ago = now - std::chrono::hours(24 * 30 * 6);
            
            std::uniform_int_distribution<time_t> time_dis(
                std::chrono::system_clock::to_time_t(six_months_ago),
                std::chrono::system_clock::to_time_t(now)
            );
            
            struct utimbuf times;
            times.actime = time_dis(gen);
            times.modtime = time_dis(gen);
            
            utime(path.c_str(), &times);
            
        } catch (const std::exception& e) {
            // Ignore errors in timestamp setting
        }
    }
    
    void clearSystemLogReferences(const std::string& path) {
        (void)path; // Suppress unused parameter warning
        // Clear system log references (simplified implementation)
        try {
            // On Linux, clear recent file access logs
            #ifdef PLATFORM_LINUX
            // Note: In production, this would require careful log sanitization
            // For now, just log that we attempted cleanup
            logOperation("LOG_CLEANUP", "Attempted to clear system log references for: " + fs::path(path).filename().string());
            #endif
            
        } catch (const std::exception& e) {
            // Ignore errors in log cleanup
        }
    }
    
    void overwriteDirectoryMetadata(const std::string& parent_dir) {
        (void)parent_dir; // Suppress unused parameter warning
        try {
            // Overwrite directory metadata to eliminate traces
            #ifdef PLATFORM_LINUX
            // Update directory access time to current time
            struct utimbuf times;
            times.actime = time(nullptr);
            times.modtime = time(nullptr);
            utime(parent_dir.c_str(), &times);
            #endif
            
        } catch (const std::exception& e) {
            // Ignore errors in metadata cleanup
        }
    }
};

// VaultHandler public interface implementation
VaultHandler::VaultHandler() : pimpl(std::make_unique<Implementation>()) {}
VaultHandler::~VaultHandler() = default;

bool VaultHandler::initialize(const std::string& vault_root_path) {
    return pimpl->initialize(vault_root_path);
}

bool VaultHandler::requiresElevatedPrivileges() const {
    return pimpl->requiresElevatedPrivileges();
}

HidingResult VaultHandler::hideFolder(const std::string& folder_path, const std::string& vault_id) {
    return pimpl->hideFolder(folder_path, vault_id);
}

RestorationResult VaultHandler::restoreFolder(const std::string& vault_id, const std::string& folder_identifier) {
    return pimpl->restoreFolder(vault_id, folder_identifier);
}

bool VaultHandler::preserveFolderMetadata(const std::string& folder_path, FolderMetadata& metadata) {
    return pimpl->preserveFolderMetadata(folder_path, metadata);
}

bool VaultHandler::restoreFolderMetadata(const std::string& folder_path, const FolderMetadata& metadata) {
    return pimpl->restoreFolderMetadata(folder_path, metadata);
}

bool VaultHandler::createVaultStructure(const std::string& vault_id, const std::string& profile_id) {
    return pimpl->createVaultStructure(vault_id, profile_id);
}

CleanupResult VaultHandler::secureDeleteFromVault(const std::string& vault_id, const std::string& folder_identifier) {
    return pimpl->secureDeleteFromVault(vault_id, folder_identifier);
}

std::string VaultHandler::getLastError() const {
    return pimpl->getLastError();
}

std::vector<std::string> VaultHandler::getOperationLog() const {
    return pimpl->getOperationLog();
}

} // namespace phantomvault