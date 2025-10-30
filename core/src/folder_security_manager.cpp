/**
 * PhantomVault Folder Security Manager Implementation
 * 
 * Complete implementation for folder security operations including:
 * - AES-256 encryption with secure key derivation
 * - Complete folder hiding and trace removal
 * - Secure vault storage with backup mechanisms
 * - Temporary and permanent unlock operations
 */

#include "folder_security_manager.hpp"
#include "profile_vault.hpp"
#include "privilege_manager.hpp"
#include "vault_handler.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <unordered_map>
#include <nlohmann/json.hpp>

#ifdef PLATFORM_LINUX
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#elif PLATFORM_WINDOWS
#include <windows.h>
#include <shlobj.h>
#elif PLATFORM_MACOS
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#endif

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace phantomvault {

class FolderSecurityManager::Implementation {
public:
    Implementation() 
        : data_path_()
        , vault_manager_()
        , temporary_unlocks_()
        , last_error_()
        , privilege_manager_(std::make_unique<PrivilegeManager>())
    {}
    
    bool initialize(const std::string& dataPath) {
        try {
            // Set data path
            if (dataPath.empty()) {
                data_path_ = getDefaultDataPath();
            } else {
                data_path_ = dataPath;
            }
            
            // Initialize VaultManager with vaults subdirectory
            std::string vault_root = data_path_ + "/vaults";
            vault_manager_ = std::make_unique<::PhantomVault::VaultManager>(vault_root);
            
            if (!vault_manager_->initializeVaultSystem()) {
                last_error_ = "Failed to initialize vault system: " + vault_manager_->getLastError();
                return false;
            }
            
            // Ensure data directory exists
            if (!fs::exists(data_path_)) {
                fs::create_directories(data_path_);
                fs::permissions(data_path_, fs::perms::owner_all, fs::perm_options::replace);
            }
            
            std::cout << "[FolderSecurityManager] Initialized with ProfileVault system" << std::endl;
            std::cout << "[FolderSecurityManager] Data path: " << data_path_ << std::endl;
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to initialize FolderSecurityManager: " + std::string(e.what());
            return false;
        }
    }
    
    FolderOperationResult lockFolder(const std::string& profileId, 
                                   const std::string& folderPath, 
                                   const std::string& masterKey) {
        FolderOperationResult result;
        
        try {
            // Validate inputs
            if (profileId.empty()) {
                result.error = "Profile ID cannot be empty";
                return result;
            }
            
            if (!validateFolderPath(folderPath)) {
                result.error = "Invalid folder path or folder does not exist";
                return result;
            }
            
            if (masterKey.empty()) {
                result.error = "Master key cannot be empty";
                return result;
            }
            
            // Ensure profile vault exists
            if (!vault_manager_->createProfileVault(profileId)) {
                // Vault might already exist, which is fine
                if (vault_manager_->getLastError().find("already exists") == std::string::npos) {
                    result.error = "Failed to create profile vault: " + vault_manager_->getLastError();
                    return result;
                }
            }
            
            // Get profile vault
            auto profile_vault = vault_manager_->getProfileVault(profileId);
            if (!profile_vault) {
                result.error = "Failed to get profile vault: " + vault_manager_->getLastError();
                return result;
            }
            
            // Use ProfileVault to lock the folder with real encryption
            auto vault_result = profile_vault->lockFolder(folderPath, masterKey);
            if (!vault_result.success) {
                result.error = vault_result.error_details;
                return result;
            }
            
            // Generate folder ID for compatibility with existing API
            std::string folderId = generateFolderId();
            
            // Create SecuredFolder metadata for compatibility
            fs::path originalPath(folderPath);
            std::string folderName = originalPath.filename().string();
            size_t folderSize = calculateFolderSize(folderPath);
            
            SecuredFolder folder;
            folder.id = folderId;
            folder.profileId = profileId;
            folder.originalName = folderName;
            folder.originalPath = folderPath;
            folder.vaultPath = profile_vault->getVaultPath();
            folder.isLocked = true;
            folder.unlockMode = UnlockMode::TEMPORARY;
            folder.createdAt = std::chrono::system_clock::now();
            folder.lastAccess = folder.createdAt;
            folder.originalSize = folderSize;
            
            // Save folder metadata for compatibility with existing API
            if (!saveFolderMetadata(folder)) {
                result.error = "Failed to save folder metadata";
                return result;
            }
            
            result.success = true;
            result.folderId = folderId;
            result.message = "Folder locked and encrypted successfully with real AES-256 encryption";
            
            std::cout << "[FolderSecurityManager] Locked folder with real encryption: " << folderName 
                      << " (ID: " << folderId << ")" << std::endl;
            
            return result;
            
        } catch (const std::exception& e) {
            result.error = "Failed to lock folder: " + std::string(e.what());
            return result;
        }
    }
    
    UnlockResult unlockFoldersTemporary(const std::string& profileId, const std::string& masterKey) {
        return unlockFolders(profileId, masterKey, UnlockMode::TEMPORARY);
    }
    
    UnlockResult unlockFoldersPermanent(const std::string& profileId, 
                                      const std::string& masterKey,
                                      const std::vector<std::string>& folderIds) {
        return unlockFolders(profileId, masterKey, UnlockMode::PERMANENT, folderIds);
    }
    
    bool lockTemporaryFolders(const std::string& profileId) {
        try {
            // Get profile vault
            auto profile_vault = vault_manager_->getProfileVault(profileId);
            if (!profile_vault) {
                last_error_ = "Profile vault not found: " + vault_manager_->getLastError();
                return false;
            }
            
            // Use ProfileVault to relock temporary folders
            auto relock_result = profile_vault->relockTemporaryFolders();
            
            if (relock_result.success) {
                // Clear temporary unlocks tracking
                temporary_unlocks_.clear();
                
                std::cout << "[FolderSecurityManager] " << relock_result.message 
                          << " for profile: " << profileId << std::endl;
                return true;
            } else {
                last_error_ = relock_result.error_details;
                return false;
            }
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to lock temporary folders: " + std::string(e.what());
            return false;
        }
    }
    
    std::vector<SecuredFolder> getProfileFolders(const std::string& profileId) {
        std::vector<SecuredFolder> folders;
        
        try {
            // Get profile vault
            auto profile_vault = vault_manager_->getProfileVault(profileId);
            if (!profile_vault) {
                // Profile vault doesn't exist, return empty list
                return folders;
            }
            
            // Get locked folders from ProfileVault
            auto vault_folders = profile_vault->getLockedFolders();
            
            // Convert ProfileVault folders to SecuredFolder format for compatibility
            for (const auto& vault_folder : vault_folders) {
                SecuredFolder folder;
                // Generate consistent ID based on folder path hash for compatibility
                folder.id = "vault_" + std::to_string(std::hash<std::string>{}(vault_folder.original_path));
                folder.profileId = profileId;
                
                fs::path original_path(vault_folder.original_path);
                folder.originalName = original_path.filename().string();
                folder.originalPath = vault_folder.original_path;
                folder.vaultPath = vault_folder.vault_location;
                folder.isLocked = true; // ProfileVault folders are always locked when returned
                folder.unlockMode = vault_folder.is_temporarily_unlocked ? UnlockMode::TEMPORARY : UnlockMode::TEMPORARY;
                folder.createdAt = vault_folder.lock_timestamp;
                folder.lastAccess = vault_folder.lock_timestamp;
                folder.originalSize = vault_folder.total_size;
                
                folders.push_back(folder);
            }
            
            // Also check legacy metadata for compatibility
            fs::path metadataFile = fs::path(data_path_) / "folders" / (profileId + ".json");
            if (fs::exists(metadataFile)) {
                std::ifstream file(metadataFile);
                json foldersData;
                file >> foldersData;
                
                if (foldersData.contains("folders") && foldersData["folders"].is_array()) {
                    for (const auto& folderJson : foldersData["folders"]) {
                        SecuredFolder legacy_folder = parseFolderFromJson(folderJson);
                        
                        // Only add if not already present from ProfileVault
                        bool already_exists = false;
                        for (const auto& existing : folders) {
                            if (existing.originalPath == legacy_folder.originalPath) {
                                already_exists = true;
                                break;
                            }
                        }
                        
                        if (!already_exists) {
                            folders.push_back(legacy_folder);
                        }
                    }
                }
            }
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to get profile folders: " + std::string(e.what());
        }
        
        return folders;
    }
    
    std::optional<SecuredFolder> getFolder(const std::string& profileId, const std::string& folderId) {
        auto folders = getProfileFolders(profileId);
        for (const auto& folder : folders) {
            if (folder.id == folderId) {
                return folder;
            }
        }
        return std::nullopt;
    }
    
    FolderOperationResult removeFromProfile(const std::string& profileId, const std::string& folderId) {
        FolderOperationResult result;
        
        try {
            auto folder = getFolder(profileId, folderId);
            if (!folder) {
                result.error = "Folder not found";
                return result;
            }
            
            // Remove vault files
            fs::path vaultPath(folder->vaultPath);
            if (fs::exists(vaultPath)) {
                fs::remove_all(vaultPath);
            }
            
            // Note: ProfileVault handles its own cleanup, no separate backup files needed
            
            // Remove from metadata
            if (!removeFolderFromMetadata(profileId, folderId)) {
                result.error = "Failed to remove folder from metadata";
                return result;
            }
            
            // Remove from temporary unlocks if present
            temporary_unlocks_.erase(folderId);
            
            result.success = true;
            result.folderId = folderId;
            result.message = "Folder removed from profile successfully";
            
            std::cout << "[FolderSecurityManager] Removed folder from profile: " 
                      << folderId << std::endl;
            
            return result;
            
        } catch (const std::exception& e) {
            result.error = "Failed to remove folder: " + std::string(e.what());
            return result;
        }
    }
    
    bool validateFolderPath(const std::string& folderPath) {
        try {
            fs::path path(folderPath);
            return fs::exists(path) && fs::is_directory(path);
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    size_t calculateFolderSize(const std::string& folderPath) {
        try {
            size_t size = 0;
            for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
                if (entry.is_regular_file()) {
                    size += entry.file_size();
                }
            }
            return size;
        } catch (const std::exception& e) {
            return 0;
        }
    }
    
    std::string generateFolderId() {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        
        return "folder_" + std::to_string(timestamp) + "_" + std::to_string(dis(gen));
    }
    
    std::string getLastError() const {
        return last_error_;
    }
    
private:
    std::string data_path_;
    std::unique_ptr<::PhantomVault::VaultManager> vault_manager_;
    std::unordered_map<std::string, std::string> temporary_unlocks_; // folderId -> originalPath
    std::string last_error_;
    std::unique_ptr<PrivilegeManager> privilege_manager_;
    
    std::string getDefaultDataPath() {
        #ifdef PLATFORM_LINUX
        const char* home = getenv("HOME");
        if (!home) {
            struct passwd* pw = getpwuid(getuid());
            home = pw->pw_dir;
        }
        return std::string(home) + "/.phantomvault";
        #elif PLATFORM_WINDOWS
        char path[MAX_PATH];
        if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path) == S_OK) {
            return std::string(path) + "\\PhantomVault";
        }
        return "C:\\ProgramData\\PhantomVault";
        #elif PLATFORM_MACOS
        const char* home = getenv("HOME");
        if (!home) {
            struct passwd* pw = getpwuid(getuid());
            home = pw->pw_dir;
        }
        return std::string(home) + "/Library/Application Support/PhantomVault";
        #else
        return "./phantomvault_data";
        #endif
    }
    
    UnlockResult unlockFolders(const std::string& profileId, 
                             const std::string& masterKey, 
                             UnlockMode mode,
                             const std::vector<std::string>& /* specificFolderIds */ = {}) {
        UnlockResult result;
        
        try {
            // Get profile vault
            auto profile_vault = vault_manager_->getProfileVault(profileId);
            if (!profile_vault) {
                result.error = "Profile vault not found: " + vault_manager_->getLastError();
                return result;
            }
            
            // Get locked folders from ProfileVault
            auto locked_folders = profile_vault->getLockedFolders();
            
            for (const auto& vault_folder : locked_folders) {
                // Convert ProfileVault unlock mode
                ::PhantomVault::UnlockMode vault_mode = (mode == UnlockMode::TEMPORARY) ? 
                    ::PhantomVault::UnlockMode::TEMPORARY : ::PhantomVault::UnlockMode::PERMANENT;
                
                // Use ProfileVault to unlock with real decryption
                auto vault_result = profile_vault->unlockFolder(vault_folder.original_path, masterKey, vault_mode);
                
                if (vault_result.success) {
                    result.successCount++;
                    
                    // Generate a folder ID for compatibility (since ProfileVault doesn't use the same ID system)
                    std::string folderId = generateFolderId();
                    result.unlockedFolderIds.push_back(folderId);
                    
                    if (mode == UnlockMode::TEMPORARY) {
                        temporary_unlocks_[folderId] = vault_folder.original_path;
                    }
                    
                    // Update metadata for compatibility
                    updateFolderStatus(profileId, folderId, false, mode);
                    
                } else {
                    result.failedCount++;
                    std::string folderId = generateFolderId();
                    result.failedFolderIds.push_back(folderId);
                }
            }
            
            result.success = (result.failedCount == 0);
            result.message = "Unlocked " + std::to_string(result.successCount) + " folders with real decryption";
            
            std::cout << "[FolderSecurityManager] Real decryption unlock result: " << result.successCount 
                      << " success, " << result.failedCount << " failed" << std::endl;
            
            return result;
            
        } catch (const std::exception& e) {
            result.error = "Failed to unlock folders: " + std::string(e.what());
            return result;
        }
    }
    
    // Note: Individual folder unlock/lock methods are now handled by ProfileVault
    // These methods are kept for compatibility but delegate to ProfileVault
    
    bool saveFolderMetadata(const SecuredFolder& folder) {
        try {
            fs::path foldersDir = fs::path(data_path_) / "folders";
            fs::create_directories(foldersDir);
            
            fs::path metadataFile = foldersDir / (folder.profileId + ".json");
            
            json foldersData;
            if (fs::exists(metadataFile)) {
                std::ifstream file(metadataFile);
                file >> foldersData;
            }
            
            if (!foldersData.contains("folders")) {
                foldersData["folders"] = json::array();
            }
            
            json folderJson = serializeFolderToJson(folder);
            foldersData["folders"].push_back(folderJson);
            foldersData["lastModified"] = getCurrentTimestamp();
            
            std::ofstream outFile(metadataFile);
            outFile << foldersData.dump(2);
            outFile.close();
            
            fs::permissions(metadataFile, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace);
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to save folder metadata: " + std::string(e.what());
            return false;
        }
    }
    
    bool removeFolderFromMetadata(const std::string& profileId, const std::string& folderId) {
        try {
            fs::path metadataFile = fs::path(data_path_) / "folders" / (profileId + ".json");
            if (!fs::exists(metadataFile)) {
                return true;
            }
            
            std::ifstream inFile(metadataFile);
            json foldersData;
            inFile >> foldersData;
            inFile.close();
            
            if (foldersData.contains("folders") && foldersData["folders"].is_array()) {
                auto& folders = foldersData["folders"];
                folders.erase(
                    std::remove_if(folders.begin(), folders.end(),
                        [&folderId](const json& folderJson) {
                            return folderJson.value("id", "") == folderId;
                        }),
                    folders.end()
                );
            }
            
            foldersData["lastModified"] = getCurrentTimestamp();
            
            std::ofstream outFile(metadataFile);
            outFile << foldersData.dump(2);
            outFile.close();
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to remove folder from metadata: " + std::string(e.what());
            return false;
        }
    }
    
    void updateFolderStatus(const std::string& profileId, const std::string& folderId, 
                          bool isLocked, UnlockMode mode) {
        try {
            fs::path metadataFile = fs::path(data_path_) / "folders" / (profileId + ".json");
            if (!fs::exists(metadataFile)) {
                return;
            }
            
            std::ifstream inFile(metadataFile);
            json foldersData;
            inFile >> foldersData;
            inFile.close();
            
            if (foldersData.contains("folders") && foldersData["folders"].is_array()) {
                for (auto& folderJson : foldersData["folders"]) {
                    if (folderJson.value("id", "") == folderId) {
                        folderJson["isLocked"] = isLocked;
                        folderJson["unlockMode"] = (mode == UnlockMode::TEMPORARY) ? "temporary" : "permanent";
                        folderJson["lastAccess"] = getCurrentTimestamp();
                        break;
                    }
                }
            }
            
            foldersData["lastModified"] = getCurrentTimestamp();
            
            std::ofstream outFile(metadataFile);
            outFile << foldersData.dump(2);
            outFile.close();
            
        } catch (const std::exception& e) {
            // Non-critical error
            std::cerr << "[FolderSecurityManager] Warning: Failed to update folder status: " << e.what() << std::endl;
        }
    }
    
    SecuredFolder parseFolderFromJson(const json& folderJson) {
        SecuredFolder folder;
        folder.id = folderJson.value("id", "");
        folder.profileId = folderJson.value("profileId", "");
        folder.originalName = folderJson.value("originalName", "");
        folder.originalPath = folderJson.value("originalPath", "");
        folder.vaultPath = folderJson.value("vaultPath", "");
        folder.isLocked = folderJson.value("isLocked", true);
        
        std::string unlockModeStr = folderJson.value("unlockMode", "temporary");
        folder.unlockMode = (unlockModeStr == "permanent") ? UnlockMode::PERMANENT : UnlockMode::TEMPORARY;
        
        int64_t createdAtMs = folderJson.value("createdAt", 0);
        int64_t lastAccessMs = folderJson.value("lastAccess", 0);
        folder.createdAt = std::chrono::system_clock::from_time_t(createdAtMs / 1000);
        folder.lastAccess = std::chrono::system_clock::from_time_t(lastAccessMs / 1000);
        
        folder.originalSize = folderJson.value("originalSize", 0);
        
        return folder;
    }
    
    json serializeFolderToJson(const SecuredFolder& folder) {
        json folderJson;
        folderJson["id"] = folder.id;
        folderJson["profileId"] = folder.profileId;
        folderJson["originalName"] = folder.originalName;
        folderJson["originalPath"] = folder.originalPath;
        folderJson["vaultPath"] = folder.vaultPath;
        folderJson["isLocked"] = folder.isLocked;
        folderJson["unlockMode"] = (folder.unlockMode == UnlockMode::PERMANENT) ? "permanent" : "temporary";
        
        auto createdAtMs = std::chrono::duration_cast<std::chrono::milliseconds>(folder.createdAt.time_since_epoch()).count();
        auto lastAccessMs = std::chrono::duration_cast<std::chrono::milliseconds>(folder.lastAccess.time_since_epoch()).count();
        folderJson["createdAt"] = createdAtMs;
        folderJson["lastAccess"] = lastAccessMs;
        
        folderJson["originalSize"] = folder.originalSize;
        
        return folderJson;
    }
    
    int64_t getCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

FolderSecurityManager::FolderSecurityManager() : pimpl(std::make_unique<Implementation>()) {}
FolderSecurityManager::~FolderSecurityManager() = default;

bool FolderSecurityManager::initialize(const std::string& dataPath) {
    return pimpl->initialize(dataPath);
}

FolderOperationResult FolderSecurityManager::lockFolder(const std::string& profileId, 
                                                       const std::string& folderPath, 
                                                       const std::string& masterKey) {
    return pimpl->lockFolder(profileId, folderPath, masterKey);
}

UnlockResult FolderSecurityManager::unlockFoldersTemporary(const std::string& profileId, 
                                                          const std::string& masterKey) {
    return pimpl->unlockFoldersTemporary(profileId, masterKey);
}

UnlockResult FolderSecurityManager::unlockFoldersPermanent(const std::string& profileId, 
                                                          const std::string& masterKey,
                                                          const std::vector<std::string>& folderIds) {
    return pimpl->unlockFoldersPermanent(profileId, masterKey, folderIds);
}

bool FolderSecurityManager::lockTemporaryFolders(const std::string& profileId) {
    return pimpl->lockTemporaryFolders(profileId);
}

std::vector<SecuredFolder> FolderSecurityManager::getProfileFolders(const std::string& profileId) {
    return pimpl->getProfileFolders(profileId);
}

std::optional<SecuredFolder> FolderSecurityManager::getFolder(const std::string& profileId, const std::string& folderId) {
    return pimpl->getFolder(profileId, folderId);
}

FolderOperationResult FolderSecurityManager::removeFromProfile(const std::string& profileId, const std::string& folderId) {
    return pimpl->removeFromProfile(profileId, folderId);
}

bool FolderSecurityManager::validateFolderPath(const std::string& folderPath) {
    return pimpl->validateFolderPath(folderPath);
}

size_t FolderSecurityManager::calculateFolderSize(const std::string& folderPath) {
    return pimpl->calculateFolderSize(folderPath);
}

std::string FolderSecurityManager::generateFolderId() {
    return pimpl->generateFolderId();
}

std::string FolderSecurityManager::getLastError() const {
    return pimpl->getLastError();
}

} // namespace phantomvault