#include "phantom_vault/encryption.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/aes.h>
#include <openssl/kdf.h>
#include <fstream>
#include <stdexcept>
#include <cstring>

namespace phantom_vault {

namespace {
    constexpr size_t KEY_SIZE = 32;  // 256 bits
    constexpr size_t IV_SIZE = 12;   // 96 bits for GCM
    constexpr size_t TAG_SIZE = 16;  // 128 bits
    constexpr size_t SALT_SIZE = 32; // 256 bits
    constexpr size_t BUFFER_SIZE = 4096;

    void handleOpenSSLError(const std::string& operation) {
        unsigned long err = ERR_get_error();
        char err_msg[256];
        ERR_error_string_n(err, err_msg, sizeof(err_msg));
        throw std::runtime_error(operation + " failed: " + err_msg);
    }
}

class EncryptionEngine::Implementation {
public:
    Implementation() : ctx_(nullptr) {
        OpenSSL_add_all_algorithms();
    }

    ~Implementation() {
        if (ctx_) {
            EVP_CIPHER_CTX_free(ctx_);
        }
        EVP_cleanup();
    }

    bool initialize() {
        if (ctx_) {
            EVP_CIPHER_CTX_free(ctx_);
        }
        ctx_ = EVP_CIPHER_CTX_new();
        return ctx_ != nullptr;
    }

    std::vector<uint8_t> generateKey() const {
        std::vector<uint8_t> key(KEY_SIZE);
        if (RAND_bytes(key.data(), KEY_SIZE) != 1) {
            handleOpenSSLError("Key generation");
        }
        return key;
    }

    std::vector<uint8_t> generateIV() const {
        std::vector<uint8_t> iv(IV_SIZE);
        if (RAND_bytes(iv.data(), IV_SIZE) != 1) {
            handleOpenSSLError("IV generation");
        }
        return iv;
    }

    std::vector<uint8_t> encryptData(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv
    ) {
        if (key.size() != KEY_SIZE || iv.size() != IV_SIZE) {
            return std::vector<uint8_t>();
        }

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            return std::vector<uint8_t>();
        }

        // Initialize encryption
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return std::vector<uint8_t>();
        }

        // Allocate output buffer with space for tag
        std::vector<uint8_t> encrypted(data.size() + TAG_SIZE);
        int outlen;

        // Encrypt data
        if (EVP_EncryptUpdate(ctx, encrypted.data(), &outlen, data.data(), data.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return std::vector<uint8_t>();
        }

        int final_len;
        if (EVP_EncryptFinal_ex(ctx, encrypted.data() + outlen, &final_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return std::vector<uint8_t>();
        }

        // Get the tag
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, encrypted.data() + outlen + final_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return std::vector<uint8_t>();
        }

        EVP_CIPHER_CTX_free(ctx);
        encrypted.resize(outlen + final_len + TAG_SIZE);
        return encrypted;
    }

    std::vector<uint8_t> decryptData(
        const std::vector<uint8_t>& encrypted_data,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv
    ) {
        if (key.size() != KEY_SIZE || iv.size() != IV_SIZE || encrypted_data.size() < TAG_SIZE) {
            return std::vector<uint8_t>();
        }

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            return std::vector<uint8_t>();
        }

        // Initialize decryption
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return std::vector<uint8_t>();
        }

        // Extract tag from encrypted data
        std::vector<uint8_t> tag(encrypted_data.end() - TAG_SIZE, encrypted_data.end());
        std::vector<uint8_t> ciphertext(encrypted_data.begin(), encrypted_data.end() - TAG_SIZE);

        // Allocate output buffer
        std::vector<uint8_t> decrypted(ciphertext.size());
        int outlen;

        // Decrypt data
        if (EVP_DecryptUpdate(ctx, decrypted.data(), &outlen, ciphertext.data(), ciphertext.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return std::vector<uint8_t>();
        }

        // Set expected tag value
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE, tag.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return std::vector<uint8_t>();
        }

        int final_len;
        if (EVP_DecryptFinal_ex(ctx, decrypted.data() + outlen, &final_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return std::vector<uint8_t>();
        }

        EVP_CIPHER_CTX_free(ctx);
        decrypted.resize(outlen + final_len);
        return decrypted;
    }

    bool encryptFile(
        const std::filesystem::path& source_path,
        const std::filesystem::path& dest_path,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv
    ) {
        if (key.size() != KEY_SIZE || iv.size() != IV_SIZE) {
            return false;
        }

        std::ifstream source(source_path, std::ios::binary);
        std::ofstream dest(dest_path, std::ios::binary);
        if (!source || !dest) {
            return false;
        }

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            return false;
        }

        // Initialize encryption
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }

        std::vector<uint8_t> buffer(BUFFER_SIZE);
        std::vector<uint8_t> outbuf(BUFFER_SIZE + EVP_MAX_BLOCK_LENGTH);
        int outlen;

        // Process file in chunks
        while (source.good()) {
            source.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
            std::streamsize bytes_read = source.gcount();
            if (bytes_read > 0) {
                if (EVP_EncryptUpdate(ctx, outbuf.data(), &outlen, buffer.data(), bytes_read) != 1) {
                    EVP_CIPHER_CTX_free(ctx);
                    return false;
                }
                dest.write(reinterpret_cast<char*>(outbuf.data()), outlen);
            }
        }

        // Finalize encryption
        if (EVP_EncryptFinal_ex(ctx, outbuf.data(), &outlen) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }
        dest.write(reinterpret_cast<char*>(outbuf.data()), outlen);

        // Get and write the tag
        unsigned char tag[TAG_SIZE];
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }
        dest.write(reinterpret_cast<char*>(tag), TAG_SIZE);

        EVP_CIPHER_CTX_free(ctx);
        return true;
    }

    bool decryptFile(
        const std::filesystem::path& source_path,
        const std::filesystem::path& dest_path,
        const std::vector<uint8_t>& key,
        const std::vector<uint8_t>& iv
    ) {
        if (key.size() != KEY_SIZE || iv.size() != IV_SIZE) {
            return false;
        }

        std::ifstream source(source_path, std::ios::binary | std::ios::ate);
        if (!source) {
            return false;
        }

        std::streamsize file_size = source.tellg();
        if (file_size < TAG_SIZE) {
            return false;
        }

        // Read the tag from the end of the file
        source.seekg(-TAG_SIZE, std::ios::end);
        unsigned char tag[TAG_SIZE];
        source.read(reinterpret_cast<char*>(tag), TAG_SIZE);

        // Reset to beginning for decryption
        source.seekg(0);
        std::ofstream dest(dest_path, std::ios::binary);
        if (!dest) {
            return false;
        }

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            return false;
        }

        // Initialize decryption
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }

        std::vector<uint8_t> buffer(BUFFER_SIZE);
        std::vector<uint8_t> outbuf(BUFFER_SIZE + EVP_MAX_BLOCK_LENGTH);
        int outlen;
        std::streamsize remaining = file_size - TAG_SIZE;

        // Process file in chunks
        while (remaining > 0 && source.good()) {
            std::streamsize to_read = std::min(remaining, static_cast<std::streamsize>(buffer.size()));
            source.read(reinterpret_cast<char*>(buffer.data()), to_read);
            std::streamsize bytes_read = source.gcount();
            if (bytes_read > 0) {
                if (EVP_DecryptUpdate(ctx, outbuf.data(), &outlen, buffer.data(), bytes_read) != 1) {
                    EVP_CIPHER_CTX_free(ctx);
                    return false;
                }
                dest.write(reinterpret_cast<char*>(outbuf.data()), outlen);
                remaining -= bytes_read;
            }
        }

        // Set expected tag value
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE, tag) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }

        // Finalize decryption
        if (EVP_DecryptFinal_ex(ctx, outbuf.data(), &outlen) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }
        dest.write(reinterpret_cast<char*>(outbuf.data()), outlen);

        EVP_CIPHER_CTX_free(ctx);
        return true;
    }

    std::vector<uint8_t> deriveKeyFromPassword(
        const std::string& password,
        const std::vector<uint8_t>& salt
    ) const {
        std::vector<uint8_t> key(KEY_SIZE);
        // Use 100,000 iterations (NIST SP 800-132 recommendation for 2025)
        if (PKCS5_PBKDF2_HMAC(
            password.c_str(),
            password.length(),
            salt.data(),
            salt.size(),
            100000,
            EVP_sha256(),
            KEY_SIZE,
            key.data()
        ) != 1) {
            handleOpenSSLError("Key derivation");
        }
        return key;
    }

    std::vector<uint8_t> generateSalt() const {
        std::vector<uint8_t> salt(SALT_SIZE);
        if (RAND_bytes(salt.data(), SALT_SIZE) != 1) {
            handleOpenSSLError("Salt generation");
        }
        return salt;
    }

private:
    EVP_CIPHER_CTX* ctx_;
};

// Public interface implementation
EncryptionEngine::EncryptionEngine() : pimpl(std::make_unique<Implementation>()) {}
EncryptionEngine::~EncryptionEngine() = default;

bool EncryptionEngine::initialize() {
    return pimpl->initialize();
}

std::vector<uint8_t> EncryptionEngine::generateKey() const {
    return pimpl->generateKey();
}

std::vector<uint8_t> EncryptionEngine::generateIV() const {
    return pimpl->generateIV();
}

std::vector<uint8_t> EncryptionEngine::encryptData(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv
) {
    return pimpl->encryptData(data, key, iv);
}

std::vector<uint8_t> EncryptionEngine::decryptData(
    const std::vector<uint8_t>& encrypted_data,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv
) {
    return pimpl->decryptData(encrypted_data, key, iv);
}

bool EncryptionEngine::encryptFile(
    const std::filesystem::path& source_path,
    const std::filesystem::path& dest_path,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv
) {
    return pimpl->encryptFile(source_path, dest_path, key, iv);
}

bool EncryptionEngine::decryptFile(
    const std::filesystem::path& source_path,
    const std::filesystem::path& dest_path,
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv
) {
    return pimpl->decryptFile(source_path, dest_path, key, iv);
}

std::vector<uint8_t> EncryptionEngine::deriveKeyFromPassword(
    const std::string& password,
    const std::vector<uint8_t>& salt
) const {
    return pimpl->deriveKeyFromPassword(password, salt);
}

std::vector<uint8_t> EncryptionEngine::generateSalt() const {
    return pimpl->generateSalt();
}

} // namespace phantom_vault 