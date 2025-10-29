/**
 * PhantomVault Unified Application Implementation
 * 
 * Integrates existing ServiceManager and core components into unified application.
 * Preserves all existing functionality while adding proper command-line interface.
 */

#include "phantomvault_application.hpp"
#include "../core/include/service_manager.hpp"
#include "../core/include/privilege_manager.hpp"
#include <iostream>
#include <cstring>
#include <signal.h>
#include <thread>
#include <chrono>

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
    
    // Check if we already have required privileges
    if (privilege_manager_->hasRequiredPermissions()) {
        return true;
    }
    
    // For GUI mode, request elevation if needed
    if (config_.mode == ApplicationMode::GUI) {
        std::cout << "PhantomVault requires administrator privileges for folder protection.\n";
        std::cout << "Requesting elevated privileges...\n";
        
        if (!privilege_manager_->requestElevatedPrivileges()) {
            last_error_ = "Failed to obtain required privileges. Please run as administrator.";
            return false;
        }
    }
    else {
        // For CLI and service modes, require existing privileges
        last_error_ = "Insufficient privileges. Please run with sudo/administrator rights.";
        return false;
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
    std::cout << "[INFO] 💻 GUI will launch shortly..." << std::endl;
    
    // TODO: Launch Electron GUI process here
    // For now, run service loop (will be replaced with GUI integration)
    
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
        // TODO: Implement service status check
        std::cout << "Service Status: Checking..." << std::endl;
        return 0;
    }
    else if (command == "start") {
        // TODO: Implement service start
        std::cout << "Starting PhantomVault service..." << std::endl;
        return 0;
    }
    else if (command == "stop") {
        // TODO: Implement service stop
        std::cout << "Stopping PhantomVault service..." << std::endl;
        return 0;
    }
    else if (command == "restart") {
        // TODO: Implement service restart
        std::cout << "Restarting PhantomVault service..." << std::endl;
        return 0;
    }
    else if (command == "profiles") {
        // TODO: Implement profile listing
        std::cout << "Available profiles:" << std::endl;
        return 0;
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