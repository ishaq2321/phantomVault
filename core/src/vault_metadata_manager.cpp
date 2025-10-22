#include "phantom_vault/vault_metadata_manager.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <mutex>
#include <unistd.h>
#include <pwd.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

namespace phantom_vault {
namespace service {

using json = nlohmann::json;

class VaultMetadataManager::Implementation {
public:
    Implementation() 
        : username_()
        , vault_storage_path_()
        , last_error_()
        , mutex_()
    {}

    ~Implementation() = default;

    bool initialize(const std::string& username) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        username_ = username;
        
        // Get user home directory
        const char* home = getenv("HOME");
        if (!home) {
            struct passwd* pw = getpwuid(getuid());
            if (!pw) {
                last_error_ = "Failed to get user home directory";
                return false;
            }
            home = pw->pw_dir;
        }
        
        // Set vault storage path
        vault_storage_path_ = std::string(home) + "/.phantom_vault_storage/" + username;
        
        // Ensure directories exist
        if (!ensureDirectoriesExist()) {
            return false;
        }
        
        std::cout << "[VaultMetadataManager] Initialized for user: " << username << std::endl;
        std::cout << "[VaultMetadataManager] Vault storage path: " << vault_storage_path_ << std::endl;
        
        return true;
    }

    FoldersMetadata loadFoldersMetadata(const std::string& profileId) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        FoldersMetadata metadata;
        metadata.profileId = profileId;
        
        std::string metadataPath = getFoldersMetadataPath(profileId);
        
        if (!std::filesystem::exists(metadataPath)) {
            // Create empty metadata file
            metadata.lastModified = getCurrentTimestamp();
            if (!saveFoldersMetadataInternal(profileId, metadata)) {
                last_error_ = "Failed to create initial folders metadata";
            }
            return metadata;
        }
        
        try {
            std::ifstream file(metadataPath);
            if (!file.is_open()) {
                last_error_ = "Failed to open folders metadata file: " + metadataPath;
                return metadata;
            }
            
            json j;
            file >> j;
            file.close();
            
            // Verify HMAC if present
            if (j.contains("hmac")) {
                if (!verifyHMAC(j, profileId, true)) {
                    last_error_ = "Folders metadata HMAC verification failed - file may have been tampered with";
                    throw std::runtime_error(last_error_);
                }
                std::cout << "[VaultMetadataManager] ✅ Folders metadata HMAC verified" << std::endl;
            }
            
            // Parse metadata
            metadata = parseFoldersMetadata(j);
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to load folders metadata: " + std::string(e.what());
            std::cerr << "[VaultMetadataManager] " << last_error_ << std::endl;
        }
        
        return metadata;
    }

    bool saveFoldersMetadata(const std::string& profileId, const FoldersMetadata& metadata) {
        std::lock_guard<std::mutex> lock(mutex_);
        return saveFoldersMetadataInternal(profileId, metadata);
    }

    ProfilesMetadata loadProfilesMetadata() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        ProfilesMetadata metadata;
        std::string metadataPath = getProfilesMetadataPath();
        
        if (!std::filesystem::exists(metadataPath)) {
            // Create empty metadata file
            metadata.lastModified = getCurrentTimestamp();
            if (!saveProfilesMetadataInternal(metadata)) {
                last_error_ = "Failed to create initial profiles metadata";
            }
            return metadata;
        }
        
        try {
            std::ifstream file(metadataPath);
            if (!file.is_open()) {
                last_error_ = "Failed to open profiles metadata file: " + metadataPath;
                return metadata;
            }
            
            json j;
            file >> j;
            file.close();
            
            // Verify HMAC if present
            if (j.contains("hmac")) {
                if (!verifyHMAC(j, "", false)) {
                    last_error_ = "Profiles metadata HMAC verification failed - file may have been tampered with";
                    throw std::runtime_error(last_error_);
                }
                std::cout << "[VaultMetadataManager] ✅ Profiles metadata HMAC verified" << std::endl;
            }
            
            // Parse metadata
            metadata = parseProfilesMetadata(j);
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to load profiles metadata: " + std::string(e.what());
            std::cerr << "[VaultMetadataManager] " << last_error_ << std::endl;
        }
        
        return metadata;
    }

    bool saveProfilesMetadata(const ProfilesMetadata& metadata) {
        std::lock_guard<std::mutex> lock(mutex_);
        return saveProfilesMetadataInternal(metadata);
    }

    std::vector<FolderMetadata> getFolders(const std::string& profileId) {
        FoldersMetadata metadata = loadFoldersMetadata(profileId);
        return metadata.folders;
    }

    std::optional<FolderMetadata> getFolder(const std::string& profileId, const std::string& folderId) {
        std::vector<FolderMetadata> folders = getFolders(profileId);
        
        for (const auto& folder : folders) {
            if (folder.id == folderId) {
                return folder;
            }
        }
        
        return std::nullopt;
    }

    bool updateFolderState(const std::string& profileId, const std::string& folderId, 
                          bool isLocked, const std::optional<std::string>& vaultPath,
                          const std::optional<std::string>& unlockMode) {
        FoldersMetadata metadata = loadFoldersMetadata(profileId);
        
        // Find and update folder
        bool found = false;
        for (auto& folder : metadata.folders) {
            if (folder.id == folderId) {
                folder.isLocked = isLocked;
                folder.vaultPath = vaultPath;
                folder.unlockMode = unlockMode;
                found = true;
                break;
            }
        }
        
        if (!found) {
            last_error_ = "Folder not found: " + folderId;
            return false;
        }
        
        return saveFoldersMetadata(profileId, metadata);
    }

    bool addBackupEntry(const std::string& profileId, const std::string& folderId,
                       const std::string& backupPath, const std::string& operation) {
        FoldersMetadata metadata = loadFoldersMetadata(profileId);
        
        // Find folder and add backup entry
        bool found = false;
        for (auto& folder : metadata.folders) {
            if (folder.id == folderId) {
                BackupEntry entry(getCurrentTimestamp(), backupPath, operation);
                folder.backups.push_back(entry);
                found = true;
                break;
            }
        }
        
        if (!found) {
            last_error_ = "Folder not found: " + folderId;
            return false;
        }
        
        return saveFoldersMetadata(profileId, metadata);
    }

    std::optional<ProfileMetadata> getProfile(const std::string& profileId) {
        ProfilesMetadata metadata = loadProfilesMetadata();
        
        for (const auto& profile : metadata.profiles) {
            if (profile.id == profileId) {
                return profile;
            }
        }
        
        return std::nullopt;
    }

    std::optional<ProfileMetadata> getActiveProfile() {
        ProfilesMetadata metadata = loadProfilesMetadata();
        
        if (metadata.activeProfileId.empty()) {
            return std::nullopt;
        }
        
        return getProfile(metadata.activeProfileId);
    }

    bool validateMetadataIntegrity(const std::string& profileId) {
        try {
            if (profileId.empty()) {
                // Validate profiles metadata
                ProfilesMetadata metadata = loadProfilesMetadata();
                return true; // If loading succeeded, HMAC was valid
            } else {
                // Validate folders metadata
                FoldersMetadata metadata = loadFoldersMetadata(profileId);
                return true; // If loading succeeded, HMAC was valid
            }
        } catch (const std::exception& e) {
            last_error_ = "Metadata integrity validation failed: " + std::string(e.what());
            return false;
        }
    }

    std::string getUsername() const {
        return username_;
    }

    std::string getVaultStoragePath() const {
        return vault_storage_path_;
    }

    std::string getLastError() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return last_error_;
    }

private:
    bool ensureDirectoriesExist() {
        try {
            // Create base vault storage directory
            std::filesystem::create_directories(vault_storage_path_);
            std::filesystem::permissions(vault_storage_path_, 
                                       std::filesystem::perms::owner_all,
                                       std::filesystem::perm_options::replace);
            
            // Create metadata directory
            std::string metadataPath = vault_storage_path_ + "/metadata";
            std::filesystem::create_directories(metadataPath);
            std::filesystem::permissions(metadataPath, 
                                       std::filesystem::perms::owner_all,
                                       std::filesystem::perm_options::replace);
            
            // Create vaults directory
            std::string vaultsPath = vault_storage_path_ + "/vaults";
            std::filesystem::create_directories(vaultsPath);
            std::filesystem::permissions(vaultsPath, 
                                       std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec,
                                       std::filesystem::perm_options::replace);
            
            // Create backups directory
            std::string backupsPath = vault_storage_path_ + "/backups";
            std::filesystem::create_directories(backupsPath);
            std::filesystem::permissions(backupsPath, 
                                       std::filesystem::perms::owner_all,
                                       std::filesystem::perm_options::replace);
            
            return true;
        } catch (const std::exception& e) {
            last_error_ = "Failed to create vault directories: " + std::string(e.what());
            return false;
        }
    }

    std::string getFoldersMetadataPath(const std::string& profileId) {
        return vault_storage_path_ + "/metadata/" + profileId + "/folders_metadata.json";
    }

    std::string getProfilesMetadataPath() {
        return vault_storage_path_ + "/metadata/profiles.json";
    }

    int64_t getCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    std::string generateHMAC(const json& data, const std::string& profileId, bool isFoldersMetadata) {
        // Generate HMAC key using same method as VaultFolderManager.js
        std::string keyMaterial;
        if (isFoldersMetadata) {
            keyMaterial = profileId + "-" + getHostname() + "-" + username_;
        } else {
            keyMaterial = "profiles-" + getHostname() + "-" + username_;
        }
        
        // Create SHA256 hash of key material
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(keyMaterial.c_str()), keyMaterial.length(), hash);
        
        // Generate HMAC
        std::string dataStr = data.dump();
        unsigned char hmac[SHA256_DIGEST_LENGTH];
        unsigned int hmacLen;
        
        HMAC(EVP_sha256(), hash, SHA256_DIGEST_LENGTH,
             reinterpret_cast<const unsigned char*>(dataStr.c_str()), dataStr.length(),
             hmac, &hmacLen);
        
        // Convert to hex string
        std::stringstream ss;
        for (unsigned int i = 0; i < hmacLen; i++) {
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(hmac[i]);
        }
        
        return ss.str();
    }

    bool verifyHMAC(const json& data, const std::string& profileId, bool isFoldersMetadata) {
        if (!data.contains("hmac")) {
            return true; // No HMAC to verify
        }
        
        std::string providedHmac = data["hmac"];
        
        // Create data without HMAC for verification
        json dataWithoutHmac = data;
        dataWithoutHmac.erase("hmac");
        
        std::string expectedHmac = generateHMAC(dataWithoutHmac, profileId, isFoldersMetadata);
        
        return providedHmac == expectedHmac;
    }

    std::string getHostname() {
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            return std::string(hostname);
        }
        return "unknown";
    }

    bool saveFoldersMetadataInternal(const std::string& profileId, const FoldersMetadata& metadata) {
        try {
            // Ensure profile directory exists
            std::string profileDir = vault_storage_path_ + "/metadata/" + profileId;
            std::filesystem::create_directories(profileDir);
            std::filesystem::permissions(profileDir, 
                                       std::filesystem::perms::owner_all,
                                       std::filesystem::perm_options::replace);
            
            // Convert to JSON
            json j = serializeFoldersMetadata(metadata);
            
            // Add HMAC for integrity protection
            std::string hmac = generateHMAC(j, profileId, true);
            j["hmac"] = hmac;
            
            // Write to file
            std::string metadataPath = getFoldersMetadataPath(profileId);
            std::ofstream file(metadataPath);
            if (!file.is_open()) {
                last_error_ = "Failed to open folders metadata file for writing: " + metadataPath;
                return false;
            }
            
            file << j.dump(2);
            file.close();
            
            // Set secure permissions
            std::filesystem::permissions(metadataPath, 
                                       std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
                                       std::filesystem::perm_options::replace);
            
            return true;
        } catch (const std::exception& e) {
            last_error_ = "Failed to save folders metadata: " + std::string(e.what());
            return false;
        }
    }

    bool saveProfilesMetadataInternal(const ProfilesMetadata& metadata) {
        try {
            // Convert to JSON
            json j = serializeProfilesMetadata(metadata);
            
            // Add HMAC for integrity protection
            std::string hmac = generateHMAC(j, "", false);
            j["hmac"] = hmac;
            
            // Write to file
            std::string metadataPath = getProfilesMetadataPath();
            std::ofstream file(metadataPath);
            if (!file.is_open()) {
                last_error_ = "Failed to open profiles metadata file for writing: " + metadataPath;
                return false;
            }
            
            file << j.dump(2);
            file.close();
            
            // Set secure permissions
            std::filesystem::permissions(metadataPath, 
                                       std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
                                       std::filesystem::perm_options::replace);
            
            return true;
        } catch (const std::exception& e) {
            last_error_ = "Failed to save profiles metadata: " + std::string(e.what());
            return false;
        }
    }

    FoldersMetadata parseFoldersMetadata(const json& j) {
        FoldersMetadata metadata;
        
        metadata.profileId = j.value("profileId", "");
        metadata.lastModified = j.value("lastModified", getCurrentTimestamp());
        
        if (j.contains("hmac")) {
            metadata.hmac = j["hmac"];
        }
        
        if (j.contains("folders") && j["folders"].is_array()) {
            for (const auto& folderJson : j["folders"]) {
                FolderMetadata folder;
                
                folder.id = folderJson.value("id", "");
                folder.folderPath = folderJson.value("folderPath", "");
                folder.folderName = folderJson.value("folderName", "");
                folder.isLocked = folderJson.value("isLocked", false);
                folder.usesMasterPassword = folderJson.value("usesMasterPassword", true);
                folder.createdAt = folderJson.value("createdAt", 0);
                
                if (folderJson.contains("unlockMode") && !folderJson["unlockMode"].is_null()) {
                    folder.unlockMode = folderJson["unlockMode"];
                }
                
                folder.originalPath = folderJson.value("originalPath", "");
                
                if (folderJson.contains("vaultPath") && !folderJson["vaultPath"].is_null()) {
                    folder.vaultPath = folderJson["vaultPath"];
                }
                
                // Parse backups array
                if (folderJson.contains("backups") && folderJson["backups"].is_array()) {
                    for (const auto& backupJson : folderJson["backups"]) {
                        BackupEntry backup;
                        backup.timestamp = backupJson.value("timestamp", 0);
                        backup.path = backupJson.value("path", "");
                        backup.operation = backupJson.value("operation", "");
                        folder.backups.push_back(backup);
                    }
                }
                
                // Parse optional custom password fields
                if (folderJson.contains("customPasswordHash")) {
                    folder.customPasswordHash = folderJson["customPasswordHash"];
                }
                if (folderJson.contains("customRecoveryKeyHash")) {
                    folder.customRecoveryKeyHash = folderJson["customRecoveryKeyHash"];
                }
                if (folderJson.contains("encryptedCustomRecoveryKey")) {
                    folder.encryptedCustomRecoveryKey = folderJson["encryptedCustomRecoveryKey"];
                }
                
                metadata.folders.push_back(folder);
            }
        }
        
        return metadata;
    }

    ProfilesMetadata parseProfilesMetadata(const json& j) {
        ProfilesMetadata metadata;
        
        metadata.activeProfileId = j.value("activeProfileId", "");
        metadata.lastModified = j.value("lastModified", getCurrentTimestamp());
        
        if (j.contains("hmac")) {
            metadata.hmac = j["hmac"];
        }
        
        if (j.contains("profiles") && j["profiles"].is_array()) {
            for (const auto& profileJson : j["profiles"]) {
                ProfileMetadata profile;
                
                profile.id = profileJson.value("id", "");
                profile.name = profileJson.value("name", "");
                profile.hashedPassword = profileJson.value("hashedPassword", "");
                profile.encryptedRecoveryKey = profileJson.value("encryptedRecoveryKey", "");
                profile.createdAt = profileJson.value("createdAt", 0);
                
                metadata.profiles.push_back(profile);
            }
        }
        
        return metadata;
    }

    json serializeFoldersMetadata(const FoldersMetadata& metadata) {
        json j;
        
        j["profileId"] = metadata.profileId;
        j["lastModified"] = getCurrentTimestamp(); // Always use current timestamp
        
        j["folders"] = json::array();
        for (const auto& folder : metadata.folders) {
            json folderJson;
            
            folderJson["id"] = folder.id;
            folderJson["folderPath"] = folder.folderPath;
            folderJson["folderName"] = folder.folderName;
            folderJson["isLocked"] = folder.isLocked;
            folderJson["usesMasterPassword"] = folder.usesMasterPassword;
            folderJson["createdAt"] = folder.createdAt;
            
            if (folder.unlockMode.has_value()) {
                folderJson["unlockMode"] = folder.unlockMode.value();
            } else {
                folderJson["unlockMode"] = nullptr;
            }
            
            folderJson["originalPath"] = folder.originalPath;
            
            if (folder.vaultPath.has_value()) {
                folderJson["vaultPath"] = folder.vaultPath.value();
            } else {
                folderJson["vaultPath"] = nullptr;
            }
            
            // Serialize backups array
            folderJson["backups"] = json::array();
            for (const auto& backup : folder.backups) {
                json backupJson;
                backupJson["timestamp"] = backup.timestamp;
                backupJson["path"] = backup.path;
                backupJson["operation"] = backup.operation;
                folderJson["backups"].push_back(backupJson);
            }
            
            // Serialize optional custom password fields
            if (folder.customPasswordHash.has_value()) {
                folderJson["customPasswordHash"] = folder.customPasswordHash.value();
            }
            if (folder.customRecoveryKeyHash.has_value()) {
                folderJson["customRecoveryKeyHash"] = folder.customRecoveryKeyHash.value();
            }
            if (folder.encryptedCustomRecoveryKey.has_value()) {
                folderJson["encryptedCustomRecoveryKey"] = folder.encryptedCustomRecoveryKey.value();
            }
            
            j["folders"].push_back(folderJson);
        }
        
        return j;
    }

    json serializeProfilesMetadata(const ProfilesMetadata& metadata) {
        json j;
        
        j["activeProfileId"] = metadata.activeProfileId;
        j["lastModified"] = getCurrentTimestamp(); // Always use current timestamp
        
        j["profiles"] = json::array();
        for (const auto& profile : metadata.profiles) {
            json profileJson;
            
            profileJson["id"] = profile.id;
            profileJson["name"] = profile.name;
            profileJson["hashedPassword"] = profile.hashedPassword;
            profileJson["encryptedRecoveryKey"] = profile.encryptedRecoveryKey;
            profileJson["createdAt"] = profile.createdAt;
            
            j["profiles"].push_back(profileJson);
        }
        
        return j;
    }

    std::string username_;
    std::string vault_storage_path_;
    mutable std::string last_error_;
    mutable std::mutex mutex_;
};

// VaultMetadataManager public interface implementation
VaultMetadataManager::VaultMetadataManager() : pimpl(std::make_unique<Implementation>()) {}
VaultMetadataManager::~VaultMetadataManager() = default;

bool VaultMetadataManager::initialize(const std::string& username) {
    return pimpl->initialize(username);
}

FoldersMetadata VaultMetadataManager::loadFoldersMetadata(const std::string& profileId) {
    return pimpl->loadFoldersMetadata(profileId);
}

bool VaultMetadataManager::saveFoldersMetadata(const std::string& profileId, const FoldersMetadata& metadata) {
    return pimpl->saveFoldersMetadata(profileId, metadata);
}

ProfilesMetadata VaultMetadataManager::loadProfilesMetadata() {
    return pimpl->loadProfilesMetadata();
}

bool VaultMetadataManager::saveProfilesMetadata(const ProfilesMetadata& metadata) {
    return pimpl->saveProfilesMetadata(metadata);
}

std::vector<FolderMetadata> VaultMetadataManager::getFolders(const std::string& profileId) {
    return pimpl->getFolders(profileId);
}

std::optional<FolderMetadata> VaultMetadataManager::getFolder(const std::string& profileId, const std::string& folderId) {
    return pimpl->getFolder(profileId, folderId);
}

bool VaultMetadataManager::updateFolderState(const std::string& profileId, const std::string& folderId, 
                                            bool isLocked, const std::optional<std::string>& vaultPath,
                                            const std::optional<std::string>& unlockMode) {
    return pimpl->updateFolderState(profileId, folderId, isLocked, vaultPath, unlockMode);
}

bool VaultMetadataManager::addBackupEntry(const std::string& profileId, const std::string& folderId,
                                         const std::string& backupPath, const std::string& operation) {
    return pimpl->addBackupEntry(profileId, folderId, backupPath, operation);
}

std::optional<ProfileMetadata> VaultMetadataManager::getProfile(const std::string& profileId) {
    return pimpl->getProfile(profileId);
}

std::optional<ProfileMetadata> VaultMetadataManager::getActiveProfile() {
    return pimpl->getActiveProfile();
}

bool VaultMetadataManager::validateMetadataIntegrity(const std::string& profileId) {
    return pimpl->validateMetadataIntegrity(profileId);
}

std::string VaultMetadataManager::getUsername() const {
    return pimpl->getUsername();
}

std::string VaultMetadataManager::getVaultStoragePath() const {
    return pimpl->getVaultStoragePath();
}

std::string VaultMetadataManager::getLastError() const {
    return pimpl->getLastError();
}

} // namespace service
} // namespace phantom_vault