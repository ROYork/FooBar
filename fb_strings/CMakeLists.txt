cmake_minimum_required(VERSION 3.26)

project(fb_strings VERSION 1.0.0 LANGUAGES CXX)

# C++17 standard required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3)
  endif()
elseif(MSVC)
  add_compile_options(/W4 /WX)
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(/O2)
  endif()
endif()

# Library sources
set(FB_STRINGS_SOURCES
  src/string_utils.cpp
  src/string_list.cpp
  src/format.cpp
)

# Create the static library
add_library(fb_strings STATIC ${FB_STRINGS_SOURCES})

target_include_directories(fb_strings
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Enable testing
enable_testing()

# Add unit tests subdirectory
add_subdirectory(ut)

# Installation rules
install(TARGETS fb_strings
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)

install(DIRECTORY include/fb DESTINATION include)
