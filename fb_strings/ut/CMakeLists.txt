# Unit Tests CMakeLists.txt

# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Test executable for string utilities
add_executable(test_string_utils
  test_string_utils.cpp
)

target_link_libraries(test_string_utils
  PRIVATE
    fb_strings
    GTest::gtest_main
)

# Test executable for string_list
add_executable(test_string_list
  test_string_list.cpp
)

target_link_libraries(test_string_list
  PRIVATE
    fb_strings
    GTest::gtest_main
)

# Test executable for format
#add_executable(test_format
#  test_format.cpp
#)

#target_link_libraries(test_format
#  PRIVATE
#    fb_strings
#    GTest::gtest_main
#)

# Benchmark executable
add_executable(benchmark_split
  benchmark_split.cpp
)

target_link_libraries(benchmark_split
  PRIVATE
    fb_strings
)

# Include GoogleTest module for test discovery
include(GoogleTest)
gtest_discover_tests(test_string_utils)
gtest_discover_tests(test_string_list)
#gtest_discover_tests(test_format)
