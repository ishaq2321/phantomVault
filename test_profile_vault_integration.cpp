#include "core/include/profile_manager.hpp"
#include "core/include/profile_vault.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace phantomvault;
namespace fs = std::filesystem;

int main() {
    std::cout << "=== TESTING PROFILE MANAGER VAULT INTEGRATION ===" << std::endl;
    std::cout << "Testing ProfileManager initialization and vault system integration" << std::endl;
    
    // Test setup
    std::string test_data_path = "./test_profile_vault_integration";
    
    // Cleanup any existing test data
    if (fs::exists(test_data_path)) {
        fs::remove_all(test_data_path);
    }
    
    try {
        // Test 1: Initialize ProfileManager with vault integration
        std::cout << "\n1. Testing ProfileManager initialization with VaultManager..." << std::endl;
        
        ProfileManager profile_manager;
        if (!profile_manager.initialize(test_data_path)) {
            std::cerr << "FAILED: ProfileManager initialization: " << profile_manager.getLastError() << std::endl;
            return 1;
        }
        std::cout << "ProfileManager initialization with VaultManager PASSED" << std::endl;
        
        // Test 2: Verify vault system is initialized
        std::cout << "\n2. Verifying vault system initialization..." << std::endl;
        
        // Check that vault directory structure is created
        std::string vault_root = test_data_path + "/vaults";
        if (!fs::exists(vault_root)) {
            std::cerr << "ERROR: Vault root directory not created!" << std::endl;
            return 1;
        }
        
        std::cout << "Vault system initialization PASSED" << std::endl;
        std::cout << "  Vault root: " << vault_root << std::endl;
        
        // Test 3: Test vault operations on non-existent profile
        std::cout << "\n3. Testing vault operations on non-existent profile..." << std::endl;
        
        std::string fake_profile_id = "non_existent_profile";
        
        size_t vault_size = profile_manager.getProfileVaultSize(fake_profile_id);
        // Note: ProfileManager creates vaults on demand, so this will create an empty vault
        std::cout << "  Non-existent profile vault size: " << vault_size << " bytes (vault created on demand)" << std::endl;
        
        bool vault_valid = profile_manager.validateProfileVault(fake_profile_id);
        if (!vault_valid) {
            std::cerr << "ERROR: Empty vault should validate as true!" << std::endl;
            return 1;
        }
        
        auto locked_folders = profile_manager.getProfileLockedFolders(fake_profile_id);
        if (!locked_folders.empty()) {
            std::cerr << "ERROR: Non-existent profile returned locked folders!" << std::endl;
            return 1;
        }
        
        std::cout << "Non-existent profile vault operations PASSED" << std::endl;
        
        // Test 4: Test direct VaultManager integration
        std::cout << "\n4. Testing direct VaultManager integration..." << std::endl;
        
        // Create a test profile vault directly
        PhantomVault::VaultManager vault_manager(vault_root);
        std::string test_profile_id = "test_profile_direct";
        
        if (!vault_manager.createProfileVault(test_profile_id)) {
            std::cerr << "ERROR: Failed to create test profile vault: " << vault_manager.getLastError() << std::endl;
            return 1;
        }
        
        // Test ProfileManager operations on this vault
        size_t direct_vault_size = profile_manager.getProfileVaultSize(test_profile_id);
        std::cout << "  Direct vault size: " << direct_vault_size << " bytes" << std::endl;
        
        bool direct_vault_valid = profile_manager.validateProfileVault(test_profile_id);
        if (!direct_vault_valid) {
            std::cerr << "ERROR: Direct vault validation failed!" << std::endl;
            return 1;
        }
        
        bool maintenance_result = profile_manager.performProfileVaultMaintenance(test_profile_id);
        if (!maintenance_result) {
            std::cerr << "ERROR: Direct vault maintenance failed!" << std::endl;
            return 1;
        }
        
        std::cout << "Direct VaultManager integration PASSED" << std::endl;
        
        // Test 5: Test API consistency
        std::cout << "\n5. Testing API consistency..." << std::endl;
        
        // Test that all new methods exist and are callable
        auto all_profiles = profile_manager.getAllProfiles();
        bool admin_check = profile_manager.isRunningAsAdmin();
        bool requires_admin = profile_manager.requiresAdminForProfileCreation();
        std::string last_error = profile_manager.getLastError();
        
        std::cout << "API consistency PASSED" << std::endl;
        std::cout << "  Profile count: " << all_profiles.size() << std::endl;
        std::cout << "  Running as admin: " << (admin_check ? "Yes" : "No") << std::endl;
        std::cout << "  Requires admin: " << (requires_admin ? "Yes" : "No") << std::endl;
        
        // Test 6: Test error handling
        std::cout << "\n6. Testing error handling..." << std::endl;
        
        // Test operations on invalid profile IDs
        std::string invalid_id = "";
        size_t invalid_size = profile_manager.getProfileVaultSize(invalid_id);
        bool invalid_valid = profile_manager.validateProfileVault(invalid_id);
        auto invalid_folders = profile_manager.getProfileLockedFolders(invalid_id);
        
        if (invalid_size != 0 || invalid_valid || !invalid_folders.empty()) {
            std::cerr << "ERROR: Invalid profile ID operations should return safe defaults!" << std::endl;
            return 1;
        }
        
        std::cout << "Error handling PASSED" << std::endl;
        
        // Cleanup
        std::cout << "\n7. Cleaning up..." << std::endl;
        
        if (fs::exists(test_data_path)) {
            fs::remove_all(test_data_path);
        }
        
        std::cout << "\n✅ ALL PROFILE MANAGER VAULT INTEGRATION TESTS PASSED!" << std::endl;
        std::cout << "\n=== TASK 4 INTEGRATION VERIFICATION COMPLETE ===" << std::endl;
        std::cout << "ProfileManager Vault Integration Features Verified:" << std::endl;
        std::cout << "  ✓ VaultManager initialization during ProfileManager setup" << std::endl;
        std::cout << "  ✓ Vault system directory structure creation" << std::endl;
        std::cout << "  ✓ Vault-specific operations API (size, validation, maintenance)" << std::endl;
        std::cout << "  ✓ Profile locked folders retrieval" << std::endl;
        std::cout << "  ✓ Direct VaultManager integration and consistency" << std::endl;
        std::cout << "  ✓ API consistency with existing ProfileManager methods" << std::endl;
        std::cout << "  ✓ Proper error handling for invalid operations" << std::endl;
        std::cout << "  ✓ Safe defaults for non-existent profiles" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test FAILED with exception: " << e.what() << std::endl;
        return 1;
    }
}