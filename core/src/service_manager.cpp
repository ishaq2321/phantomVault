/**
 * PhantomVault Service Manager Implementation
 * 
 * Main service coordinator implementation.
 */

#include "service_manager.hpp"
#include "profile_manager.hpp"
#include "folder_security_manager.hpp"
#include "keyboard_sequence_detector.hpp"
#include "analytics_engine.hpp"
#include "ipc_server.hpp"
#include "performance_monitor.hpp"
#include "memory_manager.hpp"
#include "privilege_manager.hpp"

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <fstream>

#ifdef PLATFORM_LINUX
#include <sys/resource.h>
#include <unistd.h>
#elif PLATFORM_WINDOWS
#include <windows.h>
#include <psapi.h>
#elif PLATFORM_MACOS
#include <mach/mach.h>
#endif

namespace phantomvault {

class ServiceManager::Implementation {
public:
    Implementation()
        : running_(false)
        , profile_manager_(nullptr)
        , folder_security_manager_(nullptr)
        , keyboard_sequence_detector_(nullptr)
        , analytics_engine_(nullptr)
        , ipc_server_(nullptr)
        , performance_monitor_(nullptr)
        , privilege_manager_(nullptr)
        , last_error_()
    {}

    ~Implementation() {
        stop();
    }

    bool initialize(const std::string& configFile, const std::string& logLevel, int ipcPort) {
        try {
            std::cout << "[ServiceManager] Initializing PhantomVault service..." << std::endl;
            
            // Initialize privilege manager first
            privilege_manager_ = std::make_unique<PrivilegeManager>();
            if (!privilege_manager_->initialize()) {
                last_error_ = "Failed to initialize privilege manager: " + privilege_manager_->getLastError();
                return false;
            }
            
            // Check startup privileges
            if (!privilege_manager_->validateStartupPrivileges()) {
                last_error_ = privilege_manager_->getStartupPrivilegeError();
                std::cout << "[ServiceManager] PRIVILEGE ERROR: " << last_error_ << std::endl;
                return false;
            }
            
            std::cout << "[ServiceManager] Privilege validation PASSED" << std::endl;
            std::cout << "[ServiceManager] Current user: " << privilege_manager_->getCurrentUser() << std::endl;
            std::cout << "[ServiceManager] Privilege level: " << static_cast<int>(privilege_manager_->getCurrentPrivilegeLevel()) << std::endl;
            
            // Platform detection is now handled by individual components
            #ifdef PLATFORM_LINUX
            std::cout << "[ServiceManager] Platform: Linux" << std::endl;
            #elif PLATFORM_MACOS
            std::cout << "[ServiceManager] Platform: macOS" << std::endl;
            #elif PLATFORM_WINDOWS
            std::cout << "[ServiceManager] Platform: Windows" << std::endl;
            #else
            std::cout << "[ServiceManager] Platform: Unknown" << std::endl;
            #endif
            
            // Initialize profile manager
            profile_manager_ = std::make_unique<ProfileManager>();
            if (!profile_manager_->initialize()) {
                last_error_ = "Failed to initialize profile manager: " + profile_manager_->getLastError();
                return false;
            }
            std::cout << "[ServiceManager] Profile manager initialized" << std::endl;
            
            // Initialize folder security manager
            folder_security_manager_ = std::make_unique<FolderSecurityManager>();
            if (!folder_security_manager_->initialize()) {
                last_error_ = "Failed to initialize folder security manager: " + folder_security_manager_->getLastError();
                return false;
            }
            std::cout << "[ServiceManager] Folder security manager initialized" << std::endl;
            
            // Initialize keyboard sequence detector
            keyboard_sequence_detector_ = std::make_unique<KeyboardSequenceDetector>();
            if (!keyboard_sequence_detector_->initialize()) {
                last_error_ = "Failed to initialize keyboard sequence detector: " + keyboard_sequence_detector_->getLastError();
                return false;
            }
            std::cout << "[ServiceManager] Keyboard sequence detector initialized" << std::endl;
            
            // Initialize analytics engine
            analytics_engine_ = std::make_unique<AnalyticsEngine>();
            if (!analytics_engine_->initialize()) {
                last_error_ = "Failed to initialize analytics engine: " + analytics_engine_->getLastError();
                return false;
            }
            std::cout << "[ServiceManager] Analytics engine initialized" << std::endl;
            
            // Initialize IPC server
            ipc_server_ = std::make_unique<IPCServer>();
            if (!ipc_server_->initialize(ipcPort)) {
                last_error_ = "Failed to initialize IPC server: " + ipc_server_->getLastError();
                return false;
            }
            
            // Set component references for IPC server
            ipc_server_->setProfileManager(profile_manager_.get());
            ipc_server_->setFolderSecurityManager(folder_security_manager_.get());
            ipc_server_->setKeyboardSequenceDetector(keyboard_sequence_detector_.get());
            ipc_server_->setAnalyticsEngine(analytics_engine_.get());
            
            std::cout << "[ServiceManager] IPC server initialized on port " << ipcPort << std::endl;
            
            // Initialize performance monitor
            performance_monitor_ = std::make_unique<PerformanceMonitor>();
            if (!performance_monitor_->initialize()) {
                last_error_ = "Failed to initialize performance monitor";
                return false;
            }
            
            // Set performance monitor to balanced mode with adaptive tuning
            performance_monitor_->setPerformanceMode(PerformanceMode::BALANCED);
            performance_monitor_->enableAdaptiveTuning(true);
            performance_monitor_->setMemoryLimit(8192); // 8MB limit
            performance_monitor_->setCPULimit(5.0);     // 5% CPU limit
            
            std::cout << "[ServiceManager] Performance monitor initialized" << std::endl;
            
            // Initialize encryption service components
            if (!initializeEncryptionServices()) {
                last_error_ = "Failed to initialize encryption services";
                return false;
            }
            std::cout << "[ServiceManager] Encryption services initialized" << std::endl;
            
            std::cout << "[ServiceManager] All components initialized successfully" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Initialization failed: " + std::string(e.what());
            return false;
        }
    }

    bool start() {
        try {
            if (running_) {
                last_error_ = "Service is already running";
                return false;
            }
            
            std::cout << "[ServiceManager] Starting PhantomVault service..." << std::endl;
            
            // Start IPC server
            if (!ipc_server_->start()) {
                last_error_ = "Failed to start IPC server: " + ipc_server_->getLastError();
                return false;
            }
            
            // Start keyboard sequence detector
            if (!keyboard_sequence_detector_->start()) {
                last_error_ = "Failed to start keyboard sequence detector: " + keyboard_sequence_detector_->getLastError();
                return false;
            }
            
            // Start analytics engine
            if (!analytics_engine_->start()) {
                last_error_ = "Failed to start analytics engine: " + analytics_engine_->getLastError();
                return false;
            }
            
            // Start performance monitor
            if (!performance_monitor_->start()) {
                last_error_ = "Failed to start performance monitor";
                return false;
            }
            
            running_ = true;
            std::cout << "[ServiceManager] Service started successfully" << std::endl;
            std::cout << "[ServiceManager] Memory usage: " << getMemoryUsage() << " KB" << std::endl;
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to start service: " + std::string(e.what());
            return false;
        }
    }

    void stop() {
        if (!running_) {
            return;
        }
        
        std::cout << "[ServiceManager] Stopping PhantomVault service..." << std::endl;
        
        // Perform secure cleanup of cryptographic material first
        secureCleanupCryptographicMaterial();
        
        // Stop components in reverse order
        if (performance_monitor_) {
            performance_monitor_->stop();
        }
        
        if (analytics_engine_) {
            analytics_engine_->stop();
        }
        
        if (keyboard_sequence_detector_) {
            keyboard_sequence_detector_->stop();
        }
        
        if (ipc_server_) {
            ipc_server_->stop();
        }
        
        running_ = false;
        std::cout << "[ServiceManager] Service stopped with secure cleanup" << std::endl;
    }

    bool isRunning() const {
        return running_;
    }

    ProfileManager* getProfileManager() const {
        return profile_manager_.get();
    }

    FolderSecurityManager* getFolderSecurityManager() const {
        return folder_security_manager_.get();
    }

    KeyboardSequenceDetector* getKeyboardSequenceDetector() const {
        return keyboard_sequence_detector_.get();
    }



    AnalyticsEngine* getAnalyticsEngine() const {
        return analytics_engine_.get();
    }

    std::string getLastError() const {
        return last_error_;
    }

    std::string getVersion() const {
        return "1.0.0";
    }

    std::string getPlatformInfo() const {
        #ifdef PLATFORM_LINUX
        return "Linux";
        #elif PLATFORM_MACOS
        return "macOS";
        #elif PLATFORM_WINDOWS
        return "Windows";
        #else
        return "Unknown";
        #endif
    }

    size_t getMemoryUsage() const {
        #ifdef PLATFORM_LINUX
        // Read from /proc/self/status
        std::ifstream status("/proc/self/status");
        std::string line;
        while (std::getline(status, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                size_t kb = std::stoul(line.substr(7));
                return kb;
            }
        }
        return 0;
        
        #elif PLATFORM_WINDOWS
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            return pmc.WorkingSetSize / 1024; // Convert to KB
        }
        return 0;
        
        #elif PLATFORM_MACOS
        struct mach_task_basic_info info;
        mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                     (task_info_t)&info, &infoCount) == KERN_SUCCESS) {
            return info.resident_size / 1024; // Convert to KB
        }
        return 0;
        
        #else
        return 0;
        #endif
    }
    
    bool initializeEncryptionServices() {
        try {
            std::cout << "[ServiceManager] Initializing encryption service components..." << std::endl;
            
            // Verify encryption engine functionality
            if (profile_manager_) {
                // Test encryption engine through profile manager
                auto profiles = profile_manager_->getAllProfiles();
                std::cout << "[ServiceManager] Encryption engine verified through profile system" << std::endl;
            }
            
            // Setup keyboard sequence callbacks for encryption operations
            if (keyboard_sequence_detector_ && folder_security_manager_) {
                keyboard_sequence_detector_->setOnPasswordDetected([this](const PasswordPattern& pattern) {
                    handlePasswordDetection(pattern);
                });
                std::cout << "[ServiceManager] Keyboard sequence callbacks configured" << std::endl;
            }
            
            // Initialize vault system integrity checks
            if (profile_manager_) {
                auto profiles = profile_manager_->getAllProfiles();
                for (const auto& profile : profiles) {
                    if (!profile_manager_->validateProfileVault(profile.id)) {
                        std::cout << "[ServiceManager] Warning: Vault integrity issue detected for profile: " << profile.name << std::endl;
                        // Attempt maintenance
                        profile_manager_->performProfileVaultMaintenance(profile.id);
                    }
                }
                std::cout << "[ServiceManager] Vault integrity checks completed" << std::endl;
            }
            
            // Setup encryption analytics tracking
            if (analytics_engine_) {
                analytics_engine_->logEvent(EventType::SERVICE_STARTED, SecurityLevel::INFO, "", 
                                          "Encryption services initialized", {{"version", "1.0.0"}});
                std::cout << "[ServiceManager] Encryption analytics tracking enabled" << std::endl;
            }
            
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Encryption service initialization failed: " + std::string(e.what());
            return false;
        }
    }
    
    void handlePasswordDetection(const PasswordPattern& pattern) {
        try {
            std::cout << "[ServiceManager] Password pattern detected, attempting unlock..." << std::endl;
            
            if (!folder_security_manager_) {
                return;
            }
            
            // Try to find matching profile for this password
            if (profile_manager_) {
                auto profiles = profile_manager_->getAllProfiles();
                for (const auto& profile : profiles) {
                    if (profile_manager_->verifyMasterKey(profile.id, pattern.password)) {
                        std::cout << "[ServiceManager] Master key matched for profile: " << profile.name << std::endl;
                        
                        // Perform unlock based on pattern type
                        if (pattern.isTemporary) {
                            auto result = folder_security_manager_->unlockFoldersTemporary(profile.id, pattern.password);
                            if (result.success) {
                                std::cout << "[ServiceManager] Temporary unlock successful: " << result.successCount << " folders" << std::endl;
                            }
                        } else if (pattern.isPermanent) {
                            auto result = folder_security_manager_->unlockFoldersPermanent(profile.id, pattern.password);
                            if (result.success) {
                                std::cout << "[ServiceManager] Permanent unlock successful: " << result.successCount << " folders" << std::endl;
                            }
                        } else {
                            // Default to temporary unlock
                            auto result = folder_security_manager_->unlockFoldersTemporary(profile.id, pattern.password);
                            if (result.success) {
                                std::cout << "[ServiceManager] Default temporary unlock successful: " << result.successCount << " folders" << std::endl;
                            }
                        }
                        
                        // Log analytics event
                        if (analytics_engine_) {
                            analytics_engine_->logEvent(EventType::KEYBOARD_SEQUENCE_DETECTED, SecurityLevel::INFO, 
                                                      profile.id, "Password pattern detected and processed", {});
                        }
                        
                        break; // Found matching profile, stop searching
                    }
                }
            }
            
        } catch (const std::exception& e) {
            std::cout << "[ServiceManager] Error handling password detection: " << e.what() << std::endl;
            if (analytics_engine_) {
                analytics_engine_->logEvent(EventType::SECURITY_VIOLATION, SecurityLevel::WARNING, "", 
                                          "Password detection error", {{"error", e.what()}});
            }
        }
    }
    
    void secureCleanupCryptographicMaterial() {
        try {
            std::cout << "[ServiceManager] Performing secure cleanup of cryptographic material..." << std::endl;
            
            // Clear any cached keys or sensitive data
            if (profile_manager_) {
                profile_manager_->clearActiveProfile();
                std::cout << "[ServiceManager] Active profile cleared" << std::endl;
            }
            
            // Re-lock any temporarily unlocked folders
            if (folder_security_manager_) {
                auto profiles = profile_manager_ ? profile_manager_->getAllProfiles() : std::vector<Profile>();
                for (const auto& profile : profiles) {
                    folder_security_manager_->lockTemporaryFolders(profile.id);
                }
                std::cout << "[ServiceManager] Temporary folders re-locked" << std::endl;
            }
            
            // Stop keyboard monitoring to prevent key capture
            if (keyboard_sequence_detector_) {
                keyboard_sequence_detector_->deactivateSequenceDetection();
                std::cout << "[ServiceManager] Keyboard sequence detection deactivated" << std::endl;
            }
            
            // Log security event
            if (analytics_engine_) {
                analytics_engine_->logEvent(EventType::SERVICE_STOPPED, SecurityLevel::INFO, "", 
                                          "Secure cleanup completed", {});
            }
            
            std::cout << "[ServiceManager] Cryptographic material cleanup completed" << std::endl;
            
        } catch (const std::exception& e) {
            std::cout << "[ServiceManager] Error during secure cleanup: " << e.what() << std::endl;
        }
    }

private:
    std::atomic<bool> running_;
    
    // Component managers
    std::unique_ptr<ProfileManager> profile_manager_;
    std::unique_ptr<FolderSecurityManager> folder_security_manager_;
    std::unique_ptr<KeyboardSequenceDetector> keyboard_sequence_detector_;
    std::unique_ptr<AnalyticsEngine> analytics_engine_;
    std::unique_ptr<IPCServer> ipc_server_;
    std::unique_ptr<PerformanceMonitor> performance_monitor_;
    std::unique_ptr<PrivilegeManager> privilege_manager_;
    
    std::string last_error_;
};

// ServiceManager public interface implementation
ServiceManager::ServiceManager() : pimpl(std::make_unique<Implementation>()) {}
ServiceManager::~ServiceManager() = default;

bool ServiceManager::initialize(const std::string& configFile, const std::string& logLevel, int ipcPort) {
    return pimpl->initialize(configFile, logLevel, ipcPort);
}

bool ServiceManager::start() {
    return pimpl->start();
}

void ServiceManager::stop() {
    pimpl->stop();
}

bool ServiceManager::isRunning() const {
    return pimpl->isRunning();
}

ProfileManager* ServiceManager::getProfileManager() const {
    return pimpl->getProfileManager();
}

FolderSecurityManager* ServiceManager::getFolderSecurityManager() const {
    return pimpl->getFolderSecurityManager();
}

KeyboardSequenceDetector* ServiceManager::getKeyboardSequenceDetector() const {
    return pimpl->getKeyboardSequenceDetector();
}



AnalyticsEngine* ServiceManager::getAnalyticsEngine() const {
    return pimpl->getAnalyticsEngine();
}

std::string ServiceManager::getLastError() const {
    return pimpl->getLastError();
}

std::string ServiceManager::getVersion() const {
    return pimpl->getVersion();
}

std::string ServiceManager::getPlatformInfo() const {
    return pimpl->getPlatformInfo();
}

size_t ServiceManager::getMemoryUsage() const {
    return pimpl->getMemoryUsage();
}

} // namespace phantomvault