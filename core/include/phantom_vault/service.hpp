#pragma once

#include "phantom_vault_export.h"
#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>
#include <thread>

namespace phantom_vault {
namespace service {

/**
 * @brief Configuration for the background service
 */
struct PHANTOM_VAULT_EXPORT ServiceConfig {
    std::string service_name = "phantom-vault";
    std::chrono::seconds auto_lock_timeout{300};
    std::chrono::seconds input_timeout{10};
    std::chrono::seconds recovery_window_timeout{5};
    bool enable_sudo_protection = true;
    bool enable_system_monitoring = true;
    std::string log_level = "INFO";
};

/**
 * @brief Vault state tracking for the service
 */
struct PHANTOM_VAULT_EXPORT VaultState {
    std::string profile_id;
    std::chrono::system_clock::time_point last_activity;
    std::atomic<bool> is_locked{true};
    size_t temporary_unlock_count{0};
};

/**
 * @brief Temporary unlock tracking
 */
struct PHANTOM_VAULT_EXPORT TemporaryUnlock {
    std::string folder_id;
    std::string original_location;
    std::chrono::system_clock::time_point unlock_time;
    std::string profile_id;
};

/**
 * @brief Main background service class
 * 
 * This class provides the core functionality for the PhantomVault background service,
 * including hotkey management, vault operations, and system integration.
 */
class PHANTOM_VAULT_EXPORT BackgroundService {
public:
    /**
     * @brief Constructor
     */
    BackgroundService();

    /**
     * @brief Destructor
     */
    ~BackgroundService();

    /**
     * @brief Initialize the background service
     * @param config Service configuration
     * @return true if initialization successful, false otherwise
     */
    bool initialize(const ServiceConfig& config = ServiceConfig{});

    /**
     * @brief Start the service main loop
     * @return true if service started successfully, false otherwise
     */
    bool start();

    /**
     * @brief Stop the service
     */
    void stop();

    /**
     * @brief Check if service is running
     * @return true if running, false otherwise
     */
    bool isRunning() const;

    /**
     * @brief Get current service configuration
     * @return Current configuration
     */
    const ServiceConfig& getConfig() const;

    /**
     * @brief Get current vault state
     * @return Current vault state
     */
    const VaultState& getVaultState() const;

    /**
     * @brief Get service uptime
     * @return Uptime in seconds
     */
    std::chrono::seconds getUptime() const;

    /**
     * @brief Get last error message
     * @return Error message string
     */
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class BackgroundService

/**
 * @brief Service logger with security focus
 */
class PHANTOM_VAULT_EXPORT ServiceLogger {
public:
    enum class LogLevel {
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 3,
        SECURITY = 4
    };

    /**
     * @brief Constructor
     */
    ServiceLogger();

    /**
     * @brief Destructor
     */
    ~ServiceLogger();

    /**
     * @brief Initialize logger
     * @param service_name Name of the service for logging
     * @param log_level Minimum log level to record
     * @return true if initialization successful
     */
    bool initialize(const std::string& service_name, LogLevel log_level = LogLevel::INFO);

    /**
     * @brief Log informational message
     * @param message Message to log
     */
    void logInfo(const std::string& message);

    /**
     * @brief Log warning message
     * @param message Message to log
     */
    void logWarning(const std::string& message);

    /**
     * @brief Log error message
     * @param message Message to log
     */
    void logError(const std::string& message);

    /**
     * @brief Log security event
     * @param event Security event description
     */
    void logSecurity(const std::string& event);

    /**
     * @brief Log debug message
     * @param message Message to log
     */
    void logDebug(const std::string& message);

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class ServiceLogger

/**
 * @brief Service recovery and error handling
 */
class PHANTOM_VAULT_EXPORT ServiceRecovery {
public:
    /**
     * @brief Constructor
     */
    ServiceRecovery();

    /**
     * @brief Destructor
     */
    ~ServiceRecovery();

    /**
     * @brief Initialize recovery system
     * @param service Pointer to the main service
     * @return true if initialization successful
     */
    bool initialize(BackgroundService* service);

    /**
     * @brief Handle service crash
     */
    void handleCrash();

    /**
     * @brief Restore from backup
     * @return true if restoration successful
     */
    bool restoreFromBackup();

    /**
     * @brief Validate system state
     * @return true if system state is valid
     */
    bool validateSystemState();

    /**
     * @brief Lock all folders immediately (emergency)
     */
    void emergencyLockAll();

    /**
     * @brief Clear temporary state
     */
    void clearTemporaryState();

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class ServiceRecovery

} // namespace service
} // namespace phantom_vault