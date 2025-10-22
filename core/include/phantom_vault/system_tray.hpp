#pragma once

#include "phantom_vault_export.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace phantom_vault {

/**
 * @brief Class for managing the system tray icon and menu
 */
class PHANTOM_VAULT_EXPORT SystemTray {
public:
    /**
     * @brief Menu item structure
     */
    struct MenuItem {
        std::string label;                  ///< Label to display in the menu
        std::function<void()> callback;     ///< Callback to execute when clicked
        bool is_separator = false;          ///< Whether this item is a separator
        bool is_enabled = true;             ///< Whether this item is enabled
        bool is_checked = false;            ///< Whether this item is checked (for checkable items)
        bool is_checkable = false;          ///< Whether this item can be checked
    };

    /**
     * @brief Constructor
     */
    SystemTray();

    /**
     * @brief Destructor
     */
    ~SystemTray();

    /**
     * @brief Initialize the system tray
     * @param icon_path Path to the icon file (PNG format recommended)
     * @param tooltip Tooltip text to show when hovering over the icon
     * @return true if initialization successful, false otherwise
     */
    bool initialize(const std::string& icon_path, const std::string& tooltip);

    /**
     * @brief Set the menu items
     * @param items Vector of menu items to display
     * @return true if menu was set successfully, false otherwise
     */
    bool setMenu(const std::vector<MenuItem>& items);

    /**
     * @brief Show or hide the system tray icon
     * @param visible true to show, false to hide
     */
    void setVisible(bool visible);

    /**
     * @brief Check if the system tray icon is visible
     * @return true if visible, false otherwise
     */
    bool isVisible() const;

    /**
     * @brief Show a notification balloon
     * @param title Title of the notification
     * @param message Message content
     * @param icon_type Type of icon to show (0=None, 1=Info, 2=Warning, 3=Critical)
     * @param timeout_ms How long to show the notification (in milliseconds)
     */
    void showNotification(const std::string& title, const std::string& message,
                         int icon_type = 1, int timeout_ms = 5000);

    /**
     * @brief Set the icon for the system tray
     * @param icon_path Path to the new icon file
     * @return true if icon was set successfully, false otherwise
     */
    bool setIcon(const std::string& icon_path);

    /**
     * @brief Set the tooltip for the system tray icon
     * @param tooltip New tooltip text
     */
    void setTooltip(const std::string& tooltip);

    /**
     * @brief Get the last error message
     * @return The last error message
     */
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class SystemTray

} // namespace phantom_vault 