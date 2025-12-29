cmake_minimum_required(VERSION 3.16)

# Note: fb_core target is available from the parent directory.

include(FBDetermineCxxStandard)
fb_configure_cxx_standard("fb_core/examples")

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif()

add_executable(fb_core_hello
    hello_core/main.cpp
)

target_link_libraries(fb_core_hello PRIVATE fb_core)

install(TARGETS fb_core_hello DESTINATION bin)

# Timer examples
add_executable(fb_core_timer_examples
    timer_examples.cpp
)

target_link_libraries(fb_core_timer_examples PRIVATE fb_core)

# Link threading library for timer examples
find_package(Threads REQUIRED)
target_link_libraries(fb_core_timer_examples PRIVATE Threads::Threads)

install(TARGETS fb_core_timer_examples DESTINATION bin)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION share/fb_core/examples
    FILES_MATCHING PATTERN "*.cpp" PATTERN "*.h" PATTERN "*.md"
)
