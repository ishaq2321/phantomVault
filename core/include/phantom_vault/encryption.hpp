#pragma once

#include "phantom_vault_export.h"
#include <string>
#include <vector>
#include <memory>
#include <filesystem>

namespace phantom_vault {

/**
 * @brief Class handling encryption and decryption operations
 */
class PHANTOM_VAULT_EXPORT EncryptionEngine {
public:
    /**
     * @brief Constructor
     */
    EncryptionEngine();

    /**
     * @brief Destructor
     */
    ~EncryptionEngine();

    /**
     * @brief Initialize the encryption engine
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Generate a new encryption key
     * @return Vector containing the generated key
     */
    std::vector<uint8_t> generateKey() const;

    /**
     * @brief Generate a random initialization vector (IV)
     * @return Vector containing the generated IV
     */
    std::vector<uint8_t> generateIV() const;

    /**
     * @brief Encrypt data in memory using AES-256
     * @param data Data to encrypt
     * @param key Encryption key
     * @param iv Initialization vector
     * @return Encrypted data if successful, empty vector otherwise
     */
    std::vector<uint8_t> encryptData(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv
    );

    /**
     * @brief Decrypt data in memory using AES-256
     * @param encrypted_data Data to decrypt
     * @param key Decryption key
     * @param iv Initialization vector
     * @return Decrypted data if successful, empty vector otherwise
     */
    std::vector<uint8_t> decryptData(
        const std::vector<uint8_t>& encrypted_data,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv
    );

    /**
     * @brief Encrypt a file using AES-256
     * @param source_path Path to the source file
     * @param dest_path Path where the encrypted file will be saved
     * @param key Encryption key
     * @param iv Initialization vector
     * @return true if encryption successful, false otherwise
     */
    bool encryptFile(
        const std::filesystem::path& source_path,
        const std::filesystem::path& dest_path,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv
    );

    /**
     * @brief Decrypt a file using AES-256
     * @param source_path Path to the encrypted file
     * @param dest_path Path where the decrypted file will be saved
     * @param key Decryption key
     * @param iv Initialization vector
     * @return true if decryption successful, false otherwise
     */
    bool decryptFile(
        const std::filesystem::path& source_path,
        const std::filesystem::path& dest_path,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv
    );

    /**
     * @brief Derive an encryption key from a password
     * @param password The password to derive the key from
     * @param salt Salt value for key derivation
     * @return Vector containing the derived key
     */
    std::vector<uint8_t> deriveKeyFromPassword(
        const std::string& password,
        const std::vector<uint8_t>& salt
    ) const;

    /**
     * @brief Generate a random salt for key derivation
     * @return Vector containing the generated salt
     */
    std::vector<uint8_t> generateSalt() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class EncryptionEngine

} // namespace phantom_vault 