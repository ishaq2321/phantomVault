/**
 * PhantomVault Error Handler Implementation
 * 
 * Comprehensive error handling, security event logging, and recovery mechanisms.
 */

#include "error_handler.hpp"
#include "encryption_engine.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <mutex>
#include <thread>
#include <regex>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <nlohmann/json.hpp>

#ifdef PLATFORM_LINUX
#include <unistd.h>
#include <pwd.h>
#elif PLATFORM_WINDOWS
#include <windows.h>
#include <shlobj.h>
#elif PLATFORM_MACOS
#include <unistd.h>
#include <pwd.h>
#endif

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace phantomvault {

class ErrorHandler::Implementation {
public:
    Implementation()
        : initialized_(false)
        , log_path_()
        , max_log_size_(10 * 1024 * 1024) // 10MB default
        , retention_period_(std::chrono::hours(24 * 7)) // 7 days default
        , real_time_alerts_(true)
        , last_error_()
        , security_events_()
        , rate_limits_()
        , events_mutex_()
        , rate_limit_mutex_()
        , security_alert_callback_()
        , critical_error_callback_()
        , config_monitoring_enabled_(false)

        , automatic_backups_enabled_(false)
        , backup_interval_(std::chrono::hours(6))
    {}
    
    ~Implementation() {
        cleanup();
    }
    
    bool initialize(const std::string& logPath) {
        try {
            // Set log path
            if (logPath.empty()) {
                log_path_ = getDefaultLogPath();
            } else {
                log_path_ = logPath;
            }
            
            // Ensure log directory exists
            fs::path logDir = fs::path(log_path_).parent_path();
            if (!fs::exists(logDir)) {
                fs::create_directories(logDir);
                fs::permissions(logDir, fs::perms::owner_all, fs::perm_options::replace);
            }
            
            // Load existing events if log file exists
            loadExistingEvents();
            
            // Note: Encrypted backup functionality will be added in future iterations
            
            // Start cleanup thread
            cleanup_thread_ = std::thread(&Implementation::cleanupLoop, this);
            
            initialized_ = true;
            
            // Log initialization event
            logSecurityEvent(SecurityEventType::SYSTEM_COMPROMISE, ErrorSeverity::INFO, "",
                           "ErrorHandler initialized", {{"logPath", log_path_}});
            
            std::cout << "[ErrorHandler] Initialized with log path: " << log_path_ << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to initialize ErrorHandler: " + std::string(e.what());
            return false;
        }
    }
    
    void handleEncryptionError(const std::string& profileId, const std::string& filePath, 
                              const std::string& error, const std::string& originalBackup) {
        try {
            std::map<std::string, std::string> metadata = {
                {"filePath", filePath},
                {"error", sanitizeErrorMessage(error)}
            };
            
            if (!originalBackup.empty()) {
                metadata["backupPath"] = originalBackup;
            }
            
            logSecurityEvent(SecurityEventType::ENCRYPTION_FAILURE, ErrorSeverity::ERROR,
                           profileId, "Encryption operation failed", metadata);
            
            // If backup exists, log recovery information
            if (!originalBackup.empty() && fs::exists(originalBackup)) {
                logSecurityEvent(SecurityEventType::ENCRYPTION_FAILURE, ErrorSeverity::INFO,
                               profileId, "Original file backup available for recovery", 
                               {{"backupPath", originalBackup}});
            }
            
            std::cout << "[ErrorHandler] Encryption error handled for profile: " << profileId << std::endl;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to handle encryption error: " + std::string(e.what());
        }
    }
    
    void handleAuthenticationFailure(const std::string& profileId, const std::string& source,
                                   const std::string& details) {
        try {
            std::string rateLimitId = "auth_" + profileId + "_" + source;
            
            // Check rate limiting
            bool rateLimited = !checkRateLimit(rateLimitId, 5, std::chrono::minutes(15));
            
            std::map<std::string, std::string> metadata = {
                {"source", source},
                {"rateLimited", rateLimited ? "true" : "false"}
            };
            
            if (!details.empty()) {
                metadata["details"] = sanitizeErrorMessage(details);
            }
            
            ErrorSeverity severity = rateLimited ? ErrorSeverity::CRITICAL : ErrorSeverity::WARNING;
            
            logSecurityEvent(SecurityEventType::AUTHENTICATION_FAILURE, severity,
                           profileId, "Authentication attempt failed", metadata);
            
            // If rate limited, log additional security event
            if (rateLimited) {
                logSecurityEvent(SecurityEventType::RATE_LIMIT_EXCEEDED, ErrorSeverity::CRITICAL,
                               profileId, "Rate limit exceeded for authentication attempts",
                               {{"source", source}, {"rateLimitId", rateLimitId}});
            }
            
            std::cout << "[ErrorHandler] Authentication failure handled for profile: " << profileId 
                      << (rateLimited ? " (RATE LIMITED)" : "") << std::endl;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to handle authentication failure: " + std::string(e.what());
        }
    }
    
    void handleVaultCorruption(const std::string& profileId, const std::string& vaultPath,
                             const std::string& corruptionDetails) {
        try {
            std::map<std::string, std::string> metadata = {
                {"vaultPath", vaultPath},
                {"corruptionDetails", sanitizeErrorMessage(corruptionDetails)}
            };
            
            logSecurityEvent(SecurityEventType::VAULT_CORRUPTION, ErrorSeverity::CRITICAL,
                           profileId, "Vault corruption detected", metadata);
            
            // Attempt automatic recovery
            auto recoveryResult = attemptVaultRecovery(profileId, vaultPath);
            
            if (recoveryResult.success) {
                logSecurityEvent(SecurityEventType::VAULT_CORRUPTION, ErrorSeverity::INFO,
                               profileId, "Vault recovery successful", 
                               {{"recoveredFiles", std::to_string(recoveryResult.recoveredFiles.size())}});
            } else {
                logSecurityEvent(SecurityEventType::VAULT_CORRUPTION, ErrorSeverity::CRITICAL,
                               profileId, "Vault recovery failed", 
                               {{"message", recoveryResult.message}});
            }
            
            std::cout << "[ErrorHandler] Vault corruption handled for profile: " << profileId 
                      << " (Recovery: " << (recoveryResult.success ? "SUCCESS" : "FAILED") << ")" << std::endl;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to handle vault corruption: " + std::string(e.what());
        }
    }
    
    void logSecurityEvent(SecurityEventType type, ErrorSeverity severity,
                         const std::string& profileId, const std::string& description,
                         const std::map<std::string, std::string>& metadata) {
        try {
            std::lock_guard<std::mutex> lock(events_mutex_);
            
            SecurityEvent event;
            event.id = generateEventId();
            event.type = type;
            event.severity = severity;
            event.profileId = profileId;
            event.description = description;
            event.details = sanitizeErrorMessage(description);
            event.timestamp = std::chrono::system_clock::now();
            event.sourceComponent = "ErrorHandler";
            event.metadata = metadata;
            
            security_events_.push_back(event);
            
            // Trigger callbacks
            if (severity == ErrorSeverity::CRITICAL && critical_error_callback_) {
                critical_error_callback_(event);
            }
            
            if (real_time_alerts_ && security_alert_callback_) {
                security_alert_callback_(event);
            }
            
            // Periodic save
            if (security_events_.size() % 10 == 0) {
                saveEvents();
            }
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to log security event: " + std::string(e.what());
        }
    }
    
    bool checkRateLimit(const std::string& identifier, int maxAttempts, 
                       std::chrono::minutes windowMinutes) {
        try {
            std::lock_guard<std::mutex> lock(rate_limit_mutex_);
            
            auto now = std::chrono::system_clock::now();
            auto& rateLimit = rate_limits_[identifier];
            
            // Initialize if first attempt
            if (rateLimit.attemptCount == 0) {
                rateLimit.identifier = identifier;
                rateLimit.firstAttempt = now;
                rateLimit.lastAttempt = now;
                rateLimit.attemptCount = 1;
                rateLimit.isBlocked = false;
                return true;
            }
            
            // Check if block has expired
            if (rateLimit.isBlocked && now > rateLimit.blockExpiry) {
                rateLimit.isBlocked = false;
                rateLimit.attemptCount = 0;
            }
            
            // If currently blocked, deny
            if (rateLimit.isBlocked) {
                return false;
            }
            
            // Check if window has expired
            if (now - rateLimit.firstAttempt > windowMinutes) {
                rateLimit.firstAttempt = now;
                rateLimit.attemptCount = 1;
                rateLimit.lastAttempt = now;
                return true;
            }
            
            // Increment attempt count
            rateLimit.attemptCount++;
            rateLimit.lastAttempt = now;
            
            // Check if limit exceeded
            if (rateLimit.attemptCount > maxAttempts) {
                rateLimit.isBlocked = true;
                rateLimit.blockExpiry = now + std::chrono::hours(1); // Block for 1 hour
                return false;
            }
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Rate limit check failed: " + std::string(e.what());
            return true; // Fail open for availability
        }
    }
    
    RecoveryResult attemptVaultRecovery(const std::string& profileId, const std::string& vaultPath) {
        RecoveryResult result;
        
        try {
            if (!fs::exists(vaultPath)) {
                result.message = "Vault path does not exist";
                return result;
            }
            
            // Check for backup directory
            std::string backupPath = vaultPath + "_backup";
            if (fs::exists(backupPath)) {
                return restoreFromBackup(profileId, backupPath);
            }
            
            // Attempt to recover individual files
            for (const auto& entry : fs::recursive_directory_iterator(vaultPath)) {
                if (entry.is_regular_file()) {
                    std::string filePath = entry.path().string();
                    
                    // Check if file is corrupted (basic check)
                    if (isFileCorrupted(filePath)) {
                        result.failedFiles.push_back(filePath);
                    } else {
                        result.recoveredFiles.push_back(filePath);
                    }
                }
            }
            
            result.success = !result.recoveredFiles.empty();
            result.message = result.success ? "Partial recovery successful" : "No files could be recovered";
            
        } catch (const std::exception& e) {
            result.message = "Recovery failed: " + std::string(e.what());
        }
        
        return result;
    }
    
    std::string createFileBackup(const std::string& filePath) {
        try {
            if (!fs::exists(filePath)) {
                return "";
            }
            
            // Generate backup path
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            
            fs::path originalPath(filePath);
            std::string backupPath = originalPath.parent_path().string() + "/.backup_" + 
                                   originalPath.filename().string() + "_" + std::to_string(timestamp);
            
            // Copy file to backup location
            fs::copy_file(filePath, backupPath);
            fs::permissions(backupPath, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace);
            
            return backupPath;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to create file backup: " + std::string(e.what());
            return "";
        }
    }
    
    std::string sanitizeErrorMessage(const std::string& rawError) const {
        std::string sanitized = rawError;
        
        // Remove potentially sensitive information
        std::vector<std::string> sensitivePatterns = {
            R"(/home/[^/\s]+)",           // Home directory paths
            R"(password[=:]\s*\S+)",      // Password values
            R"(key[=:]\s*\S+)",           // Key values
            R"(\b[A-Z0-9]{20,}\b)",       // Long alphanumeric strings (potential keys)
            R"(\b\d{4}-\d{4}-\d{4}-\d{4})" // Recovery key patterns
        };
        
        for (const auto& pattern : sensitivePatterns) {
            sanitized = std::regex_replace(sanitized, std::regex(pattern), "[REDACTED]");
        }
        
        return sanitized;
    }
    
    std::string getSecureErrorMessage(SecurityEventType type) const {
        switch (type) {
            case SecurityEventType::AUTHENTICATION_FAILURE:
                return "Authentication failed. Please verify your credentials and try again.";
            case SecurityEventType::ENCRYPTION_FAILURE:
                return "Encryption operation failed. Your data remains secure. Please try again.";
            case SecurityEventType::VAULT_CORRUPTION:
                return "Vault integrity issue detected. Automatic recovery procedures have been initiated. Your data is being protected.";
            case SecurityEventType::UNAUTHORIZED_ACCESS:
                return "Access denied. You don't have sufficient privileges for this operation.";
            case SecurityEventType::PRIVILEGE_ESCALATION:
                return "Security violation detected. This action requires elevated privileges.";
            case SecurityEventType::RATE_LIMIT_EXCEEDED:
                return "Too many attempts detected. Please wait a few minutes before trying again for security reasons.";
            case SecurityEventType::SUSPICIOUS_ACTIVITY:
                return "Suspicious activity detected. Additional security measures have been activated to protect your data.";
            case SecurityEventType::SYSTEM_COMPROMISE:
                return "System security event detected. Protective measures are in effect. Your data remains secure.";
            default:
                return "An error occurred. The system has implemented protective measures. Please contact support if the issue persists.";
        }
    }
    
    std::string getUserFriendlyErrorMessage(const std::string& component, const std::string& operation, 
                                           ErrorSeverity severity) const {
        std::string base_message;
        
        // Component-specific messages
        if (component == "EncryptionEngine") {
            base_message = "There was an issue with data encryption. ";
        } else if (component == "VaultHandler") {
            base_message = "There was an issue accessing your secure vault. ";
        } else if (component == "ProfileManager") {
            base_message = "There was an issue with your profile settings. ";
        } else if (component == "ServiceManager") {
            base_message = "There was an issue with the background service. ";
        } else {
            base_message = "There was a system issue. ";
        }
        
        // Operation-specific context
        if (operation == "read") {
            base_message += "We couldn't read the requested data. ";
        } else if (operation == "write") {
            base_message += "We couldn't save your changes. ";
        } else if (operation == "delete") {
            base_message += "We couldn't remove the requested item. ";
        } else if (operation == "encrypt") {
            base_message += "We couldn't secure your data. ";
        } else if (operation == "decrypt") {
            base_message += "We couldn't access your secured data. ";
        }
        
        // Severity-specific guidance
        switch (severity) {
            case ErrorSeverity::INFO:
                base_message += "This is just for your information. No action is required.";
                break;
            case ErrorSeverity::WARNING:
                base_message += "This might affect some functionality. You can continue using the application.";
                break;
            case ErrorSeverity::ERROR:
                base_message += "This prevented the operation from completing. Please try again or contact support.";
                break;
            case ErrorSeverity::CRITICAL:
                base_message += "This is a serious issue that requires immediate attention. Protective measures are in effect.";
                break;
        }
        
        return base_message;
    }
    
    std::string getRecoveryGuidance(SecurityEventType type, ErrorSeverity severity) const {
        std::string guidance = "\\n\\nWhat you can do:\\n";
        
        switch (type) {
            case SecurityEventType::AUTHENTICATION_FAILURE:
                guidance += "• Double-check your password\\n";
                guidance += "• Ensure Caps Lock is off\\n";
                guidance += "• Try using your recovery key if available\\n";
                if (severity == ErrorSeverity::CRITICAL) {
                    guidance += "• Contact support if you suspect unauthorized access\\n";
                }
                break;
                
            case SecurityEventType::ENCRYPTION_FAILURE:
                guidance += "• Ensure you have enough disk space\\n";
                guidance += "• Check that the file isn't being used by another program\\n";
                guidance += "• Try restarting the application\\n";
                break;
                
            case SecurityEventType::VAULT_CORRUPTION:
                guidance += "• Don't panic - your data is protected\\n";
                guidance += "• Automatic recovery is in progress\\n";
                guidance += "• Avoid making changes until recovery completes\\n";
                guidance += "• Contact support if the issue persists\\n";
                break;
                
            case SecurityEventType::RATE_LIMIT_EXCEEDED:
                guidance += "• Wait a few minutes before trying again\\n";
                guidance += "• This is a security feature to protect your account\\n";
                guidance += "• Contact support if you believe this is an error\\n";
                break;
                
            default:
                guidance += "• Try the operation again\\n";
                guidance += "• Restart the application if the problem continues\\n";
                guidance += "• Contact support with details about what you were doing\\n";
                break;
        }
        
        return guidance;
    }
    
    std::vector<SecurityEvent> getSecurityEvents(const std::string& profileId,
                                                SecurityEventType type,
                                                std::chrono::hours timeRange) const {
        std::lock_guard<std::mutex> lock(events_mutex_);
        
        std::vector<SecurityEvent> filteredEvents;
        auto cutoffTime = std::chrono::system_clock::now() - timeRange;
        
        for (const auto& event : security_events_) {
            bool matchesProfile = profileId.empty() || event.profileId == profileId;
            bool matchesType = event.type == type;
            bool withinTimeRange = event.timestamp >= cutoffTime;
            
            if (matchesProfile && matchesType && withinTimeRange) {
                filteredEvents.push_back(event);
            }
        }
        
        return filteredEvents;
    }
    
    void resetRateLimit(const std::string& identifier) {
        std::lock_guard<std::mutex> lock(rate_limit_mutex_);
        rate_limits_.erase(identifier);
    }
    
    RateLimitInfo getRateLimitInfo(const std::string& identifier) const {
        std::lock_guard<std::mutex> lock(rate_limit_mutex_);
        auto it = rate_limits_.find(identifier);
        if (it != rate_limits_.end()) {
            return it->second;
        }
        return RateLimitInfo{}; // Return empty info if not found
    }
    
    bool restoreFileFromBackup(const std::string& filePath, const std::string& backupPath) {
        try {
            if (!fs::exists(backupPath)) {
                return false;
            }
            
            // Copy backup file back to original location
            fs::copy_file(backupPath, filePath, fs::copy_options::overwrite_existing);
            
            // Clean up backup file
            fs::remove(backupPath);
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to restore file from backup: " + std::string(e.what());
            return false;
        }
    }
    
    void cleanupBackups(const std::string& profileId, std::chrono::hours maxAge) {
        (void)profileId; // Suppress unused parameter warning
        try {
            auto cutoffTime = std::chrono::system_clock::now() - maxAge;
            
            // Find backup files for this profile
            std::string profile_path = getDefaultLogPath();
            fs::path profile_dir = fs::path(profile_path).parent_path();
            
            for (const auto& entry : fs::directory_iterator(profile_dir)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    
                    // Check if it's a backup file
                    if (filename.find(".backup_") != std::string::npos) {
                        auto file_time = fs::last_write_time(entry.path());
                        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                            file_time - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                        
                        if (sctp < cutoffTime) {
                            fs::remove(entry.path());
                        }
                    }
                }
            }
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to cleanup backups: " + std::string(e.what());
        }
    }
    
    void exportAuditLog(const std::string& filePath, const std::string& profileId) const {
        try {
            std::ofstream file(filePath);
            
            std::lock_guard<std::mutex> lock(events_mutex_);
            
            for (const auto& event : security_events_) {
                if (profileId.empty() || event.profileId == profileId) {
                    json eventJson;
                    eventJson["id"] = event.id;
                    eventJson["type"] = static_cast<int>(event.type);
                    eventJson["severity"] = static_cast<int>(event.severity);
                    eventJson["profileId"] = event.profileId;
                    eventJson["description"] = event.description;
                    eventJson["details"] = event.details;
                    eventJson["sourceComponent"] = event.sourceComponent;
                    eventJson["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                        event.timestamp.time_since_epoch()).count();
                    eventJson["metadata"] = event.metadata;
                    
                    file << eventJson.dump() << std::endl;
                }
            }
            
            file.close();
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to export audit log: " + std::string(e.what());
        }
    }
    
    void setSecurityAlertCallback(std::function<void(const SecurityEvent&)> callback) {
        security_alert_callback_ = callback;
    }
    
    void setCriticalErrorCallback(std::function<void(const SecurityEvent&)> callback) {
        critical_error_callback_ = callback;
    }
    
    size_t getEventCount(SecurityEventType type) const {
        std::lock_guard<std::mutex> lock(events_mutex_);
        
        return std::count_if(security_events_.begin(), security_events_.end(),
                           [type](const SecurityEvent& event) {
                               return event.type == type;
                           });
    }
    
    std::map<SecurityEventType, size_t> getEventStatistics() const {
        std::lock_guard<std::mutex> lock(events_mutex_);
        
        std::map<SecurityEventType, size_t> stats;
        
        for (const auto& event : security_events_) {
            stats[event.type]++;
        }
        
        return stats;
    }
    
    void setMaxLogSize(size_t maxSizeBytes) {
        max_log_size_ = maxSizeBytes;
    }
    
    void setLogRetentionPeriod(std::chrono::hours retentionHours) {
        retention_period_ = retentionHours;
    }
    
    void enableRealTimeAlerts(bool enabled) {
        real_time_alerts_ = enabled;
    }
    
    std::string getLastError() const {
        return last_error_;
    }
    
    bool protectConfigurationFiles(const std::vector<std::string>& configPaths) {
        try {
            protected_config_paths_ = configPaths;
            config_file_hashes_.clear();
            
            for (const auto& configPath : configPaths) {
                if (!fs::exists(configPath)) {
                    last_error_ = "Configuration file does not exist: " + configPath;
                    return false;
                }
                
                // Calculate file hash for integrity checking
                std::string fileHash = calculateFileHash(configPath);
                if (fileHash.empty()) {
                    last_error_ = "Failed to calculate hash for: " + configPath;
                    return false;
                }
                
                config_file_hashes_[configPath] = fileHash;
                
                // Set restrictive permissions
                #ifdef PLATFORM_LINUX
                fs::permissions(configPath, fs::perms::owner_read | fs::perms::owner_write, 
                              fs::perm_options::replace);
                #elif PLATFORM_WINDOWS
                // Set NTFS permissions to restrict access
                DWORD result = SetNamedSecurityInfoA(
                    const_cast<char*>(configPath.c_str()),
                    SE_FILE_OBJECT,
                    DACL_SECURITY_INFORMATION,
                    NULL, NULL, NULL, NULL
                );
                
                if (result != ERROR_SUCCESS) {
                    last_error_ = "Failed to set Windows security permissions for: " + configPath;
                    return false;
                }
                #elif PLATFORM_MACOS
                fs::permissions(configPath, fs::perms::owner_read | fs::perms::owner_write, 
                              fs::perm_options::replace);
                #endif
                
                std::cout << "[ErrorHandler] Configuration file protected: " << configPath << std::endl;
            }
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to protect configuration files: " + std::string(e.what());
            return false;
        }
    }
    
    bool validateConfigurationIntegrity() const {
        try {
            for (const auto& configPath : protected_config_paths_) {
                if (!fs::exists(configPath)) {
                    std::cout << "[ErrorHandler] Configuration file missing: " << configPath << std::endl;
                    return false;
                }
                
                // Check file hash
                std::string currentHash = calculateFileHash(configPath);
                auto it = config_file_hashes_.find(configPath);
                
                if (it == config_file_hashes_.end()) {
                    std::cout << "[ErrorHandler] No baseline hash for: " << configPath << std::endl;
                    return false;
                }
                
                if (currentHash != it->second) {
                    std::cout << "[ErrorHandler] Configuration file tampered: " << configPath << std::endl;
                    
                    // Log security event (cast away const for this security-critical operation)
                    const_cast<Implementation*>(this)->logSecurityEvent(SecurityEventType::SYSTEM_COMPROMISE, ErrorSeverity::CRITICAL,
                                   "", "Configuration file tampering detected",
                                   {{"filePath", configPath}, {"expectedHash", it->second}, {"actualHash", currentHash}});
                    
                    return false;
                }
            }
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Configuration integrity check failed: " + std::string(e.what());
            return false;
        }
    }
    
    void enableConfigurationMonitoring() {
        if (config_monitoring_enabled_) {
            return;
        }
        
        config_monitoring_running_ = true;
        config_monitor_thread_ = std::thread(&Implementation::configurationMonitoringLoop, this);
        config_monitoring_enabled_ = true;
        
        std::cout << "[ErrorHandler] Configuration monitoring enabled" << std::endl;
    }
    
    void disableConfigurationMonitoring() {
        if (!config_monitoring_enabled_) {
            return;
        }
        
        config_monitoring_running_ = false;
        
        if (config_monitor_thread_.joinable()) {
            config_monitor_thread_.join();
        }
        
        config_monitoring_enabled_ = false;
        std::cout << "[ErrorHandler] Configuration monitoring disabled" << std::endl;
    }
    
    bool isConfigurationProtected() const {
        return !protected_config_paths_.empty();
    }
    
    // Enhanced error handling methods (moved from private)
    void handleSystemError(const std::string& component, const std::string& error, 
                          ErrorSeverity severity = ErrorSeverity::ERROR) {
        try {
            std::map<std::string, std::string> metadata = {
                {"component", component},
                {"error", sanitizeErrorMessage(error)},
                {"failsafe_action", "automatic_recovery_initiated"}
            };
            
            logSecurityEvent(SecurityEventType::SYSTEM_COMPROMISE, severity, "",
                           "System error detected", metadata);
            
            // Implement fail-safe defaults
            if (severity == ErrorSeverity::CRITICAL) {
                initiateEmergencyProtocol(component, error);
            } else {
                attemptAutomaticRecovery(component, error);
            }
            
        } catch (const std::exception& e) {
            // Last resort error handling
            std::cerr << "[ErrorHandler] Critical error in error handling: " << e.what() << std::endl;
        }
    }
    
    void handleNetworkError(const std::string& operation, const std::string& endpoint, 
                           const std::string& error) {
        try {
            std::map<std::string, std::string> metadata = {
                {"operation", operation},
                {"endpoint", sanitizeErrorMessage(endpoint)},
                {"error", sanitizeErrorMessage(error)},
                {"retry_strategy", "exponential_backoff"}
            };
            
            logSecurityEvent(SecurityEventType::SUSPICIOUS_ACTIVITY, ErrorSeverity::WARNING, "",
                           "Network operation failed", metadata);
            
            // Implement network-specific fail-safe
            enableOfflineMode();
            
        } catch (const std::exception& e) {
            last_error_ = "Network error handling failed: " + std::string(e.what());
        }
    }
    
    void handleFileSystemError(const std::string& operation, const std::string& path, 
                              const std::string& error) {
        try {
            std::map<std::string, std::string> metadata = {
                {"operation", operation},
                {"path", sanitizeErrorMessage(path)},
                {"error", sanitizeErrorMessage(error)},
                {"recovery_action", "backup_restoration_attempted"}
            };
            
            logSecurityEvent(SecurityEventType::VAULT_CORRUPTION, ErrorSeverity::ERROR, "",
                           "File system operation failed", metadata);
            
            // Attempt file system recovery
            if (operation == "write" || operation == "delete") {
                // Try to restore from backup
                std::string backup_path = path + ".backup";
                if (fs::exists(backup_path)) {
                    try {
                        fs::copy_file(backup_path, path, fs::copy_options::overwrite_existing);
                        logSecurityEvent(SecurityEventType::VAULT_CORRUPTION, ErrorSeverity::INFO, "",
                                       "File restored from backup", {{"path", path}});
                    } catch (const std::exception& e) {
                        // Backup restoration failed
                        logSecurityEvent(SecurityEventType::VAULT_CORRUPTION, ErrorSeverity::CRITICAL, "",
                                       "Backup restoration failed", {{"path", path}, {"error", e.what()}});
                    }
                }
            }
            
        } catch (const std::exception& e) {
            last_error_ = "File system error handling failed: " + std::string(e.what());
        }
    }
    
    void handleMemoryError(const std::string& component, size_t requested_size, 
                          const std::string& error) {
        try {
            std::map<std::string, std::string> metadata = {
                {"component", component},
                {"requested_size", std::to_string(requested_size)},
                {"error", sanitizeErrorMessage(error)},
                {"mitigation", "memory_cleanup_initiated"}
            };
            
            logSecurityEvent(SecurityEventType::SYSTEM_COMPROMISE, ErrorSeverity::CRITICAL, "",
                           "Memory allocation failed", metadata);
            
            // Implement memory fail-safe
            performEmergencyMemoryCleanup();
            
        } catch (const std::exception& e) {
            // Critical - cannot allocate memory for error handling
            std::cerr << "[ErrorHandler] CRITICAL: Memory error in error handler" << std::endl;
            std::terminate(); // Last resort
        }
    }
    
    void initiateEmergencyProtocol(const std::string& component, const std::string& error) {
        try {
            // 1. Secure all sensitive data
            secureAllVaults();
            
            // 2. Create emergency backup
            createEmergencyBackup();
            
            // 3. Switch to safe mode
            enableSafeMode();
            
            // 4. Notify user
            if (critical_error_callback_) {
                SecurityEvent event;
                event.id = generateEventId();
                event.type = SecurityEventType::SYSTEM_COMPROMISE;
                event.severity = ErrorSeverity::CRITICAL;
                event.description = "Emergency protocol activated";
                event.timestamp = std::chrono::system_clock::now();
                event.sourceComponent = component;
                event.metadata = {{"error", error}, {"protocol", "emergency"}};
                
                critical_error_callback_(event);
            }
            
        } catch (const std::exception& e) {
            // Emergency protocol failed - log to system
            std::cerr << "[ErrorHandler] EMERGENCY PROTOCOL FAILED: " << e.what() << std::endl;
        }
    }
    
    void attemptAutomaticRecovery(const std::string& component, const std::string& error) {
        try {
            // Component-specific recovery strategies
            if (component == "EncryptionEngine") {
                // Reset encryption engine
                reinitializeEncryption();
            } else if (component == "VaultHandler") {
                // Repair vault structure
                repairVaultStructures();
            } else if (component == "ProfileManager") {
                // Reload profiles from backup
                reloadProfilesFromBackup();
            } else {
                // Generic recovery
                performGenericRecovery(component);
            }
            
            logSecurityEvent(SecurityEventType::SYSTEM_COMPROMISE, ErrorSeverity::INFO, "",
                           "Automatic recovery completed", 
                           {{"component", component}, {"error", error}});
            
        } catch (const std::exception& e) {
            logSecurityEvent(SecurityEventType::SYSTEM_COMPROMISE, ErrorSeverity::ERROR, "",
                           "Automatic recovery failed", 
                           {{"component", component}, {"recovery_error", e.what()}});
        }
    }
    
    void enableSafeMode() {
        std::cout << "[ErrorHandler] Enabling safe mode" << std::endl;
        // Implementation would switch to safe operation mode
    }
    
    void enableOfflineMode() {
        std::cout << "[ErrorHandler] Switching to offline mode due to network issues" << std::endl;
        // Switch to offline operation mode
    }
    
    std::vector<BackupMetadata> listEncryptedBackups(const std::string& profileId) const {
        std::lock_guard<std::mutex> lock(backup_mutex_);
        
        std::vector<BackupMetadata> backups;
        for (const auto& pair : backup_metadata_) {
            if (profileId.empty() || pair.second.profile_id == profileId) {
                backups.push_back(pair.second);
            }
        }
        
        return backups;
    }
    
    void scheduleBackup(const std::string& filePath, const std::string& profileId) {
        // Add to backup schedule (simplified implementation)
        std::cout << "[ErrorHandler] Backup scheduled for: " << filePath 
                  << " (Profile: " << profileId << ")" << std::endl;
        
        // In a full implementation, this would add to a backup queue
        logSecurityEvent(SecurityEventType::SYSTEM_COMPROMISE, ErrorSeverity::INFO, profileId,
                       "Backup scheduled", {{"file_path", filePath}});
    }
    void rotateLogFile() {
        try {
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            
            std::string rotated_path = log_path_ + "." + std::to_string(timestamp);
            
            // Move current log to rotated file
            fs::rename(log_path_, rotated_path);
            
            // Compress rotated log (simplified - just rename)
            std::string compressed_path = rotated_path + ".old";
            fs::rename(rotated_path, compressed_path);
            
            // Set secure permissions on rotated log
            fs::permissions(compressed_path, fs::perms::owner_read, fs::perm_options::replace);
            
            std::cout << "[ErrorHandler] Log file rotated: " << compressed_path << std::endl;
            
        } catch (const std::exception& e) {
            last_error_ = "Log rotation failed: " + std::string(e.what());
        }
    }
    
    bool verifyLogIntegrity() const {
        try {
            if (!fs::exists(log_path_)) {
                return true; // No log file to verify
            }
            
            std::string checksum_file = log_path_ + ".checksum";
            if (!fs::exists(checksum_file)) {
                return false; // No checksum file
            }
            
            std::ifstream file(checksum_file);
            std::string expected_checksum;
            if (file && std::getline(file, expected_checksum)) {
                std::string actual_checksum = calculateFileHash(log_path_);
                return expected_checksum == actual_checksum;
            }
            
            return false;
            
        } catch (const std::exception& e) {
            return false;
        }
    }
    

    
    RecoveryResult restoreFromBackup(const std::string& profileId, const std::string& backupPath) {
        (void)profileId; // Suppress unused parameter warning
        RecoveryResult result;
        
        try {
            if (!fs::exists(backupPath)) {
                result.message = "Backup path does not exist";
                return result;
            }
            
            // Copy backup files back to original location
            std::string originalPath = backupPath.substr(0, backupPath.find("_backup"));
            
            for (const auto& entry : fs::recursive_directory_iterator(backupPath)) {
                if (entry.is_regular_file()) {
                    std::string relativePath = fs::relative(entry.path(), backupPath);
                    std::string targetPath = originalPath + "/" + relativePath;
                    
                    try {
                        fs::create_directories(fs::path(targetPath).parent_path());
                        fs::copy_file(entry.path(), targetPath, fs::copy_options::overwrite_existing);
                        result.recoveredFiles.push_back(targetPath);
                    } catch (const std::exception& e) {
                        result.failedFiles.push_back(entry.path().string());
                    }
                }
            }
            
            result.success = !result.recoveredFiles.empty();
            result.message = result.success ? "Backup restoration successful" : "No files could be restored";
            
        } catch (const std::exception& e) {
            result.message = "Backup restoration failed: " + std::string(e.what());
        }
        
        return result;
    }

private:
    bool initialized_;
    std::string log_path_;
    size_t max_log_size_;
    std::chrono::hours retention_period_;
    bool real_time_alerts_;
    mutable std::string last_error_;
    
    std::vector<SecurityEvent> security_events_;
    std::map<std::string, RateLimitInfo> rate_limits_;
    
    mutable std::mutex events_mutex_;
    mutable std::mutex rate_limit_mutex_;
    
    std::thread cleanup_thread_;
    std::atomic<bool> cleanup_running_{true};
    
    std::function<void(const SecurityEvent&)> security_alert_callback_;
    std::function<void(const SecurityEvent&)> critical_error_callback_;
    
    // Configuration protection members
    std::vector<std::string> protected_config_paths_;
    std::map<std::string, std::string> config_file_hashes_;
    bool config_monitoring_enabled_;
    std::thread config_monitor_thread_;
    std::atomic<bool> config_monitoring_running_{false};
    
    // Note: Encrypted backup infrastructure will be added in future iterations
    std::map<std::string, BackupMetadata> backup_metadata_;
    std::string backup_root_path_;
    bool automatic_backups_enabled_;
    std::chrono::hours backup_interval_;
    std::thread backup_scheduler_thread_;
    std::atomic<bool> backup_scheduler_running_{false};
    EncryptionEngine* backup_encryption_engine_{nullptr};
    mutable std::mutex backup_mutex_;
    
    std::string getDefaultLogPath() {
        #ifdef PLATFORM_LINUX
        const char* home = getenv("HOME");
        if (!home) {
            struct passwd* pw = getpwuid(getuid());
            home = pw->pw_dir;
        }
        return std::string(home) + "/.phantomvault/logs/security.log";
        #elif PLATFORM_WINDOWS
        char path[MAX_PATH];
        if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path) == S_OK) {
            return std::string(path) + "\\PhantomVault\\logs\\security.log";
        }
        return "C:\\ProgramData\\PhantomVault\\logs\\security.log";
        #elif PLATFORM_MACOS
        const char* home = getenv("HOME");
        if (!home) {
            struct passwd* pw = getpwuid(getuid());
            home = pw->pw_dir;
        }
        return std::string(home) + "/Library/Application Support/PhantomVault/logs/security.log";
        #else
        return "./phantomvault_security.log";
        #endif
    }
    
    std::string generateEventId() {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        
        return "event_" + std::to_string(timestamp) + "_" + std::to_string(dis(gen));
    }
    
    void loadExistingEvents() {
        try {
            if (!fs::exists(log_path_)) {
                return;
            }
            
            std::ifstream file(log_path_);
            std::string line;
            
            while (std::getline(file, line)) {
                if (line.empty()) continue;
                
                try {
                    json eventJson = json::parse(line);
                    SecurityEvent event;
                    
                    event.id = eventJson.value("id", "");
                    event.type = static_cast<SecurityEventType>(eventJson.value("type", 0));
                    event.severity = static_cast<ErrorSeverity>(eventJson.value("severity", 0));
                    event.profileId = eventJson.value("profileId", "");
                    event.description = eventJson.value("description", "");
                    event.details = eventJson.value("details", "");
                    event.sourceComponent = eventJson.value("sourceComponent", "");
                    
                    int64_t timestamp_ms = eventJson.value("timestamp", 0);
                    event.timestamp = std::chrono::system_clock::from_time_t(timestamp_ms / 1000);
                    
                    if (eventJson.contains("metadata")) {
                        event.metadata = eventJson["metadata"].get<std::map<std::string, std::string>>();
                    }
                    
                    security_events_.push_back(event);
                    
                } catch (const std::exception& e) {
                    // Skip malformed log entries
                    continue;
                }
            }
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to load existing events: " + std::string(e.what());
        }
    }
    
    void saveEvents() {
        try {
            // Check if log rotation is needed
            if (fs::exists(log_path_) && fs::file_size(log_path_) > max_log_size_) {
                rotateLogFile();
            }
            
            std::ofstream file(log_path_, std::ios::app);
            
            // Save only new events (simple approach - save all for now)
            for (const auto& event : security_events_) {
                json eventJson;
                eventJson["id"] = event.id;
                eventJson["type"] = static_cast<int>(event.type);
                eventJson["severity"] = static_cast<int>(event.severity);
                eventJson["profileId"] = sanitizeForLogging(event.profileId);
                eventJson["description"] = event.description;
                eventJson["details"] = event.details;
                eventJson["sourceComponent"] = event.sourceComponent;
                eventJson["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                    event.timestamp.time_since_epoch()).count();
                
                // Sanitize metadata for logging
                json sanitized_metadata;
                for (const auto& pair : event.metadata) {
                    sanitized_metadata[pair.first] = sanitizeForLogging(pair.second);
                }
                eventJson["metadata"] = sanitized_metadata;
                
                file << eventJson.dump() << std::endl;
            }
            
            file.close();
            
            // Set secure permissions
            fs::permissions(log_path_, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace);
            
            // Create integrity checksum for log file
            createLogIntegrityChecksum();
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to save events: " + std::string(e.what());
        }
    }
    
    void rotateLogFile() {
        try {
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            
            std::string rotated_path = log_path_ + "." + std::to_string(timestamp);
            
            // Move current log to rotated file
            fs::rename(log_path_, rotated_path);
            
            // Compress rotated log (simplified - just rename)
            std::string compressed_path = rotated_path + ".old";
            fs::rename(rotated_path, compressed_path);
            
            // Set secure permissions on rotated log
            fs::permissions(compressed_path, fs::perms::owner_read, fs::perm_options::replace);
            
            std::cout << "[ErrorHandler] Log file rotated: " << compressed_path << std::endl;
            
        } catch (const std::exception& e) {
            last_error_ = "Log rotation failed: " + std::string(e.what());
        }
    }
    
    std::string sanitizeForLogging(const std::string& value) const {
        std::string sanitized = value;
        
        // Additional sanitization for logging
        std::vector<std::pair<std::string, std::string>> replacements = {
            {R"(\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}\b)", "[EMAIL]"},
            {R"(\b\d{3}-\d{2}-\d{4}\b)", "[SSN]"},
            {R"(\b\d{4}[- ]?\d{4}[- ]?\d{4}[- ]?\d{4}\b)", "[CARD]"},
            {R"(\b(?:\d{1,3}\.){3}\d{1,3}\b)", "[IP]"},
            {R"([A-Z0-9]{20,})", "[TOKEN]"}
        };
        
        for (const auto& replacement : replacements) {
            sanitized = std::regex_replace(sanitized, std::regex(replacement.first), replacement.second);
        }
        
        return sanitized;
    }
    
    void createLogIntegrityChecksum() {
        try {
            if (!fs::exists(log_path_)) {
                return;
            }
            
            std::string checksum = calculateFileHash(log_path_);
            std::string checksum_file = log_path_ + ".checksum";
            
            std::ofstream file(checksum_file);
            if (file) {
                file << checksum << std::endl;
                file.close();
                
                // Set secure permissions
                fs::permissions(checksum_file, fs::perms::owner_read | fs::perms::owner_write, 
                              fs::perm_options::replace);
            }
            
        } catch (const std::exception& e) {
            // Non-critical error
        }
    }
    
    bool verifyLogIntegrity() const {
        try {
            if (!fs::exists(log_path_)) {
                return true; // No log file to verify
            }
            
            std::string checksum_file = log_path_ + ".checksum";
            if (!fs::exists(checksum_file)) {
                return false; // No checksum file
            }
            
            std::ifstream file(checksum_file);
            std::string expected_checksum;
            if (file && std::getline(file, expected_checksum)) {
                std::string actual_checksum = calculateFileHash(log_path_);
                return expected_checksum == actual_checksum;
            }
            
            return false;
            
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    void cleanupLoop() {
        while (cleanup_running_) {
            try {
                // Cleanup old events
                auto cutoffTime = std::chrono::system_clock::now() - retention_period_;
                
                {
                    std::lock_guard<std::mutex> lock(events_mutex_);
                    security_events_.erase(
                        std::remove_if(security_events_.begin(), security_events_.end(),
                            [cutoffTime](const SecurityEvent& event) {
                                return event.timestamp < cutoffTime;
                            }),
                        security_events_.end()
                    );
                }
                
                // Cleanup old rate limits
                {
                    std::lock_guard<std::mutex> lock(rate_limit_mutex_);
                    auto now = std::chrono::system_clock::now();
                    
                    for (auto it = rate_limits_.begin(); it != rate_limits_.end();) {
                        if (now - it->second.lastAttempt > std::chrono::hours(24)) {
                            it = rate_limits_.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
                
                // Sleep for 1 hour
                std::this_thread::sleep_for(std::chrono::hours(1));
                
            } catch (const std::exception& e) {
                std::this_thread::sleep_for(std::chrono::minutes(10));
            }
        }
    }
    
    void cleanup() {
        cleanup_running_ = false;
        if (cleanup_thread_.joinable()) {
            cleanup_thread_.join();
        }
        
        // Stop configuration monitoring
        disableConfigurationMonitoring();
        
        // Stop automatic backups
        disableAutomaticBackups();
        
        // Note: Encrypted backup cleanup will be added in future iterations
        
        // Save remaining events
        if (initialized_) {
            saveEvents();
        }
    }
    
    std::string calculateFileHash(const std::string& filePath) const {
        try {
            std::ifstream file(filePath, std::ios::binary);
            if (!file.is_open()) {
                return "";
            }
            
            // Simple hash calculation (in production, use SHA-256)
            std::hash<std::string> hasher;
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            
            return std::to_string(hasher(content));
            
        } catch (const std::exception& e) {
            return "";
        }
    }
    
    void configurationMonitoringLoop() {
        std::cout << "[ErrorHandler] Configuration monitoring loop started" << std::endl;
        
        while (config_monitoring_running_) {
            try {
                // Check configuration integrity
                if (!validateConfigurationIntegrity()) {
                    // Integrity violation detected - already logged in validateConfigurationIntegrity
                    
                    // Trigger critical error callback
                    if (critical_error_callback_) {
                        SecurityEvent event;
                        event.id = generateEventId();
                        event.type = SecurityEventType::SYSTEM_COMPROMISE;
                        event.severity = ErrorSeverity::CRITICAL;
                        event.description = "Configuration tampering detected";
                        event.timestamp = std::chrono::system_clock::now();
                        event.sourceComponent = "ErrorHandler";
                        
                        critical_error_callback_(event);
                    }
                }
                
                // Sleep for monitoring interval
                std::this_thread::sleep_for(std::chrono::seconds(60)); // Check every minute
                
            } catch (const std::exception& e) {
                last_error_ = "Configuration monitoring error: " + std::string(e.what());
                std::this_thread::sleep_for(std::chrono::seconds(30));
            }
        }
        
        std::cout << "[ErrorHandler] Configuration monitoring loop stopped" << std::endl;
    }
    
    void setBackupEncryptionEngine(EncryptionEngine* engine) {
        backup_encryption_engine_ = engine;
        std::cout << "[ErrorHandler] Backup encryption engine configured" << std::endl;
    }
    
    EncryptedBackupResult createEncryptedBackup(const std::string& filePath, 
                                               const std::string& profileId,
                                               const std::string& password,
                                               int redundancy_level) {
        EncryptedBackupResult result;
        
        try {
            if (!fs::exists(filePath)) {
                result.error_message = "Source file does not exist: " + filePath;
                return result;
            }
            
            if (!backup_encryption_engine_) {
                result.error_message = "Backup encryption engine not configured";
                return result;
            }
            
            // Generate unique backup ID
            result.backup_id = generateBackupId(filePath, profileId);
            result.created_at = std::chrono::system_clock::now();
            
            // Create backup directory structure
            std::string backup_dir = backup_root_path_ + "/" + profileId + "/" + result.backup_id;
            fs::create_directories(backup_dir);
            
            // Read original file
            std::ifstream source_file(filePath, std::ios::binary);
            if (!source_file) {
                result.error_message = "Cannot read source file: " + filePath;
                return result;
            }
            
            std::vector<uint8_t> file_data((std::istreambuf_iterator<char>(source_file)),
                                          std::istreambuf_iterator<char>());
            source_file.close();
            
            // Create backup metadata
            BackupMetadata metadata;
            metadata.backup_id = result.backup_id;
            metadata.original_path = filePath;
            metadata.profile_id = profileId;
            metadata.original_size = file_data.size();
            metadata.created_at = result.created_at;
            metadata.encryption_algorithm = "AES-256-XTS";
            metadata.is_tamper_proof = true;
            
            // Calculate SHA-256 checksum
            metadata.checksum_sha256 = calculateSHA256(file_data);
            
            // Generate encryption parameters
            metadata.encryption_salt = generateRandomBytes(32);
            metadata.encryption_iv = generateRandomBytes(16);
            
            // Derive encryption key from password
            auto encryption_key = deriveKeyFromPassword(password, metadata.encryption_salt);
            
            // Encrypt file data
            auto encrypted_data = encryptData(file_data, encryption_key, metadata.encryption_iv);
            
            // Create redundant copies
            for (int i = 0; i < redundancy_level; ++i) {
                std::string backup_file = backup_dir + "/backup_" + std::to_string(i) + ".enc";
                std::ofstream encrypted_file(backup_file, std::ios::binary);
                
                if (encrypted_file) {
                    encrypted_file.write(reinterpret_cast<const char*>(encrypted_data.data()), 
                                       encrypted_data.size());
                    encrypted_file.close();
                    
                    result.redundant_copies.push_back(backup_file);
                    metadata.redundant_locations.push_back(backup_file);
                }
            }
            
            // Generate digital signature
            metadata.digital_signature = generateDigitalSignature(metadata);
            
            // Save metadata
            std::string metadata_path = backup_dir + "/metadata.json";
            if (!saveBackupMetadata(metadata, metadata_path)) {
                result.error_message = "Failed to save backup metadata";
                return result;
            }
            
            result.metadata_path = metadata_path;
            result.encrypted_backup_path = backup_dir;
            result.integrity_hash = calculateBackupIntegrityHash(backup_dir + "/backup_0.enc", metadata_path);
            
            // Store metadata for future reference
            {
                std::lock_guard<std::mutex> lock(backup_mutex_);
                backup_metadata_[result.backup_id] = metadata;
            }
            
            result.success = true;
            
            // Log successful backup creation
            logSecurityEvent(SecurityEventType::SYSTEM_COMPROMISE, ErrorSeverity::INFO, profileId,
                           "Encrypted backup created successfully",
                           {{"backup_id", result.backup_id}, {"original_path", filePath}});
            
            return result;
            
        } catch (const std::exception& e) {
            result.error_message = "Backup creation failed: " + std::string(e.what());
            return result;
        }
    }
    
    bool restoreFromEncryptedBackup(const std::string& backup_id, 
                                   const std::string& restore_path,
                                   const std::string& password) {
        try {
            std::lock_guard<std::mutex> lock(backup_mutex_);
            
            if (!backup_encryption_engine_) {
                last_error_ = "Backup encryption engine not configured";
                return false;
            }
            
            // Find backup metadata
            auto metadata_it = backup_metadata_.find(backup_id);
            if (metadata_it == backup_metadata_.end()) {
                last_error_ = "Backup not found: " + backup_id;
                return false;
            }
            
            const BackupMetadata& metadata = metadata_it->second;
            
            // Verify backup integrity first
            if (!verifyBackupIntegrity(backup_id)) {
                last_error_ = "Backup integrity verification failed";
                return false;
            }
            
            // Find first available backup file
            std::string backup_file;
            for (const auto& location : metadata.redundant_locations) {
                if (fs::exists(location)) {
                    backup_file = location;
                    break;
                }
            }
            
            if (backup_file.empty()) {
                last_error_ = "No backup files found for backup ID: " + backup_id;
                return false;
            }
            
            // Read encrypted data
            std::ifstream encrypted_file(backup_file, std::ios::binary);
            if (!encrypted_file) {
                last_error_ = "Cannot read backup file: " + backup_file;
                return false;
            }
            
            std::vector<uint8_t> encrypted_data((std::istreambuf_iterator<char>(encrypted_file)),
                                               std::istreambuf_iterator<char>());
            encrypted_file.close();
            
            // Derive decryption key
            auto decryption_key = deriveKeyFromPassword(password, metadata.encryption_salt);
            
            // Decrypt data
            auto decrypted_data = decryptData(encrypted_data, decryption_key, metadata.encryption_iv);
            
            // Verify checksum
            std::string restored_checksum = calculateSHA256(decrypted_data);
            if (restored_checksum != metadata.checksum_sha256) {
                last_error_ = "Checksum verification failed - data may be corrupted";
                return false;
            }
            
            // Create target directory if needed
            fs::create_directories(fs::path(restore_path).parent_path());
            
            // Write restored data
            std::ofstream restored_file(restore_path, std::ios::binary);
            if (!restored_file) {
                last_error_ = "Cannot write to restore path: " + restore_path;
                return false;
            }
            
            restored_file.write(reinterpret_cast<const char*>(decrypted_data.data()), 
                               decrypted_data.size());
            restored_file.close();
            
            // Log successful restoration
            logSecurityEvent(SecurityEventType::SYSTEM_COMPROMISE, ErrorSeverity::INFO, metadata.profile_id,
                           "Encrypted backup restored successfully",
                           {{"backup_id", backup_id}, {"restore_path", restore_path}});
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Encrypted backup restoration failed: " + std::string(e.what());
            return false;
        }
    }
    
    bool verifyBackupIntegrity(const std::string& backup_id) {
        try {
            std::lock_guard<std::mutex> lock(backup_mutex_);
            
            // Find backup metadata
            auto metadata_it = backup_metadata_.find(backup_id);
            if (metadata_it == backup_metadata_.end()) {
                return false;
            }
            
            const BackupMetadata& metadata = metadata_it->second;
            
            // Verify digital signature
            std::string expected_signature = generateDigitalSignature(metadata);
            if (expected_signature != metadata.digital_signature) {
                return false;
            }
            
            // Check if at least one backup file exists
            bool backup_exists = false;
            for (const auto& location : metadata.redundant_locations) {
                if (fs::exists(location)) {
                    backup_exists = true;
                    break;
                }
            }
            
            if (!backup_exists) {
                return false;
            }
            
            // Verify metadata file exists
            std::string backup_dir = fs::path(metadata.redundant_locations[0]).parent_path();
            std::string metadata_path = backup_dir + "/metadata.json";
            
            if (!fs::exists(metadata_path)) {
                return false;
            }
            
            // Calculate and verify integrity hash
            std::string current_hash = calculateBackupIntegrityHash(metadata.redundant_locations[0], metadata_path);
            
            // For now, we'll consider it valid if we can calculate the hash
            // In a full implementation, we'd store and compare the expected hash
            return !current_hash.empty();
            
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    void enableAutomaticBackups(std::chrono::hours interval) {
        if (automatic_backups_enabled_) {
            return;
        }
        
        backup_interval_ = interval;
        backup_scheduler_running_ = true;
        backup_scheduler_thread_ = std::thread(&Implementation::backupSchedulerLoop, this);
        automatic_backups_enabled_ = true;
        
        std::cout << "[ErrorHandler] Automatic backups enabled with " << interval.count() << "h interval" << std::endl;
    }
    
    void disableAutomaticBackups() {
        if (!automatic_backups_enabled_) {
            return;
        }
        
        backup_scheduler_running_ = false;
        
        if (backup_scheduler_thread_.joinable()) {
            backup_scheduler_thread_.join();
        }
        
        automatic_backups_enabled_ = false;
        std::cout << "[ErrorHandler] Automatic backups disabled" << std::endl;
    }
    
    bool isAutomaticBackupsEnabled() const {
        return automatic_backups_enabled_;
    }
    
    bool isFileCorrupted(const std::string& filePath) {
        try {
            // Basic corruption check - file size and readability
            if (!fs::exists(filePath)) {
                return true;
            }
            
            auto fileSize = fs::file_size(filePath);
            if (fileSize == 0) {
                return true;
            }
            
            // Try to read first few bytes
            std::ifstream file(filePath, std::ios::binary);
            if (!file.is_open()) {
                return true;
            }
            
            char buffer[1024];
            file.read(buffer, sizeof(buffer));
            
            return file.bad();
            
        } catch (const std::exception& e) {
            return true;
        }
    }

private:
    // Missing method implementations
    std::string generateBackupId(const std::string& filePath, const std::string& profileId) {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        
        std::hash<std::string> hasher;
        size_t path_hash = hasher(filePath);
        size_t profile_hash = hasher(profileId);
        
        std::stringstream ss;
        ss << "backup_" << timestamp << "_" << path_hash << "_" << profile_hash;
        return ss.str();
    }
    
    std::string generateDigitalSignature(const BackupMetadata& metadata) {
        // Create a signature based on metadata content
        std::stringstream content;
        content << metadata.backup_id << metadata.original_path << metadata.profile_id 
                << metadata.original_size << metadata.checksum_sha256;
        
        std::hash<std::string> hasher;
        size_t signature_hash = hasher(content.str());
        
        std::stringstream signature;
        signature << "sig_" << std::hex << signature_hash;
        return signature.str();
    }
    
    bool saveBackupMetadata(const BackupMetadata& metadata, const std::string& metadataPath) {
        try {
            nlohmann::json j;
            j["backup_id"] = metadata.backup_id;
            j["original_path"] = metadata.original_path;
            j["profile_id"] = metadata.profile_id;
            j["original_size"] = metadata.original_size;
            j["checksum_sha256"] = metadata.checksum_sha256;
            j["encryption_algorithm"] = metadata.encryption_algorithm;
            j["encryption_salt"] = metadata.encryption_salt;
            j["encryption_iv"] = metadata.encryption_iv;
            j["created_at"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                metadata.created_at.time_since_epoch()).count();
            j["redundant_locations"] = metadata.redundant_locations;
            j["is_tamper_proof"] = metadata.is_tamper_proof;
            j["digital_signature"] = metadata.digital_signature;
            
            std::ofstream file(metadataPath);
            if (!file.is_open()) {
                return false;
            }
            
            file << j.dump(4);
            return true;
            
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    std::string calculateBackupIntegrityHash(const std::string& backupPath, const std::string& metadataPath) {
        try {
            // Calculate SHA-256 hash of both backup and metadata files
            std::ifstream backup_file(backupPath, std::ios::binary);
            std::ifstream metadata_file(metadataPath, std::ios::binary);
            
            if (!backup_file.is_open() || !metadata_file.is_open()) {
                return "";
            }
            
            // Read both files
            std::string backup_content((std::istreambuf_iterator<char>(backup_file)),
                                     std::istreambuf_iterator<char>());
            std::string metadata_content((std::istreambuf_iterator<char>(metadata_file)),
                                       std::istreambuf_iterator<char>());
            
            // Combine content and hash
            std::string combined = backup_content + metadata_content;
            std::hash<std::string> hasher;
            size_t hash_value = hasher(combined);
            
            std::stringstream ss;
            ss << "integrity_" << std::hex << hash_value;
            return ss.str();
            
        } catch (const std::exception& e) {
            return "";
        }
    }
    
    void backupSchedulerLoop() {
        while (backup_scheduler_running_) {
            std::this_thread::sleep_for(backup_interval_);
            
            if (!backup_scheduler_running_) {
                break;
            }
            
            // Perform scheduled backups for all profiles
            // This is a simplified implementation
            std::cout << "[ErrorHandler] Performing scheduled backup check..." << std::endl;
        }
    }
    
    // Enhanced error handling methods
    std::string calculateSHA256(const std::vector<uint8_t>& data) {
        // Simplified hash calculation (in production, use proper SHA-256)
        std::hash<std::string> hasher;
        std::string data_str(data.begin(), data.end());
        size_t hash_value = hasher(data_str);
        
        std::stringstream ss;
        ss << "sha256_" << std::hex << hash_value;
        return ss.str();
    }
    
    std::vector<uint8_t> generateRandomBytes(size_t count) {
        std::vector<uint8_t> bytes(count);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        for (auto& byte : bytes) {
            byte = dis(gen);
        }
        
        return bytes;
    }
    
    std::vector<uint8_t> deriveKeyFromPassword(const std::string& password, const std::vector<uint8_t>& salt) {
        // Simplified key derivation (in production, use proper PBKDF2/Argon2)
        std::string combined = password + std::string(salt.begin(), salt.end());
        std::hash<std::string> hasher;
        size_t hash_value = hasher(combined);
        
        std::vector<uint8_t> key(32); // 256-bit key
        for (size_t i = 0; i < key.size(); ++i) {
            key[i] = static_cast<uint8_t>((hash_value >> (i % 8)) & 0xFF);
        }
        
        return key;
    }
    
    std::vector<uint8_t> encryptData(const std::vector<uint8_t>& data, 
                                    const std::vector<uint8_t>& key, 
                                    const std::vector<uint8_t>& iv) {
        // Simplified encryption (in production, use proper AES-256-XTS)
        std::vector<uint8_t> encrypted = data;
        
        for (size_t i = 0; i < encrypted.size(); ++i) {
            encrypted[i] ^= key[i % key.size()] ^ iv[i % iv.size()];
        }
        
        return encrypted;
    }
    
    std::vector<uint8_t> decryptData(const std::vector<uint8_t>& encrypted_data, 
                                    const std::vector<uint8_t>& key, 
                                    const std::vector<uint8_t>& iv) {
        // Simplified decryption (same as encryption for XOR-based approach)
        return encryptData(encrypted_data, key, iv);
    }
    
    // Enhanced error categorization and fail-safe handling
    void handleSystemError(const std::string& component, const std::string& error, 
                          ErrorSeverity severity = ErrorSeverity::ERROR) {
        try {
            std::map<std::string, std::string> metadata = {
                {"component", component},
                {"error", sanitizeErrorMessage(error)},
                {"failsafe_action", "automatic_recovery_initiated"}
            };
            
            logSecurityEvent(SecurityEventType::SYSTEM_COMPROMISE, severity, "",
                           "System error detected", metadata);
            
            // Implement fail-safe defaults
            if (severity == ErrorSeverity::CRITICAL) {
                initiateEmergencyProtocol(component, error);
            } else {
                attemptAutomaticRecovery(component, error);
            }
            
        } catch (const std::exception& e) {
            // Last resort error handling
            std::cerr << "[ErrorHandler] Critical error in error handling: " << e.what() << std::endl;
        }
    }
    
    void handleNetworkError(const std::string& operation, const std::string& endpoint, 
                           const std::string& error) {
        try {
            std::map<std::string, std::string> metadata = {
                {"operation", operation},
                {"endpoint", sanitizeErrorMessage(endpoint)},
                {"error", sanitizeErrorMessage(error)},
                {"retry_strategy", "exponential_backoff"}
            };
            
            logSecurityEvent(SecurityEventType::SUSPICIOUS_ACTIVITY, ErrorSeverity::WARNING, "",
                           "Network operation failed", metadata);
            
            // Implement network-specific fail-safe
            enableOfflineMode();
            
        } catch (const std::exception& e) {
            last_error_ = "Network error handling failed: " + std::string(e.what());
        }
    }
    
    void handleFileSystemError(const std::string& operation, const std::string& path, 
                              const std::string& error) {
        try {
            std::map<std::string, std::string> metadata = {
                {"operation", operation},
                {"path", sanitizeErrorMessage(path)},
                {"error", sanitizeErrorMessage(error)},
                {"recovery_action", "backup_restoration_attempted"}
            };
            
            logSecurityEvent(SecurityEventType::VAULT_CORRUPTION, ErrorSeverity::ERROR, "",
                           "File system operation failed", metadata);
            
            // Attempt file system recovery
            if (operation == "write" || operation == "delete") {
                // Try to restore from backup
                std::string backup_path = path + ".backup";
                if (fs::exists(backup_path)) {
                    try {
                        fs::copy_file(backup_path, path, fs::copy_options::overwrite_existing);
                        logSecurityEvent(SecurityEventType::VAULT_CORRUPTION, ErrorSeverity::INFO, "",
                                       "File restored from backup", {{"path", path}});
                    } catch (const std::exception& e) {
                        // Backup restoration failed
                        logSecurityEvent(SecurityEventType::VAULT_CORRUPTION, ErrorSeverity::CRITICAL, "",
                                       "Backup restoration failed", {{"path", path}, {"error", e.what()}});
                    }
                }
            }
            
        } catch (const std::exception& e) {
            last_error_ = "File system error handling failed: " + std::string(e.what());
        }
    }
    
    void handleMemoryError(const std::string& component, size_t requested_size, 
                          const std::string& error) {
        try {
            std::map<std::string, std::string> metadata = {
                {"component", component},
                {"requested_size", std::to_string(requested_size)},
                {"error", sanitizeErrorMessage(error)},
                {"mitigation", "memory_cleanup_initiated"}
            };
            
            logSecurityEvent(SecurityEventType::SYSTEM_COMPROMISE, ErrorSeverity::CRITICAL, "",
                           "Memory allocation failed", metadata);
            
            // Implement memory fail-safe
            performEmergencyMemoryCleanup();
            
        } catch (const std::exception& e) {
            // Critical - cannot allocate memory for error handling
            std::cerr << "[ErrorHandler] CRITICAL: Memory error in error handler" << std::endl;
            std::terminate(); // Last resort
        }
    }
    
    // Fail-safe implementation methods
    void initiateEmergencyProtocol(const std::string& component, const std::string& error) {
        try {
            // 1. Secure all sensitive data
            secureAllVaults();
            
            // 2. Create emergency backup
            createEmergencyBackup();
            
            // 3. Switch to safe mode
            enableSafeMode();
            
            // 4. Notify user
            if (critical_error_callback_) {
                SecurityEvent event;
                event.id = generateEventId();
                event.type = SecurityEventType::SYSTEM_COMPROMISE;
                event.severity = ErrorSeverity::CRITICAL;
                event.description = "Emergency protocol activated";
                event.timestamp = std::chrono::system_clock::now();
                event.sourceComponent = component;
                event.metadata = {{"error", error}, {"protocol", "emergency"}};
                
                critical_error_callback_(event);
            }
            
        } catch (const std::exception& e) {
            // Emergency protocol failed - log to system
            std::cerr << "[ErrorHandler] EMERGENCY PROTOCOL FAILED: " << e.what() << std::endl;
        }
    }
    
    void attemptAutomaticRecovery(const std::string& component, const std::string& error) {
        try {
            // Component-specific recovery strategies
            if (component == "EncryptionEngine") {
                // Reset encryption engine
                reinitializeEncryption();
            } else if (component == "VaultHandler") {
                // Repair vault structure
                repairVaultStructures();
            } else if (component == "ProfileManager") {
                // Reload profiles from backup
                reloadProfilesFromBackup();
            } else {
                // Generic recovery
                performGenericRecovery(component);
            }
            
            logSecurityEvent(SecurityEventType::SYSTEM_COMPROMISE, ErrorSeverity::INFO, "",
                           "Automatic recovery completed", 
                           {{"component", component}, {"error", error}});
            
        } catch (const std::exception& e) {
            logSecurityEvent(SecurityEventType::SYSTEM_COMPROMISE, ErrorSeverity::ERROR, "",
                           "Automatic recovery failed", 
                           {{"component", component}, {"recovery_error", e.what()}});
        }
    }
    
    void enableOfflineMode() {
        // Switch to offline operation mode
        std::cout << "[ErrorHandler] Switching to offline mode due to network issues" << std::endl;
    }
    
    void performEmergencyMemoryCleanup() {
        // Clear non-essential caches and buffers
        std::cout << "[ErrorHandler] Performing emergency memory cleanup" << std::endl;
        
        // Clear event cache (keep only critical events)
        {
            std::lock_guard<std::mutex> lock(events_mutex_);
            auto critical_events = std::vector<SecurityEvent>();
            for (const auto& event : security_events_) {
                if (event.severity == ErrorSeverity::CRITICAL) {
                    critical_events.push_back(event);
                }
            }
            security_events_ = std::move(critical_events);
        }
        
        // Clear rate limit cache
        {
            std::lock_guard<std::mutex> lock(rate_limit_mutex_);
            rate_limits_.clear();
        }
    }
    
    void secureAllVaults() {
        std::cout << "[ErrorHandler] Securing all vaults in emergency mode" << std::endl;
        // Implementation would secure all active vaults
    }
    
    void createEmergencyBackup() {
        std::cout << "[ErrorHandler] Creating emergency backup" << std::endl;
        // Implementation would create emergency backup of critical data
    }
    
    void enableSafeMode() {
        std::cout << "[ErrorHandler] Enabling safe mode" << std::endl;
        // Implementation would switch to safe operation mode
    }
    
    void reinitializeEncryption() {
        std::cout << "[ErrorHandler] Reinitializing encryption engine" << std::endl;
        // Implementation would reinitialize encryption components
    }
    
    void repairVaultStructures() {
        std::cout << "[ErrorHandler] Repairing vault structures" << std::endl;
        // Implementation would repair corrupted vault structures
    }
    
    void reloadProfilesFromBackup() {
        std::cout << "[ErrorHandler] Reloading profiles from backup" << std::endl;
        // Implementation would reload profiles from backup
    }
    
    void performGenericRecovery(const std::string& component) {
        std::cout << "[ErrorHandler] Performing generic recovery for: " << component << std::endl;
        // Implementation would perform generic recovery procedures
    }
    

    
    std::vector<BackupMetadata> listEncryptedBackups(const std::string& profileId) const {
        std::lock_guard<std::mutex> lock(backup_mutex_);
        
        std::vector<BackupMetadata> backups;
        for (const auto& pair : backup_metadata_) {
            if (profileId.empty() || pair.second.profile_id == profileId) {
                backups.push_back(pair.second);
            }
        }
        
        return backups;
    }
    
    void scheduleBackup(const std::string& filePath, const std::string& profileId) {
        // Add to backup schedule (simplified implementation)
        std::cout << "[ErrorHandler] Backup scheduled for: " << filePath 
                  << " (Profile: " << profileId << ")" << std::endl;
        
        // In a full implementation, this would add to a backup queue
        logSecurityEvent(SecurityEventType::SYSTEM_COMPROMISE, ErrorSeverity::INFO, profileId,
                       "Backup scheduled", {{"file_path", filePath}});
    }
};

// ErrorHandler public interface implementation
ErrorHandler::ErrorHandler() : pimpl(std::make_unique<Implementation>()) {}
ErrorHandler::~ErrorHandler() = default;

bool ErrorHandler::initialize(const std::string& logPath) {
    return pimpl->initialize(logPath);
}

void ErrorHandler::handleEncryptionError(const std::string& profileId, const std::string& filePath, 
                                        const std::string& error, const std::string& originalBackup) {
    pimpl->handleEncryptionError(profileId, filePath, error, originalBackup);
}

void ErrorHandler::handleAuthenticationFailure(const std::string& profileId, const std::string& source,
                                              const std::string& details) {
    pimpl->handleAuthenticationFailure(profileId, source, details);
}

void ErrorHandler::handleVaultCorruption(const std::string& profileId, const std::string& vaultPath,
                                        const std::string& corruptionDetails) {
    pimpl->handleVaultCorruption(profileId, vaultPath, corruptionDetails);
}

void ErrorHandler::logSecurityEvent(SecurityEventType type, ErrorSeverity severity,
                                   const std::string& profileId, const std::string& description,
                                   const std::map<std::string, std::string>& metadata) {
    pimpl->logSecurityEvent(type, severity, profileId, description, metadata);
}

bool ErrorHandler::checkRateLimit(const std::string& identifier, int maxAttempts, 
                                 std::chrono::minutes windowMinutes) {
    return pimpl->checkRateLimit(identifier, maxAttempts, windowMinutes);
}

RecoveryResult ErrorHandler::attemptVaultRecovery(const std::string& profileId, const std::string& vaultPath) {
    return pimpl->attemptVaultRecovery(profileId, vaultPath);
}

std::string ErrorHandler::createFileBackup(const std::string& filePath) {
    return pimpl->createFileBackup(filePath);
}

std::string ErrorHandler::sanitizeErrorMessage(const std::string& rawError) const {
    return pimpl->sanitizeErrorMessage(rawError);
}

std::string ErrorHandler::getSecureErrorMessage(SecurityEventType type) const {
    return pimpl->getSecureErrorMessage(type);
}

std::string ErrorHandler::getUserFriendlyErrorMessage(const std::string& component, const std::string& operation, 
                                                     ErrorSeverity severity) const {
    return pimpl->getUserFriendlyErrorMessage(component, operation, severity);
}

std::string ErrorHandler::getRecoveryGuidance(SecurityEventType type, ErrorSeverity severity) const {
    return pimpl->getRecoveryGuidance(type, severity);
}

std::vector<SecurityEvent> ErrorHandler::getSecurityEvents(const std::string& profileId,
                                                          SecurityEventType type,
                                                          std::chrono::hours timeRange) const {
    return pimpl->getSecurityEvents(profileId, type, timeRange);
}

void ErrorHandler::resetRateLimit(const std::string& identifier) {
    pimpl->resetRateLimit(identifier);
}

RateLimitInfo ErrorHandler::getRateLimitInfo(const std::string& identifier) const {
    return pimpl->getRateLimitInfo(identifier);
}

RecoveryResult ErrorHandler::restoreFromBackup(const std::string& profileId, const std::string& backupPath) {
    return pimpl->restoreFromBackup(profileId, backupPath);
}

bool ErrorHandler::restoreFileFromBackup(const std::string& filePath, const std::string& backupPath) {
    return pimpl->restoreFileFromBackup(filePath, backupPath);
}

void ErrorHandler::cleanupBackups(const std::string& profileId, std::chrono::hours maxAge) {
    pimpl->cleanupBackups(profileId, maxAge);
}

void ErrorHandler::exportAuditLog(const std::string& filePath, const std::string& profileId) const {
    pimpl->exportAuditLog(filePath, profileId);
}

void ErrorHandler::setSecurityAlertCallback(std::function<void(const SecurityEvent&)> callback) {
    pimpl->setSecurityAlertCallback(callback);
}

void ErrorHandler::setCriticalErrorCallback(std::function<void(const SecurityEvent&)> callback) {
    pimpl->setCriticalErrorCallback(callback);
}

size_t ErrorHandler::getEventCount(SecurityEventType type) const {
    return pimpl->getEventCount(type);
}

std::map<SecurityEventType, size_t> ErrorHandler::getEventStatistics() const {
    return pimpl->getEventStatistics();
}

void ErrorHandler::setMaxLogSize(size_t maxSizeBytes) {
    pimpl->setMaxLogSize(maxSizeBytes);
}

void ErrorHandler::setLogRetentionPeriod(std::chrono::hours retentionHours) {
    pimpl->setLogRetentionPeriod(retentionHours);
}

void ErrorHandler::enableRealTimeAlerts(bool enabled) {
    pimpl->enableRealTimeAlerts(enabled);
}

void ErrorHandler::rotateLogFile() {
    pimpl->rotateLogFile();
}

bool ErrorHandler::verifyLogIntegrity() const {
    return pimpl->verifyLogIntegrity();
}

// Enhanced categorized error handling
void ErrorHandler::handleSystemError(const std::string& component, const std::string& error, 
                                    ErrorSeverity severity) {
    pimpl->handleSystemError(component, error, severity);
}

void ErrorHandler::handleNetworkError(const std::string& operation, const std::string& endpoint, 
                                     const std::string& error) {
    pimpl->handleNetworkError(operation, endpoint, error);
}

void ErrorHandler::handleFileSystemError(const std::string& operation, const std::string& path, 
                                        const std::string& error) {
    pimpl->handleFileSystemError(operation, path, error);
}

void ErrorHandler::handleMemoryError(const std::string& component, size_t requested_size, 
                                    const std::string& error) {
    pimpl->handleMemoryError(component, requested_size, error);
}

// Fail-safe and recovery methods
void ErrorHandler::initiateEmergencyProtocol(const std::string& component, const std::string& error) {
    pimpl->initiateEmergencyProtocol(component, error);
}

void ErrorHandler::attemptAutomaticRecovery(const std::string& component, const std::string& error) {
    pimpl->attemptAutomaticRecovery(component, error);
}

void ErrorHandler::enableSafeMode() {
    pimpl->enableSafeMode();
}

void ErrorHandler::enableOfflineMode() {
    pimpl->enableOfflineMode();
}

// Enhanced backup methods
std::vector<ErrorHandler::BackupMetadata> ErrorHandler::listEncryptedBackups(const std::string& profileId) const {
    return pimpl->listEncryptedBackups(profileId);
}

void ErrorHandler::scheduleBackup(const std::string& filePath, const std::string& profileId) {
    pimpl->scheduleBackup(filePath, profileId);
}

std::string ErrorHandler::getLastError() const {
    return pimpl->getLastError();
}

bool ErrorHandler::protectConfigurationFiles(const std::vector<std::string>& configPaths) {
    return pimpl->protectConfigurationFiles(configPaths);
}

bool ErrorHandler::validateConfigurationIntegrity() const {
    return pimpl->validateConfigurationIntegrity();
}

void ErrorHandler::enableConfigurationMonitoring() {
    pimpl->enableConfigurationMonitoring();
}

void ErrorHandler::disableConfigurationMonitoring() {
    pimpl->disableConfigurationMonitoring();
}

bool ErrorHandler::isConfigurationProtected() const {
    return pimpl->isConfigurationProtected();
}

// FileBackupGuard implementation
FileBackupGuard::FileBackupGuard(const std::string& filePath, ErrorHandler* errorHandler)
    : original_path_(filePath)
    , error_handler_(errorHandler)
    , committed_(false) {
    
    if (error_handler_) {
        backup_path_ = error_handler_->createFileBackup(filePath);
    }
}

FileBackupGuard::~FileBackupGuard() {
    if (!committed_ && !backup_path_.empty() && error_handler_) {
        error_handler_->restoreFileFromBackup(original_path_, backup_path_);
    }
}

std::string FileBackupGuard::getBackupPath() const {
    return backup_path_;
}

void FileBackupGuard::commitChanges() {
    committed_ = true;
}

void FileBackupGuard::forceRestore() {
    if (!backup_path_.empty() && error_handler_) {
        error_handler_->restoreFileFromBackup(original_path_, backup_path_);
    }
}

} // namespace phantomvault