/**
 * PhantomVault Analytics Engine
 * 
 * Comprehensive usage statistics collection, security event logging,
 * and privacy-aware analytics with data retention policies.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <map>

namespace phantomvault {

/**
 * Types of events tracked by the analytics engine
 */
enum class EventType {
    PROFILE_CREATED,
    PROFILE_AUTHENTICATED,
    PROFILE_AUTH_FAILED,
    FOLDER_LOCKED,
    FOLDER_UNLOCKED_TEMPORARY,
    FOLDER_UNLOCKED_PERMANENT,
    KEYBOARD_SEQUENCE_DETECTED,
    PASSWORD_PATTERN_DETECTED,
    SERVICE_STARTED,
    SERVICE_STOPPED,
    SECURITY_VIOLATION,
    SYSTEM_ERROR
};

/**
 * Security levels for events
 */
enum class SecurityLevel {
    INFO,
    WARNING,
    CRITICAL
};

/**
 * Analytics event structure
 */
struct AnalyticsEvent {
    std::string id;
    EventType type;
    SecurityLevel level;
    std::string profileId;
    std::string description;
    std::map<std::string, std::string> metadata;
    std::chrono::system_clock::time_point timestamp;
    std::string source;
};

/**
 * Usage statistics structure
 */
struct UsageStatistics {
    size_t totalProfiles = 0;
    size_t totalFolders = 0;
    size_t totalUnlockAttempts = 0;
    size_t successfulUnlocks = 0;
    size_t failedUnlocks = 0;
    size_t keyboardSequenceDetections = 0;
    size_t securityViolations = 0;
    std::chrono::system_clock::time_point firstUse;
    std::chrono::system_clock::time_point lastActivity;
    std::chrono::duration<double> totalUptime;
};

/**
 * Time-based analytics query
 */
struct AnalyticsQuery {
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    std::vector<EventType> eventTypes;
    std::vector<SecurityLevel> securityLevels;
    std::string profileId; // Empty for all profiles
    size_t maxResults = 1000;
};

/**
 * Analytics report structure
 */
struct AnalyticsReport {
    UsageStatistics statistics;
    std::vector<AnalyticsEvent> events;
    std::map<EventType, size_t> eventCounts;
    std::map<std::string, size_t> profileActivity;
    std::string generatedAt;
};

class AnalyticsEngine {
public:
    AnalyticsEngine();
    ~AnalyticsEngine();

    // Initialization and lifecycle
    bool initialize(const std::string& dataPath = "");
    bool start();
    void stop();
    bool isRunning() const;
    
    // Event logging
    void logEvent(EventType type, SecurityLevel level, const std::string& profileId, 
                  const std::string& description, const std::map<std::string, std::string>& metadata = {});
    void logSecurityEvent(const std::string& profileId, const std::string& description, 
                         const std::map<std::string, std::string>& metadata = {});
    void logUsageEvent(EventType type, const std::string& profileId, 
                      const std::string& description = "");
    
    // Statistics and queries
    UsageStatistics getUsageStatistics() const;
    UsageStatistics getProfileStatistics(const std::string& profileId) const;
    std::vector<AnalyticsEvent> queryEvents(const AnalyticsQuery& query) const;
    AnalyticsReport generateReport(const AnalyticsQuery& query) const;
    
    // Data management
    void setRetentionPolicy(std::chrono::hours retentionPeriod);
    void cleanupOldData();
    void exportData(const std::string& filePath, const AnalyticsQuery& query) const;
    void clearAllData();
    void clearProfileData(const std::string& profileId);
    
    // Privacy and security
    void enableDataCollection(bool enabled);
    bool isDataCollectionEnabled() const;
    void anonymizeData();
    size_t getStorageSize() const;
    
    // Real-time monitoring
    void setEventCallback(std::function<void(const AnalyticsEvent&)> callback);
    void setSecurityAlertCallback(std::function<void(const AnalyticsEvent&)> callback);
    
    // Error handling
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

} // namespace phantomvault