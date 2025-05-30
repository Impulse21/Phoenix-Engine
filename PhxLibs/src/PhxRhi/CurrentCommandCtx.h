#pragma once

namespace phx::rhi
{

#if defined(PHX_RHI_VULKAN)
#include <PhxRhi/vulkan/VkCommandCtx.h>
    namespace phx::rhi 
    {
        // This is the type your engine code will refer to as phx::rhi::CommandBuffer
        using CommandBuffer = vk::VkCommandCtxImpl;
    }
#elif defined(PHX_RHI_D3D12)

#error "Not Implmeneted"
#else
#error "No RHI backend selected for CommandBuffer"
#endif
}