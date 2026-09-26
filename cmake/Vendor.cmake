include(FetchContent)

# After first configure, don't re-check remotes — works offline
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "" FORCE)

# ─────────────────────────────────────────────────────────────────────────────
# phx_vendor_optimize(<target> [<target2> ...])
#
# Forces optimization on vendor targets in all configurations.
# Clang last-flag-wins means this safely overrides -O0 from Debug builds.
# Also suppresses vendor warnings and strips debug info from their objects.
# ─────────────────────────────────────────────────────────────────────────────
function(phx_vendor_optimize)
    foreach(target IN LISTS ARGN)
        if(NOT TARGET ${target})
            message(WARNING "phx_vendor_optimize: target '${target}' not found — skipping")
            continue()
        endif()

        # Resolve alias to real target
        get_target_property(real_target ${target} ALIASED_TARGET)
        if(real_target)
            set(target ${real_target})
        endif()

        # Check type — INTERFACE libraries have no sources, skip compile options
        get_target_property(target_type ${target} TYPE)

        if(NOT target_type STREQUAL "INTERFACE_LIBRARY")
            target_compile_options(${target} PRIVATE -w)
            target_compile_options(${target} PRIVATE
                $<$<CONFIG:Debug>:          -O2 -fno-omit-frame-pointer>
                $<$<CONFIG:RelWithDebInfo>: -O2 -fno-omit-frame-pointer>
                $<$<CONFIG:Release>:        -O3>
                $<$<CONFIG:Debug>:          -g0>
                $<$<CONFIG:RelWithDebInfo>: -g0>
            )
        endif()

        # SYSTEM includes work on all target types including INTERFACE
        get_target_property(_includes ${target} INTERFACE_INCLUDE_DIRECTORIES)
        if(_includes)
            set_target_properties(${target} PROPERTIES
                INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_includes}")
        endif()

    endforeach()
endfunction()

# ─────────────────────────────────────────────────────────────────────────────
# Vendor declarations
# Add a FetchContent_Declare block per dep.
# Set dep-specific CMake options BEFORE FetchContent_MakeAvailable.
# ─────────────────────────────────────────────────────────────────────────────

# ── spdlog ────────────────────────────────────────────────────────────────────
set(SPDLOG_BUILD_EXAMPLES   OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS      OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL          OFF CACHE BOOL "" FORCE)

FetchContent_Declare(spdlog
    GIT_REPOSITORY  https://github.com/gabime/spdlog.git
    GIT_TAG         v1.17.0
    GIT_SHALLOW     TRUE
)

# ── GLFW ────────────────────────────────────────────────────────────────────
set(GLFW_BUILD_EXAMPLES     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS        OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS         OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL            OFF CACHE BOOL "" FORCE)

if(PHX_PLATFORM_LINUX)
    set(GLFW_BUILD_WAYLAND ON  CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11     OFF CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(glfw
    GIT_REPOSITORY  https://github.com/glfw/glfw.git
    GIT_TAG         3.4
    GIT_SHALLOW     TRUE
    SYSTEM
)

# ── Volk ──────────────────────────────────────────────────────────────────────
FetchContent_Declare(volk
    GIT_REPOSITORY  https://github.com/zeux/volk.git
    GIT_TAG         vulkan-sdk-1.4.350.0
    GIT_SHALLOW     TRUE
)

FetchContent_MakeAvailable(volk)
phx_vendor_optimize(volk)

# ── Taskflow ──────────────────────────────────────────────────────────────────
set(TF_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(TF_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(taskflow
    GIT_REPOSITORY  https://github.com/taskflow/taskflow.git
    GIT_TAG         v4.1.0
    GIT_SHALLOW     TRUE
)

FetchContent_MakeAvailable(taskflow)
phx_vendor_optimize(Taskflow)

# ── Tracy ─────────────────────────────────────────────────────────────────────
set(TRACY_ENABLE         ON  CACHE BOOL "" FORCE)
set(TRACY_ON_DEMAND      ON  CACHE BOOL "" FORCE)   # zero overhead until GUI attaches
set(TRACY_ONLY_LOCALHOST OFF CACHE BOOL "" FORCE)
set(TRACY_NO_CALLSTACK   OFF CACHE BOOL "" FORCE)   # keep callstacks — useful for memory profiling

FetchContent_Declare(tracy
    GIT_REPOSITORY  https://github.com/wolfpld/tracy.git
    GIT_TAG         v0.14.1
    GIT_SHALLOW     TRUE
)

FetchContent_MakeAvailable(tracy)
phx_vendor_optimize(TracyClient)

# ── meshoptimizer ─────────────────────────────────────────────────────────────
# Real CMakeLists.txt; demo/gltfpack default OFF already but set explicitly
# since we only want the plain "meshoptimizer" library target.
set(MESHOPT_BUILD_DEMO        OFF CACHE BOOL "" FORCE)
set(MESHOPT_BUILD_GLTFPACK    OFF CACHE BOOL "" FORCE)
set(MESHOPT_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(meshoptimizer
    GIT_REPOSITORY  https://github.com/zeux/meshoptimizer.git
    GIT_TAG         v1.2
    GIT_SHALLOW     TRUE
)

FetchContent_MakeAvailable(meshoptimizer)
phx_vendor_optimize(meshoptimizer)

# ── VulkanMemoryAllocator ─────────────────────────────────────────────────────
FetchContent_Declare(vma
    GIT_REPOSITORY  https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG         v3.4.0
    GIT_SHALLOW     TRUE
)

FetchContent_MakeAvailable(vma)
phx_vendor_optimize(GPUOpen::VulkanMemoryAllocator)

# ── Jolt (when you bring it in) ───────────────────────────────────────────────
# set(CPP_RTTI_ENABLED         OFF CACHE BOOL "" FORCE)
# set(ENABLE_ALL_WARNINGS      OFF CACHE BOOL "" FORCE)
#
# FetchContent_Declare(jolt
#     GIT_REPOSITORY  https://github.com/jrouwe/JoltPhysics.git
#     GIT_TAG         v5.1.0
#     GIT_SHALLOW     TRUE
# )

# ── ImGui (when you bring it in) ──────────────────────────────────────────────
# ImGui has no CMakeLists — you'll add it as a manual target here
# See bottom of this file for the pattern

# ── Slang ─────────────────────────────────────────────────────────────────────
# Prebuilt binary release — not built from source.
# FetchContent_Populate used instead of MakeAvailable since there is no
# CMakeLists.txt to configure in the extracted archive.

set(PHX_SLANG_VERSION "2026.16.1" CACHE STRING "Slang release version")

if(PHX_PLATFORM_WINDOWS)
    set(_slang_archive "slang-${PHX_SLANG_VERSION}-windows-x86_64.zip")
elseif(PHX_PLATFORM_LINUX)
    set(_slang_archive "slang-${PHX_SLANG_VERSION}-linux-x86_64.tar.gz")
endif()

FetchContent_Declare(slang
    URL "https://github.com/shader-slang/slang/releases/download/v${PHX_SLANG_VERSION}/${_slang_archive}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_GetProperties(slang)
if(NOT slang_POPULATED)
    FetchContent_MakeAvailable(slang)
endif()

if(NOT TARGET slang::slang)
    add_library(slang::slang SHARED IMPORTED GLOBAL)
    set_target_properties(slang::slang PROPERTIES
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${slang_SOURCE_DIR}/include"
        INTERFACE_INCLUDE_DIRECTORIES        "${slang_SOURCE_DIR}/include"
    )

    if(PHX_PLATFORM_WINDOWS)
        set(_SLANG_DLL_PATH "${slang_SOURCE_DIR}/bin/slang.dll" CACHE INTERNAL "")
        set_target_properties(slang::slang PROPERTIES
            IMPORTED_IMPLIB   "${slang_SOURCE_DIR}/lib/slang.lib"
            IMPORTED_LOCATION "${_SLANG_DLL_PATH}"
        )
    elseif(PHX_PLATFORM_LINUX)
        set_target_properties(slang::slang PROPERTIES
            IMPORTED_LOCATION "${slang_SOURCE_DIR}/lib/libslang.so"
        )
    endif()
endif()

set(SLANG_COMPILER "${slang_SOURCE_DIR}/bin/slangc${CMAKE_EXECUTABLE_SUFFIX}"
    CACHE FILEPATH "slangc executable" FORCE)

unset(_slang_archive)

# ── HLSLPP ────────────────────────────────────────────────────────────────────
# Header-only — no root CMakeLists.txt to build (only hlslpp-config.cmake, an
# install/find_package config, not usable with FetchContent_MakeAvailable), so
# FetchContent_Populate the sources and hand-declare an INTERFACE target,
# same trick as Slang above.

FetchContent_Declare(hlslpp
    GIT_REPOSITORY  https://github.com/redorav/hlslpp.git
    GIT_TAG         3.9
    GIT_SHALLOW     TRUE
)

FetchContent_GetProperties(hlslpp)
if(NOT hlslpp_POPULATED)
    FetchContent_Populate(hlslpp)
endif()

if(NOT TARGET hlslpp::hlslpp)
    add_library(hlslpp INTERFACE)
    add_library(hlslpp::hlslpp ALIAS hlslpp)
    target_include_directories(hlslpp SYSTEM INTERFACE "${hlslpp_SOURCE_DIR}/include")
    # Unlocks float4x4::perspective/look_at/rotation_axis/scale/identity etc.
    target_compile_definitions(hlslpp INTERFACE HLSLPP_FEATURE_TRANSFORM)
endif()

# ── cgltf ─────────────────────────────────────────────────────────────────────
# Single-header C99 library, no CMakeLists.txt at all — same
# FetchContent_Populate + hand-declared INTERFACE target trick as above.
# Exactly one .cpp must define CGLTF_IMPLEMENTATION before including
# cgltf.h to get the function bodies — see PhxEngine/Core/cgltf_impl.cpp.

FetchContent_Declare(cgltf
    GIT_REPOSITORY  https://github.com/jkuhlmann/cgltf.git
    GIT_TAG         v1.15
    GIT_SHALLOW     TRUE
)

FetchContent_GetProperties(cgltf)
if(NOT cgltf_POPULATED)
    FetchContent_Populate(cgltf)
endif()

if(NOT TARGET cgltf::cgltf)
    add_library(cgltf INTERFACE)
    add_library(cgltf::cgltf ALIAS cgltf)
    target_include_directories(cgltf SYSTEM INTERFACE "${cgltf_SOURCE_DIR}")
endif()

# ── stb (stb_image, stb_image_resize2) ───────────────────────────────────────
# Single-header, no CMakeLists.txt, same FetchContent_Populate + hand-declared
# INTERFACE target trick as cgltf/hlslpp. stb doesn't tag releases at all, so
# this pins to a specific commit instead of a version tag (standard practice
# for this repo specifically). Two translation units must define
# STB_IMAGE_IMPLEMENTATION / STB_IMAGE_RESIZE_IMPLEMENTATION before including
# the respective header to get the function bodies -- see
# PhxEngine/Core/stb_impl.cpp.

FetchContent_Declare(stb
    GIT_REPOSITORY  https://github.com/nothings/stb.git
    GIT_TAG         2c980bb59875b0d32144a71867fbdebb2f77cd20
)

FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
endif()

if(NOT TARGET stb::stb)
    add_library(stb INTERFACE)
    add_library(stb::stb ALIAS stb)
    target_include_directories(stb SYSTEM INTERFACE "${stb_SOURCE_DIR}")
endif()

# ── bc7enc_rdo (bc7enc, rgbcx) ────────────────────────────────────────────────
# No usable CMakeLists.txt -- the upstream one builds an unrelated demo CLI
# tool (pulls in lodepng, optional ISPC, a test.cpp with main()), not a
# library. FetchContent_Populate the source only and hand-build a minimal
# static library from just the two files actually needed: bc7enc (BC7,
# used for baseColor/emissive/metallicRoughness per the texture format
# policy) and rgbcx (BC4/BC5, used for occlusion/normal maps). Also
# untagged upstream, same commit-pin approach as stb above.

FetchContent_Declare(bc7enc_rdo
    GIT_REPOSITORY  https://github.com/richgel999/bc7enc_rdo.git
    GIT_TAG         b9438627eef73a1157e84201b6fa6eb2ffd6d9f0
)

FetchContent_GetProperties(bc7enc_rdo)
if(NOT bc7enc_rdo_POPULATED)
    FetchContent_Populate(bc7enc_rdo)
endif()

if(NOT TARGET bc7enc::bc7enc)
    add_library(bc7enc STATIC
        "${bc7enc_rdo_SOURCE_DIR}/bc7enc.cpp"
        "${bc7enc_rdo_SOURCE_DIR}/rgbcx.cpp"
    )
    add_library(bc7enc::bc7enc ALIAS bc7enc)
    target_include_directories(bc7enc PUBLIC "${bc7enc_rdo_SOURCE_DIR}")
    phx_vendor_optimize(bc7enc)
endif()

# ── Helper: copy slang.dll next to a target on Windows ───────────────────────
# Usage: phx_copy_slang_dll(MyExecutableTarget)
function(phx_copy_slang_dll target)
    if(PHX_PLATFORM_WINDOWS)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_SLANG_DLL_PATH}"
                "$<TARGET_FILE_DIR:${target}>/slang.dll"
            COMMENT "Copying slang.dll to output directory"
        )
    endif()
endfunction()


# ─────────────────────────────────────────────────────────────────────────────
# Make available + optimize
# One MakeAvailable call, then optimize all targets from it
# ─────────────────────────────────────────────────────────────────────────────
FetchContent_MakeAvailable(spdlog)
phx_vendor_optimize(spdlog)

FetchContent_MakeAvailable(glfw)
phx_vendor_optimize(glfw)

# FetchContent_MakeAvailable(jolt)
# phx_vendor_optimize(Jolt)