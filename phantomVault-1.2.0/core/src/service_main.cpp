#include "phantom_vault/service.hpp"
#include <iostream>
#include <csignal>
#include <memory>
#include <thread>
#include <chrono>

using namespace phantom_vault::service;

// Global service instance for signal handling
std::unique_ptr<BackgroundService> g_service;

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down gracefully..." << std::endl;
    
    if (g_service) {
        g_service->stop();
    }
    
    exit(0);
}

void printUsage(const char* program_name) {
    std::cout << "PhantomVault Background Service\n";
    std::cout << "Usage: " << program_name << " [options]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --help, -h          Show this help message\n";
    std::cout << "  --version, -v       Show version information\n";
    std::cout << "  --config FILE       Use custom configuration file\n";
    std::cout << "  --log-level LEVEL   Set log level (DEBUG, INFO, WARNING, ERROR)\n";
    std::cout << "  --daemon, -d        Run as daemon (background)\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << "                    # Run in foreground\n";
    std::cout << "  " << program_name << " --daemon           # Run as daemon\n";
    std::cout << "  " << program_name << " --log-level DEBUG  # Enable debug logging\n";
}

void printVersion() {
    std::cout << "PhantomVault Background Service v1.0.0\n";
    std::cout << "Built with native C++ core for maximum performance\n";
    std::cout << "Copyright (c) 2025 PhantomVault Team\n";
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    ServiceConfig config;
    bool run_as_daemon = false;
    bool show_help = false;
    bool show_version = false;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            show_help = true;
        } else if (arg == "--version" || arg == "-v") {
            show_version = true;
        } else if (arg == "--daemon" || arg == "-d") {
            run_as_daemon = true;
        } else if (arg == "--log-level" && i + 1 < argc) {
            config.log_level = argv[++i];
        } else if (arg == "--config" && i + 1 < argc) {
            // Configuration file support can be added later
            std::cout << "Configuration file support not yet implemented\n";
            return 1;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    if (show_help) {
        printUsage(argv[0]);
        return 0;
    }
    
    if (show_version) {
        printVersion();
        return 0;
    }
    
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGQUIT, signalHandler);
    
    try {
        std::cout << "Starting PhantomVault Background Service..." << std::endl;
        
        // Create and initialize service
        g_service = std::make_unique<BackgroundService>();
        
        if (!g_service->initialize(config)) {
            std::cerr << "Failed to initialize service: " << g_service->getLastError() << std::endl;
            return 1;
        }
        
        std::cout << "Service initialized successfully" << std::endl;
        
        // Start service
        if (!g_service->start()) {
            std::cerr << "Failed to start service: " << g_service->getLastError() << std::endl;
            return 1;
        }
        
        std::cout << "Service started successfully" << std::endl;
        std::cout << "Press Ctrl+C to stop the service" << std::endl;
        
        if (run_as_daemon) {
            std::cout << "Running as daemon..." << std::endl;
            // Daemon mode - detach from terminal
            if (daemon(0, 0) != 0) {
                std::cerr << "Failed to daemonize" << std::endl;
                return 1;
            }
        } else {
            std::cout << "Running in foreground mode" << std::endl;
            std::cout << "Global hotkeys active:" << std::endl;
            std::cout << "  Ctrl+Alt+V - Unlock/Lock folders" << std::endl;
            std::cout << "  Ctrl+Alt+R - Recovery key input" << std::endl;
        }
        
        // Keep service running
        while (g_service->isRunning()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        std::cout << "Service stopped" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Service error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}