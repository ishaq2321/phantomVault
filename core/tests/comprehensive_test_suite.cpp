/**
 * PhantomVault Comprehensive Test Suite
 * 
 * Main test runner that executes all test categories:
 * - Unit tests for encryption engine
 * - Integration tests for profile vault system
 * - Security tests for cryptographic compliance
 * - Performance tests for system impact
 */

#include "test_framework.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>

using namespace phantomvault::testing;

// Forward declarations for test registration functions
void registerEncryptionEngineTests(TestFramework& framework);
void registerProfileVaultIntegrationTests(TestFramework& framework);
void registerSecurityComplianceTests(TestFramework& framework);
void registerSecurityPenetrationTests(TestFramework& framework);
void registerIntegrationTests(TestFramework& framework);
void registerPerformanceTests(TestFramework& framework);
void registerAllComponentsTests(TestFramework& framework);

void printBanner() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════════════════════╗
║                    PhantomVault Comprehensive Test Suite                     ║
║                                                                              ║
║  Testing all components for correctness, security, and performance          ║
║  • Unit Tests: Encryption Engine correctness and security                   ║
║  • Integration Tests: Profile vault isolation and access control            ║
║  • Security Tests: Cryptographic compliance and attack resistance           ║
║  • Performance Tests: Encryption operations and system impact               ║
╚══════════════════════════════════════════════════════════════════════════════╝
)" << std::endl;
}

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options] [test_category]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -v, --verbose          Enable verbose output" << std::endl;
    std::cout << "  -s, --stop-on-failure  Stop on first test failure" << std::endl;
    std::cout << "  -h, --help             Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Test Categories:" << std::endl;
    std::cout << "  EncryptionEngine       Unit tests for encryption engine" << std::endl;
    std::cout << "  ProfileVault           Integration tests for profile vault system" << std::endl;
    std::cout << "  Security               Security and cryptographic compliance tests" << std::endl;
    std::cout << "  SecurityPenetration    Advanced security and penetration tests" << std::endl;
    std::cout << "  Integration            Comprehensive integration tests" << std::endl;
    std::cout << "  Performance            Performance and system impact tests" << std::endl;
    std::cout << "  AllComponents          All component tests" << std::endl;
    std::cout << "  all                    Run all test categories (default)" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program_name << "                    # Run all tests" << std::endl;
    std::cout << "  " << program_name << " -v Security        # Run security tests with verbose output" << std::endl;
    std::cout << "  " << program_name << " -s EncryptionEngine # Run encryption tests, stop on failure" << std::endl;
}

int main(int argc, char* argv[]) {
    printBanner();
    
    // Parse command line arguments
    bool verbose = false;
    bool stop_on_failure = false;
    std::string test_category = "all";
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-s" || arg == "--stop-on-failure") {
            stop_on_failure = true;
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg[0] != '-') {
            test_category = arg;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Initialize test framework
    TestFramework framework;
    framework.setVerbose(verbose);
    framework.setStopOnFailure(stop_on_failure);
    
    // Register all test suites
    std::cout << "Registering test suites..." << std::endl;
    
    try {
        registerEncryptionEngineTests(framework);
        std::cout << "✓ Encryption Engine tests registered" << std::endl;
        
        registerProfileVaultIntegrationTests(framework);
        std::cout << "✓ Profile Vault integration tests registered" << std::endl;
        
        registerSecurityComplianceTests(framework);
        std::cout << "✓ Security compliance tests registered" << std::endl;
        
        registerSecurityPenetrationTests(framework);
        std::cout << "✓ Security penetration tests registered" << std::endl;
        
        registerIntegrationTests(framework);
        std::cout << "✓ Integration tests registered" << std::endl;
        
        registerPerformanceTests(framework);
        std::cout << "✓ Performance tests registered" << std::endl;
        
        registerAllComponentsTests(framework);
        std::cout << "✓ All components tests registered" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to register tests: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << std::endl;
    
    // Run tests
    bool success = false;
    
    try {
        if (test_category == "all") {
            std::cout << "Running all test categories..." << std::endl;
            success = framework.runAllTests();
        } else {
            std::cout << "Running test category: " << test_category << std::endl;
            success = framework.runCategory(test_category);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Test execution failed: " << e.what() << std::endl;
        return 1;
    }
    
    // Print detailed results if verbose
    if (verbose) {
        std::cout << std::endl;
        framework.printResults();
    }
    
    // Print final summary
    std::cout << std::endl;
    auto stats = framework.getStats();
    
    std::cout << "╔══════════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                           FINAL TEST RESULTS                                ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║ Total Tests:    " << std::setw(8) << stats.total_tests << "                                                   ║" << std::endl;
    std::cout << "║ Passed:         " << std::setw(8) << stats.passed_tests << "                                                   ║" << std::endl;
    std::cout << "║ Failed:         " << std::setw(8) << stats.failed_tests << "                                                   ║" << std::endl;
    std::cout << "║ Errors:         " << std::setw(8) << stats.error_tests << "                                                   ║" << std::endl;
    std::cout << "║ Skipped:        " << std::setw(8) << stats.skipped_tests << "                                                   ║" << std::endl;
    std::cout << "║ Pass Rate:      " << std::setw(6) << std::fixed << std::setprecision(1) << stats.pass_rate() << "%                                                  ║" << std::endl;
    std::cout << "║ Duration:       " << std::setw(8) << stats.total_duration.count() << "ms                                                ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════════════════════╝" << std::endl;
    
    if (success) {
        std::cout << std::endl;
        std::cout << "🎉 ALL TESTS PASSED! PhantomVault is ready for production." << std::endl;
        std::cout << std::endl;
        std::cout << "Test Coverage Summary:" << std::endl;
        std::cout << "✅ Encryption Engine: AES-256-CBC implementation verified" << std::endl;
        std::cout << "✅ Profile Vault System: Isolation and access control validated" << std::endl;
        std::cout << "✅ Security Compliance: Cryptographic standards met" << std::endl;
        std::cout << "✅ Performance: System impact within acceptable limits" << std::endl;
        std::cout << std::endl;
        std::cout << "PhantomVault has passed comprehensive testing and is production-ready!" << std::endl;
        return 0;
    } else {
        std::cout << std::endl;
        std::cout << "❌ TESTS FAILED! Please review the failures above." << std::endl;
        std::cout << std::endl;
        std::cout << "Failed tests indicate issues that must be resolved before production deployment." << std::endl;
        std::cout << "Review the test output above for specific failure details." << std::endl;
        return 1;
    }
}

// Test registration functions are implemented in their respective test files:
// - registerEncryptionEngineTests() in test_encryption_engine.cpp
// - registerProfileVaultIntegrationTests() in test_profile_vault_integration.cpp  
// - registerSecurityComplianceTests() in test_security_compliance.cpp
// - registerSecurityPenetrationTests() in test_security_penetration.cpp
// - registerIntegrationTests() in test_integration_comprehensive.cpp
// - registerPerformanceTests() in test_performance.cpp
// - registerAllComponentsTests() in test_all_components.cpp
// The linker will resolve these when all test files are compiled together