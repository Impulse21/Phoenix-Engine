cmake_minimum_required(VERSION 3.28)
project(PHX VERSION 0.1.0 LANGUAGES C CXX)

# -- Helpers ---
include(cmake/Compiler.cmake)
include(cmake/Platform.cmake)
include(cmake/Vendor.cmake)
include(cmake/SlangShader.cmake) # Shader compilation helper

# -- Options ---
option(PHX_BUILD_SAMPLES "Build sample applications" OFF)

# -- Create LibraryW ---
add_library(PhxEngine STATIC)

target_include_directories(PhxEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
phx_apply_force_include(PhxEngine)

target_link_libraries(PhxEngine PRIVATE spdlog::spdlog)
set_target_properties(spdlog PROPERTIES SYSTEM TRUE)

target_link_libraries(PhxEngine PRIVATE glfw)
set_target_properties(glfw PROPERTIES SYSTEM TRUE)

target_link_libraries(PhxEngine PRIVATE slang::slang)
set_target_properties(slang::slang PROPERTIES SYSTEM TRUE)

target_sources(PhxEngine PRIVATE
    "PhxEngine/Engine.cpp"
    
    "PhxEngine/Core/Log.cpp"
    "PhxEngine/Core/UUID.cpp"
    "PhxEngine/Core/CVar.cpp"
    "PhxEngine/Core/Thread.cpp"

    "PhxEngine/VFS/VFS.cpp"

    "PhxEngine/Memory/Memory.cpp"
    "PhxEngine/Memory/LinearAllocator.cpp"
    "PhxEngine/Memory/VirtualMemoryArena.cpp"
    # "PhxEngine/Memory/TlsfHeapAllocator.cpp"
    "PhxEngine/Memory/tlsf/tlsf.cpp"

    "PhxEngine/Renderer/RenderGraph.cpp"
    "PhxEngine/Renderer/RenderGraphRegistry.cpp"
    "PhxEngine/Renderer/ShaderCompiler.cpp"
)

# -- Platform ---
if(PHX_PLATFORM_WINDOWS)
    target_sources(PhxEngine PRIVATE
        "PhxEngine/Platform/Windows/Main.cpp"
        "PhxEngine/Platform/Windows/VirtualMemory.cpp"
        "PhxEngine/Platform/Windows/OSWindow.cpp"
        "PhxEngine/Platform/Windows/Thread.cpp"
        "PhxEngine/PLatform/Windows/IO.cpp"
    )
    
elseif(PHX_PLATFORM_LINUX)
    target_sources(PhxEngine PRIVATE
        "PhxEngine/Platform/Linux/Main.cpp"
        "PhxEngine/Platform/Linux/VirtualMemory.cpp"
        "PhxEngine/Platform/Linux/OSWindow.cpp"
        "PhxEngine/Platform/Linux/Thread.cpp"
        "PhxEngine/Platform/Linux/IO.cpp"
    )
    
endif()

# -- RHI backend ---
if(PHX_RHI_BACKEND STREQUAL "VULKAN")

    target_sources(PhxEngine PRIVATE
        "PhxEngine/RHI/Vulkan/VulkanRHIInfo.cpp"
        "PhxEngine/RHI/Vulkan/VulkanSetup.cpp"
        "PhxEngine/RHI/Vulkan/VulkanSubmission.cpp"
        "PhxEngine/RHI/Vulkan/VulkanResources.cpp"
        "PhxEngine/RHI/Vulkan/VulkanViewport.cpp"
        "PhxEngine/RHI/Vulkan/VulkanCmdBuffer.cpp"
        "PhxEngine/RHI/Vulkan/VulkanDescriptorHeap.cpp"
        "PhxEngine/RHI/Vulkan/VulkanDescriptorSystem.cpp"
    )

    # -- Vulkan platform specific units --
    if(PHX_PLATFORM_WINDOWS)
        target_sources(PhxEngine PRIVATE
            "PhxEngine/Platform/Windows/OSWindowVulkan.cpp"
        )
    
    elseif(PHX_PLATFORM_LINUX)
        target_sources(PhxEngine PRIVATE
            "PhxEngine/Platform/Linux/OSWindowVulkan.cpp"
        )
    
    endif()

    target_compile_definitions(PhxEngine PUBLIC 
        PHX_RHI_VULKAN
        VK_NO_PROTOTYPES
        VMA_STATIC_VULKAN_FUNCTIONS=0
        VMA_DYNAMIC_VULKAN_FUNCTIONS=1)
    
    find_package(Vulkan REQUIRED)
    target_link_libraries(PhxEngine PRIVATE
        volk                        # replaces Vulkan::Vulkan
        Vulkan::Headers             # headers only, no loader lib
        GPUOpen::VulkanMemoryAllocator
    )

    if(PHX_PLATFORM_LINUX)
        target_link_libraries(PhxEngine PRIVATE dl)   # dlopen for volk
    endif()

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
