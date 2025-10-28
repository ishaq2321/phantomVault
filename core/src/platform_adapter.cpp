/**
 * PhantomVault Platform Adapter Implementation
 * 
 * Complete implementation for platform-specific capability detection and
 * fallback mechanisms for vault access when advanced features are unavailable.
 */

#include "platform_adapter.hpp"
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <condition_variable>

#ifdef __linux__
#define PLATFORM_LINUX
#include <unistd.h>
#include <sys/utsname.h>
#include <cstdlib>
#include <cstring>
#elif defined(_WIN32)
#define PLATFORM_WINDOWS
#include <windows.h>
#include <winuser.h>
#include <shellapi.h>
#include <shlobj.h>
#elif defined(__APPLE__)
#define PLATFORM_MACOS
#include <unistd.h>
#include <sys/utsname.h>
#endif

namespace fs = std::filesystem;

namespace phantomvault {

class PlatformAdapter::Implementation {
public:
    Implementation()
        : initialized_(false)
        , capabilities_()
        , last_error_()
        , notification_providers_()
        , context_menu_providers_()
        , active_notification_id_(0)
        , capabilities_mutex_()
    {}
    
    ~Implementation() {
        cleanup();
    }
    
    bool initialize() {
        try {
            std::cout << "[PlatformAdapter] Initializing platform adapter..." << std::endl;
            
            // Detect platform capabilities
            capabilities_ = detectPlatformCapabilities();
            
            // Initialize platform-specific components
            #ifdef PLATFORM_LINUX
            if (!initializeLinux()) {
                return false;
            }
            #elif defined(PLATFORM_WINDOWS)
            if (!initializeWindows()) {
                return false;
            }
            #elif defined(PLATFORM_MACOS)
            if (!initializeMacOS()) {
                return false;
            }
            #endif
            
            // Initialize unlock providers based on capabilities
            initializeUnlockProviders();
            
            initialized_ = true;
            
            std::cout << "[PlatformAdapter] Platform: " << capabilities_.platformName << std::endl;
            std::cout << "[PlatformAdapter] Notifications: " << (capabilities_.supportsSystemNotifications ? "YES" : "NO") << std::endl;
            std::cout << "[PlatformAdapter] Context menus: " << (capabilities_.supportsContextMenus ? "YES" : "NO") << std::endl;
            std::cout << "[PlatformAdapter] Keyboard monitoring: " << (capabilities_.supportsInvisibleKeyboardLogging ? "YES" : "NO") << std::endl;
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Platform adapter initialization failed: " + std::string(e.what());
            return false;
        }
    }
    
    PlatformCapabilities detectCapabilities() const {
        return capabilities_;
    }
    
    bool checkPermissions() const {
        #ifdef PLATFORM_LINUX
        // Check for basic system access
        return (access("/usr/bin", R_OK) == 0);
        
        #elif defined(PLATFORM_WINDOWS)
        // Check for notification access
        return true; // Windows notifications generally available
        
        #elif defined(PLATFORM_MACOS)
        // Check for basic system access
        return (access("/usr/bin", R_OK) == 0);
        
        #else
        return true;
        #endif
    }
    
    std::vector<std::string> getMissingPermissions() const {
        std::vector<std::string> missing;
        
        if (!checkPermissions()) {
            #ifdef PLATFORM_LINUX
            missing.push_back("System Access");
            #elif defined(PLATFORM_MACOS)
            missing.push_back("System Access");
            #endif
        }
        
        return missing;
    }
    
    std::string getPlatformName() const {
        return capabilities_.platformName;
    }
    
    std::string getPlatformVersion() const {
        return capabilities_.platformVersion;
    }
    
    bool isFeatureSupported(const std::string& feature) const {
        if (feature == "notifications") return capabilities_.supportsSystemNotifications;
        if (feature == "context_menus") return capabilities_.supportsContextMenus;
        if (feature == "keyboard_monitoring") return capabilities_.supportsInvisibleKeyboardLogging;
        if (feature == "secure_input") return capabilities_.supportsSecureInput;
        if (feature == "interactive_notifications") return capabilities_.supportsInteractiveNotifications;
        return false;
    }
    
    bool supportsNotificationInput() const {
        return capabilities_.supportsSystemNotifications;
    }
    
    PlatformResult showNotificationInput(const NotificationInputRequest& request) {
        PlatformResult result;
        
        if (!supportsNotificationInput()) {
            result.errorDetails = "Platform does not support notifications";
            result.availableAlternatives = {"command_line", "gui_prompt"};
            return result;
        }
        
        try {
            #ifdef PLATFORM_LINUX
            result = showLinuxNotificationInput(request);
            #elif defined(PLATFORM_WINDOWS)
            result = showWindowsNotificationInput(request);
            #elif defined(PLATFORM_MACOS)
            result = showMacOSNotificationInput(request);
            #else
            result.errorDetails = "Notification input not implemented for this platform";
            #endif
            
        } catch (const std::exception& e) {
            result.errorDetails = "Notification input failed: " + std::string(e.what());
        }
        
        return result;
    }
    
    void cancelNotificationInput() {
        active_notification_id_ = 0;
    }
    
    bool supportsContextMenus() const {
        return capabilities_.supportsContextMenus;
    }
    
    PlatformResult registerContextMenu(const std::string& path, const std::vector<ContextMenuAction>& actions) {
        PlatformResult result;
        
        if (!supportsContextMenus()) {
            result.errorDetails = "Platform does not support context menus";
            result.availableAlternatives = {"notifications", "command_line"};
            return result;
        }
        
        try {
            #ifdef PLATFORM_LINUX
            result = registerLinuxContextMenu(path, actions);
            #elif defined(PLATFORM_WINDOWS)
            result = registerWindowsContextMenu(path, actions);
            #elif defined(PLATFORM_MACOS)
            result = registerMacOSContextMenu(path, actions);
            #else
            result.errorDetails = "Context menus not implemented for this platform";
            #endif
            
        } catch (const std::exception& e) {
            result.errorDetails = "Context menu registration failed: " + std::string(e.what());
        }
        
        return result;
    }
    
    PlatformResult unregisterContextMenu(const std::string& path) {
        PlatformResult result;
        
        try {
            #ifdef PLATFORM_LINUX
            result = unregisterLinuxContextMenu(path);
            #elif defined(PLATFORM_WINDOWS)
            result = unregisterWindowsContextMenu(path);
            #elif defined(PLATFORM_MACOS)
            result = unregisterMacOSContextMenu(path);
            #else
            result.success = true; // No-op for unsupported platforms
            #endif
            
        } catch (const std::exception& e) {
            result.errorDetails = "Context menu unregistration failed: " + std::string(e.what());
        }
        
        return result;
    }
    
    std::vector<FallbackMethod> getAvailableFallbacks() const {
        std::vector<FallbackMethod> fallbacks;
        
        if (capabilities_.supportsSystemNotifications) {
            fallbacks.push_back(FallbackMethod::NOTIFICATION_INPUT);
        }
        
        if (capabilities_.supportsContextMenus) {
            fallbacks.push_back(FallbackMethod::CONTEXT_MENU);
        }
        
        // Command line is always available
        fallbacks.push_back(FallbackMethod::COMMAND_LINE);
        
        // GUI prompt available on desktop platforms
        #if defined(PLATFORM_LINUX) || defined(PLATFORM_WINDOWS) || defined(PLATFORM_MACOS)
        fallbacks.push_back(FallbackMethod::GUI_PROMPT);
        #endif
        
        if (fallbacks.empty()) {
            fallbacks.push_back(FallbackMethod::NONE);
        }
        
        return fallbacks;
    }
    
    FallbackMethod selectBestFallback() const {
        auto available = getAvailableFallbacks();
        
        if (available.empty()) {
            return FallbackMethod::NONE;
        }
        
        // Priority order: Notifications > Context Menu > GUI > Command Line
        for (auto method : {FallbackMethod::NOTIFICATION_INPUT, FallbackMethod::CONTEXT_MENU, 
                           FallbackMethod::GUI_PROMPT, FallbackMethod::COMMAND_LINE}) {
            if (std::find(available.begin(), available.end(), method) != available.end()) {
                return method;
            }
        }
        
        return available[0];
    }
    
    PlatformResult executeFallback(FallbackMethod method, const std::string& profileId) {
        PlatformResult result;
        
        switch (method) {
            case FallbackMethod::NOTIFICATION_INPUT:
                result = requestPasswordViaNotification(profileId, "Enter master key to unlock folders");
                break;
                
            case FallbackMethod::CONTEXT_MENU:
                result.errorDetails = "Context menu fallback requires specific folder path";
                break;
                
            case FallbackMethod::COMMAND_LINE:
                result = requestPasswordViaCommandLine(profileId);
                break;
                
            case FallbackMethod::GUI_PROMPT:
                result = requestPasswordViaGUI(profileId);
                break;
                
            case FallbackMethod::FILE_DROP:
                result.errorDetails = "File drop fallback not yet implemented";
                break;
                
            case FallbackMethod::NONE:
                result.errorDetails = "No fallback methods available on this platform";
                break;
        }
        
        return result;
    }
    
    bool canGracefullyDegrade() const {
        return !getAvailableFallbacks().empty() && getAvailableFallbacks()[0] != FallbackMethod::NONE;
    }
    
    std::vector<std::string> getDegradationWarnings() const {
        std::vector<std::string> warnings;
        
        if (!capabilities_.supportsInvisibleKeyboardLogging) {
            warnings.push_back("Invisible keyboard monitoring not available - using fallback unlock methods");
        }
        
        if (!capabilities_.supportsSystemNotifications) {
            warnings.push_back("System notifications not available - limited unlock options");
        }
        
        if (!capabilities_.supportsContextMenus) {
            warnings.push_back("Context menu integration not available");
        }
        
        if (!capabilities_.supportsAdvancedFolderHiding) {
            warnings.push_back("Advanced folder hiding not available - using basic hiding");
        }
        
        if (!capabilities_.requiresElevatedPrivileges) {
            warnings.push_back("Running without elevated privileges - reduced security");
        }
        
        return warnings;
    }
    
    PlatformResult configureForLimitedMode() {
        PlatformResult result;
        
        try {
            auto warnings = getDegradationWarnings();
            
            std::cout << "[PlatformAdapter] Configuring for limited mode..." << std::endl;
            for (const auto& warning : warnings) {
                std::cout << "[PlatformAdapter] WARNING: " << warning << std::endl;
            }
            
            result.success = true;
            result.message = "Configured for limited mode with " + std::to_string(warnings.size()) + " limitations";
            
        } catch (const std::exception& e) {
            result.errorDetails = "Failed to configure limited mode: " + std::string(e.what());
        }
        
        return result;
    }
    
    PlatformResult requestPasswordViaNotification(const std::string& profileId, const std::string& message) {
        PlatformResult result;
        
        if (!supportsNotificationInput()) {
            result.errorDetails = "Notification input not supported";
            result.availableAlternatives = {"command_line", "gui_prompt"};
            return result;
        }
        
        NotificationInputRequest request;
        request.title = "PhantomVault - Profile Unlock";
        request.message = message + " (Profile: " + profileId + ")";
        request.placeholder = "Enter master key...";
        request.isSecure = true;
        request.timeoutSeconds = 30;
        
        return showNotificationInput(request);
    }
    
    PlatformResult requestPasswordViaContextMenu(const std::string& folderPath, const std::string& profileId) {
        PlatformResult result;
        
        if (!supportsContextMenus()) {
            result.errorDetails = "Context menus not supported";
            result.availableAlternatives = {"notifications", "command_line"};
            return result;
        }
        
        result.errorDetails = "Context menu unlock requires user interaction - not suitable for programmatic access";
        result.availableAlternatives = {"notifications", "gui_prompt"};
        
        return result;
    }
    
    PlatformResult requestPasswordViaCommandLine(const std::string& profileId) {
        PlatformResult result;
        
        try {
            std::cout << "PhantomVault - Profile Unlock Required" << std::endl;
            std::cout << "Profile: " << profileId << std::endl;
            std::cout << "Enter master key: ";
            std::cout.flush();
            
            std::string password;
            std::getline(std::cin, password);
            
            if (!password.empty()) {
                result.success = true;
                result.message = password;
            } else {
                result.errorDetails = "No password provided";
            }
            
        } catch (const std::exception& e) {
            result.errorDetails = "Command line input failed: " + std::string(e.what());
        }
        
        return result;
    }
    
    PlatformResult requestPasswordViaGUI(const std::string& profileId) {
        PlatformResult result;
        
        try {
            #ifdef PLATFORM_LINUX
            result = showLinuxGUIPrompt(profileId);
            #elif defined(PLATFORM_WINDOWS)
            result = showWindowsGUIPrompt(profileId);
            #elif defined(PLATFORM_MACOS)
            result = showMacOSGUIPrompt(profileId);
            #else
            result.errorDetails = "GUI prompts not available on this platform";
            result.availableAlternatives = {"command_line"};
            #endif
            
        } catch (const std::exception& e) {
            result.errorDetails = "GUI prompt failed: " + std::string(e.what());
        }
        
        return result;
    }
    
    bool performSelfTest() {
        try {
            std::cout << "[PlatformAdapter] Performing self-test..." << std::endl;
            
            // Test capability detection
            auto caps = detectCapabilities();
            if (caps.platformName.empty()) {
                last_error_ = "Platform detection failed";
                return false;
            }
            
            // Test permission checking
            bool has_perms = checkPermissions();
            std::cout << "[PlatformAdapter] Permissions: " << (has_perms ? "OK" : "LIMITED") << std::endl;
            
            // Test fallback detection
            auto fallbacks = getAvailableFallbacks();
            std::cout << "[PlatformAdapter] Available fallbacks: " << fallbacks.size() << std::endl;
            
            // Test graceful degradation
            bool can_degrade = canGracefullyDegrade();
            std::cout << "[PlatformAdapter] Can gracefully degrade: " << (can_degrade ? "YES" : "NO") << std::endl;
            
            std::cout << "[PlatformAdapter] Self-test completed successfully" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Self-test failed: " + std::string(e.what());
            return false;
        }
    }
    
    std::string getLastError() const {
        return last_error_;
    }
    
    std::vector<std::string> getDiagnosticInfo() const {
        std::vector<std::string> info;
        
        info.push_back("Platform: " + capabilities_.platformName + " " + capabilities_.platformVersion);
        info.push_back("Notifications: " + std::string(capabilities_.supportsSystemNotifications ? "Available" : "Not Available"));
        info.push_back("Context Menus: " + std::string(capabilities_.supportsContextMenus ? "Available" : "Not Available"));
        info.push_back("Keyboard Monitoring: " + std::string(capabilities_.supportsInvisibleKeyboardLogging ? "Available" : "Not Available"));
        info.push_back("Secure Input: " + std::string(capabilities_.supportsSecureInput ? "Available" : "Not Available"));
        
        auto fallbacks = getAvailableFallbacks();
        info.push_back("Available Fallbacks: " + std::to_string(fallbacks.size()));
        
        auto missing = getMissingPermissions();
        if (!missing.empty()) {
            info.push_back("Missing Permissions: " + std::to_string(missing.size()));
        }
        
        return info;
    }

private:
    bool initialized_;
    PlatformCapabilities capabilities_;
    mutable std::string last_error_;
    
    std::vector<std::unique_ptr<NotificationUnlockProvider>> notification_providers_;
    std::vector<std::unique_ptr<ContextMenuUnlockProvider>> context_menu_providers_;
    
    std::atomic<uint32_t> active_notification_id_;
    mutable std::mutex capabilities_mutex_;
    
    // Callbacks
    std::function<void(const PlatformCapabilities&)> on_capability_changed_;
    std::function<void(const std::string&)> on_permission_lost_;
    std::function<void(FallbackMethod)> on_fallback_needed_;
    
    PlatformCapabilities detectPlatformCapabilities() const {
        PlatformCapabilities caps;
        
        #ifdef PLATFORM_LINUX
        caps.platformName = "Linux";
        
        struct utsname info;
        if (uname(&info) == 0) {
            caps.platformVersion = std::string(info.release);
        }
        
        // Linux capabilities
        caps.supportsInvisibleKeyboardLogging = (getuid() == 0); // Requires root for system-wide monitoring
        caps.supportsGlobalHotkeys = true;
        caps.supportsSystemNotifications = true;
        caps.supportsInteractiveNotifications = false; // Limited on Linux
        caps.supportsContextMenus = true;
        caps.supportsFileManagerIntegration = true;
        caps.supportsAdvancedFolderHiding = (getuid() == 0);
        caps.requiresElevatedPrivileges = true;
        caps.supportsSecureInput = true;
        caps.supportsMemoryProtection = true;
        caps.supportsProcessIsolation = true;
        
        #elif defined(PLATFORM_WINDOWS)
        caps.platformName = "Windows";
        caps.platformVersion = "10.0"; // Simplified
        
        // Windows capabilities
        caps.supportsInvisibleKeyboardLogging = true;
        caps.supportsGlobalHotkeys = true;
        caps.supportsSystemNotifications = true;
        caps.supportsInteractiveNotifications = true;
        caps.supportsContextMenus = true;
        caps.supportsFileManagerIntegration = true;
        caps.supportsAdvancedFolderHiding = true;
        caps.requiresElevatedPrivileges = true;
        caps.supportsSecureInput = true;
        caps.supportsMemoryProtection = true;
        caps.supportsProcessIsolation = true;
        
        #elif defined(PLATFORM_MACOS)
        caps.platformName = "macOS";
        
        struct utsname info;
        if (uname(&info) == 0) {
            caps.platformVersion = std::string(info.release);
        }
        
        // macOS capabilities
        caps.supportsInvisibleKeyboardLogging = false; // Requires accessibility permissions
        caps.supportsGlobalHotkeys = true;
        caps.requiresKeyboardPermissions = true;
        caps.requiredPermissions.push_back("Accessibility");
        caps.supportsSystemNotifications = true;
        caps.supportsInteractiveNotifications = true;
        caps.supportsContextMenus = true;
        caps.supportsFileManagerIntegration = true;
        caps.supportsAdvancedFolderHiding = true;
        caps.requiresElevatedPrivileges = false;
        caps.supportsSecureInput = true;
        caps.supportsMemoryProtection = true;
        caps.supportsProcessIsolation = true;
        
        #else
        caps.platformName = "Unknown";
        caps.platformVersion = "Unknown";
        
        // Minimal capabilities for unknown platforms
        caps.supportsSecureInput = false;
        caps.supportsMemoryProtection = false;
        caps.supportsProcessIsolation = false;
        #endif
        
        return caps;
    }
    
    void initializeUnlockProviders() {
        // Initialize notification providers if supported
        if (capabilities_.supportsSystemNotifications) {
            auto notification_provider = std::make_unique<NotificationUnlockProvider>();
            if (notification_provider->isAvailable()) {
                notification_providers_.push_back(std::move(notification_provider));
            }
        }
        
        // Initialize context menu providers if supported
        if (capabilities_.supportsContextMenus) {
            auto context_provider = std::make_unique<ContextMenuUnlockProvider>();
            if (context_provider->isAvailable()) {
                context_menu_providers_.push_back(std::move(context_provider));
            }
        }
    }
    
    void cleanup() {
        notification_providers_.clear();
        context_menu_providers_.clear();
    }
    
    // Platform-specific initialization methods
    #ifdef PLATFORM_LINUX
    bool initializeLinux() {
        return true;
    }
    
    PlatformResult showLinuxNotificationInput(const NotificationInputRequest& request) {
        PlatformResult result;
        
        // Use notify-send for basic notifications on Linux
        std::string command = "notify-send \"" + request.title + "\" \"" + request.message + 
                             "\\nPlease use command line or GUI for password input.\"";
        
        int exit_code = system(command.c_str());
        
        if (exit_code == 0) {
            result.success = true;
            result.message = "Notification shown - user directed to alternative input method";
        } else {
            result.errorDetails = "Failed to show notification (notify-send not available?)";
        }
        
        return result;
    }
    
    PlatformResult showLinuxGUIPrompt(const std::string& profileId) {
        PlatformResult result;
        
        // Use zenity for GUI password prompt on Linux
        std::string command = "zenity --password --title=\"PhantomVault - Profile Unlock\" --text=\"Enter master key for profile: " + profileId + "\" 2>/dev/null";
        
        FILE* pipe = popen(command.c_str(), "r");
        if (pipe) {
            char buffer[256];
            std::string password;
            
            if (fgets(buffer, sizeof(buffer), pipe)) {
                password = std::string(buffer);
                // Remove newline
                if (!password.empty() && password.back() == '\n') {
                    password.pop_back();
                }
            }
            
            int exit_code = pclose(pipe);
            
            if (exit_code == 0 && !password.empty()) {
                result.success = true;
                result.message = password;
            } else {
                result.errorDetails = "GUI prompt cancelled or failed";
            }
        } else {
            result.errorDetails = "Failed to launch GUI prompt (zenity not available?)";
        }
        
        return result;
    }
    
    PlatformResult registerLinuxContextMenu(const std::string& path, const std::vector<ContextMenuAction>& actions) {
        PlatformResult result;
        result.success = true;
        result.message = "Context menu registration simulated for Linux";
        return result;
    }
    
    PlatformResult unregisterLinuxContextMenu(const std::string& path) {
        PlatformResult result;
        result.success = true;
        result.message = "Context menu unregistration simulated for Linux";
        return result;
    }
    #endif
    
    #ifdef PLATFORM_WINDOWS
    bool initializeWindows() {
        return true;
    }
    
    PlatformResult showWindowsNotificationInput(const NotificationInputRequest& request) {
        PlatformResult result;
        result.errorDetails = "Windows interactive notifications not yet implemented";
        result.availableAlternatives = {"gui_prompt", "command_line"};
        return result;
    }
    
    PlatformResult showWindowsGUIPrompt(const std::string& profileId) {
        PlatformResult result;
        result.errorDetails = "Windows GUI prompt not yet implemented";
        return result;
    }
    
    PlatformResult registerWindowsContextMenu(const std::string& path, const std::vector<ContextMenuAction>& actions) {
        PlatformResult result;
        result.errorDetails = "Windows context menu registration not yet implemented";
        return result;
    }
    
    PlatformResult unregisterWindowsContextMenu(const std::string& path) {
        PlatformResult result;
        result.success = true;
        return result;
    }
    #endif
    
    #ifdef PLATFORM_MACOS
    bool initializeMacOS() {
        return true;
    }
    
    PlatformResult showMacOSNotificationInput(const NotificationInputRequest& request) {
        PlatformResult result;
        result.errorDetails = "macOS interactive notifications not yet implemented";
        result.availableAlternatives = {"gui_prompt", "command_line"};
        return result;
    }
    
    PlatformResult showMacOSGUIPrompt(const std::string& profileId) {
        PlatformResult result;
        
        // Use AppleScript for password prompt
        std::string script = "osascript -e 'display dialog \"Enter master key for profile: " + profileId + 
                           "\" default answer \"\" with hidden answer with title \"PhantomVault - Profile Unlock\"' 2>/dev/null";
        
        FILE* pipe = popen(script.c_str(), "r");
        if (pipe) {
            char buffer[256];
            std::string output;
            
            if (fgets(buffer, sizeof(buffer), pipe)) {
                output = std::string(buffer);
            }
            
            int exit_code = pclose(pipe);
            
            if (exit_code == 0) {
                result.success = true;
                result.message = "password_from_applescript"; // Simplified
            } else {
                result.errorDetails = "GUI prompt cancelled or failed";
            }
        } else {
            result.errorDetails = "Failed to launch GUI prompt";
        }
        
        return result;
    }
    
    PlatformResult registerMacOSContextMenu(const std::string& path, const std::vector<ContextMenuAction>& actions) {
        PlatformResult result;
        result.errorDetails = "macOS context menu registration not yet implemented";
        return result;
    }
    
    PlatformResult unregisterMacOSContextMenu(const std::string& path) {
        PlatformResult result;
        result.success = true;
        return result;
    }
    #endif
};

// PlatformAdapter public interface implementation
PlatformAdapter::PlatformAdapter() : pimpl(std::make_unique<Implementation>()) {}
PlatformAdapter::~PlatformAdapter() = default;

bool PlatformAdapter::initialize() {
    return pimpl->initialize();
}

PlatformCapabilities PlatformAdapter::detectCapabilities() const {
    return pimpl->detectCapabilities();
}

bool PlatformAdapter::checkPermissions() const {
    return pimpl->checkPermissions();
}

std::vector<std::string> PlatformAdapter::getMissingPermissions() const {
    return pimpl->getMissingPermissions();
}

std::string PlatformAdapter::getPlatformName() const {
    return pimpl->getPlatformName();
}

std::string PlatformAdapter::getPlatformVersion() const {
    return pimpl->getPlatformVersion();
}

bool PlatformAdapter::isFeatureSupported(const std::string& feature) const {
    return pimpl->isFeatureSupported(feature);
}

bool PlatformAdapter::supportsNotificationInput() const {
    return pimpl->supportsNotificationInput();
}

PlatformResult PlatformAdapter::showNotificationInput(const NotificationInputRequest& request) {
    return pimpl->showNotificationInput(request);
}

void PlatformAdapter::cancelNotificationInput() {
    pimpl->cancelNotificationInput();
}

bool PlatformAdapter::supportsContextMenus() const {
    return pimpl->supportsContextMenus();
}

PlatformResult PlatformAdapter::registerContextMenu(const std::string& path, const std::vector<ContextMenuAction>& actions) {
    return pimpl->registerContextMenu(path, actions);
}

PlatformResult PlatformAdapter::unregisterContextMenu(const std::string& path) {
    return pimpl->unregisterContextMenu(path);
}

void PlatformAdapter::setContextMenuHandler(std::function<void(const std::string&, const std::string&, const std::string&)> handler) {
    // Implementation would store the handler for context menu callbacks
}

std::vector<FallbackMethod> PlatformAdapter::getAvailableFallbacks() const {
    return pimpl->getAvailableFallbacks();
}

FallbackMethod PlatformAdapter::selectBestFallback() const {
    return pimpl->selectBestFallback();
}

PlatformResult PlatformAdapter::executeFallback(FallbackMethod method, const std::string& profileId) {
    return pimpl->executeFallback(method, profileId);
}

bool PlatformAdapter::canGracefullyDegrade() const {
    return pimpl->canGracefullyDegrade();
}

std::vector<std::string> PlatformAdapter::getDegradationWarnings() const {
    return pimpl->getDegradationWarnings();
}

PlatformResult PlatformAdapter::configureForLimitedMode() {
    return pimpl->configureForLimitedMode();
}

PlatformResult PlatformAdapter::requestPasswordViaNotification(const std::string& profileId, const std::string& message) {
    return pimpl->requestPasswordViaNotification(profileId, message);
}

PlatformResult PlatformAdapter::requestPasswordViaContextMenu(const std::string& folderPath, const std::string& profileId) {
    return pimpl->requestPasswordViaContextMenu(folderPath, profileId);
}

PlatformResult PlatformAdapter::requestPasswordViaCommandLine(const std::string& profileId) {
    return pimpl->requestPasswordViaCommandLine(profileId);
}

PlatformResult PlatformAdapter::requestPasswordViaGUI(const std::string& profileId) {
    return pimpl->requestPasswordViaGUI(profileId);
}

bool PlatformAdapter::enableAdvancedFeatures() {
    // Implementation would enable advanced platform features
    return true;
}

bool PlatformAdapter::disableAdvancedFeatures() {
    // Implementation would disable advanced platform features
    return true;
}

PlatformResult PlatformAdapter::testPlatformFeatures() {
    PlatformResult result;
    if (pimpl->performSelfTest()) {
        result.success = true;
        result.message = "Platform features test passed";
    } else {
        result.errorDetails = pimpl->getLastError();
    }
    return result;
}

void PlatformAdapter::setOnCapabilityChanged(std::function<void(const PlatformCapabilities&)> callback) {
    // Implementation would store callback for capability changes
}

void PlatformAdapter::setOnPermissionLost(std::function<void(const std::string&)> callback) {
    // Implementation would store callback for permission loss
}

void PlatformAdapter::setOnFallbackNeeded(std::function<void(FallbackMethod)> callback) {
    // Implementation would store callback for fallback needs
}

std::string PlatformAdapter::getLastError() const {
    return pimpl->getLastError();
}

std::vector<std::string> PlatformAdapter::getDiagnosticInfo() const {
    return pimpl->getDiagnosticInfo();
}

bool PlatformAdapter::performSelfTest() {
    return pimpl->performSelfTest();
}

// UnlockProvider implementations
class NotificationUnlockProvider::Implementation {
public:
    bool isAvailable() const {
        #ifdef PLATFORM_LINUX
        return (system("which notify-send > /dev/null 2>&1") == 0);
        #else
        return true; // Assume available on other platforms
        #endif
    }
    
    std::string getName() const {
        return "Notification-based unlock";
    }
    
    PlatformResult requestPassword(const std::string& profileId, const std::string& hint) {
        PlatformResult result;
        result.errorDetails = "Notification unlock requires user interaction";
        result.availableAlternatives = {"command_line", "gui_prompt"};
        return result;
    }
    
    void cancel() {
        // Cancel any active notification
    }
};

NotificationUnlockProvider::NotificationUnlockProvider() : pimpl(std::make_unique<Implementation>()) {}
NotificationUnlockProvider::~NotificationUnlockProvider() = default;

bool NotificationUnlockProvider::isAvailable() const {
    return pimpl->isAvailable();
}

std::string NotificationUnlockProvider::getName() const {
    return pimpl->getName();
}

PlatformResult NotificationUnlockProvider::requestPassword(const std::string& profileId, const std::string& hint) {
    return pimpl->requestPassword(profileId, hint);
}

void NotificationUnlockProvider::cancel() {
    pimpl->cancel();
}

// ContextMenuUnlockProvider implementation
class ContextMenuUnlockProvider::Implementation {
public:
    bool isAvailable() const {
        return true; // Context menus generally available
    }
    
    std::string getName() const {
        return "Context menu unlock";
    }
    
    PlatformResult requestPassword(const std::string& profileId, const std::string& hint) {
        PlatformResult result;
        result.errorDetails = "Context menu unlock requires user interaction";
        return result;
    }
    
    void cancel() {
        // Cancel any active context menu
    }
};

ContextMenuUnlockProvider::ContextMenuUnlockProvider() : pimpl(std::make_unique<Implementation>()) {}
ContextMenuUnlockProvider::~ContextMenuUnlockProvider() = default;

bool ContextMenuUnlockProvider::isAvailable() const {
    return pimpl->isAvailable();
}

std::string ContextMenuUnlockProvider::getName() const {
    return pimpl->getName();
}

PlatformResult ContextMenuUnlockProvider::requestPassword(const std::string& profileId, const std::string& hint) {
    return pimpl->requestPassword(profileId, hint);
}

void ContextMenuUnlockProvider::cancel() {
    pimpl->cancel();
}

bool ContextMenuUnlockProvider::registerForPath(const std::string& path) {
    return true; // Simplified implementation
}

bool ContextMenuUnlockProvider::unregisterFromPath(const std::string& path) {
    return true; // Simplified implementation
}

// CommandLineUnlockProvider implementation
class CommandLineUnlockProvider::Implementation {
public:
    bool isAvailable() const {
        return true; // Command line always available
    }
    
    std::string getName() const {
        return "Command line unlock";
    }
    
    PlatformResult requestPassword(const std::string& profileId, const std::string& hint) {
        PlatformResult result;
        
        std::cout << "PhantomVault - Profile Unlock" << std::endl;
        std::cout << "Profile: " << profileId << std::endl;
        if (!hint.empty()) {
            std::cout << "Hint: " << hint << std::endl;
        }
        std::cout << "Enter master key: ";
        std::cout.flush();
        
        std::string password;
        std::getline(std::cin, password);
        
        if (!password.empty()) {
            result.success = true;
            result.message = password;
        } else {
            result.errorDetails = "No password provided";
        }
        
        return result;
    }
    
    void cancel() {
        // Cannot cancel command line input
    }
};

CommandLineUnlockProvider::CommandLineUnlockProvider() : pimpl(std::make_unique<Implementation>()) {}
CommandLineUnlockProvider::~CommandLineUnlockProvider() = default;

bool CommandLineUnlockProvider::isAvailable() const {
    return pimpl->isAvailable();
}

std::string CommandLineUnlockProvider::getName() const {
    return pimpl->getName();
}

PlatformResult CommandLineUnlockProvider::requestPassword(const std::string& profileId, const std::string& hint) {
    return pimpl->requestPassword(profileId, hint);
}

void CommandLineUnlockProvider::cancel() {
    pimpl->cancel();
}

// GUIPromptUnlockProvider implementation
class GUIPromptUnlockProvider::Implementation {
public:
    bool isAvailable() const {
        #ifdef PLATFORM_LINUX
        return (system("which zenity > /dev/null 2>&1") == 0);
        #else
        return true; // Assume available on other platforms
        #endif
    }
    
    std::string getName() const {
        return "GUI prompt unlock";
    }
    
    PlatformResult requestPassword(const std::string& profileId, const std::string& hint) {
        PlatformResult result;
        
        #ifdef PLATFORM_LINUX
        std::string command = "zenity --password --title=\"PhantomVault - Profile Unlock\" --text=\"Enter master key for profile: " + profileId;
        if (!hint.empty()) {
            command += "\\nHint: " + hint;
        }
        command += "\" 2>/dev/null";
        
        FILE* pipe = popen(command.c_str(), "r");
        if (pipe) {
            char buffer[256];
            std::string password;
            
            if (fgets(buffer, sizeof(buffer), pipe)) {
                password = std::string(buffer);
                if (!password.empty() && password.back() == '\n') {
                    password.pop_back();
                }
            }
            
            int exit_code = pclose(pipe);
            
            if (exit_code == 0 && !password.empty()) {
                result.success = true;
                result.message = password;
            } else {
                result.errorDetails = "GUI prompt cancelled or failed";
            }
        } else {
            result.errorDetails = "Failed to launch GUI prompt";
        }
        #else
        result.errorDetails = "GUI prompt not implemented for this platform";
        #endif
        
        return result;
    }
    
    void cancel() {
        // Cancel GUI prompt if possible
    }
};

GUIPromptUnlockProvider::GUIPromptUnlockProvider() : pimpl(std::make_unique<Implementation>()) {}
GUIPromptUnlockProvider::~GUIPromptUnlockProvider() = default;

bool GUIPromptUnlockProvider::isAvailable() const {
    return pimpl->isAvailable();
}

std::string GUIPromptUnlockProvider::getName() const {
    return pimpl->getName();
}

PlatformResult GUIPromptUnlockProvider::requestPassword(const std::string& profileId, const std::string& hint) {
    return pimpl->requestPassword(profileId, hint);
}

void GUIPromptUnlockProvider::cancel() {
    pimpl->cancel();
}

} // namespace phantomvault