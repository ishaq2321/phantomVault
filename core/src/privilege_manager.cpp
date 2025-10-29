/**
 * PhantomVault Privilege Manager Implementation
 * 
 * Platform-specific privilege checking, elevation, and validation.
 */

#include "privilege_manager.hpp"
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <sstream>

#ifdef PLATFORM_LINUX
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <cstdlib>
#elif PLATFORM_WINDOWS
#include <windows.h>
#include <lmcons.h>
#include <shlobj.h>
#include <sddl.h>
#elif PLATFORM_MACOS
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <Security/Security.h>
#endif

namespace phantomvault {

class PrivilegeManager::Implementation {
public:
    Implementation()
        : initialized_(false)
        , current_level_(PrivilegeLevel::USER)
        , elevation_timeout_(std::chrono::minutes(30))
        , auto_drop_elevation_(true)
        , require_elevation_for_vault_(true)
        , monitoring_active_(false)
        , last_error_()
        , privilege_loss_callback_()
        , elevation_callback_()
        , monitoring_thread_()
        , monitoring_running_(false)
    {}
    
    ~Implementation() {
        stopPrivilegeMonitoring();
    }
    
    bool initialize() {
        try {
            std::cout << "[PrivilegeManager] Initializing privilege management..." << std::endl;
            
            // Detect current privilege level
            current_level_ = detectCurrentPrivilegeLevel();
            
            std::cout << "[PrivilegeManager] Current privilege level: " << privilegeLevelToString(current_level_) << std::endl;
            std::cout << "[PrivilegeManager] Platform: " << getPlatformInfo() << std::endl;
            std::cout << "[PrivilegeManager] Current user: " << getCurrentUser() << std::endl;
            std::cout << "[PrivilegeManager] Running as service: " << (isRunningAsService() ? "Yes" : "No") << std::endl;
            
            initialized_ = true;
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to initialize PrivilegeManager: " + std::string(e.what());
            return false;
        }
    }
    
    PrivilegeCheckResult checkCurrentPrivileges() const {
        PrivilegeCheckResult result;
        result.currentLevel = current_level_;
        result.hasPrivilege = (current_level_ >= PrivilegeLevel::ADMIN);
        
        if (!result.hasPrivilege) {
            result.errorMessage = "Administrator privileges required";
            result.requiredPermissions = getRequiredPermissions();
        }
        
        return result;
    }
    
    bool hasAdminPrivileges() const {
        return current_level_ >= PrivilegeLevel::ADMIN;
    }
    
    bool hasPrivilegeForOperation(PrivilegedOperation operation) const {
        switch (operation) {
            case PrivilegedOperation::VAULT_ACCESS:
                return require_elevation_for_vault_ ? hasAdminPrivileges() : true;
            case PrivilegedOperation::FOLDER_HIDING:
                return hasAdminPrivileges();
            case PrivilegedOperation::PROFILE_CREATION:
                return hasAdminPrivileges();
            case PrivilegedOperation::SERVICE_MANAGEMENT:
                return hasAdminPrivileges();
            case PrivilegedOperation::SYSTEM_MONITORING:
                return hasAdminPrivileges();
            case PrivilegedOperation::FILE_ENCRYPTION:
                return true; // File encryption doesn't require admin
            case PrivilegedOperation::RECOVERY_OPERATIONS:
                return hasAdminPrivileges();
            default:
                return false;
        }
    }
    
    bool validateStartupPrivileges() const {
        // Check if we have the minimum required privileges for startup
        if (!hasAdminPrivileges()) {
            return false;
        }
        
        // Check platform-specific requirements
        auto missing = getMissingPermissions();
        return missing.empty();
    }
    
    std::string getStartupPrivilegeError() const {
        if (hasAdminPrivileges()) {
            auto missing = getMissingPermissions();
            if (!missing.empty()) {
                std::stringstream ss;
                ss << "Missing required permissions: ";
                for (size_t i = 0; i < missing.size(); ++i) {
                    if (i > 0) ss << ", ";
                    ss << missing[i];
                }
                return ss.str();
            }
            return "";
        }
        
        #ifdef PLATFORM_LINUX
        return "PhantomVault requires root privileges for secure folder operations.\n"
               "Please run with: sudo ./phantomvault";
        #elif PLATFORM_WINDOWS
        return "PhantomVault requires Administrator privileges for secure folder operations.\n"
               "Please run as Administrator or use 'Run as administrator' option.";
        #elif PLATFORM_MACOS
        return "PhantomVault requires Administrator privileges for secure folder operations.\n"
               "Please run with: sudo ./phantomvault";
        #else
        return "Administrator privileges required for secure operations.";
        #endif
    }
    
    ElevationResult requestElevation(const std::string& reason) {
        ElevationResult result;
        
        try {
            if (hasAdminPrivileges()) {
                result.success = true;
                result.achievedLevel = current_level_;
                result.message = "Already have required privileges";
                return result;
            }
            
            std::cout << "[PrivilegeManager] Requesting privilege elevation..." << std::endl;
            if (!reason.empty()) {
                std::cout << "[PrivilegeManager] Reason: " << reason << std::endl;
            }
            
            #ifdef PLATFORM_LINUX
            result = requestLinuxElevation(reason);
            #elif PLATFORM_WINDOWS
            result = requestWindowsElevation(reason);
            #elif PLATFORM_MACOS
            result = requestMacOSElevation(reason);
            #else
            result.errorDetails = "Privilege elevation not supported on this platform";
            #endif
            
            if (result.success) {
                current_level_ = result.achievedLevel;
                result.expiryTime = std::chrono::system_clock::now() + elevation_timeout_;
                
                if (elevation_callback_) {
                    elevation_callback_(current_level_);
                }
                
                std::cout << "[PrivilegeManager] Privilege elevation successful" << std::endl;
            } else {
                std::cout << "[PrivilegeManager] Privilege elevation failed: " << result.errorDetails << std::endl;
            }
            
        } catch (const std::exception& e) {
            result.errorDetails = "Elevation request failed: " + std::string(e.what());
        }
        
        return result;
    }
    
    ElevationResult requestElevationForOperation(PrivilegedOperation operation) {
        std::string reason = getOperationDescription(operation);
        return requestElevation(reason);
    }
    
    bool validateVaultAccess() const {
        return hasPrivilegeForOperation(PrivilegedOperation::VAULT_ACCESS);
    }
    
    bool validateFolderHiding() const {
        return hasPrivilegeForOperation(PrivilegedOperation::FOLDER_HIDING);
    }
    
    bool validateProfileCreation() const {
        return hasPrivilegeForOperation(PrivilegedOperation::PROFILE_CREATION);
    }
    
    void startPrivilegeMonitoring() {
        if (monitoring_active_) {
            return;
        }
        
        monitoring_running_ = true;
        monitoring_thread_ = std::thread(&Implementation::privilegeMonitoringLoop, this);
        monitoring_active_ = true;
        
        std::cout << "[PrivilegeManager] Started privilege monitoring" << std::endl;
    }
    
    void stopPrivilegeMonitoring() {
        if (!monitoring_active_) {
            return;
        }
        
        monitoring_running_ = false;
        
        if (monitoring_thread_.joinable()) {
            monitoring_thread_.join();
        }
        
        monitoring_active_ = false;
        std::cout << "[PrivilegeManager] Stopped privilege monitoring" << std::endl;
    }
    
    bool isPrivilegeMonitoringActive() const {
        return monitoring_active_;
    }
    
    bool attemptPrivilegeRecovery() {
        try {
            PrivilegeLevel detected_level = detectCurrentPrivilegeLevel();
            if (detected_level != current_level_) {
                current_level_ = detected_level;
                return true;
            }
            return false;
        } catch (const std::exception& e) {
            last_error_ = "Privilege recovery failed: " + std::string(e.what());
            return false;
        }
    }
    
    std::vector<std::string> getRequiredPermissions() const {
        std::vector<std::string> permissions;
        
        #ifdef PLATFORM_LINUX
        permissions.push_back("Root access (sudo)");
        permissions.push_back("File system access");
        permissions.push_back("Process management");
        #elif PLATFORM_WINDOWS
        permissions.push_back("Administrator privileges");
        permissions.push_back("File system access");
        permissions.push_back("Registry access");
        permissions.push_back("Service management");
        #elif PLATFORM_MACOS
        permissions.push_back("Administrator privileges");
        permissions.push_back("File system access");
        permissions.push_back("Keychain access");
        #endif
        
        return permissions;
    }
    
    std::vector<std::string> getMissingPermissions() const {
        std::vector<std::string> missing;
        
        if (!hasAdminPrivileges()) {
            #ifdef PLATFORM_LINUX
            missing.push_back("Root privileges");
            #elif PLATFORM_WINDOWS
            missing.push_back("Administrator privileges");
            #elif PLATFORM_MACOS
            missing.push_back("Administrator privileges");
            #endif
        }
        
        // Add platform-specific permission checks here
        
        return missing;
    }
    
    void handlePrivilegeLoss() {
        std::cout << "[PrivilegeManager] Privilege loss detected" << std::endl;
        
        PrivilegeLevel old_level = current_level_;
        current_level_ = detectCurrentPrivilegeLevel();
        
        if (privilege_loss_callback_) {
            privilege_loss_callback_(current_level_);
        }
        
        std::cout << "[PrivilegeManager] Privilege level changed from " 
                  << privilegeLevelToString(old_level) << " to " 
                  << privilegeLevelToString(current_level_) << std::endl;
    }
    
    std::string getPrivilegeErrorMessage(PrivilegedOperation operation) const {
        switch (operation) {
            case PrivilegedOperation::VAULT_ACCESS:
                return "Administrator privileges required for vault operations";
            case PrivilegedOperation::FOLDER_HIDING:
                return "Administrator privileges required for folder hiding operations";
            case PrivilegedOperation::PROFILE_CREATION:
                return "Administrator privileges required for profile creation";
            case PrivilegedOperation::SERVICE_MANAGEMENT:
                return "Administrator privileges required for service management";
            case PrivilegedOperation::SYSTEM_MONITORING:
                return "Administrator privileges required for system monitoring";
            case PrivilegedOperation::RECOVERY_OPERATIONS:
                return "Administrator privileges required for recovery operations";
            default:
                return "Administrator privileges required for this operation";
        }
    }
    
    std::string getPlatformInfo() const {
        #ifdef PLATFORM_LINUX
        return "Linux";
        #elif PLATFORM_WINDOWS
        return "Windows";
        #elif PLATFORM_MACOS
        return "macOS";
        #else
        return "Unknown";
        #endif
    }
    
    std::string getCurrentUser() const {
        #ifdef PLATFORM_LINUX
        uid_t uid = getuid();
        struct passwd* pw = getpwuid(uid);
        return pw ? pw->pw_name : "unknown";
        #elif PLATFORM_WINDOWS
        char username[UNLEN + 1];
        DWORD username_len = UNLEN + 1;
        if (GetUserNameA(username, &username_len)) {
            return std::string(username);
        }
        return "unknown";
        #elif PLATFORM_MACOS
        uid_t uid = getuid();
        struct passwd* pw = getpwuid(uid);
        return pw ? pw->pw_name : "unknown";
        #else
        return "unknown";
        #endif
    }
    
    bool isRunningAsService() const {
        #ifdef PLATFORM_LINUX
        // Check if we're running as a daemon (no controlling terminal)
        return !isatty(STDIN_FILENO);
        #elif PLATFORM_WINDOWS
        // Check if we're running as a Windows service
        return GetConsoleWindow() == NULL;
        #elif PLATFORM_MACOS
        // Check if we're running as a daemon
        return !isatty(STDIN_FILENO);
        #else
        return false;
        #endif
    }
    
    std::string getLastError() const {
        return last_error_;
    }
    
    void setPrivilegeLossCallback(std::function<void(PrivilegeLevel)> callback) {
        privilege_loss_callback_ = callback;
    }
    
    void setElevationCallback(std::function<void(PrivilegeLevel)> callback) {
        elevation_callback_ = callback;
    }
    
    void setElevationTimeout(std::chrono::minutes timeout) {
        elevation_timeout_ = timeout;
    }
    
    void setAutoDropElevation(bool enabled) {
        auto_drop_elevation_ = enabled;
    }
    
    void setRequireElevationForVault(bool required) {
        require_elevation_for_vault_ = required;
    }

private:
    bool initialized_;
    PrivilegeLevel current_level_;
    std::chrono::minutes elevation_timeout_;
    bool auto_drop_elevation_;
    bool require_elevation_for_vault_;
    bool monitoring_active_;
    mutable std::string last_error_;
    
    std::function<void(PrivilegeLevel)> privilege_loss_callback_;
    std::function<void(PrivilegeLevel)> elevation_callback_;
    
    std::thread monitoring_thread_;
    std::atomic<bool> monitoring_running_;
    
    PrivilegeLevel detectCurrentPrivilegeLevel() const {
        #ifdef PLATFORM_LINUX
        if (getuid() == 0) {
            return PrivilegeLevel::ADMIN;
        }
        
        // Check if user is in sudo group
        gid_t groups[32];
        int ngroups = 32;
        if (getgroups(ngroups, groups) != -1) {
            struct group* sudo_group = getgrnam("sudo");
            if (sudo_group) {
                for (int i = 0; i < ngroups; ++i) {
                    if (groups[i] == sudo_group->gr_gid) {
                        return PrivilegeLevel::ELEVATED;
                    }
                }
            }
        }
        
        return PrivilegeLevel::USER;
        
        #elif PLATFORM_WINDOWS
        BOOL isAdmin = FALSE;
        PSID adminGroup = NULL;
        SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
        
        if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                   DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
            CheckTokenMembership(NULL, adminGroup, &isAdmin);
            FreeSid(adminGroup);
        }
        
        return isAdmin ? PrivilegeLevel::ADMIN : PrivilegeLevel::USER;
        
        #elif PLATFORM_MACOS
        if (getuid() == 0) {
            return PrivilegeLevel::ADMIN;
        }
        
        // Check if user is in admin group
        gid_t groups[32];
        int ngroups = 32;
        if (getgroups(ngroups, groups) != -1) {
            struct group* admin_group = getgrnam("admin");
            if (admin_group) {
                for (int i = 0; i < ngroups; ++i) {
                    if (groups[i] == admin_group->gr_gid) {
                        return PrivilegeLevel::ELEVATED;
                    }
                }
            }
        }
        
        return PrivilegeLevel::USER;
        
        #else
        return PrivilegeLevel::USER;
        #endif
    }
    
    std::string privilegeLevelToString(PrivilegeLevel level) const {
        switch (level) {
            case PrivilegeLevel::USER: return "User";
            case PrivilegeLevel::ADMIN: return "Administrator";
            case PrivilegeLevel::ELEVATED: return "Elevated";
            case PrivilegeLevel::SYSTEM: return "System";
            default: return "Unknown";
        }
    }
    
    std::string getOperationDescription(PrivilegedOperation operation) const {
        switch (operation) {
            case PrivilegedOperation::VAULT_ACCESS:
                return "Access encrypted vault operations";
            case PrivilegedOperation::FOLDER_HIDING:
                return "Hide and unhide folders securely";
            case PrivilegedOperation::PROFILE_CREATION:
                return "Create new user profiles";
            case PrivilegedOperation::SERVICE_MANAGEMENT:
                return "Manage PhantomVault services";
            case PrivilegedOperation::SYSTEM_MONITORING:
                return "Monitor system events and security";
            case PrivilegedOperation::FILE_ENCRYPTION:
                return "Encrypt and decrypt files";
            case PrivilegedOperation::RECOVERY_OPERATIONS:
                return "Perform recovery and maintenance operations";
            default:
                return "Perform privileged operation";
        }
    }
    
    void privilegeMonitoringLoop() {
        std::cout << "[PrivilegeManager] Privilege monitoring loop started" << std::endl;
        
        while (monitoring_running_) {
            try {
                // Check current privilege level
                PrivilegeLevel detected_level = detectCurrentPrivilegeLevel();
                
                if (detected_level != current_level_) {
                    handlePrivilegeLoss();
                }
                
                // Sleep for monitoring interval
                std::this_thread::sleep_for(std::chrono::seconds(30));
                
            } catch (const std::exception& e) {
                last_error_ = "Privilege monitoring error: " + std::string(e.what());
                std::this_thread::sleep_for(std::chrono::seconds(10));
            }
        }
        
        std::cout << "[PrivilegeManager] Privilege monitoring loop stopped" << std::endl;
    }
    
    #ifdef PLATFORM_LINUX
    bool tryGUIElevation(const std::string& reason, ElevationResult& result);
    
    ElevationResult requestLinuxElevation(const std::string& reason) {
        ElevationResult result;
        
        // Try GUI elevation methods first (pkexec, gksu, etc.)
        if (tryGUIElevation(reason, result)) {
            return result;
        }
        
        // Fallback: request restart with sudo
        result.errorDetails = "Please restart PhantomVault with sudo privileges:\nsudo ./phantomvault";
        result.message = "Restart required with elevated privileges";
        
        return result;
    }
    
    bool tryGUIElevation(const std::string& reason, ElevationResult& result) {
        // Try pkexec first (PolicyKit - most modern)
        if (system("which pkexec > /dev/null 2>&1") == 0) {
            std::cout << "[PrivilegeManager] Attempting pkexec elevation..." << std::endl;
            
            std::string pkexec_cmd = "pkexec --user root ";
            if (!reason.empty()) {
                pkexec_cmd += "--disable-internal-agent ";
            }
            pkexec_cmd += "echo 'Elevation successful' > /dev/null 2>&1";
            
            if (system(pkexec_cmd.c_str()) == 0) {
                result.success = true;
                result.achievedLevel = PrivilegeLevel::ADMIN;
                result.message = "Elevation granted via pkexec";
                return true;
            }
        }
        
        // Try gksu (older GTK-based)
        if (system("which gksu > /dev/null 2>&1") == 0) {
            std::cout << "[PrivilegeManager] Attempting gksu elevation..." << std::endl;
            
            std::string gksu_cmd = "gksu ";
            if (!reason.empty()) {
                gksu_cmd += "-m '" + reason + "' ";
            }
            gksu_cmd += "echo 'Elevation successful' > /dev/null 2>&1";
            
            if (system(gksu_cmd.c_str()) == 0) {
                result.success = true;
                result.achievedLevel = PrivilegeLevel::ADMIN;
                result.message = "Elevation granted via gksu";
                return true;
            }
        }
        
        // Try zenity with sudo (password prompt)
        if (system("which zenity > /dev/null 2>&1") == 0 && system("which sudo > /dev/null 2>&1") == 0) {
            std::cout << "[PrivilegeManager] Attempting zenity+sudo elevation..." << std::endl;
            
            std::string zenity_cmd = "zenity --password --title='PhantomVault Authentication'";
            if (!reason.empty()) {
                zenity_cmd += " --text='" + reason + "'";
            }
            zenity_cmd += " | sudo -S echo 'Elevation successful' > /dev/null 2>&1";
            
            if (system(zenity_cmd.c_str()) == 0) {
                result.success = true;
                result.achievedLevel = PrivilegeLevel::ADMIN;
                result.message = "Elevation granted via zenity+sudo";
                return true;
            }
        }
        
        return false;
    }
    #endif
    
    #ifdef PLATFORM_WINDOWS
    ElevationResult requestWindowsElevation(const std::string& reason) {
        ElevationResult result;
        
        // On Windows, we can request UAC elevation
        SHELLEXECUTEINFOA sei = { sizeof(sei) };
        sei.lpVerb = "runas";
        sei.lpFile = "phantomvault.exe";
        sei.hwnd = NULL;
        sei.nShow = SW_NORMAL;
        
        if (ShellExecuteExA(&sei)) {
            result.success = true;
            result.achievedLevel = PrivilegeLevel::ADMIN;
            result.message = "Elevation request sent to UAC";
        } else {
            result.errorDetails = "Failed to request UAC elevation";
        }
        
        return result;
    }
    #endif
    
    #ifdef PLATFORM_MACOS
    ElevationResult requestMacOSElevation(const std::string& reason) {
        ElevationResult result;
        
        // On macOS, we can't elevate privileges from within the process
        // The user needs to restart with sudo
        result.errorDetails = "Please restart PhantomVault with sudo privileges:\nsudo ./phantomvault";
        result.message = "Restart required with elevated privileges";
        
        return result;
    }
    #endif
};

// PrivilegeManager public interface implementation
PrivilegeManager::PrivilegeManager() : pimpl(std::make_unique<Implementation>()) {}
PrivilegeManager::~PrivilegeManager() = default;

bool PrivilegeManager::initialize() {
    return pimpl->initialize();
}

PrivilegeCheckResult PrivilegeManager::checkCurrentPrivileges() const {
    return pimpl->checkCurrentPrivileges();
}

bool PrivilegeManager::hasAdminPrivileges() const {
    return pimpl->hasAdminPrivileges();
}

bool PrivilegeManager::hasPrivilegeForOperation(PrivilegedOperation operation) const {
    return pimpl->hasPrivilegeForOperation(operation);
}

PrivilegeLevel PrivilegeManager::getCurrentPrivilegeLevel() const {
    return pimpl->checkCurrentPrivileges().currentLevel;
}

bool PrivilegeManager::validateStartupPrivileges() const {
    return pimpl->validateStartupPrivileges();
}

std::string PrivilegeManager::getStartupPrivilegeError() const {
    return pimpl->getStartupPrivilegeError();
}

bool PrivilegeManager::requiresElevationForStartup() const {
    return !validateStartupPrivileges();
}

ElevationResult PrivilegeManager::requestElevation(const std::string& reason) {
    return pimpl->requestElevation(reason);
}

ElevationResult PrivilegeManager::requestElevationForOperation(PrivilegedOperation operation) {
    return pimpl->requestElevationForOperation(operation);
}

bool PrivilegeManager::validateVaultAccess() const {
    return pimpl->validateVaultAccess();
}

bool PrivilegeManager::validateFolderHiding() const {
    return pimpl->validateFolderHiding();
}

bool PrivilegeManager::validateProfileCreation() const {
    return pimpl->validateProfileCreation();
}

void PrivilegeManager::startPrivilegeMonitoring() {
    pimpl->startPrivilegeMonitoring();
}

void PrivilegeManager::stopPrivilegeMonitoring() {
    pimpl->stopPrivilegeMonitoring();
}

bool PrivilegeManager::isPrivilegeMonitoringActive() const {
    return pimpl->isPrivilegeMonitoringActive();
}

bool PrivilegeManager::validateServiceManagement() const {
    return pimpl->hasPrivilegeForOperation(PrivilegedOperation::SERVICE_MANAGEMENT);
}

bool PrivilegeManager::dropElevatedPrivileges() {
    // On most platforms, we can't actually drop privileges once elevated
    // This is more of a logical state management
    return true;
}

bool PrivilegeManager::canRequestElevation() const {
    #ifdef PLATFORM_WINDOWS
    return true; // Windows supports UAC elevation
    #else
    return false; // Unix-like systems require restart with sudo
    #endif
}

bool PrivilegeManager::attemptPrivilegeRecovery() {
    return pimpl->attemptPrivilegeRecovery();
}

void PrivilegeManager::setPrivilegeLossCallback(std::function<void(PrivilegeLevel)> callback) {
    pimpl->setPrivilegeLossCallback(callback);
}

void PrivilegeManager::setElevationCallback(std::function<void(PrivilegeLevel)> callback) {
    pimpl->setElevationCallback(callback);
}

void PrivilegeManager::setElevationTimeout(std::chrono::minutes timeout) {
    pimpl->setElevationTimeout(timeout);
}

void PrivilegeManager::setAutoDropElevation(bool enabled) {
    pimpl->setAutoDropElevation(enabled);
}

void PrivilegeManager::setRequireElevationForVault(bool required) {
    pimpl->setRequireElevationForVault(required);
}

std::vector<std::string> PrivilegeManager::getRequiredPermissions() const {
    return pimpl->getRequiredPermissions();
}

std::vector<std::string> PrivilegeManager::getMissingPermissions() const {
    return pimpl->getMissingPermissions();
}

void PrivilegeManager::handlePrivilegeLoss() {
    pimpl->handlePrivilegeLoss();
}

std::string PrivilegeManager::getPrivilegeErrorMessage(PrivilegedOperation operation) const {
    return pimpl->getPrivilegeErrorMessage(operation);
}

std::string PrivilegeManager::getPlatformInfo() const {
    return pimpl->getPlatformInfo();
}

std::string PrivilegeManager::getCurrentUser() const {
    return pimpl->getCurrentUser();
}

bool PrivilegeManager::isRunningAsService() const {
    return pimpl->isRunningAsService();
}

std::string PrivilegeManager::getLastError() const {
    return pimpl->getLastError();
}

// PrivilegeElevationGuard implementation
PrivilegeElevationGuard::PrivilegeElevationGuard(PrivilegeManager* manager, PrivilegedOperation operation)
    : manager_(manager)
    , was_elevated_(false)
    , original_level_(PrivilegeLevel::USER) {
    
    if (manager_) {
        original_level_ = manager_->getCurrentPrivilegeLevel();
        
        if (!manager_->hasPrivilegeForOperation(operation)) {
            auto result = manager_->requestElevationForOperation(operation);
            was_elevated_ = result.success;
            
            if (!was_elevated_) {
                error_message_ = result.errorDetails;
            }
        } else {
            was_elevated_ = true; // Already have required privileges
        }
    }
}

PrivilegeElevationGuard::~PrivilegeElevationGuard() {
    if (was_elevated_ && manager_) {
        // Note: On most platforms, we can't actually drop privileges
        // This is more of a logical state management
    }
}

bool PrivilegeElevationGuard::isElevated() const {
    return was_elevated_;
}

PrivilegeLevel PrivilegeElevationGuard::getLevel() const {
    return manager_ ? manager_->getCurrentPrivilegeLevel() : PrivilegeLevel::USER;
}

std::string PrivilegeElevationGuard::getErrorMessage() const {
    return error_message_;
}

} // namespace phantomvault