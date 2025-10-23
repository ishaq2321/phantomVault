#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <chrono>
#include <functional>

namespace phantom_vault::notes {

/**
 * @brief Note types
 */
enum class PHANTOM_VAULT_EXPORT NoteType {
    Text,           // Plain text note
    RichText,       // Rich text with formatting
    Markdown,       // Markdown formatted text
    Code,           // Code snippet
    Password,       // Password entry
    CreditCard,     // Credit card information
    PersonalInfo,   // Personal information
    SecureMemo      // General secure memo
};

/**
 * @brief Note priority levels
 */
enum class PHANTOM_VAULT_EXPORT NotePriority {
    Low,
    Normal,
    High,
    Critical
};

/**
 * @brief Note attachment types
 */
enum class PHANTOM_VAULT_EXPORT AttachmentType {
    File,           // File attachment
    Image,          // Image attachment
    Document,       // Document attachment
    Audio,          // Audio attachment
    Video,          // Video attachment
    Archive         // Archive attachment
};

/**
 * @brief Note attachment information
 */
struct PHANTOM_VAULT_EXPORT NoteAttachment {
    std::string id;                     // Attachment identifier
    std::string filename;               // Original filename
    std::string mimeType;               // MIME type
    AttachmentType type;                // Attachment type
    size_t size;                        // File size in bytes
    std::vector<uint8_t> encryptedData; // Encrypted file data
    std::string checksum;               // File checksum
    std::chrono::system_clock::time_point attachedTime;
    bool isCompressed;                  // Whether file is compressed
};

/**
 * @brief Note tag information
 */
struct PHANTOM_VAULT_EXPORT NoteTag {
    std::string id;                     // Tag identifier
    std::string name;                   // Tag name
    std::string color;                  // Tag color (hex)
    std::string description;            // Tag description
    int usageCount;                     // Number of times used
    std::chrono::system_clock::time_point createdTime;
};

/**
 * @brief Encrypted note structure
 */
struct PHANTOM_VAULT_EXPORT EncryptedNote {
    std::string id;                     // Note identifier
    std::string title;                  // Note title
    std::string content;                // Encrypted note content
    NoteType type;                      // Note type
    NotePriority priority;              // Note priority
    std::vector<std::string> tags;     // Note tags
    std::vector<NoteAttachment> attachments; // Note attachments
    std::string vaultId;                // Associated vault ID
    std::string userId;                 // Note owner
    std::chrono::system_clock::time_point createdTime;
    std::chrono::system_clock::time_point modifiedTime;
    std::chrono::system_clock::time_point lastAccessed;
    bool isEncrypted;                   // Encryption status
    bool isPinned;                      // Pinned status
    bool isArchived;                    // Archived status
    bool isShared;                      // Shared status
    std::string encryptionKey;          // Note-specific encryption key
    std::string checksum;               // Content checksum
    size_t version;                     // Note version
    std::map<std::string, std::string> metadata; // Additional metadata
};

/**
 * @brief Note search criteria
 */
struct PHANTOM_VAULT_EXPORT NoteSearchCriteria {
    std::string query;                  // Search query
    std::vector<NoteType> types;       // Filter by note types
    std::vector<std::string> tags;     // Filter by tags
    std::string vaultId;               // Filter by vault
    std::string userId;                // Filter by user
    NotePriority minPriority;          // Minimum priority
    std::chrono::system_clock::time_point fromDate;
    std::chrono::system_clock::time_point toDate;
    bool includeArchived;              // Include archived notes
    bool includeShared;                // Include shared notes
    int limit;                         // Result limit
    int offset;                        // Result offset
};

/**
 * @brief Note sharing information
 */
struct PHANTOM_VAULT_EXPORT NoteShare {
    std::string id;                     // Share identifier
    std::string noteId;                 // Note identifier
    std::string sharedWith;             // User ID or email
    std::set<std::string> permissions;  // Share permissions
    std::chrono::system_clock::time_point sharedTime;
    std::chrono::system_clock::time_point expiresTime;
    bool isActive;                      // Share status
    std::string accessCode;             // Access code for external sharing
    std::string shareUrl;               // Share URL
};

/**
 * @brief Encrypted notes manager interface
 */
class PHANTOM_VAULT_EXPORT NotesManager {
public:
    virtual ~NotesManager() = default;
    
    // Note management
    virtual std::string createNote(const std::string& title, const std::string& content, 
                                  NoteType type, const std::string& vaultId) = 0;
    virtual bool updateNote(const std::string& noteId, const std::string& title, 
                           const std::string& content) = 0;
    virtual bool deleteNote(const std::string& noteId) = 0;
    virtual EncryptedNote getNote(const std::string& noteId) = 0;
    virtual std::vector<EncryptedNote> getNotes(const std::string& vaultId) = 0;
    virtual std::vector<EncryptedNote> getAllNotes() = 0;
    
    // Note operations
    virtual bool pinNote(const std::string& noteId) = 0;
    virtual bool unpinNote(const std::string& noteId) = 0;
    virtual bool archiveNote(const std::string& noteId) = 0;
    virtual bool unarchiveNote(const std::string& noteId) = 0;
    virtual bool duplicateNote(const std::string& noteId) = 0;
    virtual bool moveNote(const std::string& noteId, const std::string& newVaultId) = 0;
    
    // Search and filtering
    virtual std::vector<EncryptedNote> searchNotes(const NoteSearchCriteria& criteria) = 0;
    virtual std::vector<EncryptedNote> getNotesByTag(const std::string& tag) = 0;
    virtual std::vector<EncryptedNote> getNotesByType(NoteType type) = 0;
    virtual std::vector<EncryptedNote> getPinnedNotes() = 0;
    virtual std::vector<EncryptedNote> getRecentNotes(int limit = 10) = 0;
    
    // Content operations
    virtual std::string decryptNoteContent(const std::string& noteId) = 0;
    virtual bool encryptNoteContent(const std::string& noteId, const std::string& content) = 0;
    virtual bool updateNoteContent(const std::string& noteId, const std::string& content) = 0;
    
    // Tag management
    virtual bool addTagToNote(const std::string& noteId, const std::string& tag) = 0;
    virtual bool removeTagFromNote(const std::string& noteId, const std::string& tag) = 0;
    virtual std::vector<NoteTag> getAllTags() = 0;
    virtual std::vector<NoteTag> getPopularTags(int limit = 20) = 0;
    virtual bool createTag(const std::string& name, const std::string& color) = 0;
    virtual bool deleteTag(const std::string& tagId) = 0;
    
    // Attachment management
    virtual bool addAttachment(const std::string& noteId, const std::string& filePath) = 0;
    virtual bool removeAttachment(const std::string& noteId, const std::string& attachmentId) = 0;
    virtual std::vector<NoteAttachment> getNoteAttachments(const std::string& noteId) = 0;
    virtual bool downloadAttachment(const std::string& noteId, const std::string& attachmentId, 
                                   const std::string& outputPath) = 0;
    
    // Sharing
    virtual std::string shareNote(const std::string& noteId, const std::string& sharedWith, 
                                 const std::set<std::string>& permissions) = 0;
    virtual bool unshareNote(const std::string& shareId) = 0;
    virtual std::vector<NoteShare> getNoteShares(const std::string& noteId) = 0;
    virtual std::vector<NoteShare> getSharedWithMe() = 0;
    virtual bool updateSharePermissions(const std::string& shareId, 
                                       const std::set<std::string>& permissions) = 0;
    
    // Export/Import
    virtual bool exportNote(const std::string& noteId, const std::string& filePath) = 0;
    virtual bool exportNotes(const std::vector<std::string>& noteIds, const std::string& filePath) = 0;
    virtual bool importNote(const std::string& filePath, const std::string& vaultId) = 0;
    virtual bool importNotes(const std::string& filePath, const std::string& vaultId) = 0;
    
    // Event callbacks
    virtual void setNoteCreatedCallback(std::function<void(const EncryptedNote&)> callback) = 0;
    virtual void setNoteUpdatedCallback(std::function<void(const EncryptedNote&)> callback) = 0;
    virtual void setNoteDeletedCallback(std::function<void(const std::string&)> callback) = 0;
    virtual void setNoteSharedCallback(std::function<void(const NoteShare&)> callback) = 0;
};

/**
 * @brief Local encrypted notes manager implementation
 */
class PHANTOM_VAULT_EXPORT LocalNotesManager : public NotesManager {
public:
    LocalNotesManager();
    ~LocalNotesManager() override;
    
    // Note management
    std::string createNote(const std::string& title, const std::string& content, 
                          NoteType type, const std::string& vaultId) override;
    bool updateNote(const std::string& noteId, const std::string& title, 
                   const std::string& content) override;
    bool deleteNote(const std::string& noteId) override;
    EncryptedNote getNote(const std::string& noteId) override;
    std::vector<EncryptedNote> getNotes(const std::string& vaultId) override;
    std::vector<EncryptedNote> getAllNotes() override;
    
    // Note operations
    bool pinNote(const std::string& noteId) override;
    bool unpinNote(const std::string& noteId) override;
    bool archiveNote(const std::string& noteId) override;
    bool unarchiveNote(const std::string& noteId) override;
    bool duplicateNote(const std::string& noteId) override;
    bool moveNote(const std::string& noteId, const std::string& newVaultId) override;
    
    // Search and filtering
    std::vector<EncryptedNote> searchNotes(const NoteSearchCriteria& criteria) override;
    std::vector<EncryptedNote> getNotesByTag(const std::string& tag) override;
    std::vector<EncryptedNote> getNotesByType(NoteType type) override;
    std::vector<EncryptedNote> getPinnedNotes() override;
    std::vector<EncryptedNote> getRecentNotes(int limit = 10) override;
    
    // Content operations
    std::string decryptNoteContent(const std::string& noteId) override;
    bool encryptNoteContent(const std::string& noteId, const std::string& content) override;
    bool updateNoteContent(const std::string& noteId, const std::string& content) override;
    
    // Tag management
    bool addTagToNote(const std::string& noteId, const std::string& tag) override;
    bool removeTagFromNote(const std::string& noteId, const std::string& tag) override;
    std::vector<NoteTag> getAllTags() override;
    std::vector<NoteTag> getPopularTags(int limit = 20) override;
    bool createTag(const std::string& name, const std::string& color) override;
    bool deleteTag(const std::string& tagId) override;
    
    // Attachment management
    bool addAttachment(const std::string& noteId, const std::string& filePath) override;
    bool removeAttachment(const std::string& noteId, const std::string& attachmentId) override;
    std::vector<NoteAttachment> getNoteAttachments(const std::string& noteId) override;
    bool downloadAttachment(const std::string& noteId, const std::string& attachmentId, 
                           const std::string& outputPath) override;
    
    // Sharing
    std::string shareNote(const std::string& noteId, const std::string& sharedWith, 
                         const std::set<std::string>& permissions) override;
    bool unshareNote(const std::string& shareId) override;
    std::vector<NoteShare> getNoteShares(const std::string& noteId) override;
    std::vector<NoteShare> getSharedWithMe() override;
    bool updateSharePermissions(const std::string& shareId, 
                               const std::set<std::string>& permissions) override;
    
    // Export/Import
    bool exportNote(const std::string& noteId, const std::string& filePath) override;
    bool exportNotes(const std::vector<std::string>& noteIds, const std::string& filePath) override;
    bool importNote(const std::string& filePath, const std::string& vaultId) override;
    bool importNotes(const std::string& filePath, const std::string& vaultId) override;
    
    // Event callbacks
    void setNoteCreatedCallback(std::function<void(const EncryptedNote&)> callback) override;
    void setNoteUpdatedCallback(std::function<void(const EncryptedNote&)> callback) override;
    void setNoteDeletedCallback(std::function<void(const std::string&)> callback) override;
    void setNoteSharedCallback(std::function<void(const NoteShare&)> callback) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Note encryption utilities
 */
class PHANTOM_VAULT_EXPORT NoteEncryption {
public:
    // Content encryption
    static std::string encryptContent(const std::string& content, const std::string& key);
    static std::string decryptContent(const std::string& encryptedContent, const std::string& key);
    
    // Key generation
    static std::string generateNoteKey();
    static std::string deriveKeyFromVault(const std::string& vaultId, const std::string& masterKey);
    
    // Secure storage
    static bool storeNoteSecurely(const EncryptedNote& note);
    static EncryptedNote retrieveNoteSecurely(const std::string& noteId);
    static bool deleteNoteSecurely(const std::string& noteId);
    
    // Integrity verification
    static std::string calculateChecksum(const std::string& content);
    static bool verifyChecksum(const std::string& content, const std::string& checksum);
    
    // Search indexing
    static std::string createSearchIndex(const std::string& content);
    static std::vector<std::string> extractSearchTerms(const std::string& content);
};

} // namespace phantom_vault::notes
