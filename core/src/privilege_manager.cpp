/**
 * PhantomVault Privilege Manager Implementation
 * 
 * Platform-specific privilege checking, elevation, and validation.
 * Enhanced with dual-layer access control requiring both admin privileges
 * and profile master key authentication.
 */

#include "privilege_manager.hpp"
#include "profile_manager.hpp"
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <sstream>
#include <functional>
#include <filesystem>

#ifdef PLATFORM_LINUX
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/prctl.h>
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
#include <sys/ptrace.h>
#include <Security/Security.h>
#endif

namespace fs = std::filesystem;

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
        , session_timeout_(std::chrono::minutes(15))
        , require_dual_layer_auth_(true)
        , profile_manager_(nullptr)
        , auth_state_()
        , tamper_check_hash_(0)
        , installation_path_()
        , process_protected_(false)
        , installation_protected_(false)
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
        // If dual-layer authentication is required, use that validation
        if (require_dual_layer_auth_) {
            return validateDualLayerAccess(operation);
        }
        
        // Fallback to traditional privilege checking
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
    
    void setSessionTimeout(std::chrono::minutes timeout) {
        session_timeout_ = timeout;
    }
    
    void setRequireDualLayerAuth(bool required) {
        require_dual_layer_auth_ = required;
    }
    
    void setProfileManager(ProfileManager* profileManager) {
        profile_manager_ = profileManager;
    }
    
    DualAuthResult requestDualLayerAuthentication(const std::string& profileId, 
                                                 const std::string& masterKey,
                                                 PrivilegedOperation operation) {
        DualAuthResult result;
        
        try {
            std::lock_guard<std::mutex> lock(auth_state_mutex_);
            
            // Step 1: Validate admin privileges
            if (!hasAdminPrivileges()) {
                auto elevation_result = requestElevation("Dual-layer authentication for " + getOperationDescription(operation));
                if (!elevation_result.success) {
                    result.errorDetails = "Admin privilege elevation failed: " + elevation_result.errorDetails;
                    return result;
                }
                result.adminLayerPassed = true;
            } else {
                result.adminLayerPassed = true;
            }
            
            // Step 2: Validate profile authentication
            if (!profile_manager_) {
                result.errorDetails = "Profile manager not available for authentication";
                return result;
            }
            
            // Authenticate with profile manager
            auto auth_result = profile_manager_->authenticateProfile(profileId, masterKey);
            if (!auth_result.success) {
                result.errorDetails = "Profile authentication failed: " + auth_result.error;
                return result;
            }
            
            result.profileLayerPassed = true;
            result.profileId = profileId;
            
            // Step 3: Establish authenticated session
            auto now = std::chrono::system_clock::now();
            auth_state_.hasAdminPrivileges = true;
            auth_state_.hasProfileAuthentication = true;
            auth_state_.authenticatedProfileId = profileId;
            auth_state_.adminElevationTime = now;
            auth_state_.profileAuthTime = now;
            auth_state_.sessionExpiry = now + session_timeout_;
            auth_state_.isTamperResistant = true;
            
            // Calculate tamper-resistant hash
            updateTamperCheckHash();
            
            result.success = true;
            result.sessionExpiry = auth_state_.sessionExpiry;
            result.message = "Dual-layer authentication successful";
            
            std::cout << "[PrivilegeManager] Dual-layer authentication successful for profile: " << profileId << std::endl;
            
        } catch (const std::exception& e) {
            result.errorDetails = "Dual-layer authentication failed: " + std::string(e.what());
        }
        
        return result;
    }
    
    bool validateDualLayerAccess(PrivilegedOperation operation) const {
        if (!require_dual_layer_auth_) {
            return hasPrivilegeForOperation(operation);
        }
        
        std::lock_guard<std::mutex> lock(auth_state_mutex_);
        
        // Check if authentication state is valid and not tampered
        if (!validateAuthenticationIntegrity()) {
            return false;
        }
        
        // Check if session is expired
        if (auth_state_.isExpired()) {
            return false;
        }
        
        // Check if we have both layers of authentication
        return auth_state_.isFullyAuthenticated();
    }
    
    AuthenticationState getCurrentAuthenticationState() const {
        std::lock_guard<std::mutex> lock(auth_state_mutex_);
        return auth_state_;
    }
    
    bool refreshAuthenticationSession(const std::string& profileId, const std::string& masterKey) {
        try {
            std::lock_guard<std::mutex> lock(auth_state_mutex_);
            
            // Verify current authentication
            if (auth_state_.authenticatedProfileId != profileId) {
                return false;
            }
            
            // Re-authenticate with profile manager
            if (!profile_manager_) {
                return false;
            }
            
            auto auth_result = profile_manager_->authenticateProfile(profileId, masterKey);
            if (!auth_result.success) {
                return false;
            }
            
            // Extend session
            auto now = std::chrono::system_clock::now();
            auth_state_.profileAuthTime = now;
            auth_state_.sessionExpiry = now + session_timeout_;
            
            updateTamperCheckHash();
            
            std::cout << "[PrivilegeManager] Authentication session refreshed for profile: " << profileId << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Session refresh failed: " + std::string(e.what());
            return false;
        }
    }
    
    void clearAuthenticationSession() {
        std::lock_guard<std::mutex> lock(auth_state_mutex_);
        
        auth_state_ = AuthenticationState();
        tamper_check_hash_ = 0;
        
        std::cout << "[PrivilegeManager] Authentication session cleared" << std::endl;
    }
    
    bool validateAuthenticationIntegrity() const {
        if (!auth_state_.isTamperResistant) {
            return true; // No tamper resistance enabled
        }
        
        // Calculate expected hash
        size_t expected_hash = calculateAuthStateHash();
        
        return expected_hash == tamper_check_hash_;
    }
    
    void enableTamperResistance() {
        std::lock_guard<std::mutex> lock(auth_state_mutex_);
        auth_state_.isTamperResistant = true;
        updateTamperCheckHash();
    }
    
    void disableTamperResistance() {
        std::lock_guard<std::mutex> lock(auth_state_mutex_);
        auth_state_.isTamperResistant = false;
        tamper_check_hash_ = 0;
    }
    
    void setInstallationPath(const std::string& installPath) {
        installation_path_ = installPath;
    }
    
    bool protectInstallationDirectory(const std::string& installPath) {
        try {
            installation_path_ = installPath;
            
            if (!fs::exists(installPath)) {
                last_error_ = "Installation path does not exist: " + installPath;
                return false;
            }
            
            #ifdef PLATFORM_LINUX
            // Set restrictive permissions on installation directory
            fs::permissions(installPath, fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec, 
                          fs::perm_options::replace);
            
            // Protect against unauthorized modifications
            for (const auto& entry : fs::recursive_directory_iterator(installPath)) {
                if (entry.is_regular_file()) {
                    fs::permissions(entry.path(), fs::perms::owner_read | fs::perms::owner_exec | 
                                  fs::perms::group_read | fs::perms::group_exec, fs::perm_options::replace);
                }
            }
            
            #elif PLATFORM_WINDOWS
            // Set NTFS permissions to restrict access
            DWORD result = SetNamedSecurityInfoA(
                const_cast<char*>(installPath.c_str()),
                SE_FILE_OBJECT,
                DACL_SECURITY_INFORMATION,
                NULL, NULL, NULL, NULL
            );
            
            if (result != ERROR_SUCCESS) {
                last_error_ = "Failed to set Windows security permissions";
                return false;
            }
            
            #elif PLATFORM_MACOS
            // Set restrictive permissions on macOS
            fs::permissions(installPath, fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec, 
                          fs::perm_options::replace);
            #endif
            
            installation_protected_ = true;
            std::cout << "[PrivilegeManager] Installation directory protected: " << installPath << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to protect installation directory: " + std::string(e.what());
            return false;
        }
    }
    
    bool validateInstallationIntegrity() const {
        try {
            if (installation_path_.empty() || !installation_protected_) {
                return true; // No protection enabled
            }
            
            if (!fs::exists(installation_path_)) {
                return false;
            }
            
            // Check for unauthorized modifications
            for (const auto& entry : fs::recursive_directory_iterator(installation_path_)) {
                if (entry.is_regular_file()) {
                    #ifdef PLATFORM_LINUX
                    // Check if file permissions have been tampered with
                    auto perms = fs::status(entry.path()).permissions();
                    
                    // Check if write permissions have been added inappropriately
                    if ((perms & fs::perms::others_write) != fs::perms::none ||
                        (perms & fs::perms::group_write) != fs::perms::none) {
                        std::cout << "[PrivilegeManager] Installation integrity violation detected: " 
                                  << entry.path() << std::endl;
                        return false;
                    }
                    #endif
                }
            }
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Installation integrity check failed: " + std::string(e.what());
            return false;
        }
    }
    
    void enableProcessProtection() {
        try {
            #ifdef PLATFORM_LINUX
            // Set process to be non-dumpable (prevents ptrace attachment)
            if (prctl(PR_SET_DUMPABLE, 0) != 0) {
                last_error_ = "Failed to set process non-dumpable";
                return;
            }
            
            // Set process name to make it less obvious
            if (prctl(PR_SET_NAME, "systemd-phantom") != 0) {
                last_error_ = "Failed to set process name";
                return;
            }
            
            #elif PLATFORM_WINDOWS
            // Enable DEP (Data Execution Prevention)
            DWORD flags = PROCESS_DEP_ENABLE | PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION;
            if (!SetProcessDEPPolicy(flags)) {
                last_error_ = "Failed to enable DEP";
                return;
            }
            
            // Set process priority to avoid easy termination
            if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS)) {
                last_error_ = "Failed to set process priority";
                return;
            }
            
            #elif PLATFORM_MACOS
            // Set process to be non-dumpable
            if (ptrace(PT_DENY_ATTACH, 0, 0, 0) != 0) {
                last_error_ = "Failed to deny ptrace attachment";
                return;
            }
            #endif
            
            process_protected_ = true;
            std::cout << "[PrivilegeManager] Process protection enabled" << std::endl;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to enable process protection: " + std::string(e.what());
        }
    }
    
    void disableProcessProtection() {
        try {
            #ifdef PLATFORM_LINUX
            // Re-enable dumping
            prctl(PR_SET_DUMPABLE, 1);
            #elif PLATFORM_WINDOWS
            // Reset process priority
            SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
            #endif
            
            process_protected_ = false;
            std::cout << "[PrivilegeManager] Process protection disabled" << std::endl;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to disable process protection: " + std::string(e.what());
        }
    }
    
    bool isProcessProtected() const {
        return process_protected_;
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
    
    // Dual-layer authentication members
    std::chrono::minutes session_timeout_;
    bool require_dual_layer_auth_;
    ProfileManager* profile_manager_;
    AuthenticationState auth_state_;
    mutable std::mutex auth_state_mutex_;
    size_t tamper_check_hash_;
    
    // Self-protection members
    std::string installation_path_;
    bool process_protected_;
    bool installation_protected_;
    
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
                
                // Check authentication session integrity and expiry
                if (require_dual_layer_auth_) {
                    std::lock_guard<std::mutex> lock(auth_state_mutex_);
                    
                    // Check for session expiry
                    if (auth_state_.hasProfileAuthentication && auth_state_.isExpired()) {
                        std::cout << "[PrivilegeManager] Authentication session expired, clearing session" << std::endl;
                        auth_state_ = AuthenticationState();
                        tamper_check_hash_ = 0;
                    }
                    
                    // Check for tampering
                    if (auth_state_.isTamperResistant && !validateAuthenticationIntegrity()) {
                        std::cout << "[PrivilegeManager] Authentication tampering detected, clearing session" << std::endl;
                        auth_state_ = AuthenticationState();
                        tamper_check_hash_ = 0;
                        
                        if (privilege_loss_callback_) {
                            privilege_loss_callback_(PrivilegeLevel::USER);
                        }
                    }
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
    
    // Tamper resistance helper methods
    void updateTamperCheckHash() {
        tamper_check_hash_ = calculateAuthStateHash();
    }
    
    size_t calculateAuthStateHash() const {
        // Create a hash of the authentication state to detect tampering
        std::hash<std::string> hasher;
        std::string state_data = 
            std::to_string(auth_state_.hasAdminPrivileges) +
            std::to_string(auth_state_.hasProfileAuthentication) +
            auth_state_.authenticatedProfileId +
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                auth_state_.adminElevationTime.time_since_epoch()).count()) +
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                auth_state_.profileAuthTime.time_since_epoch()).count()) +
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                auth_state_.sessionExpiry.time_since_epoch()).count());
        
        return hasher(state_data);
    }
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

// Dual-layer access control methods
DualAuthResult PrivilegeManager::requestDualLayerAuthentication(const std::string& profileId, 
                                                               const std::string& masterKey,
                                                               PrivilegedOperation operation) {
    return pimpl->requestDualLayerAuthentication(profileId, masterKey, operation);
}

bool PrivilegeManager::validateDualLayerAccess(PrivilegedOperation operation) const {
    return pimpl->validateDualLayerAccess(operation);
}

AuthenticationState PrivilegeManager::getCurrentAuthenticationState() const {
    return pimpl->getCurrentAuthenticationState();
}

bool PrivilegeManager::refreshAuthenticationSession(const std::string& profileId, const std::string& masterKey) {
    return pimpl->refreshAuthenticationSession(profileId, masterKey);
}

void PrivilegeManager::clearAuthenticationSession() {
    pimpl->clearAuthenticationSession();
}

bool PrivilegeManager::validateAuthenticationIntegrity() const {
    return pimpl->validateAuthenticationIntegrity();
}

void PrivilegeManager::enableTamperResistance() {
    pimpl->enableTamperResistance();
}

void PrivilegeManager::disableTamperResistance() {
    pimpl->disableTamperResistance();
}

void PrivilegeManager::setSessionTimeout(std::chrono::minutes timeout) {
    pimpl->setSessionTimeout(timeout);
}

void PrivilegeManager::setRequireDualLayerAuth(bool required) {
    pimpl->setRequireDualLayerAuth(required);
}

void PrivilegeManager::setProfileManager(ProfileManager* profileManager) {
    pimpl->setProfileManager(profileManager);
}

void PrivilegeManager::setInstallationPath(const std::string& installPath) {
    pimpl->setInstallationPath(installPath);
}

bool PrivilegeManager::protectInstallationDirectory(const std::string& installPath) {
    return pimpl->protectInstallationDirectory(installPath);
}

bool PrivilegeManager::validateInstallationIntegrity() const {
    return pimpl->validateInstallationIntegrity();
}

void PrivilegeManager::enableProcessProtection() {
    pimpl->enableProcessProtection();
}

void PrivilegeManager::disableProcessProtection() {
    pimpl->disableProcessProtection();
}

bool PrivilegeManager::isProcessProtected() const {
    return pimpl->isProcessProtected();
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

// DualLayerAuthGuard implementation
DualLayerAuthGuard::DualLayerAuthGuard(PrivilegeManager* manager, const std::string& profileId, 
                                      const std::string& masterKey, PrivilegedOperation operation)
    : manager_(manager)
    , was_authenticated_(false)
    , admin_layer_passed_(false)
    , profile_layer_passed_(false)
    , profile_id_(profileId) {
    
    if (manager_) {
        auto result = manager_->requestDualLayerAuthentication(profileId, masterKey, operation);
        
        was_authenticated_ = result.success;
        admin_layer_passed_ = result.adminLayerPassed;
        profile_layer_passed_ = result.profileLayerPassed;
        profile_id_ = result.profileId;
        session_expiry_ = result.sessionExpiry;
        
        if (!was_authenticated_) {
            error_message_ = result.errorDetails;
        }
    }
}

DualLayerAuthGuard::~DualLayerAuthGuard() {
    // Session cleanup is handled by the PrivilegeManager's monitoring
    // or explicit clearAuthenticationSession() calls
}

bool DualLayerAuthGuard::isAuthenticated() const {
    return was_authenticated_;
}

bool DualLayerAuthGuard::hasAdminLayer() const {
    return admin_layer_passed_;
}

bool DualLayerAuthGuard::hasProfileLayer() const {
    return profile_layer_passed_;
}

std::string DualLayerAuthGuard::getProfileId() const {
    return profile_id_;
}

std::string DualLayerAuthGuard::getErrorMessage() const {
    return error_message_;
}

std::chrono::system_clock::time_point DualLayerAuthGuard::getSessionExpiry() const {
    return session_expiry_;
}

} // namespace phantomvault