/**
 * PhantomVault Unified Application
 * 
 * Main application class that provides unified entry point for GUI, CLI, and service modes.
 * Preserves existing ServiceManager architecture while adding command-line interface.
 */

#pragma once

#include <string>
#include <memory>
#include <vector>

// Forward declarations
namespace phantomvault {
    class ServiceManager;
    class PrivilegeManager;
}

enum class ApplicationMode {
    GUI,        // Desktop GUI application (default)
    CLI,        // Command-line interface
    SERVICE,    // Background service mode
    HELP,       // Show help and exit
    VERSION     // Show version and exit
};

struct ApplicationConfig {
    ApplicationMode mode = ApplicationMode::GUI;
    std::string config_file;
    std::string log_level = "INFO";
    int ipc_port = 9876;
    bool daemon_mode = false;
    std::vector<std::string> cli_args;
};

class PhantomVaultApplication {
public:
    PhantomVaultApplication();
    ~PhantomVaultApplication();

    // Main application entry point
    int run(int argc, char* argv[]);

private:
    // Command line parsing
    ApplicationConfig parseCommandLine(int argc, char* argv[]);
    void printUsage(const char* program_name);
    void printVersion();

    // Application modes
    int runGUIMode();
    int runCLIMode();
    int runServiceMode();

    // CLI command implementations
    int checkServiceStatus();
    int startService();
    int stopService();
    int restartService();
    int listProfiles();
    int lockProfile(const std::string& profileId);
    int unlockProfile(const std::string& profileId);

    // GUI management
    bool launchElectronGUI();

    // Privilege management
    bool ensurePrivileges();

    // Core components (preserving existing architecture)
    std::unique_ptr<phantomvault::ServiceManager> service_manager_;
    std::unique_ptr<phantomvault::PrivilegeManager> privilege_manager_;
    
    ApplicationConfig config_;
    std::string last_error_;
};