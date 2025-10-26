cmake_minimum_required(VERSION 3.16)
project(PhantomVault VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -Wall")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od /Wall")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
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

# Find or fetch nlohmann/json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    # If not found, we'll use the header-only version
    message(STATUS "nlohmann/json not found, using header-only version")
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# Include directories
include_directories(include)

# Source files
set(CORE_SOURCES
    src/main.cpp
    src/service_manager.cpp
    src/profile_manager.cpp
    src/folder_security_manager.cpp
    src/keyboard_sequence_detector.cpp
    src/analytics_engine.cpp
    src/ipc_server.cpp
    src/memory_manager.cpp
    src/performance_monitor.cpp
)

# Platform-specific sources (will be added in later phases)
# For now, platform-specific code is included in the main implementations

# Create executable
add_executable(phantomvault-service ${CORE_SOURCES})

# Link libraries
target_link_libraries(phantomvault-service
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

# Link nlohmann/json
if(nlohmann_json_FOUND)
    target_link_libraries(phantomvault-service nlohmann_json::nlohmann_json)
else()
    target_link_libraries(phantomvault-service nlohmann_json)
endif()

# Platform-specific libraries
if(PLATFORM_LINUX)
    target_link_libraries(phantomvault-service
        X11
        Xext
        stdc++fs
    )
elseif(PLATFORM_MACOS)
    target_link_libraries(phantomvault-service
        "-framework CoreFoundation"
        "-framework ApplicationServices"
        "-framework Carbon"
    )
elseif(PLATFORM_WINDOWS)
    target_link_libraries(phantomvault-service
        user32
        kernel32
        advapi32
        shell32
    )
endif()

# Set output directory
set_target_properties(phantomvault-service PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install targets
install(TARGETS phantomvault-service
    RUNTIME DESTINATION bin
)

# Create service configuration files
if(PLATFORM_LINUX AND EXISTS ${CMAKE_SOURCE_DIR}/scripts/phantomvault.service.in)
    configure_file(
        ${CMAKE_SOURCE_DIR}/scripts/phantomvault.service.in
        ${CMAKE_BINARY_DIR}/phantomvault.service
        @ONLY
    )
    # Only install if we have permissions (optional)
    if(CMAKE_INSTALL_PREFIX STREQUAL "/usr/local" OR CMAKE_INSTALL_PREFIX STREQUAL "/usr")
        install(FILES ${CMAKE_BINARY_DIR}/phantomvault.service
            DESTINATION /etc/systemd/system
            OPTIONAL
        )
    endif()
endif()

# Development and testing
option(BUILD_TESTS "Build test suite" ON)
if(BUILD_TESTS AND EXISTS ${CMAKE_SOURCE_DIR}/tests)
    enable_testing()
    add_subdirectory(tests)
endif()

# Documentation
option(BUILD_DOCS "Build documentation" OFF)
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    endif()
endif()

# Print configuration summary
message(STATUS "PhantomVault Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  OpenSSL Version: ${OPENSSL_VERSION}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Build docs: ${BUILD_DOCS}")