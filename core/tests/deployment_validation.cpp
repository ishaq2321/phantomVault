/**
 * @file deployment_validation.cpp
 * @brief Deployment validation for PhantomVault production readiness
 * 
 * Validates build system, installation requirements, and deployment readiness
 */

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <cstdlib>

namespace fs = std::filesystem;

class DeploymentValidator {
public:
    struct ValidationResult {
        std::string test_name;
        bool passed;
        std::string message;
        std::chrono::milliseconds duration;
    };
    
    struct ValidationSummary {
        size_t total_tests = 0;
        size_t passed_tests = 0;
        size_t failed_tests = 0;
        std::vector<ValidationResult> results;
        
        double success_rate() const {
            return total_tests > 0 ? (double)passed_tests / total_tests * 100.0 : 0.0;
        }
    };
    
private:
    ValidationSummary summary_;
    
    void addResult(const std::string& name, bool passed, const std::string& message, 
                   std::chrono::milliseconds duration) {
        ValidationResult result;
        result.test_name = name;
        result.passed = passed;
        result.message = message;
        result.duration = duration;
        
        summary_.results.push_back(result);
        summary_.total_tests++;
        
        if (passed) {
            summary_.passed_tests++;
        } else {
            summary_.failed_tests++;
        }
    }
    
    std::chrono::milliseconds getDuration(const std::chrono::high_resolution_clock::time_point& start) {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    }
    
public:
    ValidationSummary runDeploymentValidation() {
        std::cout << "🚀 Running Deployment Validation..." << std::endl;
        std::cout << "====================================" << std::endl;
        
        validateBuildSystem();
        validateDependencies();
        validateDirectoryStructure();
        validateConfigurationFiles();
        validateInstallationRequirements();
        validateServiceConfiguration();
        validateSecurityRequirements();
        validateDocumentation();
        
        return summary_;
    }
    
private:
    void validateBuildSystem() {
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            // Check CMakeLists.txt exists
            if (!fs::exists("core/CMakeLists.txt")) {
                addResult("BuildSystem", false, "CMakeLists.txt not found", getDuration(start));
                return;
            }
            
            // Check build directory structure
            if (!fs::exists("core/build")) {
                addResult("BuildSystem", false, "Build directory not found", getDuration(start));
                return;
            }
            
            // Check if Makefile exists (indicating successful CMake configuration)
            if (!fs::exists("core/build/Makefile")) {
                addResult("BuildSystem", false, "Makefile not found - CMake configuration may have failed", getDuration(start));
                return;
            }
            
            // Check for essential build files
            std::vector<std::string> required_files = {
                "core/build/CMakeCache.txt",
                "core/build/cmake_install.cmake"
            };
            
            for (const auto& file : required_files) {
                if (!fs::exists(file)) {
                    addResult("BuildSystem", false, "Required build file missing: " + file, getDuration(start));
                    return;
                }
            }
            
            addResult("BuildSystem", true, "Build system configuration validated", getDuration(start));
            
        } catch (const std::exception& e) {
            addResult("BuildSystem", false, "Exception: " + std::string(e.what()), getDuration(start));
        }
    }
    
    void validateDependencies() {
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            std::vector<std::string> missing_deps;
            
            // Check for OpenSSL headers
            if (!fs::exists("/usr/include/openssl/evp.h") && 
                !fs::exists("/usr/local/include/openssl/evp.h")) {
                missing_deps.push_back("OpenSSL development headers");
            }
            
            // Check for nlohmann/json
            if (!fs::exists("/usr/include/nlohmann/json.hpp") &&
                !fs::exists("/usr/local/include/nlohmann/json.hpp")) {
                missing_deps.push_back("nlohmann/json library");
            }
            
            // Check for pthread support (should be available on Linux)
            if (!fs::exists("/usr/include/pthread.h")) {
                missing_deps.push_back("pthread library");
            }
            
            if (!missing_deps.empty()) {
                std::string message = "Missing dependencies: ";
                for (size_t i = 0; i < missing_deps.size(); ++i) {
                    message += missing_deps[i];
                    if (i < missing_deps.size() - 1) message += ", ";
                }
                addResult("Dependencies", false, message, getDuration(start));
                return;
            }
            
            addResult("Dependencies", true, "All required dependencies available", getDuration(start));
            
        } catch (const std::exception& e) {
            addResult("Dependencies", false, "Exception: " + std::string(e.what()), getDuration(start));
        }
    }
    
    void validateDirectoryStructure() {
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            std::vector<std::string> required_dirs = {
                "core",
                "core/src",
                "core/include",
                "core/tests",
                "core/build",
                "src",
                "gui",
                "installer",
                "docs"
            };
            
            std::vector<std::string> missing_dirs;
            
            for (const auto& dir : required_dirs) {
                if (!fs::exists(dir) || !fs::is_directory(dir)) {
                    missing_dirs.push_back(dir);
                }
            }
            
            if (!missing_dirs.empty()) {
                std::string message = "Missing directories: ";
                for (size_t i = 0; i < missing_dirs.size(); ++i) {
                    message += missing_dirs[i];
                    if (i < missing_dirs.size() - 1) message += ", ";
                }
                addResult("DirectoryStructure", false, message, getDuration(start));
                return;
            }
            
            // Check for essential source files
            std::vector<std::string> essential_files = {
                "core/src/encryption_engine.cpp",
                "core/src/vault_handler.cpp",
                "core/src/profile_manager.cpp",
                "core/include/encryption_engine.hpp",
                "core/include/vault_handler.hpp",
                "core/include/profile_manager.hpp"
            };
            
            std::vector<std::string> missing_files;
            
            for (const auto& file : essential_files) {
                if (!fs::exists(file)) {
                    missing_files.push_back(file);
                }
            }
            
            if (!missing_files.empty()) {
                std::string message = "Missing essential files: ";
                for (size_t i = 0; i < missing_files.size(); ++i) {
                    message += missing_files[i];
                    if (i < missing_files.size() - 1) message += ", ";
                }
                addResult("DirectoryStructure", false, message, getDuration(start));
                return;
            }
            
            addResult("DirectoryStructure", true, "Directory structure validated", getDuration(start));
            
        } catch (const std::exception& e) {
            addResult("DirectoryStructure", false, "Exception: " + std::string(e.what()), getDuration(start));
        }
    }
    
    void validateConfigurationFiles() {
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            std::vector<std::string> config_files = {
                "core/CMakeLists.txt",
                "CMakeLists.txt"
            };
            
            std::vector<std::string> missing_configs;
            
            for (const auto& config : config_files) {
                if (!fs::exists(config)) {
                    missing_configs.push_back(config);
                }
            }
            
            if (!missing_configs.empty()) {
                std::string message = "Missing configuration files: ";
                for (size_t i = 0; i < missing_configs.size(); ++i) {
                    message += missing_configs[i];
                    if (i < missing_configs.size() - 1) message += ", ";
                }
                addResult("Configuration", false, message, getDuration(start));
                return;
            }
            
            // Validate CMakeLists.txt content
            std::ifstream cmake_file("core/CMakeLists.txt");
            if (!cmake_file.is_open()) {
                addResult("Configuration", false, "Cannot read core/CMakeLists.txt", getDuration(start));
                return;
            }
            
            std::string cmake_content((std::istreambuf_iterator<char>(cmake_file)),
                                     std::istreambuf_iterator<char>());
            cmake_file.close();
            
            // Check for essential CMake directives
            std::vector<std::string> required_directives = {
                "cmake_minimum_required",
                "project",
                "find_package",
                "add_executable"
            };
            
            for (const auto& directive : required_directives) {
                if (cmake_content.find(directive) == std::string::npos) {
                    addResult("Configuration", false, "CMakeLists.txt missing directive: " + directive, getDuration(start));
                    return;
                }
            }
            
            addResult("Configuration", true, "Configuration files validated", getDuration(start));
            
        } catch (const std::exception& e) {
            addResult("Configuration", false, "Exception: " + std::string(e.what()), getDuration(start));
        }
    }
    
    void validateInstallationRequirements() {
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            // Check system requirements
            std::vector<std::pair<std::string, std::string>> requirements = {
                {"g++", "C++ compiler"},
                {"cmake", "Build system"},
                {"make", "Build tool"},
                {"pkg-config", "Package configuration"}
            };
            
            std::vector<std::string> missing_tools;
            
            for (const auto& req : requirements) {
                std::string command = "which " + req.first + " > /dev/null 2>&1";
                int result = std::system(command.c_str());
                
                if (result != 0) {
                    missing_tools.push_back(req.second + " (" + req.first + ")");
                }
            }
            
            if (!missing_tools.empty()) {
                std::string message = "Missing build tools: ";
                for (size_t i = 0; i < missing_tools.size(); ++i) {
                    message += missing_tools[i];
                    if (i < missing_tools.size() - 1) message += ", ";
                }
                addResult("Installation", false, message, getDuration(start));
                return;
            }
            
            // Check disk space (should have at least 100MB free)
            auto space = fs::space(".");
            auto free_mb = space.free / (1024 * 1024);
            
            if (free_mb < 100) {
                addResult("Installation", false, "Insufficient disk space: " + std::to_string(free_mb) + "MB free", getDuration(start));
                return;
            }
            
            addResult("Installation", true, "Installation requirements satisfied (" + std::to_string(free_mb) + "MB free)", getDuration(start));
            
        } catch (const std::exception& e) {
            addResult("Installation", false, "Exception: " + std::string(e.what()), getDuration(start));
        }
    }
    
    void validateServiceConfiguration() {
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            // Check for service configuration files
            std::vector<std::string> service_files = {
                "core/scripts/phantomvault.service.in",
                "core/build/phantomvault.service"
            };
            
            bool has_service_config = false;
            for (const auto& file : service_files) {
                if (fs::exists(file)) {
                    has_service_config = true;
                    break;
                }
            }
            
            if (!has_service_config) {
                addResult("ServiceConfig", false, "No service configuration files found", getDuration(start));
                return;
            }
            
            // Check if systemd is available (for Linux deployment)
            if (fs::exists("/bin/systemctl") || fs::exists("/usr/bin/systemctl")) {
                addResult("ServiceConfig", true, "Service configuration available (systemd detected)", getDuration(start));
            } else {
                addResult("ServiceConfig", true, "Service configuration available (systemd not detected)", getDuration(start));
            }
            
        } catch (const std::exception& e) {
            addResult("ServiceConfig", false, "Exception: " + std::string(e.what()), getDuration(start));
        }
    }
    
    void validateSecurityRequirements() {
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            // Check file permissions on sensitive directories
            std::vector<std::string> sensitive_dirs = {
                "core/src",
                "core/include"
            };
            
            for (const auto& dir : sensitive_dirs) {
                if (fs::exists(dir)) {
                    auto perms = fs::status(dir).permissions();
                    
                    // Check that directory is not world-writable
                    if ((perms & fs::perms::others_write) != fs::perms::none) {
                        addResult("Security", false, "Directory " + dir + " is world-writable", getDuration(start));
                        return;
                    }
                }
            }
            
            // Check for secure compilation flags in build system
            if (fs::exists("core/build/CMakeCache.txt")) {
                std::ifstream cache_file("core/build/CMakeCache.txt");
                std::string cache_content((std::istreambuf_iterator<char>(cache_file)),
                                         std::istreambuf_iterator<char>());
                cache_file.close();
                
                // Look for security-related compiler flags
                bool has_security_flags = false;
                std::vector<std::string> security_flags = {
                    "-fstack-protector",
                    "-D_FORTIFY_SOURCE",
                    "-fPIE",
                    "-pie"
                };
                
                for (const auto& flag : security_flags) {
                    if (cache_content.find(flag) != std::string::npos) {
                        has_security_flags = true;
                        break;
                    }
                }
                
                if (has_security_flags) {
                    addResult("Security", true, "Security compilation flags detected", getDuration(start));
                } else {
                    addResult("Security", true, "Basic security requirements met (no hardening flags detected)", getDuration(start));
                }
            } else {
                addResult("Security", true, "Basic security requirements met", getDuration(start));
            }
            
        } catch (const std::exception& e) {
            addResult("Security", false, "Exception: " + std::string(e.what()), getDuration(start));
        }
    }
    
    void validateDocumentation() {
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            std::vector<std::string> doc_files = {
                "README.md",
                "docs"
            };
            
            std::vector<std::string> missing_docs;
            
            for (const auto& doc : doc_files) {
                if (!fs::exists(doc)) {
                    missing_docs.push_back(doc);
                }
            }
            
            if (missing_docs.size() == doc_files.size()) {
                addResult("Documentation", false, "No documentation found", getDuration(start));
                return;
            }
            
            // Check README.md content if it exists
            if (fs::exists("README.md")) {
                std::ifstream readme("README.md");
                std::string readme_content((std::istreambuf_iterator<char>(readme)),
                                          std::istreambuf_iterator<char>());
                readme.close();
                
                if (readme_content.length() < 100) {
                    addResult("Documentation", false, "README.md appears incomplete", getDuration(start));
                    return;
                }
            }
            
            addResult("Documentation", true, "Documentation available", getDuration(start));
            
        } catch (const std::exception& e) {
            addResult("Documentation", false, "Exception: " + std::string(e.what()), getDuration(start));
        }
    }
};

int main() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════════════════════╗
║                    PhantomVault Deployment Validation                        ║
║                                                                              ║
║  Comprehensive validation for production deployment readiness                ║
╚══════════════════════════════════════════════════════════════════════════════╝
)" << std::endl;
    
    DeploymentValidator validator;
    auto summary = validator.runDeploymentValidation();
    
    std::cout << "\n📊 DEPLOYMENT VALIDATION RESULTS" << std::endl;
    std::cout << "=================================" << std::endl;
    
    for (const auto& result : summary.results) {
        std::string status = result.passed ? "✅ PASS" : "❌ FAIL";
        std::cout << status << " " << std::setw(18) << std::left << result.test_name 
                  << " (" << std::setw(4) << result.duration.count() << "ms) - " 
                  << result.message << std::endl;
    }
    
    std::cout << "\n📈 DEPLOYMENT SUMMARY" << std::endl;
    std::cout << "=====================" << std::endl;
    std::cout << "Total Validations: " << summary.total_tests << std::endl;
    std::cout << "Passed: " << summary.passed_tests << std::endl;
    std::cout << "Failed: " << summary.failed_tests << std::endl;
    std::cout << "Success Rate: " << std::fixed << std::setprecision(1) << summary.success_rate() << "%" << std::endl;
    
    if (summary.success_rate() >= 90.0) {
        std::cout << "\n🎉 DEPLOYMENT READY! All critical deployment requirements validated." << std::endl;
        std::cout << "✅ Build system configured" << std::endl;
        std::cout << "✅ Dependencies available" << std::endl;
        std::cout << "✅ Directory structure complete" << std::endl;
        std::cout << "✅ Configuration files present" << std::endl;
        std::cout << "✅ Installation requirements met" << std::endl;
        std::cout << "✅ Service configuration available" << std::endl;
        std::cout << "✅ Security requirements satisfied" << std::endl;
        std::cout << "✅ Documentation present" << std::endl;
        return 0;
    } else if (summary.success_rate() >= 75.0) {
        std::cout << "\n⚠️  MOSTLY READY: Some deployment requirements need attention." << std::endl;
        return 1;
    } else {
        std::cout << "\n🚨 NOT READY: Critical deployment issues found. Address failures before deployment." << std::endl;
        return 2;
    }
}