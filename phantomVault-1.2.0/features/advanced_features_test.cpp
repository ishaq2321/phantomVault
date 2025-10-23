#include <gtest/gtest.h>
#include "cloud-backup/cloud_backup.hpp"
#include "multi-user/user_management.hpp"
#include "biometric/biometric_auth.hpp"
#include "notes/encrypted_notes.hpp"
#include "emergency/emergency_lockdown.hpp"
#include "logging/activity_logger.hpp"

using namespace phantom_vault::cloud;
using namespace phantom_vault::users;
using namespace phantom_vault::biometric;
using namespace phantom_vault::notes;
using namespace phantom_vault::emergency;
using namespace phantom_vault::logging;

class AdvancedFeaturesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test data
    }
    
    void TearDown() override {
        // Cleanup test data
    }
};

// Cloud Backup Tests
TEST_F(AdvancedFeaturesTest, CloudBackupConfiguration) {
    CloudBackupManager manager;
    CloudConfig config;
    config.provider = "aws_s3";
    config.bucket_name = "test-bucket";
    config.region = "us-east-1";
    config.auto_sync = true;
    config.encrypt_before_upload = true;
    
    EXPECT_TRUE(manager.configure(config));
    EXPECT_TRUE(manager.isConfigured());
    
    auto retrievedConfig = manager.getConfiguration();
    EXPECT_EQ(retrievedConfig.provider, "aws_s3");
    EXPECT_EQ(retrievedConfig.bucket_name, "test-bucket");
}

TEST_F(AdvancedFeaturesTest, CloudBackupOperations) {
    CloudBackupManager manager;
    CloudConfig config;
    config.provider = "aws_s3";
    config.bucket_name = "test-bucket";
    config.region = "us-east-1";
    
    ASSERT_TRUE(manager.configure(config));
    
    // Test backup operation
    EXPECT_TRUE(manager.backupVault("test-vault-1"));
    
    // Test restore operation
    EXPECT_TRUE(manager.restoreVault("test-vault-1"));
    
    // Test sync operation
    EXPECT_TRUE(manager.syncVault("test-vault-1"));
    
    // Test status
    EXPECT_TRUE(manager.isVaultSynced("test-vault-1"));
}

TEST_F(AdvancedFeaturesTest, CloudProviderCreation) {
    // Test AWS S3 provider
    auto awsProvider = std::make_shared<AWSS3Provider>("access_key", "secret_key", "bucket", "us-east-1");
    EXPECT_TRUE(awsProvider->authenticate("credentials"));
    EXPECT_TRUE(awsProvider->isAuthenticated());
    
    // Test Google Drive provider
    auto gdriveProvider = std::make_shared<GoogleDriveProvider>("client_id", "client_secret", "refresh_token");
    EXPECT_TRUE(gdriveProvider->authenticate("credentials"));
    EXPECT_TRUE(gdriveProvider->isAuthenticated());
}

// Multi-User Tests
TEST_F(AdvancedFeaturesTest, UserManagement) {
    LocalUserManager userManager;
    
    // Create test user
    UserAccount user;
    user.id = "test-user-1";
    user.username = "testuser";
    user.email = "test@example.com";
    user.displayName = "Test User";
    user.role = UserRole::User;
    user.isActive = true;
    
    EXPECT_TRUE(userManager.createUser(user, "password123"));
    
    // Test authentication
    auto authResult = userManager.authenticate("testuser", "password123");
    EXPECT_TRUE(authResult.success);
    EXPECT_EQ(authResult.userRole, UserRole::User);
    
    // Test user retrieval
    auto retrievedUser = userManager.getUser("test-user-1");
    EXPECT_EQ(retrievedUser.username, "testuser");
    EXPECT_EQ(retrievedUser.email, "test@example.com");
}

TEST_F(AdvancedFeaturesTest, UserPermissions) {
    LocalUserManager userManager;
    
    // Create test user
    UserAccount user;
    user.id = "test-user-2";
    user.username = "testuser2";
    user.role = UserRole::User;
    user.isActive = true;
    
    ASSERT_TRUE(userManager.createUser(user, "password123"));
    
    // Test permission management
    EXPECT_TRUE(userManager.grantPermission("test-user-2", Permission::CreateVault));
    EXPECT_TRUE(userManager.hasPermission("test-user-2", Permission::CreateVault));
    
    EXPECT_TRUE(userManager.revokePermission("test-user-2", Permission::CreateVault));
    EXPECT_FALSE(userManager.hasPermission("test-user-2", Permission::CreateVault));
}

TEST_F(AdvancedFeaturesTest, VaultAccessControl) {
    LocalUserManager userManager;
    
    // Create test user
    UserAccount user;
    user.id = "test-user-3";
    user.username = "testuser3";
    user.role = UserRole::User;
    user.isActive = true;
    
    ASSERT_TRUE(userManager.createUser(user, "password123"));
    
    // Test vault access
    std::set<Permission> permissions = {Permission::ViewVault, Permission::ModifyVault};
    EXPECT_TRUE(userManager.grantVaultAccess("test-vault-1", "test-user-3", permissions));
    EXPECT_TRUE(userManager.hasVaultAccess("test-vault-1", "test-user-3", Permission::ViewVault));
    
    EXPECT_TRUE(userManager.revokeVaultAccess("test-vault-1", "test-user-3"));
    EXPECT_FALSE(userManager.hasVaultAccess("test-vault-1", "test-user-3", Permission::ViewVault));
}

// Biometric Authentication Tests
TEST_F(AdvancedFeaturesTest, BiometricManager) {
    BiometricManager manager;
    
    EXPECT_TRUE(manager.initialize());
    
    // Test device availability
    auto devices = manager.getAvailableDevices();
    EXPECT_FALSE(devices.empty());
    
    // Test enrollment
    EXPECT_TRUE(manager.enrollUser("test-user-1", BiometricType::Fingerprint));
    EXPECT_TRUE(manager.isUserEnrolled("test-user-1", BiometricType::Fingerprint));
    
    // Test authentication
    EXPECT_TRUE(manager.authenticateUser("test-user-1", BiometricType::Fingerprint));
    
    manager.shutdown();
}

TEST_F(AdvancedFeaturesTest, BiometricSecurity) {
    // Test template encryption
    std::vector<uint8_t> templateData = {0x01, 0x02, 0x03, 0x04};
    std::string key = "test-key-123";
    
    auto encrypted = BiometricSecurity::encryptTemplate(templateData, key);
    EXPECT_FALSE(encrypted.empty());
    
    auto decrypted = BiometricSecurity::decryptTemplate(encrypted, key);
    EXPECT_EQ(templateData, decrypted);
    
    // Test template hashing
    auto hash = BiometricSecurity::hashTemplate(templateData);
    EXPECT_FALSE(hash.empty());
    EXPECT_TRUE(BiometricSecurity::verifyTemplateHash(templateData, hash));
    
    // Test quality assessment
    int quality = BiometricSecurity::assessTemplateQuality(templateData);
    EXPECT_GE(quality, 0);
    EXPECT_LE(quality, 100);
}

// Encrypted Notes Tests
TEST_F(AdvancedFeaturesTest, NotesManager) {
    LocalNotesManager notesManager;
    
    // Create test note
    std::string noteId = notesManager.createNote("Test Note", "This is a test note", 
                                                 NoteType::Text, "test-vault-1");
    EXPECT_FALSE(noteId.empty());
    
    // Test note retrieval
    auto note = notesManager.getNote(noteId);
    EXPECT_EQ(note.title, "Test Note");
    EXPECT_EQ(note.type, NoteType::Text);
    
    // Test note update
    EXPECT_TRUE(notesManager.updateNote(noteId, "Updated Note", "Updated content"));
    
    // Test note operations
    EXPECT_TRUE(notesManager.pinNote(noteId));
    EXPECT_TRUE(notesManager.unpinNote(noteId));
    
    EXPECT_TRUE(notesManager.archiveNote(noteId));
    EXPECT_TRUE(notesManager.unarchiveNote(noteId));
}

TEST_F(AdvancedFeaturesTest, NotesSearch) {
    LocalNotesManager notesManager;
    
    // Create test notes
    std::string note1 = notesManager.createNote("Note 1", "Content 1", NoteType::Text, "vault-1");
    std::string note2 = notesManager.createNote("Note 2", "Content 2", NoteType::Markdown, "vault-1");
    
    // Test search
    NoteSearchCriteria criteria;
    criteria.query = "Note";
    criteria.vaultId = "vault-1";
    
    auto results = notesManager.searchNotes(criteria);
    EXPECT_GE(results.size(), 2);
    
    // Test tag management
    EXPECT_TRUE(notesManager.addTagToNote(note1, "important"));
    EXPECT_TRUE(notesManager.addTagToNote(note1, "work"));
    
    auto notesByTag = notesManager.getNotesByTag("important");
    EXPECT_FALSE(notesByTag.empty());
}

TEST_F(AdvancedFeaturesTest, NotesEncryption) {
    // Test content encryption
    std::string content = "This is sensitive content";
    std::string key = "test-encryption-key";
    
    auto encrypted = NoteEncryption::encryptContent(content, key);
    EXPECT_FALSE(encrypted.empty());
    EXPECT_NE(encrypted, content);
    
    auto decrypted = NoteEncryption::decryptContent(encrypted, key);
    EXPECT_EQ(decrypted, content);
    
    // Test key generation
    auto noteKey = NoteEncryption::generateNoteKey();
    EXPECT_FALSE(noteKey.empty());
    
    // Test checksum
    auto checksum = NoteEncryption::calculateChecksum(content);
    EXPECT_FALSE(checksum.empty());
    EXPECT_TRUE(NoteEncryption::verifyChecksum(content, checksum));
}

// Emergency Lockdown Tests
TEST_F(AdvancedFeaturesTest, EmergencyLockdown) {
    LocalEmergencyLockdownManager lockdownManager;
    
    // Configure lockdown
    LockdownConfig config;
    config.enabled = true;
    config.triggers = {LockdownTrigger::Manual, LockdownTrigger::PanicButton};
    config.defaultLevel = LockdownLevel::Soft;
    config.activationDelay = std::chrono::seconds(5);
    
    EXPECT_TRUE(lockdownManager.configure(config));
    EXPECT_TRUE(lockdownManager.isEnabled());
    
    // Test lockdown activation
    EXPECT_TRUE(lockdownManager.activateLockdown(LockdownTrigger::Manual, LockdownLevel::Soft));
    EXPECT_TRUE(lockdownManager.isLockdownActive());
    
    // Test recovery code
    EXPECT_TRUE(lockdownManager.generateRecoveryCode());
    auto recoveryCode = lockdownManager.getRecoveryCode();
    EXPECT_FALSE(recoveryCode.empty());
    
    // Test lockdown deactivation
    EXPECT_TRUE(lockdownManager.deactivateLockdown(recoveryCode));
    EXPECT_FALSE(lockdownManager.isLockdownActive());
}

TEST_F(AdvancedFeaturesTest, PanicButton) {
    PanicButtonHandler panicHandler;
    
    // Register panic button
    EXPECT_TRUE(panicHandler.registerPanicButton("panic-btn-1", LockdownLevel::Hard));
    EXPECT_TRUE(panicHandler.isPanicButtonRegistered("panic-btn-1"));
    
    // Test panic button trigger
    EXPECT_TRUE(panicHandler.triggerPanicButton("panic-btn-1"));
    
    // Test global panic button
    EXPECT_TRUE(panicHandler.enableGlobalPanicButton(LockdownLevel::Medium));
    EXPECT_TRUE(panicHandler.isGlobalPanicButtonEnabled());
}

TEST_F(AdvancedFeaturesTest, EmergencySecurity) {
    // Test secure wipe
    std::string testFile = "/tmp/test_wipe.txt";
    std::ofstream file(testFile);
    file << "test data";
    file.close();
    
    EXPECT_TRUE(EmergencySecurity::secureWipeFile(testFile, 3));
    
    // Test vault protection
    EXPECT_TRUE(EmergencySecurity::lockAllVaults());
    EXPECT_TRUE(EmergencySecurity::clearClipboard());
    EXPECT_TRUE(EmergencySecurity::clearTempFiles());
    
    // Test emergency protocols
    EXPECT_TRUE(EmergencySecurity::executeEmergencyProtocol(LockdownLevel::Soft));
}

// Activity Logging Tests
TEST_F(AdvancedFeaturesTest, ActivityLogging) {
    LocalActivityLogger logger;
    
    // Test basic logging
    EXPECT_TRUE(logger.logActivity(ActivityType::UserLogin, LogLevel::Info, "user-1", "User logged in"));
    EXPECT_TRUE(logger.logSecurityEvent("user-1", "Failed login attempt"));
    EXPECT_TRUE(logger.logError("user-1", "Database connection failed"));
    
    // Test log retrieval
    auto userLogs = logger.getUserLogs("user-1", 10);
    EXPECT_GE(userLogs.size(), 3);
    
    auto recentLogs = logger.getRecentLogs(5);
    EXPECT_LE(recentLogs.size(), 5);
    
    auto securityLogs = logger.getSecurityLogs(10);
    EXPECT_GE(securityLogs.size(), 1);
}

TEST_F(AdvancedFeaturesTest, LogFiltering) {
    LocalActivityLogger logger;
    
    // Create test logs
    logger.logActivity(ActivityType::UserLogin, LogLevel::Info, "user-1", "Login 1");
    logger.logActivity(ActivityType::VaultCreated, LogLevel::Info, "user-1", "Vault created");
    logger.logActivity(ActivityType::UserLogin, LogLevel::Info, "user-2", "Login 2");
    
    // Test filtering
    LogFilter filter;
    filter.types = {ActivityType::UserLogin};
    filter.userId = "user-1";
    
    auto filteredLogs = logger.getLogs(filter);
    EXPECT_EQ(filteredLogs.size(), 1);
    EXPECT_EQ(filteredLogs[0].type, ActivityType::UserLogin);
    EXPECT_EQ(filteredLogs[0].userId, "user-1");
}

TEST_F(AdvancedFeaturesTest, LogStatistics) {
    LocalActivityLogger logger;
    
    // Create test logs
    logger.logActivity(ActivityType::UserLogin, LogLevel::Info, "user-1", "Login");
    logger.logActivity(ActivityType::VaultCreated, LogLevel::Info, "user-1", "Vault created");
    logger.logActivity(ActivityType::UserLogin, LogLevel::Warning, "user-2", "Failed login");
    
    // Test statistics
    auto stats = logger.getStatistics();
    EXPECT_GE(stats.totalEntries, 3);
    EXPECT_GT(stats.entriesByLevel[static_cast<int>(LogLevel::Info)], 0);
    EXPECT_GT(stats.entriesByLevel[static_cast<int>(LogLevel::Warning)], 0);
    
    auto activityCounts = logger.getActivityCounts();
    EXPECT_GT(activityCounts["UserLogin"], 0);
    EXPECT_GT(activityCounts["VaultCreated"], 0);
}

TEST_F(AdvancedFeaturesTest, LogSecurity) {
    // Test log encryption
    ActivityLogEntry entry;
    entry.id = "test-log-1";
    entry.type = ActivityType::UserLogin;
    entry.level = LogLevel::Info;
    entry.userId = "user-1";
    entry.description = "User logged in";
    entry.timestamp = std::chrono::system_clock::now();
    
    std::string key = "test-encryption-key";
    auto encrypted = LogSecurity::encryptLogEntry(entry, key);
    EXPECT_FALSE(encrypted.empty());
    
    auto decrypted = LogSecurity::decryptLogEntry(encrypted, key);
    EXPECT_EQ(decrypted.id, entry.id);
    EXPECT_EQ(decrypted.type, entry.type);
    
    // Test checksum
    auto checksum = LogSecurity::calculateLogChecksum(entry);
    EXPECT_FALSE(checksum.empty());
    EXPECT_TRUE(LogSecurity::verifyLogChecksum(entry));
    
    // Test sensitive data masking
    entry.details["password"] = "secret123";
    auto masked = LogSecurity::maskSensitiveData(entry);
    EXPECT_TRUE(masked.details.find("password") == masked.details.end() || 
                masked.details["password"] != "secret123");
}

// Integration Tests
TEST_F(AdvancedFeaturesTest, CloudBackupIntegration) {
    CloudBackupManager backupManager;
    LocalUserManager userManager;
    LocalActivityLogger logger;
    
    // Configure services
    CloudConfig config;
    config.provider = "aws_s3";
    config.bucket_name = "test-bucket";
    backupManager.configure(config);
    
    // Create user
    UserAccount user;
    user.id = "test-user";
    user.username = "testuser";
    user.role = UserRole::User;
    user.isActive = true;
    userManager.createUser(user, "password");
    
    // Test integrated workflow
    EXPECT_TRUE(backupManager.backupVault("test-vault"));
    EXPECT_TRUE(logger.logActivity(ActivityType::VaultBackedUp, LogLevel::Info, "test-user", "Vault backed up"));
    
    auto logs = logger.getUserLogs("test-user", 10);
    EXPECT_FALSE(logs.empty());
}

TEST_F(AdvancedFeaturesTest, EmergencyLockdownIntegration) {
    LocalEmergencyLockdownManager lockdownManager;
    LocalActivityLogger logger;
    LocalNotesManager notesManager;
    
    // Configure lockdown
    LockdownConfig config;
    config.enabled = true;
    config.triggers = {LockdownTrigger::Manual};
    lockdownManager.configure(config);
    
    // Create test note
    std::string noteId = notesManager.createNote("Test Note", "Sensitive content", 
                                                 NoteType::Text, "test-vault");
    
    // Test emergency lockdown
    EXPECT_TRUE(lockdownManager.activateLockdown(LockdownTrigger::Manual, LockdownLevel::Hard));
    
    // Verify logging
    auto logs = logger.getSecurityLogs(10);
    EXPECT_FALSE(logs.empty());
    
    // Test emergency security
    EXPECT_TRUE(EmergencySecurity::lockAllVaults());
    EXPECT_TRUE(EmergencySecurity::clearClipboard());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
