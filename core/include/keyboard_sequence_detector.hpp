/**
 * PhantomVault Keyboard Sequence Detector
 * 
 * Detects Ctrl+Alt+V sequences and keyboard patterns.
 */

#pragma once

#include <string>
#include <memory>

namespace phantomvault {

class KeyboardSequenceDetector {
public:
    KeyboardSequenceDetector();
    ~KeyboardSequenceDetector();

    bool initialize();
    bool start();
    void stop();
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

} // namespace phantomvault