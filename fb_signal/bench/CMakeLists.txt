add_executable(fb_signal_bench
  fb_signal_bench.cpp
)

target_link_libraries(fb_signal_bench PRIVATE fb_signal)
target_compile_options(fb_signal_bench PRIVATE -O3 -Wno-unused-variable -Wno-unused-const-variable -Wno-error)
