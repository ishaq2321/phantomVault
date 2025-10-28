#include "core/include/keyboard_sequence_detector.hpp"
#include <iostream>

using namespace phantomvault;

int main() {
    std::cout << "=== KEYBOARD SEQUENCE DETECTOR TEST ===" << std::endl;
    
    KeyboardSequenceDetector detector;
    
    // Test initialization
    if (!detector.initialize()) {
        std::cerr << "Failed to initialize: " << detector.getLastError() << std::endl;
        return 1;
    }
    std::cout << "✓ Initialization successful" << std::endl;
    
    // Test platform capabilities
    auto caps = detector.getPlatformCapabilities();
    std::cout << "✓ Platform capabilities:" << std::endl;
    std::cout << "  - Invisible logging: " << (caps.supportsInvisibleLogging ? "Yes" : "No") << std::endl;
    std::cout << "  - Hotkeys: " << (caps.supportsHotkeys ? "Yes" : "No") << std::endl;
    std::cout << "  - Requires permissions: " << (caps.requiresPermissions ? "Yes" : "No") << std::endl;
    
    // Test password pattern extraction
    auto patterns = detector.extractPasswordPatterns("T+mypassword123 P+permanent normalpass");
    std::cout << "✓ Found " << patterns.size() << " password patterns" << std::endl;
    
    for (const auto& pattern : patterns) {
        std::cout << "  - [" << pattern.password.length() << " chars]";
        if (pattern.isTemporary) std::cout << " (Temporary)";
        if (pattern.isPermanent) std::cout << " (Permanent)";
        std::cout << std::endl;
    }
    
    // Test manual input processing
    bool detected = false;
    detector.setOnPasswordDetected([&](const PasswordPattern& p) {
        detected = true;
        std::cout << "✓ Password detected: [" << p.password.length() << " chars]" << std::endl;
    });
    
    detector.processManualInput("T+testpass123");
    
    if (!detected) {
        std::cerr << "Failed to detect password" << std::endl;
        return 1;
    }
    
    std::cout << "✓ All tests passed!" << std::endl;
    return 0;
}
