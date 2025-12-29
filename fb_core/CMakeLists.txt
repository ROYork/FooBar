cmake_minimum_required(VERSION 3.16)

project(fb_core
    VERSION 0.1.0
    DESCRIPTION "FB_CORE - Core utilities for the FooBar library"
    LANGUAGES CXX
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

include(FBDetermineCxxStandard)
fb_configure_cxx_standard("fb_core")

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

set(FB_CORE_SOURCES
    src/Library.cpp
    src/csv_parser.cpp
)

set(FB_CORE_HEADERS
    include/fb/fb_core.h
    include/fb/stop_watch.h
    include/fb/timer.h
    include/fb/circular_buffer.h
    include/fb/circular_buffer_iterator.h
    include/fb/span_compat.h
    include/fb/csv_parser.h
)

add_library(fb_core ${FB_CORE_SOURCES} ${FB_CORE_HEADERS})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT fb_core_ipo_supported OUTPUT fb_core_ipo_error)
    if(fb_core_ipo_supported)
        set_property(TARGET fb_core PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "fb_core: IPO not enabled (${fb_core_ipo_error})")
    endif()
endif()

target_include_directories(fb_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/fb_signal/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Apply consistent, high-quality warnings to this target
include(CompilerWarnings)
set_project_warnings(fb_core)

set_target_properties(fb_core PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${FB_CORE_HEADERS}"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

set_property(TARGET fb_core PROPERTY POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)

install(TARGETS fb_core
    EXPORT fb_coreTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fb
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT fb_coreTargets
    FILE fb_coreTargets.cmake
    NAMESPACE fb::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fb_core
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    fb_coreConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/fb_coreConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/fb_coreConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fb_core
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/fb_coreConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/fb_coreConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fb_core
)

option(FB_CORE_BUILD_TESTS "Build fb_core unit tests" ON)

if(FB_CORE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(ut)
endif()

option(FB_CORE_BUILD_EXAMPLES "Build fb_core examples" ON)

if(FB_CORE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

message(STATUS "")
message(STATUS "FB_CORE Configuration Summary:")
message(STATUS "  Version:           ${PROJECT_VERSION}")
message(STATUS "  Build Type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard:      ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Tests:       ${FB_CORE_BUILD_TESTS}")
message(STATUS "  Build Examples:    ${FB_CORE_BUILD_EXAMPLES}")
message(STATUS "  Install Prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
