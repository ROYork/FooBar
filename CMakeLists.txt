cmake_minimum_required(VERSION 3.26)

# Project configuration
project(FooBar
    VERSION 1.0.0
    DESCRIPTION "A cross-platform C++ utility library"
    LANGUAGES CXX
)

option(FB_FORCE_CXX17 "Force project to build with C++17 even when C++20 is available" OFF)
option(FB_BUILD_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# Professional warning configuration for production builds
if(MSVC)
    # Warning level 4, but allow specific safe patterns
    add_compile_options(/W4)
    # Disable warnings that are acceptable in production code
    add_compile_options(
        /wd4100  # Unreferenced formal parameter (common in interfaces)
        /wd4146  # Unary minus on unsigned (safe mathematical operations)
        /wd4244  # Conversion warnings (handled explicitly where critical)
        /wd4267  # size_t to int conversion
        /wd4324  # Structure padding (intentional for performance)
        /wd4996  # Deprecated function warnings (for platform compatibility)
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wno-unused-parameter      # Common in interface implementations
        -Wno-sign-conversion       # Too noisy for production code
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options(
            -Wno-maybe-uninitialized
            -Wno-null-dereference  # GCC 13+ false positives with std::istreambuf_iterator
            -Wno-array-bounds      # GCC 12+ false positives with runtime bounds checking
        )
    endif()
endif()


# Optionally treat warnings as errors in production builds
if(FB_BUILD_WARNINGS_AS_ERRORS)
    if(MSVC)
        add_compile_options(/WX)
    else()
        add_compile_options(-Werror)
    endif()
endif()


add_subdirectory(fb_strings)
add_subdirectory(fb_signal)
add_subdirectory(fb_core)

enable_testing()
