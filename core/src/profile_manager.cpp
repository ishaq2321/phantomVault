/**
 * PhantomVault Profile Manager Implementation
 * 
 * Complete implementation for secure profile management with admin-only creation,
 * master key authentication, and recovery key generation.
 */

#include "profile_manager.hpp"
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
        , active_profile_id_()
        , last_error_()
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
            
            std::cout << "[ProfileManager] Initialized with data path: " << data_path_ << std::endl;
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
            
            // Create profile data
            json profileData;
            profileData["id"] = profileId;
            profileData["name"] = name;
            profileData["masterKeyHash"] = masterKeyHash;
            profileData["encryptedRecoveryKey"] = encryptedRecoveryKey;
            profileData["createdAt"] = getCurrentTimestamp();
            profileData["lastAccess"] = getCurrentTimestamp();
            
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
            result.message = "Profile created successfully";
            
            std::cout << "[ProfileManager] Created profile: " << name << " (ID: " << profileId << ")" << std::endl;
            
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
            
            std::cout << "[ProfileManager] Deleted profile: " << profileId << std::endl;
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
                
                std::cout << "[ProfileManager] Authenticated profile: " << profile->name << std::endl;
            } else {
                result.error = "Invalid master key";
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
            
            // Generate new recovery key
            std::string newRecoveryKey = generateRecoveryKey();
            
            // Hash new master key
            std::string newMasterKeyHash = hashPassword(newKey);
            
            // Encrypt new recovery key with new master key
            std::string newEncryptedRecoveryKey = encryptRecoveryKey(newRecoveryKey, newKey);
            
            // Update profile
            fs::path profileFile = fs::path(data_path_) / "profiles" / (profileId + ".json");
            std::ifstream inFile(profileFile);
            json profileData;
            inFile >> profileData;
            inFile.close();
            
            profileData["masterKeyHash"] = newMasterKeyHash;
            profileData["encryptedRecoveryKey"] = newEncryptedRecoveryKey;
            profileData["lastAccess"] = getCurrentTimestamp();
            
            std::ofstream outFile(profileFile);
            outFile << profileData.dump(2);
            outFile.close();
            
            result.success = true;
            result.profileId = profileId;
            result.recoveryKey = newRecoveryKey;
            result.message = "Password changed successfully";
            
            std::cout << "[ProfileManager] Changed password for profile: " << profileId << std::endl;
            
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
                
                std::string encryptedRecoveryKey = profileData["encryptedRecoveryKey"];
                std::string decryptedKey = decryptRecoveryKey(encryptedRecoveryKey, recoveryKey);
                
                if (decryptedKey == recoveryKey) {
                    // Recovery key matches, but we can't return the actual master key
                    // Instead, we return the profile ID to indicate success
                    return profile.id;
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
    
    std::string getLastError() const {
        return last_error_;
    }
    
private:
    std::string data_path_;
    std::string active_profile_id_;
    std::string last_error_;
    
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
        // Simple XOR encryption for recovery key storage
        // In production, this would use proper AES encryption
        std::string encrypted = recoveryKey;
        for (size_t i = 0; i < encrypted.length(); ++i) {
            encrypted[i] ^= masterKey[i % masterKey.length()];
        }
        
        // Convert to hex
        std::stringstream ss;
        for (char c : encrypted) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)c;
        }
        
        return ss.str();
    }
    
    std::string decryptRecoveryKey(const std::string& encryptedHex, const std::string& recoveryKey) {
        try {
            // Convert hex to bytes
            std::string encrypted;
            for (size_t i = 0; i < encryptedHex.length(); i += 2) {
                std::string byteStr = encryptedHex.substr(i, 2);
                encrypted += (char)std::stoi(byteStr, nullptr, 16);
            }
            
            // Try to decrypt with recovery key
            std::string decrypted = encrypted;
            for (size_t i = 0; i < decrypted.length(); ++i) {
                decrypted[i] ^= recoveryKey[i % recoveryKey.length()];
            }
            
            return decrypted;
            
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
            profile.folderCount = 0; // Will be updated when folder system is implemented
            
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

std::string ProfileManager::getLastError() const {
    return pimpl->getLastError();
}

} // namespace phantomvault