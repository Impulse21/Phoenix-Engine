cmake_minimum_required(VERSION 3.22)

project(PhxEngine)

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

option(PHX_ENABLE_VULKAN "Enable Vulkan backend" OFF)

# DirectX 12 - Only available on Windows and if the backend is enabled
cmake_dependent_option(
    PHX_ENABLE_DX12
    "Enable DirectX 12 Backend" ON OFF
    IS_WINDOWS
)
cmake_dependent_option(
    BUILD_USING_ONECORE
    "Build for Windows 10/Windows 11 only" OFF OFF
    IS_WINDOWS
)

# GDK - Only available on Windows and if DirectX 12 is enabled
cmake_dependent_option(
    PHX_ENABLE_GDK
    "Enable GDK for PC" OFF OFF
    IS_WINDOWS
)

if(PHX_ENABLE_DX12 AND PHX_ENABLE_VULKAN)
    message(FATAL_ERROR "Only one backend can be enabled: PHX_ENABLE_DX12 or PHX_ENABLE_VULKAN.")
endif()

if (IS_WINDOWS)
    if(NOT MSVC)
      message(FATAL_ERROR "ERROR: This CMake file requires MSVC or MSVC-like compiler.")
    endif()

endif()

if (BUILD_USING_ONECORE)
    set(CMAKE_CXX_STANDARD_LIBRARIES "onecore_apiset.lib user32.lib")
    set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "onecore_apiset.lib user32.lib")
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

# Common compile options for multiple targets
set(COMMON_COMPILE_OPTIONS
    # All warnings, warnings as errors, be pedantic.
    -Wall
    -Wextra
    -Werror
    -Wpedantic

    # Disable warnings about C++98 incompatibility. We're using C++20 features...
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-old-style-cast
    -Wno-float-equal
    -Wno-reserved-identifier
    -Wno-newline-eof
    -Wno-switch
    -Wno-switch-enum
    -Wno-switch-default
    -Wno-covered-switch-default
    -Wno-reorder-ctor
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
)

add_subdirectory(3rdparty)
add_subdirectory(phxengine)
add_subdirectory(phxeditor)
#add_subdirectory(Tests)
#add_subdirectory(Tools/PhxAssetConverter)
#add_subdirectory(Tools/PhxShaderCompiler)