cmake_minimum_required(VERSION 3.16)

# Note: fb_net target is available from parent directory

# Enable C++20/17
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

# Check compiler support
include(CheckCXXCompilerFlag)
if(MSVC)
    check_cxx_compiler_flag("/std:c++20" COMPILER_SUPPORTS_CXX20)
    check_cxx_compiler_flag("/std:c++17" COMPILER_SUPPORTS_CXX17)
else()
    check_cxx_compiler_flag("-std=c++20" COMPILER_SUPPORTS_CXX20)
    check_cxx_compiler_flag("-std=c++17" COMPILER_SUPPORTS_CXX17)
endif()

if(COMPILER_SUPPORTS_CXX20)
    set(CMAKE_CXX_STANDARD 20)
elseif(COMPILER_SUPPORTS_CXX17)
    set(CMAKE_CXX_STANDARD 17)
else()
    message(FATAL_ERROR "Compiler does not support C++17 or C++20")
endif()

# Compiler flags
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif()

# Example 1: Simple HTTP Server
add_executable(http_server http_server/http_server.cpp)
target_link_libraries(http_server fb_net)

# Example 2: Chat Server
add_executable(chat_server chat_server/chat_server.cpp)
target_link_libraries(chat_server fb_net)

add_executable(chat_client chat_server/chat_client.cpp)
target_link_libraries(chat_client fb_net)

# Example 3: UDP Ping Client/Server
add_executable(udp_ping_server udp_ping/udp_ping_server.cpp)
target_link_libraries(udp_ping_server fb_net)

add_executable(udp_ping_client udp_ping/udp_ping_client.cpp)
target_link_libraries(udp_ping_client fb_net)

# Example 4: File Transfer
add_executable(file_server file_transfer/file_server.cpp)
target_link_libraries(file_server fb_net)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
    target_link_libraries(file_server stdc++fs)
endif()

add_executable(file_client file_transfer/file_client.cpp)
target_link_libraries(file_client fb_net)

# Example 5: Port Scanner
add_executable(port_scanner port_scanner/port_scanner.cpp)
target_link_libraries(port_scanner fb_net)

add_executable(signal_demo signal_demo/signal_demo.cpp)
target_link_libraries(signal_demo fb_net)

# Example 6: UDP Signal-Driven Programming
add_executable(udp_signal_demo udp_signal_demo/udp_signal_demo.cpp)
target_link_libraries(udp_signal_demo fb_net)

# Integration test
add_executable(test_integration if_tests/test_integration.cpp)
target_link_libraries(test_integration fb_net)

# compatibility test
add_executable(test_legacy_compat if_tests/test_legacy_compat.cpp)
target_link_libraries(test_legacy_compat fb_net)

# Performance benchmark
add_executable(benchmark_performance if_tests/benchmark_performance.cpp)
target_link_libraries(benchmark_performance fb_net)

# Memory validation test
add_executable(test_memory_validation if_tests/test_memory_validation.cpp)
target_link_libraries(test_memory_validation fb_net)

# Add to test suite
if(FB_NET_BUILD_TESTS)
    add_test(NAME integration_tests COMMAND test_integration)
    add_test(NAME legacy_compatibility COMMAND test_legacy_compat)
    add_test(NAME memory_validation COMMAND test_memory_validation)

    set_tests_properties(integration_tests legacy_compatibility memory_validation PROPERTIES
        TIMEOUT 300
    )
endif()

# Install examples
install(TARGETS
    http_server
    chat_server chat_client
    udp_ping_server udp_ping_client
    file_server file_client
    port_scanner
    signal_demo
    udp_signal_demo
    DESTINATION bin
)

# Install example source files for reference
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION share/fb_net/examples
    FILES_MATCHING PATTERN "*.cpp" PATTERN "*.h" PATTERN "*.md"
)
