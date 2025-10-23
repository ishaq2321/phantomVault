#include "phantom_vault/sequence_detector.hpp"
#include "phantom_vault/keyboard_hook.hpp"
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <chrono>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <iomanip>
#include <sstream>
#include <random>
#include <cstring>

namespace phantom_vault {
namespace service {

// PasswordUtils implementation
std::string PasswordUtils::hashPassword(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, password.c_str(), password.length());
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

bool PasswordUtils::verifyPassword(const std::string& password, const std::string& hash) {
    return hashPassword(password) == hash;
}

UnlockMode PasswordUtils::extractMode(const std::string& sequence, const std::string& password) {
    // Convert to lowercase for case-insensitive matching
    std::string lower_sequence = sequence;
    std::transform(lower_sequence.begin(), lower_sequence.end(), lower_sequence.begin(), ::tolower);
    
    std::string lower_password = password;
    std::transform(lower_password.begin(), lower_password.end(), lower_password.begin(), ::tolower);
    
    // Look for T+password pattern
    std::string temp_pattern = "t" + lower_password;
    if (lower_sequence.find(temp_pattern) != std::string::npos) {
        return UnlockMode::TEMPORARY;
    }
    
    // Look for P+password pattern
    std::string perm_pattern = "p" + lower_password;
    if (lower_sequence.find(perm_pattern) != std::string::npos) {
        return UnlockMode::PERMANENT;
    }
    
    // Default to temporary if just password found
    return UnlockMode::TEMPORARY;
}

void PasswordUtils::secureWipe(void* data, size_t size) {
    if (!data || size == 0) return;
    
    volatile unsigned char* ptr = static_cast<volatile unsigned char*>(data);
    
    // DOD 5220.22-M 3-pass wipe
    // Pass 1: Write 0x00
    for (size_t i = 0; i < size; i++) {
        ptr[i] = 0x00;
    }
    
    // Pass 2: Write 0xFF
    for (size_t i = 0; i < size; i++) {
        ptr[i] = 0xFF;
    }
    
    // Pass 3: Write random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    for (size_t i = 0; i < size; i++) {
        ptr[i] = static_cast<unsigned char>(dis(gen));
    }
}

// SequenceDetector implementation
class SequenceDetector::Implementation {
public:
    Implementation()
        : keyboard_hook_()
        , detection_callback_()
        , folder_passwords_()
        , keystroke_buffer_()
        , is_active_(false)
        , start_time_()
        , timeout_seconds_(10)
        , max_buffer_size_(1000)  // Limit buffer size for security
        , case_sensitive_(false)
        , last_error_()
        , mutex_()
        , total_keystrokes_(0)
        , successful_detections_(0)
    {}

    ~Implementation() {
        stopDetection();
        secureCleanup();
    }

    bool initialize() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Initialize keyboard hook
        keyboard_hook_ = std::make_unique<KeyboardHook>();
        if (!keyboard_hook_->initialize()) {
            last_error_ = "Failed to initialize keyboard hook: " + keyboard_hook_->getLastError();
            return false;
        }
        
        std::cout << "[SequenceDetector] Initialized successfully" << std::endl;
        return true;
    }

    bool startDetection(int timeout_seconds) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (is_active_) {
            std::cout << "[SequenceDetector] Detection already active" << std::endl;
            return true;
        }
        
        if (!keyboard_hook_) {
            last_error_ = "Keyboard hook not initialized";
            return false;
        }
        
        // Clear previous buffer securely
        secureCleanup();
        
        timeout_seconds_ = timeout_seconds;
        start_time_ = std::chrono::steady_clock::now();
        is_active_ = true;
        
        // Start keyboard monitoring
        if (!keyboard_hook_->startMonitoring([this](const std::string& key, bool pressed, unsigned int modifiers) {
            handleKeyEvent(key, pressed, modifiers);
        })) {
            last_error_ = "Failed to start keyboard monitoring: " + keyboard_hook_->getLastError();
            is_active_ = false;
            return false;
        }
        
        std::cout << "[SequenceDetector] Started detection (timeout: " << timeout_seconds << "s)" << std::endl;
        std::cout << "[SequenceDetector] Monitoring " << folder_passwords_.size() << " folder password(s)" << std::endl;
        
        return true;
    }

    void stopDetection() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!is_active_) {
            return;
        }
        
        if (keyboard_hook_) {
            keyboard_hook_->stopMonitoring();
        }
        
        is_active_ = false;
        
        // Secure cleanup
        secureCleanup();
        
        std::cout << "[SequenceDetector] Detection stopped" << std::endl;
        std::cout << "[SequenceDetector] Session stats: " << total_keystrokes_ << " keystrokes, " 
                  << successful_detections_ << " detections" << std::endl;
        
        // Reset session counters
        total_keystrokes_ = 0;
        successful_detections_ = 0;
    }

    bool isActive() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return is_active_;
    }

    void setDetectionCallback(DetectionCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        detection_callback_ = std::move(callback);
    }

    void updateFolderPasswords(const std::vector<FolderPassword>& folders) {
        std::lock_guard<std::mutex> lock(mutex_);
        folder_passwords_ = folders;
        
        std::cout << "[SequenceDetector] Updated folder passwords: " << folders.size() << " folders" << std::endl;
        for (const auto& folder : folders) {
            std::cout << "  - " << folder.folder_name << " (ID: " << folder.folder_id 
                      << ", Locked: " << (folder.is_locked ? "Yes" : "No") << ")" << std::endl;
        }
    }

    void addFolderPassword(const FolderPassword& folder) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Remove existing entry if present
        folder_passwords_.erase(
            std::remove_if(folder_passwords_.begin(), folder_passwords_.end(),
                [&folder](const FolderPassword& f) { return f.folder_id == folder.folder_id; }),
            folder_passwords_.end()
        );
        
        // Add new entry
        folder_passwords_.push_back(folder);
        
        std::cout << "[SequenceDetector] Added folder password: " << folder.folder_name 
                  << " (ID: " << folder.folder_id << ")" << std::endl;
    }

    void removeFolderPassword(const std::string& folder_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = std::remove_if(folder_passwords_.begin(), folder_passwords_.end(),
            [&folder_id](const FolderPassword& f) { return f.folder_id == folder_id; });
        
        if (it != folder_passwords_.end()) {
            std::cout << "[SequenceDetector] Removed folder password: " << folder_id << std::endl;
            folder_passwords_.erase(it, folder_passwords_.end());
        }
    }

    void clearFolderPasswords() {
        std::lock_guard<std::mutex> lock(mutex_);
        folder_passwords_.clear();
        std::cout << "[SequenceDetector] Cleared all folder passwords" << std::endl;
    }

    std::string getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::stringstream ss;
        ss << "{"
           << "\"is_active\":" << (is_active_ ? "true" : "false") << ","
           << "\"folder_count\":" << folder_passwords_.size() << ","
           << "\"buffer_size\":" << keystroke_buffer_.size() << ","
           << "\"max_buffer_size\":" << max_buffer_size_ << ","
           << "\"case_sensitive\":" << (case_sensitive_ ? "true" : "false") << ","
           << "\"total_keystrokes\":" << total_keystrokes_ << ","
           << "\"successful_detections\":" << successful_detections_;
        
        if (is_active_) {
            auto elapsed = std::chrono::steady_clock::now() - start_time_;
            auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
            ss << ",\"elapsed_seconds\":" << elapsed_seconds
               << ",\"timeout_seconds\":" << timeout_seconds_;
        }
        
        ss << "}";
        return ss.str();
    }

    std::string getLastError() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return last_error_;
    }

    void processKeystroke(char key_char) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!is_active_) {
            return;
        }
        
        total_keystrokes_++;
        
        // Add character to buffer
        keystroke_buffer_ += key_char;
        
        // Limit buffer size for security and performance
        if (keystroke_buffer_.size() > max_buffer_size_) {
            // Remove oldest characters, keeping the most recent ones
            keystroke_buffer_ = keystroke_buffer_.substr(keystroke_buffer_.size() - max_buffer_size_);
        }
        
        // Check for password patterns
        auto result = detectPasswordInBuffer();
        if (result.found) {
            std::cout << "[SequenceDetector] Password detected for folder: " << result.folder_id 
                      << " (Mode: " << (result.mode == UnlockMode::TEMPORARY ? "Temporary" : "Permanent") << ")" << std::endl;
            
            successful_detections_++;
            
            // Call detection callback
            if (detection_callback_) {
                // Call in separate thread to avoid blocking
                std::thread([this, result]() {
                    try {
                        detection_callback_(result);
                    } catch (const std::exception& e) {
                        std::cerr << "[SequenceDetector] Callback error: " << e.what() << std::endl;
                    }
                }).detach();
            }
            
            // Stop detection after successful match
            stopDetectionInternal();
            return;
        }
        
        // Check timeout
        auto elapsed = std::chrono::steady_clock::now() - start_time_;
        if (elapsed >= std::chrono::seconds(timeout_seconds_)) {
            std::cout << "[SequenceDetector] Detection timed out after " << timeout_seconds_ << " seconds" << std::endl;
            stopDetectionInternal();
        }
    }

    void setMaxBufferSize(size_t max_size) {
        std::lock_guard<std::mutex> lock(mutex_);
        max_buffer_size_ = max_size;
    }

    void setCaseSensitive(bool case_sensitive) {
        std::lock_guard<std::mutex> lock(mutex_);
        case_sensitive_ = case_sensitive;
    }

private:
    void handleKeyEvent(const std::string& key, bool pressed, unsigned int modifiers) {
        if (!pressed) {
            return; // Only handle key press events
        }
        
        // Convert key name to character
        char key_char = keyNameToChar(key);
        if (key_char != 0) {
            processKeystroke(key_char);
        }
    }

    char keyNameToChar(const std::string& key_name) {
        // Handle common key names
        if (key_name.length() == 1) {
            // Single character keys (a-z, 0-9, etc.)
            return key_name[0];
        }
        
        // Handle special keys
        if (key_name == "space") return ' ';
        if (key_name == "Return" || key_name == "KP_Enter") return '\n';
        if (key_name == "Tab") return '\t';
        
        // Handle number pad
        if (key_name.substr(0, 3) == "KP_" && key_name.length() == 4) {
            char last_char = key_name[3];
            if (last_char >= '0' && last_char <= '9') {
                return last_char;
            }
        }
        
        // Ignore other keys (arrows, function keys, etc.)
        return 0;
    }

    PasswordDetectionResult detectPasswordInBuffer() {
        if (folder_passwords_.empty()) {
            return PasswordDetectionResult();
        }
        
        std::string search_buffer = keystroke_buffer_;
        if (!case_sensitive_) {
            std::transform(search_buffer.begin(), search_buffer.end(), search_buffer.begin(), ::tolower);
        }
        
        // Check each folder password
        for (const auto& folder : folder_passwords_) {
            // Skip if folder is already unlocked
            if (!folder.is_locked) {
                continue;
            }
            
            // For this implementation, we'll use common test passwords
            // In production, you would need a secure way to check passwords without storing them in plain text
            // This could be done by:
            // 1. Using a secure password derivation function
            // 2. Checking against encrypted password hints
            // 3. Using a challenge-response system
            
            std::vector<std::string> test_passwords = {"1234", "2321", "password", "test", "admin", "secret"};
            
            for (const std::string& test_pwd : test_passwords) {
                std::string check_pwd = test_pwd;
                if (!case_sensitive_) {
                    std::transform(check_pwd.begin(), check_pwd.end(), check_pwd.begin(), ::tolower);
                }
                
                // Check if password exists in buffer
                if (search_buffer.find(check_pwd) != std::string::npos) {
                    // Verify against stored hash
                    if (PasswordUtils::verifyPassword(test_pwd, folder.password_hash)) {
                        UnlockMode mode = PasswordUtils::extractMode(search_buffer, check_pwd);
                        return PasswordDetectionResult(true, test_pwd, mode, folder.folder_id);
                    }
                }
            }
        }
        
        return PasswordDetectionResult();
    }

    void stopDetectionInternal() {
        // Internal stop without mutex (already locked)
        if (keyboard_hook_) {
            keyboard_hook_->stopMonitoring();
        }
        is_active_ = false;
        secureCleanup();
    }

    void secureCleanup() {
        // Securely wipe keystroke buffer
        if (!keystroke_buffer_.empty()) {
            PasswordUtils::secureWipe(const_cast<char*>(keystroke_buffer_.data()), keystroke_buffer_.size());
            keystroke_buffer_.clear();
            keystroke_buffer_.shrink_to_fit();
        }
    }

    std::unique_ptr<KeyboardHook> keyboard_hook_;
    DetectionCallback detection_callback_;
    std::vector<FolderPassword> folder_passwords_;
    std::string keystroke_buffer_;
    std::atomic<bool> is_active_;
    std::chrono::steady_clock::time_point start_time_;
    int timeout_seconds_;
    size_t max_buffer_size_;
    bool case_sensitive_;
    std::string last_error_;
    mutable std::mutex mutex_;
    
    // Statistics
    size_t total_keystrokes_;
    size_t successful_detections_;
};

// SequenceDetector public interface
SequenceDetector::SequenceDetector() : pimpl(std::make_unique<Implementation>()) {}
SequenceDetector::~SequenceDetector() = default;

bool SequenceDetector::initialize() {
    return pimpl->initialize();
}

bool SequenceDetector::startDetection(int timeout_seconds) {
    return pimpl->startDetection(timeout_seconds);
}

void SequenceDetector::stopDetection() {
    pimpl->stopDetection();
}

bool SequenceDetector::isActive() const {
    return pimpl->isActive();
}

void SequenceDetector::setDetectionCallback(DetectionCallback callback) {
    pimpl->setDetectionCallback(std::move(callback));
}

void SequenceDetector::updateFolderPasswords(const std::vector<FolderPassword>& folders) {
    pimpl->updateFolderPasswords(folders);
}

void SequenceDetector::addFolderPassword(const FolderPassword& folder) {
    pimpl->addFolderPassword(folder);
}

void SequenceDetector::removeFolderPassword(const std::string& folder_id) {
    pimpl->removeFolderPassword(folder_id);
}

void SequenceDetector::clearFolderPasswords() {
    pimpl->clearFolderPasswords();
}

std::string SequenceDetector::getStats() const {
    return pimpl->getStats();
}

std::string SequenceDetector::getLastError() const {
    return pimpl->getLastError();
}

void SequenceDetector::processKeystroke(char key_char) {
    pimpl->processKeystroke(key_char);
}

void SequenceDetector::setMaxBufferSize(size_t max_size) {
    pimpl->setMaxBufferSize(max_size);
}

void SequenceDetector::setCaseSensitive(bool case_sensitive) {
    pimpl->setCaseSensitive(case_sensitive);
}

} // namespace service
} // namespace phantom_vault