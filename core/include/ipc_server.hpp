/**
 * PhantomVault IPC Server
 * 
 * HTTP-based IPC server for communication between GUI and service.
 * Provides RESTful API endpoints for all service operations.
 */

#pragma once

#include <string>
#include <memory>
#include <functional>
#include <map>

namespace phantomvault {

// Forward declarations
class ProfileManager;
class FolderSecurityManager;
class KeyboardSequenceDetector;
class AnalyticsEngine;

/**
 * HTTP request structure
 */
struct HttpRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
    std::map<std::string, std::string> query_params;
};

/**
 * HTTP response structure
 */
struct HttpResponse {
    int status_code = 200;
    std::map<std::string, std::string> headers;
    std::string body;
    
    HttpResponse() {
        headers["Content-Type"] = "application/json";
        headers["Access-Control-Allow-Origin"] = "*";
        headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
        headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
    }
};

/**
 * Request handler function type
 */
using RequestHandler = std::function<HttpResponse(const HttpRequest&)>;

class IPCServer {
public:
    IPCServer();
    ~IPCServer();

    // Initialization and lifecycle
    bool initialize(int port);
    bool start();
    void stop();
    bool isRunning() const;
    
    // Component registration
    void setProfileManager(ProfileManager* manager);
    void setFolderSecurityManager(FolderSecurityManager* manager);
    void setKeyboardSequenceDetector(KeyboardSequenceDetector* detector);
    void setAnalyticsEngine(AnalyticsEngine* engine);
    
    // Route registration
    void registerRoute(const std::string& method, const std::string& path, RequestHandler handler);
    
    // Utility methods
    int getPort() const;
    size_t getRequestCount() const;
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

} // namespace phantomvault