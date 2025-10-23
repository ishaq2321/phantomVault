#pragma once

#include "phantom_vault_export.h"
#include <string>
#include <memory>

namespace phantom_vault {

/**
 * @brief Class for managing application autostart
 */
class PHANTOM_VAULT_EXPORT StartupManager {
public:
    /**
     * @brief Constructor
     */
    StartupManager();

    /**
     * @brief Destructor
     */
    ~StartupManager();

    /**
     * @brief Initialize the startup manager
     * @param app_name Name of the application
     * @param exec_path Path to the executable
     * @param icon_path Path to the application icon
     * @return true if initialization successful, false otherwise
     */
    bool initialize(const std::string& app_name,
                   const std::string& exec_path,
                   const std::string& icon_path);

    /**
     * @brief Enable or disable autostart
     * @param enable true to enable, false to disable
     * @return true if operation successful, false otherwise
     */
    bool setAutostart(bool enable);

    /**
     * @brief Check if autostart is enabled
     * @return true if enabled, false otherwise
     */
    bool isAutostartEnabled() const;

    /**
     * @brief Update the autostart command
     * @param exec_path New path to the executable
     * @param args Additional command line arguments
     * @return true if update successful, false otherwise
     */
    bool updateCommand(const std::string& exec_path,
                      const std::string& args = "");

    /**
     * @brief Get the last error message
     * @return The last error message
     */
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class StartupManager

} // namespace phantom_vault 