/**
 * PhantomVault Platform Adapter
 * 
 * Provides platform-specific capability detection and fallback mechanisms
 * for vault access when advanced features are unavailable.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <optional>

namespace phantomvault {

/**
 * Platform-specific capabilities
 */
struct PlatformCapabilities {
    // Keyboard monitoring capabilities
    bool supportsInvisibleKeyboardLogging = false;
    bool supportsGlobalHotkeys = false;
    bool requiresKeyboardPermissions = false;
    
    // Notification system capabilities
    bool supportsSystemNotifications = false;
    bool supportsInteractiveNotifications = false;
    bool supportsNotificationInput = false;
    
    // Context menu capabilities
    bool supportsContextMenus = false;
    bool supportsFileManagerIntegration = false;
    bool supportsCustomContextActions = false;
    
    // Folder hiding capabilities
    bool supportsAdvancedFolderHiding = false;
    bool supportsFileSystemHooks = false;
    bool requiresElevatedPrivileges = false;
    
    // Security features
    bool supportsSecureInput = false;
    bool supportsMemoryProtection = false;
    bool supportsProcessIsolation = false;
    
    // Platform identification
    std::string platformName;
    std::string platformVersion;
    std::vector<std::string> requiredPermissions;
    std::vector<std::string> optionalFeatures;
};

/**
 * Notification-based input request
 */
struct NotificationInputRequest {
    std::string title;
    std::string message;
    std::string placeholder;
    bool isSecure = true;
    int timeoutSeconds = 30;
    std::function<void(const std::string&)> onInput;
    std::function<void()> onTimeout;
    std::function<void()> onCancel;
};

/**
 * Context menu action
 */
struct ContextMenuAction {
    std::string id;
    std::string label;
    std::string icon;
    bool requiresPassword = true;
    std::function<void(const std::string&, const std::string&)> handler; // path, password
};

/**
 * Platform adapter result
 */
struct PlatformResult {
    bool success = false;
    std::string message;
    std::string errorDetails;
    std::vector<std::string> availableAlternatives;
};

/**
 * Fallback unlock method
 */
enum class FallbackMethod {
    NOTIFICATION_INPUT,
    CONTEXT_MENU,
    COMMAND_LINE,
    GUI_PROMPT,
    FILE_DROP,
    NONE
};

/**
 * Platform adapter for capability detection and fallbacks
 */
class PlatformAdapter {
public:
    PlatformAdapter();
    ~PlatformAdapter();

    // Initialization and capability detection
    bool initialize();
    PlatformCapabilities detectCapabilities() const;
    bool checkPermissions() const;
    std::vector<std::string> getMissingPermissions() const;
    
    // Platform identification
    std::string getPlatformName() const;
    std::string getPlatformVersion() const;
    bool isFeatureSupported(const std::string& feature) const;
    
    // Notification-based input
    bool supportsNotificationInput() const;
    PlatformResult showNotificationInput(const NotificationInputRequest& request);
    void cancelNotificationInput();
    
    // Context menu integration
    bool supportsContextMenus() const;
    PlatformResult registerContextMenu(const std::string& path, const std::vector<ContextMenuAction>& actions);
    PlatformResult unregisterContextMenu(const std::string& path);
    void setContextMenuHandler(std::function<void(const std::string&, const std::string&, const std::string&)> handler);
    
    // Fallback method detection and selection
    std::vector<FallbackMethod> getAvailableFallbacks() const;
    FallbackMethod selectBestFallback() const;
    PlatformResult executeFallback(FallbackMethod method, const std::string& profileId);
    
    // Graceful degradation
    bool canGracefullyDegrade() const;
    std::vector<std::string> getDegradationWarnings() const;
    PlatformResult configureForLimitedMode();
    
    // Platform-specific unlock mechanisms
    PlatformResult requestPasswordViaNotification(const std::string& profileId, const std::string& message);
    PlatformResult requestPasswordViaContextMenu(const std::string& folderPath, const std::string& profileId);
    PlatformResult requestPasswordViaCommandLine(const std::string& profileId);
    PlatformResult requestPasswordViaGUI(const std::string& profileId);
    
    // Capability-specific helpers
    bool enableAdvancedFeatures();
    bool disableAdvancedFeatures();
    PlatformResult testPlatformFeatures();
    
    // Event callbacks
    void setOnCapabilityChanged(std::function<void(const PlatformCapabilities&)> callback);
    void setOnPermissionLost(std::function<void(const std::string&)> callback);
    void setOnFallbackNeeded(std::function<void(FallbackMethod)> callback);
    
    // Error handling and diagnostics
    std::string getLastError() const;
    std::vector<std::string> getDiagnosticInfo() const;
    bool performSelfTest();

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

/**
 * Platform-specific unlock provider interface
 */
class UnlockProvider {
public:
    virtual ~UnlockProvider() = default;
    
    virtual bool isAvailable() const = 0;
    virtual std::string getName() const = 0;
    virtual PlatformResult requestPassword(const std::string& profileId, const std::string& hint) = 0;
    virtual void cancel() = 0;
};

/**
 * Notification-based unlock provider
 */
class NotificationUnlockProvider : public UnlockProvider {
public:
    NotificationUnlockProvider();
    ~NotificationUnlockProvider() override;
    
    bool isAvailable() const override;
    std::string getName() const override;
    PlatformResult requestPassword(const std::string& profileId, const std::string& hint) override;
    void cancel() override;
    
private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

/**
 * Context menu unlock provider
 */
class ContextMenuUnlockProvider : public UnlockProvider {
public:
    ContextMenuUnlockProvider();
    ~ContextMenuUnlockProvider() override;
    
    bool isAvailable() const override;
    std::string getName() const override;
    PlatformResult requestPassword(const std::string& profileId, const std::string& hint) override;
    void cancel() override;
    
    // Context menu specific methods
    bool registerForPath(const std::string& path);
    bool unregisterFromPath(const std::string& path);
    
private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

/**
 * Command line unlock provider
 */
class CommandLineUnlockProvider : public UnlockProvider {
public:
    CommandLineUnlockProvider();
    ~CommandLineUnlockProvider() override;
    
    bool isAvailable() const override;
    std::string getName() const override;
    PlatformResult requestPassword(const std::string& profileId, const std::string& hint) override;
    void cancel() override;
    
private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

/**
 * GUI prompt unlock provider
 */
class GUIPromptUnlockProvider : public UnlockProvider {
public:
    GUIPromptUnlockProvider();
    ~GUIPromptUnlockProvider() override;
    
    bool isAvailable() const override;
    std::string getName() const override;
    PlatformResult requestPassword(const std::string& profileId, const std::string& hint) override;
    void cancel() override;
    
private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

} // namespace phantomvault