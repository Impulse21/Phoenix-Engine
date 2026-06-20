if(WIN32)
    add_compile_definitions(PHX_PLATFORM_WINDOWS NOMINMAX)
    set(PHX_PLATFORM_WINDOWS ON)

    # DX12 only available on Windows
    set(PHX_RHI_BACKEND_OPTIONS "VULKAN;DX12")
    set(PHX_RHI_BACKEND "VULKAN" CACHE STRING
        "RHI backend to build against (VULKAN or DX12)")
        
elseif(UNIX AND NOT APPLE)
    add_compile_definitions(PHX_PLATFORM_LINUX)
    set(PHX_PLATFORM_LINUX ON)

    # Linux — Vulkan only
    set(PHX_RHI_BACKEND "VULKAN" CACHE STRING "RHI backend (Linux: VULKAN only)" FORCE)
else()
    message(FATAL_ERROR "PHX: unsupported platform")
endif()

message(STATUS "PHX platform : ${CMAKE_SYSTEM_NAME}")
message(STATUS "PHX RHI      : ${PHX_RHI_BACKEND}")