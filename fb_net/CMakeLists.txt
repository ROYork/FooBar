cmake_minimum_required(VERSION 3.16)

# Project configuration
project(fb_net
    VERSION 1.0.0
    DESCRIPTION "FB_NET - A cross-platform C++ networking library"
    LANGUAGES CXX
)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# Configure C++ standard with optional C++17 fallback
include(FBDetermineCxxStandard)
fb_configure_cxx_standard("fb_net")

# Platform detection
if(WIN32)
    message(STATUS "Building for Windows")
    set(FB_NET_PLATFORM_WINDOWS TRUE)
elseif(APPLE)
    message(STATUS "Building for macOS")
    set(FB_NET_PLATFORM_MACOS TRUE)
elseif(UNIX)
    message(STATUS "Building for Linux/Unix")
    set(FB_NET_PLATFORM_LINUX TRUE)
endif()

# Compiler-specific flags
if(MSVC)
    # Visual Studio specific flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
    
    # Disable specific MSVC warnings
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(_WINSOCK_DEPRECATED_NO_WARNINGS)
else()
    # GCC/Clang specific flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# Source files
set(FB_NET_SOURCES
    src/socket_address.cpp
    src/socket_base.cpp
    src/tcp_client.cpp
    src/server_socket.cpp
    src/socket_stream.cpp
    src/poll_set.cpp
    src/udp_socket.cpp
    src/tcp_server_connection.cpp
    src/tcp_server.cpp
    src/udp_handler.cpp
    src/udp_client.cpp
    src/udp_server.cpp
    src/Library.cpp
)

# Header files (for IDE support)
set(FB_NET_HEADERS
    include/fb/fb_net.h
    include/fb/socket_address.h
    include/fb/socket_base.h
    include/fb/tcp_client.h
    include/fb/server_socket.h
    include/fb/socket_stream.h
    include/fb/poll_set.h
    include/fb/udp_socket.h
    include/fb/tcp_server_connection.h
    include/fb/tcp_server.h
    include/fb/udp_handler.h
    include/fb/udp_client.h
    include/fb/udp_server.h
)

# Create the library
add_library(fb_net ${FB_NET_SOURCES} ${FB_NET_HEADERS})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT fb_net_ipo_supported OUTPUT fb_net_ipo_error)
    if(fb_net_ipo_supported)
        set_property(TARGET fb_net PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "fb_net: IPO not enabled (${fb_net_ipo_error})")
    endif()
endif()

# Include directories
target_include_directories(fb_net
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/fb_signal/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Apply consistent, high-quality warnings to this target
include(CompilerWarnings)
set_project_warnings(fb_net)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(fb_net PRIVATE ws2_32 mswsock)
elseif(UNIX AND NOT APPLE)
    # Linux specific libraries if needed
    find_package(Threads REQUIRED)
    target_link_libraries(fb_net PRIVATE Threads::Threads)
elseif(APPLE)
    # macOS specific libraries if needed
    find_package(Threads REQUIRED)
    target_link_libraries(fb_net PRIVATE Threads::Threads)
endif()

# Compiler definitions based on platform
if(WIN32)
    target_compile_definitions(fb_net PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
endif()

# Export symbols for Windows DLL
if(WIN32 AND BUILD_SHARED_LIBS)
    target_compile_definitions(fb_net PRIVATE FB_NET_EXPORTS)
    target_compile_definitions(fb_net INTERFACE FB_NET_DLL)
endif()

# Set target properties
set_target_properties(fb_net PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${FB_NET_HEADERS}"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

# Enable position independent code
set_property(TARGET fb_net PROPERTY POSITION_INDEPENDENT_CODE ON)

# Installation configuration
include(GNUInstallDirs)

# Install the library
install(TARGETS fb_net
    EXPORT fb_netTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fb
)

# Install headers with directory structure
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# Export targets for find_package
install(EXPORT fb_netTargets
    FILE fb_netTargets.cmake
    NAMESPACE fb::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fb_net
)

# Create and install config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    fb_netConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/fb_netConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/fb_netConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fb_net
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/fb_netConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/fb_netConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fb_net
)

# Unit testing support
option(FB_NET_BUILD_TESTS "Build fb_net unit tests" ON)

if(FB_NET_BUILD_TESTS)
    enable_testing()
    add_subdirectory(ut)
endif()

# Examples
option(FB_NET_BUILD_EXAMPLES "Build fb_net examples" ON)

if(FB_NET_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Additional test executables
option(FB_NET_BUILD_INTEGRATION_TESTS "Build integration and validation tests" ON)

   
 # Add to test suite
if(FB_NET_BUILD_TESTS)
  add_test(NAME integration_tests COMMAND test_integration)
  add_test(NAME legacy_compatibility COMMAND test_legacy_compat)
  add_test(NAME memory_validation COMMAND test_memory_validation)
        
  set_tests_properties(integration_tests legacy_compatibility memory_validation PROPERTIES
       TIMEOUT 300
  )
endif()


# Print configuration summary
message(STATUS "")
message(STATUS "FB_NET Configuration Summary:")
message(STATUS "  Version:           ${PROJECT_VERSION}")
message(STATUS "  Build Type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard:      ${CMAKE_CXX_STANDARD}")
message(STATUS "  Install Prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Build Tests:       ${FB_NET_BUILD_TESTS}")
message(STATUS "  Build Examples:    ${FB_NET_BUILD_EXAMPLES}")
message(STATUS "  Platform:          ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler:          ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "")
