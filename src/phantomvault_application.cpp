/**
 * PhantomVault Unified Application Implementation
 * 
 * Integrates existing ServiceManager and core components into unified application.
 * Preserves all existing functionality while adding proper command-line interface.
 */

#include "phantomvault_application.hpp"
#include "../core/include/service_manager.hpp"
#include "../core/include/privilege_manager.hpp"
#include "../core/include/profile_manager.hpp"
#include "../core/include/folder_security_manager.hpp"
#include <iostream>
#include <cstring>
#include <signal.h>
#include <thread>
#include <chrono>
#include <filesystem>
#include <cstdlib>

// Global application instance for signal handling
static PhantomVaultApplication* g_app_instance = nullptr;
static std::atomic<bool> g_running{true};

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    g_running = false;
}

PhantomVaultApplication::PhantomVaultApplication() {
    g_app_instance = this;
}

PhantomVaultApplication::~PhantomVaultApplication() {
    g_app_instance = nullptr;
}

int PhantomVaultApplication::run(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        config_ = parseCommandLine(argc, argv);
        
        // Handle help and version modes
        if (config_.mode == ApplicationMode::HELP) {
            printUsage(argv[0]);
            return 0;
        }
        
        if (config_.mode == ApplicationMode::VERSION) {
            printVersion();
            return 0;
        }
        
        // Set up signal handlers for graceful shutdown
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        #ifndef _WIN32
        signal(SIGQUIT, signalHandler);
        #endif
        
        // Initialize privilege manager (preserving existing implementation)
        privilege_manager_ = std::make_unique<phantomvault::PrivilegeManager>();
        
        // Ensure we have required privileges
        if (!ensurePrivileges()) {
            std::cerr << "Error: " << last_error_ << std::endl;
            return 1;
        }
        
        // Run in appropriate mode
        switch (config_.mode) {
            case ApplicationMode::GUI:
                return runGUIMode();
            case ApplicationMode::CLI:
                return runCLIMode();
            case ApplicationMode::SERVICE:
                return runServiceMode();
            default:
                std::cerr << "Error: Invalid application mode" << std::endl;
                return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}

ApplicationConfig PhantomVaultApplication::parseCommandLine(int argc, char* argv[]) {
    ApplicationConfig config;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            config.mode = ApplicationMode::HELP;
            return config;
        }
        else if (arg == "--version" || arg == "-v") {
            config.mode = ApplicationMode::VERSION;
            return config;
        }
        else if (arg == "--gui") {
            config.mode = ApplicationMode::GUI;
        }
        else if (arg == "--cli") {
            config.mode = ApplicationMode::CLI;
        }
        else if (arg == "--service") {
            config.mode = ApplicationMode::SERVICE;
        }
        else if (arg == "--daemon" || arg == "-d") {
            config.daemon_mode = true;
        }
        else if (arg == "--config" && i + 1 < argc) {
            config.config_file = argv[++i];
        }
        else if (arg == "--log-level" && i + 1 < argc) {
            config.log_level = argv[++i];
        }
        else if (arg == "--port" && i + 1 < argc) {
            config.ipc_port = std::stoi(argv[++i]);
        }
        else {
            // Collect remaining arguments for CLI mode
            config.cli_args.push_back(arg);
        }
    }
    
    // Default to GUI mode if no mode specified
    if (config.mode != ApplicationMode::CLI && config.mode != ApplicationMode::SERVICE) {
        config.mode = ApplicationMode::GUI;
    }
    
    return config;
}

void PhantomVaultApplication::printUsage(const char* program_name) {
    std::cout << "PhantomVault - Invisible Folder Security with Profile-Based Management\n\n";
    std::cout << "Usage: " << program_name << " [OPTIONS] [COMMAND]\n\n";
    std::cout << "Modes:\n";
    std::cout << "  --gui                 Launch desktop GUI application (default)\n";
    std::cout << "  --cli                 Run in command-line interface mode\n";
    std::cout << "  --service             Run as background service\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help           Show this help message\n";
    std::cout << "  -v, --version        Show version information\n";
    std::cout << "  -d, --daemon         Run service in daemon mode\n";
    std::cout << "  --config FILE        Use custom configuration file\n";
    std::cout << "  --log-level LEVEL    Set log level (DEBUG, INFO, WARN, ERROR)\n";
    std::cout << "  --port PORT          Set IPC server port (default: 9876)\n\n";
    std::cout << "CLI Commands:\n";
    std::cout << "  status               Show service status\n";
    std::cout << "  start                Start the service\n";
    std::cout << "  stop                 Stop the service\n";
    std::cout << "  restart              Restart the service\n";
    std::cout << "  profiles             List available profiles\n";
    std::cout << "  lock [profile]       Lock folders for profile\n";
    std::cout << "  unlock [profile]     Unlock folders for profile\n\n";
    std::cout << "Examples:\n";
    std::cout << "  sudo " << program_name << "                    # Launch GUI with privileges\n";
    std::cout << "  sudo " << program_name << " --service          # Run as background service\n";
    std::cout << "  " << program_name << " --cli status           # Check service status\n";
    std::cout << "  " << program_name << " --cli profiles         # List profiles\n\n";
    std::cout << "Global Hotkey: Press Ctrl+Alt+V anywhere to unlock folders\n";
}

void PhantomVaultApplication::printVersion() {
    std::cout << "PhantomVault v1.0.0\n";
    std::cout << "Military-grade folder security with invisible access\n";
    std::cout << "Built with AES-256 encryption and cross-platform support\n";
}

bool PhantomVaultApplication::ensurePrivileges() {
    if (!privilege_manager_) {
        last_error_ = "Privilege manager not initialized";
        return false;
    }
    
    // Initialize privilege manager first
    if (!privilege_manager_->initialize()) {
        last_error_ = "Failed to initialize privilege manager";
        return false;
    }
    
    // Check if we already have required privileges
    if (privilege_manager_->hasAdminPrivileges()) {
        return true;
    }
    
    // For GUI mode, request elevation if needed
    if (config_.mode == ApplicationMode::GUI) {
        std::cout << "PhantomVault requires administrator privileges for folder protection.\n";
        std::cout << "Requesting elevated privileges...\n";
        
        auto elevation_result = privilege_manager_->requestElevation("PhantomVault requires admin privileges for folder protection");
        if (!elevation_result.success) {
            last_error_ = "Failed to obtain required privileges: " + elevation_result.errorDetails;
            return false;
        }
    }
    else {
        // For CLI and service modes, require existing privileges
        if (!privilege_manager_->validateStartupPrivileges()) {
            last_error_ = privilege_manager_->getStartupPrivilegeError();
            return false;
        }
    }
    
    return true;
}

int PhantomVaultApplication::runGUIMode() {
    std::cout << "=== PhantomVault Desktop Application ===" << std::endl;
    std::cout << "Starting GUI with system service integration..." << std::endl;
    
    // Initialize service manager (preserving existing architecture)
    service_manager_ = std::make_unique<phantomvault::ServiceManager>();
    
    if (!service_manager_->initialize(config_.config_file, config_.log_level, config_.ipc_port)) {
        std::cerr << "Failed to initialize service: " << service_manager_->getLastError() << std::endl;
        return 1;
    }
    
    if (!service_manager_->start()) {
        std::cerr << "Failed to start service: " << service_manager_->getLastError() << std::endl;
        return 1;
    }
    
    std::cout << "[INFO] 🚀 Service started successfully" << std::endl;
    std::cout << "[INFO] 🎯 Global hotkey active: Press Ctrl+Alt+V anywhere to unlock folders" << std::endl;
    std::cout << "[INFO] 📡 IPC server listening on port " << config_.ipc_port << std::endl;
    std::cout << "[INFO] 💻 Launching GUI application..." << std::endl;
    
    // Launch Electron GUI process
    if (!launchElectronGUI()) {
        std::cerr << "Failed to launch GUI application" << std::endl;
        service_manager_->stop();
        return 1;
    }
    
    std::cout << "[INFO] ✅ GUI application launched successfully" << std::endl;
    std::cout << "[INFO] Service running in background..." << std::endl;
    
    // Main service loop - keep service running while GUI is active
    while (g_running && service_manager_->isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "[INFO] Shutting down service..." << std::endl;
    service_manager_->stop();
    
    return 0;
}

int PhantomVaultApplication::runCLIMode() {
    std::cout << "=== PhantomVault CLI ===" << std::endl;
    
    if (config_.cli_args.empty()) {
        std::cerr << "Error: No CLI command specified. Use --help for usage." << std::endl;
        return 1;
    }
    
    std::string command = config_.cli_args[0];
    
    if (command == "status") {
        return checkServiceStatus();
    }
    else if (command == "start") {
        return startService();
    }
    else if (command == "stop") {
        return stopService();
    }
    else if (command == "restart") {
        return restartService();
    }
    else if (command == "profiles") {
        return listProfiles();
    }
    else if (command == "lock" && config_.cli_args.size() > 1) {
        return lockProfile(config_.cli_args[1]);
    }
    else if (command == "unlock" && config_.cli_args.size() > 1) {
        return unlockProfile(config_.cli_args[1]);
    }
    else {
        std::cerr << "Error: Unknown command '" << command << "'. Use --help for usage." << std::endl;
        return 1;
    }
}

int PhantomVaultApplication::runServiceMode() {
    std::cout << "=== PhantomVault Background Service ===" << std::endl;
    std::cout << "Starting system-wide folder protection service..." << std::endl;
    
    // Initialize service manager (preserving existing implementation)
    service_manager_ = std::make_unique<phantomvault::ServiceManager>();
    
    if (!service_manager_->initialize(config_.config_file, config_.log_level, config_.ipc_port)) {
        std::cerr << "Failed to initialize service: " << service_manager_->getLastError() << std::endl;
        return 1;
    }
    
    if (!service_manager_->start()) {
        std::cerr << "Failed to start service: " << service_manager_->getLastError() << std::endl;
        return 1;
    }
    
    std::cout << "[INFO] 🚀 PhantomVault service started successfully" << std::endl;
    std::cout << "[INFO] 🎯 Global keyboard monitoring active (Ctrl+Alt+V)" << std::endl;
    std::cout << "[INFO] 🔒 Folder protection system ready" << std::endl;
    std::cout << "[INFO] 📡 IPC server listening on port " << config_.ipc_port << std::endl;
    
    // Main service loop
    while (g_running && service_manager_->isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "[INFO] Shutting down service gracefully..." << std::endl;
    service_manager_->stop();
    std::cout << "[INFO] Service stopped" << std::endl;
    
    return 0;
}

// CLI Command Implementations

int PhantomVaultApplication::checkServiceStatus() {
    std::cout << "Checking PhantomVault service status..." << std::endl;
    
    // Try to connect to existing service via IPC
    // For now, we'll create a temporary service manager to check status
    auto temp_service = std::make_unique<phantomvault::ServiceManager>();
    
    if (temp_service->isRunning()) {
        std::cout << "✅ PhantomVault service is running" << std::endl;
        std::cout << "   Version: " << temp_service->getVersion() << std::endl;
        std::cout << "   Platform: " << temp_service->getPlatformInfo() << std::endl;
        std::cout << "   Memory Usage: " << temp_service->getMemoryUsage() << " bytes" << std::endl;
        return 0;
    } else {
        std::cout << "❌ PhantomVault service is not running" << std::endl;
        return 1;
    }
}

int PhantomVaultApplication::startService() {
    std::cout << "Starting PhantomVault service..." << std::endl;
    
    // Initialize service manager
    service_manager_ = std::make_unique<phantomvault::ServiceManager>();
    
    if (!service_manager_->initialize(config_.config_file, config_.log_level, config_.ipc_port)) {
        std::cerr << "❌ Failed to initialize service: " << service_manager_->getLastError() << std::endl;
        return 1;
    }
    
    if (!service_manager_->start()) {
        std::cerr << "❌ Failed to start service: " << service_manager_->getLastError() << std::endl;
        return 1;
    }
    
    std::cout << "✅ PhantomVault service started successfully" << std::endl;
    std::cout << "   IPC Port: " << config_.ipc_port << std::endl;
    std::cout << "   Log Level: " << config_.log_level << std::endl;
    
    // Keep service running in foreground for CLI start
    std::cout << "Press Ctrl+C to stop the service..." << std::endl;
    while (g_running && service_manager_->isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    service_manager_->stop();
    std::cout << "Service stopped." << std::endl;
    return 0;
}

int PhantomVaultApplication::stopService() {
    std::cout << "Stopping PhantomVault service..." << std::endl;
    
    // Try to connect to running service via HTTP IPC
    std::string stop_command = "curl -s -X POST http://localhost:" + std::to_string(config_.ipc_port) + "/api/service/stop";
    
    int result = system(stop_command.c_str());
    
    if (result == 0) {
        std::cout << "✅ PhantomVault service stopped successfully" << std::endl;
        return 0;
    } else {
        // If IPC fails, try to find and terminate the process
        std::cout << "IPC stop failed, attempting process termination..." << std::endl;
        
        // Try to find PhantomVault processes
        int kill_result = system("pkill -f phantomvault 2>/dev/null");
        
        if (kill_result == 0) {
            std::cout << "✅ PhantomVault service terminated" << std::endl;
            return 0;
        } else {
            std::cerr << "❌ No running PhantomVault service found" << std::endl;
            return 1;
        }
    }
}

int PhantomVaultApplication::restartService() {
    std::cout << "Restarting PhantomVault service..." << std::endl;
    
    // Stop first, then start
    int stop_result = stopService();
    if (stop_result != 0) {
        return stop_result;
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Brief pause
    
    return startService();
}

int PhantomVaultApplication::listProfiles() {
    std::cout << "Listing PhantomVault profiles..." << std::endl;
    
    // Initialize service manager to access ProfileManager
    service_manager_ = std::make_unique<phantomvault::ServiceManager>();
    
    if (!service_manager_->initialize()) {
        std::cerr << "❌ Failed to initialize service: " << service_manager_->getLastError() << std::endl;
        return 1;
    }
    
    auto* profile_manager = service_manager_->getProfileManager();
    if (!profile_manager) {
        std::cerr << "❌ Failed to access profile manager" << std::endl;
        return 1;
    }
    
    auto profiles = profile_manager->getAllProfiles();
    
    if (profiles.empty()) {
        std::cout << "No profiles found. Create a profile first using the GUI." << std::endl;
        return 0;
    }
    
    std::cout << "Available profiles:" << std::endl;
    for (const auto& profile : profiles) {
        std::cout << "  • " << profile.name << " (ID: " << profile.id << ")" << std::endl;
        std::cout << "    Created: " << std::chrono::duration_cast<std::chrono::seconds>(
            profile.createdAt.time_since_epoch()).count() << std::endl;
        std::cout << "    Folders: " << profile.folderCount << std::endl;
        std::cout << "    Status: " << (profile.isActive ? "Active" : "Inactive") << std::endl;
        std::cout << std::endl;
    }
    
    return 0;
}

int PhantomVaultApplication::lockProfile(const std::string& profileId) {
    std::cout << "Locking profile: " << profileId << std::endl;
    
    // Initialize service manager to access components
    service_manager_ = std::make_unique<phantomvault::ServiceManager>();
    
    if (!service_manager_->initialize()) {
        std::cerr << "❌ Failed to initialize service: " << service_manager_->getLastError() << std::endl;
        return 1;
    }
    
    auto* folder_manager = service_manager_->getFolderSecurityManager();
    if (!folder_manager) {
        std::cerr << "❌ Failed to access folder security manager" << std::endl;
        return 1;
    }
    
    // Lock temporary folders for the profile
    if (folder_manager->lockTemporaryFolders(profileId)) {
        std::cout << "✅ Profile folders locked successfully" << std::endl;
        return 0;
    } else {
        std::cerr << "❌ Failed to lock profile folders: " << folder_manager->getLastError() << std::endl;
        return 1;
    }
}

int PhantomVaultApplication::unlockProfile(const std::string& profileId) {
    std::cout << "Unlocking profile: " << profileId << std::endl;
    std::cout << "❌ Profile unlock requires master key authentication" << std::endl;
    std::cout << "Use the GUI application for secure profile unlock operations" << std::endl;
    
    // For security reasons, we don't allow CLI unlock without proper authentication
    // This would require implementing secure password input in CLI
    
    return 1;
}

bool PhantomVaultApplication::launchElectronGUI() {
    // Check if GUI directory exists
    if (!std::filesystem::exists("gui")) {
        last_error_ = "GUI directory not found. Please ensure PhantomVault is properly installed.";
        return false;
    }
    
    // Check if GUI is built
    if (!std::filesystem::exists("gui/dist")) {
        std::cout << "[INFO] GUI not built, building now..." << std::endl;
        
        // Try to build the GUI
        int build_result = system("cd gui && npm run build > /dev/null 2>&1");
        if (build_result != 0) {
            last_error_ = "Failed to build GUI. Run 'cd gui && npm install && npm run build' manually.";
            return false;
        }
    }
    
    // Launch Electron GUI in background
    std::cout << "[INFO] Starting Electron GUI process..." << std::endl;
    
    // Use system() to launch GUI in background
    // The GUI will connect to the service via IPC on the configured port
    int launch_result = system("cd gui && npm run dev > /dev/null 2>&1 &");
    
    if (launch_result != 0) {
        // Try alternative launch method
        launch_result = system("cd gui && electron . > /dev/null 2>&1 &");
        
        if (launch_result != 0) {
            last_error_ = "Failed to launch Electron GUI. Ensure Node.js and Electron are installed.";
            return false;
        }
    }
    
    // Give GUI time to start
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    return true;
}