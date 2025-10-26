/**
 * PhantomVault Keyboard Sequence Detector Implementation
 * 
 * Stub implementation for keyboard sequence detection.
 * TODO: Full implementation in task 4.
 */

#include "keyboard_sequence_detector.hpp"
#include <iostream>

namespace phantomvault {

class KeyboardSequenceDetector::Implementation {
public:
    Implementation() : last_error_() {}
    
    bool initialize() {
        std::cout << "[KeyboardSequenceDetector] Initialized (stub)" << std::endl;
        return true;
    }
    
    bool start() {
        std::cout << "[KeyboardSequenceDetector] Started (stub)" << std::endl;
        return true;
    }
    
    void stop() {
        std::cout << "[KeyboardSequenceDetector] Stopped (stub)" << std::endl;
    }
    
    std::string getLastError() const {
        return last_error_;
    }
    
private:
    std::string last_error_;
};

KeyboardSequenceDetector::KeyboardSequenceDetector() : pimpl(std::make_unique<Implementation>()) {}
KeyboardSequenceDetector::~KeyboardSequenceDetector() = default;

bool KeyboardSequenceDetector::initialize() {
    return pimpl->initialize();
}

bool KeyboardSequenceDetector::start() {
    return pimpl->start();
}

void KeyboardSequenceDetector::stop() {
    pimpl->stop();
}

std::string KeyboardSequenceDetector::getLastError() const {
    return pimpl->getLastError();
}

} // namespace phantomvault