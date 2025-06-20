#pragma once

#if defined(PHX_RHI_VULKAN)

#include <PhxRhi/vulkan/VkGfxDevice.h>
#include <PhxRhi/vulkan/VkCommandCtx.h>

    namespace phx::rhi 
    {
        // This is the type your engine code will refer to as phx::rhi::CommandBuffer
        using GfxDevice = vk::VkGfxDeviceImpl;
        using GfxCommandCtx = vk::VkGfxCommandCtx;
        using ComputeCommandCtx = vk::VkComputeCommandCtx;
        using CopyCommandCtx = vk::VkCopyCommandCtx;
    }

#elif defined(PHX_RHI_D3D12)

#error "Not Implmeneted"

#else

#error "No RHI backend selected. Please define PHX_RHI_VULKAN or PHX_RHI_D3D12, etc."

#endif