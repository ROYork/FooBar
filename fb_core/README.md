# fb_core

`fb_core` is the core foundational library for the project. The goal of this
module is to provide shared utilities, platform abstractions, and convenience
helpers that can be reused by higher-level components such as `fb_net`,
`fb_signal`, and `fb_strings`.

## Repository Layout

- `cmake/` – package configuration templates.
- `doc/` – reference documentation and status tracking.
- `examples/` – small programs that demonstrate `fb_core` usage.
- `include/` – public headers exported by the library.
- `src/` – library implementation sources.
- `ut/` – unit tests and supporting assets.


Use the `FB_CORE_BUILD_TESTS` and `FB_CORE_BUILD_EXAMPLES` options to enable or
disable tests and sample programs as needed.

