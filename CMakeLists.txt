project(PhxEngine)
cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")

# Check the active compiler
if (MSVC)
    set(USING_MSVC TRUE)
    message(STATUS "Using MSVC Compiler")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(USING_CLANG TRUE)
    message(STATUS "Using LLVM/Clang Compiler")
else()
    message(WARNING "Unknown Compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

string(APPEND CMAKE_CXX_FLAGS_DEBUG " -D_DEBUG")
include(CMakeDependentOption)

# Detect platform
if (WIN32)
    set(IS_WINDOWS TRUE)
else()
    set(IS_WINDOWS FALSE)
endif()

option(PHX_TARGET_GDK "Target GDK for PC" ON)
option(PHX_ENABLE_VULKAN "Enable Vulkan backend" OFF)

# DirectX 12 - Only available on Windows and if the backend is enabled
cmake_dependent_option(
    PHX_ENABLE_DX12
    "Enable DirectX 12 Backend" ON OFF
    IS_WINDOWS
)

# GDK - Only available on Windows and if DirectX 12 is enabled
cmake_dependent_option(
    PHX_ENABLE_GDK
    "Enable GDK for PC" OFF OFF
    IS_WINDOWS
)

if(PHX_ENABLE_DX12 AND PHX_ENABLE_VULKAN)
    message(FATAL_ERROR "Only one backend can be enabled: USE_DX12 or USE_VULKAN.")
endif()
#if (USING_MSVC)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX /MP")
#endif()


# Set Windows SDK Version
set(CMAKE_SYSTEM_VERSION 10.0.19041.0)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Output/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_PROFILE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# Output everything to Bin because some commands copies it to destination exe
# Not the best way, could get rid of .libs, but for now it'll do




include(CMakeDependentOption)
add_subdirectory(PhxEngine)
#add_subdirectory(PhxEditor)
#add_subdirectory(Tests)
#add_subdirectory(Tools/PhxAssetConverter)
#add_subdirectory(Tools/PhxShaderCompiler)