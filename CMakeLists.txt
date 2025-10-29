cmake_minimum_required(VERSION 3.16)
project(PhantomVault-Unified VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Platform detection
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    add_definitions(-DPLATFORM_WINDOWS)
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
    add_definitions(-DPLATFORM_MACOS)
elseif(UNIX)
    set(PLATFORM_LINUX TRUE)
    add_definitions(-DPLATFORM_LINUX)
endif()

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(core/include)
include_directories(src)

# Core library sources (without main.cpp)
set(CORE_SOURCES
    core/src/service_manager.cpp
    core/src/profile_manager.cpp
    core/src/folder_security_manager.cpp
    core/src/keyboard_sequence_detector.cpp
    core/src/enhanced_keyboard_detector.cpp
    core/src/analytics_engine.cpp
    core/src/ipc_server.cpp
    core/src/memory_manager.cpp
    core/src/performance_monitor.cpp
    core/src/encryption_engine.cpp
    core/src/profile_vault.cpp
    core/src/vault_handler.cpp
    core/src/error_handler.cpp
    core/src/privilege_manager.cpp
    core/src/platform_adapter.cpp
)

# Unified application sources
set(APP_SOURCES
    src/main.cpp
    src/phantomvault_application.cpp
)

# Create unified executable
add_executable(phantomvault ${APP_SOURCES} ${CORE_SOURCES})

# Link libraries
target_link_libraries(phantomvault
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

# Platform-specific libraries
if(PLATFORM_LINUX)
    target_link_libraries(phantomvault
        X11
        Xext
        stdc++fs
    )
elseif(PLATFORM_MACOS)
    target_link_libraries(phantomvault
        "-framework CoreFoundation"
        "-framework ApplicationServices"
        "-framework Carbon"
    )
elseif(PLATFORM_WINDOWS)
    target_link_libraries(phantomvault
        user32
        kernel32
        advapi32
        shell32
    )
endif()

# Set output directory
set_target_properties(phantomvault PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install targets
install(TARGETS phantomvault
    RUNTIME DESTINATION bin
)

# Print configuration summary
message(STATUS "PhantomVault Unified Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  OpenSSL Version: ${OPENSSL_VERSION}")