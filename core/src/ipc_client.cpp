/**
 * PhantomVault IPC Client Implementation
 */

#include "ipc_client.hpp"
#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include <sstream>
#include <iostream>

namespace phantomvault {

// Helper for HTTP responses
struct HTTPResponse {
    std::string data;
    long response_code;
};

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, HTTPResponse* response) {
    size_t total_size = size * nmemb;
    response->data.append(static_cast<char*>(contents), total_size);
    return total_size;
}

class IPCClient::Implementation {
public:
    Implementation(const std::string& host, int port) 
        : host_(host), port_(port), curl_(nullptr), connected_(false) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl_ = curl_easy_init();
    }

    ~Implementation() {
        if (curl_) {
            curl_easy_cleanup(curl_);
        }
        curl_global_cleanup();
    }

    bool connect() {
        if (!curl_) {
            last_error_ = "Failed to initialize CURL";
            return false;
        }

        // Test connection with platform endpoint (which exists)
        auto response = makeRequest("GET", "/api/platform");
        connected_ = response.success;
        
        if (!connected_) {
            last_error_ = "Cannot connect to PhantomVault service on " + host_ + ":" + std::to_string(port_);
        }
        
        return connected_;
    }

    void disconnect() {
        connected_ = false;
    }

    bool isConnected() const {
        return connected_;
    }

    IPCResponse getStatus() {
        // Use platform endpoint to get service status
        return makeRequest("GET", "/api/platform");
    }

    IPCResponse listProfiles() {
        return makeRequest("GET", "/api/profiles");
    }

    IPCResponse createProfile(const std::string& name, const std::string& password) {
        Json::Value payload;
        payload["name"] = name;
        payload["masterKey"] = password;  // Server expects 'masterKey' not 'password'
        
        Json::StreamWriterBuilder builder;
        std::string json_data = Json::writeString(builder, payload);
        
        return makeRequest("POST", "/api/profiles", json_data);
    }

    IPCResponse lockProfile(const std::string& profile_name) {
        Json::Value payload;
        payload["profile"] = profile_name;
        
        Json::StreamWriterBuilder builder;
        std::string json_data = Json::writeString(builder, payload);
        
        return makeRequest("POST", "/api/vault/lock/temporary", json_data);
    }

    IPCResponse unlockProfile(const std::string& profile_name) {
        Json::Value payload;
        payload["profile"] = profile_name;
        
        Json::StreamWriterBuilder builder;
        std::string json_data = Json::writeString(builder, payload);
        
        return makeRequest("POST", "/api/vault/unlock/temporary", json_data);
    }

    IPCResponse testKeyboard() {
        return makeRequest("GET", "/api/test-keyboard");
    }

    IPCResponse stopService() {
        return makeRequest("POST", "/api/service/stop");
    }

    IPCResponse restartService() {
        return makeRequest("POST", "/api/service/restart");
    }

    std::string getLastError() const {
        return last_error_;
    }

private:
    IPCResponse makeRequest(const std::string& method, const std::string& endpoint, const std::string& data = "") {
        IPCResponse response;
        response.success = false;

        if (!curl_) {
            response.message = "CURL not initialized";
            return response;
        }

        HTTPResponse http_response;
        std::string url = "http://" + host_ + ":" + std::to_string(port_) + endpoint;

        // Set CURL options
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &http_response);
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 2L);

        // Set method and data
        if (method == "POST") {
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, data.c_str());
            
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
        }

        // Perform request
        CURLcode res = curl_easy_perform(curl_);
        
        if (res != CURLE_OK) {
            response.message = "HTTP request failed: " + std::string(curl_easy_strerror(res));
            last_error_ = response.message;
            return response;
        }

        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_response.response_code);

        // Parse JSON response
        try {
            Json::Value json_response;
            Json::CharReaderBuilder builder;
            std::string errors;
            
            std::istringstream stream(http_response.data);
            if (Json::parseFromStream(builder, stream, &json_response, &errors)) {
                // Check both HTTP status and JSON success field
                response.success = (http_response.response_code == 200) && json_response.get("success", false).asBool();
                response.message = json_response.get("message", "Service responded successfully").asString();
                
                // Extract data fields
                if (json_response.isMember("data") && json_response["data"].isObject()) {
                    for (const auto& key : json_response["data"].getMemberNames()) {
                        response.data[key] = json_response["data"][key].asString();
                    }
                }
            } else {
                response.message = "Failed to parse JSON response: " + errors;
            }
        } catch (const std::exception& e) {
            response.message = "JSON parsing error: " + std::string(e.what());
        }

        return response;
    }

    std::string host_;
    int port_;
    CURL* curl_;
    bool connected_;
    std::string last_error_;
};

// Public interface implementation
IPCClient::IPCClient(const std::string& host, int port) 
    : impl_(std::make_unique<Implementation>(host, port)) {
}

IPCClient::~IPCClient() = default;

bool IPCClient::connect() {
    return impl_->connect();
}

void IPCClient::disconnect() {
    impl_->disconnect();
}

bool IPCClient::isConnected() const {
    return impl_->isConnected();
}

IPCResponse IPCClient::getStatus() {
    return impl_->getStatus();
}

IPCResponse IPCClient::listProfiles() {
    return impl_->listProfiles();
}

IPCResponse IPCClient::createProfile(const std::string& name, const std::string& password) {
    return impl_->createProfile(name, password);
}

IPCResponse IPCClient::lockProfile(const std::string& profile_name) {
    return impl_->lockProfile(profile_name);
}

IPCResponse IPCClient::unlockProfile(const std::string& profile_name) {
    return impl_->unlockProfile(profile_name);
}

IPCResponse IPCClient::testKeyboard() {
    return impl_->testKeyboard();
}

IPCResponse IPCClient::stopService() {
    return impl_->stopService();
}

IPCResponse IPCClient::restartService() {
    return impl_->restartService();
}

std::string IPCClient::getLastError() const {
    return impl_->getLastError();
}

} // namespace phantomvault