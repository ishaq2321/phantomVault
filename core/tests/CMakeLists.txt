# Enable testing
enable_testing()

# Find GTest package
find_package(GTest REQUIRED)

# Core tests
add_executable(core_tests
    core_test.cpp
    encryption_test.cpp
    filesystem_test.cpp
    storage_test.cpp
    recovery_test.cpp
    integration_test.cpp
    performance_test.cpp
    keyboard_hook_test.cpp
    system_tray_test.cpp
    startup_manager_test.cpp
    process_concealer_test.cpp
    sequence_detector_test.cpp
    test_vault_metadata_manager.cpp
    test_vault_encryption_manager.cpp
    test_vault_storage_manager.cpp
)

target_link_libraries(core_tests
    PRIVATE
        phantom_vault_core
        GTest::gtest
        GTest::gtest_main
        nlohmann_json::nlohmann_json
        X11::X11
        X11::Xtst
        X11::Xi
        Qt5::Core
        Qt5::Widgets
        Qt5::Test
)

# Add tests to CTest
add_test(NAME CoreTests COMMAND core_tests) 