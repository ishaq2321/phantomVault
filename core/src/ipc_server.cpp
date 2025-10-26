/**
 * PhantomVault IPC Server Implementation
 * 
 * Stub implementation for IPC communication.
 * TODO: Full implementation in task 7.
 */

#include "ipc_server.hpp"
#include <iostream>

namespace phantomvault {

class IPCServer::Implementation {
public:
    Implementation() : last_error_(), port_(0) {}
    
    bool initialize(int port) {
        port_ = port;
        std::cout << "[IPCServer] Initialized on port " << port << " (stub)" << std::endl;
        return true;
    }
    
    bool start() {
        std::cout << "[IPCServer] Started (stub)" << std::endl;
        return true;
    }
    
    void stop() {
        std::cout << "[IPCServer] Stopped (stub)" << std::endl;
    }
    
    std::string getLastError() const {
        return last_error_;
    }
    
private:
    std::string last_error_;
    int port_;
};

IPCServer::IPCServer() : pimpl(std::make_unique<Implementation>()) {}
IPCServer::~IPCServer() = default;

bool IPCServer::initialize(int port) {
    return pimpl->initialize(port);
}

bool IPCServer::start() {
    return pimpl->start();
}

void IPCServer::stop() {
    pimpl->stop();
}

std::string IPCServer::getLastError() const {
    return pimpl->getLastError();
}

} // namespace phantomvault