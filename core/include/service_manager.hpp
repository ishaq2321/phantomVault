/**
 * PhantomVault Service Manager
 * 
 * Main service coordinator that manages all PhantomVault components.
 * Handles initialization, lifecycle, and coordination between components.
 */

#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <functional>

namespace phantomvault {

// Forward declarations
class ProfileManager;
class FolderSecurityManager;
class KeyboardSequenceDetector;
class AnalyticsEngine;
class IPCServer;

/**
 * Main service manager class
 * 
 * Coordinates all PhantomVault components and manages service lifecycle.
 * Designed for minimal resource usage (< 10MB RAM) and low battery impact.
 */
class ServiceManager {
public:
    ServiceManager();
    ~ServiceManager();

    // Service lifecycle
    bool initialize(const std::string& configFile = "", 
                   const std::string& logLevel = "INFO", 
                   int ipcPort = 9876);
    bool start();
    void stop();
    bool isRunning() const;

    // Component access
    ProfileManager* getProfileManager() const;
    FolderSecurityManager* getFolderSecurityManager() const;
    KeyboardSequenceDetector* getKeyboardSequenceDetector() const;
    AnalyticsEngine* getAnalyticsEngine() const;

    // Error handling
    std::string getLastError() const;

    // Service information
    std::string getVersion() const;
    std::string getPlatformInfo() const;
    size_t getMemoryUsage() const;
    
    // Process protection
    void enableProcessProtection();
    void disableProcessProtection();
    bool isProcessProtected() const;
    void setTerminationCallback(std::function<void()> callback);

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

} // namespace phantomvault