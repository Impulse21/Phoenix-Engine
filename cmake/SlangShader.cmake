# ── SlangShader.cmake ─────────────────────────────────────────────────────────
# Provides phx_compile_slang_shader() to compile .slang source files to SPIR-V
# at build time using slangc.
#
# Compiled SPIR-V files are written to:
#   ${CMAKE_CURRENT_BINARY_DIR}/Shaders/<name>.<stage>.spv
#
# Usage:
#   phx_compile_slang_shader(
#       TARGET      MyTarget            # target to attach compiled shaders to
#       SOURCE      Shaders/Foo.slang   # .slang source file
#       ENTRY       VS_Main             # entry point name
#       STAGE       vertex              # vertex | fragment | compute
#       OUTPUT_VAR  FOO_VERT_SPV        # variable to receive the output path
#   )
#
# A convenience wrapper phx_compile_slang_shader_pair() handles vertex+fragment
# from the same source file in one call.
# ─────────────────────────────────────────────────────────────────────────────

cmake_minimum_required(VERSION 3.25)

if(NOT SLANG_COMPILER)
    message(FATAL_ERROR
        "SlangShader: SLANG_COMPILER is not set.\n"
        "Include FetchSlang.cmake before SlangShader.cmake.")
endif()

# ── phx_compile_slang_shader ──────────────────────────────────────────────────
function(phx_compile_slang_shader)
    cmake_parse_arguments(ARG "" "TARGET;SOURCE;ENTRY;STAGE;OUTPUT_VAR" "" ${ARGN})

    if(NOT ARG_TARGET OR NOT ARG_SOURCE OR NOT ARG_ENTRY OR NOT ARG_STAGE)
        message(FATAL_ERROR
            "phx_compile_slang_shader: TARGET, SOURCE, ENTRY and STAGE are required")
    endif()

    # Resolve absolute source path
    if(NOT IS_ABSOLUTE "${ARG_SOURCE}")
        set(ARG_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_SOURCE}")
    endif()

    if(NOT EXISTS "${ARG_SOURCE}")
        message(FATAL_ERROR
            "phx_compile_slang_shader: source file not found: ${ARG_SOURCE}")
    endif()

    # Output directory and file
    set(_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/Shaders")
    file(MAKE_DIRECTORY "${_OUTPUT_DIR}")

    get_filename_component(_NAME "${ARG_SOURCE}" NAME_WE)
    set(_OUTPUT_SPV "${_OUTPUT_DIR}/${_NAME}.${ARG_STAGE}.spv")

    # Map stage name to slangc profile
    if(ARG_STAGE STREQUAL "vertex")
        set(_PROFILE "vs_6_0")
    elseif(ARG_STAGE STREQUAL "fragment")
        set(_PROFILE "ps_6_0")
    elseif(ARG_STAGE STREQUAL "compute")
        set(_PROFILE "cs_6_0")
    else()
        message(FATAL_ERROR "phx_compile_slang_shader: unknown stage '${ARG_STAGE}'")
    endif()

    # Compile command
    # -target spirv       — output SPIR-V for Vulkan
    # -emit-spirv-directly — bypass GLSL intermediate, cleaner output
    # -fvk-use-entrypoint-name — preserve entry point name in SPIR-V
    # -matrix-layout-column-major — match GLM and common Vulkan convention
    add_custom_command(
        OUTPUT  "${_OUTPUT_SPV}"
        COMMAND "${SLANG_COMPILER}"
                "${ARG_SOURCE}"
                -target    spirv
                -profile   "${_PROFILE}"
                -entry     "${ARG_ENTRY}"
                -emit-spirv-directly
                -fvk-use-entrypoint-name
                -matrix-layout-column-major
                -o         "${_OUTPUT_SPV}"
        DEPENDS "${ARG_SOURCE}"
        COMMENT "Slang → SPIR-V: ${_NAME}.${ARG_STAGE}.spv"
        VERBATIM
    )

    # Attach to target as a generated source so the build system tracks it
    target_sources(${ARG_TARGET} PRIVATE "${_OUTPUT_SPV}")

    # Source group keeps IDE explorers tidy
    source_group("Shaders\\Generated" FILES "${_OUTPUT_SPV}")

    # Return output path to caller if requested
    if(ARG_OUTPUT_VAR)
        set(${ARG_OUTPUT_VAR} "${_OUTPUT_SPV}" PARENT_SCOPE)
    endif()

    # Expose the output directory as a compile definition so the app can
    # locate shaders at runtime using a relative path
    target_compile_definitions(${ARG_TARGET} PRIVATE
        PHX_SHADER_OUTPUT_DIR="${_OUTPUT_DIR}"
    )

    unset(_OUTPUT_DIR)
    unset(_OUTPUT_SPV)
    unset(_NAME)
    unset(_PROFILE)
endfunction()

# ── phx_compile_slang_shader_pair ────────────────────────────────────────────
# Convenience wrapper: compiles both vertex and fragment stages from one file.
#
# Usage:
#   phx_compile_slang_shader_pair(
#       TARGET      MyTarget
#       SOURCE      Shaders/Foo.slang
#       VERT_ENTRY  VS_Main
#       FRAG_ENTRY  FS_Main
#   )
function(phx_compile_slang_shader_pair)
    cmake_parse_arguments(ARG "" "TARGET;SOURCE;VERT_ENTRY;FRAG_ENTRY" "" ${ARGN})

    if(NOT ARG_TARGET OR NOT ARG_SOURCE OR NOT ARG_VERT_ENTRY OR NOT ARG_FRAG_ENTRY)
        message(FATAL_ERROR
            "phx_compile_slang_shader_pair: TARGET, SOURCE, VERT_ENTRY and FRAG_ENTRY are required")
    endif()

    phx_compile_slang_shader(
        TARGET    ${ARG_TARGET}
        SOURCE    ${ARG_SOURCE}
        ENTRY     ${ARG_VERT_ENTRY}
        STAGE     vertex
    )

    phx_compile_slang_shader(
        TARGET    ${ARG_TARGET}
        SOURCE    ${ARG_SOURCE}
        ENTRY     ${ARG_FRAG_ENTRY}
        STAGE     fragment
    )
endfunction()
