function(fb_configure_cxx_standard COMPONENT_NAME)
    if(NOT DEFINED FB_FORCE_CXX17)
        set(FB_FORCE_CXX17 OFF)
    endif()

    include(CheckCXXCompilerFlag)

    # Use appropriate compiler flag syntax based on compiler
    if(MSVC)
        check_cxx_compiler_flag("/std:c++20" FB_COMPILER_SUPPORTS_CXX20)
        check_cxx_compiler_flag("/std:c++17" FB_COMPILER_SUPPORTS_CXX17)
    else()
        check_cxx_compiler_flag("-std=c++20" FB_COMPILER_SUPPORTS_CXX20)
        check_cxx_compiler_flag("-std=c++17" FB_COMPILER_SUPPORTS_CXX17)
    endif()

    set(CMAKE_CXX_EXTENSIONS OFF PARENT_SCOPE)
    set(CMAKE_CXX_STANDARD_REQUIRED ON PARENT_SCOPE)

    if(FB_FORCE_CXX17)
        if(NOT FB_COMPILER_SUPPORTS_CXX17)
            message(FATAL_ERROR "${COMPONENT_NAME}: FB_FORCE_CXX17=ON but compiler lacks C++17 support")
        endif()

        set(CMAKE_CXX_STANDARD 17 PARENT_SCOPE)
        message(STATUS "${COMPONENT_NAME}: Forced C++17 standard")
        return()
    endif()

    if(FB_COMPILER_SUPPORTS_CXX20)
        set(CMAKE_CXX_STANDARD 20 PARENT_SCOPE)
        message(STATUS "${COMPONENT_NAME}: Using C++20 standard")
    elseif(FB_COMPILER_SUPPORTS_CXX17)
        set(CMAKE_CXX_STANDARD 17 PARENT_SCOPE)
        message(STATUS "${COMPONENT_NAME}: Falling back to C++17 standard")
    else()
        message(FATAL_ERROR "${COMPONENT_NAME}: Compiler does not support C++17 or C++20")
    endif()
endfunction()

