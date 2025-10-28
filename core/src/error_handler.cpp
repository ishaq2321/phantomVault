/**
 * PhantomVault Error Handler Implementation
 * 
 * Comprehensive error handling, security event logging, and recovery mechanisms.
 */

#include "error_handler.hpp"
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
                return "Authentication failed. Please verify your credentials.";
            case SecurityEventType::ENCRYPTION_FAILURE:
                return "Encryption operation failed. Please try again.";
            case SecurityEventType::VAULT_CORRUPTION:
                return "Vault integrity issue detected. Recovery procedures initiated.";
            case SecurityEventType::UNAUTHORIZED_ACCESS:
                return "Access denied. Insufficient privileges.";
            case SecurityEventType::RATE_LIMIT_EXCEEDED:
                return "Too many attempts. Please wait before trying again.";
            case SecurityEventType::SUSPICIOUS_ACTIVITY:
                return "Suspicious activity detected. Security measures activated.";
            default:
                return "An error occurred. Please contact support if the issue persists.";
        }
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
            std::ofstream file(log_path_, std::ios::app);
            
            // Save only new events (simple approach - save all for now)
            for (const auto& event : security_events_) {
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
            
            file.close();
            
            // Set secure permissions
            fs::permissions(log_path_, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::replace);
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to save events: " + std::string(e.what());
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
        
        // Save remaining events
        if (initialized_) {
            saveEvents();
        }
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
    
    RecoveryResult restoreFromBackup(const std::string& profileId, const std::string& backupPath) {
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

std::string ErrorHandler::getLastError() const {
    return pimpl->getLastError();
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