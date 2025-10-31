#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <memory>
#include <functional>
#include <map>

namespace phantomvault {

// Forward declarations
namespace PhantomVault {
    class EncryptionEngine;
}

/**
 * @brief Error severity levels for security events
 */
enum class ErrorSeverity {
    INFO,       // Informational events
    WARNING,    // Warning events that may indicate issues
    ERROR,      // Error events that affect functionality
    CRITICAL    // Critical security events requiring immediate attention
};

/**
 * @brief Types of security events
 */
enum class SecurityEventType {
    AUTHENTICATION_FAILURE,
    ENCRYPTION_FAILURE,
    VAULT_CORRUPTION,
    UNAUTHORIZED_ACCESS,
    PRIVILEGE_ESCALATION,
    RATE_LIMIT_EXCEEDED,
    SUSPICIOUS_ACTIVITY,
    SYSTEM_COMPROMISE
};

/**
 * @brief Security event information
 */
struct SecurityEvent {
    std::string id;
    SecurityEventType type;
    ErrorSeverity severity;
    std::string profileId;
    std::string description;
    std::string details;
    std::chrono::system_clock::time_point timestamp;
    std::string sourceComponent;
    std::map<std::string, std::string> metadata;
    
    SecurityEvent() : timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Error recovery result
 */
struct RecoveryResult {
    bool success;
    std::string message;
    std::vector<std::string> recoveredFiles;
    std::vector<std::string> failedFiles;
    
    RecoveryResult() : success(false) {}
};

/**
 * @brief Rate limiting information
 */
struct RateLimitInfo {
    std::string identifier;
    int attemptCount;
    std::chrono::system_clock::time_point firstAttempt;
    std::chrono::system_clock::time_point lastAttempt;
    bool isBlocked;
    std::chrono::system_clock::time_point blockExpiry;
};

/**
 * @brief Comprehensive error handler for PhantomVault
 * 
 * Provides robust error handling, security event logging, rate limiting,
 * and recovery mechanisms for the encryption system.
 */
class ErrorHandler {
public:
    ErrorHandler();
    ~ErrorHandler();
    
    // Initialization
    bool initialize(const std::string& logPath = "");
    
    // Error handling
    void handleEncryptionError(const std::string& profileId, const std::string& filePath, 
                              const std::string& error, const std::string& originalBackup = "");
    void handleAuthenticationFailure(const std::string& profileId, const std::string& source,
                                   const std::string& details = "");
    void handleVaultCorruption(const std::string& profileId, const std::string& vaultPath,
                             const std::string& corruptionDetails);
    
    // Security event logging
    void logSecurityEvent(SecurityEventType type, ErrorSeverity severity,
                         const std::string& profileId, const std::string& description,
                         const std::map<std::string, std::string>& metadata = {});
    
    // Rate limiting
    bool checkRateLimit(const std::string& identifier, int maxAttempts = 5, 
                       std::chrono::minutes windowMinutes = std::chrono::minutes(15));
    void resetRateLimit(const std::string& identifier);
    RateLimitInfo getRateLimitInfo(const std::string& identifier) const;
    
    // Vault recovery
    RecoveryResult attemptVaultRecovery(const std::string& profileId, const std::string& vaultPath);
    RecoveryResult restoreFromBackup(const std::string& profileId, const std::string& backupPath);
    
    // File preservation
    std::string createFileBackup(const std::string& filePath);
    bool restoreFileFromBackup(const std::string& filePath, const std::string& backupPath);
    void cleanupBackups(const std::string& profileId, std::chrono::hours maxAge = std::chrono::hours(24));
    
    // Audit trail
    std::vector<SecurityEvent> getSecurityEvents(const std::string& profileId = "",
                                                SecurityEventType type = SecurityEventType::AUTHENTICATION_FAILURE,
                                                std::chrono::hours timeRange = std::chrono::hours(24)) const;
    void exportAuditLog(const std::string& filePath, const std::string& profileId = "") const;
    
    // Error message sanitization and user-friendly messages
    std::string sanitizeErrorMessage(const std::string& rawError) const;
    std::string getSecureErrorMessage(SecurityEventType type) const;
    std::string getUserFriendlyErrorMessage(const std::string& component, const std::string& operation, 
                                           ErrorSeverity severity) const;
    std::string getRecoveryGuidance(SecurityEventType type, ErrorSeverity severity) const;
    
    // Callbacks
    void setSecurityAlertCallback(std::function<void(const SecurityEvent&)> callback);
    void setCriticalErrorCallback(std::function<void(const SecurityEvent&)> callback);
    
    // Statistics
    size_t getEventCount(SecurityEventType type = SecurityEventType::AUTHENTICATION_FAILURE) const;
    std::map<SecurityEventType, size_t> getEventStatistics() const;
    
    // Configuration and log management
    void setMaxLogSize(size_t maxSizeBytes);
    void setLogRetentionPeriod(std::chrono::hours retentionHours);
    void enableRealTimeAlerts(bool enabled);
    void rotateLogFile();
    bool verifyLogIntegrity() const;
    
    // Configuration protection
    bool protectConfigurationFiles(const std::vector<std::string>& configPaths);
    bool validateConfigurationIntegrity() const;
    void enableConfigurationMonitoring();
    void disableConfigurationMonitoring();
    bool isConfigurationProtected() const;
    
    // Encrypted backup infrastructure
    struct EncryptedBackupResult {
        bool success;
        std::string backup_id;
        std::string encrypted_backup_path;
        std::string metadata_path;
        std::vector<std::string> redundant_copies;
        std::string integrity_hash;
        std::chrono::system_clock::time_point created_at;
        std::string error_message;
        
        EncryptedBackupResult() : success(false) {}
    };
    
    struct BackupMetadata {
        std::string backup_id;
        std::string original_path;
        std::string profile_id;
        size_t original_size;
        std::string checksum_sha256;
        std::string encryption_algorithm;
        std::vector<uint8_t> encryption_salt;
        std::vector<uint8_t> encryption_iv;
        std::chrono::system_clock::time_point created_at;
        std::vector<std::string> redundant_locations;
        bool is_tamper_proof;
        std::string digital_signature;
    };
    
    EncryptedBackupResult createEncryptedBackup(const std::string& filePath, 
                                               const std::string& profileId,
                                               const std::string& password,
                                               int redundancy_level = 3);
    bool restoreFromEncryptedBackup(const std::string& backup_id, 
                                   const std::string& restore_path,
                                   const std::string& password);
    bool verifyBackupIntegrity(const std::string& backup_id);
    std::vector<BackupMetadata> listEncryptedBackups(const std::string& profileId = "") const;
    
    // Automatic backup scheduling
    void enableAutomaticBackups(std::chrono::hours interval = std::chrono::hours(6));
    void disableAutomaticBackups();
    bool isAutomaticBackupsEnabled() const;
    void scheduleBackup(const std::string& filePath, const std::string& profileId);
    void setBackupEncryptionEngine(class EncryptionEngine* engine);
    
    // Enhanced categorized error handling
    void handleSystemError(const std::string& component, const std::string& error, 
                          ErrorSeverity severity = ErrorSeverity::ERROR);
    void handleNetworkError(const std::string& operation, const std::string& endpoint, 
                           const std::string& error);
    void handleFileSystemError(const std::string& operation, const std::string& path, 
                              const std::string& error);
    void handleMemoryError(const std::string& component, size_t requested_size, 
                          const std::string& error);
    
    // Fail-safe and recovery methods
    void initiateEmergencyProtocol(const std::string& component, const std::string& error);
    void attemptAutomaticRecovery(const std::string& component, const std::string& error);
    void enableSafeMode();
    void enableOfflineMode();
    

    
    // Error handling
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

/**
 * @brief RAII class for file backup and restoration
 */
class FileBackupGuard {
public:
    FileBackupGuard(const std::string& filePath, ErrorHandler* errorHandler);
    ~FileBackupGuard();
    
    std::string getBackupPath() const;
    void commitChanges(); // Don't restore on destruction
    void forceRestore();  // Force restoration even if committed
    
private:
    std::string original_path_;
    std::string backup_path_;
    ErrorHandler* error_handler_;
    bool committed_;
};

} // namespace phantomvault