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
add_executable(test_format
  test_format.cpp
)

target_link_libraries(test_format
  PRIVATE
    fb_strings
    GTest::gtest_main
)

# Benchmark executable
add_executable(benchmark_split
  benchmark_split.cpp
)

target_link_libraries(benchmark_split
  PRIVATE
    fb_strings
)

# Test executable for string_builder
add_executable(test_string_builder
  test_string_builder.cpp
)

target_link_libraries(test_string_builder
  PRIVATE
    fb_strings
    GTest::gtest_main
)

# Test executable for string_hash
add_executable(test_string_hash
  test_string_hash.cpp
)

target_link_libraries(test_string_hash
  PRIVATE
    fb_strings
    GTest::gtest_main
)

# Test executable for string_random
add_executable(test_string_random
  test_string_random.cpp
)

target_link_libraries(test_string_random
  PRIVATE
    fb_strings
    GTest::gtest_main
)

# Test executable for utf8_utils
add_executable(test_utf8_utils
  test_utf8_utils.cpp
)

target_link_libraries(test_utf8_utils
  PRIVATE
    fb_strings
    GTest::gtest_main
)

# Test executable for string_iterators
add_executable(test_string_iterators
  test_string_iterators.cpp
)

target_link_libraries(test_string_iterators
  PRIVATE
    fb_strings
    GTest::gtest_main
)

# Include GoogleTest module for test discovery
include(GoogleTest)
gtest_discover_tests(test_string_utils)
gtest_discover_tests(test_string_list)
gtest_discover_tests(test_format)
gtest_discover_tests(test_string_builder)
gtest_discover_tests(test_string_hash)
gtest_discover_tests(test_string_random)
gtest_discover_tests(test_utf8_utils)
gtest_discover_tests(test_string_iterators)
