# PhantomVault Tests
# Basic test configuration - will be expanded later

cmake_minimum_required(VERSION 3.16)

# For now, just create a placeholder test
add_executable(basic_test basic_test.cpp)
target_link_libraries(basic_test ${CMAKE_THREAD_LIBS_INIT})

# Add test
add_test(NAME BasicTest COMMAND basic_test)