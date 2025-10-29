#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <chrono>
#include <memory_resource>

// Forward declarations
struct evp_cipher_ctx_st;
typedef struct evp_cipher_ctx_st EVP_CIPHER_CTX;

namespace PhantomVault {

/**
 * @brief Core encryption engine providing AES-256-XTS encryption with Argon2id key derivation
 * 
 * This class implements military-grade cryptographic protection using industry-standard algorithms:
 * - AES-256-XTS for file encryption (stronger than CBC, prevents watermarking attacks)
 * - Argon2id for key derivation (memory-hard, resistant to GPU attacks)
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
        std::string compression_algorithm;
        size_t original_size;
        size_t compressed_size;
        bool success;
        std::string error_message;
        
        EncryptionResult() : algorithm("AES-256-XTS"), compression_algorithm("zstd"), original_size(0), compressed_size(0), success(false) {}
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
     * @brief Configuration for Argon2id key derivation
     */
    struct KeyDerivationConfig {
        uint32_t memory_cost;    // Memory usage in KiB
        uint32_t time_cost;      // Number of iterations
        uint32_t parallelism;    // Number of parallel threads
        int salt_length;
        int key_length;
        
        KeyDerivationConfig() : memory_cost(65536), time_cost(3), parallelism(4), salt_length(32), key_length(64) {}
        KeyDerivationConfig(uint32_t mem, uint32_t time, uint32_t parallel, int salt_len, int key_len) 
            : memory_cost(mem), time_cost(time), parallelism(parallel), salt_length(salt_len), key_length(key_len) {}
    };

    static constexpr size_t AES_BLOCK_SIZE = 16;
    static constexpr size_t AES_KEY_SIZE = 64;  // 512 bits for XTS mode (2 x 256-bit keys)
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
     * @brief Decrypt file with compression support
     * @param encrypted_data The encrypted file data
     * @param password Password for key derivation
     * @param iv Initialization vector used during encryption
     * @param salt Salt used for key derivation
     * @param compression_algorithm Compression algorithm used ("zstd", "none")
     * @param original_size Original size before compression
     * @param config Key derivation configuration
     * @return Decrypted and decompressed file data, empty vector on failure
     */
    std::vector<uint8_t> decryptFile(const std::vector<uint8_t>& encrypted_data,
                                    const std::string& password,
                                    const std::vector<uint8_t>& iv,
                                    const std::vector<uint8_t>& salt,
                                    const std::string& compression_algorithm,
                                    size_t original_size,
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
     * @brief Derive encryption key from password using Argon2id
     * @param password Input password
     * @param salt Cryptographic salt
     * @param config Argon2id configuration parameters
     * @return Derived key, empty vector on failure
     */
    std::vector<uint8_t> deriveKey(const std::string& password,
                                  const std::vector<uint8_t>& salt,
                                  const KeyDerivationConfig& config = KeyDerivationConfig());

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

    // Compression utilities

    /**
     * @brief Compress data using Zstandard algorithm
     * @param data Data to compress
     * @param compression_level Compression level (1-22, higher = better compression)
     * @return Compressed data, empty vector on failure
     */
    std::vector<uint8_t> compressData(const std::vector<uint8_t>& data, int compression_level = 3);

    /**
     * @brief Decompress data using Zstandard algorithm
     * @param compressed_data Compressed data to decompress
     * @param original_size Expected size of decompressed data
     * @return Decompressed data, empty vector on failure
     */
    std::vector<uint8_t> decompressData(const std::vector<uint8_t>& compressed_data, size_t original_size);

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

    /**
     * @brief Constant-time comparison to prevent timing attacks
     * @param a First data buffer
     * @param b Second data buffer
     * @param size Size of both buffers
     * @return true if buffers are equal, false otherwise
     */
    static bool constantTimeCompare(const void* a, const void* b, size_t size);

    /**
     * @brief Constant-time comparison for vectors
     * @param a First vector
     * @param b Second vector
     * @return true if vectors are equal, false otherwise
     */
    static bool constantTimeCompare(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b);

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
    
    // SIMD and parallel processing optimizations
    void enableSIMDOptimizations();
    void disableSIMDOptimizations();
    bool isSIMDEnabled() const;
    void setParallelProcessingThreads(size_t thread_count);
    size_t getParallelProcessingThreads() const;
    
    // Performance profiling and optimization
    void enablePerformanceProfiling();
    void disablePerformanceProfiling();
    std::chrono::nanoseconds getLastOperationTime() const;
    double getThroughputMBps() const;
    
    // Memory pool allocation for zero-overhead operations
    void enableMemoryPooling();
    void disableMemoryPooling();
    size_t getMemoryPoolSize() const;

private:
    // OpenSSL context management (must be first for proper initialization order)
    class OpenSSLContext;
    std::unique_ptr<OpenSSLContext> ssl_context_;
    
    std::string last_error_;
    
    // SIMD and performance optimization members
    bool simd_enabled_;
    size_t parallel_threads_;
    bool profiling_enabled_;
    mutable std::chrono::nanoseconds last_operation_time_;
    mutable double last_throughput_mbps_;
    bool memory_pooling_enabled_;
    std::unique_ptr<std::pmr::memory_resource> memory_pool_;

    // Internal encryption/decryption helpers
    bool encryptChunk(const uint8_t* input, size_t input_len,
                     const uint8_t* key, const uint8_t* iv,
                     std::vector<uint8_t>& output);
    
    bool decryptChunk(const uint8_t* input, size_t input_len,
                     const uint8_t* key, const uint8_t* iv,
                     std::vector<uint8_t>& output);
    
    // SIMD-optimized encryption helper
    bool encryptDataSIMD(const std::vector<uint8_t>& data, 
                        std::vector<uint8_t>& encrypted_data,
                        EVP_CIPHER_CTX* ctx, int& len, int& total_len);

    // Error handling
    void setError(const std::string& error);
    void clearError();
};

} // namespace PhantomVault