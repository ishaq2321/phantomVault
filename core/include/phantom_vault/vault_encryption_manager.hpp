#pragma once

#include "phantom_vault_export.h"
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <functional>

namespace phantom_vault {
namespace service {

/**
 * @brief Result structure for encryption operations
 */
struct PHANTOM_VAULT_EXPORT EncryptionResult {
    bool success;
    std::string error_message;
    std::vector<std::string> processed_files;
    size_t total_files;
    size_t failed_files;
    
    EncryptionResult() : success(false), total_files(0), failed_files(0) {}
};

/**
 * @brief Password verification result
 */
struct PHANTOM_VAULT_EXPORT PasswordVerificationResult {
    bool is_valid;
    std::string error_message;
    
    PasswordVerificationResult() : is_valid(false) {}
    PasswordVerificationResult(bool valid) : is_valid(valid) {}
    PasswordVerificationResult(bool valid, const std::string& error) 
        : is_valid(valid), error_message(error) {}
};

/**
 * @brief Vault encryption manager for native service
 * 
 * Provides folder-level encryption and decryption operations using the same
 * EncryptionEngine as the Electron app, ensuring full compatibility.
 */
class PHANTOM_VAULT_EXPORT VaultEncryptionManager {
public:
    /**
     * @brief Progress callback type for encryption operations
     * @param current_file Current file being processed
     * @param files_processed Number of files processed so far
     * @param total_files Total number of files to process
     */
    using ProgressCallback = std::function<void(const std::string& current_file, 
                                               size_t files_processed, 
                                               size_t total_files)>;

    /**
     * @brief Constructor
     */
    VaultEncryptionManager();

    /**
     * @brief Destructor
     */
    ~VaultEncryptionManager();

    /**
     * @brief Initialize the encryption manager
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * @brief Encrypt a folder recursively
     * @param folder_path Path to folder to encrypt
     * @param password Password for encryption
     * @param progress_callback Optional progress callback
     * @return Encryption result with success status and details
     */
    EncryptionResult encryptFolder(const std::filesystem::path& folder_path, 
                                  const std::string& password,
                                  ProgressCallback progress_callback = nullptr);

    /**
     * @brief Decrypt a folder recursively
     * @param folder_path Path to folder to decrypt
     * @param password Password for decryption
     * @param progress_callback Optional progress callback
     * @return Encryption result with success status and details
     */
    EncryptionResult decryptFolder(const std::filesystem::path& folder_path, 
                                  const std::string& password,
                                  ProgressCallback progress_callback = nullptr);

    /**
     * @brief Verify password against hashed password
     * @param password Plain text password to verify
     * @param hashed_password Hashed password in format "salt:hash"
     * @return Password verification result
     */
    PasswordVerificationResult verifyPassword(const std::string& password, 
                                             const std::string& hashed_password);

    /**
     * @brief Hash password using PBKDF2 (compatible with VaultFolderManager.js)
     * @param password Plain text password
     * @param salt Optional salt (generates random if not provided)
     * @return Hashed password in format "salt:hash"
     */
    std::string hashPassword(const std::string& password, 
                           const std::string& salt = "");

    /**
     * @brief Derive encryption key from password using PBKDF2
     * @param password Plain text password
     * @param salt Salt for key derivation
     * @return Derived key bytes
     */
    std::vector<uint8_t> deriveKey(const std::string& password, 
                                  const std::vector<uint8_t>& salt);

    /**
     * @brief Generate random salt for key derivation
     * @return Random salt bytes
     */
    std::vector<uint8_t> generateSalt();

    /**
     * @brief Check if a folder appears to be encrypted
     * @param folder_path Path to folder to check
     * @return true if folder contains encrypted files, false otherwise
     */
    bool isFolderEncrypted(const std::filesystem::path& folder_path);

    /**
     * @brief Get encryption statistics for a folder
     * @param folder_path Path to folder to analyze
     * @return Total number of files that would be processed
     */
    size_t getFolderFileCount(const std::filesystem::path& folder_path);

    /**
     * @brief Get last error message
     * @return Error message string
     */
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
}; // class VaultEncryptionManager

} // namespace service
} // namespace phantom_vault