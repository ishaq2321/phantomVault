/**
 * Simple PhantomVault Service for Testing
 * Provides basic API endpoints for GUI testing
 */

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <thread>
#include <chrono>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace fs = std::filesystem;

struct Profile {
    std::string id;
    std::string name;
    std::string masterKey;
    bool authenticated = false;
    std::vector<std::string> folders;
};

class SimplePhantomVaultService {
private:
    int server_socket_;
    bool running_;
    std::map<std::string, Profile> profiles_;
    std::string authenticated_profile_;

public:
    SimplePhantomVaultService() : server_socket_(-1), running_(false) {}

    bool start(int port = 9876) {
        server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket_ < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }

        int opt = 1;
        setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr("127.0.0.1");
        address.sin_port = htons(port);

        if (bind(server_socket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "Failed to bind to port " << port << std::endl;
            return false;
        }

        if (listen(server_socket_, 10) < 0) {
            std::cerr << "Failed to listen" << std::endl;
            return false;
        }

        running_ = true;
        std::cout << "[PhantomVault] Service started on port " << port << std::endl;
        std::cout << "[PhantomVault] Features active:" << std::endl;
        std::cout << "  • Profile-based folder security" << std::endl;
        std::cout << "  • Real AES-256 encryption simulation" << std::endl;
        std::cout << "  • Admin mode support" << std::endl;

        return true;
    }

    void run() {
        while (running_) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
            
            if (client_socket >= 0) {
                std::thread([this, client_socket]() {
                    handleRequest(client_socket);
                }).detach();
            }
        }
    }

private:
    void handleRequest(int client_socket) {
        char buffer[4096];
        int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string request(buffer);
            
            std::string response = processRequest(request);
            send(client_socket, response.c_str(), response.length(), 0);
        }
        
        close(client_socket);
    }

    std::string processRequest(const std::string& request) {
        std::istringstream stream(request);
        std::string method, path, version;
        stream >> method >> path >> version;

        std::cout << "[API] " << method << " " << path << std::endl;

        // Extract body for POST requests
        std::string body;
        size_t body_start = request.find("\r\n\r\n");
        if (body_start != std::string::npos) {
            body = request.substr(body_start + 4);
        }

        std::string json_response;

        if (method == "GET" && path == "/api/profiles") {
            json_response = handleGetProfiles();
        } else if (method == "POST" && path == "/api/profiles") {
            json_response = handleCreateProfile(body);
        } else if (method == "POST" && path.find("/api/profiles/") == 0 && path.find("/authenticate") != std::string::npos) {
            std::string profile_id = extractProfileId(path);
            json_response = handleAuthenticateProfile(profile_id, body);
        } else if (method == "GET" && path == "/api/platform") {
            json_response = handleGetPlatform();
        } else if (method == "POST" && path == "/api/vault/lock") {
            json_response = handleLockFolder(body);
        } else if (method == "GET" && path.find("/api/vault/folders") == 0) {
            std::string profile_id = extractQueryParam(path, "profileId");
            json_response = handleGetVaultFolders(profile_id);
        } else if (method == "GET" && path.find("/api/vault/stats") == 0) {
            std::string profile_id = extractQueryParam(path, "profileId");
            json_response = handleGetVaultStats(profile_id);
        } else if (method == "POST" && path == "/api/vault/unlock/temporary") {
            json_response = handleUnlockTemporary(body);
        } else if (method == "POST" && path == "/api/vault/unlock/permanent") {
            json_response = handleUnlockPermanent(body);
        } else {
            json_response = R"({"success": false, "error": "Endpoint not found"})";
        }

        return createHttpResponse(json_response);
    }

    std::string extractProfileId(const std::string& path) {
        size_t start = path.find("/api/profiles/") + 14;
        size_t end = path.find("/", start);
        if (end == std::string::npos) end = path.length();
        return path.substr(start, end - start);
    }

    std::string extractQueryParam(const std::string& path, const std::string& param) {
        size_t query_start = path.find('?');
        if (query_start == std::string::npos) return "";
        
        std::string query = path.substr(query_start + 1);
        size_t param_start = query.find(param + "=");
        if (param_start == std::string::npos) return "";
        
        param_start += param.length() + 1;
        size_t param_end = query.find('&', param_start);
        if (param_end == std::string::npos) param_end = query.length();
        
        return query.substr(param_start, param_end - param_start);
    }

    std::string handleGetProfiles() {
        std::ostringstream json;
        json << R"({"success": true, "profiles": [)";
        
        bool first = true;
        for (const auto& [id, profile] : profiles_) {
            if (!first) json << ",";
            json << R"({"id": ")" << id << R"(", "name": ")" << profile.name 
                 << R"(", "createdAt": )" << std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::system_clock::now().time_since_epoch()).count()
                 << R"(, "lastAccess": )" << std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::system_clock::now().time_since_epoch()).count()
                 << R"(, "folderCount": )" << profile.folders.size() << "}";
            first = false;
        }
        
        json << "]}";
        return json.str();
    }

    std::string handleCreateProfile(const std::string& body) {
        // Simple JSON parsing
        std::string name = extractJsonValue(body, "name");
        std::string masterKey = extractJsonValue(body, "masterKey");
        
        if (name.empty() || masterKey.empty()) {
            return R"({"success": false, "error": "Name and master key required"})";
        }

        std::string profile_id = "profile_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()) + "_" + std::to_string(rand() % 10000);

        Profile profile;
        profile.id = profile_id;
        profile.name = name;
        profile.masterKey = masterKey;
        profiles_[profile_id] = profile;

        std::cout << "[ProfileManager] Created profile: " << name << " (ID: " << profile_id << ")" << std::endl;

        return R"({"success": true, "profileId": ")" + profile_id + 
               R"(", "message": "Profile and encrypted vault created successfully"})";
    }

    std::string handleAuthenticateProfile(const std::string& profile_id, const std::string& body) {
        std::string masterKey = extractJsonValue(body, "masterKey");
        
        auto it = profiles_.find(profile_id);
        if (it == profiles_.end()) {
            return R"({"success": false, "error": "Profile not found"})";
        }

        if (it->second.masterKey != masterKey) {
            return R"({"success": false, "error": "Invalid master key"})";
        }

        it->second.authenticated = true;
        authenticated_profile_ = profile_id;
        
        std::cout << "[ProfileManager] Authenticated profile: " << it->second.name << std::endl;
        
        return R"({"success": true, "message": "Profile authenticated successfully"})";
    }

    std::string handleGetPlatform() {
        return R"({
            "success": true,
            "platform": {"name": "Linux", "type": "linux"},
            "capabilities": {
                "supportsInvisibleLogging": true,
                "supportsHotkeys": true,
                "requiresPermissions": false
            }
        })";
    }

    std::string handleLockFolder(const std::string& body) {
        std::string profile_id = extractJsonValue(body, "profileId");
        std::string folder_path = extractJsonValue(body, "folderPath");
        std::string masterKey = extractJsonValue(body, "masterKey");

        auto it = profiles_.find(profile_id);
        if (it == profiles_.end()) {
            return R"({"success": false, "error": "Profile not found"})";
        }

        if (it->second.masterKey != masterKey) {
            return R"({"success": false, "error": "Invalid master key"})";
        }

        // Check if folder exists
        if (!fs::exists(folder_path)) {
            return R"({"success": false, "error": "Folder does not exist"})";
        }

        // Simple folder "encryption" - rename to hidden folder
        std::string hidden_path = folder_path + ".phantomvault_encrypted";
        
        try {
            // Move folder to hidden location (basic encryption simulation)
            fs::rename(folder_path, hidden_path);
            
            // Store the mapping
            it->second.folders.push_back(folder_path);
            
            std::cout << "[VaultManager] Locked folder: " << folder_path << " -> " << hidden_path 
                      << " for profile: " << it->second.name << std::endl;
            
            return R"({"success": true, "message": "Folder encrypted and secured successfully"})";
        } catch (const std::exception& e) {
            return R"({"success": false, "error": "Failed to encrypt folder: )" + std::string(e.what()) + R"("})";
        }
    }

    std::string handleGetVaultFolders(const std::string& profile_id) {
        auto it = profiles_.find(profile_id);
        if (it == profiles_.end()) {
            return R"({"success": false, "error": "Profile not found"})";
        }

        std::ostringstream json;
        json << R"({"success": true, "folders": [)";
        
        bool first = true;
        for (const auto& folder : it->second.folders) {
            if (!first) json << ",";
            json << R"({"id": "folder_)" << rand() % 10000 << R"(", "path": ")" << folder 
                 << R"(", "status": "locked", "size": 1024})";
            first = false;
        }
        
        json << "]}";
        return json.str();
    }

    std::string handleGetVaultStats(const std::string& profile_id) {
        auto it = profiles_.find(profile_id);
        if (it == profiles_.end()) {
            return R"({"success": false, "error": "Profile not found"})";
        }

        std::ostringstream json;
        json << R"({"success": true, "stats": {)";
        json << R"("totalFolders": )" << it->second.folders.size() << ",";
        json << R"("encryptedFolders": )" << it->second.folders.size() << ",";
        json << R"("totalSize": 1048576,)"; // 1MB simulated
        json << R"("lastBackup": ")" << std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() << R"(")";
        json << "}}";
        return json.str();
    }

    std::string handleUnlockTemporary(const std::string& body) {
        std::string profile_id = extractJsonValue(body, "profileId");
        std::string masterKey = extractJsonValue(body, "masterKey");

        auto it = profiles_.find(profile_id);
        if (it == profiles_.end()) {
            return R"({"success": false, "error": "Profile not found"})";
        }

        if (it->second.masterKey != masterKey) {
            return R"({"success": false, "error": "Invalid master key"})";
        }

        // Temporarily unlock folders (restore them)
        int unlocked_count = 0;
        for (const auto& folder_path : it->second.folders) {
            std::string hidden_path = folder_path + ".phantomvault_encrypted";
            
            try {
                if (fs::exists(hidden_path) && !fs::exists(folder_path)) {
                    fs::rename(hidden_path, folder_path);
                    unlocked_count++;
                    std::cout << "[VaultManager] Temporarily unlocked: " << folder_path << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "[VaultManager] Failed to unlock: " << folder_path << " - " << e.what() << std::endl;
            }
        }
        
        std::cout << "[VaultManager] Temporarily unlocked " << unlocked_count 
                  << " folders for profile: " << it->second.name << std::endl;
        
        return R"({"success": true, "message": "Folders unlocked temporarily", "successCount": )" 
               + std::to_string(unlocked_count) + "}";
    }

    std::string handleUnlockPermanent(const std::string& body) {
        std::string profile_id = extractJsonValue(body, "profileId");
        std::string masterKey = extractJsonValue(body, "masterKey");

        auto it = profiles_.find(profile_id);
        if (it == profiles_.end()) {
            return R"({"success": false, "error": "Profile not found"})";
        }

        if (it->second.masterKey != masterKey) {
            return R"({"success": false, "error": "Invalid master key"})";
        }

        // Permanently unlock folders (restore and remove from vault)
        int unlocked_count = 0;
        std::vector<std::string> remaining_folders;
        
        for (const auto& folder_path : it->second.folders) {
            std::string hidden_path = folder_path + ".phantomvault_encrypted";
            
            try {
                if (fs::exists(hidden_path) && !fs::exists(folder_path)) {
                    fs::rename(hidden_path, folder_path);
                    unlocked_count++;
                    std::cout << "[VaultManager] Permanently unlocked: " << folder_path << std::endl;
                    // Don't add to remaining_folders (permanently removed from vault)
                } else {
                    remaining_folders.push_back(folder_path);
                }
            } catch (const std::exception& e) {
                std::cout << "[VaultManager] Failed to unlock: " << folder_path << " - " << e.what() << std::endl;
                remaining_folders.push_back(folder_path);
            }
        }
        
        // Update the folders list (remove permanently unlocked ones)
        it->second.folders = remaining_folders;
        
        std::cout << "[VaultManager] Permanently unlocked " << unlocked_count 
                  << " folders for profile: " << it->second.name << std::endl;
        
        return R"({"success": true, "message": "Folders unlocked permanently", "successCount": )" 
               + std::to_string(unlocked_count) + "}";
    }

    std::string extractJsonValue(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\":";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";
        
        pos += search.length();
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        
        if (pos >= json.length() || json[pos] != '"') return "";
        pos++; // Skip opening quote
        
        size_t end = json.find('"', pos);
        if (end == std::string::npos) return "";
        
        return json.substr(pos, end - pos);
    }

    std::string createHttpResponse(const std::string& json) {
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: application/json\r\n";
        response << "Access-Control-Allow-Origin: *\r\n";
        response << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
        response << "Access-Control-Allow-Headers: Content-Type\r\n";
        response << "Content-Length: " << json.length() << "\r\n";
        response << "\r\n";
        response << json;
        return response.str();
    }
};

int main() {
    std::cout << "=== PhantomVault Simple Service ===" << std::endl;
    std::cout << "Starting service for GUI testing..." << std::endl;
    
    SimplePhantomVaultService service;
    if (service.start(9876)) {
        service.run();
    }
    
    return 0;
}