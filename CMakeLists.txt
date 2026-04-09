cmake_minimum_required(VERSION 3.28)

project(PhxEngine
    VERSION 1.0.0
    LANGUAGES CXX C
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for LazyVim/VSCode
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Enable solution folders for Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Platform detection
if(WIN32)
  set(PLATFORM_WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
  set(PLATFORM_LINUX TRUE)
endif()

# Build configurations
set(CMAKE_CONFIGURATION_TYPES
    "Debug;RelWithDebInfo;Release"
    CACHE STRING "" FORCE)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# FetchContent setup
include(FetchContent)

# Set FetchContent base directory
set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/_deps)

include(Utils)

#==============================================================================
# Dependencies - All FetchContent declarations
#==============================================================================

include(Dependencies)

# Vulkan SDK (system-installed)
find_package(Vulkan REQUIRED)


#==============================================================================
# Compiler Settings
#==============================================================================
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

# Global compiler warnings and settings
if(MSVC)
  add_compile_options(
        /W4
        /WX  # Warnings as errors
        /permissive-
        /Zc:__cplusplus
        /wd4201  # nameless struct/union
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(${CLANG_WARNING_OVERRIDES})
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        # Disable specific warnings as per your Premake config
        ${CLANG_WARNING_OVERRIDES}
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

if(MSVC OR CMAKE_GENERATOR MATCHES "Visual Studio")    
    include(VisualStudioFilters)
    # Choose filter style: DIRECTORY, TYPE, or HYBRID
    set_default_filter_style(DIRECTORY)
endif()

# Platform-specific definitions
if(PLATFORM_WINDOWS)
    add_compile_definitions(
        PHX_PLATFORM_WINDOWS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        VC_EXTRALEAN
        _CRT_SECURE_NO_WARNINGS
        HLSLPP_FEATURE_TRANSFORM
    )
elseif(PLATFORM_LINUX)
    add_compile_definitions(
        PHX_PLATFORM_LINUX
        HLSLPP_FEATURE_TRANSFORM
    )
endif()

# RHI Backend - Default to Vulkan
add_compile_definitions(PHX_RHI_VULKAN)

#==============================================================================
# Vendor Libraries
#==============================================================================

# Put vendor projects in "Vendors" solution folder
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

# Create vendor library targets
add_subdirectory(vendor)

#==============================================================================
# Phoenix Engine Libraries
#==============================================================================

add_subdirectory(PhxLibs/PhxCore)
add_subdirectory(PhxLibs/PhxAsset)
add_subdirectory(PhxLibs/PhxRhi)
add_subdirectory(PhxLibs/PhxRenderer)
add_subdirectory(PhxLibs/PhxResource)
add_subdirectory(PhxLibs/PhxWorld)
add_subdirectory(PhxLibs/PhxEngine)

add_subdirectory(PhxLibs/PhxResourceCompiler)

#==============================================================================
# Applications
#==============================================================================
add_subdirectory(PhxApps/PhxRuntime)
add_subdirectory(PhxApps/PhxResourceCompiler)
