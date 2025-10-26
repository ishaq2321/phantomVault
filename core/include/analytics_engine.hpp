/**
 * PhantomVault Analytics Engine
 * 
 * Tracks usage statistics and security events.
 */

#pragma once

#include <string>
#include <memory>

namespace phantomvault {

class AnalyticsEngine {
public:
    AnalyticsEngine();
    ~AnalyticsEngine();

    bool initialize();
    bool start();
    void stop();
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

} // namespace phantomvault