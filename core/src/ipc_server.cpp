#include "phantom_vault/ipc_server.hpp"
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>
#include <vector>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

// JSON library for message serialization
#include <nlohmann/json.hpp>

namespace phantom_vault {
namespace service {

// IPCMessage implementation
std::string IPCMessage::serialize() const {
    nlohmann::json j;
    j["type"] = messageTypeToString(type);
    j["payload"] = payload;
    j["request_id"] = request_id;
    j["client_id"] = client_id;
    return j.dump();
}

IPCMessage IPCMessage::deserialize(const std::string& json) {
    try {
        nlohmann::json j = nlohmann::json::parse(json);
        IPCMessage msg;
        msg.type = stringToMessageType(j.value("type", ""));
        msg.payload = j.value("payload", "");
        msg.request_id = j.value("request_id", "");
        msg.client_id = j.value("client_id", "");
        return msg;
    } catch (const std::exception& e) {
        std::cout << "[IPCMessage] Deserialization error: " << e.what() << std::endl;
        return IPCMessage(); // Returns UNKNOWN type
    }
}

// Message type conversion functions
std::string messageTypeToString(IPCMessageType type) {
    switch (type) {
        case IPCMessageType::VAULT_STATE_UPDATE: return "VAULT_STATE_UPDATE";
        case IPCMessageType::FOLDER_STATUS_UPDATE: return "FOLDER_STATUS_UPDATE";
        case IPCMessageType::PROFILE_UPDATE: return "PROFILE_UPDATE";
        case IPCMessageType::ERROR_NOTIFICATION: return "ERROR_NOTIFICATION";
        case IPCMessageType::GET_VAULT_STATE: return "GET_VAULT_STATE";
        case IPCMessageType::GET_PROFILES: return "GET_PROFILES";
        case IPCMessageType::ADD_FOLDER: return "ADD_FOLDER";
        case IPCMessageType::REMOVE_FOLDER: return "REMOVE_FOLDER";
        case IPCMessageType::UNLOCK_FOLDERS: return "UNLOCK_FOLDERS";
        case IPCMessageType::LOCK_FOLDERS: return "LOCK_FOLDERS";
        case IPCMessageType::CREATE_PROFILE: return "CREATE_PROFILE";
        case IPCMessageType::DELETE_PROFILE: return "DELETE_PROFILE";
        case IPCMessageType::PING: return "PING";
        case IPCMessageType::PONG: return "PONG";
        case IPCMessageType::SHUTDOWN: return "SHUTDOWN";
        case IPCMessageType::UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

IPCMessageType stringToMessageType(const std::string& str) {
    if (str == "VAULT_STATE_UPDATE") return IPCMessageType::VAULT_STATE_UPDATE;
    if (str == "FOLDER_STATUS_UPDATE") return IPCMessageType::FOLDER_STATUS_UPDATE;
    if (str == "PROFILE_UPDATE") return IPCMessageType::PROFILE_UPDATE;
    if (str == "ERROR_NOTIFICATION") return IPCMessageType::ERROR_NOTIFICATION;
    if (str == "GET_VAULT_STATE") return IPCMessageType::GET_VAULT_STATE;
    if (str == "GET_PROFILES") return IPCMessageType::GET_PROFILES;
    if (str == "ADD_FOLDER") return IPCMessageType::ADD_FOLDER;
    if (str == "REMOVE_FOLDER") return IPCMessageType::REMOVE_FOLDER;
    if (str == "UNLOCK_FOLDERS") return IPCMessageType::UNLOCK_FOLDERS;
    if (str == "LOCK_FOLDERS") return IPCMessageType::LOCK_FOLDERS;
    if (str == "CREATE_PROFILE") return IPCMessageType::CREATE_PROFILE;
    if (str == "DELETE_PROFILE") return IPCMessageType::DELETE_PROFILE;
    if (str == "PING") return IPCMessageType::PING;
    if (str == "PONG") return IPCMessageType::PONG;
    if (str == "SHUTDOWN") return IPCMessageType::SHUTDOWN;
    return IPCMessageType::UNKNOWN;
}

// IPCServer implementation
class IPCServer::Implementation {
public:
    Implementation()
        : server_socket_(-1)
        , is_running_(false)
        , should_stop_(false)
        , socket_path_()
        , server_thread_()
        , clients_()
        , message_handlers_()
        , client_callback_()
        , last_error_()
        , mutex_()
        , next_client_id_(1)
    {}

    ~Implementation() {
        stop();
    }

    bool initialize(const std::string& socket_path) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (is_running_) {
            last_error_ = "Server is already running";
            return false;
        }
        
        socket_path_ = socket_path;
        
        // Remove existing socket file
        unlink(socket_path_.c_str());
        
        // Create Unix domain socket
        server_socket_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_socket_ == -1) {
            last_error_ = "Failed to create socket: " + std::string(strerror(errno));
            return false;
        }
        
        // Set socket to non-blocking
        int flags = fcntl(server_socket_, F_GETFL, 0);
        if (flags == -1 || fcntl(server_socket_, F_SETFL, flags | O_NONBLOCK) == -1) {
            last_error_ = "Failed to set socket non-blocking: " + std::string(strerror(errno));
            close(server_socket_);
            server_socket_ = -1;
            return false;
        }
        
        // Bind socket
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);
        
        if (bind(server_socket_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            last_error_ = "Failed to bind socket: " + std::string(strerror(errno));
            close(server_socket_);
            server_socket_ = -1;
            return false;
        }
        
        // Listen for connections
        if (listen(server_socket_, 5) == -1) {
            last_error_ = "Failed to listen on socket: " + std::string(strerror(errno));
            close(server_socket_);
            server_socket_ = -1;
            return false;
        }
        
        std::cout << "[IPCServer] Initialized on socket: " << socket_path_ << std::endl;
        return true;
    }

    bool start() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (is_running_) {
            return true;
        }
        
        if (server_socket_ == -1) {
            last_error_ = "Server not initialized";
            return false;
        }
        
        should_stop_ = false;
        is_running_ = true;
        
        // Start server thread
        server_thread_ = std::thread([this]() {
            serverLoop();
        });
        
        std::cout << "[IPCServer] Started successfully" << std::endl;
        return true;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!is_running_) {
                return;
            }
            
            should_stop_ = true;
            is_running_ = false;
        }
        
        // Join server thread
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        
        // Close all client connections
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& [client_id, client] : clients_) {
                close(client->socket_fd);
            }
            clients_.clear();
        }
        
        // Close server socket
        if (server_socket_ != -1) {
            close(server_socket_);
            server_socket_ = -1;
        }
        
        // Remove socket file
        if (!socket_path_.empty()) {
            unlink(socket_path_.c_str());
        }
        
        std::cout << "[IPCServer] Stopped" << std::endl;
    }

    bool isRunning() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return is_running_;
    }

    void setMessageHandler(IPCMessageType type, MessageHandler handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        message_handlers_[type] = handler;
    }

    void setClientCallback(ClientCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        client_callback_ = callback;
    }

    bool sendMessage(const std::string& client_id, const IPCMessage& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = clients_.find(client_id);
        if (it == clients_.end()) {
            return false;
        }
        
        return sendMessageToSocket(it->second->socket_fd, message);
    }

    int broadcastMessage(const IPCMessage& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        int sent_count = 0;
        for (auto& [client_id, client] : clients_) {
            if (sendMessageToSocket(client->socket_fd, message)) {
                sent_count++;
            }
        }
        
        return sent_count;
    }

    std::vector<std::string> getConnectedClients() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<std::string> client_ids;
        for (const auto& [client_id, client] : clients_) {
            client_ids.push_back(client_id);
        }
        
        return client_ids;
    }

    std::shared_ptr<IPCClient> getClientInfo(const std::string& client_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = clients_.find(client_id);
        if (it != clients_.end()) {
            return it->second;
        }
        
        return nullptr;
    }

    bool disconnectClient(const std::string& client_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = clients_.find(client_id);
        if (it == clients_.end()) {
            return false;
        }
        
        close(it->second->socket_fd);
        clients_.erase(it);
        
        // Notify callback
        if (client_callback_) {
            client_callback_(client_id, false);
        }
        
        return true;
    }

    std::string getServerStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        nlohmann::json stats;
        stats["running"] = is_running_.load();
        stats["socket_path"] = socket_path_;
        stats["connected_clients"] = clients_.size();
        stats["message_handlers"] = message_handlers_.size();
        
        nlohmann::json client_list = nlohmann::json::array();
        for (const auto& [client_id, client] : clients_) {
            nlohmann::json client_info;
            client_info["id"] = client_id;
            client_info["socket_fd"] = client->socket_fd;
            client_info["connected_at"] = std::chrono::duration_cast<std::chrono::seconds>(
                client->connected_at.time_since_epoch()).count();
            client_list.push_back(client_info);
        }
        stats["clients"] = client_list;
        
        return stats.dump(2);
    }

    std::string getLastError() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return last_error_;
    }

private:
    void serverLoop() {
        std::cout << "[IPCServer] Server loop started" << std::endl;
        
        while (!should_stop_) {
            try {
                // Accept new connections
                acceptNewConnections();
                
                // Handle existing client messages
                handleClientMessages();
                
                // Clean up disconnected clients
                cleanupDisconnectedClients();
                
                // Small delay to prevent busy waiting
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                
            } catch (const std::exception& e) {
                std::cout << "[IPCServer] Exception in server loop: " << e.what() << std::endl;
            }
        }
        
        std::cout << "[IPCServer] Server loop ended" << std::endl;
    }

    void acceptNewConnections() {
        struct sockaddr_un client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cout << "[IPCServer] Accept error: " << strerror(errno) << std::endl;
            }
            return;
        }
        
        // Set client socket to non-blocking
        int flags = fcntl(client_socket, F_GETFL, 0);
        if (flags == -1 || fcntl(client_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
            std::cout << "[IPCServer] Failed to set client socket non-blocking" << std::endl;
            close(client_socket);
            return;
        }
        
        // Create client ID
        std::string client_id = "client_" + std::to_string(next_client_id_++);
        
        // Add client
        {
            std::lock_guard<std::mutex> lock(mutex_);
            clients_[client_id] = std::make_shared<IPCClient>(client_socket, client_id);
        }
        
        std::cout << "[IPCServer] New client connected: " << client_id << std::endl;
        
        // Notify callback
        if (client_callback_) {
            client_callback_(client_id, true);
        }
    }

    void handleClientMessages() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (auto& [client_id, client] : clients_) {
            std::string message_data = readMessageFromSocket(client->socket_fd);
            if (!message_data.empty()) {
                client->last_activity = std::chrono::system_clock::now();
                processClientMessage(client_id, message_data);
            }
        }
    }

    void processClientMessage(const std::string& client_id, const std::string& message_data) {
        IPCMessage message = IPCMessage::deserialize(message_data);
        if (message.type == IPCMessageType::UNKNOWN) {
            std::cout << "[IPCServer] Received unknown message from " << client_id << std::endl;
            return;
        }
        
        message.client_id = client_id; // Ensure client ID is set
        
        std::cout << "[IPCServer] Received " << messageTypeToString(message.type) 
                  << " from " << client_id << std::endl;
        
        // Handle built-in messages
        if (message.type == IPCMessageType::PING) {
            IPCMessage pong(IPCMessageType::PONG, "pong", message.request_id);
            sendMessage(client_id, pong);
            return;
        }
        
        // Find and call message handler
        auto handler_it = message_handlers_.find(message.type);
        if (handler_it != message_handlers_.end()) {
            try {
                IPCMessage response = handler_it->second(message, client_id);
                if (response.type != IPCMessageType::UNKNOWN && !message.request_id.empty()) {
                    response.request_id = message.request_id;
                    sendMessage(client_id, response);
                }
            } catch (const std::exception& e) {
                std::cout << "[IPCServer] Handler exception: " << e.what() << std::endl;
                
                // Send error response
                IPCMessage error(IPCMessageType::ERROR_NOTIFICATION, 
                               "Handler error: " + std::string(e.what()), 
                               message.request_id);
                sendMessage(client_id, error);
            }
        } else {
            std::cout << "[IPCServer] No handler for message type: " 
                      << messageTypeToString(message.type) << std::endl;
        }
    }

    void cleanupDisconnectedClients() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = clients_.begin();
        while (it != clients_.end()) {
            // Check if client is still connected by trying to send empty data
            char test_byte = 0;
            ssize_t result = send(it->second->socket_fd, &test_byte, 0, MSG_NOSIGNAL);
            
            if (result == -1 && (errno == EPIPE || errno == ECONNRESET)) {
                std::cout << "[IPCServer] Client disconnected: " << it->first << std::endl;
                
                close(it->second->socket_fd);
                
                // Notify callback
                if (client_callback_) {
                    client_callback_(it->first, false);
                }
                
                it = clients_.erase(it);
            } else {
                ++it;
            }
        }
    }

    bool sendMessageToSocket(int socket_fd, const IPCMessage& message) {
        std::string serialized = message.serialize();
        uint32_t message_length = serialized.length();
        
        // Send message length first
        ssize_t sent = send(socket_fd, &message_length, sizeof(message_length), MSG_NOSIGNAL);
        if (sent != sizeof(message_length)) {
            return false;
        }
        
        // Send message data
        sent = send(socket_fd, serialized.c_str(), message_length, MSG_NOSIGNAL);
        return (sent == static_cast<ssize_t>(message_length));
    }

    std::string readMessageFromSocket(int socket_fd) {
        // Read message length first
        uint32_t message_length;
        ssize_t received = recv(socket_fd, &message_length, sizeof(message_length), MSG_DONTWAIT);
        if (received != sizeof(message_length)) {
            return "";
        }
        
        // Sanity check message length
        if (message_length > 1024 * 1024) { // 1MB limit
            std::cout << "[IPCServer] Message too large: " << message_length << " bytes" << std::endl;
            return "";
        }
        
        // Read message data
        std::string message_data(message_length, '\0');
        received = recv(socket_fd, &message_data[0], message_length, MSG_DONTWAIT);
        if (received != static_cast<ssize_t>(message_length)) {
            return "";
        }
        
        return message_data;
    }

    int server_socket_;
    std::atomic<bool> is_running_;
    std::atomic<bool> should_stop_;
    std::string socket_path_;
    std::thread server_thread_;
    std::map<std::string, std::shared_ptr<IPCClient>> clients_;
    std::map<IPCMessageType, MessageHandler> message_handlers_;
    ClientCallback client_callback_;
    std::string last_error_;
    mutable std::mutex mutex_;
    std::atomic<int> next_client_id_;
};

// IPCServer public interface
IPCServer::IPCServer() : pimpl(std::make_unique<Implementation>()) {}
IPCServer::~IPCServer() = default;

bool IPCServer::initialize(const std::string& socket_path) {
    return pimpl->initialize(socket_path);
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

void IPCServer::setMessageHandler(IPCMessageType type, MessageHandler handler) {
    pimpl->setMessageHandler(type, handler);
}

void IPCServer::setClientCallback(ClientCallback callback) {
    pimpl->setClientCallback(callback);
}

bool IPCServer::sendMessage(const std::string& client_id, const IPCMessage& message) {
    return pimpl->sendMessage(client_id, message);
}

int IPCServer::broadcastMessage(const IPCMessage& message) {
    return pimpl->broadcastMessage(message);
}

std::vector<std::string> IPCServer::getConnectedClients() const {
    return pimpl->getConnectedClients();
}

std::shared_ptr<IPCClient> IPCServer::getClientInfo(const std::string& client_id) const {
    return pimpl->getClientInfo(client_id);
}

bool IPCServer::disconnectClient(const std::string& client_id) {
    return pimpl->disconnectClient(client_id);
}

std::string IPCServer::getServerStats() const {
    return pimpl->getServerStats();
}

std::string IPCServer::getLastError() const {
    return pimpl->getLastError();
}

} // namespace service
} // namespace phantom_vault