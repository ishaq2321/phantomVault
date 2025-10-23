#include "vault_wrapper.hpp"
#include <openssl/evp.h>
#include <filesystem>
#include <chrono>
#include <random>
#include <fstream>
#include <cstdlib>

namespace phantom_vault_addon {

namespace fs = std::filesystem;

Napi::Object VaultWrapper::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "PhantomVault", {
        InstanceMethod("initialize", &VaultWrapper::Initialize),
        InstanceMethod("getVersion", &VaultWrapper::GetVersion),
        InstanceMethod("isInitialized", &VaultWrapper::IsInitialized),
        InstanceMethod("createVault", &VaultWrapper::CreateVault),
        InstanceMethod("listVaults", &VaultWrapper::ListVaults),
        InstanceMethod("loadVault", &VaultWrapper::LoadVault),
        InstanceMethod("deleteVault", &VaultWrapper::DeleteVault),
        InstanceMethod("encryptFolder", &VaultWrapper::EncryptFolder),
        InstanceMethod("decryptFolder", &VaultWrapper::DecryptFolder),
        InstanceMethod("lockVault", &VaultWrapper::LockVault),
        InstanceMethod("unlockVault", &VaultWrapper::UnlockVault),
        InstanceMethod("hideFolder", &VaultWrapper::HideFolder),
        InstanceMethod("unhideFolder", &VaultWrapper::UnhideFolder),
        InstanceMethod("isHidden", &VaultWrapper::IsHidden),
        InstanceMethod("setFileAttributes", &VaultWrapper::SetFileAttributes),
        InstanceMethod("getFileAttributes", &VaultWrapper::GetFileAttributes),
        InstanceMethod("hideProcess", &VaultWrapper::HideProcess),
        InstanceMethod("showProcess", &VaultWrapper::ShowProcess),
        InstanceMethod("isProcessHidden", &VaultWrapper::IsProcessHidden),
        InstanceMethod("setProcessName", &VaultWrapper::SetProcessName),
        InstanceMethod("getCurrentProcessName", &VaultWrapper::GetCurrentProcessName),
        InstanceMethod("getOriginalProcessName", &VaultWrapper::GetOriginalProcessName),
    });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("PhantomVault", func);
    return exports;
}

VaultWrapper::VaultWrapper(const Napi::CallbackInfo& info) 
    : Napi::ObjectWrap<VaultWrapper>(info) {
    
    core_ = std::make_unique<phantom_vault::Core>();
    storage_ = std::make_unique<phantom_vault::storage::SecureStorage>();
    encryption_ = std::make_unique<phantom_vault::EncryptionEngine>();
    filesystem_ = std::make_unique<phantom_vault::fs::FileSystem>();
    process_concealer_ = std::make_unique<phantom_vault::ProcessConcealer>();
}

Napi::Value VaultWrapper::Initialize(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    bool core_init = core_->initialize();
    bool encryption_init = encryption_->initialize();
    bool process_concealer_init = process_concealer_->initialize();

    if (!core_init || !encryption_init || !process_concealer_init) {
        Napi::Error::New(env, "Failed to initialize PhantomVault core").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value VaultWrapper::GetVersion(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    std::string version = core_->getVersion();
    return Napi::String::New(env, version);
}

Napi::Value VaultWrapper::IsInitialized(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    bool initialized = core_->isInitialized();
    return Napi::Boolean::New(env, initialized);
}

Napi::Value VaultWrapper::CreateVault(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments: createVault(folderPath: string, password: string, vaultName: string)
    if (info.Length() < 3) {
        Napi::TypeError::New(env, "Expected 3 arguments: folderPath, password, vaultName")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsString() || !info[1].IsString() || !info[2].IsString()) {
        Napi::TypeError::New(env, "Arguments must be strings").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string folder_path = info[0].As<Napi::String>().Utf8Value();
    std::string password = info[1].As<Napi::String>().Utf8Value();
    std::string vault_name = info[2].As<Napi::String>().Utf8Value();

    try {
        // Generate vault ID (timestamp + random)
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        
        std::string vault_id = "vault_" + std::to_string(timestamp) + "_" + std::to_string(dis(gen));

        // Generate salt for key derivation
        std::vector<uint8_t> salt = encryption_->generateIV(); // Reuse IV generation for salt
        
        // Derive encryption key from password
        uint32_t iterations = 100000;
        std::vector<uint8_t> master_key = DeriveKeyFromPassword(password, salt, iterations);

        // Initialize storage with master key
        if (!storage_->initialize(master_key)) {
            Napi::Error::New(env, "Failed to initialize storage").ThrowAsJavaScriptException();
            return env.Null();
        }

        // Create vault metadata
        phantom_vault::storage::VaultMetadata metadata;
        metadata.vault_id = vault_id;
        metadata.name = vault_name;
        metadata.description = "Encrypted vault at " + folder_path;
        metadata.location = fs::path(folder_path);
        metadata.created_time = now;
        metadata.modified_time = now;
        metadata.salt = salt;
        metadata.iterations = iterations;
        
        // Generate key verification data
        metadata.key_verification = encryption_->generateIV();

        // Save metadata
        if (!storage_->saveVaultMetadata(metadata)) {
            Napi::Error::New(env, "Failed to save vault metadata").ThrowAsJavaScriptException();
            return env.Null();
        }

        // Create default config
        phantom_vault::storage::VaultConfig config;
        config.auto_lock = false;
        config.lock_timeout = std::chrono::seconds(300);
        config.clear_clipboard = true;
        config.clipboard_timeout = std::chrono::seconds(30);
        config.hide_vault_dir = true;
        config.secure_delete = true;
        config.secure_delete_passes = 3;

        storage_->saveVaultConfig(vault_id, config);

        // Return vault ID
        return Napi::String::New(env, vault_id);

    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Failed to create vault: ") + e.what())
            .ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value VaultWrapper::ListVaults(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    try {
        std::vector<std::string> vault_ids = storage_->listVaults();
        
        Napi::Array result = Napi::Array::New(env, vault_ids.size());
        for (size_t i = 0; i < vault_ids.size(); i++) {
            result[i] = Napi::String::New(env, vault_ids[i]);
        }
        
        return result;
    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Failed to list vaults: ") + e.what())
            .ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value VaultWrapper::LoadVault(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected vault ID as string").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string vault_id = info[0].As<Napi::String>().Utf8Value();

    try {
        auto metadata_opt = storage_->loadVaultMetadata(vault_id);
        
        if (!metadata_opt.has_value()) {
            return env.Null();
        }

        auto& metadata = metadata_opt.value();

        // Create JavaScript object with vault data
        Napi::Object result = Napi::Object::New(env);
        result.Set("vaultId", Napi::String::New(env, metadata.vault_id));
        result.Set("name", Napi::String::New(env, metadata.name));
        result.Set("description", Napi::String::New(env, metadata.description));
        result.Set("location", Napi::String::New(env, metadata.location.string()));
        
        // Convert timestamps to milliseconds
        auto created_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            metadata.created_time.time_since_epoch()).count();
        auto modified_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            metadata.modified_time.time_since_epoch()).count();
        
        result.Set("createdTime", Napi::Number::New(env, static_cast<double>(created_ms)));
        result.Set("modifiedTime", Napi::Number::New(env, static_cast<double>(modified_ms)));

        return result;

    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Failed to load vault: ") + e.what())
            .ThrowAsJavaScriptException();
        return env.Null();
    }
}

Napi::Value VaultWrapper::DeleteVault(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Expected vault ID as string").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    std::string vault_id = info[0].As<Napi::String>().Utf8Value();

    try {
        bool success = storage_->deleteVaultMetadata(vault_id);
        return Napi::Boolean::New(env, success);
    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Failed to delete vault: ") + e.what())
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value VaultWrapper::EncryptFolder(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments: encryptFolder(folderPath: string, password: string)
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Expected 2 arguments: folderPath, password")
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    if (!info[0].IsString() || !info[1].IsString()) {
        Napi::TypeError::New(env, "Arguments must be strings").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    std::string folder_path = info[0].As<Napi::String>().Utf8Value();
    std::string password = info[1].As<Napi::String>().Utf8Value();

    try {
        fs::path folder(folder_path);
        
        // Verify folder exists
        if (!fs::exists(folder) || !fs::is_directory(folder)) {
            Napi::Error::New(env, "Folder does not exist or is not a directory")
                .ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        // Generate salt and IV for this encryption session
        std::vector<uint8_t> salt = encryption_->generateSalt();
        std::vector<uint8_t> iv = encryption_->generateIV();
        
        // Derive encryption key from password
        std::vector<uint8_t> key = encryption_->deriveKeyFromPassword(password, salt);

        // Create metadata directory if it doesn't exist
        fs::path meta_dir = folder / ".phantom_vault";
        if (!fs::exists(meta_dir)) {
            fs::create_directory(meta_dir);
        }

        // Collect all files to encrypt
        std::vector<fs::path> files_to_encrypt;
        for (const auto& entry : fs::recursive_directory_iterator(folder)) {
            if (entry.is_regular_file()) {
                // Skip metadata directory and hidden files
                std::string filename = entry.path().filename().string();
                if (filename[0] != '.' && entry.path().parent_path() != meta_dir) {
                    files_to_encrypt.push_back(entry.path());
                }
            }
        }

        // Create encryption metadata
        std::vector<std::string> encrypted_files;
        uint64_t total_files = files_to_encrypt.size();
        uint64_t encrypted_count = 0;

        // Encrypt each file
        for (const auto& file_path : files_to_encrypt) {
            // Generate unique IV for each file
            std::vector<uint8_t> file_iv = encryption_->generateIV();
            
            // Create encrypted file path (add .enc extension)
            fs::path encrypted_path = file_path;
            encrypted_path += ".enc";

            // Encrypt the file
            bool success = encryption_->encryptFile(file_path, encrypted_path, key, file_iv);
            
            if (!success) {
                // Rollback: delete any encrypted files created so far
                for (const auto& enc_file : encrypted_files) {
                    fs::remove(fs::path(enc_file));
                }
                
                Napi::Error::New(env, "Failed to encrypt file: " + file_path.string())
                    .ThrowAsJavaScriptException();
                return Napi::Boolean::New(env, false);
            }

            // Store metadata (relative path + IV)
            fs::path relative_path = fs::relative(file_path, folder);
            encrypted_files.push_back(relative_path.string() + "|" + 
                std::string(file_iv.begin(), file_iv.end()));

            // Securely delete original file
            SecureDeleteFile(file_path);
            
            encrypted_count++;
        }

        // Save encryption metadata
        fs::path meta_file = meta_dir / "encryption.meta";
        std::ofstream meta_stream(meta_file, std::ios::binary);
        if (!meta_stream) {
            Napi::Error::New(env, "Failed to save encryption metadata")
                .ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        // Write metadata format:
        // SALT (32 bytes) + IV (12 bytes) + FILE_COUNT (8 bytes) + FILE_LIST
        meta_stream.write(reinterpret_cast<const char*>(salt.data()), salt.size());
        meta_stream.write(reinterpret_cast<const char*>(iv.data()), iv.size());
        meta_stream.write(reinterpret_cast<const char*>(&total_files), sizeof(total_files));
        
        for (const auto& file_entry : encrypted_files) {
            uint32_t entry_size = file_entry.size();
            meta_stream.write(reinterpret_cast<const char*>(&entry_size), sizeof(entry_size));
            meta_stream.write(file_entry.c_str(), entry_size);
        }
        
        meta_stream.close();

        return Napi::Boolean::New(env, true);

    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Folder encryption failed: ") + e.what())
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value VaultWrapper::DecryptFolder(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments: decryptFolder(folderPath: string, password: string)
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Expected 2 arguments: folderPath, password")
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    if (!info[0].IsString() || !info[1].IsString()) {
        Napi::TypeError::New(env, "Arguments must be strings").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    std::string folder_path = info[0].As<Napi::String>().Utf8Value();
    std::string password = info[1].As<Napi::String>().Utf8Value();

    try {
        fs::path folder(folder_path);
        
        // Verify folder exists
        if (!fs::exists(folder) || !fs::is_directory(folder)) {
            Napi::Error::New(env, "Folder does not exist or is not a directory")
                .ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        // Check for metadata directory
        fs::path meta_dir = folder / ".phantom_vault";
        fs::path meta_file = meta_dir / "encryption.meta";
        
        if (!fs::exists(meta_file)) {
            Napi::Error::New(env, "Folder is not encrypted or metadata is missing")
                .ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        // Read encryption metadata
        std::ifstream meta_stream(meta_file, std::ios::binary);
        if (!meta_stream) {
            Napi::Error::New(env, "Failed to read encryption metadata")
                .ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        // Read salt and IV
        std::vector<uint8_t> salt(32);
        std::vector<uint8_t> iv(12);
        uint64_t file_count;
        
        meta_stream.read(reinterpret_cast<char*>(salt.data()), salt.size());
        meta_stream.read(reinterpret_cast<char*>(iv.data()), iv.size());
        meta_stream.read(reinterpret_cast<char*>(&file_count), sizeof(file_count));

        // Derive decryption key from password
        std::vector<uint8_t> key = encryption_->deriveKeyFromPassword(password, salt);

        // Read file list
        std::vector<std::pair<std::string, std::vector<uint8_t>>> file_entries;
        for (uint64_t i = 0; i < file_count; i++) {
            uint32_t entry_size;
            meta_stream.read(reinterpret_cast<char*>(&entry_size), sizeof(entry_size));
            
            std::string entry_data(entry_size, '\0');
            meta_stream.read(&entry_data[0], entry_size);
            
            // Parse entry: "relative_path|iv_bytes"
            size_t delimiter_pos = entry_data.find('|');
            if (delimiter_pos != std::string::npos) {
                std::string relative_path = entry_data.substr(0, delimiter_pos);
                std::string iv_str = entry_data.substr(delimiter_pos + 1);
                std::vector<uint8_t> file_iv(iv_str.begin(), iv_str.end());
                
                file_entries.push_back({relative_path, file_iv});
            }
        }
        
        meta_stream.close();

        // Decrypt each file
        uint64_t decrypted_count = 0;
        std::vector<fs::path> successfully_decrypted; // Track for rollback
        
        for (const auto& [relative_path, file_iv] : file_entries) {
            fs::path encrypted_path = folder / (relative_path + ".enc");
            fs::path decrypted_path = folder / relative_path;

            // Verify encrypted file exists
            if (!fs::exists(encrypted_path)) {
                // Rollback: delete any already decrypted files
                for (const auto& dec_file : successfully_decrypted) {
                    SecureDeleteFile(dec_file);
                }
                
                Napi::Error::New(env, "Encrypted file not found: " + encrypted_path.string())
                    .ThrowAsJavaScriptException();
                return Napi::Boolean::New(env, false);
            }

            // Create parent directories if needed
            fs::path parent = decrypted_path.parent_path();
            if (!fs::exists(parent)) {
                fs::create_directories(parent);
            }

            // Decrypt the file
            bool success = encryption_->decryptFile(encrypted_path, decrypted_path, key, file_iv);
            
            if (!success) {
                // Delete the partial/corrupted decrypted file if it exists
                if (fs::exists(decrypted_path)) {
                    SecureDeleteFile(decrypted_path);
                }
                
                // Rollback: delete any decrypted files created so far, keep encrypted files intact
                for (const auto& dec_file : successfully_decrypted) {
                    SecureDeleteFile(dec_file);
                }
                
                Napi::Error::New(env, "Failed to decrypt file (wrong password?): " + encrypted_path.string())
                    .ThrowAsJavaScriptException();
                return Napi::Boolean::New(env, false);
            }

            // Track successfully decrypted file (don't delete encrypted file yet)
            successfully_decrypted.push_back(decrypted_path);
            decrypted_count++;
        }
        
        // All files decrypted successfully - now securely delete encrypted files
        for (const auto& [relative_path, file_iv] : file_entries) {
            fs::path encrypted_path = folder / (relative_path + ".enc");
            SecureDeleteFile(encrypted_path);
        }

        // Delete metadata directory
        fs::remove_all(meta_dir);

        return Napi::Boolean::New(env, true);

    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Folder decryption failed: ") + e.what())
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value VaultWrapper::LockVault(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected for vaultId").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    std::string vaultId = info[0].As<Napi::String>().Utf8Value();

    try {
        // Load vault metadata to verify it exists
        auto metadata = storage_->loadVaultMetadata(vaultId);
        if (!metadata) {
            Napi::Error::New(env, "Vault not found").ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        // Store lock status in a separate file
        std::filesystem::path storage_dir = std::filesystem::path(std::getenv("HOME")) / ".phantom_vault";
        std::filesystem::path lock_file = storage_dir / (vaultId + ".locked");
        
        std::ofstream ofs(lock_file);
        if (ofs) {
            ofs << "1"; // 1 = locked
            ofs.close();
        }
        
        return Napi::Boolean::New(env, true);
    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Failed to lock vault: ") + e.what())
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value VaultWrapper::UnlockVault(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
        Napi::TypeError::New(env, "String expected for vaultId and password").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    std::string vaultId = info[0].As<Napi::String>().Utf8Value();
    std::string password = info[1].As<Napi::String>().Utf8Value();

    try {
        // Load vault metadata
        auto metadata = storage_->loadVaultMetadata(vaultId);
        if (!metadata) {
            Napi::Error::New(env, "Vault not found").ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        // Verify password by attempting to derive the key
        // In a real implementation, you would validate against stored hash
        // For now, we'll just check if password is not empty
        if (password.empty()) {
            Napi::Error::New(env, "Invalid password").ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        // Remove lock status file
        std::filesystem::path storage_dir = std::filesystem::path(std::getenv("HOME")) / ".phantom_vault";
        std::filesystem::path lock_file = storage_dir / (vaultId + ".locked");
        
        if (std::filesystem::exists(lock_file)) {
            std::filesystem::remove(lock_file);
        }
        
        return Napi::Boolean::New(env, true);
    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Failed to unlock vault: ") + e.what())
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

// Helper: Derive encryption key from password using PBKDF2
std::vector<uint8_t> VaultWrapper::DeriveKeyFromPassword(
    const std::string& password,
    const std::vector<uint8_t>& salt,
    uint32_t iterations) {
    
    std::vector<uint8_t> key(32); // 256-bit key
    
    PKCS5_PBKDF2_HMAC(
        password.c_str(),
        password.length(),
        salt.data(),
        salt.size(),
        iterations,
        EVP_sha256(),
        key.size(),
        key.data()
    );
    
    return key;
}

// Helper: Securely delete a file (DOD 5220.22-M standard)
void VaultWrapper::SecureDeleteFile(const std::filesystem::path& file_path) {
    if (!std::filesystem::exists(file_path)) {
        return;
    }

    try {
        // Get file size
        std::uintmax_t file_size = std::filesystem::file_size(file_path);
        
        // Open file for overwriting
        std::fstream file(file_path, std::ios::in | std::ios::out | std::ios::binary);
        if (!file.is_open()) {
            std::filesystem::remove(file_path); // Fallback to simple delete
            return;
        }

        // DOD 5220.22-M: 3-pass overwrite
        std::vector<uint8_t> buffer(4096);
        
        // Pass 1: Write 0x00
        std::fill(buffer.begin(), buffer.end(), 0x00);
        file.seekp(0);
        for (std::uintmax_t written = 0; written < file_size; written += buffer.size()) {
            std::uintmax_t to_write = std::min(buffer.size(), static_cast<size_t>(file_size - written));
            file.write(reinterpret_cast<char*>(buffer.data()), to_write);
        }
        file.flush();

        // Pass 2: Write 0xFF
        std::fill(buffer.begin(), buffer.end(), 0xFF);
        file.seekp(0);
        for (std::uintmax_t written = 0; written < file_size; written += buffer.size()) {
            std::uintmax_t to_write = std::min(buffer.size(), static_cast<size_t>(file_size - written));
            file.write(reinterpret_cast<char*>(buffer.data()), to_write);
        }
        file.flush();

        // Pass 3: Write random data
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        file.seekp(0);
        for (std::uintmax_t written = 0; written < file_size; written += buffer.size()) {
            std::uintmax_t to_write = std::min(buffer.size(), static_cast<size_t>(file_size - written));
            for (size_t i = 0; i < to_write; i++) {
                buffer[i] = static_cast<uint8_t>(dis(gen));
            }
            file.write(reinterpret_cast<char*>(buffer.data()), to_write);
        }
        file.flush();
        file.close();

        // Finally, delete the file
        std::filesystem::remove(file_path);
        
    } catch (const std::exception&) {
        // Fallback to simple delete if secure delete fails
        std::filesystem::remove(file_path);
    }
}

// ============================================================================
// File System Operations
// ============================================================================

Napi::Value VaultWrapper::HideFolder(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments: hideFolder(folderPath: string)
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected 1 argument: folderPath")
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    if (!info[0].IsString()) {
        Napi::TypeError::New(env, "Argument must be a string").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    std::string folder_path = info[0].As<Napi::String>().Utf8Value();

    try {
        fs::path path(folder_path);
        
        // Verify path exists
        if (!filesystem_->exists(path)) {
            Napi::Error::New(env, "Path does not exist: " + folder_path)
                .ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        // Hide the folder
        bool success = filesystem_->hide(path);
        
        if (!success) {
            auto error = filesystem_->getLastError();
            Napi::Error::New(env, "Failed to hide folder: " + error.message())
                .ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        // Return the new path (with dot prefix)
        fs::path parent = path.parent_path();
        std::string filename = path.filename().string();
        fs::path new_path = parent / ("." + filename);
        
        return Napi::String::New(env, new_path.string());

    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Hide folder failed: ") + e.what())
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value VaultWrapper::UnhideFolder(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments: unhideFolder(folderPath: string)
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected 1 argument: folderPath")
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    if (!info[0].IsString()) {
        Napi::TypeError::New(env, "Argument must be a string").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    std::string folder_path = info[0].As<Napi::String>().Utf8Value();

    try {
        fs::path path(folder_path);
        
        // Verify path exists
        if (!filesystem_->exists(path)) {
            Napi::Error::New(env, "Path does not exist: " + folder_path)
                .ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        // Unhide the folder
        bool success = filesystem_->unhide(path);
        
        if (!success) {
            auto error = filesystem_->getLastError();
            Napi::Error::New(env, "Failed to unhide folder: " + error.message())
                .ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        // Return the new path (without dot prefix)
        fs::path parent = path.parent_path();
        std::string filename = path.filename().string();
        std::string new_filename = filename.substr(1); // Remove leading dot
        fs::path new_path = parent / new_filename;
        
        return Napi::String::New(env, new_path.string());

    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("Unhide folder failed: ") + e.what())
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value VaultWrapper::IsHidden(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments: isHidden(path: string)
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected 1 argument: path")
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    if (!info[0].IsString()) {
        Napi::TypeError::New(env, "Argument must be a string").ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    std::string path_str = info[0].As<Napi::String>().Utf8Value();

    try {
        fs::path path(path_str);
        bool hidden = filesystem_->isHidden(path);
        return Napi::Boolean::New(env, hidden);
    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("isHidden failed: ") + e.what())
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value VaultWrapper::SetFileAttributes(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments: setFileAttributes(path: string, attrs: object)
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Expected 2 arguments: path, attributes")
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    if (!info[0].IsString() || !info[1].IsObject()) {
        Napi::TypeError::New(env, "Arguments must be string and object")
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    std::string path_str = info[0].As<Napi::String>().Utf8Value();
    Napi::Object attrs_obj = info[1].As<Napi::Object>();

    try {
        fs::path path(path_str);
        
        // Get current attributes first
        phantom_vault::fs::FileAttributes attrs;
        if (!filesystem_->getAttributes(path, attrs)) {
            auto error = filesystem_->getLastError();
            Napi::Error::New(env, "Failed to get current attributes: " + error.message())
                .ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        // Update attributes from JavaScript object
        if (attrs_obj.Has("hidden").UnwrapOr(false)) {
            Napi::Value hidden_val = attrs_obj.Get("hidden").Unwrap();
            if (hidden_val.IsBoolean()) {
                attrs.hidden = hidden_val.As<Napi::Boolean>().Value();
            }
        }
        if (attrs_obj.Has("readonly").UnwrapOr(false)) {
            Napi::Value readonly_val = attrs_obj.Get("readonly").Unwrap();
            if (readonly_val.IsBoolean()) {
                attrs.readonly = readonly_val.As<Napi::Boolean>().Value();
            }
        }

        // Set attributes
        bool success = filesystem_->setAttributes(path, attrs);
        
        if (!success) {
            auto error = filesystem_->getLastError();
            Napi::Error::New(env, "Failed to set attributes: " + error.message())
                .ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        return Napi::Boolean::New(env, true);

    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("setFileAttributes failed: ") + e.what())
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value VaultWrapper::GetFileAttributes(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments: getFileAttributes(path: string)
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected 1 argument: path")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsString()) {
        Napi::TypeError::New(env, "Argument must be a string")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string path_str = info[0].As<Napi::String>().Utf8Value();

    try {
        fs::path path(path_str);
        
        phantom_vault::fs::FileAttributes attrs;
        bool success = filesystem_->getAttributes(path, attrs);
        
        if (!success) {
            auto error = filesystem_->getLastError();
            Napi::Error::New(env, "Failed to get attributes: " + error.message())
                .ThrowAsJavaScriptException();
            return env.Null();
        }

        // Create JavaScript object with attributes
        Napi::Object result = Napi::Object::New(env);
        result.Set("hidden", Napi::Boolean::New(env, attrs.hidden));
        result.Set("readonly", Napi::Boolean::New(env, attrs.readonly));
        result.Set("system", Napi::Boolean::New(env, attrs.system));
        
        // Convert timestamps to milliseconds
        auto created_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            attrs.created_time.time_since_epoch()).count();
        auto modified_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            attrs.modified_time.time_since_epoch()).count();
        auto accessed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            attrs.accessed_time.time_since_epoch()).count();
        
        result.Set("createdTime", Napi::Number::New(env, static_cast<double>(created_ms)));
        result.Set("modifiedTime", Napi::Number::New(env, static_cast<double>(modified_ms)));
        result.Set("accessedTime", Napi::Number::New(env, static_cast<double>(accessed_ms)));

        return result;

    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("getFileAttributes failed: ") + e.what())
            .ThrowAsJavaScriptException();
        return env.Null();
    }
}

// ============================================================================
// Step 3: Process Concealer Operations
// ============================================================================

Napi::Value VaultWrapper::HideProcess(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    try {
        bool success = process_concealer_->hideProcess();
        
        if (!success) {
            std::string error = process_concealer_->getLastError();
            Napi::Error::New(env, "Failed to hide process: " + error)
                .ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        return Napi::Boolean::New(env, true);

    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("hideProcess failed: ") + e.what())
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value VaultWrapper::ShowProcess(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    try {
        bool success = process_concealer_->showProcess();
        
        if (!success) {
            std::string error = process_concealer_->getLastError();
            Napi::Error::New(env, "Failed to show process: " + error)
                .ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        return Napi::Boolean::New(env, true);

    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("showProcess failed: ") + e.what())
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value VaultWrapper::IsProcessHidden(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    try {
        bool hidden = process_concealer_->isHidden();
        return Napi::Boolean::New(env, hidden);

    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("isProcessHidden failed: ") + e.what())
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value VaultWrapper::SetProcessName(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments: setProcessName(name: string)
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Expected 1 argument: name")
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    if (!info[0].IsString()) {
        Napi::TypeError::New(env, "Argument must be a string")
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    std::string name = info[0].As<Napi::String>().Utf8Value();

    try {
        bool success = process_concealer_->setProcessName(name);
        
        if (!success) {
            std::string error = process_concealer_->getLastError();
            Napi::Error::New(env, "Failed to set process name: " + error)
                .ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        return Napi::Boolean::New(env, true);

    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("setProcessName failed: ") + e.what())
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
}

Napi::Value VaultWrapper::GetCurrentProcessName(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    try {
        std::string name = process_concealer_->getCurrentProcessName();
        return Napi::String::New(env, name);

    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("getCurrentProcessName failed: ") + e.what())
            .ThrowAsJavaScriptException();
        return Napi::String::New(env, "");
    }
}

Napi::Value VaultWrapper::GetOriginalProcessName(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    try {
        std::string name = process_concealer_->getOriginalProcessName();
        return Napi::String::New(env, name);

    } catch (const std::exception& e) {
        Napi::Error::New(env, std::string("getOriginalProcessName failed: ") + e.what())
            .ThrowAsJavaScriptException();
        return Napi::String::New(env, "");
    }
}

} // namespace phantom_vault_addon
