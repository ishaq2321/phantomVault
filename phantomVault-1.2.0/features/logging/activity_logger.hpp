#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <chrono>
#include <functional>

namespace phantom_vault::logging {

/**
 * @brief Log levels
 */
enum class PHANTOM_VAULT_EXPORT LogLevel {
    Trace,      // Detailed trace information
    Debug,      // Debug information
    Info,       // General information
    Warning,    // Warning messages
    Error,      // Error messages
    Critical,   // Critical error messages
    Security    // Security-related messages
};

/**
 * @brief Activity types
 */
enum class PHANTOM_VAULT_EXPORT ActivityType {
    // Authentication activities
    UserLogin,
    UserLogout,
    PasswordChange,
    BiometricEnrollment,
    BiometricAuthentication,
    PasswordRecovery,
    
    // Vault activities
    VaultCreated,
    VaultDeleted,
    VaultOpened,
    VaultClosed,
    VaultLocked,
    VaultUnlocked,
    VaultShared,
    VaultUnshared,
    VaultBackedUp,
    VaultRestored,
    
    // File activities
    FileAdded,
    FileDeleted,
    FileModified,
    FileMoved,
    FileCopied,
    FileEncrypted,
    FileDecrypted,
    
    // System activities
    SystemStartup,
    SystemShutdown,
    ConfigurationChanged,
    UpdateInstalled,
    ErrorOccurred,
    SecurityAlert,
    
    // User activities
    UserCreated,
    UserDeleted,
    UserModified,
    PermissionGranted,
    PermissionRevoked,
    GroupCreated,
    GroupDeleted,
    
    // Emergency activities
    EmergencyLockdown,
    EmergencyRecovery,
    PanicButtonPressed,
    SecurityBreach,
    
    // Cloud activities
    CloudSyncStarted,
    CloudSyncCompleted,
    CloudSyncFailed,
    CloudBackupCreated,
    CloudBackupRestored,
    
    // Note activities
    NoteCreated,
    NoteModified,
    NoteDeleted,
    NoteShared,
    NoteExported,
    NoteImported
};

/**
 * @brief Activity log entry
 */
struct PHANTOM_VAULT_EXPORT ActivityLogEntry {
    std::string id;                         // Log entry identifier
    ActivityType type;                      // Activity type
    LogLevel level;                         // Log level
    std::string userId;                     // User identifier
    std::string sessionId;                  // Session identifier
    std::string deviceId;                   // Device identifier
    std::string ipAddress;                  // IP address
    std::string description;                // Activity description
    std::map<std::string, std::string> details; // Additional details
    std::chrono::system_clock::time_point timestamp;
    std::string source;                     // Source component
    std::string category;                   // Log category
    bool isSensitive;                       // Contains sensitive data
    std::string checksum;                   // Entry checksum
    size_t size;                           // Entry size in bytes
};

/**
 * @brief Log filter criteria
 */
struct PHANTOM_VAULT_EXPORT LogFilter {
    std::vector<ActivityType> types;        // Filter by activity types
    std::vector<LogLevel> levels;           // Filter by log levels
    std::string userId;                     // Filter by user
    std::string sessionId;                  // Filter by session
    std::string deviceId;                   // Filter by device
    std::string source;                     // Filter by source
    std::string category;                   // Filter by category
    std::chrono::system_clock::time_point fromTime;
    std::chrono::system_clock::time_point toTime;
    bool includeSensitive;                  // Include sensitive entries
    int limit;                             // Result limit
    int offset;                            // Result offset
};

/**
 * @brief Log statistics
 */
struct PHANTOM_VAULT_EXPORT LogStatistics {
    size_t totalEntries;                    // Total log entries
    size_t entriesByLevel[7];              // Entries by log level
    size_t entriesByType[50];              // Entries by activity type
    std::chrono::system_clock::time_point oldestEntry;
    std::chrono::system_clock::time_point newestEntry;
    size_t totalSize;                       // Total log size in bytes
    size_t averageEntrySize;               // Average entry size
    std::map<std::string, size_t> entriesByUser;
    std::map<std::string, size_t> entriesByDevice;
    std::map<std::string, size_t> entriesBySource;
};

/**
 * @brief Activity logger interface
 */
class PHANTOM_VAULT_EXPORT ActivityLogger {
public:
    virtual ~ActivityLogger() = default;
    
    // Logging operations
    virtual bool logActivity(const ActivityLogEntry& entry) = 0;
    virtual bool logActivity(ActivityType type, LogLevel level, const std::string& userId,
                            const std::string& description, const std::map<std::string, std::string>& details = {}) = 0;
    virtual bool logSecurityEvent(const std::string& userId, const std::string& event,
                                 const std::map<std::string, std::string>& details = {}) = 0;
    virtual bool logError(const std::string& userId, const std::string& error,
                         const std::map<std::string, std::string>& details = {}) = 0;
    
    // Query operations
    virtual std::vector<ActivityLogEntry> getLogs(const LogFilter& filter) = 0;
    virtual std::vector<ActivityLogEntry> getUserLogs(const std::string& userId, int limit = 100) = 0;
    virtual std::vector<ActivityLogEntry> getRecentLogs(int limit = 50) = 0;
    virtual std::vector<ActivityLogEntry> getSecurityLogs(int limit = 100) = 0;
    virtual std::vector<ActivityLogEntry> getErrorLogs(int limit = 100) = 0;
    
    // Statistics
    virtual LogStatistics getStatistics() const = 0;
    virtual LogStatistics getStatistics(const LogFilter& filter) const = 0;
    virtual std::map<std::string, size_t> getActivityCounts() const = 0;
    virtual std::map<std::string, size_t> getUserActivityCounts() const = 0;
    
    // Maintenance
    virtual bool clearLogs() = 0;
    virtual bool clearUserLogs(const std::string& userId) = 0;
    virtual bool clearOldLogs(int daysToKeep) = 0;
    virtual bool archiveLogs(const std::string& archivePath) = 0;
    virtual bool compressLogs() = 0;
    
    // Export/Import
    virtual bool exportLogs(const std::string& filePath, const LogFilter& filter) = 0;
    virtual bool importLogs(const std::string& filePath) = 0;
    virtual bool exportStatistics(const std::string& filePath) = 0;
    
    // Configuration
    virtual bool setLogLevel(LogLevel level) = 0;
    virtual LogLevel getLogLevel() const = 0;
    virtual bool setMaxLogSize(size_t maxSize) = 0;
    virtual size_t getMaxLogSize() const = 0;
    virtual bool setRetentionDays(int days) = 0;
    virtual int getRetentionDays() const = 0;
    
    // Event callbacks
    virtual void setLogAddedCallback(std::function<void(const ActivityLogEntry&)> callback) = 0;
    virtual void setSecurityAlertCallback(std::function<void(const ActivityLogEntry&)> callback) = 0;
    virtual void setErrorCallback(std::function<void(const std::string&)> callback) = 0;
};

/**
 * @brief Local activity logger implementation
 */
class PHANTOM_VAULT_EXPORT LocalActivityLogger : public ActivityLogger {
public:
    LocalActivityLogger();
    ~LocalActivityLogger() override;
    
    // Logging operations
    bool logActivity(const ActivityLogEntry& entry) override;
    bool logActivity(ActivityType type, LogLevel level, const std::string& userId,
                    const std::string& description, const std::map<std::string, std::string>& details = {}) override;
    bool logSecurityEvent(const std::string& userId, const std::string& event,
                         const std::map<std::string, std::string>& details = {}) override;
    bool logError(const std::string& userId, const std::string& error,
                 const std::map<std::string, std::string>& details = {}) override;
    
    // Query operations
    std::vector<ActivityLogEntry> getLogs(const LogFilter& filter) override;
    std::vector<ActivityLogEntry> getUserLogs(const std::string& userId, int limit = 100) override;
    std::vector<ActivityLogEntry> getRecentLogs(int limit = 50) override;
    std::vector<ActivityLogEntry> getSecurityLogs(int limit = 100) override;
    std::vector<ActivityLogEntry> getErrorLogs(int limit = 100) override;
    
    // Statistics
    LogStatistics getStatistics() const override;
    LogStatistics getStatistics(const LogFilter& filter) const override;
    std::map<std::string, size_t> getActivityCounts() const override;
    std::map<std::string, size_t> getUserActivityCounts() const override;
    
    // Maintenance
    bool clearLogs() override;
    bool clearUserLogs(const std::string& userId) override;
    bool clearOldLogs(int daysToKeep) override;
    bool archiveLogs(const std::string& archivePath) override;
    bool compressLogs() override;
    
    // Export/Import
    bool exportLogs(const std::string& filePath, const LogFilter& filter) override;
    bool importLogs(const std::string& filePath) override;
    bool exportStatistics(const std::string& filePath) override;
    
    // Configuration
    bool setLogLevel(LogLevel level) override;
    LogLevel getLogLevel() const override;
    bool setMaxLogSize(size_t maxSize) override;
    size_t getMaxLogSize() const override;
    bool setRetentionDays(int days) override;
    int getRetentionDays() const override;
    
    // Event callbacks
    void setLogAddedCallback(std::function<void(const ActivityLogEntry&)> callback) override;
    void setSecurityAlertCallback(std::function<void(const ActivityLogEntry&)> callback) override;
    void setErrorCallback(std::function<void(const std::string&)> callback) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Log analysis and monitoring
 */
class PHANTOM_VAULT_EXPORT LogAnalyzer {
public:
    virtual ~LogAnalyzer() = default;
    
    // Pattern analysis
    virtual std::vector<std::string> detectAnomalies(const LogFilter& filter) = 0;
    virtual std::vector<std::string> detectSecurityThreats(const LogFilter& filter) = 0;
    virtual std::vector<std::string> detectPerformanceIssues(const LogFilter& filter) = 0;
    virtual std::vector<std::string> detectUserBehaviorPatterns(const std::string& userId) = 0;
    
    // Trend analysis
    virtual std::map<std::string, int> getActivityTrends(const LogFilter& filter) = 0;
    virtual std::map<std::string, int> getErrorTrends(const LogFilter& filter) = 0;
    virtual std::map<std::string, int> getSecurityTrends(const LogFilter& filter) = 0;
    
    // Reporting
    virtual std::string generateSecurityReport(const LogFilter& filter) = 0;
    virtual std::string generateActivityReport(const LogFilter& filter) = 0;
    virtual std::string generateErrorReport(const LogFilter& filter) = 0;
    virtual std::string generateComplianceReport(const LogFilter& filter) = 0;
    
    // Real-time monitoring
    virtual bool startRealTimeMonitoring() = 0;
    virtual bool stopRealTimeMonitoring() = 0;
    virtual bool isRealTimeMonitoring() const = 0;
    
    // Event callbacks
    virtual void setAnomalyDetectedCallback(std::function<void(const std::string&)> callback) = 0;
    virtual void setSecurityThreatDetectedCallback(std::function<void(const std::string&)> callback) = 0;
    virtual void setPerformanceIssueDetectedCallback(std::function<void(const std::string&)> callback) = 0;
};

/**
 * @brief Log encryption and security
 */
class PHANTOM_VAULT_EXPORT LogSecurity {
public:
    // Log encryption
    static std::string encryptLogEntry(const ActivityLogEntry& entry, const std::string& key);
    static ActivityLogEntry decryptLogEntry(const std::string& encryptedData, const std::string& key);
    
    // Secure storage
    static bool storeLogSecurely(const ActivityLogEntry& entry);
    static ActivityLogEntry retrieveLogSecurely(const std::string& logId);
    static bool deleteLogSecurely(const std::string& logId);
    
    // Integrity verification
    static std::string calculateLogChecksum(const ActivityLogEntry& entry);
    static bool verifyLogChecksum(const ActivityLogEntry& entry);
    
    // Sensitive data masking
    static ActivityLogEntry maskSensitiveData(const ActivityLogEntry& entry);
    static std::string maskSensitiveString(const std::string& str);
    
    // Audit trail
    static bool createAuditTrail(const std::string& action, const std::string& userId);
    static std::vector<std::string> getAuditTrail(const std::string& userId);
};

} // namespace phantom_vault::logging
