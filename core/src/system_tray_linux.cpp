#include "phantom_vault/system_tray.hpp"
#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QIcon>
#include <QString>
#include <QObject>
#include <memory>
#include <stdexcept>
#include <vector>
#include <filesystem>

namespace phantom_vault {

class SystemTray::Implementation : public QObject {
    Q_OBJECT
public:
    Implementation()
        : QObject(nullptr)
        , app_(nullptr)
        , tray_icon_(nullptr)
        , menu_(nullptr)
        , is_visible_(false)
        , last_error_()
    {
        // Create Qt application if it doesn't exist
        if (!QApplication::instance()) {
            static int argc = 1;
            static char* argv[] = {const_cast<char*>("phantom_vault")};
            app_ = std::make_unique<QApplication>(argc, argv);
        }

        // Create tray icon and menu
        tray_icon_ = std::make_unique<QSystemTrayIcon>();
        menu_ = std::make_unique<QMenu>();
        tray_icon_->setContextMenu(menu_.get());

        // Connect signals
        connect(tray_icon_.get(), &QSystemTrayIcon::activated,
                this, &Implementation::handleActivated);
    }

    ~Implementation() {
        if (tray_icon_) {
            tray_icon_->hide();
        }
    }

    bool initialize(const std::string& icon_path, const std::string& tooltip) {
        if (!QSystemTrayIcon::isSystemTrayAvailable()) {
            last_error_ = "System tray is not available";
            return false;
        }

        if (!setIcon(icon_path)) {
            return false;
        }

        tray_icon_->setToolTip(QString::fromStdString(tooltip));
        return true;
    }

    bool setMenu(const std::vector<MenuItem>& items) {
        menu_->clear();
        menu_items_.clear();
        menu_items_ = items;

        for (const auto& item : items) {
            if (item.is_separator) {
                menu_->addSeparator();
                continue;
            }

            QAction* action = menu_->addAction(QString::fromStdString(item.label));
            action->setEnabled(item.is_enabled);
            
            if (item.is_checkable) {
                action->setCheckable(true);
                action->setChecked(item.is_checked);
            }

            connect(action, &QAction::triggered, this, [this, action]() {
                // Find and execute the corresponding callback
                for (const auto& menu_item : menu_items_) {
                    if (action->text() == QString::fromStdString(menu_item.label)) {
                        if (menu_item.callback) {
                            menu_item.callback();
                        }
                        break;
                    }
                }
            });
        }

        return true;
    }

    void setVisible(bool visible) {
        if (tray_icon_) {
            if (visible) {
                tray_icon_->show();
            } else {
                tray_icon_->hide();
            }
            is_visible_ = visible;
        }
    }

    bool isVisible() const {
        return is_visible_;
    }

    void showNotification(const std::string& title, const std::string& message,
                         int icon_type, int timeout_ms) {
        if (!tray_icon_) return;

        QSystemTrayIcon::MessageIcon qt_icon;
        switch (icon_type) {
            case 0: qt_icon = QSystemTrayIcon::NoIcon; break;
            case 2: qt_icon = QSystemTrayIcon::Warning; break;
            case 3: qt_icon = QSystemTrayIcon::Critical; break;
            default: qt_icon = QSystemTrayIcon::Information; break;
        }

        tray_icon_->showMessage(
            QString::fromStdString(title),
            QString::fromStdString(message),
            qt_icon,
            timeout_ms
        );
    }

    bool setIcon(const std::string& icon_path) {
        // Check if file exists and is readable
        if (!std::filesystem::exists(icon_path)) {
            last_error_ = "Icon file does not exist: " + icon_path;
            return false;
        }

        QIcon icon(QString::fromStdString(icon_path));
        if (icon.isNull()) {
            last_error_ = "Failed to load icon: " + icon_path;
            return false;
        }

        tray_icon_->setIcon(icon);
        return true;
    }

    void setTooltip(const std::string& tooltip) {
        if (tray_icon_) {
            tray_icon_->setToolTip(QString::fromStdString(tooltip));
        }
    }

    std::string getLastError() const {
        return last_error_;
    }

private slots:
    void handleActivated(QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            // Handle left click or double click if needed
        }
    }

private:
    std::unique_ptr<QApplication> app_;
    std::unique_ptr<QSystemTrayIcon> tray_icon_;
    std::unique_ptr<QMenu> menu_;
    std::vector<MenuItem> menu_items_;
    bool is_visible_;
    std::string last_error_;
};

// Public interface implementation
SystemTray::SystemTray() : pimpl(std::make_unique<Implementation>()) {}
SystemTray::~SystemTray() = default;

bool SystemTray::initialize(const std::string& icon_path, const std::string& tooltip) {
    return pimpl->initialize(icon_path, tooltip);
}

bool SystemTray::setMenu(const std::vector<MenuItem>& items) {
    return pimpl->setMenu(items);
}

void SystemTray::setVisible(bool visible) {
    pimpl->setVisible(visible);
}

bool SystemTray::isVisible() const {
    return pimpl->isVisible();
}

void SystemTray::showNotification(const std::string& title, const std::string& message,
                                int icon_type, int timeout_ms) {
    pimpl->showNotification(title, message, icon_type, timeout_ms);
}

bool SystemTray::setIcon(const std::string& icon_path) {
    return pimpl->setIcon(icon_path);
}

void SystemTray::setTooltip(const std::string& tooltip) {
    pimpl->setTooltip(tooltip);
}

std::string SystemTray::getLastError() const {
    return pimpl->getLastError();
}

} // namespace phantom_vault

#include "system_tray_linux.moc" 