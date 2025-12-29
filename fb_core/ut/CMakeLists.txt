cmake_minimum_required(VERSION 3.16)

# Note: fb_core target is available from the parent directory.

find_package(GTest QUIET)

if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)

    add_library(GTest::GTest ALIAS gtest)
    add_library(GTest::Main ALIAS gtest_main)
endif()

enable_testing()

include(FBDetermineCxxStandard)
fb_configure_cxx_standard("fb_core/tests")

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wno-unused-parameter")
endif()

set(FB_CORE_TEST_SOURCES
    test_main.cpp
    test_library_info.cpp
    test_stop_watch.cpp
    test_timer.cpp
    test_circular_buffer.cpp
)

add_executable(fb_core_unit_tests ${FB_CORE_TEST_SOURCES})

target_link_libraries(fb_core_unit_tests
    PRIVATE
        fb_core
        GTest::GTest
        GTest::Main
)

# Link threading library for timer tests
find_package(Threads REQUIRED)
target_link_libraries(fb_core_unit_tests PRIVATE Threads::Threads)

add_test(NAME fb_core_unit_tests COMMAND fb_core_unit_tests)

install(TARGETS fb_core_unit_tests DESTINATION bin)
gtest_discover_tests(fb_core_unit_tests DISCOVERY_TIMEOUT 10)