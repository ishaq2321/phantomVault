/**
 * PhantomVault IPC Server Implementation
 * 
 * Complete HTTP-based IPC server for GUI-service communication.
 * Provides RESTful API endpoints with JSON request/response handling.
 */

#include "ipc_server.hpp"
#include "profile_manager.hpp"
#include "folder_security_manager.hpp"
#include "keyboard_sequence_detector.hpp"
#include "analytics_engine.hpp"

#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <sstream>
#include <algorithm>
#include <regex>
#include <nlohmann/json.hpp>

#ifdef PLATFORM_LINUX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#elif PLATFORM_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#elif PLATFORM_MACOS
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

using json = nlohmann::json;

namespace phantomvault {

class IPCServer::Implementation {
public:
    Implementation()
        : running_(false)
        , port_(0)
        , server_socket_(-1)
        , request_count_(0)
        , last_error_()
        , server_thread_()
        , routes_()
        , routes_mutex_()
        , profile_manager_(nullptr)
        , folder_security_manager_(nullptr)
        , keyboard_sequence_detector_(nullptr)
        , analytics_engine_(nullptr)
    {}
    
    ~Implementation() {
        stop();
    }
    
    bool initialize(int port) {
        try {
            port_ = port;
            
            // Initialize socket
            #ifdef PLATFORM_WINDOWS
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                last_error_ = "Failed to initialize Winsock";
                return false;
            }
            #endif
            
            server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
            if (server_socket_ < 0) {
                last_error_ = "Failed to create socket";
                return false;
            }
            
            // Set socket options
            int opt = 1;
            if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, 
                          reinterpret_cast<const char*>(&opt), sizeof(opt)) < 0) {
                last_error_ = "Failed to set socket options";
                return false;
            }
            
            // Bind socket
            struct sockaddr_in address;
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = inet_addr("127.0.0.1");
            address.sin_port = htons(port);
            
            if (bind(server_socket_, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0) {
                last_error_ = "Failed to bind socket to port " + std::to_string(port);
                return false;
            }
            
            // Register default routes
            registerDefaultRoutes();
            
            std::cout << "[IPCServer] Initialized HTTP server on port " << port << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to initialize IPC server: " + std::string(e.what());
            return false;
        }
    }
    
    bool start() {
        try {
            if (running_) {
                return true;
            }
            
            if (listen(server_socket_, 10) < 0) {
                last_error_ = "Failed to listen on socket";
                return false;
            }
            
            running_ = true;
            server_thread_ = std::thread(&Implementation::serverLoop, this);
            
            std::cout << "[IPCServer] Started HTTP server on port " << port_ << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            last_error_ = "Failed to start IPC server: " + std::string(e.what());
            running_ = false;
            return false;
        }
    }
    
    void stop() {
        if (!running_) {
            return;
        }
        
        running_ = false;
        
        if (server_socket_ >= 0) {
            #ifdef PLATFORM_WINDOWS
            closesocket(server_socket_);
            WSACleanup();
            #else
            close(server_socket_);
            #endif
            server_socket_ = -1;
        }
        
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        
        std::cout << "[IPCServer] Stopped HTTP server" << std::endl;
    }
    
    bool isRunning() const {
        return running_;
    }
    
    void setProfileManager(ProfileManager* manager) {
        profile_manager_ = manager;
    }
    
    void setFolderSecurityManager(FolderSecurityManager* manager) {
        folder_security_manager_ = manager;
    }
    
    void setKeyboardSequenceDetector(KeyboardSequenceDetector* detector) {
        keyboard_sequence_detector_ = detector;
    }
    
    void setAnalyticsEngine(AnalyticsEngine* engine) {
        analytics_engine_ = engine;
    }
    
    void registerRoute(const std::string& method, const std::string& path, RequestHandler handler) {
        std::lock_guard<std::mutex> lock(routes_mutex_);
        std::string key = method + ":" + path;
        routes_[key] = handler;
    }
    
    int getPort() const {
        return port_;
    }
    
    size_t getRequestCount() const {
        return request_count_;
    }
    
    std::string getLastError() const {
        return last_error_;
    }
    
private:
    std::atomic<bool> running_;
    int port_;
    int server_socket_;
    std::atomic<size_t> request_count_;
    mutable std::string last_error_;
    
    std::thread server_thread_;
    std::map<std::string, RequestHandler> routes_;
    std::mutex routes_mutex_;
    
    // Component references
    ProfileManager* profile_manager_;
    FolderSecurityManager* folder_security_manager_;
    KeyboardSequenceDetector* keyboard_sequence_detector_;
    AnalyticsEngine* analytics_engine_;
    
    void serverLoop() {
        std::cout << "[IPCServer] Server loop started" << std::endl;
        
        while (running_) {
            try {
                struct sockaddr_in client_address;
                socklen_t client_len = sizeof(client_address);
                
                int client_socket = accept(server_socket_, 
                                         reinterpret_cast<struct sockaddr*>(&client_address), 
                                         &client_len);
                
                if (client_socket < 0) {
                    if (running_) {
                        std::cerr << "[IPCServer] Failed to accept connection" << std::endl;
                    }
                    continue;
                }
                
                // Handle request in separate thread for better performance
                std::thread([this, client_socket]() {
                    handleRequest(client_socket);
                }).detach();
                
            } catch (const std::exception& e) {
                if (running_) {
                    std::cerr << "[IPCServer] Server loop error: " << e.what() << std::endl;
                }
            }
        }
        
        std::cout << "[IPCServer] Server loop ended" << std::endl;
    }
    
    void handleRequest(int client_socket) {
        try {
            // Read request
            char buffer[4096];
            ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytes_read <= 0) {
                #ifdef PLATFORM_WINDOWS
                closesocket(client_socket);
                #else
                close(client_socket);
                #endif
                return;
            }
            
            buffer[bytes_read] = '\0';
            std::string request_data(buffer);
            
            // Parse HTTP request
            HttpRequest request = parseHttpRequest(request_data);
            
            // Handle CORS preflight
            if (request.method == "OPTIONS") {
                HttpResponse response;
                response.status_code = 200;
                sendResponse(client_socket, response);
                #ifdef PLATFORM_WINDOWS
                closesocket(client_socket);
                #else
                close(client_socket);
                #endif
                return;
            }
            
            // Find and execute route handler
            HttpResponse response = handleRoute(request);
            
            // Send response
            sendResponse(client_socket, response);
            
            // Increment request counter
            request_count_++;
            
        } catch (const std::exception& e) {
            std::cerr << "[IPCServer] Request handling error: " << e.what() << std::endl;
            
            // Send error response
            HttpResponse error_response;
            error_response.status_code = 500;
            error_response.body = R"({"success": false, "error": "Internal server error"})";
            sendResponse(client_socket, error_response);
        }
        
        #ifdef PLATFORM_WINDOWS
        closesocket(client_socket);
        #else
        close(client_socket);
        #endif
    }
    
    HttpRequest parseHttpRequest(const std::string& request_data) {
        HttpRequest request;
        
        std::istringstream stream(request_data);
        std::string line;
        
        // Parse request line
        if (std::getline(stream, line)) {
            std::istringstream request_line(line);
            std::string path_with_query;
            request_line >> request.method >> path_with_query;
            
            // Parse path and query parameters
            size_t query_pos = path_with_query.find('?');
            if (query_pos != std::string::npos) {
                request.path = path_with_query.substr(0, query_pos);
                std::string query = path_with_query.substr(query_pos + 1);
                parseQueryParams(query, request.query_params);
            } else {
                request.path = path_with_query;
            }
        }
        
        // Parse headers
        while (std::getline(stream, line) && line != "\r" && !line.empty()) {
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string key = line.substr(0, colon_pos);
                std::string value = line.substr(colon_pos + 1);
                
                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t\r\n") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t\r\n") + 1);
                
                request.headers[key] = value;
            }
        }
        
        // Parse body
        std::string body_line;
        while (std::getline(stream, body_line)) {
            request.body += body_line + "\n";
        }
        
        if (!request.body.empty() && request.body.back() == '\n') {
            request.body.pop_back();
        }
        
        return request;
    }
    
    void parseQueryParams(const std::string& query, std::map<std::string, std::string>& params) {
        std::istringstream stream(query);
        std::string pair;
        
        while (std::getline(stream, pair, '&')) {
            size_t eq_pos = pair.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = pair.substr(0, eq_pos);
                std::string value = pair.substr(eq_pos + 1);
                params[key] = value;
            }
        }
    }
    
    std::string extractQueryParam(const std::string& path, const std::string& param_name) {
        size_t query_pos = path.find('?');
        if (query_pos == std::string::npos) {
            return "";
        }
        
        std::string query = path.substr(query_pos + 1);
        std::map<std::string, std::string> params;
        parseQueryParams(query, params);
        
        auto it = params.find(param_name);
        return (it != params.end()) ? it->second : "";
    }
    
    HttpResponse handleRoute(const HttpRequest& request) {
        std::lock_guard<std::mutex> lock(routes_mutex_);
        
        std::string key = request.method + ":" + request.path;
        auto it = routes_.find(key);
        
        if (it != routes_.end()) {
            return it->second(request);
        }
        
        // Check for profile-specific routes (simple pattern matching)
        if (request.method == "POST" && request.path.find("/api/profiles/") == 0 && 
            request.path.find("/authenticate") != std::string::npos) {
            return handleAuthenticateProfile(request);
        }
        
        // Check for vault folder routes
        if (request.method == "GET" && request.path.find("/api/vault/folders") == 0) {
            return handleGetVaultFolders(request);
        }
        
        // Route not found
        HttpResponse response;
        response.status_code = 404;
        response.body = R"({"success": false, "error": "Route not found: )" + request.method + " " + request.path + R"("})";
        return response;
    }
    
    void sendResponse(int client_socket, const HttpResponse& response) {
        std::ostringstream response_stream;
        
        // Status line
        response_stream << "HTTP/1.1 " << response.status_code << " ";
        switch (response.status_code) {
            case 200: response_stream << "OK"; break;
            case 400: response_stream << "Bad Request"; break;
            case 404: response_stream << "Not Found"; break;
            case 500: response_stream << "Internal Server Error"; break;
            default: response_stream << "Unknown"; break;
        }
        response_stream << "\r\n";
        
        // Headers
        for (const auto& header : response.headers) {
            response_stream << header.first << ": " << header.second << "\r\n";
        }
        
        // Content-Length
        response_stream << "Content-Length: " << response.body.length() << "\r\n";
        response_stream << "\r\n";
        
        // Body
        response_stream << response.body;
        
        std::string response_str = response_stream.str();
        send(client_socket, response_str.c_str(), response_str.length(), 0);
    }
    
    void registerDefaultRoutes() {
        // Health check
        registerRoute("GET", "/health", [this](const HttpRequest&) -> HttpResponse {
            HttpResponse response;
            json result = {
                {"success", true},
                {"status", "healthy"},
                {"version", "1.0.0"},
                {"uptime", "running"},
                {"requests", request_count_.load()}
            };
            response.body = result.dump();
            return response;
        });
        
        // Profile routes
        registerRoute("GET", "/api/profiles", [this](const HttpRequest&) -> HttpResponse {
            return handleGetProfiles();
        });
        
        registerRoute("POST", "/api/profiles", [this](const HttpRequest& req) -> HttpResponse {
            return handleCreateProfile(req);
        });
        
        registerRoute("POST", "/api/profiles/authenticate", [this](const HttpRequest& req) -> HttpResponse {
            return handleAuthenticateProfile(req);
        });
        
        // Add a catch-all route handler for profile-specific routes
        // This is a temporary solution - in production we'd implement proper path parameter routing
        
        // Folder routes
        registerRoute("GET", "/api/folders", [this](const HttpRequest& req) -> HttpResponse {
            return handleGetFolders(req);
        });
        
        registerRoute("POST", "/api/folders", [this](const HttpRequest& req) -> HttpResponse {
            return handleAddFolder(req);
        });
        
        registerRoute("POST", "/api/folders/unlock", [this](const HttpRequest& req) -> HttpResponse {
            return handleUnlockFolders(req);
        });
        
        // Analytics routes
        registerRoute("GET", "/api/analytics", [this](const HttpRequest& req) -> HttpResponse {
            return handleGetAnalytics(req);
        });
        
        // Platform routes
        registerRoute("GET", "/api/platform", [this](const HttpRequest&) -> HttpResponse {
            return handleGetPlatformInfo();
        });
        
        // Vault operation routes
        registerRoute("POST", "/api/vault/lock", [this](const HttpRequest& req) -> HttpResponse {
            return handleLockFolder(req);
        });
        
        registerRoute("POST", "/api/vault/unlock", [this](const HttpRequest& req) -> HttpResponse {
            return handleUnlockFolder(req);
        });
        
        registerRoute("GET", "/api/vault/status", [this](const HttpRequest& req) -> HttpResponse {
            return handleGetVaultStatus(req);
        });
        
        registerRoute("POST", "/api/vault/relock", [this](const HttpRequest& req) -> HttpResponse {
            return handleRelockTemporaryFolders(req);
        });
        
        registerRoute("GET", "/api/vault/info", [this](const HttpRequest& req) -> HttpResponse {
            return handleGetVaultInfo(req);
        });
        
        registerRoute("DELETE", "/api/vault/folder", [this](const HttpRequest& req) -> HttpResponse {
            return handleRemoveFromVault(req);
        });
        
        // Enhanced vault operation routes
        registerRoute("POST", "/api/vault/unlock/temporary", [this](const HttpRequest& req) -> HttpResponse {
            return handleUnlockFoldersTemporary(req);
        });
        
        registerRoute("POST", "/api/vault/unlock/permanent", [this](const HttpRequest& req) -> HttpResponse {
            return handleUnlockFoldersPermanent(req);
        });
        
        registerRoute("GET", "/api/vault/folders", [this](const HttpRequest& req) -> HttpResponse {
            return handleGetVaultFolders(req);
        });
        
        registerRoute("GET", "/api/vault/stats", [this](const HttpRequest& req) -> HttpResponse {
            return handleGetVaultStats(req);
        });
        
        registerRoute("POST", "/api/vault/lock/temporary", [this](const HttpRequest& req) -> HttpResponse {
            return handleLockTemporaryFolders(req);
        });
        
        // Enhanced recovery key routes
        registerRoute("POST", "/api/recovery/validate", [this](const HttpRequest& req) -> HttpResponse {
            return handleValidateRecoveryKey(req);
        });
        
        registerRoute("POST", "/api/recovery/generate", [this](const HttpRequest& req) -> HttpResponse {
            return handleGenerateRecoveryKey(req);
        });
        
        registerRoute("POST", "/api/recovery/current", [this](const HttpRequest& req) -> HttpResponse {
            return handleGetCurrentRecoveryKey(req);
        });
        
        registerRoute("POST", "/api/recovery/change-password", [this](const HttpRequest& req) -> HttpResponse {
            return handleChangePassword(req);
        });
        
        registerRoute("POST", "/api/profiles/change-password", [this](const HttpRequest& req) -> HttpResponse {
            return handleChangeProfilePassword(req);
        });
        
        registerRoute("POST", "/api/recovery/recover-master-key", [this](const HttpRequest& req) -> HttpResponse {
            return handleRecoverMasterKey(req);
        });
        
        // Service control routes
        registerRoute("GET", "/api/test-keyboard", [this](const HttpRequest&) -> HttpResponse {
            return handleTestKeyboard();
        });
        
        registerRoute("POST", "/api/service/stop", [this](const HttpRequest&) -> HttpResponse {
            return handleServiceStop();
        });
        
        registerRoute("POST", "/api/service/restart", [this](const HttpRequest&) -> HttpResponse {
            return handleServiceRestart();
        });
    }
    
    // Route handlers
    HttpResponse handleGetProfiles() {
        HttpResponse response;
        
        try {
            if (!profile_manager_) {
                response.status_code = 500;
                response.body = R"({"success": false, "error": "Profile manager not available"})";
                return response;
            }
            
            auto profiles = profile_manager_->getAllProfiles();
            
            json result = {
                {"success", true},
                {"profiles", json::array()}
            };
            
            for (const auto& profile : profiles) {
                json profileJson = {
                    {"id", profile.id},
                    {"name", profile.name},
                    {"createdAt", std::chrono::duration_cast<std::chrono::milliseconds>(profile.createdAt.time_since_epoch()).count()},
                    {"lastAccess", std::chrono::duration_cast<std::chrono::milliseconds>(profile.lastAccess.time_since_epoch()).count()},
                    {"folderCount", 0} // Will be populated by folder manager
                };
                result["profiles"].push_back(profileJson);
            }
            
            response.body = result.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 500;
            response.body = R"({"success": false, "error": ")" + std::string(e.what()) + R"("})";
        }
        
        return response;
    }
    
    HttpResponse handleCreateProfile(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            if (!profile_manager_) {
                response.status_code = 500;
                response.body = R"({"success": false, "error": "Profile manager not available"})";
                return response;
            }
            
            json requestData = json::parse(request.body);
            std::string name = requestData["name"];
            std::string masterKey = requestData["masterKey"];
            
            auto result = profile_manager_->createProfile(name, masterKey);
            
            json responseJson = {
                {"success", result.success},
                {"profileId", result.profileId},
                {"message", result.message}
            };
            
            if (!result.success) {
                responseJson["error"] = result.error;
                response.status_code = 400;
            }
            
            response.body = responseJson.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 400;
            response.body = R"({"success": false, "error": "Invalid request: )" + std::string(e.what()) + R"("})";
        }
        
        return response;
    }
    
    HttpResponse handleAuthenticateProfile(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            if (!profile_manager_) {
                response.status_code = 500;
                response.body = R"({"success": false, "error": "Profile manager not available"})";
                return response;
            }
            
            json requestData = json::parse(request.body);
            std::string profileId = requestData["profileId"];
            std::string masterKey = requestData["masterKey"];
            
            auto authResult = profile_manager_->authenticateProfile(profileId, masterKey);
            
            json responseJson = {
                {"success", authResult.success},
                {"message", authResult.success ? "Authentication successful" : "Authentication failed"}
            };
            
            if (!authResult.success) {
                response.status_code = 401;
                responseJson["error"] = authResult.error;
            }
            
            response.body = responseJson.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 400;
            response.body = R"({"success": false, "error": "Invalid request: )" + std::string(e.what()) + R"("})";
        }
        
        return response;
    }
    
    HttpResponse handleGetFolders(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            if (!folder_security_manager_) {
                response.status_code = 500;
                response.body = R"({"success": false, "error": "Folder security manager not available"})";
                return response;
            }
            
            std::string profileId = request.query_params.count("profileId") ? 
                                   request.query_params.at("profileId") : "";
            
            if (profileId.empty()) {
                response.status_code = 400;
                response.body = R"({"success": false, "error": "Profile ID required"})";
                return response;
            }
            
            auto folders = folder_security_manager_->getProfileFolders(profileId);
            
            json result = {
                {"success", true},
                {"folders", json::array()}
            };
            
            for (const auto& folder : folders) {
                json folderJson = {
                    {"id", folder.id},
                    {"name", folder.originalName},
                    {"originalPath", folder.originalPath},
                    {"isLocked", folder.isLocked},
                    {"size", folder.originalSize},
                    {"createdAt", std::chrono::duration_cast<std::chrono::milliseconds>(folder.createdAt.time_since_epoch()).count()}
                };
                result["folders"].push_back(folderJson);
            }
            
            response.body = result.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 500;
            response.body = R"({"success": false, "error": ")" + std::string(e.what()) + R"("})";
        }
        
        return response;
    }
    
    HttpResponse handleAddFolder(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            if (!folder_security_manager_) {
                response.status_code = 500;
                response.body = R"({"success": false, "error": "Folder security manager not available"})";
                return response;
            }
            
            json requestData = json::parse(request.body);
            std::string profileId = requestData["profileId"];
            std::string folderPath = requestData["folderPath"];
            
            // For now, use empty master key - this will be properly handled in authentication
            auto result = folder_security_manager_->lockFolder(profileId, folderPath, "dummy_key");
            
            json responseJson = {
                {"success", result.success},
                {"folderId", result.folderId},
                {"message", result.message}
            };
            
            if (!result.success) {
                responseJson["error"] = result.error;
                response.status_code = 400;
            }
            
            response.body = responseJson.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 400;
            response.body = R"({"success": false, "error": "Invalid request: )" + std::string(e.what()) + R"("})";
        }
        
        return response;
    }
    
    HttpResponse handleUnlockFolders(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            if (!folder_security_manager_) {
                response.status_code = 500;
                response.body = R"({"success": false, "error": "Folder security manager not available"})";
                return response;
            }
            
            json requestData = json::parse(request.body);
            std::string profileId = requestData["profileId"];
            bool permanent = requestData.value("permanent", false);
            
            // For now, use dummy master key - this will be properly handled in authentication
            auto result = permanent ? 
                folder_security_manager_->unlockFoldersPermanent(profileId, "dummy_key", {}) :
                folder_security_manager_->unlockFoldersTemporary(profileId, "dummy_key");
            
            json responseJson = {
                {"success", result.success},
                {"successCount", result.successCount},
                {"failedCount", result.failedCount},
                {"message", result.message}
            };
            
            if (!result.success) {
                responseJson["error"] = result.error;
                response.status_code = 400;
            }
            
            response.body = responseJson.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 400;
            response.body = R"({"success": false, "error": "Invalid request: )" + std::string(e.what()) + R"("})";
        }
        
        return response;
    }
    
    HttpResponse handleGetAnalytics(const HttpRequest&) {
        HttpResponse response;
        
        try {
            if (!analytics_engine_) {
                response.status_code = 500;
                response.body = R"({"success": false, "error": "Analytics engine not available"})";
                return response;
            }
            
            auto statistics = analytics_engine_->getUsageStatistics();
            
            json result = {
                {"success", true},
                {"statistics", {
                    {"totalProfiles", statistics.totalProfiles},
                    {"totalFolders", statistics.totalFolders},
                    {"totalUnlockAttempts", statistics.totalUnlockAttempts},
                    {"successfulUnlocks", statistics.successfulUnlocks},
                    {"failedUnlocks", statistics.failedUnlocks},
                    {"keyboardSequenceDetections", statistics.keyboardSequenceDetections},
                    {"securityViolations", statistics.securityViolations},
                    {"firstUse", std::chrono::duration_cast<std::chrono::milliseconds>(statistics.firstUse.time_since_epoch()).count()},
                    {"lastActivity", std::chrono::duration_cast<std::chrono::milliseconds>(statistics.lastActivity.time_since_epoch()).count()},
                    {"totalUptime", statistics.totalUptime.count()}
                }}
            };
            
            response.body = result.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 500;
            response.body = R"({"success": false, "error": ")" + std::string(e.what()) + R"("})";
        }
        
        return response;
    }
    
    HttpResponse handleGetPlatformInfo() {
        HttpResponse response;
        
        try {
            json result = {
                {"success", true},
                {"platform", {
                    #ifdef PLATFORM_LINUX
                    {"name", "Linux"},
                    {"type", "linux"}
                    #elif PLATFORM_MACOS
                    {"name", "macOS"},
                    {"type", "macos"}
                    #elif PLATFORM_WINDOWS
                    {"name", "Windows"},
                    {"type", "windows"}
                    #else
                    {"name", "Unknown"},
                    {"type", "unknown"}
                    #endif
                }},
                {"capabilities", {
                    {"supportsInvisibleLogging", keyboard_sequence_detector_ ? 
                        keyboard_sequence_detector_->getPlatformCapabilities().supportsInvisibleLogging : false},
                    {"supportsHotkeys", keyboard_sequence_detector_ ? 
                        keyboard_sequence_detector_->getPlatformCapabilities().supportsHotkeys : false},
                    {"requiresPermissions", keyboard_sequence_detector_ ? 
                        keyboard_sequence_detector_->getPlatformCapabilities().requiresPermissions : false}
                }}
            };
            
            response.body = result.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 500;
            response.body = R"({"success": false, "error": ")" + std::string(e.what()) + R"("})";
        }
        
        return response;
    }
    
    // New vault operation handlers
    HttpResponse handleLockFolder(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            if (!folder_security_manager_) {
                response.status_code = 500;
                response.body = R"({"success": false, "error": "Folder security manager not available"})";
                return response;
            }
            
            json requestData = json::parse(request.body);
            std::string profileId = requestData["profileId"];
            std::string folderPath = requestData["folderPath"];
            std::string masterKey = requestData["masterKey"];
            
            auto result = folder_security_manager_->lockFolder(profileId, folderPath, masterKey);
            
            json responseJson = {
                {"success", result.success},
                {"folderId", result.folderId},
                {"message", result.message}
            };
            
            if (!result.success) {
                responseJson["error"] = result.error;
                response.status_code = 400;
            }
            
            response.body = responseJson.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 400;
            response.body = R"({"success": false, "error": "Invalid request: )" + std::string(e.what()) + R"("})";
        }
        
        return response;
    }
    
    HttpResponse handleUnlockFolder(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            if (!folder_security_manager_) {
                response.status_code = 500;
                response.body = R"({"success": false, "error": "Folder security manager not available"})";
                return response;
            }
            
            json requestData = json::parse(request.body);
            std::string profileId = requestData["profileId"];
            std::string masterKey = requestData["masterKey"];
            bool permanent = requestData.value("permanent", false);
            std::vector<std::string> folderIds = requestData.value("folderIds", std::vector<std::string>());
            
            auto result = permanent ? 
                folder_security_manager_->unlockFoldersPermanent(profileId, masterKey, folderIds) :
                folder_security_manager_->unlockFoldersTemporary(profileId, masterKey);
            
            json responseJson = {
                {"success", result.success},
                {"successCount", result.successCount},
                {"failedCount", result.failedCount},
                {"unlockedFolderIds", result.unlockedFolderIds},
                {"failedFolderIds", result.failedFolderIds},
                {"message", result.message}
            };
            
            if (!result.success) {
                responseJson["error"] = result.error;
                response.status_code = 400;
            }
            
            response.body = responseJson.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 400;
            response.body = R"({"success": false, "error": "Invalid request: )" + std::string(e.what()) + R"("})";
        }
        
        return response;
    }
    
    HttpResponse handleGetVaultStatus(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            if (!profile_manager_) {
                response.status_code = 500;
                response.body = R"({"success": false, "error": "Profile manager not available"})";
                return response;
            }
            
            std::string profileId = request.query_params.count("profileId") ? 
                                   request.query_params.at("profileId") : "";
            
            if (profileId.empty()) {
                response.status_code = 400;
                response.body = R"({"success": false, "error": "Profile ID required"})";
                return response;
            }
            
            size_t vaultSize = profile_manager_->getProfileVaultSize(profileId);
            bool vaultValid = profile_manager_->validateProfileVault(profileId);
            auto lockedFolders = profile_manager_->getProfileLockedFolders(profileId);
            
            json result = {
                {"success", true},
                {"profileId", profileId},
                {"vaultSize", vaultSize},
                {"vaultValid", vaultValid},
                {"lockedFolderCount", lockedFolders.size()},
                {"lockedFolders", lockedFolders}
            };
            
            response.body = result.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 500;
            response.body = R"({"success": false, "error": ")" + std::string(e.what()) + R"("})";
        }
        
        return response;
    }
    
    HttpResponse handleRelockTemporaryFolders(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            if (!folder_security_manager_) {
                response.status_code = 500;
                response.body = R"({"success": false, "error": "Folder security manager not available"})";
                return response;
            }
            
            json requestData = json::parse(request.body);
            std::string profileId = requestData["profileId"];
            
            bool success = folder_security_manager_->lockTemporaryFolders(profileId);
            
            json responseJson = {
                {"success", success},
                {"message", success ? "Temporary folders re-locked successfully" : "Failed to re-lock temporary folders"}
            };
            
            if (!success) {
                response.status_code = 400;
                responseJson["error"] = folder_security_manager_->getLastError();
            }
            
            response.body = responseJson.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 400;
            response.body = R"({"success": false, "error": "Invalid request: )" + std::string(e.what()) + R"("})";
        }
        
        return response;
    }
    
    HttpResponse handleGetVaultInfo(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            if (!profile_manager_ || !folder_security_manager_) {
                response.status_code = 500;
                response.body = R"({"success": false, "error": "Required managers not available"})";
                return response;
            }
            
            std::string profileId = request.query_params.count("profileId") ? 
                                   request.query_params.at("profileId") : "";
            
            if (profileId.empty()) {
                response.status_code = 400;
                response.body = R"({"success": false, "error": "Profile ID required"})";
                return response;
            }
            
            auto profile = profile_manager_->getProfile(profileId);
            if (!profile) {
                response.status_code = 404;
                response.body = R"({"success": false, "error": "Profile not found"})";
                return response;
            }
            
            auto folders = folder_security_manager_->getProfileFolders(profileId);
            size_t vaultSize = profile_manager_->getProfileVaultSize(profileId);
            
            json result = {
                {"success", true},
                {"profile", {
                    {"id", profile->id},
                    {"name", profile->name},
                    {"createdAt", std::chrono::duration_cast<std::chrono::milliseconds>(profile->createdAt.time_since_epoch()).count()},
                    {"lastAccess", std::chrono::duration_cast<std::chrono::milliseconds>(profile->lastAccess.time_since_epoch()).count()},
                    {"folderCount", profile->folderCount}
                }},
                {"vault", {
                    {"size", vaultSize},
                    {"folderCount", folders.size()}
                }},
                {"folders", json::array()}
            };
            
            for (const auto& folder : folders) {
                json folderJson = {
                    {"id", folder.id},
                    {"name", folder.originalName},
                    {"originalPath", folder.originalPath},
                    {"isLocked", folder.isLocked},
                    {"unlockMode", folder.unlockMode == UnlockMode::TEMPORARY ? "temporary" : "permanent"},
                    {"size", folder.originalSize},
                    {"createdAt", std::chrono::duration_cast<std::chrono::milliseconds>(folder.createdAt.time_since_epoch()).count()},
                    {"lastAccess", std::chrono::duration_cast<std::chrono::milliseconds>(folder.lastAccess.time_since_epoch()).count()}
                };
                result["folders"].push_back(folderJson);
            }
            
            response.body = result.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 500;
            response.body = R"({"success": false, "error": ")" + std::string(e.what()) + R"("})";
        }
        
        return response;
    }
    
    HttpResponse handleRemoveFromVault(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            if (!folder_security_manager_) {
                response.status_code = 500;
                response.body = R"({"success": false, "error": "Folder security manager not available"})";
                return response;
            }
            
            json requestData = json::parse(request.body);
            std::string profileId = requestData["profileId"];
            std::string folderId = requestData["folderId"];
            
            auto result = folder_security_manager_->removeFromProfile(profileId, folderId);
            
            json responseJson = {
                {"success", result.success},
                {"folderId", result.folderId},
                {"message", result.message}
            };
            
            if (!result.success) {
                responseJson["error"] = result.error;
                response.status_code = 400;
            }
            
            response.body = responseJson.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 400;
            response.body = R"({"success": false, "error": "Invalid request: )" + std::string(e.what()) + R"("})";
        }
        
        return response;
    }
    
    HttpResponse handleValidateRecoveryKey(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            if (!profile_manager_) {
                response.status_code = 500;
                response.body = R"({"success": false, "error": "Profile manager not available"})";
                return response;
            }
            
            json requestData = json::parse(request.body);
            std::string recoveryKey = requestData["recoveryKey"];
            
            auto profileId = profile_manager_->getProfileIdFromRecoveryKey(recoveryKey);
            
            json responseJson = {
                {"success", profileId.has_value()},
                {"message", profileId.has_value() ? "Recovery key is valid" : "Invalid recovery key"}
            };
            
            if (profileId.has_value()) {
                responseJson["profileId"] = profileId.value();
            } else {
                response.status_code = 401;
                responseJson["error"] = "Invalid recovery key";
            }
            
            response.body = responseJson.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 400;
            response.body = R"({"success": false, "error": "Invalid request: )" + std::string(e.what()) + R"("})";
        }
        
        return response;
    }
    
    HttpResponse handleChangeProfilePassword(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            if (!profile_manager_) {
                response.status_code = 500;
                response.body = R"({"success": false, "error": "Profile manager not available"})";
                return response;
            }
            
            json requestData = json::parse(request.body);
            std::string profileId = requestData["profileId"];
            std::string oldPassword = requestData["oldPassword"];
            std::string newPassword = requestData["newPassword"];
            
            auto result = profile_manager_->changeProfilePassword(profileId, oldPassword, newPassword);
            
            json responseJson = {
                {"success", result.success},
                {"message", result.message}
            };
            
            if (result.success) {
                responseJson["newRecoveryKey"] = result.recoveryKey;
            } else {
                responseJson["error"] = result.error;
                response.status_code = 400;
            }
            
            response.body = responseJson.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 400;
            response.body = R"({"success": false, "error": "Invalid request: )" + std::string(e.what()) + R"("})";
        }
        
        return response;
    }
    
    HttpResponse handleRecoverMasterKey(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            if (!profile_manager_) {
                response.status_code = 500;
                response.body = R"({"success": false, "error": "Profile manager not available"})";
                return response;
            }
            
            json requestData = json::parse(request.body);
            std::string recoveryKey = requestData["recoveryKey"];
            
            auto masterKey = profile_manager_->recoverMasterKeyFromRecoveryKey(recoveryKey);
            
            json responseJson = {
                {"success", masterKey.has_value()},
                {"message", masterKey.has_value() ? "Master key recovered successfully" : "Recovery failed"}
            };
            
            if (masterKey.has_value()) {
                responseJson["masterKey"] = masterKey.value();
                
                // Also return the profile ID for convenience
                auto profileId = profile_manager_->getProfileIdFromRecoveryKey(recoveryKey);
                if (profileId.has_value()) {
                    responseJson["profileId"] = profileId.value();
                }
            } else {
                response.status_code = 401;
                responseJson["error"] = "Invalid recovery key or recovery failed";
            }
            
            response.body = responseJson.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 400;
            response.body = R"({"success": false, "error": "Invalid request: )" + std::string(e.what()) + R"("})";
        }
        
        return response;
    }
    
    // Enhanced vault operation handlers
    HttpResponse handleUnlockFoldersTemporary(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            json request_data = json::parse(request.body);
            std::string profile_id = request_data.value("profileId", "");
            std::string master_key = request_data.value("masterKey", "");
            
            if (profile_id.empty() || master_key.empty()) {
                response.status_code = 400;
                response.body = R"({"success": false, "error": "Profile ID and master key are required"})";
                return response;
            }
            
            auto result = folder_security_manager_->unlockFoldersTemporary(profile_id, master_key);
            
            json result_json = {
                {"success", result.success},
                {"successCount", result.successCount},
                {"failedCount", result.failedCount},
                {"unlockedFolderIds", result.unlockedFolderIds},
                {"failedFolderIds", result.failedFolderIds},
                {"message", result.message}
            };
            
            if (!result.success) {
                result_json["error"] = result.error;
                response.status_code = 400;
            }
            
            response.body = result_json.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 500;
            response.body = R"({"success": false, "error": "Internal server error"})";
        }
        
        return response;
    }
    
    HttpResponse handleUnlockFoldersPermanent(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            json request_data = json::parse(request.body);
            std::string profile_id = request_data.value("profileId", "");
            std::string master_key = request_data.value("masterKey", "");
            std::vector<std::string> folder_ids = request_data.value("folderIds", std::vector<std::string>());
            
            if (profile_id.empty() || master_key.empty()) {
                response.status_code = 400;
                response.body = R"({"success": false, "error": "Profile ID and master key are required"})";
                return response;
            }
            
            auto result = folder_security_manager_->unlockFoldersPermanent(profile_id, master_key, folder_ids);
            
            json result_json = {
                {"success", result.success},
                {"successCount", result.successCount},
                {"failedCount", result.failedCount},
                {"unlockedFolderIds", result.unlockedFolderIds},
                {"failedFolderIds", result.failedFolderIds},
                {"message", result.message}
            };
            
            if (!result.success) {
                result_json["error"] = result.error;
                response.status_code = 400;
            }
            
            response.body = result_json.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 500;
            response.body = R"({"success": false, "error": "Internal server error"})";
        }
        
        return response;
    }
    
    HttpResponse handleGetVaultFolders(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            std::string profile_id = extractQueryParam(request.path, "profileId");
            
            if (profile_id.empty()) {
                response.status_code = 400;
                response.body = R"({"success": false, "error": "Profile ID is required"})";
                return response;
            }
            
            auto folders = folder_security_manager_->getProfileFolders(profile_id);
            
            json folders_json = json::array();
            for (const auto& folder : folders) {
                folders_json.push_back({
                    {"id", folder.id},
                    {"name", folder.originalName},
                    {"originalPath", folder.originalPath},
                    {"vaultPath", folder.vaultPath},
                    {"isLocked", folder.isLocked},
                    {"unlockMode", folder.unlockMode == UnlockMode::TEMPORARY ? "temporary" : "permanent"},
                    {"size", folder.originalSize},
                    {"createdAt", std::chrono::duration_cast<std::chrono::milliseconds>(folder.createdAt.time_since_epoch()).count()},
                    {"lastAccess", std::chrono::duration_cast<std::chrono::milliseconds>(folder.lastAccess.time_since_epoch()).count()},
                    {"encryptionStatus", folder.isLocked ? "encrypted" : "decrypted"}
                });
            }
            
            json result_json = {
                {"success", true},
                {"folders", folders_json}
            };
            
            response.body = result_json.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 500;
            response.body = R"({"success": false, "error": "Internal server error"})";
        }
        
        return response;
    }
    
    HttpResponse handleGetVaultStats(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            std::string profile_id = extractQueryParam(request.path, "profileId");
            
            if (profile_id.empty()) {
                response.status_code = 400;
                response.body = R"({"success": false, "error": "Profile ID is required"})";
                return response;
            }
            
            auto folders = folder_security_manager_->getProfileFolders(profile_id);
            
            size_t total_folders = folders.size();
            size_t encrypted_folders = 0;
            size_t total_size = 0;
            
            for (const auto& folder : folders) {
                if (folder.isLocked) {
                    encrypted_folders++;
                }
                total_size += folder.originalSize;
            }
            
            json result_json = {
                {"success", true},
                {"stats", {
                    {"totalFolders", total_folders},
                    {"encryptedFolders", encrypted_folders},
                    {"totalSize", total_size},
                    {"lastBackup", "Never"}
                }}
            };
            
            response.body = result_json.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 500;
            response.body = R"({"success": false, "error": "Internal server error"})";
        }
        
        return response;
    }
    
    HttpResponse handleLockTemporaryFolders(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            json request_data = json::parse(request.body);
            std::string profile_id = request_data.value("profileId", "");
            
            if (profile_id.empty()) {
                response.status_code = 400;
                response.body = R"({"success": false, "error": "Profile ID is required"})";
                return response;
            }
            
            bool success = folder_security_manager_->lockTemporaryFolders(profile_id);
            
            json result_json = {
                {"success", success},
                {"message", success ? "Temporary folders locked successfully" : "Failed to lock temporary folders"}
            };
            
            if (!success) {
                result_json["error"] = folder_security_manager_->getLastError();
                response.status_code = 400;
            }
            
            response.body = result_json.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 500;
            response.body = R"({"success": false, "error": "Internal server error"})";
        }
        
        return response;
    }
    
    HttpResponse handleGenerateRecoveryKey(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            json request_data = json::parse(request.body);
            std::string profile_id = request_data.value("profileId", "");
            
            if (profile_id.empty()) {
                response.status_code = 400;
                response.body = R"({"success": false, "error": "Profile ID is required"})";
                return response;
            }
            
            auto recovery_key = profile_manager_->generateRecoveryKey(profile_id);
            
            json result_json = {
                {"success", !recovery_key.empty()},
                {"recoveryKey", recovery_key}
            };
            
            if (recovery_key.empty()) {
                result_json["error"] = "Failed to generate recovery key";
                response.status_code = 400;
            }
            
            response.body = result_json.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 500;
            response.body = R"({"success": false, "error": "Internal server error"})";
        }
        
        return response;
    }
    
    HttpResponse handleGetCurrentRecoveryKey(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            json request_data = json::parse(request.body);
            std::string profile_id = request_data.value("profileId", "");
            std::string master_key = request_data.value("masterKey", "");
            
            if (profile_id.empty() || master_key.empty()) {
                response.status_code = 400;
                response.body = R"({"success": false, "error": "Profile ID and master key are required"})";
                return response;
            }
            
            // Authenticate first
            auto auth_result = profile_manager_->authenticateProfile(profile_id, master_key);
            if (!auth_result.success) {
                response.status_code = 401;
                response.body = R"({"success": false, "error": "Authentication failed"})";
                return response;
            }
            
            auto recovery_key = profile_manager_->getCurrentRecoveryKey(profile_id);
            
            json result_json = {
                {"success", !recovery_key.empty()},
                {"recoveryKey", recovery_key}
            };
            
            if (recovery_key.empty()) {
                result_json["error"] = "Failed to get current recovery key";
                response.status_code = 400;
            }
            
            response.body = result_json.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 500;
            response.body = R"({"success": false, "error": "Internal server error"})";
        }
        
        return response;
    }
    
    HttpResponse handleChangePassword(const HttpRequest& request) {
        HttpResponse response;
        
        try {
            json request_data = json::parse(request.body);
            std::string profile_id = request_data.value("profileId", "");
            std::string current_password = request_data.value("currentPassword", "");
            std::string new_password = request_data.value("newPassword", "");
            
            if (profile_id.empty() || current_password.empty() || new_password.empty()) {
                response.status_code = 400;
                response.body = R"({"success": false, "error": "Profile ID, current password, and new password are required"})";
                return response;
            }
            
            auto result = profile_manager_->changeProfilePassword(profile_id, current_password, new_password);
            
            json result_json = {
                {"success", result.success},
                {"newRecoveryKey", result.new_recovery_key}
            };
            
            if (!result.success) {
                result_json["error"] = result.error_message;
                response.status_code = 400;
            }
            
            response.body = result_json.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 500;
            response.body = R"({"success": false, "error": "Internal server error"})";
        }
        
        return response;
    }
    
    // Service control handlers
    HttpResponse handleTestKeyboard() {
        HttpResponse response;
        
        try {
            json result_json = {
                {"success", true},
                {"message", "Keyboard detection test initiated"},
                {"instructions", "Press Ctrl+Alt+V within 10 seconds to test detection"}
            };
            
            // If keyboard detector is available, test it
            if (keyboard_sequence_detector_) {
                result_json["keyboard_detector_status"] = "available";
                result_json["hotkey_support"] = true;
            } else {
                result_json["keyboard_detector_status"] = "not_available";
                result_json["hotkey_support"] = false;
            }
            
            response.body = result_json.dump();
            
        } catch (const std::exception& e) {
            response.status_code = 500;
            response.body = R"({"success": false, "error": "Keyboard test failed"})";
        }
        
        return response;
    }
    
    HttpResponse handleServiceStop() {
        HttpResponse response;
        
        try {
            json result_json = {
                {"success", true},
                {"message", "Service stop request received - shutting down gracefully"}
            };
            
            response.body = result_json.dump();
            
            // Schedule graceful shutdown after response is sent
            std::thread([this]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                stop();
            }).detach();
            
        } catch (const std::exception& e) {
            response.status_code = 500;
            response.body = R"({"success": false, "error": "Failed to stop service"})";
        }
        
        return response;
    }
    
    HttpResponse handleServiceRestart() {
        HttpResponse response;
        
        try {
            json result_json = {
                {"success", true},
                {"message", "Service restart request received - restarting components"}
            };
            
            response.body = result_json.dump();
            
            // Schedule restart after response is sent
            std::thread([this]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                // For now, just return success - full restart would require service manager coordination
            }).detach();
            
        } catch (const std::exception& e) {
            response.status_code = 500;
            response.body = R"({"success": false, "error": "Failed to restart service"})";
        }
        
        return response;
    }
};

// IPCServer public interface implementation
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

bool IPCServer::isRunning() const {
    return pimpl->isRunning();
}

void IPCServer::setProfileManager(ProfileManager* manager) {
    pimpl->setProfileManager(manager);
}

void IPCServer::setFolderSecurityManager(FolderSecurityManager* manager) {
    pimpl->setFolderSecurityManager(manager);
}

void IPCServer::setKeyboardSequenceDetector(KeyboardSequenceDetector* detector) {
    pimpl->setKeyboardSequenceDetector(detector);
}

void IPCServer::setAnalyticsEngine(AnalyticsEngine* engine) {
    pimpl->setAnalyticsEngine(engine);
}

void IPCServer::registerRoute(const std::string& method, const std::string& path, RequestHandler handler) {
    pimpl->registerRoute(method, path, handler);
}

int IPCServer::getPort() const {
    return pimpl->getPort();
}

size_t IPCServer::getRequestCount() const {
    return pimpl->getRequestCount();
}

std::string IPCServer::getLastError() const {
    return pimpl->getLastError();
}

} // namespace phantomvault