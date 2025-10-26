/**
 * PhantomVault IPC Server
 * 
 * Handles communication between GUI and service.
 */

#pragma once

#include <string>
#include <memory>

namespace phantomvault {

class IPCServer {
public:
    IPCServer();
    ~IPCServer();

    bool initialize(int port);
    bool start();
    void stop();
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

} // namespace phantomvault