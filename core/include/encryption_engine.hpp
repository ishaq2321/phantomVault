#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstdint>

namespace PhantomVault {

/**
 * @brief Core encryption engine providing AES-256-CBC encryption with PBKDF2 key derivation
 * 
 * This class implements genuine cryptographic protection using industry-standard algorithms:
 * - AES-256-CBC for file encryption
 * - PBKDF2 with SHA-256 for key derivation
 * - Cryptographically secure random number generation
 * - Chunked processing for large files
 */
class EncryptionEngine {
public:
    /**
     * @brief Result structure for encryption operations
     */
    struct EncryptionResult {
        std::vector<uint8_t> encrypted_data;
        std::vector<uint8_t> iv;
        std::vector<uint8_t> salt;
        std::string algorithm;
        bool success;
        std::string error_message;
        
        EncryptionResult() : algorithm("AES-256-CBC"), success(false) {}
    };

    /**
     * @brief Metadata for encrypted files
     */
    struct FileMetadata {
        std::string original_path;
        std::string original_permissions;
        int64_t original_size;
        int64_t created_timestamp;
        int64_t modified_timestamp;
        int64_t accessed_timestamp;
        std::string checksum_sha256;
    };

    /**
     * @brief Configuration for key derivation
     */
    struct KeyDerivationConfig {
        int iterations;
        int salt_length;
        int key_length;
        
        KeyDerivationConfig() : iterations(100000), salt_length(32), key_length(32) {}
        KeyDerivationConfig(int iter, int salt_len, int key_len) 
            : iterations(iter), salt_length(salt_len), key_length(key_len) {}
    };

    static constexpr size_t AES_BLOCK_SIZE = 16;
    static constexpr size_t AES_KEY_SIZE = 32;  // 256 bits
    static constexpr size_t DEFAULT_CHUNK_SIZE = 1024 * 1024;  // 1MB chunks

    EncryptionEngine();
    ~EncryptionEngine();

    // Core encryption/decryption operations
    
    /**
     * @brief Encrypt a file using AES-256-CBC
     * @param file_path Path to the file to encrypt
     * @param password Password for key derivation
     * @param config Key derivation configuration
     * @return EncryptionResult containing encrypted data and metadata
     */
    EncryptionResult encryptFile(const std::string& file_path, 
                                const std::string& password,
                                const KeyDerivationConfig& config = KeyDerivationConfig());

    /**
     * @brief Decrypt a file using AES-256-CBC
     * @param encrypted_data The encrypted file data
     * @param password Password for key derivation
     * @param iv Initialization vector used during encryption
     * @param salt Salt used for key derivation
     * @param config Key derivation configuration
     * @return Decrypted file data, empty vector on failure
     */
    std::vector<uint8_t> decryptFile(const std::vector<uint8_t>& encrypted_data,
                                    const std::string& password,
                                    const std::vector<uint8_t>& iv,
                                    const std::vector<uint8_t>& salt,
                                    const KeyDerivationConfig& config = KeyDerivationConfig());

    /**
     * @brief Encrypt data in memory using AES-256-CBC
     * @param data Data to encrypt
     * @param key 256-bit encryption key
     * @param iv 128-bit initialization vector
     * @return Encrypted data, empty vector on failure
     */
    std::vector<uint8_t> encryptData(const std::vector<uint8_t>& data,
                                    const std::vector<uint8_t>& key,
                                    const std::vector<uint8_t>& iv);

    /**
     * @brief Decrypt data in memory using AES-256-CBC
     * @param encrypted_data Data to decrypt
     * @param key 256-bit encryption key
     * @param iv 128-bit initialization vector
     * @return Decrypted data, empty vector on failure
     */
    std::vector<uint8_t> decryptData(const std::vector<uint8_t>& encrypted_data,
                                    const std::vector<uint8_t>& key,
                                    const std::vector<uint8_t>& iv);

    // Key derivation and cryptographic utilities

    /**
     * @brief Derive encryption key from password using PBKDF2-SHA256
     * @param password Input password
     * @param salt Cryptographic salt
     * @param iterations Number of PBKDF2 iterations
     * @param key_length Desired key length in bytes
     * @return Derived key, empty vector on failure
     */
    std::vector<uint8_t> deriveKey(const std::string& password,
                                  const std::vector<uint8_t>& salt,
                                  int iterations = 100000,
                                  int key_length = AES_KEY_SIZE);

    /**
     * @brief Generate cryptographically secure random bytes
     * @param length Number of bytes to generate
     * @return Random bytes, empty vector on failure
     */
    std::vector<uint8_t> generateRandomBytes(size_t length);

    /**
     * @brief Generate cryptographically secure salt
     * @param length Salt length in bytes (default 32)
     * @return Random salt, empty vector on failure
     */
    std::vector<uint8_t> generateSalt(size_t length = 32);

    /**
     * @brief Generate initialization vector for AES
     * @return 128-bit IV, empty vector on failure
     */
    std::vector<uint8_t> generateIV();

    // File utilities

    /**
     * @brief Calculate SHA-256 checksum of file
     * @param file_path Path to file
     * @return Hex-encoded SHA-256 hash, empty string on failure
     */
    std::string calculateFileChecksum(const std::string& file_path);

    /**
     * @brief Get file metadata (size, timestamps, permissions)
     * @param file_path Path to file
     * @return FileMetadata structure
     */
    FileMetadata getFileMetadata(const std::string& file_path);

    /**
     * @brief Securely wipe memory containing sensitive data
     * @param data Pointer to data to wipe
     * @param size Size of data in bytes
     */
    static void secureWipe(void* data, size_t size);

    /**
     * @brief Securely wipe vector containing sensitive data
     * @param data Vector to wipe
     */
    static void secureWipe(std::vector<uint8_t>& data);

    // Validation and testing

    /**
     * @brief Validate encryption engine functionality
     * @return true if all self-tests pass
     */
    bool selfTest();

    /**
     * @brief Get last error message
     * @return Error message from last failed operation
     */
    const std::string& getLastError() const { return last_error_; }

private:
    std::string last_error_;

    // Internal encryption/decryption helpers
    bool encryptChunk(const uint8_t* input, size_t input_len,
                     const uint8_t* key, const uint8_t* iv,
                     std::vector<uint8_t>& output);
    
    bool decryptChunk(const uint8_t* input, size_t input_len,
                     const uint8_t* key, const uint8_t* iv,
                     std::vector<uint8_t>& output);

    // Error handling
    void setError(const std::string& error);
    void clearError();

    // OpenSSL context management
    class OpenSSLContext;
    std::unique_ptr<OpenSSLContext> ssl_context_;
};

} // namespace PhantomVault