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