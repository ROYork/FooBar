cmake_minimum_required(VERSION 3.16)

# Note: fb_net target is available from parent directory

# Try to find GTest
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    # Download and build GTest if not found
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )
    
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(googletest)
    
    # Create aliases to match find_package behavior
    add_library(GTest::GTest ALIAS gtest)
    add_library(GTest::Main ALIAS gtest_main)
    add_library(GTest::GMock ALIAS gmock)
    add_library(GTest::GMockMain ALIAS gmock_main)
endif()

# Enable testing
enable_testing()

# Set C++ standard
include(FBDetermineCxxStandard)
fb_configure_cxx_standard("fb_net/tests")

# Compiler flags
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    # Disable specific warnings for tests
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996")  # Disable deprecated warnings
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")  # GTest uses unused parameters
endif()

# Test sources
set(TEST_SOURCES
    test_main.cpp
    test_socket_address.cpp
    test_stream_socket.cpp
    test_server_socket.cpp
    test_datagram_socket.cpp
    test_socket_stream.cpp
    test_poll_set.cpp
    test_tcp_server.cpp
    test_udp_server.cpp
    test_signal_integration.cpp
)

# Create the test executable
add_executable(fb_net_unit_tests ${TEST_SOURCES})

# Link libraries
target_link_libraries(fb_net_unit_tests
    PRIVATE
        fb_net
        GTest::GTest
        GTest::Main
)

# Add test to CTest
add_test(
    NAME fb_net_unit_tests
    COMMAND fb_net_unit_tests
)

# Test properties
set_tests_properties(fb_net_unit_tests PROPERTIES
    TIMEOUT 120
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Group test files for IDEs
source_group("Tests" FILES ${TEST_SOURCES})

# Coverage support (if available)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(fb_net_unit_tests PRIVATE --coverage)
        target_link_options(fb_net_unit_tests PRIVATE --coverage)
    endif()
endif()

# Installation
install(TARGETS fb_net_unit_tests
    DESTINATION bin
)

# Test data files (if any)
file(GLOB TEST_DATA_FILES "data/*")
if(TEST_DATA_FILES)
    install(FILES ${TEST_DATA_FILES}
        DESTINATION share/fb_net/test_data
    )
endif()
