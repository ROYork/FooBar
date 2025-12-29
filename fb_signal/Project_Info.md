# Project: fb_signal - A C++ High-performance Signal/Slot library

## Brief

This is a high-performance signal/slot library for use in High Frequency Trading (HFT) applications written in C++. It will be a core component of a larger library, but the `fb_signal` will be self contained.
 
- **Namespace:** `fb`
- **Build System:** CMAKE
- **Unit Test System:** GTest (in folder 'ut/')
- **Design Spec:** FB_SIGNAL_SLOT_OPEN.md
- **Coding Standards:** 'Coding_Standards.md'.
- **Documentation:** In the 'docs/' folder provide relevant documentation such as: 
    - detailed design
    - design choice explanation
    - usage examples 
    - Caveats and gotchas  
 
# General Instructions

- Cmake should run from project root using command: `cmake -S . -B ./build/`
- Build should be able to run from project root: `cmake --build build -j8` 
- All unit tests should be able to run from project root: `ctest --test-dir build/ --output-on-failure`

**IMPORTANT:** Your task is not complete until the project builds warning free and all GTest unit tests pass!

