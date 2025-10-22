#pragma once

#include "phantom_vault_export.h"
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <chrono>
#include <optional>

namespace phantom_vault {
namespace storage {

/**
 * @brief Structure representing vault metadata
 */
struct PHANTOM_VAULT_EXPORT VaultMetadata {
    std::string vault_id;                    // Unique identifier for the vault
    std::string name;                        // User-friendly vault name
    std::string description;                 // Optional vault description
    std::filesystem::path location;          // Path to the vault directory
    std::chrono::system_clock::time_point created_time;  // Vault creation time
    std::chrono::system_clock::time_point modified_time; // Last modification time
    std::vector<uint8_t> key_verification;   // Key verification data
    std::vector<uint8_t> salt;              // Salt for key derivation
    uint32_t iterations;                     // Number of iterations for key derivation
};

/**
 * @brief Structure representing vault configuration
 */
struct PHANTOM_VAULT_EXPORT VaultConfig {
    bool auto_lock;                          // Whether to auto-lock after timeout
    std::chrono::seconds lock_timeout;       // Auto-lock timeout duration
    bool clear_clipboard;                    // Whether to clear clipboard after copying
    std::chrono::seconds clipboard_timeout;  // Clipboard clear timeout
    bool hide_vault_dir;                     // Whether to hide vault directory
    bool secure_delete;                      // Whether to use secure deletion
    uint32_t secure_delete_passes;          // Number of passes for secure deletion
};

/**
 * @brief Structure representing recovery questions
 */
struct PHANTOM_VAULT_EXPORT RecoveryQuestion {
    std::string question_id;                 // Unique identifier for the question
    std::string question_text;               // The recovery question text
    std::vector<uint8_t> answer_hash;        // Hashed answer for verification
    std::vector<uint8_t> salt;               // Salt used for hashing the answer
};

/**
 * @brief Structure representing recovery information
 */
struct PHANTOM_VAULT_EXPORT RecoveryInfo {
    std::string vault_id;                    // Associated vault ID
    std::vector<RecoveryQuestion> questions; // Recovery questions and answers
    std::vector<uint8_t> recovery_key;       // Encrypted recovery key
    std::vector<uint8_t> recovery_iv;        // IV for recovery key encryption
    std::chrono::system_clock::time_point created_time;  // When recovery was set up
    std::chrono::system_clock::time_point last_used;     // Last time recovery was used
    uint32_t attempts_remaining;             // Number of recovery attempts left
};

/**
 * @brief Class for secure storage operations
 */
class PHANTOM_VAULT_EXPORT SecureStorage {
public:
    /**
     * @brief Constructor
     */
    SecureStorage();

    /**
     * @brief Destructor
     */
    ~SecureStorage();

    /**
     * @brief Initialize storage with master key
     * @param master_key The master encryption key
     * @return true if successful, false otherwise
     */
    bool initialize(const std::vector<uint8_t>& master_key);

    /**
     * @brief Save vault metadata
     * @param metadata The metadata to save
     * @return true if successful, false otherwise
     */
    bool saveVaultMetadata(const VaultMetadata& metadata);

    /**
     * @brief Load vault metadata
     * @param vault_id The ID of the vault to load
     * @return The metadata if successful, nullopt otherwise
     */
    std::optional<VaultMetadata> loadVaultMetadata(const std::string& vault_id);

    /**
     * @brief List all vault IDs
     * @return Vector of vault IDs
     */
    std::vector<std::string> listVaults();

    /**
     * @brief Delete vault metadata
     * @param vault_id The ID of the vault to delete
     * @return true if successful, false otherwise
     */
    bool deleteVaultMetadata(const std::string& vault_id);

    /**
     * @brief Save vault configuration
     * @param vault_id The ID of the vault
     * @param config The configuration to save
     * @return true if successful, false otherwise
     */
    bool saveVaultConfig(const std::string& vault_id, const VaultConfig& config);

    /**
     * @brief Load vault configuration
     * @param vault_id The ID of the vault
     * @return The configuration if successful, nullopt otherwise
     */
    std::optional<VaultConfig> loadVaultConfig(const std::string& vault_id);

    /**
     * @brief Set up password recovery for a vault
     * @param vault_id The ID of the vault
     * @param recovery_info Recovery information including questions and answers
     * @return true if successful, false otherwise
     */
    bool setupPasswordRecovery(const std::string& vault_id, const RecoveryInfo& recovery_info);

    /**
     * @brief Verify recovery answers and get recovery key
     * @param vault_id The ID of the vault
     * @param answers Vector of answers to recovery questions
     * @return Recovery key if answers are correct, empty vector otherwise
     */
    std::vector<uint8_t> verifyRecoveryAnswers(const std::string& vault_id, const std::vector<std::string>& answers);

    /**
     * @brief Check if password recovery is set up for a vault
     * @param vault_id The ID of the vault
     * @return true if recovery is set up, false otherwise
     */
    bool hasPasswordRecovery(const std::string& vault_id);

    /**
     * @brief Get recovery questions for a vault
     * @param vault_id The ID of the vault
     * @return Vector of recovery questions if available, empty vector otherwise
     */
    std::vector<RecoveryQuestion> getRecoveryQuestions(const std::string& vault_id);

    /**
     * @brief Remove password recovery for a vault
     * @param vault_id The ID of the vault
     * @return true if successful, false otherwise
     */
    bool removePasswordRecovery(const std::string& vault_id);

    /**
     * @brief Get the last error message
     * @return Error message string
     */
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class SecureStorage

} // namespace storage
} // namespace phantom_vault 