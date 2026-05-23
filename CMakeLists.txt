cmake_minimum_required(VERSION 3.28)
project(PHX VERSION 0.1.0 LANGUAGES C CXX)

# -- Helpers ---
include(cmake/Compiler.cmake)
include(cmake/Platform.cmake)
include(cmake/Vendor.cmake)

# -- Options ---
option(PHX_BUILD_SAMPLES "Build sample applications" OFF)

# -- Create LibraryW ---
add_library(PhxEngine STATIC)

target_include_directories(PhxEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
phx_apply_force_include(PhxEngine)
target_link_libraries(PhxEngine PRIVATE spdlog::spdlog)

target_sources(PhxEngine PRIVATE
    "PhxEngine/Engine.cpp"
    "PhxEngine/Core/Log.cpp"
)

# -- Platform ---
if(PHX_PLATFORM_WINDOWS)
    target_sources(PhxEngine PRIVATE
        "PhxEngine/Platform/Windows/Main.cpp"
    )
    
elseif(PHX_PLATFORM_LINUX)
    target_sources(PhxEngine PRIVATE
        "PhxEngine/Platform/Linux/Main.cpp"
    )
    
endif()

# -- RHI backend ---
if(PHX_RHI_BACKEND STREQUAL "VULKAN")
    target_compile_definitions(PhxEngine PUBLIC PHX_RHI_VULKAN)
    
    find_package(Vulkan REQUIRED)
    target_link_libraries(PhxEngine PRIVATE Vulkan::Vulkan)


elseif(PHX_RHI_BACKEND STREQUAL "DX12")
    if(NOT PHX_PLATFORM_WINDOWS)
        message(FATAL_ERROR "PHX: DX12 backend is Windows only")
    endif()

    target_compile_definitions(PhxEngine PUBLIC PHX_RHI_DX12)
    target_link_libraries(PhxEngine PRIVATE d3d12.lib dxgi.lib dxguid.lib)

endif()

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    )
endif()

if(PHX_BUILD_SAMPLES)
    add_subdirectory(Samples)
endif()
