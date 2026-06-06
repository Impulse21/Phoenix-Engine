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