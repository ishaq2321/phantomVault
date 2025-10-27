#include "encryption_engine.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/err.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <sys/stat.h>

#ifdef PLATFORM_LINUX
#include <unistd.h>
#include <sys/types.h>
#endif

namespace PhantomVault {

// OpenSSL context management
class EncryptionEngine::OpenSSLContext {
public:
    OpenSSLContext() {
        // Initialize OpenSSL
        OpenSSL_add_all_algorithms();
        ERR_load_crypto_strings();
    }
    
    ~OpenSSLContext() {
        // Cleanup OpenSSL
        EVP_cleanup();
        ERR_free_strings();
    }
};

EncryptionEngine::EncryptionEngine() 
    : ssl_context_(std::make_unique<OpenSSLContext>()) {
    clearError();
}

EncryptionEngine::~EncryptionEngine() = default;

EncryptionEngine::EncryptionResult EncryptionEngine::encryptFile(
    const std::string& file_path, 
    const std::string& password,
    const KeyDerivationConfig& config) {
    
    clearError();
    EncryptionResult result;
    
    // Read file data
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        setError("Failed to open file: " + file_path);
        result.error_message = last_error_;
        return result;
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (file_size == 0) {
        setError("File is empty: " + file_path);
        result.error_message = last_error_;
        return result;
    }
    
    // Read file data
    std::vector<uint8_t> file_data(file_size);
    file.read(reinterpret_cast<char*>(file_data.data()), file_size);
    file.close();
    
    if (file.gcount() != static_cast<std::streamsize>(file_size)) {
        setError("Failed to read complete file: " + file_path);
        result.error_message = last_error_;
        return result;
    }
    
    // Generate salt and IV
    result.salt = generateSalt(config.salt_length);
    result.iv = generateIV();
    
    if (result.salt.empty() || result.iv.empty()) {
        setError("Failed to generate cryptographic parameters");
        result.error_message = last_error_;
        return result;
    }
    
    // Derive key
    std::vector<uint8_t> key = deriveKey(password, result.salt, config.iterations, config.key_length);
    if (key.empty()) {
        result.error_message = last_error_;
        return result;
    }
    
    // Encrypt data
    result.encrypted_data = encryptData(file_data, key, result.iv);
    
    // Secure cleanup
    secureWipe(file_data);
    secureWipe(key);
    
    if (result.encrypted_data.empty()) {
        result.error_message = last_error_;
        return result;
    }
    
    result.success = true;
    return result;
}

std::vector<uint8_t> EncryptionEngine::decryptFile(
    const std::vector<uint8_t>& encrypted_data,
    const std::string& password,
    const std::vector<uint8_t>& iv,
    const std::vector<uint8_t>& salt,
    const KeyDerivationConfig& config) {
    
    clearError();
    
    // Derive key
    std::vector<uint8_t> key = deriveKey(password, salt, config.iterations, config.key_length);
    if (key.empty()) {
        return {};
    }
    
    // Decrypt data
    std::vector<uint8_t> decrypted_data = decryptData(encrypted_data, key, iv);
    
    // Secure cleanup
    secureWipe(key);
    
    return decrypted_data;
}

std::vector<uint8_t> EncryptionEngine::encryptData(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv) {
    
    clearError();
    
    if (key.size() != AES_KEY_SIZE) {
        setError("Invalid key size for AES-256");
        return {};
    }
    
    if (iv.size() != AES_BLOCK_SIZE) {
        setError("Invalid IV size for AES");
        return {};
    }
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        setError("Failed to create cipher context");
        return {};
    }
    
    std::vector<uint8_t> encrypted_data;
    
    do {
        // Initialize encryption
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
            setError("Failed to initialize encryption");
            break;
        }
        
        // Calculate maximum output size
        size_t max_output_size = data.size() + AES_BLOCK_SIZE;
        encrypted_data.resize(max_output_size);
        
        int len = 0;
        int total_len = 0;
        
        // Encrypt data
        if (EVP_EncryptUpdate(ctx, encrypted_data.data(), &len, data.data(), data.size()) != 1) {
            setError("Failed to encrypt data");
            break;
        }
        total_len += len;
        
        // Finalize encryption (adds padding)
        if (EVP_EncryptFinal_ex(ctx, encrypted_data.data() + total_len, &len) != 1) {
            setError("Failed to finalize encryption");
            break;
        }
        total_len += len;
        
        // Resize to actual encrypted size
        encrypted_data.resize(total_len);
        
    } while (false);
    
    EVP_CIPHER_CTX_free(ctx);
    
    if (!last_error_.empty()) {
        encrypted_data.clear();
    }
    
    return encrypted_data;
}

std::vector<uint8_t> EncryptionEngine::decryptData(
    const std::vector<uint8_t>& encrypted_data,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv) {
    
    clearError();
    
    if (key.size() != AES_KEY_SIZE) {
        setError("Invalid key size for AES-256");
        return {};
    }
    
    if (iv.size() != AES_BLOCK_SIZE) {
        setError("Invalid IV size for AES");
        return {};
    }
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        setError("Failed to create cipher context");
        return {};
    }
    
    std::vector<uint8_t> decrypted_data;
    
    do {
        // Initialize decryption
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
            setError("Failed to initialize decryption");
            break;
        }
        
        // Calculate maximum output size
        decrypted_data.resize(encrypted_data.size() + AES_BLOCK_SIZE);
        
        int len = 0;
        int total_len = 0;
        
        // Decrypt data
        if (EVP_DecryptUpdate(ctx, decrypted_data.data(), &len, 
                             encrypted_data.data(), encrypted_data.size()) != 1) {
            setError("Failed to decrypt data");
            break;
        }
        total_len += len;
        
        // Finalize decryption (removes padding)
        if (EVP_DecryptFinal_ex(ctx, decrypted_data.data() + total_len, &len) != 1) {
            setError("Failed to finalize decryption - invalid padding or wrong key");
            break;
        }
        total_len += len;
        
        // Resize to actual decrypted size
        decrypted_data.resize(total_len);
        
    } while (false);
    
    EVP_CIPHER_CTX_free(ctx);
    
    if (!last_error_.empty()) {
        decrypted_data.clear();
    }
    
    return decrypted_data;
}

std::vector<uint8_t> EncryptionEngine::deriveKey(
    const std::string& password,
    const std::vector<uint8_t>& salt,
    int iterations,
    int key_length) {
    
    clearError();
    
    if (password.empty()) {
        setError("Password cannot be empty");
        return {};
    }
    
    if (salt.empty()) {
        setError("Salt cannot be empty");
        return {};
    }
    
    if (iterations < 10000) {
        setError("Iteration count too low (minimum 10,000)");
        return {};
    }
    
    std::vector<uint8_t> key(key_length);
    
    int result = PKCS5_PBKDF2_HMAC(
        password.c_str(), password.length(),
        salt.data(), salt.size(),
        iterations,
        EVP_sha256(),
        key_length,
        key.data()
    );
    
    if (result != 1) {
        setError("PBKDF2 key derivation failed");
        return {};
    }
    
    return key;
}

std::vector<uint8_t> EncryptionEngine::generateRandomBytes(size_t length) {
    clearError();
    
    if (length == 0) {
        setError("Cannot generate zero-length random data");
        return {};
    }
    
    std::vector<uint8_t> random_data(length);
    
    if (RAND_bytes(random_data.data(), length) != 1) {
        setError("Failed to generate random bytes");
        return {};
    }
    
    return random_data;
}

std::vector<uint8_t> EncryptionEngine::generateSalt(size_t length) {
    return generateRandomBytes(length);
}

std::vector<uint8_t> EncryptionEngine::generateIV() {
    return generateRandomBytes(AES_BLOCK_SIZE);
}

std::string EncryptionEngine::calculateFileChecksum(const std::string& file_path) {
    clearError();
    
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        setError("Failed to open file for checksum: " + file_path);
        return "";
    }
    
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        setError("Failed to create hash context");
        return "";
    }
    
    std::string checksum;
    
    do {
        if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
            setError("Failed to initialize SHA-256");
            break;
        }
        
        char buffer[8192];
        while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
            if (EVP_DigestUpdate(ctx, buffer, file.gcount()) != 1) {
                setError("Failed to update hash");
                break;
            }
        }
        
        if (!last_error_.empty()) break;
        
        unsigned char hash[SHA256_DIGEST_LENGTH];
        unsigned int hash_len;
        
        if (EVP_DigestFinal_ex(ctx, hash, &hash_len) != 1) {
            setError("Failed to finalize hash");
            break;
        }
        
        // Convert to hex string
        std::stringstream ss;
        for (unsigned int i = 0; i < hash_len; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        checksum = ss.str();
        
    } while (false);
    
    EVP_MD_CTX_free(ctx);
    file.close();
    
    return checksum;
}

EncryptionEngine::FileMetadata EncryptionEngine::getFileMetadata(const std::string& file_path) {
    clearError();
    FileMetadata metadata;
    
    struct stat file_stat;
    if (stat(file_path.c_str(), &file_stat) != 0) {
        setError("Failed to get file statistics: " + file_path);
        return metadata;
    }
    
    metadata.original_path = file_path;
    metadata.original_size = file_stat.st_size;
    metadata.created_timestamp = file_stat.st_ctime;
    metadata.modified_timestamp = file_stat.st_mtime;
    metadata.accessed_timestamp = file_stat.st_atime;
    
    // Convert permissions to string
    std::stringstream perm_ss;
    perm_ss << std::oct << (file_stat.st_mode & 0777);
    metadata.original_permissions = perm_ss.str();
    
    // Calculate checksum
    metadata.checksum_sha256 = calculateFileChecksum(file_path);
    
    return metadata;
}

void EncryptionEngine::secureWipe(void* data, size_t size) {
    if (data && size > 0) {
        // Use volatile to prevent compiler optimization
        volatile uint8_t* ptr = static_cast<volatile uint8_t*>(data);
        for (size_t i = 0; i < size; ++i) {
            ptr[i] = 0;
        }
    }
}

void EncryptionEngine::secureWipe(std::vector<uint8_t>& data) {
    if (!data.empty()) {
        secureWipe(data.data(), data.size());
        data.clear();
    }
}

bool EncryptionEngine::selfTest() {
    clearError();
    
    // Test 1: Key derivation
    std::string test_password = "test_password_123";
    std::vector<uint8_t> test_salt = generateSalt(32);
    if (test_salt.empty()) {
        setError("Self-test failed: Cannot generate salt");
        return false;
    }
    
    std::vector<uint8_t> key1 = deriveKey(test_password, test_salt, 10000, 32);
    std::vector<uint8_t> key2 = deriveKey(test_password, test_salt, 10000, 32);
    
    if (key1.empty() || key2.empty() || key1 != key2) {
        setError("Self-test failed: Key derivation inconsistent");
        return false;
    }
    
    // Test 2: Encryption/Decryption
    std::string test_data = "This is a test message for encryption validation.";
    std::vector<uint8_t> data_bytes(test_data.begin(), test_data.end());
    
    std::vector<uint8_t> iv = generateIV();
    if (iv.empty()) {
        setError("Self-test failed: Cannot generate IV");
        return false;
    }
    
    std::vector<uint8_t> encrypted = encryptData(data_bytes, key1, iv);
    if (encrypted.empty()) {
        setError("Self-test failed: Encryption failed");
        return false;
    }
    
    std::vector<uint8_t> decrypted = decryptData(encrypted, key1, iv);
    if (decrypted.empty() || decrypted != data_bytes) {
        setError("Self-test failed: Decryption failed or data mismatch");
        return false;
    }
    
    // Test 3: Random number generation
    std::vector<uint8_t> random1 = generateRandomBytes(32);
    std::vector<uint8_t> random2 = generateRandomBytes(32);
    
    if (random1.empty() || random2.empty() || random1 == random2) {
        setError("Self-test failed: Random number generation failed");
        return false;
    }
    
    // Cleanup
    secureWipe(key1);
    secureWipe(key2);
    
    return true;
}

void EncryptionEngine::setError(const std::string& error) {
    last_error_ = error;
    
    // Also capture OpenSSL errors if available
    unsigned long ssl_error = ERR_get_error();
    if (ssl_error != 0) {
        char ssl_error_buf[256];
        ERR_error_string_n(ssl_error, ssl_error_buf, sizeof(ssl_error_buf));
        last_error_ += " (OpenSSL: " + std::string(ssl_error_buf) + ")";
    }
}

void EncryptionEngine::clearError() {
    last_error_.clear();
    ERR_clear_error();
}

} // namespace PhantomVault