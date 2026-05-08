# ── Enforce Clang ────────────────────────────────────────────────────────────
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR
        "PHX requires Clang. Got: ${CMAKE_CXX_COMPILER_ID}\n"
        "Configure with: -DCMAKE_TOOLCHAIN_FILE=cmake/ClangToolchain.cmake")
endif()

# ── C++20 ────────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)    # no GNU extensions

# Export compile commands for LazyVim/VSCode
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Warnings ─────────────────────────────────────────────────────────────────
set(CLANG_WARNING_OVERRIDES
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-old-style-cast
    -Wno-float-equal
    -Wno-reserved-identifier
    -Wno-newline-eof
    -Wno-switch
    -Wno-switch-enum
    -Wno-switch-default
    -Wno-reorder-ctor
    -Wno-covered-switch-default
    -Wno-ctad-maybe-unsupported
    -Wno-language-extension-token
    -Wno-global-constructors
    -Wno-missing-variable-declarations
    -Wno-exit-time-destructors
    -Wno-nonportable-system-include-path
    -Wno-sign-conversion
    -Wno-unused-member-function
    -Wno-nested-anon-types
    -Wno-gnu-anonymous-struct
    -Wno-cast-function-type
    -Wno-misleading-indentation
    -Wno-tautological-undefined-compare
    -Wno-deprecated-declarations
    -Wno-nullability-completeness
    -Wno-missing-field-initializers
)

add_compile_options(
    -Wall
    -Wextra
    -Wshadow
    -Wcast-align
    ${CLANG_WARNING_OVERRIDES}
)

# ── Per-configuration flags ───────────────────────────────────────────────────
# Debug
add_compile_options($<$<CONFIG:Debug>:-O0>)
add_compile_options($<$<CONFIG:Debug>:-g>)
add_compile_definitions($<$<CONFIG:Debug>:PHX_DEBUG>)

# RelWithDebInfo
add_compile_options($<$<CONFIG:RelWithDebInfo>:-O2>)
add_compile_options($<$<CONFIG:RelWithDebInfo>:-gline-tables-only>)
add_compile_definitions($<$<CONFIG:RelWithDebInfo>:PHX_RELEASE>)
add_compile_definitions($<$<CONFIG:RelWithDebInfo>:PHX_DEBUG_INFO>)

# Release
add_compile_options($<$<CONFIG:Release>:-O3>)
add_compile_definitions($<$<CONFIG:Release>:PHX_RELEASE>)
add_compile_definitions($<$<CONFIG:Release>:NDEBUG>)