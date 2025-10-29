#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

namespace phantomvault {

/**
 * @brief Privilege levels for different operations
 */
enum class PrivilegeLevel {
    USER,           // Normal user privileges
    ADMIN,          // Administrator privileges
    ELEVATED,       // Temporarily elevated privileges
    SYSTEM          // System-level privileges (if available)
};

/**
 * @brief Privilege check result
 */
struct PrivilegeCheckResult {
    bool hasPrivilege;
    PrivilegeLevel currentLevel;
    std::string errorMessage;
    std::vector<std::string> requiredPermissions;
    
    PrivilegeCheckResult() : hasPrivilege(false), currentLevel(PrivilegeLevel::USER) {}
};

/**
 * @brief Privilege elevation result
 */
struct ElevationResult {
    bool success;
    PrivilegeLevel achievedLevel;
    std::string message;
    std::string errorDetails;
    std::chrono::system_clock::time_point expiryTime;
    
    ElevationResult() : success(false), achievedLevel(PrivilegeLevel::USER) {}
};

/**
 * @brief Operations that require elevated privileges
 */
enum class PrivilegedOperation {
    VAULT_ACCESS,           // Access to vault operations
    FOLDER_HIDING,          // Hide/unhide folders
    PROFILE_CREATION,       // Create new profiles
    SERVICE_MANAGEMENT,     // Start/stop services
    SYSTEM_MONITORING,      // Monitor system events
    FILE_ENCRYPTION,        // Encrypt/decrypt files
    RECOVERY_OPERATIONS     // Recovery and maintenance
};

/**
 * @brief Authentication state for dual-layer access control
 */
struct AuthenticationState {
    bool hasAdminPrivileges;
    bool hasProfileAuthentication;
    std::string authenticatedProfileId;
    std::chrono::system_clock::time_point adminElevationTime;
    std::chrono::system_clock::time_point profileAuthTime;
    std::chrono::system_clock::time_point sessionExpiry;
    bool isTamperResistant;
    
    AuthenticationState() 
        : hasAdminPrivileges(false)
        , hasProfileAuthentication(false)
        , isTamperResistant(false) {}
        
    bool isFullyAuthenticated() const {
        return hasAdminPrivileges && hasProfileAuthentication && !isExpired();
    }
    
    bool isExpired() const {
        auto now = std::chrono::system_clock::now();
        return now > sessionExpiry;
    }
};

/**
 * @brief Dual-layer authentication result
 */
struct DualAuthResult {
    bool success;
    bool adminLayerPassed;
    bool profileLayerPassed;
    std::string profileId;
    std::string message;
    std::string errorDetails;
    std::chrono::system_clock::time_point sessionExpiry;
    
    DualAuthResult() : success(false), adminLayerPassed(false), profileLayerPassed(false) {}
};

/**
 * @brief Privilege Manager for PhantomVault
 * 
 * Manages privilege checking, elevation requests, and privilege validation
 * for secure operations. Provides platform-specific privilege management.
 */
class PrivilegeManager {
public:
    PrivilegeManager();
    ~PrivilegeManager();
    
    // Initialization
    bool initialize();
    
    // Privilege checking
    PrivilegeCheckResult checkCurrentPrivileges() const;
    bool hasAdminPrivileges() const;
    bool hasPrivilegeForOperation(PrivilegedOperation operation) const;
    PrivilegeLevel getCurrentPrivilegeLevel() const;
    
    // Startup validation
    bool validateStartupPrivileges() const;
    std::string getStartupPrivilegeError() const;
    bool requiresElevationForStartup() const;
    
    // Privilege elevation
    ElevationResult requestElevation(const std::string& reason = "");
    ElevationResult requestElevationForOperation(PrivilegedOperation operation);
    bool dropElevatedPrivileges();
    
    // Dual-layer access control
    DualAuthResult requestDualLayerAuthentication(const std::string& profileId, 
                                                 const std::string& masterKey,
                                                 PrivilegedOperation operation);
    bool validateDualLayerAccess(PrivilegedOperation operation) const;
    AuthenticationState getCurrentAuthenticationState() const;
    bool refreshAuthenticationSession(const std::string& profileId, const std::string& masterKey);
    void clearAuthenticationSession();
    
    // Tamper-resistant authentication state
    bool validateAuthenticationIntegrity() const;
    void enableTamperResistance();
    void disableTamperResistance();
    
    // Operation-specific validation
    bool validateVaultAccess() const;
    bool validateFolderHiding() const;
    bool validateProfileCreation() const;
    bool validateServiceManagement() const;
    
    // Privilege monitoring
    void startPrivilegeMonitoring();
    void stopPrivilegeMonitoring();
    bool isPrivilegeMonitoringActive() const;
    
    // Platform-specific operations
    std::vector<std::string> getRequiredPermissions() const;
    std::vector<std::string> getMissingPermissions() const;
    bool canRequestElevation() const;
    
    // Error handling and recovery
    void handlePrivilegeLoss();
    bool attemptPrivilegeRecovery();
    std::string getPrivilegeErrorMessage(PrivilegedOperation operation) const;
    
    // Callbacks
    void setPrivilegeLossCallback(std::function<void(PrivilegeLevel)> callback);
    void setElevationCallback(std::function<void(PrivilegeLevel)> callback);
    
    // Configuration
    void setElevationTimeout(std::chrono::minutes timeout);
    void setAutoDropElevation(bool enabled);
    void setRequireElevationForVault(bool required);
    void setSessionTimeout(std::chrono::minutes timeout);
    void setRequireDualLayerAuth(bool required);
    void setProfileManager(class ProfileManager* profileManager);
    
    // Information
    std::string getPlatformInfo() const;
    std::string getCurrentUser() const;
    bool isRunningAsService() const;
    
    // Error handling
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

/**
 * @brief RAII class for temporary privilege elevation
 */
class PrivilegeElevationGuard {
public:
    PrivilegeElevationGuard(PrivilegeManager* manager, PrivilegedOperation operation);
    ~PrivilegeElevationGuard();
    
    bool isElevated() const;
    PrivilegeLevel getLevel() const;
    std::string getErrorMessage() const;
    
private:
    PrivilegeManager* manager_;
    bool was_elevated_;
    PrivilegeLevel original_level_;
    std::string error_message_;
};

/**
 * @brief RAII class for dual-layer authentication
 */
class DualLayerAuthGuard {
public:
    DualLayerAuthGuard(PrivilegeManager* manager, const std::string& profileId, 
                      const std::string& masterKey, PrivilegedOperation operation);
    ~DualLayerAuthGuard();
    
    bool isAuthenticated() const;
    bool hasAdminLayer() const;
    bool hasProfileLayer() const;
    std::string getProfileId() const;
    std::string getErrorMessage() const;
    std::chrono::system_clock::time_point getSessionExpiry() const;
    
private:
    PrivilegeManager* manager_;
    bool was_authenticated_;
    bool admin_layer_passed_;
    bool profile_layer_passed_;
    std::string profile_id_;
    std::string error_message_;
    std::chrono::system_clock::time_point session_expiry_;
};

} // namespace phantomvault