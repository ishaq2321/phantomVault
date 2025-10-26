/**
 * PhantomVault Service - Main Entry Point
 * 
 * Lightweight C++ service for invisible folder security with profile-based management.
 * Designed for < 10MB RAM usage with minimal battery impact.
 */

#include <iostream>
#include <memory>
#include <csignal>
#include <thread>
#include <chrono>

#include "service_manager.hpp"

using namespace phantomvault;

// Global service instance for signal handling
std::unique_ptr<ServiceManager> g_service;

/**
 * Signal handler for graceful shutdown
 */
void signalHandler(int signal) {
    std::cout << "\n[PhantomVault] Received signal " << signal << ", shutting down gracefully..." << std::endl;
    
    if (g_service) {
        g_service->stop();
    }
    
    exit(0);
}

/**
 * Print usage information
 */
void printUsage(const char* program_name) {
    std::cout << "PhantomVault Service v1.0.0\n";
    std::cout << "Invisible Folder Security with Profile-Based Management\n\n";
    std::cout << "Usage: " << program_name << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help, -h          Show this help message\n";
    std::cout << "  --version, -v       Show version information\n";
    std::cout << "  --daemon, -d        Run as daemon (background)\n";
    std::cout << "  --config FILE       Use custom configuration file\n";
    std::cout << "  --log-level LEVEL   Set log level (DEBUG, INFO, WARNING, ERROR)\n";
    std::cout << "  --port PORT         Set IPC server port (default: 9876)\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << "                    # Run in foreground\n";
    std::cout << "  " << program_name << " --daemon           # Run as daemon\n";
    std::cout << "  " << program_name << " --log-level DEBUG  # Enable debug logging\n";
    std::cout << "\nFor more information, visit: https://github.com/ishaq2321/phantomVault\n";
}

/**
 * Print version information
 */
void printVersion() {
    std::cout << "PhantomVault Service v1.0.0\n";
    std::cout << "Built with C++17 for maximum performance and security\n";
    std::cout << "Platform: ";
    
    #ifdef PLATFORM_LINUX
    std::cout << "Linux\n";
    #elif PLATFORM_MACOS
    std::cout << "macOS\n";
    #elif PLATFORM_WINDOWS
    std::cout << "Windows\n";
    #else
    std::cout << "Unknown\n";
    #endif
    
    std::cout << "Copyright (c) 2025 PhantomVault Team\n";
    std::cout << "Licensed under MIT License\n";
}

/**
 * Parse command line arguments
 */
struct ServiceConfig {
    bool show_help = false;
    bool show_version = false;
    bool run_as_daemon = false;
    std::string config_file;
    std::string log_level = "INFO";
    int port = 9876;
};

ServiceConfig parseArguments(int argc, char* argv[]) {
    ServiceConfig config;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            config.show_help = true;
        } else if (arg == "--version" || arg == "-v") {
            config.show_version = true;
        } else if (arg == "--daemon" || arg == "-d") {
            config.run_as_daemon = true;
        } else if (arg == "--log-level" && i + 1 < argc) {
            config.log_level = argv[++i];
        } else if (arg == "--config" && i + 1 < argc) {
            config.config_file = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            config.port = std::stoi(argv[++i]);
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            config.show_help = true;
            break;
        }
    }
    
    return config;
}

/**
 * Main entry point
 */
int main(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        ServiceConfig config = parseArguments(argc, argv);
        
        if (config.show_help) {
            printUsage(argv[0]);
            return 0;
        }
        
        if (config.show_version) {
            printVersion();
            return 0;
        }
        
        // Set up signal handlers for graceful shutdown
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        #ifndef PLATFORM_WINDOWS
        signal(SIGQUIT, signalHandler);
        #endif
        
        std::cout << "[PhantomVault] Starting service..." << std::endl;
        std::cout << "[PhantomVault] Version: 1.0.0" << std::endl;
        std::cout << "[PhantomVault] Log level: " << config.log_level << std::endl;
        std::cout << "[PhantomVault] IPC port: " << config.port << std::endl;
        
        // Create and initialize service
        g_service = std::make_unique<ServiceManager>();
        
        if (!g_service->initialize(config.config_file, config.log_level, config.port)) {
            std::cerr << "[PhantomVault] Failed to initialize service: " << g_service->getLastError() << std::endl;
            return 1;
        }
        
        std::cout << "[PhantomVault] Service initialized successfully" << std::endl;
        
        // Start service
        if (!g_service->start()) {
            std::cerr << "[PhantomVault] Failed to start service: " << g_service->getLastError() << std::endl;
            return 1;
        }
        
        std::cout << "[PhantomVault] Service started successfully" << std::endl;
        
        if (config.run_as_daemon) {
            std::cout << "[PhantomVault] Running as daemon..." << std::endl;
            #ifndef PLATFORM_WINDOWS
            // Daemonize process on Unix-like systems
            if (daemon(0, 0) != 0) {
                std::cerr << "[PhantomVault] Failed to daemonize" << std::endl;
                return 1;
            }
            #endif
        } else {
            std::cout << "[PhantomVault] Running in foreground mode" << std::endl;
            std::cout << "[PhantomVault] Features active:" << std::endl;
            std::cout << "  • Profile-based folder security" << std::endl;
            std::cout << "  • Invisible keyboard sequence detection (Ctrl+Alt+V)" << std::endl;
            std::cout << "  • AES-256 encryption with secure backups" << std::endl;
            std::cout << "  • Cross-platform compatibility" << std::endl;
            std::cout << "  • < 10MB RAM usage optimization" << std::endl;
            std::cout << "[PhantomVault] Press Ctrl+C to stop the service" << std::endl;
        }
        
        // Keep service running
        while (g_service->isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        std::cout << "[PhantomVault] Service stopped" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[PhantomVault] Service error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "[PhantomVault] Unknown service error occurred" << std::endl;
        return 1;
    }
    
    return 0;
}