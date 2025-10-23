#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <chrono>
#include <functional>

namespace phantom_vault::users {

/**
 * @brief User roles and permissions
 */
enum class PHANTOM_VAULT_EXPORT UserRole {
    Owner,          // Full access to all vaults
    Admin,          // Administrative access
    User,           // Standard user access
    Guest,          // Limited access
    Viewer          // Read-only access
};

/**
 * @brief User permissions
 */
enum class PHANTOM_VAULT_EXPORT Permission {
    CreateVault,
    DeleteVault,
    ModifyVault,
    ViewVault,
    ShareVault,
    BackupVault,
    RestoreVault,
    ManageUsers,
    ViewLogs,
    ConfigureSettings
};

/**
 * @brief User account information
 */
struct PHANTOM_VAULT_EXPORT UserAccount {
    std::string id;                     // Unique user ID
    std::string username;               // Username
    std::string email;                  // Email address
    std::string displayName;            // Display name
    UserRole role;                      // User role
    std::set<Permission> permissions;   // User permissions
    std::chrono::system_clock::time_point createdTime;
    std::chrono::system_clock::time_point lastLogin;
    bool isActive;                      // Account status
    bool requiresPasswordChange;        // Password change required
    std::string profileImage;           // Profile image path
    std::map<std::string, std::string> preferences; // User preferences
};

/**
 * @brief Vault access control
 */
struct PHANTOM_VAULT_EXPORT VaultAccess {
    std::string vaultId;                // Vault identifier
    std::string userId;                 // User identifier
    std::set<Permission> permissions;   // Vault-specific permissions
    std::chrono::system_clock::time_point grantedTime;
    std::chrono::system_clock::time_point expiresTime;
    bool isActive;                      // Access status
    std::string grantedBy;              // Who granted access
};

/**
 * @brief User session information
 */
struct PHANTOM_VAULT_EXPORT UserSession {
    std::string sessionId;              // Session identifier
    std::string userId;                 // User identifier
    std::string deviceId;               // Device identifier
    std::string ipAddress;              // IP address
    std::chrono::system_clock::time_point loginTime;
    std::chrono::system_clock::time_point lastActivity;
    std::chrono::minutes timeout;       // Session timeout
    bool isActive;                      // Session status
};

/**
 * @brief User authentication result
 */
struct PHANTOM_VAULT_EXPORT AuthResult {
    bool success;                       // Authentication success
    std::string sessionId;              // Session ID if successful
    std::string errorMessage;           // Error message if failed
    UserRole userRole;                  // User role
    std::set<Permission> permissions;   // User permissions
    bool requiresPasswordChange;        // Password change required
    std::chrono::minutes sessionTimeout; // Session timeout
};

/**
 * @brief User management interface
 */
class PHANTOM_VAULT_EXPORT UserManager {
public:
    virtual ~UserManager() = default;
    
    // User account management
    virtual bool createUser(const UserAccount& user, const std::string& password) = 0;
    virtual bool updateUser(const UserAccount& user) = 0;
    virtual bool deleteUser(const std::string& userId) = 0;
    virtual UserAccount getUser(const std::string& userId) = 0;
    virtual std::vector<UserAccount> getAllUsers() = 0;
    virtual bool activateUser(const std::string& userId) = 0;
    virtual bool deactivateUser(const std::string& userId) = 0;
    
    // Authentication
    virtual AuthResult authenticate(const std::string& username, const std::string& password) = 0;
    virtual bool changePassword(const std::string& userId, const std::string& oldPassword, const std::string& newPassword) = 0;
    virtual bool resetPassword(const std::string& userId, const std::string& newPassword) = 0;
    virtual bool logout(const std::string& sessionId) = 0;
    
    // Session management
    virtual bool isValidSession(const std::string& sessionId) = 0;
    virtual UserSession getSession(const std::string& sessionId) = 0;
    virtual std::vector<UserSession> getUserSessions(const std::string& userId) = 0;
    virtual bool terminateSession(const std::string& sessionId) = 0;
    virtual bool terminateAllUserSessions(const std::string& userId) = 0;
    
    // Permission management
    virtual bool grantPermission(const std::string& userId, Permission permission) = 0;
    virtual bool revokePermission(const std::string& userId, Permission permission) = 0;
    virtual bool hasPermission(const std::string& userId, Permission permission) = 0;
    virtual std::set<Permission> getUserPermissions(const std::string& userId) = 0;
    
    // Vault access control
    virtual bool grantVaultAccess(const std::string& vaultId, const std::string& userId, const std::set<Permission>& permissions) = 0;
    virtual bool revokeVaultAccess(const std::string& vaultId, const std::string& userId) = 0;
    virtual bool hasVaultAccess(const std::string& vaultId, const std::string& userId, Permission permission) = 0;
    virtual std::vector<VaultAccess> getVaultAccessList(const std::string& vaultId) = 0;
    virtual std::vector<VaultAccess> getUserVaultAccess(const std::string& userId) = 0;
    
    // User preferences
    virtual bool setUserPreference(const std::string& userId, const std::string& key, const std::string& value) = 0;
    virtual std::string getUserPreference(const std::string& userId, const std::string& key) = 0;
    virtual std::map<std::string, std::string> getUserPreferences(const std::string& userId) = 0;
    
    // Event callbacks
    virtual void setUserCreatedCallback(std::function<void(const UserAccount&)> callback) = 0;
    virtual void setUserDeletedCallback(std::function<void(const std::string&)> callback) = 0;
    virtual void setUserLoginCallback(std::function<void(const UserAccount&)> callback) = 0;
    virtual void setUserLogoutCallback(std::function<void(const std::string&)> callback) = 0;
    virtual void setPermissionChangedCallback(std::function<void(const std::string&, Permission, bool)> callback) = 0;
};

/**
 * @brief Local user manager implementation
 */
class PHANTOM_VAULT_EXPORT LocalUserManager : public UserManager {
public:
    LocalUserManager();
    ~LocalUserManager() override;
    
    // User account management
    bool createUser(const UserAccount& user, const std::string& password) override;
    bool updateUser(const UserAccount& user) override;
    bool deleteUser(const std::string& userId) override;
    UserAccount getUser(const std::string& userId) override;
    std::vector<UserAccount> getAllUsers() override;
    bool activateUser(const std::string& userId) override;
    bool deactivateUser(const std::string& userId) override;
    
    // Authentication
    AuthResult authenticate(const std::string& username, const std::string& password) override;
    bool changePassword(const std::string& userId, const std::string& oldPassword, const std::string& newPassword) override;
    bool resetPassword(const std::string& userId, const std::string& newPassword) override;
    bool logout(const std::string& sessionId) override;
    
    // Session management
    bool isValidSession(const std::string& sessionId) override;
    UserSession getSession(const std::string& sessionId) override;
    std::vector<UserSession> getUserSessions(const std::string& userId) override;
    bool terminateSession(const std::string& sessionId) override;
    bool terminateAllUserSessions(const std::string& userId) override;
    
    // Permission management
    bool grantPermission(const std::string& userId, Permission permission) override;
    bool revokePermission(const std::string& userId, Permission permission) override;
    bool hasPermission(const std::string& userId, Permission permission) override;
    std::set<Permission> getUserPermissions(const std::string& userId) override;
    
    // Vault access control
    bool grantVaultAccess(const std::string& vaultId, const std::string& userId, const std::set<Permission>& permissions) override;
    bool revokeVaultAccess(const std::string& vaultId, const std::string& userId) override;
    bool hasVaultAccess(const std::string& vaultId, const std::string& userId, Permission permission) override;
    std::vector<VaultAccess> getVaultAccessList(const std::string& vaultId) override;
    std::vector<VaultAccess> getUserVaultAccess(const std::string& userId) override;
    
    // User preferences
    bool setUserPreference(const std::string& userId, const std::string& key, const std::string& value) override;
    std::string getUserPreference(const std::string& userId, const std::string& key) override;
    std::map<std::string, std::string> getUserPreferences(const std::string& userId) override;
    
    // Event callbacks
    void setUserCreatedCallback(std::function<void(const UserAccount&)> callback) override;
    void setUserDeletedCallback(std::function<void(const std::string&)> callback) override;
    void setUserLoginCallback(std::function<void(const UserAccount&)> callback) override;
    void setUserLogoutCallback(std::function<void(const std::string&)> callback) override;
    void setPermissionChangedCallback(std::function<void(const std::string&, Permission, bool)> callback) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief User group management
 */
class PHANTOM_VAULT_EXPORT UserGroupManager {
public:
    virtual ~UserGroupManager() = default;
    
    virtual bool createGroup(const std::string& groupName, const std::string& description) = 0;
    virtual bool deleteGroup(const std::string& groupName) = 0;
    virtual bool addUserToGroup(const std::string& groupName, const std::string& userId) = 0;
    virtual bool removeUserFromGroup(const std::string& groupName, const std::string& userId) = 0;
    virtual std::vector<std::string> getGroupMembers(const std::string& groupName) = 0;
    virtual std::vector<std::string> getUserGroups(const std::string& userId) = 0;
    virtual bool setGroupPermissions(const std::string& groupName, const std::set<Permission>& permissions) = 0;
    virtual std::set<Permission> getGroupPermissions(const std::string& groupName) = 0;
};

/**
 * @brief User activity logging
 */
class PHANTOM_VAULT_EXPORT UserActivityLogger {
public:
    virtual ~UserActivityLogger() = default;
    
    virtual void logUserLogin(const std::string& userId, const std::string& ipAddress) = 0;
    virtual void logUserLogout(const std::string& userId) = 0;
    virtual void logVaultAccess(const std::string& userId, const std::string& vaultId, const std::string& action) = 0;
    virtual void logPermissionChange(const std::string& adminId, const std::string& targetUserId, Permission permission, bool granted) = 0;
    virtual void logUserCreation(const std::string& adminId, const std::string& newUserId) = 0;
    virtual void logUserDeletion(const std::string& adminId, const std::string& deletedUserId) = 0;
    
    virtual std::vector<std::string> getUserActivityLog(const std::string& userId, int limit = 100) = 0;
    virtual std::vector<std::string> getSystemActivityLog(int limit = 100) = 0;
    virtual bool clearUserLogs(const std::string& userId) = 0;
    virtual bool clearSystemLogs() = 0;
};

} // namespace phantom_vault::users
