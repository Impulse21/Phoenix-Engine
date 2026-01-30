# Dependencies.cmake
# All FetchContent declarations for Phoenix Engine dependencies

include(FetchContent)

# Set FetchContent base directory
set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/_deps CACHE PATH "FetchContent dependency directory")

message(STATUS "FetchContent will download to: ${FETCHCONTENT_BASE_DIR}")

#==============================================================================
# Logging
#==============================================================================

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
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

FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0
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

FetchContent_Declare(
    cereal
    GIT_REPOSITORY https://github.com/USCiLab/cereal.git
    GIT_TAG v1.3.2
)
set(JUST_INSTALL_CEREAL ON CACHE BOOL "" FORCE)
set(SKIP_PERFORMANCE_COMPARISON ON CACHE BOOL "" FORCE)

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
    cereal
    meshoptimizer
    vma
    volk
    vk-bootstrap
)

message(STATUS "Populating header-only and custom libraries...")

# Header-only and custom libraries that need manual handling
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
    message(STATUS "  - ImGui populated at: ${imgui_SOURCE_DIR}")
endif()

FetchContent_GetProperties(tracy)
if(NOT tracy_POPULATED)
    FetchContent_Populate(tracy)
    message(STATUS "  - Tracy populated at: ${tracy_SOURCE_DIR}")
endif()

FetchContent_GetProperties(hlslpp)
if(NOT hlslpp_POPULATED)
    FetchContent_Populate(hlslpp)
    message(STATUS "  - hlslpp populated at: ${hlslpp_SOURCE_DIR}")
endif()

FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
    message(STATUS "  - stb populated at: ${stb_SOURCE_DIR}")
endif()

FetchContent_GetProperties(cgltf)
if(NOT cgltf_POPULATED)
    FetchContent_Populate(cgltf)
    message(STATUS "  - cgltf populated at: ${cgltf_SOURCE_DIR}")
endif()

FetchContent_GetProperties(tlsf)
if(NOT tlsf_POPULATED)
    FetchContent_Populate(tlsf)
    message(STATUS "  - tlsf populated at: ${tlsf_SOURCE_DIR}")
endif()

FetchContent_GetProperties(bc7enc_rdo)
if(NOT bc7enc_rdo_POPULATED)
    FetchContent_Populate(bc7enc_rdo)
    message(STATUS "  - bc7enc_rdo populated at: ${bc7enc_rdo_SOURCE_DIR}")
endif()

FetchContent_GetProperties(slang)
if(NOT slang_POPULATED)
    FetchContent_Populate(slang)
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
