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

# Test executable
add_executable(fb_signals_tests
  fb_signals/signal_test.cpp
  fb_signals/connection_test.cpp
  fb_signals/callable_test.cpp
  fb_signals/thread_safety_test.cpp
  fb_signals/edge_cases_test.cpp
  fb_signals/queued_delivery_test.cpp
  fb_signals/latency_benchmark_test.cpp
)

target_link_libraries(fb_signals_tests PRIVATE
  fb_signal
  GTest::gtest_main
  Threads::Threads
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(fb_signals_tests)
