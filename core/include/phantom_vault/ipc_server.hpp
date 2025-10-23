#pragma once

#include "phantom_vault_export.h"
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <map>
#include <chrono>

namespace phantom_vault {
namespace service {

/**
 * @brief IPC message types for service-GUI communication
 */
enum class IPCMessageType {
    // Service -> GUI
    VAULT_STATE_UPDATE,     // Vault state changed
    FOLDER_STATUS_UPDATE,   // Folder lock/unlock status changed
    PROFILE_UPDATE,         // Profile information changed
    ERROR_NOTIFICATION,     // Error occurred
    
    // GUI -> Service
    GET_VAULT_STATE,        // Request current vault state
    GET_PROFILES,           // Request profile list
    ADD_FOLDER,             // Add folder to vault
    REMOVE_FOLDER,          // Remove folder from vault
    UNLOCK_FOLDERS,         // Unlock folders with password
    LOCK_FOLDERS,           // Lock folders
    CREATE_PROFILE,         // Create new profile
    DELETE_PROFILE,         // Delete profile
    PASSWORD_INPUT,         // Password input from GUI (fallback method)
    
    // Bidirectional
    PING,                   // Connection test
    PONG,                   // Ping response
    SHUTDOWN,               // Shutdown request/acknowledgment
    
    // Special
    UNKNOWN                 // Unknown message type
};

/**
 * @brief IPC message structure
 */
struct PHANTOM_VAULT_EXPORT IPCMessage {
    IPCMessageType type;
    std::string payload;        // JSON payload
    std::string request_id;     // For request/response matching
    std::string client_id;      // Client identifier
    
    IPCMessage() : type(IPCMessageType::UNKNOWN) {}
    IPCMessage(IPCMessageType t, const std::string& p = "", const std::string& rid = "")
        : type(t), payload(p), request_id(rid) {}
    
    // Serialize to JSON string
    std::string serialize() const;
    
    // Deserialize from JSON string
    static IPCMessage deserialize(const std::string& json);
};

/**
 * @brief IPC client connection information
 */
struct PHANTOM_VAULT_EXPORT IPCClient {
    int socket_fd;
    std::string client_id;
    std::chrono::system_clock::time_point connected_at;
    std::chrono::system_clock::time_point last_activity;
    
    IPCClient(int fd, const std::string& id)
        : socket_fd(fd), client_id(id)
        , connected_at(std::chrono::system_clock::now())
        , last_activity(std::chrono::system_clock::now()) {}
};

/**
 * @brief IPC server for PhantomVault service
 * 
 * Provides secure Unix domain socket communication between the background service
 * and GUI applications. Supports multiple concurrent clients and message routing.
 */
class PHANTOM_VAULT_EXPORT IPCServer {
public:
    /**
     * @brief Message handler callback type
     */
    using MessageHandler = std::function<IPCMessage(const IPCMessage&, const std::string& client_id)>;
    
    /**
     * @brief Client connection callback type
     */
    using ClientCallback = std::function<void(const std::string& client_id, bool connected)>;

    /**
     * @brief Constructor
     */
    IPCServer();

    /**
     * @brief Destructor
     */
    ~IPCServer();

    /**
     * @brief Initialize the IPC server
     * @param socket_path Path to Unix domain socket
     * @return true if initialization successful, false otherwise
     */
    bool initialize(const std::string& socket_path);

    /**
     * @brief Start the IPC server
     * @return true if started successfully, false otherwise
     */
    bool start();

    /**
     * @brief Stop the IPC server
     */
    void stop();

    /**
     * @brief Check if server is running
     * @return true if running, false otherwise
     */
    bool isRunning() const;

    /**
     * @brief Set message handler for specific message type
     * @param type Message type to handle
     * @param handler Handler function
     */
    void setMessageHandler(IPCMessageType type, MessageHandler handler);

    /**
     * @brief Set client connection callback
     * @param callback Callback function for client connect/disconnect
     */
    void setClientCallback(ClientCallback callback);

    /**
     * @brief Send message to specific client
     * @param client_id Client identifier
     * @param message Message to send
     * @return true if sent successfully, false otherwise
     */
    bool sendMessage(const std::string& client_id, const IPCMessage& message);

    /**
     * @brief Broadcast message to all connected clients
     * @param message Message to broadcast
     * @return Number of clients message was sent to
     */
    int broadcastMessage(const IPCMessage& message);

    /**
     * @brief Get list of connected clients
     * @return Vector of client IDs
     */
    std::vector<std::string> getConnectedClients() const;

    /**
     * @brief Get client connection information
     * @param client_id Client identifier
     * @return Client information or nullptr if not found
     */
    std::shared_ptr<IPCClient> getClientInfo(const std::string& client_id) const;

    /**
     * @brief Disconnect specific client
     * @param client_id Client identifier
     * @return true if disconnected, false if client not found
     */
    bool disconnectClient(const std::string& client_id);

    /**
     * @brief Get server statistics
     * @return Statistics as JSON string
     */
    std::string getServerStats() const;

    /**
     * @brief Get last error message
     * @return Error message string
     */
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class IPCServer

/**
 * @brief Helper functions for IPC message type conversion
 */
PHANTOM_VAULT_EXPORT std::string messageTypeToString(IPCMessageType type);
PHANTOM_VAULT_EXPORT IPCMessageType stringToMessageType(const std::string& str);

} // namespace service
} // namespace phantom_vault