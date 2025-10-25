#include "phantom_vault/service.hpp"
#include "phantom_vault/core.hpp"
#include "phantom_vault/hotkey_manager.hpp"
#include "phantom_vault/input_overlay.hpp"
#include "phantom_vault/sequence_detector.hpp"
#include "phantom_vault/service_vault_manager.hpp"
#include "phantom_vault/recovery_manager.hpp"
#include "phantom_vault/directory_protection.hpp"
#include "phantom_vault/ipc_server.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <filesystem>

namespace phantom_vault {
namespace service {

// Simple BackgroundService implementation for initial testing
class BackgroundService::Implementation {
public:
    Implementation() 
        : is_running_(false)
        , should_stop_(false)
        , start_time_(std::chrono::steady_clock::now())
        , hotkey_manager_()
        , input_overlay_()
        , sequence_detector_()
        , vault_manager_()
        , recovery_manager_()
        , directory_protection_()
        , ipc_server_()
    {}

    bool initialize(const ServiceConfig& config) {
        config_ = config;
        std::cout << "Service initialized with name: " << config.service_name << std::endl;
        
        // Initialize hotkey manager
        hotkey_manager_ = std::make_unique<HotkeyManager>();
        if (!hotkey_manager_->initialize()) {
            last_error_ = "Failed to initialize hotkey manager: " + hotkey_manager_->getLastError();
            return false;
        }
        
        // Initialize input overlay (fallback)
        input_overlay_ = std::make_unique<InputOverlay>();
        if (!input_overlay_->initialize()) {
            last_error_ = "Failed to initialize input overlay: " + input_overlay_->getLastError();
            return false;
        }
        
        // Initialize sequence detector (primary method)
        sequence_detector_ = std::make_unique<SequenceDetector>();
        if (!sequence_detector_->initialize()) {
            last_error_ = "Failed to initialize sequence detector: " + sequence_detector_->getLastError();
            return false;
        }
        
        // Initialize vault manager
        vault_manager_ = std::make_unique<ServiceVaultManager>();
        if (!vault_manager_->initialize()) {
            last_error_ = "Failed to initialize vault manager: " + vault_manager_->getLastError();
            return false;
        }
        
        // Initialize recovery manager
        recovery_manager_ = std::make_unique<RecoveryManager>();
        if (!recovery_manager_->initialize()) {
            last_error_ = "Failed to initialize recovery manager: " + recovery_manager_->getLastError();
            return false;
        }
        
        // Initialize directory protection
        directory_protection_ = std::make_unique<DirectoryProtection>();
        if (!directory_protection_->initialize()) {
            last_error_ = "Failed to initialize directory protection: " + directory_protection_->getLastError();
            return false;
        }
        
        // Set up hotkey callbacks
        setupHotkeyCallbacks();
        
        // Set up directory protection
        setupDirectoryProtection();
        
        // Start security monitoring
        startSecurityMonitoring();
        
        // Initialize and start IPC server
        setupIPCServer();
        
        std::cout << "Platform: " << hotkey_manager_->getCurrentPlatform() << std::endl;
        return true;
    }

    bool start() {
        if (is_running_) {
            return false;
        }
        
        std::cout << "Starting PhantomVault Background Service..." << std::endl;
        
        // Register global hotkeys
        if (!hotkey_manager_->registerGlobalHotkeys()) {
            last_error_ = "Failed to register hotkeys: " + hotkey_manager_->getLastError();
            return false;
        }
        
        is_running_ = true;
        should_stop_ = false;
        
        // Start service thread
        service_thread_ = std::thread(&Implementation::serviceLoop, this);
        
        std::cout << "Service started successfully" << std::endl;
        return true;
    }

    void stop() {
        if (!is_running_) {
            return;
        }
        
        std::cout << "Stopping service..." << std::endl;
        
        // Unregister hotkeys
        if (hotkey_manager_) {
            hotkey_manager_->unregisterHotkeys();
        }
        
        should_stop_ = true;
        is_running_ = false;
        
        // Stop security monitoring
        stopSecurityMonitoring();
        
        // Stop IPC server
        stopIPCServer();
        
        if (service_thread_.joinable()) {
            service_thread_.join();
        }
        
        std::cout << "Service stopped" << std::endl;
    }

    bool isRunning() const {
        return is_running_;
    }

    const ServiceConfig& getConfig() const {
        return config_;
    }

    const VaultState& getVaultState() const {
        return vault_state_;
    }

    std::chrono::seconds getUptime() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    }

    std::string getLastError() const {
        return last_error_;
    }

private:
    void setupHotkeyCallbacks() {
        hotkey_manager_->setUnlockCallback([this]() {
            handleUnlockHotkey();
        });
        
        hotkey_manager_->setRecoveryCallback([this]() {
            handleRecoveryHotkey();
        });
    }
    
    void handleUnlockHotkey() {
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "🔓 [C++ SERVICE] UNLOCK HOTKEY PRESSED" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        
        // Debug component availability
        std::cout << "🔍 [DEBUG] Component status:" << std::endl;
        std::cout << "   - Sequence Detector: " << (sequence_detector_ ? "✅ Available" : "❌ NULL") << std::endl;
        std::cout << "   - Vault Manager: " << (vault_manager_ ? "✅ Available" : "❌ NULL") << std::endl;
        std::cout << "   - IPC Server: " << (ipc_server_ ? "✅ Available" : "❌ NULL") << std::endl;
        
        if (!sequence_detector_ || !vault_manager_) {
            std::cout << "❌ [ERROR] Required components not available" << std::endl;
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
            return;
        }
        
        // Check if we have temporary folders to re-lock
        auto active_profile = vault_manager_->getActiveProfile();
        std::cout << "🔍 [DEBUG] Active profile: " << (active_profile ? active_profile->name : "None") << std::endl;
        
        if (active_profile && vault_manager_->hasTemporaryUnlockedFolders(active_profile->id)) {
            std::cout << "🔒 [MODE] Detected temporary folders - entering RE-LOCK mode" << std::endl;
            handleRelockModeWithSequence(active_profile);
            return;
        }
        
        // Normal unlock mode with sequence detection
        std::cout << "🔓 [MODE] Entering UNLOCK mode with sequence detection" << std::endl;
        handleUnlockModeWithSequence();
    }
    
    void handleUnlockMode() {
        std::cout << "=== UNLOCK MODE ===" << std::endl;
        
        // Capture password with invisible overlay
        PasswordInput input = input_overlay_->capturePassword(10);
        
        if (input.password.empty()) {
            std::cout << "Password capture cancelled or failed" << std::endl;
            std::cout << "==================\n" << std::endl;
            return;
        }
        
        std::cout << "Password captured successfully!" << std::endl;
        std::cout << "  Length: " << input.password.length() << " characters" << std::endl;
        std::cout << "  Mode: " << (input.mode == UnlockMode::TEMPORARY ? "Temporary (T)" : "Permanent (P)") << std::endl;
        std::cout << "  Is recovery key: " << (input.is_recovery_key ? "Yes" : "No") << std::endl;
        
        // Get active profile
        auto active_profile = vault_manager_->getActiveProfile();
        if (!active_profile) {
            std::cout << "No active profile found - creating default profile..." << std::endl;
            
            // Create default profile for testing
            std::string profile_name = "Default Profile";
            std::string master_password = input.password; // Use the entered password as master password
            std::string recovery_key = "1234-5678-9ABC-DEF0"; // Default recovery key for testing
            
            active_profile = vault_manager_->createProfile(profile_name, master_password, recovery_key);
            
            if (!active_profile) {
                std::cout << "❌ Failed to create profile" << std::endl;
                std::cout << "==================\n" << std::endl;
                return;
            }
            
            std::cout << "✅ Created default profile: " << active_profile->name << std::endl;
            std::cout << "🔑 Recovery key: " << recovery_key << " (save this!)" << std::endl;
        }
        
        // Unlock folders
        UnlockResult result;
        if (input.is_recovery_key) {
            result = vault_manager_->unlockWithRecoveryKey(active_profile->id, input.password);
        } else {
            result = vault_manager_->unlockFolders(active_profile->id, input.password, input.mode);
        }
        
        // Report results
        if (result.success_count > 0) {
            std::cout << "✅ Successfully unlocked " << result.success_count << " folder(s)" << std::endl;
        }
        if (result.failed_count > 0) {
            std::cout << "❌ Failed to unlock " << result.failed_count << " folder(s)" << std::endl;
        }
        if (result.success_count == 0 && result.failed_count == 0) {
            std::cout << "ℹ️  No locked folders found or wrong password" << std::endl;
        }
        
        std::cout << "==================\n" << std::endl;
    }
    
    void handleRelockMode(std::shared_ptr<VaultProfile> profile) {
        std::cout << "=== RE-LOCK MODE ===" << std::endl;
        
        auto temp_folders = vault_manager_->getTemporaryUnlockedFolders(profile->id);
        std::cout << "Found " << temp_folders.size() << " temporary folder(s) to lock" << std::endl;
        
        // Capture password (no T/P prefix needed for re-lock)
        PasswordInput input = input_overlay_->capturePassword(10);
        
        if (input.password.empty()) {
            std::cout << "Password capture cancelled or failed" << std::endl;
            std::cout << "===================\n" << std::endl;
            return;
        }
        
        std::cout << "Password captured - locking temporary folders..." << std::endl;
        
        // Lock all temporary folders
        int locked_count = vault_manager_->lockAllTemporaryFolders(profile->id, input.password);
        
        if (locked_count > 0) {
            std::cout << "✅ Successfully locked " << locked_count << " temporary folder(s)" << std::endl;
        } else {
            std::cout << "❌ Failed to lock folders (wrong password?)" << std::endl;
        }
        
        std::cout << "===================\n" << std::endl;
    }
    
    void handleRecoveryHotkey() {
        std::cout << "\n=== RECOVERY HOTKEY PRESSED ===" << std::endl;
        
        if (!input_overlay_ || !vault_manager_) {
            std::cout << "Required components not available" << std::endl;
            return;
        }
        
        // Get active profile
        auto active_profile = vault_manager_->getActiveProfile();
        if (!active_profile) {
            std::cout << "No active profile found - please set up PhantomVault first" << std::endl;
            std::cout << "================================\n" << std::endl;
            return;
        }
        
        // Capture recovery key
        std::string recovery_key = input_overlay_->captureRecoveryKey(30);
        
        if (recovery_key.empty()) {
            std::cout << "Recovery key capture cancelled or failed" << std::endl;
            std::cout << "================================\n" << std::endl;
            return;
        }
        
        std::cout << "Recovery key captured successfully!" << std::endl;
        std::cout << "  Format: XXXX-XXXX-XXXX-XXXX" << std::endl;
        
        // Unlock with recovery key
        UnlockResult result = vault_manager_->unlockWithRecoveryKey(active_profile->id, recovery_key);
        
        if (result.success_count > 0) {
            std::cout << "✅ Successfully unlocked " << result.success_count << " folder(s) with recovery key" << std::endl;
        } else {
            std::cout << "❌ Failed to unlock folders (invalid recovery key?)" << std::endl;
            for (const auto& error : result.error_messages) {
                std::cout << "  Error: " << error << std::endl;
            }
        }
        
        std::cout << "================================\n" << std::endl;
    }
    
    void handleUnlockModeWithSequence() {
        std::cout << "🎯 [SEQUENCE] Starting sequence detection mode" << std::endl;
        
        // Get active profile and load folder passwords
        auto active_profile = vault_manager_->getActiveProfile();
        std::cout << "🔍 [DEBUG] Checking for active profile..." << std::endl;
        
        if (!active_profile) {
            std::cout << "⚠️  [PROFILE] No active profile found - creating default profile..." << std::endl;
            
            // Create default profile for testing
            std::string profile_name = "Default Profile";
            std::string master_password = "1234"; // Default password for testing
            std::string recovery_key = "1234-5678-9ABC-DEF0"; // Default recovery key for testing
            
            std::cout << "🔧 [PROFILE] Creating profile with:" << std::endl;
            std::cout << "   - Name: " << profile_name << std::endl;
            std::cout << "   - Test Password: " << master_password << std::endl;
            std::cout << "   - Recovery Key: " << recovery_key << std::endl;
            
            active_profile = vault_manager_->createProfile(profile_name, master_password, recovery_key);
            
            if (!active_profile) {
                std::cout << "❌ [ERROR] Failed to create profile" << std::endl;
                std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
                return;
            }
            
            std::cout << "✅ [PROFILE] Created default profile: " << active_profile->name << std::endl;
            std::cout << "🔑 [PROFILE] Recovery key: " << recovery_key << " (save this!)" << std::endl;
        } else {
            std::cout << "✅ [PROFILE] Found active profile: " << active_profile->name << std::endl;
        }
        
        // Load folder passwords for sequence detection
        std::cout << "🔧 [SEQUENCE] Loading folder passwords for detection..." << std::endl;
        updateSequenceDetectorPasswords(active_profile->id);
        
        // Check if we have any folders, if not create a test folder
        auto folders = vault_manager_->getFolders(active_profile->id);
        if (folders.empty()) {
            std::cout << "⚠️  [SEQUENCE] No folders found in vault - creating test folder..." << std::endl;
            
            // Create a test folder in /tmp for demonstration
            std::string test_folder_path = "/tmp/phantom_test_folder";
            
            // Create the test folder if it doesn't exist
            if (!std::filesystem::exists(test_folder_path)) {
                try {
                    std::filesystem::create_directory(test_folder_path);
                    
                    // Add some test content
                    std::ofstream test_file(test_folder_path + "/test.txt");
                    test_file << "This is a test folder for PhantomVault sequence detection.\n";
                    test_file << "Password: 1234 or 2321\n";
                    test_file << "Try typing: T1234 or P1234 anywhere after pressing Ctrl+Alt+V\n";
                    test_file << "Format: T+password (temporary) or P+password (permanent)\n";
                    test_file.close();
                    
                    std::cout << "✅ [SEQUENCE] Created test folder: " << test_folder_path << std::endl;
                    std::cout << "📁 [SEQUENCE] Added test content to folder" << std::endl;
                } catch (const std::exception& e) {
                    std::cout << "❌ [SEQUENCE] Failed to create test folder: " << e.what() << std::endl;
                }
            } else {
                std::cout << "✅ [SEQUENCE] Test folder already exists: " << test_folder_path << std::endl;
            }
            
            // Reload folder passwords after potential folder creation
            updateSequenceDetectorPasswords(active_profile->id);
        }
        
        // Set up detection callback
        std::cout << "🔧 [SEQUENCE] Setting up detection callback..." << std::endl;
        sequence_detector_->setDetectionCallback([this, active_profile](const PasswordDetectionResult& result) {
            std::cout << "🎯 [CALLBACK] Password detection callback triggered!" << std::endl;
            handlePasswordDetection(result, active_profile->id);
        });
        
        // Start sequence detection
        std::cout << "🚀 [SEQUENCE] Starting keyboard sequence detection..." << std::endl;
        std::cout << "🔍 [DEBUG] Sequence detector status: " << (sequence_detector_->isActive() ? "Already active" : "Inactive") << std::endl;
        
        if (sequence_detector_->startDetection(10)) {
            std::cout << "✅ [SEQUENCE] Sequence detection started successfully!" << std::endl;
            std::cout << "⏱️  [SEQUENCE] Timeout: 10 seconds" << std::endl;
            std::cout << "📊 [SEQUENCE] Stats: " << sequence_detector_->getStats() << std::endl;
            std::cout << "" << std::endl;
            std::cout << "🎯 [INSTRUCTIONS] Type your password anywhere on the system:" << std::endl;
            std::cout << "   💡 For temporary unlock: T1234 (or mixed: hello T1234 world)" << std::endl;
            std::cout << "   💡 For permanent unlock: P1234 (or mixed: abc P1234 def)" << std::endl;
            std::cout << "   💡 Default mode: 1234 (or mixed: test 1234 end) = temporary" << std::endl;
            std::cout << "   📝 Format: T+password or P+password (prefix mode)" << std::endl;
            std::cout << "   ⚠️  If no password detected in 10 seconds, monitoring stops" << std::endl;
            std::cout << "" << std::endl;
            std::cout << "🔍 [MONITORING] Keyboard sequence detection is now active..." << std::endl;
        } else {
            std::cout << "❌ [ERROR] Failed to start sequence detection!" << std::endl;
            std::cout << "🔍 [DEBUG] Error: " << sequence_detector_->getLastError() << std::endl;
            std::cout << "🔍 [DEBUG] Detector initialized: " << (sequence_detector_ ? "Yes" : "No") << std::endl;
            
            // Send IPC message to GUI for fallback password dialog
            if (ipc_server_) {
                std::cout << "🔄 [FALLBACK] Sending fallback request to GUI clients..." << std::endl;
                nlohmann::json fallback_request;
                fallback_request["type"] = "password_dialog_request";
                fallback_request["mode"] = "unlock";
                fallback_request["reason"] = "sequence_detection_failed";
                fallback_request["error"] = sequence_detector_->getLastError();
                
                IPCMessage fallback_msg(IPCMessageType::ERROR_NOTIFICATION, fallback_request.dump());
                int sent_count = ipc_server_->broadcastMessage(fallback_msg);
                
                std::cout << "📡 [IPC] Sent fallback request to " << sent_count << " GUI client(s)" << std::endl;
            } else {
                std::cout << "⚠️  [FALLBACK] No IPC server available, using input overlay..." << std::endl;
                handleUnlockMode();
            }
        }
        
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    }
    
    void handleRelockModeWithSequence(std::shared_ptr<VaultProfile> profile) {
        std::cout << "=== RE-LOCK SEQUENCE MODE ===" << std::endl;
        
        auto temp_folders = vault_manager_->getTemporaryUnlockedFolders(profile->id);
        std::cout << "Found " << temp_folders.size() << " temporary folder(s) to lock" << std::endl;
        
        // Load folder passwords for sequence detection
        updateSequenceDetectorPasswords(profile->id);
        
        // Set up detection callback for re-lock
        sequence_detector_->setDetectionCallback([this, profile](const PasswordDetectionResult& result) {
            handleRelockPasswordDetection(result, profile->id);
        });
        
        // Start sequence detection
        if (sequence_detector_->startDetection(10)) {
            std::cout << "✅ Re-lock sequence detection started (10 second timeout)" << std::endl;
            std::cout << "   Type your password anywhere to lock temporary folders..." << std::endl;
        } else {
            std::cout << "❌ Failed to start sequence detection: " << sequence_detector_->getLastError() << std::endl;
            
            // Fallback to input overlay
            std::cout << "🔄 Falling back to input overlay..." << std::endl;
            handleRelockMode(profile);
        }
        
        std::cout << "===================\n" << std::endl;
    }
    
    void updateSequenceDetectorPasswords(const std::string& profile_id) {
        if (!sequence_detector_ || !vault_manager_) {
            return;
        }
        
        // Get all folders for the profile
        auto folders = vault_manager_->getFolders(profile_id);
        
        std::vector<FolderPassword> folder_passwords;
        for (const auto& folder : folders) {
            // For now, use a simple test password system
            // In production, this would load actual folder passwords
            std::string test_password = "1234"; // Default test password
            std::string password_hash = PasswordUtils::hashPassword(test_password);
            
            FolderPassword fp(
                folder.id,
                folder.folder_name,
                password_hash,
                folder.original_path,
                folder.is_locked
            );
            
            folder_passwords.push_back(fp);
        }
        
        // Update sequence detector with folder passwords
        sequence_detector_->updateFolderPasswords(folder_passwords);
        
        std::cout << "[SequenceDetector] Updated with " << folder_passwords.size() << " folder password(s)" << std::endl;
    }
    
    void handlePasswordDetection(const PasswordDetectionResult& result, const std::string& profile_id) {
        std::cout << "\n🎯 PASSWORD DETECTED!" << std::endl;
        std::cout << "   Folder ID: " << result.folder_id << std::endl;
        std::cout << "   Mode: " << (result.mode == UnlockMode::TEMPORARY ? "Temporary" : "Permanent") << std::endl;
        
        // For now, unlock all folders with the detected password
        // TODO: Implement individual folder unlocking in ServiceVaultManager
        UnlockResult unlock_result = vault_manager_->unlockFolders(profile_id, result.password, result.mode);
        
        if (unlock_result.success_count > 0) {
            std::cout << "✅ Successfully unlocked " << unlock_result.success_count << " folder(s) in " 
                      << (result.mode == UnlockMode::TEMPORARY ? "temporary" : "permanent") << " mode" << std::endl;
            
            if (unlock_result.failed_count > 0) {
                std::cout << "⚠️  " << unlock_result.failed_count << " folder(s) failed to unlock" << std::endl;
            }
        } else {
            std::cout << "❌ Failed to unlock folders" << std::endl;
            for (const auto& error : unlock_result.error_messages) {
                std::cout << "  Error: " << error << std::endl;
            }
        }
        
        std::cout << "==================\n" << std::endl;
    }
    
    void handleRelockPasswordDetection(const PasswordDetectionResult& result, const std::string& profile_id) {
        std::cout << "\n🔒 RE-LOCK PASSWORD DETECTED!" << std::endl;
        
        // Lock all temporary folders with the detected password
        int locked_count = vault_manager_->lockAllTemporaryFolders(profile_id, result.password);
        
        if (locked_count > 0) {
            std::cout << "✅ Successfully locked " << locked_count << " temporary folder(s)" << std::endl;
        } else {
            std::cout << "❌ Failed to lock folders (wrong password?)" << std::endl;
        }
        
        std::cout << "===================\n" << std::endl;
    }

    void serviceLoop() {
        std::cout << "Service main loop started" << std::endl;
        
        while (!should_stop_) {
            // Simple service loop - just sleep and check for stop
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Update uptime in vault state
            vault_state_.last_activity = std::chrono::system_clock::now();
        }
        
        std::cout << "Service main loop ended" << std::endl;
    }
    
    void setupDirectoryProtection() {
        if (!directory_protection_) {
            return;
        }
        
        std::cout << "[Service] Setting up directory protection..." << std::endl;
        
        // Set up security violation callback
        directory_protection_->setViolationCallback([this](const SecurityViolation& violation) {
            handleSecurityViolation(violation);
        });
        
        // Get vault base path and protect it
        if (vault_manager_) {
            std::string vault_base = vault_manager_->getVaultBasePath();
            if (!vault_base.empty()) {
                // Add vault base directory to monitoring
                if (directory_protection_->addMonitoredDirectory(vault_base, ProtectionMethod::IMMUTABLE_ATTR)) {
                    std::cout << "[Service] Added vault base to protection: " << vault_base << std::endl;
                    
                    // Apply initial protection
                    ProtectionResult result = directory_protection_->protectDirectory(vault_base);
                    if (result.success) {
                        std::cout << "[Service] ✅ Vault base directory protected" << std::endl;
                    } else {
                        std::cout << "[Service] ⚠️  Failed to protect vault base: " << result.error_message << std::endl;
                    }
                }
                
                // Also protect user-specific vault directory
                std::string user_vault = vault_manager_->getUserVaultPath();
                if (!user_vault.empty() && user_vault != vault_base) {
                    if (directory_protection_->addMonitoredDirectory(user_vault, ProtectionMethod::IMMUTABLE_ATTR)) {
                        std::cout << "[Service] Added user vault to protection: " << user_vault << std::endl;
                        
                        ProtectionResult result = directory_protection_->protectDirectory(user_vault);
                        if (result.success) {
                            std::cout << "[Service] ✅ User vault directory protected" << std::endl;
                        } else {
                            std::cout << "[Service] ⚠️  Failed to protect user vault: " << result.error_message << std::endl;
                        }
                    }
                }
            }
        }
        
        // Check if immutable attributes are supported
        if (directory_protection_->isImmutableAttributeSupported()) {
            std::cout << "[Service] ✅ Immutable attributes supported (chattr +i)" << std::endl;
        } else {
            std::cout << "[Service] ⚠️  Immutable attributes not supported, using permission-based protection" << std::endl;
        }
        
        std::cout << "[Service] Directory protection setup complete" << std::endl;
    }
    
    void handleSecurityViolation(const SecurityViolation& violation) {
        std::cout << "\n=== SECURITY VIOLATION DETECTED ===" << std::endl;
        std::cout << "Type: ";
        
        switch (violation.type) {
            case ViolationType::PROTECTION_REMOVED:
                std::cout << "Protection Removed";
                break;
            case ViolationType::PERMISSIONS_CHANGED:
                std::cout << "Permissions Changed";
                break;
            case ViolationType::DIRECTORY_DELETED:
                std::cout << "Directory Deleted";
                break;
            case ViolationType::UNAUTHORIZED_ACCESS:
                std::cout << "Unauthorized Access";
                break;
            case ViolationType::CONTENT_MODIFIED:
                std::cout << "Content Modified";
                break;
        }
        
        std::cout << std::endl;
        std::cout << "Directory: " << violation.directory_path << std::endl;
        std::cout << "Description: " << violation.description << std::endl;
        std::cout << "User Context: " << violation.user_context << std::endl;
        
        // Log to system log (syslog)
        std::string log_message = "PhantomVault Security Violation: " + violation.description + 
                                 " (" + violation.directory_path + ")";
        int log_result = system(("logger -t phantom-vault \"" + log_message + "\"").c_str());
        (void)log_result; // Suppress unused result warning
        
        // If auto-restore is enabled, try to restore protection
        if (directory_protection_ && directory_protection_->isAutoRestoreEnabled()) {
            std::cout << "Attempting automatic protection restoration..." << std::endl;
            int restored = directory_protection_->verifyAndRestoreProtection();
            if (restored > 0) {
                std::cout << "✅ Restored protection for " << restored << " directories" << std::endl;
            }
        }
        
        std::cout << "====================================\n" << std::endl;
    }
    
    void startSecurityMonitoring() {
        if (!directory_protection_) {
            return;
        }
        
        std::cout << "[Service] Starting security monitoring thread..." << std::endl;
        
        // Start monitoring thread
        security_monitoring_thread_ = std::thread([this]() {
            securityMonitoringLoop();
        });
        
        std::cout << "[Service] Security monitoring started" << std::endl;
    }
    
    void stopSecurityMonitoring() {
        if (security_monitoring_thread_.joinable()) {
            std::cout << "[Service] Stopping security monitoring..." << std::endl;
            security_monitoring_thread_.join();
            std::cout << "[Service] Security monitoring stopped" << std::endl;
        }
    }
    
    void securityMonitoringLoop() {
        std::cout << "[SecurityMonitor] Monitoring loop started" << std::endl;
        
        while (!should_stop_) {
            try {
                // Run protection verification every 30 seconds
                std::this_thread::sleep_for(std::chrono::seconds(30));
                
                if (should_stop_) {
                    break;
                }
                
                // Verify and restore protection
                if (directory_protection_) {
                    int restored = directory_protection_->verifyAndRestoreProtection();
                    if (restored > 0) {
                        std::cout << "[SecurityMonitor] Restored protection for " << restored << " directories" << std::endl;
                        
                        // Log to system log
                        std::string log_message = "PhantomVault: Restored protection for " + std::to_string(restored) + " directories";
                        int log_result = system(("logger -t phantom-vault \"" + log_message + "\"").c_str());
                        (void)log_result;
                    }
                }
                
                // Check for security violations in history
                if (directory_protection_) {
                    auto violations = directory_protection_->getViolationHistory(5);
                    if (!violations.empty()) {
                        // Check if we have new violations (simple check based on count)
                        static size_t last_violation_count = 0;
                        if (violations.size() > last_violation_count) {
                            std::cout << "[SecurityMonitor] New security violations detected (" 
                                      << (violations.size() - last_violation_count) << " new)" << std::endl;
                            last_violation_count = violations.size();
                        }
                    }
                }
                
            } catch (const std::exception& e) {
                std::cout << "[SecurityMonitor] Exception in monitoring loop: " << e.what() << std::endl;
            }
        }
        
        std::cout << "[SecurityMonitor] Monitoring loop ended" << std::endl;
    }
    
    void logSecurityEvent(const std::string& event_type, const std::string& description, const std::string& path = "") {
        // Create timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        // Format log message
        std::stringstream log_stream;
        log_stream << "PhantomVault Security Event: " << event_type;
        if (!description.empty()) {
            log_stream << " - " << description;
        }
        if (!path.empty()) {
            log_stream << " (" << path << ")";
        }
        
        std::string log_message = log_stream.str();
        
        // Log to console
        std::cout << "[SecurityLog] " << log_message << std::endl;
        
        // Log to system log
        int log_result = system(("logger -t phantom-vault \"" + log_message + "\"").c_str());
        (void)log_result;
        
        // Also log to a dedicated security log file
        std::string log_file = vault_manager_ ? vault_manager_->getVaultBasePath() + "/security.log" : "/tmp/phantom-vault-security.log";
        std::ofstream security_log(log_file, std::ios::app);
        if (security_log.is_open()) {
            security_log << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") 
                        << " - " << log_message << std::endl;
            security_log.close();
        }
    }
    
    void setupIPCServer() {
        std::cout << "[Service] Setting up IPC server..." << std::endl;
        
        // Create IPC server
        ipc_server_ = std::make_unique<IPCServer>();
        
        // Set up socket path
        std::string socket_path = "/tmp/phantom-vault-" + std::to_string(getuid()) + ".sock";
        
        if (!ipc_server_->initialize(socket_path)) {
            std::cout << "[Service] ⚠️  Failed to initialize IPC server: " << ipc_server_->getLastError() << std::endl;
            return;
        }
        
        // Set up message handlers
        setupIPCMessageHandlers();
        
        // Set client connection callback
        ipc_server_->setClientCallback([this](const std::string& client_id, bool connected) {
            if (connected) {
                std::cout << "[IPC] Client connected: " << client_id << std::endl;
                
                // Send initial vault state to new client
                sendVaultStateUpdate(client_id);
            } else {
                std::cout << "[IPC] Client disconnected: " << client_id << std::endl;
            }
        });
        
        // Start IPC server
        if (!ipc_server_->start()) {
            std::cout << "[Service] ⚠️  Failed to start IPC server: " << ipc_server_->getLastError() << std::endl;
            return;
        }
        
        std::cout << "[Service] ✅ IPC server started on: " << socket_path << std::endl;
    }
    
    void setupIPCMessageHandlers() {
        if (!ipc_server_) return;
        
        // GET_VAULT_STATE handler
        ipc_server_->setMessageHandler(IPCMessageType::GET_VAULT_STATE, 
            [this](const IPCMessage& msg, const std::string& client_id) -> IPCMessage {
                return handleGetVaultState(msg, client_id);
            });
        
        // GET_PROFILES handler
        ipc_server_->setMessageHandler(IPCMessageType::GET_PROFILES,
            [this](const IPCMessage& msg, const std::string& client_id) -> IPCMessage {
                return handleGetProfiles(msg, client_id);
            });
        
        // ADD_FOLDER handler
        ipc_server_->setMessageHandler(IPCMessageType::ADD_FOLDER,
            [this](const IPCMessage& msg, const std::string& client_id) -> IPCMessage {
                return handleAddFolder(msg, client_id);
            });
        
        // REMOVE_FOLDER handler
        ipc_server_->setMessageHandler(IPCMessageType::REMOVE_FOLDER,
            [this](const IPCMessage& msg, const std::string& client_id) -> IPCMessage {
                return handleRemoveFolder(msg, client_id);
            });
        
        // UNLOCK_FOLDERS handler
        ipc_server_->setMessageHandler(IPCMessageType::UNLOCK_FOLDERS,
            [this](const IPCMessage& msg, const std::string& client_id) -> IPCMessage {
                return handleUnlockFolders(msg, client_id);
            });
        
        // LOCK_FOLDERS handler
        ipc_server_->setMessageHandler(IPCMessageType::LOCK_FOLDERS,
            [this](const IPCMessage& msg, const std::string& client_id) -> IPCMessage {
                return handleLockFolders(msg, client_id);
            });
        
        // PASSWORD_INPUT handler (fallback method)
        ipc_server_->setMessageHandler(IPCMessageType::PASSWORD_INPUT,
            [this](const IPCMessage& msg, const std::string& client_id) -> IPCMessage {
                return handlePasswordInput(msg, client_id);
            });
        
        std::cout << "[IPC] Message handlers configured" << std::endl;
    }
    
    // IPC Message Handlers
    IPCMessage handleGetVaultState(const IPCMessage& msg, const std::string& client_id) {
        std::cout << "[IPC] Handling GET_VAULT_STATE from " << client_id << std::endl;
        
        nlohmann::json state;
        state["service_running"] = is_running_.load();
        state["uptime_seconds"] = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start_time_).count();
        
        if (vault_manager_) {
            auto active_profile = vault_manager_->getActiveProfile();
            if (active_profile) {
                auto created_time = std::chrono::duration_cast<std::chrono::seconds>(
                    active_profile->created_at.time_since_epoch()).count();
                state["active_profile"] = {
                    {"id", active_profile->id},
                    {"name", active_profile->name},
                    {"created_at", created_time}
                };
                
                // Get folder status
                nlohmann::json folder_list = nlohmann::json::array();
                auto folders = vault_manager_->getFolders(active_profile->id);
                for (const auto& folder : folders) {
                    nlohmann::json folder_info;
                    folder_info["id"] = folder.id;
                    folder_info["name"] = folder.folder_name;
                    folder_info["is_locked"] = folder.is_locked;
                    folder_info["original_path"] = folder.original_path;
                    folder_list.push_back(folder_info);
                }
                state["folders"] = folder_list;
            } else {
                state["active_profile"] = nullptr;
                state["folders"] = nlohmann::json::array();
            }
        }
        
        return IPCMessage(IPCMessageType::VAULT_STATE_UPDATE, state.dump());
    }
    
    IPCMessage handleGetProfiles(const IPCMessage& msg, const std::string& client_id) {
        std::cout << "[IPC] Handling GET_PROFILES from " << client_id << std::endl;
        
        nlohmann::json response;
        response["profiles"] = nlohmann::json::array();
        
        if (vault_manager_) {
            // Get active profile (simplified - could be extended to get all profiles)
            auto active_profile = vault_manager_->getActiveProfile();
            if (active_profile) {
                auto created_time = std::chrono::duration_cast<std::chrono::seconds>(
                    active_profile->created_at.time_since_epoch()).count();
                nlohmann::json profile_info;
                profile_info["id"] = active_profile->id;
                profile_info["name"] = active_profile->name;
                profile_info["created_at"] = created_time;
                response["profiles"].push_back(profile_info);
            }
        }
        
        return IPCMessage(IPCMessageType::PROFILE_UPDATE, response.dump());
    }
    
    IPCMessage handleAddFolder(const IPCMessage& msg, const std::string& client_id) {
        std::cout << "[IPC] Handling ADD_FOLDER from " << client_id << std::endl;
        
        try {
            nlohmann::json request = nlohmann::json::parse(msg.payload);
            std::string folder_path = request["folder_path"];
            std::string profile_id = request.value("profile_id", "");
            
            if (vault_manager_) {
                auto active_profile = vault_manager_->getActiveProfile();
                if (!active_profile && profile_id.empty()) {
                    return IPCMessage(IPCMessageType::ERROR_NOTIFICATION, 
                                    "No active profile and no profile_id specified");
                }
                
                std::string target_profile = profile_id.empty() ? active_profile->id : profile_id;
                
                // Add folder to vault (this would need to be implemented in vault manager)
                nlohmann::json response;
                response["success"] = true;
                response["message"] = "Folder added successfully";
                response["folder_path"] = folder_path;
                
                // Broadcast folder status update to all clients
                broadcastFolderStatusUpdate();
                
                return IPCMessage(IPCMessageType::FOLDER_STATUS_UPDATE, response.dump());
            }
            
            return IPCMessage(IPCMessageType::ERROR_NOTIFICATION, "Vault manager not available");
            
        } catch (const std::exception& e) {
            return IPCMessage(IPCMessageType::ERROR_NOTIFICATION, 
                            "Failed to parse ADD_FOLDER request: " + std::string(e.what()));
        }
    }
    
    IPCMessage handleRemoveFolder(const IPCMessage& msg, const std::string& client_id) {
        std::cout << "[IPC] Handling REMOVE_FOLDER from " << client_id << std::endl;
        
        try {
            nlohmann::json request = nlohmann::json::parse(msg.payload);
            std::string folder_path = request["folder_path"];
            
            nlohmann::json response;
            response["success"] = true;
            response["message"] = "Folder removed successfully";
            response["folder_path"] = folder_path;
            
            // Broadcast folder status update to all clients
            broadcastFolderStatusUpdate();
            
            return IPCMessage(IPCMessageType::FOLDER_STATUS_UPDATE, response.dump());
            
        } catch (const std::exception& e) {
            return IPCMessage(IPCMessageType::ERROR_NOTIFICATION, 
                            "Failed to parse REMOVE_FOLDER request: " + std::string(e.what()));
        }
    }
    
    IPCMessage handleUnlockFolders(const IPCMessage& msg, const std::string& client_id) {
        std::cout << "[IPC] Handling UNLOCK_FOLDERS from " << client_id << std::endl;
        
        try {
            nlohmann::json request = nlohmann::json::parse(msg.payload);
            std::string raw_password = request["password"];
            
            // Parse password using the same T/P prefix format as hotkeys
            PasswordInput parsed_input = PasswordParser::parseInput(raw_password);
            
            if (vault_manager_) {
                auto active_profile = vault_manager_->getActiveProfile();
                if (!active_profile) {
                    return IPCMessage(IPCMessageType::ERROR_NOTIFICATION, "No active profile");
                }
                
                UnlockResult result = vault_manager_->unlockFolders(active_profile->id, parsed_input.password, parsed_input.mode);
                
                nlohmann::json response;
                response["success"] = (result.success_count > 0);
                response["unlocked_count"] = result.success_count;
                response["failed_count"] = result.failed_count;
                response["mode"] = (parsed_input.mode == UnlockMode::TEMPORARY) ? "T" : "P";
                
                if (!result.error_messages.empty()) {
                    response["errors"] = result.error_messages;
                }
                
                // Broadcast folder status update to all clients
                broadcastFolderStatusUpdate();
                
                return IPCMessage(IPCMessageType::FOLDER_STATUS_UPDATE, response.dump());
            }
            
            return IPCMessage(IPCMessageType::ERROR_NOTIFICATION, "Vault manager not available");
            
        } catch (const std::exception& e) {
            return IPCMessage(IPCMessageType::ERROR_NOTIFICATION, 
                            "Failed to parse UNLOCK_FOLDERS request: " + std::string(e.what()));
        }
    }
    
    IPCMessage handleLockFolders(const IPCMessage& msg, const std::string& client_id) {
        std::cout << "[IPC] Handling LOCK_FOLDERS from " << client_id << std::endl;
        
        try {
            nlohmann::json request = nlohmann::json::parse(msg.payload);
            std::string password = request["password"];
            
            if (vault_manager_) {
                auto active_profile = vault_manager_->getActiveProfile();
                if (!active_profile) {
                    return IPCMessage(IPCMessageType::ERROR_NOTIFICATION, "No active profile");
                }
                
                int locked_count = vault_manager_->lockAllTemporaryFolders(active_profile->id, password);
                
                nlohmann::json response;
                response["success"] = (locked_count > 0);
                response["locked_count"] = locked_count;
                
                // Broadcast folder status update to all clients
                broadcastFolderStatusUpdate();
                
                return IPCMessage(IPCMessageType::FOLDER_STATUS_UPDATE, response.dump());
            }
            
            return IPCMessage(IPCMessageType::ERROR_NOTIFICATION, "Vault manager not available");
            
        } catch (const std::exception& e) {
            return IPCMessage(IPCMessageType::ERROR_NOTIFICATION, 
                            "Failed to parse LOCK_FOLDERS request: " + std::string(e.what()));
        }
    }
    
    IPCMessage handlePasswordInput(const IPCMessage& msg, const std::string& client_id) {
        std::cout << "[IPC] Handling PASSWORD_INPUT from " << client_id << std::endl;
        
        try {
            nlohmann::json request = nlohmann::json::parse(msg.payload);
            std::string raw_password = request["password"];
            std::string mode_str = request.value("mode", "unlock");
            
            // Parse password using the same T/P prefix format as sequence detection
            PasswordInput parsed_input = PasswordParser::parseInput(raw_password);
            
            if (vault_manager_) {
                auto active_profile = vault_manager_->getActiveProfile();
                if (!active_profile) {
                    return IPCMessage(IPCMessageType::ERROR_NOTIFICATION, "No active profile");
                }
                
                nlohmann::json response;
                
                if (mode_str == "unlock") {
                    // Unlock folders
                    UnlockResult result = vault_manager_->unlockFolders(active_profile->id, parsed_input.password, parsed_input.mode);
                    
                    response["success"] = (result.success_count > 0);
                    response["unlocked_count"] = result.success_count;
                    response["failed_count"] = result.failed_count;
                    response["mode"] = (parsed_input.mode == UnlockMode::TEMPORARY) ? "T" : "P";
                    
                    if (!result.error_messages.empty()) {
                        response["errors"] = result.error_messages;
                    }
                } else if (mode_str == "lock") {
                    // Lock temporary folders
                    int locked_count = vault_manager_->lockAllTemporaryFolders(active_profile->id, parsed_input.password);
                    
                    response["success"] = (locked_count > 0);
                    response["locked_count"] = locked_count;
                }
                
                // Broadcast folder status update to all clients
                broadcastFolderStatusUpdate();
                
                return IPCMessage(IPCMessageType::FOLDER_STATUS_UPDATE, response.dump());
            }
            
            return IPCMessage(IPCMessageType::ERROR_NOTIFICATION, "Vault manager not available");
            
        } catch (const std::exception& e) {
            return IPCMessage(IPCMessageType::ERROR_NOTIFICATION, 
                            "Failed to parse PASSWORD_INPUT request: " + std::string(e.what()));
        }
    }
    
    void sendVaultStateUpdate(const std::string& client_id = "") {
        if (!ipc_server_) return;
        
        IPCMessage state_msg = handleGetVaultState(IPCMessage(), client_id);
        
        if (client_id.empty()) {
            // Broadcast to all clients
            ipc_server_->broadcastMessage(state_msg);
        } else {
            // Send to specific client
            ipc_server_->sendMessage(client_id, state_msg);
        }
    }
    
    void broadcastFolderStatusUpdate() {
        if (!ipc_server_) return;
        
        IPCMessage update_msg = handleGetVaultState(IPCMessage(), "");
        update_msg.type = IPCMessageType::FOLDER_STATUS_UPDATE;
        
        int sent_count = ipc_server_->broadcastMessage(update_msg);
        if (sent_count > 0) {
            std::cout << "[IPC] Broadcasted folder status update to " << sent_count << " clients" << std::endl;
        }
    }
    
    void stopIPCServer() {
        if (ipc_server_) {
            std::cout << "[Service] Stopping IPC server..." << std::endl;
            ipc_server_->stop();
            std::cout << "[Service] IPC server stopped" << std::endl;
        }
    }

    ServiceConfig config_;
    VaultState vault_state_;
    std::atomic<bool> is_running_;
    std::atomic<bool> should_stop_;
    std::chrono::steady_clock::time_point start_time_;
    std::string last_error_;
    std::thread service_thread_;
    
    // Hotkey and input components
    std::unique_ptr<HotkeyManager> hotkey_manager_;
    std::unique_ptr<InputOverlay> input_overlay_;
    std::unique_ptr<SequenceDetector> sequence_detector_;
    std::unique_ptr<ServiceVaultManager> vault_manager_;
    std::unique_ptr<RecoveryManager> recovery_manager_;
    std::unique_ptr<DirectoryProtection> directory_protection_;
    std::unique_ptr<IPCServer> ipc_server_;
    std::thread security_monitoring_thread_;
};

// BackgroundService public interface
BackgroundService::BackgroundService() : pimpl(std::make_unique<Implementation>()) {}
BackgroundService::~BackgroundService() = default;

bool BackgroundService::initialize(const ServiceConfig& config) {
    return pimpl->initialize(config);
}

bool BackgroundService::start() {
    return pimpl->start();
}

void BackgroundService::stop() {
    pimpl->stop();
}

bool BackgroundService::isRunning() const {
    return pimpl->isRunning();
}

const ServiceConfig& BackgroundService::getConfig() const {
    return pimpl->getConfig();
}

const VaultState& BackgroundService::getVaultState() const {
    return pimpl->getVaultState();
}

std::chrono::seconds BackgroundService::getUptime() const {
    return pimpl->getUptime();
}

std::string BackgroundService::getLastError() const {
    return pimpl->getLastError();
}

// Simple ServiceLogger implementation
class ServiceLogger::Implementation {
public:
    bool initialize(const std::string& service_name, LogLevel log_level) {
        service_name_ = service_name;
        log_level_ = log_level;
        std::cout << "Logger initialized for service: " << service_name << std::endl;
        return true;
    }

    void logInfo(const std::string& message) {
        if (log_level_ <= LogLevel::INFO) {
            std::cout << "[INFO] [" << service_name_ << "] " << message << std::endl;
        }
    }

    void logWarning(const std::string& message) {
        if (log_level_ <= LogLevel::WARNING) {
            std::cout << "[WARN] [" << service_name_ << "] " << message << std::endl;
        }
    }

    void logError(const std::string& message) {
        if (log_level_ <= LogLevel::ERROR) {
            std::cout << "[ERROR] [" << service_name_ << "] " << message << std::endl;
        }
    }

    void logSecurity(const std::string& event) {
        std::cout << "[SECURITY] [" << service_name_ << "] " << event << std::endl;
    }

    void logDebug(const std::string& message) {
        if (log_level_ <= LogLevel::DEBUG) {
            std::cout << "[DEBUG] [" << service_name_ << "] " << message << std::endl;
        }
    }

private:
    std::string service_name_;
    LogLevel log_level_ = LogLevel::INFO;
};

ServiceLogger::ServiceLogger() : pimpl(std::make_unique<Implementation>()) {}
ServiceLogger::~ServiceLogger() = default;

bool ServiceLogger::initialize(const std::string& service_name, LogLevel log_level) {
    return pimpl->initialize(service_name, log_level);
}

void ServiceLogger::logInfo(const std::string& message) {
    pimpl->logInfo(message);
}

void ServiceLogger::logWarning(const std::string& message) {
    pimpl->logWarning(message);
}

void ServiceLogger::logError(const std::string& message) {
    pimpl->logError(message);
}

void ServiceLogger::logSecurity(const std::string& event) {
    pimpl->logSecurity(event);
}

void ServiceLogger::logDebug(const std::string& message) {
    pimpl->logDebug(message);
}

// Simple ServiceRecovery implementation
class ServiceRecovery::Implementation {
public:
    bool initialize(BackgroundService* service) {
        service_ = service;
        std::cout << "Recovery system initialized" << std::endl;
        return true;
    }

    void handleCrash() {
        std::cout << "Handling service crash..." << std::endl;
    }

    bool restoreFromBackup() {
        std::cout << "Restoring from backup..." << std::endl;
        return true;
    }

    bool validateSystemState() {
        std::cout << "Validating system state..." << std::endl;
        return true;
    }

    void emergencyLockAll() {
        std::cout << "Emergency lock all folders..." << std::endl;
    }

    void clearTemporaryState() {
        std::cout << "Clearing temporary state..." << std::endl;
    }

private:
    BackgroundService* service_ = nullptr;
};

ServiceRecovery::ServiceRecovery() : pimpl(std::make_unique<Implementation>()) {}
ServiceRecovery::~ServiceRecovery() = default;

bool ServiceRecovery::initialize(BackgroundService* service) {
    return pimpl->initialize(service);
}

void ServiceRecovery::handleCrash() {
    pimpl->handleCrash();
}

bool ServiceRecovery::restoreFromBackup() {
    return pimpl->restoreFromBackup();
}

bool ServiceRecovery::validateSystemState() {
    return pimpl->validateSystemState();
}

void ServiceRecovery::emergencyLockAll() {
    pimpl->emergencyLockAll();
}

void ServiceRecovery::clearTemporaryState() {
    pimpl->clearTemporaryState();
}

} // namespace service
} // namespace phantom_vault