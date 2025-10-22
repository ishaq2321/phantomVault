#include "phantom_vault/process_concealer.hpp"
#include <sys/prctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstring>
#include <array>
#include <errno.h>

namespace phantom_vault {

namespace {
    // Maximum length for process name in Linux
    constexpr size_t MAX_PROC_NAME_LEN = 16;

    // Get process name from /proc/self/comm
    std::string readProcName() {
        std::ifstream comm("/proc/self/comm");
        std::string name;
        std::getline(comm, name);
        return name;
    }

    // Get process cmdline
    std::string readProcCmdline() {
        std::ifstream cmdline("/proc/self/cmdline");
        std::string line;
        std::getline(cmdline, line, '\0');
        return line;
    }

    // Hide process by unlinking its /proc entry (requires root)
    bool unlinkProcEntry() {
        std::string proc_path = "/proc/self";
        return unlink(proc_path.c_str()) == 0;
    }

    // Modify process visibility in /proc
    bool setProcessVisibility(bool visible) {
        std::string proc_path = "/proc/self";
        mode_t mode = visible ? 0755 : 0000;
        return chmod(proc_path.c_str(), mode) == 0;
    }
}

class ProcessConcealer::Implementation {
public:
    Implementation()
        : initialized_(false)
        , original_name_()
        , current_name_()
        , hidden_(false)
        , last_error_()
    {}

    bool initialize() {
        try {
            // Store original process name
            original_name_ = readProcName();
            current_name_ = original_name_;
            initialized_ = true;
            return true;
        } catch (const std::exception& e) {
            last_error_ = std::string("Initialization failed: ") + e.what();
            return false;
        }
    }

    bool setProcessName(const std::string& name) {
        if (!initialized_) {
            last_error_ = "Not initialized";
            return false;
        }

        if (name.empty()) {
            last_error_ = "Process name cannot be empty";
            return false;
        }

        try {
            // Truncate name if necessary (Linux limit is 16 chars)
            std::string truncated_name = name.substr(0, MAX_PROC_NAME_LEN - 1);

            // Set process name using prctl
            if (prctl(PR_SET_NAME, truncated_name.c_str(), 0, 0, 0) != 0) {
                last_error_ = std::string("Failed to set process name: ") + strerror(errno);
                return false;
            }

            // Update /proc/self/comm
            std::ofstream comm("/proc/self/comm");
            if (!comm) {
                last_error_ = "Failed to open /proc/self/comm for writing";
                return false;
            }
            comm << truncated_name;
            
            current_name_ = truncated_name;
            return true;
        } catch (const std::exception& e) {
            last_error_ = std::string("Failed to set process name: ") + e.what();
            return false;
        }
    }

    bool hideProcess() {
        if (!initialized_) {
            last_error_ = "Not initialized";
            return false;
        }

        if (hidden_) {
            return true;  // Already hidden
        }

        try {
            // Try to hide the process using various methods
            bool success = false;

            // Method 1: Try to modify process visibility in /proc
            if (setProcessVisibility(false)) {
                success = true;
            }

            // Method 2: Try to unlink /proc entry (requires root)
            if (!success && unlinkProcEntry()) {
                success = true;
            }

            // Method 3: Disguise the process name
            if (!success) {
                std::string random_name = "kworker/" + std::to_string(getpid() % 100) + ":0";
                if (!setProcessName(random_name)) {
                    last_error_ = "Failed to hide process: all methods failed";
                    return false;
                }
                success = true;
            }

            hidden_ = success;
            return success;
        } catch (const std::exception& e) {
            last_error_ = std::string("Failed to hide process: ") + e.what();
            return false;
        }
    }

    bool showProcess() {
        if (!initialized_) {
            last_error_ = "Not initialized";
            return false;
        }

        if (!hidden_) {
            return true;  // Already visible
        }

        try {
            // Restore process visibility
            setProcessVisibility(true);

            // Restore original process name
            if (!setProcessName(original_name_)) {
                last_error_ = "Failed to restore process name";
                return false;
            }

            hidden_ = false;
            return true;
        } catch (const std::exception& e) {
            last_error_ = std::string("Failed to show process: ") + e.what();
            return false;
        }
    }

    bool isHidden() const {
        return hidden_;
    }

    std::string getCurrentProcessName() const {
        return current_name_;
    }

    std::string getOriginalProcessName() const {
        return original_name_;
    }

    std::string getLastError() const {
        return last_error_;
    }

private:
    bool initialized_;
    std::string original_name_;
    std::string current_name_;
    bool hidden_;
    std::string last_error_;
};

// Public interface implementation
ProcessConcealer::ProcessConcealer() : pimpl(std::make_unique<Implementation>()) {}
ProcessConcealer::~ProcessConcealer() = default;

bool ProcessConcealer::initialize() {
    return pimpl->initialize();
}

bool ProcessConcealer::setProcessName(const std::string& name) {
    return pimpl->setProcessName(name);
}

bool ProcessConcealer::hideProcess() {
    return pimpl->hideProcess();
}

bool ProcessConcealer::showProcess() {
    return pimpl->showProcess();
}

bool ProcessConcealer::isHidden() const {
    return pimpl->isHidden();
}

std::string ProcessConcealer::getCurrentProcessName() const {
    return pimpl->getCurrentProcessName();
}

std::string ProcessConcealer::getOriginalProcessName() const {
    return pimpl->getOriginalProcessName();
}

std::string ProcessConcealer::getLastError() const {
    return pimpl->getLastError();
}

} // namespace phantom_vault 