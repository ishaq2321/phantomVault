/**
 * PhantomVault Keyboard Sequence Detector
 * 
 * Detects Ctrl+Alt+V sequences and performs invisible keyboard pattern matching
 * for multi-profile password detection with platform-specific implementations.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

namespace phantomvault {

// Forward declarations for X11
#ifdef PLATFORM_LINUX
// Forward declarations to avoid header pollution
typedef char* XPointer;
struct XRecordInterceptData;
void x11KeyboardCallback(char* closure, struct XRecordInterceptData* data);
#endif

/**
 * Detected password pattern
 */
struct PasswordPattern {
    std::string password;
    bool isTemporary = false;  // T+ prefix
    bool isPermanent = false;  // P+ prefix
    std::chrono::system_clock::time_point detectedAt;
};

/**
 * Keyboard sequence detection result
 */
struct SequenceResult {
    bool success = false;
    std::vector<PasswordPattern> patterns;
    std::string message;
    std::string error;
};

/**
 * Platform capabilities for keyboard detection
 */
struct PlatformCapabilities {
    bool supportsInvisibleLogging = false;
    bool supportsHotkeys = false;
    bool requiresPermissions = false;
    std::vector<std::string> requiredPermissions;
};

class KeyboardSequenceDetector {
    friend void x11KeyboardCallback(char* closure, struct XRecordInterceptData* data);
public:
    KeyboardSequenceDetector();
    ~KeyboardSequenceDetector();

    // Initialization and lifecycle
    bool initialize();
    bool start();
    void stop();
    bool isRunning() const;
    
    // Platform capabilities
    PlatformCapabilities getPlatformCapabilities() const;
    bool checkPermissions() const;
    
    // Sequence detection control
    void activateSequenceDetection(int timeoutSeconds = 10);
    void deactivateSequenceDetection();
    bool isSequenceDetectionActive() const;
    
    // Pattern matching
    std::vector<PasswordPattern> extractPasswordPatterns(const std::string& input) const;
    bool isValidPasswordPattern(const std::string& pattern) const;
    
    // Callbacks for detected sequences
    void setOnSequenceDetected(std::function<void()> callback);
    void setOnPasswordDetected(std::function<void(const PasswordPattern&)> callback);
    void setOnSequenceTimeout(std::function<void()> callback);
    
    // Manual input methods (for platforms without invisible logging)
    void showPasswordPrompt();
    void showNotificationPrompt();
    bool processManualInput(const std::string& input);
    
    // Statistics and monitoring
    size_t getDetectionCount() const;
    std::chrono::system_clock::time_point getLastDetectionTime() const;
    
    // Ultra-fast performance enhancements
    void enableHardwareLevelMonitoring();
    void disableHardwareLevelMonitoring();
    void setRealTimePriority(bool enabled);
    void enableLockFreeProcessing();
    void disableLockFreeProcessing();
    
    // Performance metrics
    std::chrono::nanoseconds getAverageResponseTime() const;
    std::chrono::nanoseconds getLastResponseTime() const;
    size_t getProcessedEventsPerSecond() const;
    
    // Error handling
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

} // namespace phantomvault