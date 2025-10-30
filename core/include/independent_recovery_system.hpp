/**
 * PhantomVault Independent Recovery System
 * 
 * Standalone recovery system that operates independently of the main application.
 * Designed to work with corrupted installations and provide emergency recovery
 * capabilities through safe boot mode.
 */

#pragma once

#include "vault_handler.hpp"
#include "profile_manager.hpp"
#include "error_handler.hpp"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>

namespace phantomvault {

/**
 * Recovery system status
 */
enum class RecoverySystemStatus {
    HEALTHY,
    DEGRADED,
    CORRUPTED,
    CRITICAL_FAILURE,
    EMERGENCY_MODE
};

/**
 * Recovery operation result
 */
struct RecoveryOperationResult {
    bool success = false;
    std::string operation_id;
    std::string message;
    std::string error_details;
    std::vector<std::string> recovered_items;
    std::vector<std::string> failed_items;
    std::chrono::system_clock::time_point timestamp;
    
    RecoveryOperationResult() : timestamp(std::chrono::system_clock::now()) {}
};

/**
 * Emergency recovery context
 */
struct EmergencyRecoveryContext {
    std::string profile_id;
    std::string vault_id;
    std::string recovery_key;
    std::string emergency_password;
    bool safe_boot_mode = false;
    bool bypass_integrity_checks = false;
    std::vector<std::string> critical_paths;
};

/**
 * Recovery wizard step
 */
struct RecoveryWizardStep {
    std::string step_id;
    std::string title;
    std::string description;
    std::vector<std::string> options;
    bool requires_user_input = false;
    bool is_critical = false;
    std::string next_step_id;
};

/**
 * Independent Recovery System
 * 
 * Provides comprehensive recovery capabilities that work independently
 * of the main PhantomVault application, including emergency recovery
 * through safe boot mode.
 */
class IndependentRecoverySystem {
public:
    IndependentRecoverySystem();
    ~IndependentRecoverySystem();

    // System initialization and status
    bool initialize(const std::string& data_path = "");
    bool initializeEmergencyMode(const std::string& data_path = "");
    RecoverySystemStatus getSystemStatus() const;
    bool isOperational() const;
    
    // Independent vault recovery operations
    RecoveryOperationResult recoverVaultIndependently(const std::string& vault_id, 
                                                     const std::string& profile_id = "");
    RecoveryOperationResult recoverCorruptedVault(const std::string& vault_id,
                                                 const EmergencyRecoveryContext& context);
    RecoveryOperationResult repairVaultStructure(const std::string& vault_id);
    RecoveryOperationResult validateAndRepairAllVaults();
    
    // Profile recovery operations
    RecoveryOperationResult recoverProfileData(const std::string& profile_id,
                                              const std::string& recovery_key);
    RecoveryOperationResult recoverProfileFromBackup(const std::string& profile_id,
                                                    const std::string& backup_path);
    RecoveryOperationResult resetProfilePassword(const std::string& profile_id,
                                                const std::string& recovery_key,
                                                const std::string& new_password);
    
    // Emergency recovery mode
    bool enterEmergencyMode(const EmergencyRecoveryContext& context);
    bool exitEmergencyMode();
    bool isInEmergencyMode() const;
    RecoveryOperationResult performEmergencyRecovery(const EmergencyRecoveryContext& context);
    
    // Safe boot recovery
    bool initializeSafeBootMode();
    RecoveryOperationResult performSafeBootRecovery();
    std::vector<std::string> detectCorruptedComponents();
    RecoveryOperationResult repairCorruptedComponents(const std::vector<std::string>& components);
    
    // Recovery wizard
    std::vector<RecoveryWizardStep> getRecoveryWizardSteps(const std::string& recovery_type = "");
    RecoveryOperationResult executeWizardStep(const std::string& step_id, 
                                             const std::vector<std::string>& user_inputs = {});
    bool isWizardActive() const;
    void resetWizard();
    
    // Backup and restore operations
    RecoveryOperationResult createEmergencyBackup(const std::string& profile_id);
    RecoveryOperationResult restoreFromEmergencyBackup(const std::string& backup_id,
                                                      const std::string& restore_path);
    std::vector<std::string> listAvailableBackups(const std::string& profile_id = "") const;
    RecoveryOperationResult verifyBackupIntegrity(const std::string& backup_id);
    
    // System diagnostics
    std::vector<std::string> performSystemDiagnostics();
    RecoveryOperationResult generateDiagnosticReport();
    bool testRecoveryCapabilities();
    
    // Configuration and settings
    bool setRecoveryConfiguration(const std::string& config_key, const std::string& config_value);
    std::string getRecoveryConfiguration(const std::string& config_key) const;
    bool resetToDefaultConfiguration();
    
    // Error handling and logging
    std::string getLastError() const;
    std::vector<std::string> getRecoveryLog() const;
    void clearRecoveryLog();

private:
    class Implementation;
    std::unique_ptr<Implementation> pimpl;
};

/**
 * Recovery System Factory
 * 
 * Creates recovery system instances with different configurations
 */
class RecoverySystemFactory {
public:
    static std::unique_ptr<IndependentRecoverySystem> createStandardRecoverySystem();
    static std::unique_ptr<IndependentRecoverySystem> createEmergencyRecoverySystem();
    static std::unique_ptr<IndependentRecoverySystem> createSafeBootRecoverySystem();
    static std::unique_ptr<IndependentRecoverySystem> createPortableRecoverySystem(const std::string& portable_path);
};

} // namespace phantomvault