# PhantomVault Comprehensive Test Suite CMake Configuration

cmake_minimum_required(VERSION 3.16)
project(PhantomVaultTests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags for testing
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g -O2")

# Add debug flags for testing
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -fsanitize=address -fsanitize=undefined")

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Platform-specific definitions
if(UNIX AND NOT APPLE)
    add_definitions(-DPLATFORM_LINUX)
elseif(APPLE)
    add_definitions(-DPLATFORM_MACOS)
elseif(WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
endif()

# Core source files (needed for testing)
set(CORE_SOURCES
    ../src/encryption_engine.cpp
    ../src/profile_vault.cpp
    ../src/profile_manager.cpp
    ../src/folder_security_manager.cpp
    ../src/privilege_manager.cpp
    ../src/error_handler.cpp
    ../src/vault_handler.cpp
    ../src/platform_adapter.cpp
    ../src/keyboard_sequence_detector.cpp
    ../src/analytics_engine.cpp
)

# Test framework sources
set(TEST_FRAMEWORK_SOURCES
    test_framework.cpp
)

# Test suite sources
set(TEST_SOURCES
    test_encryption_engine.cpp
    test_profile_vault_integration.cpp
    test_security_compliance.cpp
    test_performance.cpp
    comprehensive_test_suite.cpp
)

# Create test framework library
add_library(test_framework STATIC ${TEST_FRAMEWORK_SOURCES})
target_link_libraries(test_framework ${CMAKE_THREAD_LIBS_INIT})

# Create comprehensive test suite executable
add_executable(comprehensive_test_suite 
    ${TEST_SOURCES}
    ${CORE_SOURCES}
)

# Link libraries
target_link_libraries(comprehensive_test_suite
    test_framework
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CMAKE_THREAD_LIBS_INIT}
)

# Platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(comprehensive_test_suite X11 pthread)
elseif(APPLE)
    target_link_libraries(comprehensive_test_suite "-framework Security" "-framework CoreFoundation")
elseif(WIN32)
    target_link_libraries(comprehensive_test_suite advapi32 user32 shell32)
endif()

# Individual test executables (optional, for focused testing)
add_executable(test_encryption_engine
    test_encryption_engine.cpp
    ../src/encryption_engine.cpp
    test_framework.cpp
)
target_link_libraries(test_encryption_engine OpenSSL::SSL OpenSSL::Crypto ${CMAKE_THREAD_LIBS_INIT})

add_executable(test_profile_vault_integration
    test_profile_vault_integration.cpp
    ../src/profile_vault.cpp
    ../src/encryption_engine.cpp
    ../src/profile_manager.cpp
    ../src/folder_security_manager.cpp
    test_framework.cpp
)
target_link_libraries(test_profile_vault_integration OpenSSL::SSL OpenSSL::Crypto ${CMAKE_THREAD_LIBS_INIT})

add_executable(test_security_compliance
    test_security_compliance.cpp
    ../src/encryption_engine.cpp
    ../src/profile_manager.cpp
    ../src/privilege_manager.cpp
    ../src/error_handler.cpp
    test_framework.cpp
)
target_link_libraries(test_security_compliance OpenSSL::SSL OpenSSL::Crypto ${CMAKE_THREAD_LIBS_INIT})

add_executable(test_performance
    test_performance.cpp
    ../src/encryption_engine.cpp
    ../src/profile_vault.cpp
    ../src/folder_security_manager.cpp
    test_framework.cpp
)
target_link_libraries(test_performance OpenSSL::SSL OpenSSL::Crypto ${CMAKE_THREAD_LIBS_INIT})

# Test targets
enable_testing()

# Add comprehensive test
add_test(NAME ComprehensiveTestSuite COMMAND comprehensive_test_suite)

# Add individual test categories
add_test(NAME EncryptionEngineTests COMMAND comprehensive_test_suite EncryptionEngine)
add_test(NAME ProfileVaultTests COMMAND comprehensive_test_suite ProfileVault)
add_test(NAME SecurityTests COMMAND comprehensive_test_suite Security)
add_test(NAME PerformanceTests COMMAND comprehensive_test_suite Performance)

# Custom targets for easy testing
add_custom_target(test_all
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS comprehensive_test_suite
    COMMENT "Running all PhantomVault tests"
)

add_custom_target(test_quick
    COMMAND comprehensive_test_suite EncryptionEngine
    DEPENDS comprehensive_test_suite
    COMMENT "Running quick encryption engine tests"
)

add_custom_target(test_security
    COMMAND comprehensive_test_suite Security
    DEPENDS comprehensive_test_suite
    COMMENT "Running security compliance tests"
)

add_custom_target(test_performance
    COMMAND comprehensive_test_suite Performance
    DEPENDS comprehensive_test_suite
    COMMENT "Running performance tests"
)

# Install test executables (optional)
install(TARGETS comprehensive_test_suite
    RUNTIME DESTINATION bin/tests
)

# Print configuration summary
message(STATUS "PhantomVault Test Suite Configuration:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  OpenSSL Version: ${OPENSSL_VERSION}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "")
message(STATUS "Available test targets:")
message(STATUS "  make test_all        - Run all tests")
message(STATUS "  make test_quick      - Run quick tests")
message(STATUS "  make test_security   - Run security tests")
message(STATUS "  make test_performance - Run performance tests")
message(STATUS "")
message(STATUS "Test executables will be built in: ${CMAKE_BINARY_DIR}")