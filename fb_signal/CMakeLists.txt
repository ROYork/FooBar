cmake_minimum_required(VERSION 3.26)
project(fb_signal VERSION 1.0.0 LANGUAGES CXX)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Option to enable warnings as errors (default ON for standalone, OFF when consumed)
option(FB_SIGNAL_WERROR "Treat warnings as errors" OFF)

# Only apply strict flags when this is the top-level project
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(FB_SIGNAL_WERROR ON CACHE BOOL "" FORCE)
endif()

# Compiler-specific flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
  add_compile_options(-Wno-unused-parameter)
  add_compile_options(-fno-exceptions)

  if(FB_SIGNAL_WERROR)
    add_compile_options(-Werror)
  endif()

  # Release optimizations
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -march=native)
  endif()

  # ThreadSanitizer support
  if(ENABLE_TSAN)
    message(STATUS "ThreadSanitizer enabled.")
    set(FB_SIGNAL_TSAN_ENABLED TRUE)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
  endif()
elseif(MSVC)
  add_compile_options(/W4)
  add_compile_options(/EHs-c-)

  if(FB_SIGNAL_WERROR)
    add_compile_options(/WX)
  endif()

  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(/O2)
  endif()
endif()

# Header-only library
add_library(fb_signal INTERFACE)
target_include_directories(fb_signal INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Require threads
find_package(Threads REQUIRED)
target_link_libraries(fb_signal INTERFACE Threads::Threads)

# Tests
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(ut)
endif()

# Benchmarks
option(BUILD_BENCH "Build benchmarks" ON)
if(BUILD_BENCH)
  add_subdirectory(bench)
endif()
