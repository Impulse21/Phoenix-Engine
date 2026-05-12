cmake_minimum_required(VERSION 3.28)
project(PHX VERSION 0.1.0 LANGUAGES C CXX)

# -- Helpers ---
include(cmake/Compiler.cmake)
include(cmake/Platform.cmake)
include(cmake/Vendor.cmake)

# -- Options ---
option(PHX_BUILD_SAMPLES "Build sample applications" OFF)

# -- Create LibraryW ---
add_library(libPhxEngine STATIC)

target_include_directories(libPhxEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
phx_apply_force_include(libPhxEngine)
target_link_libraries(libPhxEngine PRIVATE spdlog::spdlog)

target_sources(libPhxEngine PRIVATE
    "PhxEngine/Engine.cpp"
)

# -- Platform ---
if(PHX_PLATFORM_WINDOWS)
    target_sources(libPhxEngine PRIVATE ${PHX_PLATFORM_WIN32})
elseif(PHX_PLATFORM_LINUX)
    target_sources(libPhxEngine PRIVATE ${PHX_PLATFORM_LINUX})
endif()

# -- RHI backend ---
# 3. Platform Configuration
if(PHX_PLATFORM_WINDOWS)
    target_sources(libPhxEngine PRIVATE
        "PhxEngine/Platform/Windows/Main.cpp"
    )
elseif(PHX_PLATFORM_LINUX)
    target_sources(libPhxEngine PRIVATE
        "PhxEngine/Platform/Linux/Main.cpp"
    )
    
endif()

if(PHX_RHI_BACKEND STREQUAL "VULKAN")
    target_compile_definitions(libPhxEngine PUBLIC PHX_RHI_VULKAN)
    
    find_package(Vulkan REQUIRED)
    target_link_libraries(libPhxEngine PRIVATE Vulkan::Vulkan)


elseif(PHX_RHI_BACKEND STREQUAL "DX12")
    if(NOT PHX_PLATFORM_WINDOWS)
        message(FATAL_ERROR "PHX: DX12 backend is Windows only")
    endif()

    target_compile_definitions(libPhxEngine PUBLIC PHX_RHI_DX12)
    target_link_libraries(libPhxEngine PRIVATE d3d12.lib dxgi.lib dxguid.lib)

endif()

if(PHX_BUILD_SAMPLES)
    add_subdirectory(Samples)
endif()