cmake_minimum_required(VERSION 3.28)
project(PHX VERSION 0.1.0 LANGUAGES C CXX)

# -- Helpers ---
include(cmake/Compiler.cmake)
include(cmake/Platform.cmake)

# -- Options ---
option(PHX_BUILD_SAMPLES "Build sample applications" OFF)

# -- Collect Source files ---
file(GLOB_RECURSE PHX_CORE_SOURCES        CONFIGURE_DEPENDS "Engine/Core/*.cpp")
file(GLOB_RECURSE PHX_MEMORY_SOURCES      CONFIGURE_DEPENDS "Engine/Memory/*.cpp")
file(GLOB_RECURSE PHX_JOBS_SOURCES        CONFIGURE_DEPENDS "Engine/Jobs/*.cpp")
file(GLOB_RECURSE PHX_RENDERER_SOURCES    CONFIGURE_DEPENDS "Engine/Renderer/*.cpp")

# Platform backends — globbed separately so only one gets linked
file(GLOB_RECURSE PHX_PLATFORM_WIN32      CONFIGURE_DEPENDS "Engine/Platform/Win32/*.cpp")
file(GLOB_RECURSE PHX_PLATFORM_LINUX      CONFIGURE_DEPENDS "Engine/Platform/Linux/*.cpp")

# RHI backends — same pattern
file(GLOB_RECURSE PHX_RHI_VULKAN          CONFIGURE_DEPENDS "Engine/RHI/Vulkan/*.cpp")
file(GLOB_RECURSE PHX_RHI_DX12            CONFIGURE_DEPENDS "Engine/RHI/DX12/*.cpp")

# Engine.cpp alongside IApp.h at root
file(GLOB PHX_ENGINE_ROOT_SOURCES         CONFIGURE_DEPENDS "Engine/*.cpp")


#  -- Build PhxEngine library ---
add_library(libPhxEngine STATIC 
    ${PHX_ENGINE_ROOT_SOURCES}
    ${PHX_CORE_SOURCES}
    ${PHX_MEMORY_SOURCES}
    ${PHX_JOBS_SOURCES}
    ${PHX_RENDERER_SOURCES}
)

target_include_directories(libPhxEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# -- Platform ---
if(PHX_PLATFORM_WINDOWS)
    target_sources(libPhxEngine PRIVATE ${PHX_PLATFORM_WIN32})
elseif(PHX_PLATFORM_LINUX)
    target_sources(libPhxEngine PRIVATE ${PHX_PLATFORM_LINUX})
endif()

# -- RHI backend ---
if(PHX_RHI_BACKEND STREQUAL "VULKAN")
    target_sources(libPhxEngine PRIVATE ${PHX_RHI_VULKAN})
    target_compile_definitions(libPhxEngine PUBLIC PHX_RHI_VULKAN)

    find_package(Vulkan REQUIRED)
    target_link_libraries(libPhxEngine PRIVATE Vulkan::Vulkan)

elseif(PHX_RHI_BACKEND STREQUAL "DX12")
    if(NOT PHX_PLATFORM_WINDOWS)
        message(FATAL_ERROR "PHX: DX12 backend is Windows only")
    endif()

    target_sources(libPhxEngine PRIVATE ${PHX_RHI_DX12})
    target_compile_definitions(libPhxEngine PUBLIC PHX_RHI_DX12)
    target_link_libraries(libPhxEngine PRIVATE d3d12.lib dxgi.lib dxguid.lib)
endif()

# -- Samples ---
if(PHX_BUILD_SAMPLES)
    add_subdirectory(Samples)
endif()