# Dependencies.cmake
# All FetchContent declarations for Phoenix Engine dependencies

include(FetchContent)
include(Utils)  # For configure_vendor_target()

#==============================================================================
# Third-Party Warning Suppression
#==============================================================================

# Disable deprecation warnings from third-party code
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "Disable deprecation warnings" FORCE)

# Set modern policy defaults for dependencies
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)  # Require project VERSION
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)  # Let normal variables override CACHE
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)  # MSVC runtime library flags

# Suppress specific policy warnings
set(CMAKE_POLICY_WARNING_CMP0048 OFF)
set(CMAKE_POLICY_WARNING_CMP0077 OFF)
set(CMAKE_POLICY_WARNING_CMP0091 OFF)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "Disable deprecation warnings" FORCE)


message(STATUS "Third-party deprecation warnings suppressed")

#==============================================================================
# FetchContent Configuration
#==============================================================================

# IMPORTANT: Using SOURCE_DIR (not BINARY_DIR) for shared cache
# This allows different presets to share downloaded source code
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/_deps" CACHE PATH "FetchContent dependency directory" FORCE)

# Only update/download if source doesn't exist (speeds up reconfigure)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Disable update step for already-populated content")

# Show download progress (useful for debugging)
set(FETCHCONTENT_QUIET OFF CACHE BOOL "Show FetchContent download progress")

message(STATUS "==============================================")
message(STATUS "FetchContent Configuration:")
message(STATUS "  Base Directory: ${FETCHCONTENT_BASE_DIR}")
message(STATUS "  Updates Disconnected: ${FETCHCONTENT_UPDATES_DISCONNECTED}")
message(STATUS "==============================================")
message(STATUS "")

#==============================================================================
# Dependency Build Configuration
#==============================================================================

# IMPORTANT: Dependencies are always built in Release mode by default
# This prevents conflicts when switching between Debug/Release configurations
# and ensures optimal performance for third-party libraries.
#
# To debug a dependency (rare), set: cmake -DFORCE_DEBUG_DEPS=ON

if(NOT DEFINED FORCE_DEBUG_DEPS)
    set(FORCE_DEBUG_DEPS OFF CACHE BOOL "Force dependencies to build in Debug mode")
endif()

if(FORCE_DEBUG_DEPS)
    message(WARNING "FORCE_DEBUG_DEPS=ON: Dependencies will build in current configuration")
    message(WARNING "This may cause rebuild issues when switching configurations!")
else()
    # Save current build type
    set(_ORIGINAL_BUILD_TYPE ${CMAKE_BUILD_TYPE})
    
    # Force Release for dependencies
    set(CMAKE_BUILD_TYPE Release)
    
    message(STATUS "Dependencies will build in Release mode (optimized)")
    message(STATUS "  Your project builds in: ${_ORIGINAL_BUILD_TYPE}")
    message(STATUS "  To debug dependencies: cmake -DFORCE_DEBUG_DEPS=ON")
endif()

#==============================================================================
# Logging
#==============================================================================

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.17.0
)

#==============================================================================
# Entity Component System
#==============================================================================

FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.14.0
)

#==============================================================================
# Serialization & Configuration
#==============================================================================

set(CMAKE_POLICY_DEFAULT_CMP0000 NEW CACHE STRING "" FORCE)

FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG yaml-cpp-0.9.0
    OVERRIDE_FIND_PACKAGE
)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install OFF CACHE BOOL "" FORCE)

#==============================================================================
# Windowing & Input
#==============================================================================

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

#==============================================================================
# User Interface
#==============================================================================

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.5
)

#==============================================================================
# Profiling
#==============================================================================

FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG v0.11.1
)
set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
set(TRACY_ON_DEMAND ON CACHE BOOL "" FORCE)

#==============================================================================
# Math Libraries
#==============================================================================

FetchContent_Declare(
    hlslpp
    GIT_REPOSITORY https://github.com/redorav/hlslpp.git
    GIT_TAG 3.8
)

#==============================================================================
# Mesh & Model Processing
#==============================================================================

FetchContent_Declare(
    meshoptimizer
    GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
    GIT_TAG v0.21
)

FetchContent_Declare(
    cgltf
    GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
    GIT_TAG v1.14
)

#==============================================================================
# Image & Texture Libraries
#==============================================================================

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master  # Note: stb doesn't use tags, master is appropriate
)

FetchContent_Declare(
    bc7enc_rdo
    GIT_REPOSITORY https://github.com/richgel999/bc7enc_rdo.git
    GIT_TAG master  # Note: No versioned releases available
)

#==============================================================================
# Memory Allocators
#==============================================================================

FetchContent_Declare(
    tlsf
    GIT_REPOSITORY https://github.com/mattconte/tlsf.git
    GIT_TAG master  # Note: No versioned releases available
)

#==============================================================================
# Vulkan Utilities
#==============================================================================

FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.1.0
)

FetchContent_Declare(
    volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG 1.3.295
)

FetchContent_Declare(
    vk-bootstrap
    GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
    GIT_TAG v1.3.295
)

#==============================================================================
# Shader Compiler (Platform-specific binaries)
#==============================================================================

if(PLATFORM_LINUX)
    FetchContent_Declare(
        slang
        URL https://github.com/shader-slang/slang/releases/download/v2025.23.2/slang-2025.23.2-linux-x86_64.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
elseif(PLATFORM_WINDOWS)
    FetchContent_Declare(
        slang
        URL https://github.com/shader-slang/slang/releases/download/v2025.23.2/slang-2025.23.2-windows-x86_64.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
endif()

#==============================================================================
# Make Dependencies Available
#==============================================================================

message(STATUS "Fetching dependencies with CMake support...")

# Fetch libraries with native CMake support
FetchContent_MakeAvailable(
    spdlog
    entt
    yaml-cpp
    glfw
    json
    meshoptimizer
    vma
    volk
    vk-bootstrap
)

message(STATUS "Configuring vendor targets...")

# Configure all FetchContent targets with SYSTEM includes and optimizations
configure_vendor_target(spdlog FOLDER "Vendors/Logging")
configure_vendor_target(EnTT FOLDER "Vendors/ECS")
configure_vendor_target(glfw FOLDER "Vendors/Windowing")
configure_vendor_target(nlohmann_json FOLDER "Vendors/Serialization")
configure_vendor_target(meshoptimizer FOLDER "Vendors/Optimization")
configure_vendor_target(VulkanMemoryAllocator FOLDER "Vendors/Graphics")
configure_vendor_target(volk FOLDER "Vendors/Graphics")
configure_vendor_target(vk-bootstrap FOLDER "Vendors/Graphics")
configure_vendor_target(yaml-cpp FOLDER "Vendors/Serialization")

if(TARGET yaml-cpp)    
    set_target_properties(yaml-cpp PROPERTIES DEBUG_POSTFIX "")
endif()

# Handle GLFW sub-targets
if(TARGET update_mappings)
    set_target_properties(update_mappings PROPERTIES FOLDER "Vendors/Windowing/GLFW")
endif()

# Handle volk_headers if it exists
if(TARGET volk_headers)
    configure_vendor_target(volk_headers FOLDER "Vendors/Graphics")
endif()

#==============================================================================
# Header-Only and Custom Libraries
#==============================================================================

message(STATUS "Fetching header-only and custom libraries...")

# These libraries don't have CMakeLists or need custom handling
# We use FetchContent_MakeAvailable which is modern and recommended

FetchContent_MakeAvailable(
    imgui
    tracy
    hlslpp
    stb
    cgltf
    tlsf
    slang
)

FetchContent_GetProperties(bc7enc_rdo)
if(NOT bc7enc_rdo_POPULATED)
    # Temporarily suppress the CMake 3.28+ deprecation warning for Populate
    cmake_policy(PUSH)
    if(POLICY CMP0169)
        cmake_policy(SET CMP0169 OLD)
    endif()
    
    FetchContent_Populate(bc7enc_rdo)
    
    cmake_policy(POP)
endif()

message(STATUS "All dependencies fetched successfully!")

#==============================================================================
# Restore Build Configuration
#==============================================================================

if(NOT FORCE_DEBUG_DEPS)
    # Restore original build type for your project
    set(CMAKE_BUILD_TYPE ${_ORIGINAL_BUILD_TYPE})
    message(STATUS "Restored build type to: ${CMAKE_BUILD_TYPE}")
endif()

#==============================================================================
# Export Dependency Paths
#==============================================================================

# Export important paths for use in other CMakeLists
# Note: Since this file is include()'d (not add_subdirectory), 
# these are set in the current scope (which is what we want!)

set(IMGUI_SOURCE_DIR ${imgui_SOURCE_DIR})
set(TRACY_SOURCE_DIR ${tracy_SOURCE_DIR})
set(HLSLPP_SOURCE_DIR ${hlslpp_SOURCE_DIR})
set(STB_SOURCE_DIR ${stb_SOURCE_DIR})
set(CGLTF_SOURCE_DIR ${cgltf_SOURCE_DIR})
set(TLSF_SOURCE_DIR ${tlsf_SOURCE_DIR})
set(BC7ENC_RDO_SOURCE_DIR ${bc7enc_rdo_SOURCE_DIR})
set(SLANG_SOURCE_DIR ${slang_SOURCE_DIR})

message(STATUS "")
message(STATUS "==============================================")
message(STATUS "Dependencies Summary:")
message(STATUS "  Total Fetched: 18 libraries")
message(STATUS "  Build Mode: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Cache Directory: ${FETCHCONTENT_BASE_DIR}")
message(STATUS "==============================================")
