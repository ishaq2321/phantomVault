/**
 * PhantomVault Platform Detection Manager Implementation
 * 
 * Basic implementation for platform detection.
 */

#include "platform_detection_manager.hpp"
#include <iostream>

#ifdef PLATFORM_LINUX
#include <X11/Xlib.h>
#include <cstdlib>
#elif PLATFORM_WINDOWS
#include <windows.h>
#elif PLATFORM_MACOS
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace phantomvault {

class PlatformDetectionManager::Implementation {
public:
    Implementation() : last_error_(), detected_platform_(Platform::UNKNOWN) {}
    
    Platform detectPlatform() {
        if (detected_platform_ != Platform::UNKNOWN) {
            return detected_platform_;
        }
        
        #ifdef PLATFORM_LINUX
        // Check for Wayland
        if (std::getenv("WAYLAND_DISPLAY")) {
            detected_platform_ = Platform::LINUX_WAYLAND;
        } else if (std::getenv("DISPLAY")) {
            detected_platform_ = Platform::LINUX_X11;
        } else {
            detected_platform_ = Platform::LINUX_X11; // Default to X11
        }
        #elif PLATFORM_WINDOWS
        detected_platform_ = Platform::WINDOWS;
        #elif PLATFORM_MACOS
        detected_platform_ = Platform::MACOS;
        #else
        detected_platform_ = Platform::UNKNOWN;
        #endif
        
        return detected_platform_;
    }
    
    std::string getPlatformName() const {
        switch (detected_platform_) {
            case Platform::LINUX_X11:
                return "Linux (X11)";
            case Platform::LINUX_WAYLAND:
                return "Linux (Wayland)";
            case Platform::MACOS:
                return "macOS";
            case Platform::WINDOWS:
                return "Windows";
            default:
                return "Unknown";
        }
    }
    
    std::string getPlatformVersion() const {
        // TODO: Implement platform version detection
        return "Unknown";
    }
    
    bool supportsInvisibleLogging() const {
        switch (detected_platform_) {
            case Platform::LINUX_X11:
            case Platform::WINDOWS:
            case Platform::MACOS:
                return true;
            case Platform::LINUX_WAYLAND:
                return false; // Wayland has security restrictions
            default:
                return false;
        }
    }
    
    bool supportsNotifications() const {
        return true; // Most platforms support notifications
    }
    
    bool supportsSystemTray() const {
        return true; // Most platforms support system tray
    }
    
    bool supportsAutoStart() const {
        return true; // Most platforms support auto-start
    }
    
    std::vector<UnlockMethod> getSupportedUnlockMethods() const {
        std::vector<UnlockMethod> methods;
        
        if (supportsInvisibleLogging()) {
            methods.push_back(UnlockMethod::INVISIBLE_LOGGING);
        }
        
        if (supportsNotifications()) {
            methods.push_back(UnlockMethod::NOTIFICATION_PROMPT);
        }
        
        methods.push_back(UnlockMethod::LEFT_CLICK);
        methods.push_back(UnlockMethod::MANUAL_INPUT);
        
        return methods;
    }
    
    UnlockMethod getRecommendedUnlockMethod() const {
        if (supportsInvisibleLogging()) {
            return UnlockMethod::INVISIBLE_LOGGING;
        } else if (supportsNotifications()) {
            return UnlockMethod::NOTIFICATION_PROMPT;
        } else {
            return UnlockMethod::MANUAL_INPUT;
        }
    }
    
    void setPreferredUnlockMethod(UnlockMethod method) {
        preferred_unlock_method_ = method;
    }
    
    UnlockMethod getPreferredUnlockMethod() const {
        return preferred_unlock_method_;
    }
    
    std::string getPlatformGuidance() const {
        switch (detected_platform_) {
            case Platform::LINUX_X11:
                return "X11 detected. Invisible keyboard logging is supported. "
                       "Press Ctrl+Alt+V and type your password anywhere to unlock folders.";
            case Platform::LINUX_WAYLAND:
                return "Wayland detected. For security reasons, invisible keyboard logging is not supported. "
                       "Press Ctrl+Alt+V to receive a notification prompt for password input.";
            case Platform::MACOS:
                return "macOS detected. Invisible keyboard logging is supported with accessibility permissions. "
                       "You may need to grant accessibility permissions in System Preferences.";
            case Platform::WINDOWS:
                return "Windows detected. Invisible keyboard logging is supported. "
                       "Press Ctrl+Alt+V and type your password anywhere to unlock folders.";
            default:
                return "Platform not fully supported. Manual unlock methods are available.";
        }
    }
    
    std::string getUnlockMethodDescription(UnlockMethod method) const {
        switch (method) {
            case UnlockMethod::INVISIBLE_LOGGING:
                return "Invisible keyboard logging - type password anywhere after Ctrl+Alt+V";
            case UnlockMethod::NOTIFICATION_PROMPT:
                return "Notification prompt - receive notification to enter password";
            case UnlockMethod::LEFT_CLICK:
                return "Left-click unlock - click to unlock specific folders";
            case UnlockMethod::MANUAL_INPUT:
                return "Manual input - enter password in dialog box";
            default:
                return "Unknown unlock method";
        }
    }
    
    std::string getSetupInstructions() const {
        // TODO: Implement platform-specific setup instructions
        return "Setup instructions not implemented yet";
    }
    
    std::string getSystemInfo() const {
        return getPlatformName() + " " + getPlatformVersion();
    }
    
    bool checkPermissions() const {
        // TODO: Implement permission checking
        return true;
    }
    
    std::vector<std::string> getRequiredPermissions() const {
        std::vector<std::string> permissions;
        
        switch (detected_platform_) {
            case Platform::MACOS:
                permissions.push_back("Accessibility");
                permissions.push_back("Full Disk Access");
                break;
            case Platform::LINUX_X11:
            case Platform::LINUX_WAYLAND:
                permissions.push_back("Input monitoring");
                break;
            case Platform::WINDOWS:
                permissions.push_back("Administrator privileges");
                break;
            default:
                break;
        }
        
        return permissions;
    }
    
    std::string getLastError() const {
        return last_error_;
    }
    
private:
    std::string last_error_;
    Platform detected_platform_;
    UnlockMethod preferred_unlock_method_ = UnlockMethod::INVISIBLE_LOGGING;
};

PlatformDetectionManager::PlatformDetectionManager() : pimpl(std::make_unique<Implementation>()) {}
PlatformDetectionManager::~PlatformDetectionManager() = default;

PlatformDetectionManager::Platform PlatformDetectionManager::detectPlatform() {
    return pimpl->detectPlatform();
}

std::string PlatformDetectionManager::getPlatformName() const {
    return pimpl->getPlatformName();
}

std::string PlatformDetectionManager::getPlatformVersion() const {
    return pimpl->getPlatformVersion();
}

bool PlatformDetectionManager::supportsInvisibleLogging() const {
    return pimpl->supportsInvisibleLogging();
}

bool PlatformDetectionManager::supportsNotifications() const {
    return pimpl->supportsNotifications();
}

bool PlatformDetectionManager::supportsSystemTray() const {
    return pimpl->supportsSystemTray();
}

bool PlatformDetectionManager::supportsAutoStart() const {
    return pimpl->supportsAutoStart();
}

std::vector<PlatformDetectionManager::UnlockMethod> PlatformDetectionManager::getSupportedUnlockMethods() const {
    return pimpl->getSupportedUnlockMethods();
}

PlatformDetectionManager::UnlockMethod PlatformDetectionManager::getRecommendedUnlockMethod() const {
    return pimpl->getRecommendedUnlockMethod();
}

void PlatformDetectionManager::setPreferredUnlockMethod(UnlockMethod method) {
    pimpl->setPreferredUnlockMethod(method);
}

PlatformDetectionManager::UnlockMethod PlatformDetectionManager::getPreferredUnlockMethod() const {
    return pimpl->getPreferredUnlockMethod();
}

std::string PlatformDetectionManager::getPlatformGuidance() const {
    return pimpl->getPlatformGuidance();
}

std::string PlatformDetectionManager::getUnlockMethodDescription(UnlockMethod method) const {
    return pimpl->getUnlockMethodDescription(method);
}

std::string PlatformDetectionManager::getSetupInstructions() const {
    return pimpl->getSetupInstructions();
}

std::string PlatformDetectionManager::getSystemInfo() const {
    return pimpl->getSystemInfo();
}

bool PlatformDetectionManager::checkPermissions() const {
    return pimpl->checkPermissions();
}

std::vector<std::string> PlatformDetectionManager::getRequiredPermissions() const {
    return pimpl->getRequiredPermissions();
}

std::string PlatformDetectionManager::getLastError() const {
    return pimpl->getLastError();
}

} // namespace phantomvault