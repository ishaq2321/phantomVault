/**
 * PhantomVault Profile Manager
 * 
 * Manages user profiles with separate master keys and recovery keys.
 * Handles profile creation (admin-only), authentication, and management.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>

namespace phantomvault {

/**
 * Profile data structure
 */
struct Profile {
    std::string id;
    std::string name;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastAccess;
    bool isActive = false;
    size_t folderCount = 0;
};

/**
 * Profile operation result structure
 */
struct ProfileResult {
    bool success = false;
    std::string profileId;
    std::string recoveryKey;
    std::string new_recovery_key;
    std::string message;
    std::string error;
    std::string error_message;
};

/**
 * Authentication result
 */
struct AuthResult {
    bool success = false;
    std::string profileId;
    std::string message;
    std::string error;
};

/**
 * Profile Manager class
 * 
 * Handles all profile-related operations with security as the top priority.
 * Enforces admin-only profile creation and secure authentication.
 */
class ProfileManager {
public:
    ProfileManager();
    ~ProfileManager();

    // Initialization
    bool initialize(const std::string& dataPath = "");

    // Profile lifecycle (admin-only for creation)
    ProfileResult createProfile(const std::string& name, const std::string& masterKey);
    std::vector<Profile> getAllProfiles();
    std::optional<Profile> getProfile(const std::string& profileId);
    bool deleteProfile(const std::string& profileId, const std::string& masterKey);

    // Authentication
    AuthResult authenticateProfile(const std::string& profileId, const std::string& masterKey);
    bool verifyMasterKey(const std::string& profileId, const std::string& masterKey);
    
    // Password management
    ProfileResult changeProfilePassword(const std::string& profileId, 
                                      const std::string& oldKey, 
                                      const std::string& newKey);

    // Recovery operations
    std::string recoverMasterKey(const std::string& recoveryKey);
    std::optional<std::string> getProfileIdFromRecoveryKey(const std::string& recoveryKey);
    std::optional<std::string> recoverMasterKeyFromRecoveryKey(const std::string& recoveryKey);
    
    // Enhanced recovery operations
    std::string generateRecoveryKey(const std::string& profileId);
    std::string getCurrentRecoveryKey(const std::string& profileId);

    // Session management
    void setActiveProfile(const std::string& profileId);
    std::optional<Profile> getActiveProfile();
    void clearActiveProfile();

    // Vault operations
    size_t getProfileVaultSize(const std::string& profileId) const;
    bool validateProfileVault(const std::string& profileId) const;
    bool performProfileVaultMaintenance(const std::string& profileId);
    std::vector<std::string> getProfileLockedFolders(const std::string& profileId) const;

    // Admin operations
    bool isRunningAsAdmin();
    bool requiresAdminForProfileCreation();

    // Error handling
    std::string getLastError() const;
    
    // Pre-computed lookup system for ultra-fast access
    void enableMemoryMappedLookup();
    void disableMemoryMappedLookup();
    bool isMemoryMappedLookupEnabled() const;
    void preloadProfileCache();
    void warmupLookupTables();
    
    // O(1) profile access optimizations
    std::optional<Profile> getProfileFast(const std::string& profileId) const;
    bool isProfileCached(const std::string& profileId) const;
    void invalidateProfileCache(const std::string& profileId);
    size_t getCacheHitRate() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

} // namespace phantomvault