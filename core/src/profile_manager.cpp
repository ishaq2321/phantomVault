/**
 * PhantomVault Profile Manager Implementation
 * 
 * Complete implementation for secure profile management with admin-only creation,
 * master key authentication, and recovery key generation.
 */

#include "profile_manager.hpp"
#include "profile_vault.hpp"
#include "error_handler.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <nlohmann/json.hpp>

#ifdef PLATFORM_LINUX
#include <unistd.h>
#include <pwd.h>
#elif PLATFORM_WINDOWS
#include <windows.h>
#include <lmcons.h>
#include <shlobj.h>
#include <sddl.h>
#elif PLATFORM_MACOS
#include <unistd.h>
#include <pwd.h>
#endif

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace phantomvault {

class ProfileManager::Implementation {
public:
    Implementation() 
        : data_path_()
        , vault_manager_()
        , active_profile_id_()
        , last_error_()
        , error_handler_(std::make_unique<ErrorHandler>())
    {}
    
    bool initialize(const std::string& dataPath) {
        try {
            // Set data path
            if (dataPath.empty()) {
                data_path_ = getDefaultDataPath();
            } else {
                data_path_ = dataPath;
            }
            
            // Ensure data directory exists
            if (!fs::exists(data_path_)) {
                fs::create_directories(data_path_);
                fs::permissions(data_path_, fs::perms::owner_all, fs::perm_options::replace);
            }
            
            // Ensure profiles directory exists
            fs::path profiles_dir = fs::path(data_path_) / "profiles";
            if (!fs::exists(profiles_dir)) {
                fs::create_directories(profiles_dir);
                fs::permissions(profiles_dir, fs::perms::owner_all, fs::perm_options::replace);
            }
            
            // Initialize VaultManager with vaults subdirectory
            std::string vault_root = data_path_ + "/vaults";
            vault_manager_ = std::make_unique<PhantomVault::VaultManager>(vault_root);
            
            if (!vault_manager_->initializeVaultSystem()) {
                last_error_ = "Failed to initialize vault system: " + vault_manager_->getLastError();
                return false;
            }
            
            // Initialize error handler
            std::string error_log_path = data_path_ + "/logs/profile_security.log";
            if (!error_handler_->initialize(error_log_path)) {
                last_error_ = "Failed to initialize error handler: " + error_handler_->getLastError();
                return false;
            }
            
            std::cout << "[ProfileManager] Initialized with data path: " << data_path_ << std::endl;
            std::cout << "[ProfileManager] Vault system initialized: " << vault_root << std::endl;
            std::cout << "[ProfileManager] Error handler initialized: " << error_log_path << std::endl;
            std::cout << "[ProfileManager] Admin mode: " << (isRunningAsAdmin() ? "Yes" : "No") << std::endl;
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to initialize ProfileManager: " + std::string(e.what());
            return false;
        }
    }
    
    ProfileResult createProfile(const std::string& name, const std::string& masterKey) {
        ProfileResult result;
        
        try {
            // Check admin privileges
            if (!isRunningAsAdmin()) {
                result.error = "Admin privileges required for profile creation";
                return result;
            }
            
            // Validate input
            if (name.empty()) {
                result.error = "Profile name cannot be empty";
                return result;
            }
            
            if (masterKey.length() < 4) {
                result.error = "Master key must be at least 4 characters";
                return result;
            }
            
            // Check if profile name already exists
            auto profiles = getAllProfiles();
            for (const auto& profile : profiles) {
                if (profile.name == name) {
                    result.error = "Profile name already exists";
                    return result;
                }
            }
            
            // Generate profile ID
            std::string profileId = generateProfileId();
            
            // Generate recovery key
            std::string recoveryKey = generateRecoveryKey();
            
            // Hash master key
            std::string masterKeyHash = hashPassword(masterKey);
            
            // Encrypt recovery key with master key
            std::string encryptedRecoveryKey = encryptRecoveryKey(recoveryKey, masterKey);
            
            // Hash recovery key for validation
            std::string recoveryKeyHash = hashRecoveryKey(recoveryKey);
            
            // Encrypt master key with recovery key for recovery purposes
            std::string masterKeyEncryptedWithRecovery = encryptMasterKeyWithRecoveryKey(masterKey, recoveryKey);
            
            // Create profile data
            json profileData;
            profileData["id"] = profileId;
            profileData["name"] = name;
            profileData["masterKeyHash"] = masterKeyHash;
            profileData["encryptedRecoveryKey"] = encryptedRecoveryKey;
            profileData["recoveryKeyHash"] = recoveryKeyHash;
            profileData["masterKeyEncryptedWithRecovery"] = masterKeyEncryptedWithRecovery;
            profileData["createdAt"] = getCurrentTimestamp();
            profileData["lastAccess"] = getCurrentTimestamp();
            
            // Create ProfileVault for this profile
            if (!vault_manager_->createProfileVault(profileId)) {
                result.error = "Failed to create profile vault: " + vault_manager_->getLastError();
                return result;
            }
            
            // Save profile
            fs::path profileFile = fs::path(data_path_) / "profiles" / (profileId + ".json");
            std::ofstream file(profileFile);
            if (!file) {
                result.error = "Failed to create profile file";
                return result;
            }
            
            file << profileData.dump(2);
            file.close();
            
            // Set secure permissions
            fs::permissions(profileFile, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace);
            
            // Update profiles index
            updateProfilesIndex();
            
            result.success = true;
            result.profileId = profileId;
            result.recoveryKey = recoveryKey;
            result.message = "Profile and encrypted vault created successfully";
            
            std::cout << "[ProfileManager] Created profile with vault: " << name << " (ID: " << profileId << ")" << std::endl;
            
            return result;
            
        } catch (const std::exception& e) {
            result.error = "Failed to create profile: " + std::string(e.what());
            return result;
        }
    }
    
    std::vector<Profile> getAllProfiles() {
        std::vector<Profile> profiles;
        
        try {
            fs::path profiles_dir = fs::path(data_path_) / "profiles";
            if (!fs::exists(profiles_dir)) {
                return profiles;
            }
            
            for (const auto& entry : fs::directory_iterator(profiles_dir)) {
                if (entry.path().extension() == ".json") {
                    auto profile = loadProfile(entry.path());
                    if (profile) {
                        profiles.push_back(*profile);
                    }
                }
            }
            
            // Sort by creation time
            std::sort(profiles.begin(), profiles.end(), 
                     [](const Profile& a, const Profile& b) {
                         return a.createdAt < b.createdAt;
                     });
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to load profiles: " + std::string(e.what());
        }
        
        return profiles;
    }
    
    std::optional<Profile> getProfile(const std::string& profileId) {
        try {
            fs::path profileFile = fs::path(data_path_) / "profiles" / (profileId + ".json");
            return loadProfile(profileFile);
        } catch (const std::exception& e) {
            last_error_ = "Failed to get profile: " + std::string(e.what());
            return std::nullopt;
        }
    }
    
    bool deleteProfile(const std::string& profileId, const std::string& masterKey) {
        try {
            // Verify master key first
            if (!verifyMasterKey(profileId, masterKey)) {
                last_error_ = "Invalid master key";
                return false;
            }
            
            // Delete ProfileVault first (this will handle encrypted data cleanup)
            if (!vault_manager_->deleteProfileVault(profileId, masterKey)) {
                last_error_ = "Failed to delete profile vault: " + vault_manager_->getLastError();
                return false;
            }
            
            // Remove profile file
            fs::path profileFile = fs::path(data_path_) / "profiles" / (profileId + ".json");
            if (fs::exists(profileFile)) {
                fs::remove(profileFile);
            }
            
            // Clear active profile if it was the deleted one
            if (active_profile_id_ == profileId) {
                clearActiveProfile();
            }
            
            // Update profiles index
            updateProfilesIndex();
            
            std::cout << "[ProfileManager] Deleted profile and vault: " << profileId << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to delete profile: " + std::string(e.what());
            return false;
        }
    }
    
    AuthResult authenticateProfile(const std::string& profileId, const std::string& masterKey) {
        AuthResult result;
        
        try {
            auto profile = getProfile(profileId);
            if (!profile) {
                result.error = "Profile not found";
                return result;
            }
            
            if (verifyMasterKey(profileId, masterKey)) {
                result.success = true;
                result.profileId = profileId;
                result.message = "Authentication successful";
                
                // Update last access time
                updateLastAccess(profileId);
                
                // Log successful authentication
                if (error_handler_) {
                    error_handler_->logSecurityEvent(SecurityEventType::AUTHENTICATION_FAILURE, 
                                                   ErrorSeverity::INFO, profileId, 
                                                   "Profile authentication successful", {});
                }
                
                std::cout << "[ProfileManager] Authenticated profile: " << profile->name << std::endl;
            } else {
                result.error = error_handler_ ? 
                    error_handler_->getSecureErrorMessage(SecurityEventType::AUTHENTICATION_FAILURE) :
                    "Invalid master key";
                
                // Log authentication failure with rate limiting
                if (error_handler_) {
                    error_handler_->handleAuthenticationFailure(profileId, "ProfileManager", 
                                                              "Master key verification failed");
                }
            }
            
            return result;
            
        } catch (const std::exception& e) {
            result.error = "Authentication failed: " + std::string(e.what());
            return result;
        }
    }
    
    bool verifyMasterKey(const std::string& profileId, const std::string& masterKey) {
        try {
            fs::path profileFile = fs::path(data_path_) / "profiles" / (profileId + ".json");
            if (!fs::exists(profileFile)) {
                return false;
            }
            
            std::ifstream file(profileFile);
            json profileData;
            file >> profileData;
            
            std::string storedHash = profileData["masterKeyHash"];
            return verifyPassword(masterKey, storedHash);
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to verify master key: " + std::string(e.what());
            return false;
        }
    }
    
    ProfileResult changeProfilePassword(const std::string& profileId, const std::string& oldKey, const std::string& newKey) {
        ProfileResult result;
        
        try {
            // Verify old key first
            if (!verifyMasterKey(profileId, oldKey)) {
                result.error = "Invalid current master key";
                return result;
            }
            
            // Validate new key
            if (newKey.length() < 4) {
                result.error = "New master key must be at least 4 characters";
                return result;
            }
            
            // Get ProfileVault to handle encrypted data re-encryption
            auto profile_vault = vault_manager_->getProfileVault(profileId);
            if (profile_vault) {
                // Get all locked folders
                auto locked_folders = profile_vault->getLockedFolders();
                
                // Temporarily unlock all folders with old key
                std::vector<std::string> temp_unlocked_paths;
                for (const auto& folder : locked_folders) {
                    auto unlock_result = profile_vault->unlockFolder(folder.original_path, oldKey, PhantomVault::UnlockMode::TEMPORARY);
                    if (unlock_result.success) {
                        temp_unlocked_paths.push_back(folder.original_path);
                    }
                }
                
                // Re-lock all folders with new key
                for (const auto& folder_path : temp_unlocked_paths) {
                    auto lock_result = profile_vault->lockFolder(folder_path, newKey);
                    if (!lock_result.success) {
                        result.error = "Failed to re-encrypt vault data with new key";
                        return result;
                    }
                }
            }
            
            // Generate new recovery key
            std::string newRecoveryKey = generateRecoveryKey();
            
            // Hash new master key
            std::string newMasterKeyHash = hashPassword(newKey);
            
            // Encrypt new recovery key with new master key
            std::string newEncryptedRecoveryKey = encryptRecoveryKey(newRecoveryKey, newKey);
            
            // Hash new recovery key for validation
            std::string newRecoveryKeyHash = hashRecoveryKey(newRecoveryKey);
            
            // Encrypt new master key with new recovery key
            std::string newMasterKeyEncryptedWithRecovery = encryptMasterKeyWithRecoveryKey(newKey, newRecoveryKey);
            
            // Update profile
            fs::path profileFile = fs::path(data_path_) / "profiles" / (profileId + ".json");
            std::ifstream inFile(profileFile);
            json profileData;
            inFile >> profileData;
            inFile.close();
            
            profileData["masterKeyHash"] = newMasterKeyHash;
            profileData["encryptedRecoveryKey"] = newEncryptedRecoveryKey;
            profileData["recoveryKeyHash"] = newRecoveryKeyHash;
            profileData["masterKeyEncryptedWithRecovery"] = newMasterKeyEncryptedWithRecovery;
            profileData["lastAccess"] = getCurrentTimestamp();
            
            std::ofstream outFile(profileFile);
            outFile << profileData.dump(2);
            outFile.close();
            
            result.success = true;
            result.profileId = profileId;
            result.recoveryKey = newRecoveryKey;
            result.message = "Password changed successfully and vault data re-encrypted";
            
            std::cout << "[ProfileManager] Changed password and re-encrypted vault for profile: " << profileId << std::endl;
            
            return result;
            
        } catch (const std::exception& e) {
            result.error = "Failed to change password: " + std::string(e.what());
            return result;
        }
    }
    
    std::string recoverMasterKey(const std::string& recoveryKey) {
        try {
            // Search all profiles for matching recovery key
            auto profiles = getAllProfiles();
            for (const auto& profile : profiles) {
                fs::path profileFile = fs::path(data_path_) / "profiles" / (profile.id + ".json");
                std::ifstream file(profileFile);
                json profileData;
                file >> profileData;
                
                // For recovery key validation, we need to check if the provided recovery key
                // matches the one stored for this profile. Since we can't decrypt without
                // the master key, we store a hash of the recovery key for validation.
                if (profileData.contains("recoveryKeyHash")) {
                    std::string storedRecoveryHash = profileData["recoveryKeyHash"];
                    std::string providedRecoveryHash = hashRecoveryKey(recoveryKey);
                    
                    if (storedRecoveryHash == providedRecoveryHash) {
                        // Recovery key matches - return profile ID for validation
                        return profile.id;
                    }
                }
            }
            
            return ""; // Recovery key not found
            
        } catch (const std::exception& e) {
            last_error_ = "Recovery failed: " + std::string(e.what());
            return "";
        }
    }
    
    std::optional<std::string> getProfileIdFromRecoveryKey(const std::string& recoveryKey) {
        std::string profileId = recoverMasterKey(recoveryKey);
        if (!profileId.empty()) {
            return profileId;
        }
        return std::nullopt;
    }
    
    std::optional<std::string> recoverMasterKeyFromRecoveryKey(const std::string& recoveryKey) {
        try {
            // First validate the recovery key and get profile ID
            auto profileId = getProfileIdFromRecoveryKey(recoveryKey);
            if (!profileId.has_value()) {
                return std::nullopt;
            }
            
            // Load profile data
            fs::path profileFile = fs::path(data_path_) / "profiles" / (profileId.value() + ".json");
            std::ifstream file(profileFile);
            json profileData;
            file >> profileData;
            
            if (!profileData.contains("encryptedRecoveryKey")) {
                return std::nullopt;
            }
            
            // The recovery key itself is stored encrypted with the master key
            // For recovery, we need a different approach: store the master key encrypted with recovery key
            if (profileData.contains("masterKeyEncryptedWithRecovery")) {
                std::string encryptedMasterKey = profileData["masterKeyEncryptedWithRecovery"];
                std::string decryptedMasterKey = decryptMasterKeyWithRecoveryKey(encryptedMasterKey, recoveryKey);
                
                if (!decryptedMasterKey.empty()) {
                    return decryptedMasterKey;
                }
            }
            
            return std::nullopt;
            
        } catch (const std::exception& e) {
            last_error_ = "Master key recovery failed: " + std::string(e.what());
            return std::nullopt;
        }
    }
    
    std::string decryptMasterKeyWithRecoveryKey(const std::string& encryptedData, const std::string& recoveryKey) {
        try {
            // Parse encrypted data: salt:iv:encrypted_data (same format as recovery key encryption)
            std::vector<std::string> parts;
            std::stringstream ss(encryptedData);
            std::string part;
            
            while (std::getline(ss, part, ':')) {
                parts.push_back(part);
            }
            
            if (parts.size() != 3) {
                return ""; // Invalid format
            }
            
            // Convert hex parts to bytes
            unsigned char salt[16], iv[16];
            
            // Parse salt
            if (parts[0].length() != 32) return "";
            for (int i = 0; i < 16; ++i) {
                std::string byteStr = parts[0].substr(i * 2, 2);
                salt[i] = (unsigned char)std::stoi(byteStr, nullptr, 16);
            }
            
            // Parse IV
            if (parts[1].length() != 32) return "";
            for (int i = 0; i < 16; ++i) {
                std::string byteStr = parts[1].substr(i * 2, 2);
                iv[i] = (unsigned char)std::stoi(byteStr, nullptr, 16);
            }
            
            // Parse encrypted data
            std::vector<unsigned char> encrypted_bytes;
            for (size_t i = 0; i < parts[2].length(); i += 2) {
                std::string byteStr = parts[2].substr(i, 2);
                encrypted_bytes.push_back((unsigned char)std::stoi(byteStr, nullptr, 16));
            }
            
            // Derive decryption key from recovery key
            unsigned char derived_key[32];
            if (PKCS5_PBKDF2_HMAC(recoveryKey.c_str(), recoveryKey.length(), salt, sizeof(salt),
                                 50000, EVP_sha256(), sizeof(derived_key), derived_key) != 1) {
                return "";
            }
            
            // Decrypt using AES-256-CBC
            EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
            if (!ctx) {
                return "";
            }
            
            std::string decrypted_data;
            
            if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, derived_key, iv) == 1) {
                unsigned char decrypted[1024];
                int len = 0, total_len = 0;
                
                if (EVP_DecryptUpdate(ctx, decrypted, &len, encrypted_bytes.data(), encrypted_bytes.size()) == 1) {
                    total_len += len;
                    
                    if (EVP_DecryptFinal_ex(ctx, decrypted + total_len, &len) == 1) {
                        total_len += len;
                        decrypted_data = std::string(reinterpret_cast<char*>(decrypted), total_len);
                    }
                }
            }
            
            EVP_CIPHER_CTX_free(ctx);
            
            // Secure cleanup
            memset(derived_key, 0, sizeof(derived_key));
            
            return decrypted_data;
            
        } catch (const std::exception& e) {
            return "";
        }
    }
    
    std::string encryptMasterKeyWithRecoveryKey(const std::string& masterKey, const std::string& recoveryKey) {
        try {
            // Generate a unique salt for this encryption
            unsigned char salt[16];
            if (RAND_bytes(salt, sizeof(salt)) != 1) {
                throw std::runtime_error("Failed to generate salt for master key encryption");
            }
            
            // Derive encryption key from recovery key using PBKDF2
            unsigned char derived_key[32];
            if (PKCS5_PBKDF2_HMAC(recoveryKey.c_str(), recoveryKey.length(), salt, sizeof(salt),
                                 50000, EVP_sha256(), sizeof(derived_key), derived_key) != 1) {
                throw std::runtime_error("Failed to derive key for master key encryption");
            }
            
            // Generate IV for AES encryption
            unsigned char iv[16];
            if (RAND_bytes(iv, sizeof(iv)) != 1) {
                throw std::runtime_error("Failed to generate IV for master key encryption");
            }
            
            // Encrypt master key using AES-256-CBC
            EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
            if (!ctx) {
                throw std::runtime_error("Failed to create cipher context for master key");
            }
            
            std::string encrypted_data;
            
            if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, derived_key, iv) == 1) {
                unsigned char encrypted[1024];
                int len = 0, total_len = 0;
                
                if (EVP_EncryptUpdate(ctx, encrypted, &len, 
                                    reinterpret_cast<const unsigned char*>(masterKey.c_str()), 
                                    masterKey.length()) == 1) {
                    total_len += len;
                    
                    if (EVP_EncryptFinal_ex(ctx, encrypted + total_len, &len) == 1) {
                        total_len += len;
                        
                        // Create final encrypted package: salt + iv + encrypted_data
                        std::stringstream ss;
                        
                        // Add salt (hex)
                        for (int i = 0; i < 16; ++i) {
                            ss << std::hex << std::setw(2) << std::setfill('0') << (int)salt[i];
                        }
                        ss << ":";
                        
                        // Add IV (hex)
                        for (int i = 0; i < 16; ++i) {
                            ss << std::hex << std::setw(2) << std::setfill('0') << (int)iv[i];
                        }
                        ss << ":";
                        
                        // Add encrypted data (hex)
                        for (int i = 0; i < total_len; ++i) {
                            ss << std::hex << std::setw(2) << std::setfill('0') << (int)encrypted[i];
                        }
                        
                        encrypted_data = ss.str();
                    }
                }
            }
            
            EVP_CIPHER_CTX_free(ctx);
            
            // Secure cleanup
            memset(derived_key, 0, sizeof(derived_key));
            memset(iv, 0, sizeof(iv));
            memset(salt, 0, sizeof(salt));
            
            if (encrypted_data.empty()) {
                throw std::runtime_error("Master key encryption failed");
            }
            
            return encrypted_data;
            
        } catch (const std::exception& e) {
            throw std::runtime_error("Master key encryption error: " + std::string(e.what()));
        }
    }
    
    void setActiveProfile(const std::string& profileId) {
        active_profile_id_ = profileId;
        std::cout << "[ProfileManager] Set active profile: " << profileId << std::endl;
    }
    
    std::optional<Profile> getActiveProfile() {
        if (active_profile_id_.empty()) {
            return std::nullopt;
        }
        return getProfile(active_profile_id_);
    }
    
    void clearActiveProfile() {
        active_profile_id_.clear();
        std::cout << "[ProfileManager] Cleared active profile" << std::endl;
    }
    
    bool isRunningAsAdmin() {
        #ifdef PLATFORM_LINUX
        return getuid() == 0;
        #elif PLATFORM_WINDOWS
        BOOL isAdmin = FALSE;
        PSID adminGroup = NULL;
        SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
        
        if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
            CheckTokenMembership(NULL, adminGroup, &isAdmin);
            FreeSid(adminGroup);
        }
        
        return isAdmin == TRUE;
        #elif PLATFORM_MACOS
        return getuid() == 0;
        #else
        return false;
        #endif
    }
    
    bool requiresAdminForProfileCreation() {
        return true;
    }
    
    size_t getProfileVaultSize(const std::string& profileId) const {
        try {
            if (profileId.empty()) {
                return 0;
            }
            
            // Create a temporary non-const instance for vault access
            ProfileManager::Implementation* non_const_this = const_cast<ProfileManager::Implementation*>(this);
            auto profile_vault = non_const_this->vault_manager_->getProfileVault(profileId);
            if (profile_vault) {
                return profile_vault->getVaultSize();
            }
            return 0;
        } catch (const std::exception& e) {
            // Can't modify last_error_ in const method, just return 0
            return 0;
        }
    }
    
    bool validateProfileVault(const std::string& profileId) const {
        try {
            if (profileId.empty()) {
                return false;
            }
            
            // Create a temporary non-const instance for vault access
            ProfileManager::Implementation* non_const_this = const_cast<ProfileManager::Implementation*>(this);
            auto profile_vault = non_const_this->vault_manager_->getProfileVault(profileId);
            if (profile_vault) {
                return profile_vault->validateVaultIntegrity();
            }
            return false;
        } catch (const std::exception& e) {
            // Can't modify last_error_ in const method, just return false
            return false;
        }
    }
    
    bool performProfileVaultMaintenance(const std::string& profileId) {
        try {
            auto profile_vault = vault_manager_->getProfileVault(profileId);
            if (profile_vault) {
                return profile_vault->cleanupCorruptedEntries();
            }
            return false;
        } catch (const std::exception& e) {
            last_error_ = "Failed to perform vault maintenance: " + std::string(e.what());
            return false;
        }
    }
    
    std::vector<std::string> getProfileLockedFolders(const std::string& profileId) const {
        std::vector<std::string> folder_paths;
        try {
            if (profileId.empty()) {
                return folder_paths;
            }
            
            // Create a temporary non-const instance for vault access
            ProfileManager::Implementation* non_const_this = const_cast<ProfileManager::Implementation*>(this);
            auto profile_vault = non_const_this->vault_manager_->getProfileVault(profileId);
            if (profile_vault) {
                auto locked_folders = profile_vault->getLockedFolders();
                for (const auto& folder : locked_folders) {
                    folder_paths.push_back(folder.original_path);
                }
            }
        } catch (const std::exception& e) {
            // Can't modify last_error_ in const method, just return empty vector
        }
        return folder_paths;
    }

    std::string getLastError() const {
        return last_error_;
    }
    
private:
    std::string data_path_;
    std::unique_ptr<PhantomVault::VaultManager> vault_manager_;
    std::string active_profile_id_;
    std::string last_error_;
    std::unique_ptr<ErrorHandler> error_handler_;
    
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
    
    std::string generateProfileId() {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        
        return "profile_" + std::to_string(timestamp) + "_" + std::to_string(dis(gen));
    }
    
    std::string generateRecoveryKey() {
        const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, chars.size() - 1);
        
        std::string recoveryKey;
        for (int i = 0; i < 24; ++i) {
            recoveryKey += chars[dis(gen)];
            if ((i + 1) % 4 == 0 && i < 23) {
                recoveryKey += "-";
            }
        }
        
        return recoveryKey;
    }
    
    std::string hashRecoveryKey(const std::string& recoveryKey) {
        try {
            // Generate salt for recovery key hash
            unsigned char salt[16];
            if (RAND_bytes(salt, sizeof(salt)) != 1) {
                throw std::runtime_error("Failed to generate salt for recovery key hash");
            }
            
            // Hash recovery key with PBKDF2
            unsigned char hash[32];
            if (PKCS5_PBKDF2_HMAC(recoveryKey.c_str(), recoveryKey.length(), salt, sizeof(salt),
                                 100000, EVP_sha256(), sizeof(hash), hash) != 1) {
                throw std::runtime_error("Failed to hash recovery key");
            }
            
            // Combine salt and hash
            std::stringstream ss;
            for (int i = 0; i < 16; ++i) {
                ss << std::hex << std::setw(2) << std::setfill('0') << (int)salt[i];
            }
            ss << ":";
            for (int i = 0; i < 32; ++i) {
                ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
            }
            
            return ss.str();
            
        } catch (const std::exception& e) {
            throw std::runtime_error("Recovery key hashing failed: " + std::string(e.what()));
        }
    }
    
    std::string hashPassword(const std::string& password) {
        // Generate salt
        unsigned char salt[16];
        if (RAND_bytes(salt, sizeof(salt)) != 1) {
            throw std::runtime_error("Failed to generate salt");
        }
        
        // Hash password with PBKDF2
        unsigned char hash[32];
        if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(), salt, sizeof(salt),
                             100000, EVP_sha256(), sizeof(hash), hash) != 1) {
            throw std::runtime_error("Failed to hash password");
        }
        
        // Combine salt and hash
        std::stringstream ss;
        for (int i = 0; i < 16; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)salt[i];
        }
        ss << ":";
        for (int i = 0; i < 32; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        
        return ss.str();
    }
    
    bool verifyPassword(const std::string& password, const std::string& storedHash) {
        try {
            // Split salt and hash
            size_t colonPos = storedHash.find(':');
            if (colonPos == std::string::npos) {
                return false;
            }
            
            std::string saltHex = storedHash.substr(0, colonPos);
            std::string hashHex = storedHash.substr(colonPos + 1);
            
            // Convert hex salt to bytes
            unsigned char salt[16];
            for (int i = 0; i < 16; ++i) {
                std::string byteStr = saltHex.substr(i * 2, 2);
                salt[i] = (unsigned char)std::stoi(byteStr, nullptr, 16);
            }
            
            // Hash provided password
            unsigned char hash[32];
            if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(), salt, sizeof(salt),
                                 100000, EVP_sha256(), sizeof(hash), hash) != 1) {
                return false;
            }
            
            // Convert hash to hex
            std::stringstream ss;
            for (int i = 0; i < 32; ++i) {
                ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
            }
            
            return ss.str() == hashHex;
            
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    std::string encryptRecoveryKey(const std::string& recoveryKey, const std::string& masterKey) {
        try {
            // Use proper AES encryption for recovery key storage
            // Generate a unique salt for this recovery key
            unsigned char salt[16];
            if (RAND_bytes(salt, sizeof(salt)) != 1) {
                throw std::runtime_error("Failed to generate salt for recovery key encryption");
            }
            
            // Derive encryption key from master key using PBKDF2
            unsigned char derived_key[32];
            if (PKCS5_PBKDF2_HMAC(masterKey.c_str(), masterKey.length(), salt, sizeof(salt),
                                 50000, EVP_sha256(), sizeof(derived_key), derived_key) != 1) {
                throw std::runtime_error("Failed to derive key for recovery key encryption");
            }
            
            // Generate IV for AES encryption
            unsigned char iv[16];
            if (RAND_bytes(iv, sizeof(iv)) != 1) {
                throw std::runtime_error("Failed to generate IV for recovery key encryption");
            }
            
            // Encrypt recovery key using AES-256-CBC
            EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
            if (!ctx) {
                throw std::runtime_error("Failed to create cipher context for recovery key");
            }
            
            std::string encrypted_data;
            
            if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, derived_key, iv) == 1) {
                unsigned char encrypted[1024];
                int len = 0, total_len = 0;
                
                if (EVP_EncryptUpdate(ctx, encrypted, &len, 
                                    reinterpret_cast<const unsigned char*>(recoveryKey.c_str()), 
                                    recoveryKey.length()) == 1) {
                    total_len += len;
                    
                    if (EVP_EncryptFinal_ex(ctx, encrypted + total_len, &len) == 1) {
                        total_len += len;
                        
                        // Create final encrypted package: salt + iv + encrypted_data
                        std::stringstream ss;
                        
                        // Add salt (hex)
                        for (int i = 0; i < 16; ++i) {
                            ss << std::hex << std::setw(2) << std::setfill('0') << (int)salt[i];
                        }
                        ss << ":";
                        
                        // Add IV (hex)
                        for (int i = 0; i < 16; ++i) {
                            ss << std::hex << std::setw(2) << std::setfill('0') << (int)iv[i];
                        }
                        ss << ":";
                        
                        // Add encrypted data (hex)
                        for (int i = 0; i < total_len; ++i) {
                            ss << std::hex << std::setw(2) << std::setfill('0') << (int)encrypted[i];
                        }
                        
                        encrypted_data = ss.str();
                    }
                }
            }
            
            EVP_CIPHER_CTX_free(ctx);
            
            // Secure cleanup
            memset(derived_key, 0, sizeof(derived_key));
            memset(iv, 0, sizeof(iv));
            memset(salt, 0, sizeof(salt));
            
            if (encrypted_data.empty()) {
                throw std::runtime_error("Recovery key encryption failed");
            }
            
            return encrypted_data;
            
        } catch (const std::exception& e) {
            throw std::runtime_error("Recovery key encryption error: " + std::string(e.what()));
        }
    }
    
    std::string decryptRecoveryKey(const std::string& encryptedData, const std::string& masterKey) {
        try {
            // Parse encrypted data: salt:iv:encrypted_data
            std::vector<std::string> parts;
            std::stringstream ss(encryptedData);
            std::string part;
            
            while (std::getline(ss, part, ':')) {
                parts.push_back(part);
            }
            
            if (parts.size() != 3) {
                return ""; // Invalid format
            }
            
            // Convert hex parts to bytes
            unsigned char salt[16], iv[16];
            
            // Parse salt
            if (parts[0].length() != 32) return "";
            for (int i = 0; i < 16; ++i) {
                std::string byteStr = parts[0].substr(i * 2, 2);
                salt[i] = (unsigned char)std::stoi(byteStr, nullptr, 16);
            }
            
            // Parse IV
            if (parts[1].length() != 32) return "";
            for (int i = 0; i < 16; ++i) {
                std::string byteStr = parts[1].substr(i * 2, 2);
                iv[i] = (unsigned char)std::stoi(byteStr, nullptr, 16);
            }
            
            // Parse encrypted data
            std::vector<unsigned char> encrypted_bytes;
            for (size_t i = 0; i < parts[2].length(); i += 2) {
                std::string byteStr = parts[2].substr(i, 2);
                encrypted_bytes.push_back((unsigned char)std::stoi(byteStr, nullptr, 16));
            }
            
            // Derive decryption key from master key
            unsigned char derived_key[32];
            if (PKCS5_PBKDF2_HMAC(masterKey.c_str(), masterKey.length(), salt, sizeof(salt),
                                 50000, EVP_sha256(), sizeof(derived_key), derived_key) != 1) {
                return "";
            }
            
            // Decrypt using AES-256-CBC
            EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
            if (!ctx) {
                return "";
            }
            
            std::string decrypted_data;
            
            if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, derived_key, iv) == 1) {
                unsigned char decrypted[1024];
                int len = 0, total_len = 0;
                
                if (EVP_DecryptUpdate(ctx, decrypted, &len, encrypted_bytes.data(), encrypted_bytes.size()) == 1) {
                    total_len += len;
                    
                    if (EVP_DecryptFinal_ex(ctx, decrypted + total_len, &len) == 1) {
                        total_len += len;
                        decrypted_data = std::string(reinterpret_cast<char*>(decrypted), total_len);
                    }
                }
            }
            
            EVP_CIPHER_CTX_free(ctx);
            
            // Secure cleanup
            memset(derived_key, 0, sizeof(derived_key));
            
            return decrypted_data;
            
        } catch (const std::exception& e) {
            return "";
        }
    }
    
    int64_t getCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
    
    std::optional<Profile> loadProfile(const fs::path& profileFile) {
        try {
            if (!fs::exists(profileFile)) {
                return std::nullopt;
            }
            
            std::ifstream file(profileFile);
            json profileData;
            file >> profileData;
            
            Profile profile;
            profile.id = profileData["id"];
            profile.name = profileData["name"];
            int64_t createdAtMs = profileData["createdAt"];
            int64_t lastAccessMs = profileData["lastAccess"];
            profile.createdAt = std::chrono::system_clock::from_time_t(createdAtMs / 1000);
            profile.lastAccess = std::chrono::system_clock::from_time_t(lastAccessMs / 1000);
            profile.isActive = (profile.id == active_profile_id_);
            
            // Get folder count from ProfileVault
            profile.folderCount = 0;
            try {
                auto profile_vault = vault_manager_->getProfileVault(profile.id);
                if (profile_vault) {
                    auto locked_folders = profile_vault->getLockedFolders();
                    profile.folderCount = locked_folders.size();
                }
            } catch (const std::exception& e) {
                // Non-critical error, just use 0
                profile.folderCount = 0;
            }
            
            return profile;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to load profile: " + std::string(e.what());
            return std::nullopt;
        }
    }
    
    void updateLastAccess(const std::string& profileId) {
        try {
            fs::path profileFile = fs::path(data_path_) / "profiles" / (profileId + ".json");
            std::ifstream inFile(profileFile);
            json profileData;
            inFile >> profileData;
            inFile.close();
            
            profileData["lastAccess"] = getCurrentTimestamp();
            
            std::ofstream outFile(profileFile);
            outFile << profileData.dump(2);
            outFile.close();
            
        } catch (const std::exception& e) {
            // Non-critical error, just log it
            std::cerr << "[ProfileManager] Failed to update last access: " << e.what() << std::endl;
        }
    }
    
    void updateProfilesIndex() {
        // This could be used to maintain an index file for faster profile loading
        // For now, we just scan the directory each time
    }
};

// ProfileManager public interface implementation
ProfileManager::ProfileManager() : pimpl(std::make_unique<Implementation>()) {}
ProfileManager::~ProfileManager() = default;

bool ProfileManager::initialize(const std::string& dataPath) {
    return pimpl->initialize(dataPath);
}

ProfileResult ProfileManager::createProfile(const std::string& name, const std::string& masterKey) {
    return pimpl->createProfile(name, masterKey);
}

std::vector<Profile> ProfileManager::getAllProfiles() {
    return pimpl->getAllProfiles();
}

std::optional<Profile> ProfileManager::getProfile(const std::string& profileId) {
    return pimpl->getProfile(profileId);
}

bool ProfileManager::deleteProfile(const std::string& profileId, const std::string& masterKey) {
    return pimpl->deleteProfile(profileId, masterKey);
}

AuthResult ProfileManager::authenticateProfile(const std::string& profileId, const std::string& masterKey) {
    return pimpl->authenticateProfile(profileId, masterKey);
}

bool ProfileManager::verifyMasterKey(const std::string& profileId, const std::string& masterKey) {
    return pimpl->verifyMasterKey(profileId, masterKey);
}

ProfileResult ProfileManager::changeProfilePassword(const std::string& profileId, const std::string& oldKey, const std::string& newKey) {
    return pimpl->changeProfilePassword(profileId, oldKey, newKey);
}

std::string ProfileManager::recoverMasterKey(const std::string& recoveryKey) {
    return pimpl->recoverMasterKey(recoveryKey);
}

std::optional<std::string> ProfileManager::getProfileIdFromRecoveryKey(const std::string& recoveryKey) {
    return pimpl->getProfileIdFromRecoveryKey(recoveryKey);
}

std::optional<std::string> ProfileManager::recoverMasterKeyFromRecoveryKey(const std::string& recoveryKey) {
    return pimpl->recoverMasterKeyFromRecoveryKey(recoveryKey);
}

void ProfileManager::setActiveProfile(const std::string& profileId) {
    pimpl->setActiveProfile(profileId);
}

std::optional<Profile> ProfileManager::getActiveProfile() {
    return pimpl->getActiveProfile();
}

void ProfileManager::clearActiveProfile() {
    pimpl->clearActiveProfile();
}

bool ProfileManager::isRunningAsAdmin() {
    return pimpl->isRunningAsAdmin();
}

bool ProfileManager::requiresAdminForProfileCreation() {
    return pimpl->requiresAdminForProfileCreation();
}

size_t ProfileManager::getProfileVaultSize(const std::string& profileId) const {
    return pimpl->getProfileVaultSize(profileId);
}

bool ProfileManager::validateProfileVault(const std::string& profileId) const {
    return pimpl->validateProfileVault(profileId);
}

bool ProfileManager::performProfileVaultMaintenance(const std::string& profileId) {
    return pimpl->performProfileVaultMaintenance(profileId);
}

std::vector<std::string> ProfileManager::getProfileLockedFolders(const std::string& profileId) const {
    return pimpl->getProfileLockedFolders(profileId);
}

std::string ProfileManager::getLastError() const {
    return pimpl->getLastError();
}

} // namespace phantomvault