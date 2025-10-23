#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <chrono>
#include <functional>

namespace phantom_vault::emergency {

/**
 * @brief Emergency lockdown trigger types
 */
enum class PHANTOM_VAULT_EXPORT LockdownTrigger {
    Manual,             // Manual activation
    PanicButton,        // Panic button pressed
    Timeout,            // Session timeout
    UnauthorizedAccess, // Unauthorized access detected
    Tampering,          // System tampering detected
    NetworkIntrusion,   // Network intrusion detected
    BiometricFailure,   // Biometric authentication failure
    MultipleFailures,   // Multiple authentication failures
    RemoteCommand,      // Remote lockdown command
    Scheduled           // Scheduled lockdown
};

/**
 * @brief Emergency lockdown levels
 */
enum class PHANTOM_VAULT_EXPORT LockdownLevel {
    Soft,       // Soft lockdown - lock vaults only
    Medium,     // Medium lockdown - lock vaults and clear sensitive data
    Hard,       // Hard lockdown - full system lockdown
    Nuclear     // Nuclear lockdown - wipe all data
};

/**
 * @brief Emergency lockdown status
 */
enum class PHANTOM_VAULT_EXPORT LockdownStatus {
    Inactive,       // No lockdown active
    Activating,     // Lockdown in progress
    Active,         // Lockdown active
    Deactivating,   // Lockdown being deactivated
    Error           // Lockdown error
};

/**
 * @brief Emergency lockdown configuration
 */
struct PHANTOM_VAULT_EXPORT LockdownConfig {
    bool enabled;                           // Emergency lockdown enabled
    std::set<LockdownTrigger> triggers;    // Enabled triggers
    LockdownLevel defaultLevel;            // Default lockdown level
    std::chrono::seconds activationDelay;  // Delay before activation
    std::chrono::seconds deactivationDelay; // Delay before deactivation
    bool requireConfirmation;              // Require confirmation for activation
    bool allowRemoteDeactivation;          // Allow remote deactivation
    std::string emergencyContact;          // Emergency contact information
    std::string recoveryCode;              // Recovery code for deactivation
    bool wipeOnNuclear;                    // Wipe data on nuclear lockdown
    bool notifyEmergencyContact;           // Notify emergency contact
    std::vector<std::string> protectedVaults; // Vaults to protect during lockdown
    std::vector<std::string> excludedVaults;  // Vaults to exclude from lockdown
};

/**
 * @brief Emergency lockdown event
 */
struct PHANTOM_VAULT_EXPORT LockdownEvent {
    std::string id;                        // Event identifier
    LockdownTrigger trigger;               // Trigger type
    LockdownLevel level;                   // Lockdown level
    std::chrono::system_clock::time_point timestamp;
    std::string userId;                    // User who triggered (if applicable)
    std::string deviceId;                  // Device identifier
    std::string ipAddress;                 // IP address
    std::string description;               // Event description
    bool wasSuccessful;                    // Activation success
    std::string errorMessage;              // Error message if failed
    std::chrono::seconds duration;        // Lockdown duration
};

/**
 * @brief Emergency contact information
 */
struct PHANTOM_VAULT_EXPORT EmergencyContact {
    std::string id;                        // Contact identifier
    std::string name;                      // Contact name
    std::string email;                     // Email address
    std::string phone;                     // Phone number
    std::string role;                      // Contact role
    bool isPrimary;                        // Primary contact
    bool notifyOnLockdown;                 // Notify on lockdown
    bool notifyOnRecovery;                 // Notify on recovery
    std::string encryptionKey;             // Encryption key for secure communication
};

/**
 * @brief Emergency lockdown manager interface
 */
class PHANTOM_VAULT_EXPORT EmergencyLockdownManager {
public:
    virtual ~EmergencyLockdownManager() = default;
    
    // Configuration
    virtual bool configure(const LockdownConfig& config) = 0;
    virtual LockdownConfig getConfiguration() const = 0;
    virtual bool isEnabled() const = 0;
    virtual bool enable() = 0;
    virtual bool disable() = 0;
    
    // Lockdown operations
    virtual bool activateLockdown(LockdownTrigger trigger, LockdownLevel level = LockdownLevel::Soft) = 0;
    virtual bool deactivateLockdown(const std::string& recoveryCode) = 0;
    virtual bool forceDeactivateLockdown() = 0;
    virtual bool isLockdownActive() const = 0;
    virtual LockdownStatus getLockdownStatus() const = 0;
    
    // Trigger management
    virtual bool enableTrigger(LockdownTrigger trigger) = 0;
    virtual bool disableTrigger(LockdownTrigger trigger) = 0;
    virtual bool isTriggerEnabled(LockdownTrigger trigger) const = 0;
    virtual std::set<LockdownTrigger> getEnabledTriggers() const = 0;
    
    // Emergency contacts
    virtual bool addEmergencyContact(const EmergencyContact& contact) = 0;
    virtual bool updateEmergencyContact(const EmergencyContact& contact) = 0;
    virtual bool removeEmergencyContact(const std::string& contactId) = 0;
    virtual std::vector<EmergencyContact> getEmergencyContacts() const = 0;
    virtual EmergencyContact getPrimaryContact() const = 0;
    
    // Event history
    virtual std::vector<LockdownEvent> getLockdownHistory() const = 0;
    virtual std::vector<LockdownEvent> getRecentEvents(int limit = 10) const = 0;
    virtual bool clearEventHistory() = 0;
    
    // Recovery
    virtual bool generateRecoveryCode() = 0;
    virtual std::string getRecoveryCode() const = 0;
    virtual bool validateRecoveryCode(const std::string& code) const = 0;
    virtual bool resetRecoveryCode() = 0;
    
    // Monitoring
    virtual bool startMonitoring() = 0;
    virtual bool stopMonitoring() = 0;
    virtual bool isMonitoring() const = 0;
    
    // Event callbacks
    virtual void setLockdownActivatedCallback(std::function<void(const LockdownEvent&)> callback) = 0;
    virtual void setLockdownDeactivatedCallback(std::function<void(const LockdownEvent&)> callback) = 0;
    virtual void setTriggerActivatedCallback(std::function<void(LockdownTrigger)> callback) = 0;
    virtual void setErrorCallback(std::function<void(const std::string&)> callback) = 0;
};

/**
 * @brief Local emergency lockdown manager implementation
 */
class PHANTOM_VAULT_EXPORT LocalEmergencyLockdownManager : public EmergencyLockdownManager {
public:
    LocalEmergencyLockdownManager();
    ~LocalEmergencyLockdownManager() override;
    
    // Configuration
    bool configure(const LockdownConfig& config) override;
    LockdownConfig getConfiguration() const override;
    bool isEnabled() const override;
    bool enable() override;
    bool disable() override;
    
    // Lockdown operations
    bool activateLockdown(LockdownTrigger trigger, LockdownLevel level = LockdownLevel::Soft) override;
    bool deactivateLockdown(const std::string& recoveryCode) override;
    bool forceDeactivateLockdown() override;
    bool isLockdownActive() const override;
    LockdownStatus getLockdownStatus() const override;
    
    // Trigger management
    bool enableTrigger(LockdownTrigger trigger) override;
    bool disableTrigger(LockdownTrigger trigger) override;
    bool isTriggerEnabled(LockdownTrigger trigger) const override;
    std::set<LockdownTrigger> getEnabledTriggers() const override;
    
    // Emergency contacts
    bool addEmergencyContact(const EmergencyContact& contact) override;
    bool updateEmergencyContact(const EmergencyContact& contact) override;
    bool removeEmergencyContact(const std::string& contactId) override;
    std::vector<EmergencyContact> getEmergencyContacts() const override;
    EmergencyContact getPrimaryContact() const override;
    
    // Event history
    std::vector<LockdownEvent> getLockdownHistory() const override;
    std::vector<LockdownEvent> getRecentEvents(int limit = 10) const override;
    bool clearEventHistory() override;
    
    // Recovery
    bool generateRecoveryCode() override;
    std::string getRecoveryCode() const override;
    bool validateRecoveryCode(const std::string& code) const override;
    bool resetRecoveryCode() override;
    
    // Monitoring
    bool startMonitoring() override;
    bool stopMonitoring() override;
    bool isMonitoring() const override;
    
    // Event callbacks
    void setLockdownActivatedCallback(std::function<void(const LockdownEvent&)> callback) override;
    void setLockdownDeactivatedCallback(std::function<void(const LockdownEvent&)> callback) override;
    void setTriggerActivatedCallback(std::function<void(LockdownTrigger)> callback) override;
    void setErrorCallback(std::function<void(const std::string&)> callback) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Panic button handler
 */
class PHANTOM_VAULT_EXPORT PanicButtonHandler {
public:
    PanicButtonHandler();
    ~PanicButtonHandler();
    
    // Panic button operations
    bool registerPanicButton(const std::string& buttonId, LockdownLevel level);
    bool unregisterPanicButton(const std::string& buttonId);
    bool triggerPanicButton(const std::string& buttonId);
    bool isPanicButtonRegistered(const std::string& buttonId) const;
    
    // Global panic button
    bool enableGlobalPanicButton(LockdownLevel level);
    bool disableGlobalPanicButton();
    bool isGlobalPanicButtonEnabled() const;
    
    // Event callbacks
    void setPanicButtonPressedCallback(std::function<void(const std::string&, LockdownLevel)> callback);
    void setPanicButtonReleasedCallback(std::function<void(const std::string&)> callback);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Emergency notification system
 */
class PHANTOM_VAULT_EXPORT EmergencyNotificationSystem {
public:
    virtual ~EmergencyNotificationSystem() = default;
    
    // Notification methods
    virtual bool sendEmailNotification(const std::string& to, const std::string& subject, 
                                      const std::string& body) = 0;
    virtual bool sendSMSNotification(const std::string& to, const std::string& message) = 0;
    virtual bool sendPushNotification(const std::string& deviceId, const std::string& message) = 0;
    virtual bool sendSystemNotification(const std::string& title, const std::string& message) = 0;
    
    // Emergency notifications
    virtual bool notifyLockdownActivated(const LockdownEvent& event) = 0;
    virtual bool notifyLockdownDeactivated(const LockdownEvent& event) = 0;
    virtual bool notifyEmergencyContact(const EmergencyContact& contact, const LockdownEvent& event) = 0;
    virtual bool notifyRecoveryCodeGenerated(const std::string& code) = 0;
    
    // Configuration
    virtual bool configureEmail(const std::string& smtpServer, int port, 
                               const std::string& username, const std::string& password) = 0;
    virtual bool configureSMS(const std::string& provider, const std::string& apiKey) = 0;
    virtual bool configurePush(const std::string& service, const std::string& apiKey) = 0;
};

/**
 * @brief Emergency security utilities
 */
class PHANTOM_VAULT_EXPORT EmergencySecurity {
public:
    // Data protection
    static bool secureWipeFile(const std::string& filePath, int passes = 3);
    static bool secureWipeDirectory(const std::string& dirPath, int passes = 3);
    static bool secureWipeMemory(void* ptr, size_t size);
    
    // Vault protection
    static bool lockAllVaults();
    static bool encryptAllVaults();
    static bool hideAllVaults();
    static bool clearVaultCache();
    
    // System protection
    static bool clearClipboard();
    static bool clearTempFiles();
    static bool clearLogs();
    static bool disableNetwork();
    static bool enableNetwork();
    
    // Recovery utilities
    static bool createRecoveryBackup(const std::string& backupPath);
    static bool restoreFromRecoveryBackup(const std::string& backupPath);
    static bool validateRecoveryBackup(const std::string& backupPath);
    
    // Emergency protocols
    static bool executeEmergencyProtocol(LockdownLevel level);
    static bool validateEmergencyIntegrity();
    static bool generateEmergencyReport(const std::string& outputPath);
};

} // namespace phantom_vault::emergency
