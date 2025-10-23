#include "phantom_vault/startup_manager.hpp"
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <sstream>

namespace phantom_vault {

namespace {
    std::string getAutostartDir() {
        const char* xdg_config_home = std::getenv("XDG_CONFIG_HOME");
        std::filesystem::path config_dir;
        
        if (xdg_config_home && *xdg_config_home) {
            config_dir = xdg_config_home;
        } else {
            const char* home = std::getenv("HOME");
            if (!home) {
                throw std::runtime_error("HOME environment variable not set");
            }
            config_dir = std::filesystem::path(home) / ".config";
        }
        
        return (config_dir / "autostart").string();
    }

    std::string createDesktopEntry(const std::string& app_name,
                                 const std::string& exec_path,
                                 const std::string& icon_path) {
        std::ostringstream entry;
        entry << "[Desktop Entry]\n"
              << "Type=Application\n"
              << "Version=1.0\n"
              << "Name=" << app_name << "\n"
              << "Comment=" << app_name << " startup script\n"
              << "Exec=" << exec_path << "\n"
              << "Icon=" << icon_path << "\n"
              << "Terminal=false\n"
              << "Categories=Utility;\n"
              << "X-GNOME-Autostart-enabled=true\n";
        return entry.str();
    }
}

class StartupManager::Implementation {
public:
    Implementation()
        : initialized_(false)
        , app_name_()
        , desktop_file_()
        , last_error_()
    {}

    bool initialize(const std::string& app_name,
                   const std::string& exec_path,
                   const std::string& icon_path) {
        try {
            // Validate paths
            if (!std::filesystem::exists(exec_path)) {
                last_error_ = "Executable path does not exist: " + exec_path;
                return false;
            }

            if (!std::filesystem::exists(icon_path)) {
                last_error_ = "Icon path does not exist: " + icon_path;
                return false;
            }

            app_name_ = app_name;
            desktop_file_ = std::filesystem::path(getAutostartDir()) / 
                          (app_name + ".desktop");

            // Create autostart directory if it doesn't exist
            std::filesystem::create_directories(getAutostartDir());

            // Store the desktop entry content
            desktop_entry_ = createDesktopEntry(app_name, exec_path, icon_path);
            
            initialized_ = true;
            return true;
        } catch (const std::exception& e) {
            last_error_ = std::string("Initialization failed: ") + e.what();
            return false;
        }
    }

    bool setAutostart(bool enable) {
        if (!initialized_) {
            last_error_ = "Not initialized";
            return false;
        }

        try {
            if (enable) {
                // Create or update the desktop entry file
                std::ofstream file(desktop_file_);
                if (!file) {
                    last_error_ = "Failed to create desktop entry file";
                    return false;
                }
                file << desktop_entry_;
                file.close();
            } else {
                // Remove the desktop entry file if it exists
                if (std::filesystem::exists(desktop_file_)) {
                    std::filesystem::remove(desktop_file_);
                }
            }
            return true;
        } catch (const std::exception& e) {
            last_error_ = std::string("Failed to ") + (enable ? "enable" : "disable") +
                         " autostart: " + e.what();
            return false;
        }
    }

    bool isAutostartEnabled() const {
        if (!initialized_) {
            return false;
        }
        return std::filesystem::exists(desktop_file_);
    }

    bool updateCommand(const std::string& exec_path, const std::string& args) {
        if (!initialized_) {
            last_error_ = "Not initialized";
            return false;
        }

        try {
            std::string command = exec_path;
            if (!args.empty()) {
                command += " " + args;
            }

            // Update the Exec line in the desktop entry
            std::string new_entry = desktop_entry_;
            size_t exec_pos = new_entry.find("Exec=");
            if (exec_pos == std::string::npos) {
                last_error_ = "Invalid desktop entry format";
                return false;
            }

            size_t line_end = new_entry.find('\n', exec_pos);
            if (line_end == std::string::npos) {
                line_end = new_entry.length();
            }

            new_entry.replace(exec_pos + 5, line_end - (exec_pos + 5), command);
            desktop_entry_ = new_entry;

            // If autostart is enabled, update the file
            if (isAutostartEnabled()) {
                return setAutostart(true);
            }
            return true;
        } catch (const std::exception& e) {
            last_error_ = std::string("Failed to update command: ") + e.what();
            return false;
        }
    }

    std::string getLastError() const {
        return last_error_;
    }

private:
    bool initialized_;
    std::string app_name_;
    std::filesystem::path desktop_file_;
    std::string desktop_entry_;
    std::string last_error_;
};

// Public interface implementation
StartupManager::StartupManager() : pimpl(std::make_unique<Implementation>()) {}
StartupManager::~StartupManager() = default;

bool StartupManager::initialize(const std::string& app_name,
                              const std::string& exec_path,
                              const std::string& icon_path) {
    return pimpl->initialize(app_name, exec_path, icon_path);
}

bool StartupManager::setAutostart(bool enable) {
    return pimpl->setAutostart(enable);
}

bool StartupManager::isAutostartEnabled() const {
    return pimpl->isAutostartEnabled();
}

bool StartupManager::updateCommand(const std::string& exec_path,
                                 const std::string& args) {
    return pimpl->updateCommand(exec_path, args);
}

std::string StartupManager::getLastError() const {
    return pimpl->getLastError();
}

} // namespace phantom_vault 