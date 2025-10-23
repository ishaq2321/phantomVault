#include "phantom_vault/directory_protection.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>

namespace phantom_vault {
namespace service {

class DirectoryProtection::Implementation {
public:
    Implementation()
        : auto_restore_enabled_(true)
        , violation_callback_()
        , monitored_directories_()
        , violation_history_()
        , last_error_()
        , mutex_()
    {}

    ~Implementation() {
        // Clean up any monitoring threads
    }

    bool initialize() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if we have the necessary tools
        if (!checkSystemCapabilities()) {
            last_error_ = "System does not support required protection methods";
            return false;
        }
        
        std::cout << "[DirectoryProtection] Initialized successfully" << std::endl;
        return true;
    }

    ProtectionResult protectDirectory(const std::string& directory_path, ProtectionMethod method) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::cout << "[DirectoryProtection] Protecting directory: " << directory_path << std::endl;
        
        // Check if directory exists
        if (!directoryExists(directory_path)) {
            return ProtectionResult(false, "Directory does not exist: " + directory_path);
        }
        
        ProtectionResult result;
        result.method_used = method;
        
        switch (method) {
            case ProtectionMethod::IMMUTABLE_ATTR:
                result = applyImmutableAttribute(directory_path);
                break;
            case ProtectionMethod::PERMISSIONS:
                result = applyPermissionProtection(directory_path);
                break;
            case ProtectionMethod::BOTH:
                result = applyImmutableAttribute(directory_path);
                if (result.success) {
                    auto perm_result = applyPermissionProtection(directory_path);
                    if (!perm_result.success) {
                        result.error_message += " (Permission protection failed: " + perm_result.error_message + ")";
                    }
                }
                break;
        }
        
        if (result.success) {
            std::cout << "[DirectoryProtection] Successfully protected: " << directory_path << std::endl;
            result.status = ProtectionStatus::PROTECTED;
        } else {
            std::cout << "[DirectoryProtection] Failed to protect: " << directory_path 
                      << " - " << result.error_message << std::endl;
            result.status = ProtectionStatus::ERROR;
        }
        
        return result;
    }

    ProtectionResult unprotectDirectory(const std::string& directory_path) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::cout << "[DirectoryProtection] Removing protection from: " << directory_path << std::endl;
        
        if (!directoryExists(directory_path)) {
            return ProtectionResult(false, "Directory does not exist: " + directory_path);
        }
        
        // Remove immutable attribute
        ProtectionResult result = removeImmutableAttribute(directory_path);
        
        // Also restore normal permissions
        if (result.success) {
            auto perm_result = restoreNormalPermissions(directory_path);
            if (!perm_result.success) {
                result.error_message += " (Permission restoration failed: " + perm_result.error_message + ")";
            }
        }
        
        if (result.success) {
            std::cout << "[DirectoryProtection] Successfully unprotected: " << directory_path << std::endl;
            result.status = ProtectionStatus::UNPROTECTED;
        }
        
        return result;
    }

    ProtectionStatus checkProtectionStatus(const std::string& directory_path) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!directoryExists(directory_path)) {
            return ProtectionStatus::MISSING;
        }
        
        // Check for immutable attribute
        if (hasImmutableAttribute(directory_path)) {
            return ProtectionStatus::PROTECTED;
        }
        
        // Check for restrictive permissions
        if (hasRestrictivePermissions(directory_path)) {
            return ProtectionStatus::PROTECTED;
        }
        
        return ProtectionStatus::UNPROTECTED;
    }

    int verifyAndRestoreProtection() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        int restored_count = 0;
        
        for (const auto& entry : monitored_directories_) {
            const std::string& path = entry.first;
            ProtectionMethod method = entry.second;
            
            ProtectionStatus status = checkProtectionStatus(path);
            
            if (status == ProtectionStatus::UNPROTECTED) {
                std::cout << "[DirectoryProtection] Restoring protection for: " << path << std::endl;
                
                // Log security violation
                SecurityViolation violation(ViolationType::PROTECTION_REMOVED, path, 
                                          "Protection was removed, restoring automatically");
                logSecurityViolation(violation);
                
                // Restore protection
                ProtectionResult result = protectDirectory(path, method);
                if (result.success) {
                    restored_count++;
                }
            } else if (status == ProtectionStatus::MISSING) {
                std::cout << "[DirectoryProtection] Directory missing: " << path << std::endl;
                
                SecurityViolation violation(ViolationType::DIRECTORY_DELETED, path, 
                                          "Monitored directory was deleted");
                logSecurityViolation(violation);
            }
        }
        
        if (restored_count > 0) {
            std::cout << "[DirectoryProtection] Restored protection for " << restored_count << " directories" << std::endl;
        }
        
        return restored_count;
    }

    bool addMonitoredDirectory(const std::string& directory_path, ProtectionMethod method) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!directoryExists(directory_path)) {
            last_error_ = "Directory does not exist: " + directory_path;
            return false;
        }
        
        monitored_directories_[directory_path] = method;
        std::cout << "[DirectoryProtection] Added to monitoring: " << directory_path << std::endl;
        return true;
    }

    bool removeMonitoredDirectory(const std::string& directory_path) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = monitored_directories_.find(directory_path);
        if (it != monitored_directories_.end()) {
            monitored_directories_.erase(it);
            std::cout << "[DirectoryProtection] Removed from monitoring: " << directory_path << std::endl;
            return true;
        }
        
        return false;
    }

    std::vector<std::string> getMonitoredDirectories() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<std::string> directories;
        for (const auto& entry : monitored_directories_) {
            directories.push_back(entry.first);
        }
        return directories;
    }

    void setViolationCallback(ViolationCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        violation_callback_ = callback;
    }

    bool isImmutableAttributeSupported() {
        // Test if chattr command is available and working
        int result = system("which chattr > /dev/null 2>&1");
        if (result != 0) {
            return false;
        }
        
        // Test if lsattr command is available
        result = system("which lsattr > /dev/null 2>&1");
        return (result == 0);
    }

    std::string getProtectionInfo(const std::string& directory_path) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::stringstream info;
        info << "Directory: " << directory_path << "\n";
        
        if (!directoryExists(directory_path)) {
            info << "Status: MISSING\n";
            return info.str();
        }
        
        ProtectionStatus status = checkProtectionStatus(directory_path);
        info << "Status: ";
        switch (status) {
            case ProtectionStatus::PROTECTED:
                info << "PROTECTED\n";
                break;
            case ProtectionStatus::UNPROTECTED:
                info << "UNPROTECTED\n";
                break;
            case ProtectionStatus::MISSING:
                info << "MISSING\n";
                break;
            case ProtectionStatus::ERROR:
                info << "ERROR\n";
                break;
        }
        
        // Check immutable attribute
        if (hasImmutableAttribute(directory_path)) {
            info << "Immutable: YES\n";
        } else {
            info << "Immutable: NO\n";
        }
        
        // Check permissions
        struct stat st;
        if (stat(directory_path.c_str(), &st) == 0) {
            info << "Permissions: " << std::oct << (st.st_mode & 0777) << std::dec << "\n";
            info << "Owner: " << st.st_uid << "\n";
            info << "Group: " << st.st_gid << "\n";
        }
        
        // Check if monitored
        auto it = monitored_directories_.find(directory_path);
        if (it != monitored_directories_.end()) {
            info << "Monitored: YES\n";
            info << "Method: ";
            switch (it->second) {
                case ProtectionMethod::IMMUTABLE_ATTR:
                    info << "IMMUTABLE_ATTR\n";
                    break;
                case ProtectionMethod::PERMISSIONS:
                    info << "PERMISSIONS\n";
                    break;
                case ProtectionMethod::BOTH:
                    info << "BOTH\n";
                    break;
            }
        } else {
            info << "Monitored: NO\n";
        }
        
        return info.str();
    }

    void setAutoRestoreEnabled(bool enabled) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto_restore_enabled_ = enabled;
    }

    bool isAutoRestoreEnabled() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return auto_restore_enabled_;
    }

    std::string getLastError() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return last_error_;
    }

    std::vector<SecurityViolation> getViolationHistory(size_t max_entries) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (max_entries == 0 || max_entries >= violation_history_.size()) {
            return violation_history_;
        }
        
        // Return the most recent entries
        auto start_it = violation_history_.end() - max_entries;
        return std::vector<SecurityViolation>(start_it, violation_history_.end());
    }

    void clearViolationHistory() {
        std::lock_guard<std::mutex> lock(mutex_);
        violation_history_.clear();
    }

private:
    bool checkSystemCapabilities() {
        // Check if we can use chattr/lsattr
        if (!isImmutableAttributeSupported()) {
            std::cout << "[DirectoryProtection] Warning: chattr/lsattr not available" << std::endl;
        }
        
        // Check if we can modify file permissions
        if (getuid() == 0) {
            std::cout << "[DirectoryProtection] Warning: Running as root is not recommended" << std::endl;
        }
        
        return true; // We can always try permission-based protection
    }

    bool directoryExists(const std::string& path) {
        struct stat st;
        return (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode));
    }

    ProtectionResult applyImmutableAttribute(const std::string& directory_path) {
        if (!isImmutableAttributeSupported()) {
            return ProtectionResult(false, "Immutable attributes not supported on this system");
        }
        
        std::string command = "chattr +i \"" + directory_path + "\" 2>/dev/null";
        int result = system(command.c_str());
        
        if (result == 0) {
            return ProtectionResult(true);
        } else {
            return ProtectionResult(false, "Failed to set immutable attribute (chattr +i failed)");
        }
    }

    ProtectionResult removeImmutableAttribute(const std::string& directory_path) {
        if (!isImmutableAttributeSupported()) {
            return ProtectionResult(true); // Nothing to remove
        }
        
        std::string command = "chattr -i \"" + directory_path + "\" 2>/dev/null";
        int result = system(command.c_str());
        
        if (result == 0) {
            return ProtectionResult(true);
        } else {
            return ProtectionResult(false, "Failed to remove immutable attribute (chattr -i failed)");
        }
    }

    ProtectionResult applyPermissionProtection(const std::string& directory_path) {
        // Set restrictive permissions (owner read-only)
        if (chmod(directory_path.c_str(), S_IRUSR | S_IXUSR) == 0) {
            return ProtectionResult(true);
        } else {
            return ProtectionResult(false, "Failed to set restrictive permissions");
        }
    }

    ProtectionResult restoreNormalPermissions(const std::string& directory_path) {
        // Restore normal permissions (owner read/write/execute)
        if (chmod(directory_path.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0) {
            return ProtectionResult(true);
        } else {
            return ProtectionResult(false, "Failed to restore normal permissions");
        }
    }

    bool hasImmutableAttribute(const std::string& directory_path) {
        if (!isImmutableAttributeSupported()) {
            return false;
        }
        
        std::string command = "lsattr -d \"" + directory_path + "\" 2>/dev/null";
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            return false;
        }
        
        char buffer[256];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
        
        // Check if 'i' flag is present in the attributes
        return (result.find('i') != std::string::npos);
    }

    bool hasRestrictivePermissions(const std::string& directory_path) {
        struct stat st;
        if (stat(directory_path.c_str(), &st) != 0) {
            return false;
        }
        
        // Check if permissions are restrictive (no write access)
        return ((st.st_mode & S_IWUSR) == 0);
    }

    void logSecurityViolation(const SecurityViolation& violation) {
        violation_history_.push_back(violation);
        
        // Keep history size manageable
        if (violation_history_.size() > 1000) {
            violation_history_.erase(violation_history_.begin(), 
                                   violation_history_.begin() + 100);
        }
        
        // Call violation callback if set
        if (violation_callback_) {
            violation_callback_(violation);
        }
        
        // Log to system log
        std::cout << "[DirectoryProtection] SECURITY VIOLATION: " 
                  << violation.description << " (" << violation.directory_path << ")" << std::endl;
    }

    std::atomic<bool> auto_restore_enabled_;
    ViolationCallback violation_callback_;
    std::map<std::string, ProtectionMethod> monitored_directories_;
    std::vector<SecurityViolation> violation_history_;
    std::string last_error_;
    mutable std::mutex mutex_;
};

// DirectoryProtection public interface
DirectoryProtection::DirectoryProtection() : pimpl(std::make_unique<Implementation>()) {}
DirectoryProtection::~DirectoryProtection() = default;

bool DirectoryProtection::initialize() {
    return pimpl->initialize();
}

ProtectionResult DirectoryProtection::protectDirectory(const std::string& directory_path, ProtectionMethod method) {
    return pimpl->protectDirectory(directory_path, method);
}

ProtectionResult DirectoryProtection::unprotectDirectory(const std::string& directory_path) {
    return pimpl->unprotectDirectory(directory_path);
}

ProtectionStatus DirectoryProtection::checkProtectionStatus(const std::string& directory_path) {
    return pimpl->checkProtectionStatus(directory_path);
}

int DirectoryProtection::verifyAndRestoreProtection() {
    return pimpl->verifyAndRestoreProtection();
}

bool DirectoryProtection::addMonitoredDirectory(const std::string& directory_path, ProtectionMethod method) {
    return pimpl->addMonitoredDirectory(directory_path, method);
}

bool DirectoryProtection::removeMonitoredDirectory(const std::string& directory_path) {
    return pimpl->removeMonitoredDirectory(directory_path);
}

std::vector<std::string> DirectoryProtection::getMonitoredDirectories() const {
    return pimpl->getMonitoredDirectories();
}

void DirectoryProtection::setViolationCallback(ViolationCallback callback) {
    pimpl->setViolationCallback(callback);
}

bool DirectoryProtection::isImmutableAttributeSupported() {
    return pimpl->isImmutableAttributeSupported();
}

std::string DirectoryProtection::getProtectionInfo(const std::string& directory_path) {
    return pimpl->getProtectionInfo(directory_path);
}

void DirectoryProtection::setAutoRestoreEnabled(bool enabled) {
    pimpl->setAutoRestoreEnabled(enabled);
}

bool DirectoryProtection::isAutoRestoreEnabled() const {
    return pimpl->isAutoRestoreEnabled();
}

std::string DirectoryProtection::getLastError() const {
    return pimpl->getLastError();
}

std::vector<SecurityViolation> DirectoryProtection::getViolationHistory(size_t max_entries) const {
    return pimpl->getViolationHistory(max_entries);
}

void DirectoryProtection::clearViolationHistory() {
    pimpl->clearViolationHistory();
}

} // namespace service
} // namespace phantom_vault