#include "phantom_vault/service_vault_manager.hpp"
#include "phantom_vault/vault_metadata_manager.hpp"
#include "phantom_vault/vault_encryption_manager.hpp"
#include "phantom_vault/vault_storage_manager.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <regex>
#include <random>
#include <iomanip>
#include <sstream>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include <mutex>
#include <thread>

namespace phantom_vault {
namespace service {

namespace fs = std::filesystem;

class ServiceVaultManager::Implementation {
public:
    Implementation()
        : metadata_manager_()
        , encryption_manager_()
        , storage_manager_()
        , username_()
        , temporary_unlocks_()
        , temporary_unlocks_mutex_()
        , last_error_()
    {}

    ~Implementation() = default;

    bool initialize() {
        // Get current username
        username_ = getCurrentUsername();
        
        // Initialize metadata manager
        metadata_manager_ = std::make_unique<VaultMetadataManager>();
        if (!metadata_manager_->initialize(username_)) {
            last_error_ = "Failed to initialize metadata manager: " + metadata_manager_->getLastError();
            return false;
        }

        // Initialize encryption manager
        encryption_manager_ = std::make_unique<VaultEncryptionManager>();
        if (!encryption_manager_->initialize()) {
            last_error_ = "Failed to initialize encryption manager: " + encryption_manager_->getLastError();
            return false;
        }

        // Initialize storage manager
        storage_manager_ = std::make_unique<VaultStorageManager>();
        if (!storage_manager_->initialize(username_)) {
            last_error_ = "Failed to initialize storage manager: " + storage_manager_->getLastError();
            return false;
        }

        std::cout << "[ServiceVaultManager] Initialized successfully for user: " << username_ << std::endl;
        std::cout << "  Vault storage path: " << storage_manager_->getUserVaultPath() << std::endl;
        
        return true;
    }

    std::shared_ptr<VaultProfile> getActiveProfile() {
        auto profiles_metadata = metadata_manager_->loadProfilesMetadata();
        if (profiles_metadata.profiles.empty()) {
            return nullptr;
        }

        // Get active profile or first profile
        std::string active_id = profiles_metadata.activeProfileId;
        if (active_id.empty() && !profiles_metadata.profiles.empty()) {
            active_id = profiles_metadata.profiles[0].id;
        }

        auto profile_opt = metadata_manager_->getProfile(active_id);
        if (!profile_opt) {
            return nullptr;
        }

        // Convert to VaultProfile
        auto vault_profile = std::make_shared<VaultProfile>();
        vault_profile->id = profile_opt->id;
        vault_profile->name = profile_opt->name;
        vault_profile->os_user = username_;
        vault_profile->created_at = std::chrono::system_clock::from_time_t(profile_opt->createdAt / 1000);
        vault_profile->last_access = vault_profile->created_at;
        vault_profile->master_password_hash = profile_opt->hashedPassword;
        vault_profile->encrypted_recovery_key = profile_opt->encryptedRecoveryKey;

        return vault_profile;
    }

    std::shared_ptr<VaultProfile> createProfile(
        const std::string& name,
        const std::string& master_password,
        const std::string& recovery_key
    ) {
        // Create profile metadata
        ProfileMetadata profile;
        profile.id = generateProfileId();
        profile.name = name;
        profile.hashedPassword = encryption_manager_->hashPassword(master_password);
        profile.encryptedRecoveryKey = encryptRecoveryKey(recovery_key, master_password);
        profile.createdAt = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        // Load existing profiles
        auto profiles_metadata = metadata_manager_->loadProfilesMetadata();
        profiles_metadata.profiles.push_back(profile);
        profiles_metadata.activeProfileId = profile.id;
        profiles_metadata.lastModified = profile.createdAt;

        // Save profiles
        if (!metadata_manager_->saveProfilesMetadata(profiles_metadata)) {
            last_error_ = "Failed to save profile: " + metadata_manager_->getLastError();
            return nullptr;
        }

        // Convert to VaultProfile
        auto vault_profile = std::make_shared<VaultProfile>();
        vault_profile->id = profile.id;
        vault_profile->name = profile.name;
        vault_profile->os_user = username_;
        vault_profile->created_at = std::chrono::system_clock::from_time_t(profile.createdAt / 1000);
        vault_profile->last_access = vault_profile->created_at;
        vault_profile->master_password_hash = profile.hashedPassword;
        vault_profile->encrypted_recovery_key = profile.encryptedRecoveryKey;

        std::cout << "[ServiceVaultManager] Created profile: " << profile.name << " (ID: " << profile.id << ")" << std::endl;
        return vault_profile;
    }

    bool verifyMasterPassword(const std::string& profile_id, const std::string& password) {
        auto profile_opt = metadata_manager_->getProfile(profile_id);
        if (!profile_opt) {
            last_error_ = "Profile not found";
            return false;
        }

        auto result = encryption_manager_->verifyPassword(password, profile_opt->hashedPassword);
        return result.is_valid;
    }

    std::vector<VaultFolder> getFolders(const std::string& profile_id) {
        auto folders_metadata = metadata_manager_->loadFoldersMetadata(profile_id);
        std::vector<VaultFolder> result;

        for (const auto& folder_meta : folders_metadata.folders) {
            VaultFolder folder;
            folder.id = folder_meta.id;
            folder.profile_id = profile_id;
            folder.folder_name = folder_meta.folderName;
            folder.original_path = folder_meta.originalPath;
            folder.vault_path = folder_meta.vaultPath.value_or("");
            folder.is_locked = folder_meta.isLocked;
            folder.uses_master_password = folder_meta.usesMasterPassword;
            folder.created_at = std::chrono::system_clock::from_time_t(folder_meta.createdAt / 1000);

            // Convert unlock mode
            if (folder_meta.unlockMode) {
                if (folder_meta.unlockMode == "temporary") {
                    folder.unlock_mode = UnlockMode::TEMPORARY;
                } else if (folder_meta.unlockMode == "permanent") {
                    folder.unlock_mode = UnlockMode::PERMANENT;
                }
            }

            result.push_back(folder);
        }

        return result;
    }

    std::shared_ptr<VaultFolder> getFolder(const std::string& profile_id, const std::string& folder_id) {
        auto folder_opt = metadata_manager_->getFolder(profile_id, folder_id);
        if (!folder_opt) {
            return nullptr;
        }

        auto vault_folder = std::make_shared<VaultFolder>();
        vault_folder->id = folder_opt->id;
        vault_folder->profile_id = profile_id;
        vault_folder->folder_name = folder_opt->folderName;
        vault_folder->original_path = folder_opt->originalPath;
        vault_folder->vault_path = folder_opt->vaultPath.value_or("");
        vault_folder->is_locked = folder_opt->isLocked;
        vault_folder->uses_master_password = folder_opt->usesMasterPassword;
        vault_folder->created_at = std::chrono::system_clock::from_time_t(folder_opt->createdAt / 1000);

        return vault_folder;
    }

    UnlockResult unlockFolders(
        const std::string& profile_id,
        const std::string& password,
        UnlockMode mode
    ) {
        UnlockResult result;
        
        // Verify password first
        if (!verifyMasterPassword(profile_id, password)) {
            result.error_messages.push_back("Invalid password");
            return result;
        }

        std::cout << "[ServiceVaultManager] Unlocking folders for profile: " << profile_id << std::endl;
        std::cout << "  Mode: " << (mode == UnlockMode::TEMPORARY ? "Temporary" : "Permanent") << std::endl;

        // Get locked folders that use master password
        auto folders = getFolders(profile_id);
        
        // Start transaction for atomic operations
        if (!storage_manager_->beginTransaction()) {
            result.error_messages.push_back("Failed to begin transaction: " + storage_manager_->getLastError());
            return result;
        }

        try {
            for (auto& folder : folders) {
                if (folder.is_locked && folder.uses_master_password) {
                    if (unlockSingleFolder(folder, password, mode)) {
                        result.success_count++;
                        std::cout << "  ✅ Unlocked: " << folder.folder_name << std::endl;
                        
                        // Track temporary unlocks
                        if (mode == UnlockMode::TEMPORARY) {
                            registerTemporaryUnlock(folder.id, folder.original_path);
                        }
                    } else {
                        result.failed_count++;
                        result.failed_folder_ids.push_back(folder.id);
                        result.error_messages.push_back("Failed to unlock: " + folder.folder_name);
                        std::cout << "  ❌ Failed: " << folder.folder_name << std::endl;
                    }
                }
            }

            // Commit transaction if all operations succeeded
            if (result.failed_count == 0) {
                storage_manager_->commitTransaction();
            } else {
                storage_manager_->rollbackTransaction();
            }

        } catch (const std::exception& e) {
            storage_manager_->rollbackTransaction();
            result.error_messages.push_back("Unlock operation failed: " + std::string(e.what()));
        }

        std::cout << "[ServiceVaultManager] Unlock complete: " << result.success_count 
                  << " success, " << result.failed_count << " failed" << std::endl;

        return result;
    }

    UnlockResult unlockWithRecoveryKey(
        const std::string& profile_id,
        const std::string& recovery_key
    ) {
        UnlockResult result;
        
        auto profile_opt = metadata_manager_->getProfile(profile_id);
        if (!profile_opt) {
            result.error_messages.push_back("Profile not found");
            return result;
        }

        // Decrypt master password using recovery key
        std::string master_password = decryptRecoveryKey(profile_opt->encryptedRecoveryKey, recovery_key);
        if (master_password.empty()) {
            result.error_messages.push_back("Invalid recovery key");
            return result;
        }

        std::cout << "[ServiceVaultManager] Unlocking with recovery key for profile: " << profile_id << std::endl;

        // Use master password to unlock folders in temporary mode
        return unlockFolders(profile_id, master_password, UnlockMode::TEMPORARY);
    }

    int lockAllTemporaryFolders(const std::string& profile_id, const std::string& password) {
        int locked_count = 0;
        
        std::cout << "[ServiceVaultManager] Locking temporary folders for profile: " << profile_id << std::endl;

        // Get temporarily unlocked folders
        std::vector<std::string> temp_folder_ids;
        {
            std::lock_guard<std::mutex> lock(temporary_unlocks_mutex_);
            for (const auto& [folder_id, original_path] : temporary_unlocks_) {
                temp_folder_ids.push_back(folder_id);
            }
        }

        // Start transaction
        if (!storage_manager_->beginTransaction()) {
            std::cerr << "Failed to begin transaction for locking: " << storage_manager_->getLastError() << std::endl;
            return 0;
        }

        try {
            for (const auto& folder_id : temp_folder_ids) {
                auto folder = getFolder(profile_id, folder_id);
                if (folder && !folder->is_locked) {
                    if (lockSingleFolder(*folder, password)) {
                        locked_count++;
                        unregisterTemporaryUnlock(folder_id);
                        std::cout << "  ✅ Locked: " << folder->folder_name << std::endl;
                    } else {
                        std::cout << "  ❌ Failed to lock: " << folder->folder_name << std::endl;
                    }
                }
            }

            storage_manager_->commitTransaction();

        } catch (const std::exception& e) {
            storage_manager_->rollbackTransaction();
            std::cerr << "Lock operation failed: " << e.what() << std::endl;
        }

        std::cout << "[ServiceVaultManager] Locked " << locked_count << " temporary folders" << std::endl;
        return locked_count;
    }

    std::vector<VaultFolder> getTemporaryUnlockedFolders(const std::string& profile_id) {
        std::vector<VaultFolder> result;
        
        std::lock_guard<std::mutex> lock(temporary_unlocks_mutex_);
        for (const auto& [folder_id, original_path] : temporary_unlocks_) {
            auto folder = getFolder(profile_id, folder_id);
            if (folder && !folder->is_locked) {
                result.push_back(*folder);
            }
        }
        
        return result;
    }

    bool hasTemporaryUnlockedFolders(const std::string& profile_id) {
        std::lock_guard<std::mutex> lock(temporary_unlocks_mutex_);
        return !temporary_unlocks_.empty();
    }

    std::string getVaultBasePath() const {
        return storage_manager_->getVaultBasePath().string();
    }

    std::string getUserVaultPath() const {
        return storage_manager_->getUserVaultPath().string();
    }

    std::string getLastError() const {
        return last_error_;
    }

private:
    std::string getCurrentUsername() {
        const char* user = getenv("USER");
        if (!user) {
            struct passwd* pw = getpwuid(getuid());
            user = pw->pw_name;
        }
        return std::string(user);
    }

    std::string generateProfileId() {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        
        return "profile_" + std::to_string(timestamp) + "_" + std::to_string(dis(gen));
    }

    std::string encryptRecoveryKey(const std::string& recovery_key, const std::string& master_password) {
        // Use proper key derivation and encryption
        auto salt = encryption_manager_->generateSalt();
        auto key = encryption_manager_->deriveKey(master_password, salt);
        
        // Create a simple encrypted format: salt:encrypted_key
        std::stringstream ss;
        for (uint8_t byte : salt) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        ss << ":";
        
        // Simple XOR encryption with derived key (sufficient for recovery key storage)
        std::string encrypted_key;
        for (size_t i = 0; i < recovery_key.length(); ++i) {
            encrypted_key += static_cast<char>(recovery_key[i] ^ key[i % key.size()]);
        }
        
        for (char c : encrypted_key) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
        }
        
        return ss.str();
    }

    std::string decryptRecoveryKey(const std::string& encrypted_recovery_key, const std::string& recovery_key) {
        // Parse the encrypted format: salt:encrypted_key
        size_t colon_pos = encrypted_recovery_key.find(':');
        if (colon_pos == std::string::npos) {
            return "";
        }
        
        std::string salt_hex = encrypted_recovery_key.substr(0, colon_pos);
        std::string encrypted_hex = encrypted_recovery_key.substr(colon_pos + 1);
        
        // Convert hex salt back to bytes
        std::vector<uint8_t> salt;
        for (size_t i = 0; i < salt_hex.length(); i += 2) {
            std::string byte_str = salt_hex.substr(i, 2);
            salt.push_back(static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16)));
        }
        
        // Convert hex encrypted key back to bytes
        std::string encrypted_key;
        for (size_t i = 0; i < encrypted_hex.length(); i += 2) {
            std::string byte_str = encrypted_hex.substr(i, 2);
            encrypted_key += static_cast<char>(std::stoi(byte_str, nullptr, 16));
        }
        
        // Try to decrypt with the provided recovery key
        auto key = encryption_manager_->deriveKey(recovery_key, salt);
        std::string decrypted;
        for (size_t i = 0; i < encrypted_key.length(); ++i) {
            decrypted += static_cast<char>(encrypted_key[i] ^ key[i % key.size()]);
        }
        
        // Verify this matches the expected recovery key format
        if (decrypted == recovery_key) {
            // This is a simplified approach - in practice, we'd return the master password
            // For now, return the recovery key to indicate successful validation
            return recovery_key;
        }
        
        return "";
    }

    bool unlockSingleFolder(VaultFolder& folder, const std::string& password, UnlockMode mode) {
        try {
            fs::path vault_path = storage_manager_->generateVaultPath(folder.folder_name, folder.id);
            fs::path original_path(folder.original_path);

            // Create backup before operation
            auto backup_path = storage_manager_->generateBackupPath(folder.folder_name, "pre-unlock");
            auto backup_result = storage_manager_->createBackup(vault_path, backup_path, "pre-unlock");
            if (!backup_result.success) {
                last_error_ = "Failed to create backup: " + backup_result.error_message;
                return false;
            }

            // Decrypt folder
            auto decrypt_result = encryption_manager_->decryptFolder(vault_path, password);
            if (!decrypt_result.success) {
                last_error_ = "Failed to decrypt folder: " + decrypt_result.error_message;
                return false;
            }

            // Move from vault to original location
            auto move_result = storage_manager_->moveFromVault(vault_path, original_path);
            if (!move_result.success) {
                last_error_ = "Failed to move from vault: " + move_result.error_message;
                return false;
            }

            // Update metadata
            std::string unlock_mode_str = (mode == UnlockMode::TEMPORARY) ? "temporary" : "permanent";
            if (!metadata_manager_->updateFolderState(folder.profile_id, folder.id, false, std::nullopt, unlock_mode_str)) {
                last_error_ = "Failed to update metadata: " + metadata_manager_->getLastError();
                return false;
            }

            // Add backup entry
            metadata_manager_->addBackupEntry(folder.profile_id, folder.id, backup_path.string(), "pre-unlock");

            // Update folder object
            folder.is_locked = false;
            folder.unlock_mode = mode;
            folder.vault_path = "";

            return true;

        } catch (const std::exception& e) {
            last_error_ = "Unlock operation failed: " + std::string(e.what());
            return false;
        }
    }

    bool lockSingleFolder(VaultFolder& folder, const std::string& password) {
        try {
            fs::path original_path(folder.original_path);
            fs::path vault_path = storage_manager_->generateVaultPath(folder.folder_name, folder.id);

            // Create backup before operation
            auto backup_path = storage_manager_->generateBackupPath(folder.folder_name, "pre-lock");
            auto backup_result = storage_manager_->createBackup(original_path, backup_path, "pre-lock");
            if (!backup_result.success) {
                last_error_ = "Failed to create backup: " + backup_result.error_message;
                return false;
            }

            // Move to vault location
            auto move_result = storage_manager_->moveToVault(original_path, vault_path);
            if (!move_result.success) {
                last_error_ = "Failed to move to vault: " + move_result.error_message;
                return false;
            }

            // Encrypt folder
            auto encrypt_result = encryption_manager_->encryptFolder(vault_path, password);
            if (!encrypt_result.success) {
                last_error_ = "Failed to encrypt folder: " + encrypt_result.error_message;
                return false;
            }

            // Update metadata
            if (!metadata_manager_->updateFolderState(folder.profile_id, folder.id, true, vault_path.string())) {
                last_error_ = "Failed to update metadata: " + metadata_manager_->getLastError();
                return false;
            }

            // Add backup entry
            metadata_manager_->addBackupEntry(folder.profile_id, folder.id, backup_path.string(), "pre-lock");

            // Update folder object
            folder.is_locked = true;
            folder.vault_path = vault_path.string();

            return true;

        } catch (const std::exception& e) {
            last_error_ = "Lock operation failed: " + std::string(e.what());
            return false;
        }
    }

    void registerTemporaryUnlock(const std::string& folder_id, const std::string& original_path) {
        std::lock_guard<std::mutex> lock(temporary_unlocks_mutex_);
        temporary_unlocks_[folder_id] = original_path;
        std::cout << "[ServiceVaultManager] Registered temporary unlock: " << folder_id << std::endl;
    }

    void unregisterTemporaryUnlock(const std::string& folder_id) {
        std::lock_guard<std::mutex> lock(temporary_unlocks_mutex_);
        temporary_unlocks_.erase(folder_id);
        std::cout << "[ServiceVaultManager] Unregistered temporary unlock: " << folder_id << std::endl;
    }

    // Member variables
    std::unique_ptr<VaultMetadataManager> metadata_manager_;
    std::unique_ptr<VaultEncryptionManager> encryption_manager_;
    std::unique_ptr<VaultStorageManager> storage_manager_;
    
    std::string username_;
    
    // Temporary unlock tracking
    std::unordered_map<std::string, std::string> temporary_unlocks_;
    std::mutex temporary_unlocks_mutex_;
    
    std::string last_error_;
};

// ServiceVaultManager public interface implementation
ServiceVaultManager::ServiceVaultManager() : pimpl(std::make_unique<Implementation>()) {}
ServiceVaultManager::~ServiceVaultManager() = default;

bool ServiceVaultManager::initialize() {
    return pimpl->initialize();
}

std::shared_ptr<VaultProfile> ServiceVaultManager::getActiveProfile() {
    return pimpl->getActiveProfile();
}

std::shared_ptr<VaultProfile> ServiceVaultManager::createProfile(
    const std::string& name,
    const std::string& master_password,
    const std::string& recovery_key
) {
    return pimpl->createProfile(name, master_password, recovery_key);
}

bool ServiceVaultManager::verifyMasterPassword(const std::string& profile_id, const std::string& password) {
    return pimpl->verifyMasterPassword(profile_id, password);
}

std::vector<VaultFolder> ServiceVaultManager::getFolders(const std::string& profile_id) {
    return pimpl->getFolders(profile_id);
}

std::shared_ptr<VaultFolder> ServiceVaultManager::getFolder(const std::string& profile_id, const std::string& folder_id) {
    return pimpl->getFolder(profile_id, folder_id);
}

UnlockResult ServiceVaultManager::unlockFolders(
    const std::string& profile_id,
    const std::string& password,
    UnlockMode mode
) {
    return pimpl->unlockFolders(profile_id, password, mode);
}

UnlockResult ServiceVaultManager::unlockWithRecoveryKey(
    const std::string& profile_id,
    const std::string& recovery_key
) {
    return pimpl->unlockWithRecoveryKey(profile_id, recovery_key);
}

int ServiceVaultManager::lockAllTemporaryFolders(const std::string& profile_id, const std::string& password) {
    return pimpl->lockAllTemporaryFolders(profile_id, password);
}

std::vector<VaultFolder> ServiceVaultManager::getTemporaryUnlockedFolders(const std::string& profile_id) {
    return pimpl->getTemporaryUnlockedFolders(profile_id);
}

bool ServiceVaultManager::hasTemporaryUnlockedFolders(const std::string& profile_id) {
    return pimpl->hasTemporaryUnlockedFolders(profile_id);
}

std::string ServiceVaultManager::getVaultBasePath() const {
    return pimpl->getVaultBasePath();
}

std::string ServiceVaultManager::getUserVaultPath() const {
    return pimpl->getUserVaultPath();
}

std::string ServiceVaultManager::getLastError() const {
    return pimpl->getLastError();
}

} // namespace service
} // namespace phantom_vault