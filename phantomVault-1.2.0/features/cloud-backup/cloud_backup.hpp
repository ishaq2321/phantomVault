#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

namespace phantom_vault::cloud {

/**
 * @brief Cloud backup provider interface
 */
class PHANTOM_VAULT_EXPORT CloudProvider {
public:
    virtual ~CloudProvider() = default;
    
    // Authentication
    virtual bool authenticate(const std::string& credentials) = 0;
    virtual bool isAuthenticated() const = 0;
    virtual void logout() = 0;
    
    // Backup operations
    virtual bool uploadVault(const std::string& vaultId, const std::vector<uint8_t>& encryptedData) = 0;
    virtual bool downloadVault(const std::string& vaultId, std::vector<uint8_t>& encryptedData) = 0;
    virtual bool deleteVault(const std::string& vaultId) = 0;
    virtual bool vaultExists(const std::string& vaultId) = 0;
    
    // Metadata operations
    virtual std::vector<std::string> listVaults() = 0;
    virtual std::chrono::system_clock::time_point getLastModified(const std::string& vaultId) = 0;
    virtual size_t getVaultSize(const std::string& vaultId) = 0;
    
    // Sync operations
    virtual bool syncVault(const std::string& vaultId) = 0;
    virtual bool isVaultSynced(const std::string& vaultId) = 0;
};

/**
 * @brief Cloud backup configuration
 */
struct PHANTOM_VAULT_EXPORT CloudConfig {
    std::string provider;                    // "aws_s3", "google_drive", "dropbox", "onedrive"
    std::string credentials;                 // Encrypted credentials
    std::string bucket_name;                 // Cloud storage bucket/container name
    std::string region;                      // Cloud region
    bool auto_sync;                          // Enable automatic synchronization
    std::chrono::minutes sync_interval;      // Sync interval
    bool encrypt_before_upload;              // Additional encryption layer
    std::string encryption_key;              // Cloud-specific encryption key
    size_t max_upload_size;                  // Maximum upload size in bytes
    int retry_attempts;                      // Number of retry attempts
    std::chrono::seconds timeout;            // Request timeout
};

/**
 * @brief Cloud backup status
 */
enum class PHANTOM_VAULT_EXPORT BackupStatus {
    NotConfigured,
    Configured,
    Syncing,
    Synced,
    Error,
    Offline
};

/**
 * @brief Cloud backup manager
 */
class PHANTOM_VAULT_EXPORT CloudBackupManager {
public:
    CloudBackupManager();
    ~CloudBackupManager();
    
    // Configuration
    bool configure(const CloudConfig& config);
    bool isConfigured() const;
    CloudConfig getConfiguration() const;
    
    // Provider management
    bool setProvider(const std::string& providerType);
    std::shared_ptr<CloudProvider> getProvider() const;
    
    // Backup operations
    bool backupVault(const std::string& vaultId);
    bool restoreVault(const std::string& vaultId);
    bool deleteBackup(const std::string& vaultId);
    
    // Sync operations
    bool syncAllVaults();
    bool syncVault(const std::string& vaultId);
    bool isVaultSynced(const std::string& vaultId) const;
    
    // Status and monitoring
    BackupStatus getStatus() const;
    std::vector<std::string> getBackedUpVaults() const;
    std::chrono::system_clock::time_point getLastSyncTime() const;
    
    // Event callbacks
    void setProgressCallback(std::function<void(const std::string&, int)> callback);
    void setStatusCallback(std::function<void(BackupStatus)> callback);
    void setErrorCallback(std::function<void(const std::string&)> callback);
    
    // Utility functions
    bool testConnection();
    size_t getTotalBackupSize() const;
    bool cleanupOldBackups(int daysToKeep = 30);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief AWS S3 cloud provider implementation
 */
class PHANTOM_VAULT_EXPORT AWSS3Provider : public CloudProvider {
public:
    AWSS3Provider(const std::string& accessKey, const std::string& secretKey, 
                  const std::string& bucketName, const std::string& region);
    
    bool authenticate(const std::string& credentials) override;
    bool isAuthenticated() const override;
    void logout() override;
    
    bool uploadVault(const std::string& vaultId, const std::vector<uint8_t>& encryptedData) override;
    bool downloadVault(const std::string& vaultId, std::vector<uint8_t>& encryptedData) override;
    bool deleteVault(const std::string& vaultId) override;
    bool vaultExists(const std::string& vaultId) override;
    
    std::vector<std::string> listVaults() override;
    std::chrono::system_clock::time_point getLastModified(const std::string& vaultId) override;
    size_t getVaultSize(const std::string& vaultId) override;
    
    bool syncVault(const std::string& vaultId) override;
    bool isVaultSynced(const std::string& vaultId) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Google Drive cloud provider implementation
 */
class PHANTOM_VAULT_EXPORT GoogleDriveProvider : public CloudProvider {
public:
    GoogleDriveProvider(const std::string& clientId, const std::string& clientSecret,
                       const std::string& refreshToken);
    
    bool authenticate(const std::string& credentials) override;
    bool isAuthenticated() const override;
    void logout() override;
    
    bool uploadVault(const std::string& vaultId, const std::vector<uint8_t>& encryptedData) override;
    bool downloadVault(const std::string& vaultId, std::vector<uint8_t>& encryptedData) override;
    bool deleteVault(const std::string& vaultId) override;
    bool vaultExists(const std::string& vaultId) override;
    
    std::vector<std::string> listVaults() override;
    std::chrono::system_clock::time_point getLastModified(const std::string& vaultId) override;
    size_t getVaultSize(const std::string& vaultId) override;
    
    bool syncVault(const std::string& vaultId) override;
    bool isVaultSynced(const std::string& vaultId) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace phantom_vault::cloud
