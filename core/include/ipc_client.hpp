/**
 * PhantomVault IPC Client
 * 
 * Client for communicating with PhantomVault service via HTTP/JSON IPC
 */

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>

namespace phantomvault {

struct IPCResponse {
    bool success;
    std::string message;
    std::map<std::string, std::string> data;
    std::string raw_json;  // Raw JSON response for complex parsing
};

class IPCClient {
public:
    IPCClient(const std::string& host = "127.0.0.1", int port = 9876);
    ~IPCClient();

    // Connection management
    bool connect();
    void disconnect();
    bool isConnected() const;

    // Service operations
    IPCResponse getStatus();
    IPCResponse listProfiles();
    IPCResponse createProfile(const std::string& name, const std::string& password);
    IPCResponse lockProfile(const std::string& profile_name);
    IPCResponse unlockProfile(const std::string& profile_name);
    IPCResponse testKeyboard();
    IPCResponse stopService();
    IPCResponse restartService();

    // Utility
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> impl_;
};

} // namespace phantomvault