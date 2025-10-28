/**
 * Enhanced Keyboard Sequence Detector
 * 
 * Extends the base keyboard detector with real platform-specific implementations
 * for production-ready keyboard monitoring and sequence detection.
 */

#pragma once

#include "keyboard_sequence_detector.hpp"
#include <memory>

namespace phantomvault {

/**
 * Enhanced keyboard detection with real platform implementations
 */
class EnhancedKeyboardDetector : public KeyboardSequenceDetector {
public:
    EnhancedKeyboardDetector();
    ~EnhancedKeyboardDetector();

    // Enhanced initialization with real platform detection
    bool initializeWithRealPlatformDetection();
    
    // Real X11 keyboard monitoring (Linux)
    bool enableX11KeyboardMonitoring();
    void disableX11KeyboardMonitoring();
    
    // Real Windows keyboard hooks
    bool enableWindowsKeyboardHooks();
    void disableWindowsKeyboardHooks();
    
    // Real macOS event taps
    bool enableMacOSEventTaps();
    void disableMacOSEventTaps();
    
    // Enhanced sequence detection with real capture
    bool startRealKeyboardCapture();
    void stopRealKeyboardCapture();
    
    // Security enhancements
    bool validateSecureSequenceCapture() const;
    void clearSecurityBuffers();
    
    // Platform-specific status
    std::string getPlatformImplementationStatus() const;
    bool isRealImplementationActive() const;

private:
    class EnhancedImplementation;
    std::unique_ptr<EnhancedImplementation> enhanced_impl_;
};

} // namespace phantomvault