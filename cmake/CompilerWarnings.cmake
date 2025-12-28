# CompilerWarnings.cmake
# Professional compiler warning configuration for cross-platform builds
# Supports: MSVC (Windows), GCC (Linux), Clang (macOS)

# Function to set target-specific warning levels
function(set_project_warnings target_name)

  set(MSVC_WARNINGS
    /W4                    # High warning level
    /WX-                   # Treat warnings as errors (disabled for now, enable when all warnings fixed)
    /permissive-           # Conformance mode
    /w14242                # 'identifier': conversion from 'type1' to 'type2', possible loss of data
    /w14254                # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
    /w14263                # 'function': member function does not override any base class virtual member function
    /w14265                # 'classname': class has virtual functions, but destructor is not virtual
    /w14287                # 'operator': unsigned/negative constant mismatch
    /we4289                # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the for-loop scope
    /w14296                # 'operator': expression is always 'boolean_value'
    /w14311                # 'variable': pointer truncation from 'type1' to 'type2'
    /w14545                # expression before comma evaluates to a function which is missing an argument list
    /w14546                # function call before comma missing argument list
    /w14547                # 'operator': operator before comma has no effect; expected operator with side-effect
    /w14549                # 'operator': operator before comma has no effect; did you intend 'operator'?
    /w14555                # expression has no effect; expected expression with side-effect
    /w14619                # pragma warning: there is no warning number 'number'
    /w14640                # Enable warning on thread un-safe static member initialization
    /w14826                # Conversion from 'type1' to 'type2' is sign-extended
    /w14905                # wide string literal cast to 'LPSTR'
    /w14906                # string literal cast to 'LPWSTR'
    /w14928                # illegal copy-initialization; more than one user-defined conversion has been implicitly applied

    # Disable specific warnings that are acceptable in production code
    /wd4100                # Unreferenced formal parameter (common in interface implementations)
    /wd4244                # Conversion warnings (we handle these explicitly where needed)
    /wd4267                # size_t to int conversion (handled case-by-case)
    /wd4324                # Structure padding (intentional for performance)
  )

  set(CLANG_WARNINGS
    -Wall
    -Wextra                    # Reasonable and standard
    -Wshadow                   # Warn if variable shadows another
    -Wnon-virtual-dtor         # Warn if class with virtual func has no virtual dtor
    -Wold-style-cast           # Warn for c-style casts
    -Wcast-align               # Warn for potential performance problem casts
    -Wunused                   # Warn on anything being unused
    -Woverloaded-virtual       # Warn if you overload (not override) a virtual function
    -Wpedantic                 # Warn if non-standard C++ is used
    -Wconversion               # Warn on type conversions that may lose data
    -Wsign-conversion          # Warn on sign conversions
    # -Wnull-dereference       # Disabled: GCC 13+ produces false positives with std::istreambuf_iterator
    -Wdouble-promotion         # Warn if float is implicitly promoted to double
    -Wformat=2                 # Warn on security issues around functions that format output
    -Wimplicit-fallthrough     # Warn on statements that fallthrough without an explicit annotation
  )

  set(GCC_WARNINGS
    ${CLANG_WARNINGS}
    -Wmisleading-indentation   # Warn if indentation implies blocks where blocks do not exist
    -Wduplicated-cond          # Warn if if / else chain has duplicated conditions
    -Wduplicated-branches      # Warn if if / else branches have duplicated code
    -Wlogical-op               # Warn about logical operations being used where bitwise were probably wanted
    -Wuseless-cast             # Warn if you perform a cast to the same type
    -Wno-array-bounds          # GCC 12+ false positives with runtime bounds checking
  )

  if(MSVC)
    set(PROJECT_WARNINGS_LIST ${MSVC_WARNINGS})
  elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    set(PROJECT_WARNINGS_LIST ${CLANG_WARNINGS})
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(PROJECT_WARNINGS_LIST ${GCC_WARNINGS})
  else()
    message(AUTHOR_WARNING "No compiler warnings set for CXX compiler: '${CMAKE_CXX_COMPILER_ID}'")
  endif()

  target_compile_options(${target_name} INTERFACE ${PROJECT_WARNINGS_LIST})

endfunction()

# Function for disabling specific warnings on legacy/example code
function(set_permissive_warnings target_name)
  if(MSVC)
    target_compile_options(${target_name} PRIVATE
      /W3                    # Standard warning level
      /wd4100                # Unreferenced parameter
      /wd4244                # Conversion warnings
      /wd4267                # size_t conversion
      /wd4996                # Deprecated function warnings
    )
  else()
    target_compile_options(${target_name} PRIVATE
      -Wno-unused-parameter
      -Wno-conversion
      -Wno-sign-conversion
      -Wno-deprecated-declarations
    )
  endif()
endfunction()
