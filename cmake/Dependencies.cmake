# Dependencies.cmake
# All FetchContent declarations for Phoenix Engine dependencies

include(FetchContent)

# Disable deprecation warnings from dependencies
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "Disable deprecation warnings" FORCE)

# Set modern policy defaults
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)  # Project VERSION
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)  # CACHE variables
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)  # MSVC runtime

# Suppress specific warnings
set(CMAKE_POLICY_WARNING_CMP0048 OFF)
set(CMAKE_POLICY_WARNING_CMP0077 OFF)
set(CMAKE_POLICY_WARNING_CMP0091 OFF)

# Set FetchContent base directory

# If I ever nee dplatform specific dpendencies, uncomment below:
#[[
# In cmake/Dependencies.cmake
if(WIN32)
    set(PLATFORM_SUFFIX "-windows")
elseif(UNIX)
    set(PLATFORM_SUFFIX "-linux")
endif()

set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.build/_deps${PLATFORM_SUFFIX}")
]]


# Only have a global dependency to avoid donwloading multiple times.
set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/_deps CACHE PATH "FetchContent dependency directory")
#set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/_deps CACHE PATH "FetchContent dependency directory")
#

# Disable updated
# Only update/download if source doesn't exist (cache populated dependencies)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Disable update step for already-populated content")

# Show what's being fetched (useful for debugging)
set(FETCHCONTENT_QUIET OFF CACHE BOOL "Show FetchContent download progress")

message(STATUS "FetchContent will download to: ${FETCHCONTENT_BASE_DIR}")
message(STATUS "FetchContent updates: ${FETCHCONTENT_UPDATES_DISCONNECTED}")

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
    GIT_TAG master
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
    GIT_TAG 3.4
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
    GIT_TAG master
)

FetchContent_Declare(
    bc7enc_rdo
    GIT_REPOSITORY https://github.com/richgel999/bc7enc_rdo.git
    GIT_TAG master
)

#==============================================================================
# Memory Allocators
#==============================================================================

FetchContent_Declare(
    tlsf
    GIT_REPOSITORY https://github.com/mattconte/tlsf.git
    GIT_TAG master
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

message(STATUS "Populating header-only and custom libraries...")

# Header-only and custom libraries that need manual handling
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_MakeAvailable(imgui)
  message(STATUS "  - ImGui populated at: ${imgui_SOURCE_DIR}")
endif()

FetchContent_GetProperties(tracy)
if(NOT tracy_POPULATED)
  FetchContent_MakeAvailable(tracy)
  message(STATUS "  - Tracy populated at: ${tracy_SOURCE_DIR}")
endif()

FetchContent_GetProperties(hlslpp)
if(NOT hlslpp_POPULATED)
  FetchContent_MakeAvailable(hlslpp)
  message(STATUS "  - hlslpp populated at: ${hlslpp_SOURCE_DIR}")
endif()

FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
  FetchContent_MakeAvailable(stb)
  message(STATUS "  - stb populated at: ${stb_SOURCE_DIR}")
endif()

FetchContent_GetProperties(cgltf)
if(NOT cgltf_POPULATED)
  FetchContent_MakeAvailable(cgltf)
  message(STATUS "  - cgltf populated at: ${cgltf_SOURCE_DIR}")
endif()

FetchContent_GetProperties(tlsf)
if(NOT tlsf_POPULATED)
  FetchContent_MakeAvailable(tlsf)
  message(STATUS "  - tlsf populated at: ${tlsf_SOURCE_DIR}")
endif()

FetchContent_GetProperties(bc7enc_rdo)
if(NOT bc7enc_rdo_POPULATED)
  FetchContent_Populate(bc7enc_rdo)
  message(STATUS "  - bc7enc_rdo populated at: ${bc7enc_rdo_SOURCE_DIR}")
endif()

FetchContent_GetProperties(slang)
if(NOT slang_POPULATED)
  FetchContent_MakeAvailable(slang)
  message(STATUS "  - Slang populated at: ${slang_SOURCE_DIR}")
endif()

message(STATUS "All dependencies fetched successfully!")

# Export important paths for use in other CMakeLists
set(IMGUI_SOURCE_DIR ${imgui_SOURCE_DIR} PARENT_SCOPE)
set(TRACY_SOURCE_DIR ${tracy_SOURCE_DIR} PARENT_SCOPE)
set(HLSLPP_SOURCE_DIR ${hlslpp_SOURCE_DIR} PARENT_SCOPE)
set(STB_SOURCE_DIR ${stb_SOURCE_DIR} PARENT_SCOPE)
set(CGLTF_SOURCE_DIR ${cgltf_SOURCE_DIR} PARENT_SCOPE)
set(TLSF_SOURCE_DIR ${tlsf_SOURCE_DIR} PARENT_SCOPE)
set(BC7ENC_RDO_SOURCE_DIR ${bc7enc_rdo_SOURCE_DIR} PARENT_SCOPE)
set(SLANG_SOURCE_DIR ${slang_SOURCE_DIR} PARENT_SCOPE)
