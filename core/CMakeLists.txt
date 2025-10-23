# Core library CMake configuration
cmake_minimum_required(VERSION 3.16)
project(PhantomVaultCore VERSION 1.0.4 LANGUAGES CXX)

# Set CMake policies to suppress warnings
cmake_policy(SET CMP0012 NEW)

find_package(OpenSSL REQUIRED)
find_package(GTest REQUIRED)

# Add X11 dependencies
find_package(X11 REQUIRED)
find_package(X11 COMPONENTS Xtst Xi REQUIRED)

# Add Qt dependencies
find_package(Qt5 COMPONENTS Core Widgets Test REQUIRED)

include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

# Enable Qt MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(phantom_vault_core SHARED
    src/core.cpp
    src/encryption.cpp
    src/filesystem_linux.cpp
    src/storage.cpp
    src/keyboard_hook_linux.cpp
    src/system_tray_linux.cpp
    src/startup_manager_linux.cpp
    src/process_concealer_linux.cpp
    src/service_simple.cpp
    src/hotkey_manager.cpp
    src/input_overlay.cpp
    src/sequence_detector.cpp
    src/service_vault_manager.cpp
    src/recovery_manager.cpp
    src/directory_protection.cpp
    src/ipc_server.cpp
    src/vault_metadata_manager.cpp
    src/vault_encryption_manager.cpp
    src/vault_storage_manager.cpp
    src/version.cpp
)

# Add include directories
target_include_directories(phantom_vault_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${OPENSSL_INCLUDE_DIR}
)

# Set up modern compile features
target_compile_features(phantom_vault_core PUBLIC cxx_std_20)

# Add compile definitions
target_compile_definitions(phantom_vault_core
    PRIVATE
        PHANTOM_VAULT_CORE_EXPORTS
    PUBLIC
        $<$<CONFIG:Debug>:PHANTOM_VAULT_DEBUG>
)

# Find threads package
find_package(Threads REQUIRED)

# Link dependencies
target_link_libraries(phantom_vault_core
    PRIVATE
        OpenSSL::Crypto
        OpenSSL::SSL
        nlohmann_json::nlohmann_json
        X11::X11
        X11::Xtst
        X11::Xi
        Qt5::Core
        Qt5::Widgets
        Threads::Threads
)

# Set up export header
include(GenerateExportHeader)
generate_export_header(phantom_vault_core
    BASE_NAME PHANTOM_VAULT
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/include/phantom_vault_export.h
)

# Add version header
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/phantom_vault/version.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/phantom_vault/version.hpp
)

# Add service executable
add_executable(phantom_vault_service
    src/service_main.cpp
)

target_link_libraries(phantom_vault_service
    PRIVATE
        phantom_vault_core
)

# Add tests
add_subdirectory(tests) 