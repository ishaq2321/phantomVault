#include "phantom_vault/vault_encryption_manager.hpp"
#include "phantom_vault/encryption.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <thread>
#include <future>
#include <queue>
#include <map>
#include <openssl/rand.h>
#include <openssl/evp.h>

namespace phantom_vault {
namespace service {

class VaultEncryptionManager::Implementation {
public:
    Implementation()
        : encryption_engine_()
        , last_error_()
        , mutex_()
    {}

    ~Implementation() = default;

    bool initialize() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        encryption_engine_ = std::make_unique<EncryptionEngine>();
        if (!encryption_engine_->initialize()) {
            last_error_ = "Failed to initialize encryption engine";
            return false;
        }
        
        std::cout << "[VaultEncryptionManager] Initialized successfully" << std::endl;
        return true;
    }

    EncryptionResult encryptFolder(const std::filesystem::path& folder_path, 
                                  const std::string& password,
                                  ProgressCallback progress_callback) {
        EncryptionResult result;
        
        if (!encryption_engine_) {
            result.error_message = "Encryption engine not initialized";
            return result;
        }
        
        if (!std::filesystem::exists(folder_path)) {
            result.error_message = "Folder does not exist: " + folder_path.string();
            return result;
        }
        
        if (!std::filesystem::is_directory(folder_path)) {
            result.error_message = "Path is not a directory: " + folder_path.string();
            return result;
        }
        
        std::cout << "[VaultEncryptionManager] Encrypting folder: " << folder_path << std::endl;
        
        try {
            // Get list of all files to encrypt
            std::vector<std::filesystem::path> files_to_encrypt;
            collectFiles(folder_path, files_to_encrypt);
            
            result.total_files = files_to_encrypt.size();
            
            if (result.total_files == 0) {
                std::cout << "[VaultEncryptionManager] No files to encrypt" << std::endl;
                result.success = true;
                return result;
            }
            
            std::cout << "[VaultEncryptionManager] Found " << result.total_files << " files to encrypt" << std::endl;
            
            // Generate salt and derive key
            std::vector<uint8_t> salt = encryption_engine_->generateSalt();
            std::vector<uint8_t> key = encryption_engine_->deriveKeyFromPassword(password, salt);
            
            // Store file IVs for metadata
            std::vector<std::pair<std::string, std::vector<uint8_t>>> file_ivs;
            
            // Process files
            size_t processed = 0;
            for (const auto& file_path : files_to_encrypt) {
                if (progress_callback) {
                    progress_callback(file_path.filename().string(), processed, result.total_files);
                }
                
                std::vector<uint8_t> file_iv;
                if (encryptSingleFile(file_path, key, file_iv)) {
                    result.processed_files.push_back(file_path.string());
                    
                    // Store relative path and IV for metadata
                    std::filesystem::path relative_path = std::filesystem::relative(file_path, folder_path);
                    file_ivs.emplace_back(relative_path.string(), file_iv);
                    
                    processed++;
                } else {
                    result.failed_files++;
                    std::cerr << "[VaultEncryptionManager] Failed to encrypt: " << file_path << std::endl;
                }
            }
            
            // Save salt and encryption metadata
            if (!saveFolderEncryptionMetadata(folder_path, salt, file_ivs)) {
                result.error_message = "Failed to save encryption metadata";
                return result;
            }
            
            result.success = (result.failed_files == 0);
            
            if (result.success) {
                std::cout << "[VaultEncryptionManager] ✅ Folder encrypted successfully (" 
                         << processed << " files)" << std::endl;
            } else {
                std::cout << "[VaultEncryptionManager] ⚠️ Folder encryption completed with " 
                         << result.failed_files << " failures" << std::endl;
            }
            
        } catch (const std::exception& e) {
            result.error_message = "Encryption failed: " + std::string(e.what());
            std::cerr << "[VaultEncryptionManager] " << result.error_message << std::endl;
        }
        
        return result;
    }

    EncryptionResult decryptFolder(const std::filesystem::path& folder_path, 
                                  const std::string& password,
                                  ProgressCallback progress_callback) {
        EncryptionResult result;
        
        if (!encryption_engine_) {
            result.error_message = "Encryption engine not initialized";
            return result;
        }
        
        if (!std::filesystem::exists(folder_path)) {
            result.error_message = "Folder does not exist: " + folder_path.string();
            return result;
        }
        
        std::cout << "[VaultEncryptionManager] Decrypting folder: " << folder_path << std::endl;
        
        try {
            // Load encryption metadata
            std::vector<uint8_t> salt;
            std::map<std::string, std::vector<uint8_t>> file_ivs;
            if (!loadFolderEncryptionMetadata(folder_path, salt, file_ivs)) {
                result.error_message = "Failed to load encryption metadata";
                return result;
            }
            
            // Derive key
            std::vector<uint8_t> key = encryption_engine_->deriveKeyFromPassword(password, salt);
            
            result.total_files = file_ivs.size();
            
            if (result.total_files == 0) {
                std::cout << "[VaultEncryptionManager] No encrypted files found" << std::endl;
                result.success = true;
                return result;
            }
            
            std::cout << "[VaultEncryptionManager] Found " << result.total_files << " files to decrypt" << std::endl;
            
            // Process files using metadata
            size_t processed = 0;
            for (const auto& [relative_path, iv] : file_ivs) {
                if (progress_callback) {
                    std::string display_name = std::filesystem::path(relative_path).filename().string();
                    progress_callback(display_name, processed, result.total_files);
                }
                
                std::filesystem::path encrypted_file_path = folder_path / (relative_path + ".enc");
                
                if (decryptSingleFile(encrypted_file_path, key, iv)) {
                    result.processed_files.push_back(encrypted_file_path.string());
                    processed++;
                } else {
                    result.failed_files++;
                    std::cerr << "[VaultEncryptionManager] Failed to decrypt: " << encrypted_file_path << std::endl;
                }
            }
            
            // Remove encryption metadata
            removeFolderEncryptionMetadata(folder_path);
            
            result.success = (result.failed_files == 0);
            
            if (result.success) {
                std::cout << "[VaultEncryptionManager] ✅ Folder decrypted successfully (" 
                         << processed << " files)" << std::endl;
            } else {
                std::cout << "[VaultEncryptionManager] ⚠️ Folder decryption completed with " 
                         << result.failed_files << " failures" << std::endl;
            }
            
        } catch (const std::exception& e) {
            result.error_message = "Decryption failed: " + std::string(e.what());
            std::cerr << "[VaultEncryptionManager] " << result.error_message << std::endl;
        }
        
        return result;
    }

    PasswordVerificationResult verifyPassword(const std::string& password, 
                                             const std::string& hashed_password) {
        try {
            // Parse salt:hash format (compatible with VaultFolderManager.js)
            size_t colon_pos = hashed_password.find(':');
            if (colon_pos == std::string::npos) {
                return PasswordVerificationResult(false, "Invalid hash format");
            }
            
            std::string salt_hex = hashed_password.substr(0, colon_pos);
            std::string expected_hash_hex = hashed_password.substr(colon_pos + 1);
            
            // Convert salt from hex
            std::vector<uint8_t> salt = hexToBytes(salt_hex);
            
            // Hash the provided password with the same salt
            std::string computed_hash = hashPasswordWithSalt(password, salt);
            
            // Extract just the hash part (after the colon)
            size_t computed_colon_pos = computed_hash.find(':');
            if (computed_colon_pos == std::string::npos) {
                return PasswordVerificationResult(false, "Hash computation failed");
            }
            
            std::string computed_hash_hex = computed_hash.substr(computed_colon_pos + 1);
            
            bool is_valid = (computed_hash_hex == expected_hash_hex);
            return PasswordVerificationResult(is_valid);
            
        } catch (const std::exception& e) {
            return PasswordVerificationResult(false, "Password verification error: " + std::string(e.what()));
        }
    }

    std::string hashPassword(const std::string& password, const std::string& salt_hex) {
        std::vector<uint8_t> salt;
        
        if (salt_hex.empty()) {
            // Generate random salt
            salt.resize(32);
            if (RAND_bytes(salt.data(), 32) != 1) {
                throw std::runtime_error("Failed to generate random salt");
            }
        } else {
            // Use provided salt
            salt = hexToBytes(salt_hex);
        }
        
        return hashPasswordWithSalt(password, salt);
    }

    std::vector<uint8_t> deriveKey(const std::string& password, 
                                  const std::vector<uint8_t>& salt) {
        if (!encryption_engine_) {
            throw std::runtime_error("Encryption engine not initialized");
        }
        
        return encryption_engine_->deriveKeyFromPassword(password, salt);
    }

    std::vector<uint8_t> generateSalt() {
        if (!encryption_engine_) {
            throw std::runtime_error("Encryption engine not initialized");
        }
        
        return encryption_engine_->generateSalt();
    }

    bool isFolderEncrypted(const std::filesystem::path& folder_path) {
        try {
            // Check for encryption metadata file
            std::filesystem::path metadata_path = folder_path / ".phantom_vault" / "encryption.meta";
            if (std::filesystem::exists(metadata_path)) {
                return true;
            }
            
            // Check for .enc files
            for (const auto& entry : std::filesystem::recursive_directory_iterator(folder_path)) {
                if (entry.is_regular_file() && entry.path().extension() == ".enc") {
                    return true;
                }
            }
            
            return false;
        } catch (const std::exception& e) {
            std::cerr << "[VaultEncryptionManager] Error checking encryption status: " << e.what() << std::endl;
            return false;
        }
    }

    size_t getFolderFileCount(const std::filesystem::path& folder_path) {
        try {
            size_t count = 0;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(folder_path)) {
                if (entry.is_regular_file()) {
                    // Skip metadata files
                    if (entry.path().filename() != ".phantom_vault_encryption") {
                        count++;
                    }
                }
            }
            return count;
        } catch (const std::exception& e) {
            std::cerr << "[VaultEncryptionManager] Error counting files: " << e.what() << std::endl;
            return 0;
        }
    }

    std::string getLastError() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return last_error_;
    }

private:
    void collectFiles(const std::filesystem::path& folder_path, 
                     std::vector<std::filesystem::path>& files) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(folder_path)) {
            if (entry.is_regular_file()) {
                // Skip already encrypted files and metadata
                std::string filename = entry.path().filename().string();
                if (!filename.ends_with(".enc") && filename != ".phantom_vault_encryption") {
                    files.push_back(entry.path());
                }
            }
        }
    }

    void collectEncryptedFiles(const std::filesystem::path& folder_path, 
                              std::vector<std::filesystem::path>& files) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(folder_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".enc") {
                files.push_back(entry.path());
            }
        }
    }

    bool encryptSingleFile(const std::filesystem::path& file_path, 
                          const std::vector<uint8_t>& key,
                          std::vector<uint8_t>& iv) {
        try {
            // Generate unique IV for this file
            iv = encryption_engine_->generateIV();
            
            // Create encrypted file path
            std::filesystem::path encrypted_path = file_path;
            encrypted_path += ".enc";
            
            // Encrypt the file
            bool success = encryption_engine_->encryptFile(file_path, encrypted_path, key, iv);
            
            if (success) {
                // Remove original file
                std::filesystem::remove(file_path);
                return true;
            }
            
            return false;
        } catch (const std::exception& e) {
            std::cerr << "[VaultEncryptionManager] File encryption error: " << e.what() << std::endl;
            return false;
        }
    }

    bool decryptSingleFile(const std::filesystem::path& encrypted_file_path, 
                          const std::vector<uint8_t>& key,
                          const std::vector<uint8_t>& iv) {
        try {
            // Create decrypted file path (remove .enc extension)
            std::filesystem::path decrypted_path = encrypted_file_path;
            if (decrypted_path.extension() == ".enc") {
                decrypted_path.replace_extension("");
            }
            
            // Decrypt the file using the stored IV
            bool success = encryption_engine_->decryptFile(encrypted_file_path, decrypted_path, key, iv);
            
            if (success) {
                // Remove encrypted file
                std::filesystem::remove(encrypted_file_path);
                return true;
            }
            
            return false;
        } catch (const std::exception& e) {
            std::cerr << "[VaultEncryptionManager] File decryption error: " << e.what() << std::endl;
            return false;
        }
    }

    bool saveFolderEncryptionMetadata(const std::filesystem::path& folder_path, 
                                     const std::vector<uint8_t>& salt,
                                     const std::vector<std::pair<std::string, std::vector<uint8_t>>>& file_ivs) {
        try {
            std::filesystem::path metadata_dir = folder_path / ".phantom_vault";
            std::filesystem::create_directories(metadata_dir);
            
            std::filesystem::path metadata_path = metadata_dir / "encryption.meta";
            
            std::ofstream file(metadata_path, std::ios::binary);
            if (!file.is_open()) {
                return false;
            }
            
            // Write salt
            file.write(reinterpret_cast<const char*>(salt.data()), salt.size());
            
            // Write number of files
            uint64_t file_count = file_ivs.size();
            file.write(reinterpret_cast<const char*>(&file_count), sizeof(file_count));
            
            // Write file paths and IVs
            for (const auto& [file_path, iv] : file_ivs) {
                // Write path length and path
                uint32_t path_len = file_path.length();
                file.write(reinterpret_cast<const char*>(&path_len), sizeof(path_len));
                file.write(file_path.c_str(), path_len);
                
                // Write IV
                file.write(reinterpret_cast<const char*>(iv.data()), iv.size());
            }
            
            file.close();
            
            // Hide the metadata directory
            std::filesystem::permissions(metadata_dir, 
                                       std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec,
                                       std::filesystem::perm_options::replace);
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[VaultEncryptionManager] Failed to save metadata: " << e.what() << std::endl;
            return false;
        }
    }

    bool loadFolderEncryptionMetadata(const std::filesystem::path& folder_path, 
                                     std::vector<uint8_t>& salt,
                                     std::map<std::string, std::vector<uint8_t>>& file_ivs) {
        try {
            std::filesystem::path metadata_path = folder_path / ".phantom_vault" / "encryption.meta";
            
            if (!std::filesystem::exists(metadata_path)) {
                return false;
            }
            
            std::ifstream file(metadata_path, std::ios::binary);
            if (!file.is_open()) {
                return false;
            }
            
            // Read salt
            salt.resize(32); // Standard salt size
            file.read(reinterpret_cast<char*>(salt.data()), salt.size());
            
            // Read number of files
            uint64_t file_count;
            file.read(reinterpret_cast<char*>(&file_count), sizeof(file_count));
            
            // Read file paths and IVs
            for (uint64_t i = 0; i < file_count; i++) {
                // Read path length and path
                uint32_t path_len;
                file.read(reinterpret_cast<char*>(&path_len), sizeof(path_len));
                
                std::string file_path(path_len, '\0');
                file.read(&file_path[0], path_len);
                
                // Read IV
                std::vector<uint8_t> iv(12); // GCM IV size
                file.read(reinterpret_cast<char*>(iv.data()), iv.size());
                
                file_ivs[file_path] = iv;
            }
            
            file.close();
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[VaultEncryptionManager] Failed to load metadata: " << e.what() << std::endl;
            return false;
        }
    }

    void removeFolderEncryptionMetadata(const std::filesystem::path& folder_path) {
        try {
            std::filesystem::path metadata_dir = folder_path / ".phantom_vault";
            if (std::filesystem::exists(metadata_dir)) {
                std::filesystem::remove_all(metadata_dir);
            }
        } catch (const std::exception& e) {
            std::cerr << "[VaultEncryptionManager] Failed to remove metadata: " << e.what() << std::endl;
        }
    }

    std::string hashPasswordWithSalt(const std::string& password, 
                                    const std::vector<uint8_t>& salt) {
        // Use PBKDF2 with 100,000 iterations (same as VaultFolderManager.js)
        const int iterations = 100000;
        const int key_length = 64; // 512 bits
        
        std::vector<uint8_t> hash(key_length);
        
        if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                             salt.data(), salt.size(),
                             iterations, EVP_sha512(),
                             key_length, hash.data()) != 1) {
            throw std::runtime_error("PBKDF2 hash computation failed");
        }
        
        // Convert to hex format: salt:hash
        std::string salt_hex = bytesToHex(salt);
        std::string hash_hex = bytesToHex(hash);
        
        return salt_hex + ":" + hash_hex;
    }

    std::vector<uint8_t> hexToBytes(const std::string& hex) {
        std::vector<uint8_t> bytes;
        bytes.reserve(hex.length() / 2);
        
        for (size_t i = 0; i < hex.length(); i += 2) {
            std::string byte_string = hex.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::strtol(byte_string.c_str(), nullptr, 16));
            bytes.push_back(byte);
        }
        
        return bytes;
    }

    std::string bytesToHex(const std::vector<uint8_t>& bytes) {
        std::string hex;
        hex.reserve(bytes.size() * 2);
        
        for (uint8_t byte : bytes) {
            char hex_chars[3];
            snprintf(hex_chars, sizeof(hex_chars), "%02x", byte);
            hex += hex_chars;
        }
        
        return hex;
    }

    std::unique_ptr<EncryptionEngine> encryption_engine_;
    mutable std::string last_error_;
    mutable std::mutex mutex_;
};

// VaultEncryptionManager public interface implementation
VaultEncryptionManager::VaultEncryptionManager() : pimpl(std::make_unique<Implementation>()) {}
VaultEncryptionManager::~VaultEncryptionManager() = default;

bool VaultEncryptionManager::initialize() {
    return pimpl->initialize();
}

EncryptionResult VaultEncryptionManager::encryptFolder(const std::filesystem::path& folder_path, 
                                                      const std::string& password,
                                                      ProgressCallback progress_callback) {
    return pimpl->encryptFolder(folder_path, password, progress_callback);
}

EncryptionResult VaultEncryptionManager::decryptFolder(const std::filesystem::path& folder_path, 
                                                      const std::string& password,
                                                      ProgressCallback progress_callback) {
    return pimpl->decryptFolder(folder_path, password, progress_callback);
}

PasswordVerificationResult VaultEncryptionManager::verifyPassword(const std::string& password, 
                                                                 const std::string& hashed_password) {
    return pimpl->verifyPassword(password, hashed_password);
}

std::string VaultEncryptionManager::hashPassword(const std::string& password, 
                                                const std::string& salt) {
    return pimpl->hashPassword(password, salt);
}

std::vector<uint8_t> VaultEncryptionManager::deriveKey(const std::string& password, 
                                                      const std::vector<uint8_t>& salt) {
    return pimpl->deriveKey(password, salt);
}

std::vector<uint8_t> VaultEncryptionManager::generateSalt() {
    return pimpl->generateSalt();
}

bool VaultEncryptionManager::isFolderEncrypted(const std::filesystem::path& folder_path) {
    return pimpl->isFolderEncrypted(folder_path);
}

size_t VaultEncryptionManager::getFolderFileCount(const std::filesystem::path& folder_path) {
    return pimpl->getFolderFileCount(folder_path);
}

std::string VaultEncryptionManager::getLastError() const {
    return pimpl->getLastError();
}

} // namespace service
} // namespace phantom_vault