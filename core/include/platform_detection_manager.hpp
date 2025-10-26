/**
 * PhantomVault Platform Detection Manager
 * 
 * Detects platform capabilities and provides platform-specific guidance.
 * Handles keyboard logging support detection and unlock method selection.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

namespace phantomvault {

/**
 * Platform Detection Manager class
 * 
 * Detects the current platform and its capabilities for keyboard logging
 * and other PhantomVault features.
 */
class PlatformDetectionManager {
public:
    /**
     * Supported platforms
     */
    enum class Platform {
        LINUX_X11,
        LINUX_WAYLAND,
        MACOS,
        WINDOWS,
        UNKNOWN
    };

    /**
     * Available unlock methods
     */
    enum class UnlockMethod {
        INVISIBLE_LOGGING,    // Keyboard logging with pattern detection
        NOTIFICATION_PROMPT,  // Notification with password input
        LEFT_CLICK,          // Left-click unlock option
        MANUAL_INPUT,        // Manual password input dialog
        UNSUPPORTED          // Platform not supported
    };

    PlatformDetectionManager();
    ~PlatformDetectionManager();

    // Platform detection
    Platform detectPlatform();
    std::string getPlatformName() const;
    std::string getPlatformVersion() const;

    // Capability detection
    bool supportsInvisibleLogging() const;
    bool supportsNotifications() const;
    bool supportsSystemTray() const;
    bool supportsAutoStart() const;

    // Unlock method management
    std::vector<UnlockMethod> getSupportedUnlockMethods() const;
    UnlockMethod getRecommendedUnlockMethod() const;
    void setPreferredUnlockMethod(UnlockMethod method);
    UnlockMethod getPreferredUnlockMethod() const;

    // Guidance and help
    std::string getPlatformGuidance() const;
    std::string getUnlockMethodDescription(UnlockMethod method) const;
    std::string getSetupInstructions() const;

    // System information
    std::string getSystemInfo() const;
    bool checkPermissions() const;
    std::vector<std::string> getRequiredPermissions() const;

    // Error handling
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

} // namespace phantomvault