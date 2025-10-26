/**
 * PhantomVault Folder Security Manager
 * 
 * Manages folder locking, encryption, and security operations with complete
 * trace removal and secure backup mechanisms.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>

namespace phantomvault {

/**
 * Unlock modes for folder operations
 */
enum class UnlockMode {
    TEMPORARY,   // Folder unlocked temporarily, auto-locks on session end
    PERMANENT    // Folder unlocked permanently, removed from vault management
};

/**
 * Secured folder information
 */
struct SecuredFolder {
    std::string id;
    std::string profileId;
    std::string originalName;
    std::string originalPath;
    std::string vaultPath;
    bool isLocked;
    UnlockMode unlockMode;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastAccess;
    size_t originalSize;
};

/**
 * Folder operation results
 */
struct FolderOperationResult {
    bool success = false;
    std::string folderId;
    std::string message;
    std::string error;
};

/**
 * Unlock operation results
 */
struct UnlockResult {
    bool success = false;
    int successCount = 0;
    int failedCount = 0;
    std::vector<std::string> unlockedFolderIds;
    std::vector<std::string> failedFolderIds;
    std::string message;
    std::string error;
};

/**
 * Folder Security Manager class
 * 
 * Provides complete folder security with encryption, hiding, and backup mechanisms.
 * Designed for profile-scoped operations with complete trace removal.
 */
class FolderSecurityManager {
public:
    FolderSecurityManager();
    ~FolderSecurityManager();

    // Initialization
    bool initialize(const std::string& dataPath = "");

    // Folder security operations
    FolderOperationResult lockFolder(const std::string& profileId, 
                                   const std::string& folderPath, 
                                   const std::string& masterKey);
    
    UnlockResult unlockFoldersTemporary(const std::string& profileId, 
                                      const std::string& masterKey);
    
    UnlockResult unlockFoldersPermanent(const std::string& profileId, 
                                      const std::string& masterKey,
                                      const std::vector<std::string>& folderIds = {});
    
    bool lockTemporaryFolders(const std::string& profileId);
    
    // Folder management
    std::vector<SecuredFolder> getProfileFolders(const std::string& profileId);
    std::optional<SecuredFolder> getFolder(const std::string& profileId, const std::string& folderId);
    FolderOperationResult removeFromProfile(const std::string& profileId, const std::string& folderId);
    
    // Utility functions
    bool validateFolderPath(const std::string& folderPath);
    size_t calculateFolderSize(const std::string& folderPath);
    std::string generateFolderId();
    
    // Error handling
    std::string getLastError() const;

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

} // namespace phantomvault