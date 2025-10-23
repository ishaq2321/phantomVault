#pragma once

#include <napi.h>
#include <phantom_vault/core.hpp>
#include <phantom_vault/storage.hpp>
#include <phantom_vault/encryption.hpp>
#include <phantom_vault/filesystem.hpp>
#include <phantom_vault/process_concealer.hpp>
#include <memory>
#include <string>

namespace phantom_vault_addon {

/**
 * @brief Wrapper class that exposes PhantomVault C++ API to Node.js
 */
class VaultWrapper : public Napi::ObjectWrap<VaultWrapper> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    VaultWrapper(const Napi::CallbackInfo& info);

private:
    // Core instances
    std::unique_ptr<phantom_vault::Core> core_;
    std::unique_ptr<phantom_vault::storage::SecureStorage> storage_;
    std::unique_ptr<phantom_vault::EncryptionEngine> encryption_;
    std::unique_ptr<phantom_vault::fs::FileSystem> filesystem_;
    std::unique_ptr<phantom_vault::ProcessConcealer> process_concealer_;

    // Initialization
    Napi::Value Initialize(const Napi::CallbackInfo& info);
    Napi::Value GetVersion(const Napi::CallbackInfo& info);
    Napi::Value IsInitialized(const Napi::CallbackInfo& info);

    // Vault operations
    Napi::Value CreateVault(const Napi::CallbackInfo& info);
    Napi::Value ListVaults(const Napi::CallbackInfo& info);
    Napi::Value LoadVault(const Napi::CallbackInfo& info);
    Napi::Value DeleteVault(const Napi::CallbackInfo& info);

    // Encryption operations
    Napi::Value EncryptFolder(const Napi::CallbackInfo& info);
    Napi::Value DecryptFolder(const Napi::CallbackInfo& info);
    Napi::Value LockVault(const Napi::CallbackInfo& info);
    Napi::Value UnlockVault(const Napi::CallbackInfo& info);

    // File system operations
    Napi::Value HideFolder(const Napi::CallbackInfo& info);
    Napi::Value UnhideFolder(const Napi::CallbackInfo& info);
    Napi::Value IsHidden(const Napi::CallbackInfo& info);
    Napi::Value SetFileAttributes(const Napi::CallbackInfo& info);
    Napi::Value GetFileAttributes(const Napi::CallbackInfo& info);
    
    // Process concealer operations
    Napi::Value HideProcess(const Napi::CallbackInfo& info);
    Napi::Value ShowProcess(const Napi::CallbackInfo& info);
    Napi::Value IsProcessHidden(const Napi::CallbackInfo& info);
    Napi::Value SetProcessName(const Napi::CallbackInfo& info);
    Napi::Value GetCurrentProcessName(const Napi::CallbackInfo& info);
    Napi::Value GetOriginalProcessName(const Napi::CallbackInfo& info);

    // Helper methods
    std::vector<uint8_t> DeriveKeyFromPassword(const std::string& password, 
                                                const std::vector<uint8_t>& salt,
                                                uint32_t iterations);
    void SecureDeleteFile(const std::filesystem::path& file_path);
};

} // namespace phantom_vault_addon
