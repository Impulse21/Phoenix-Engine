#include "RHIVulkan.h"

#include <PhxEngine/Core/Thread.h>

using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::vulkan;

bool rhi::BeginCommandRecording(CommandBufferHandle handle)
{
    if (!handle.IsValid())
        return false;

    CommandBufferImpl* cmd_impl = g_context.pool_cmd_buffer.Get(handle);

    // Only support one thtread at the moment.
    PHX_ASSERT(Thread::IsMainThread());
    if (cmd_impl->cmd_buffer !=  VK_NULL_HANDLE)
    {
        PHX_LOG_ERROR(Log::Channels::RHI, "Command Buffer is already started");
        PHX_ASSERT(false);
        return false;
    }
    
    // Determine what queue we are and claim a vk_command_buffer
    switch (cmd_impl->queue_type)
    {
    case CommandQueueType::Graphics:
        break;
    case CommandQueueType::Compute:
    case CommandQueueType::Copy:
    default:
        PHX_LOG_ERROR(Log::Channels::RHI, "Unsupported command Queue");
        return false;
    }
    
    FrameContext& frame_ctx = g_context.GetCurrentFrameCtx();
    if (frame_ctx.cmd_in_use == k_max_raw_per_frame)
    {
        PHX_LOG_ERROR(Log::Channels::RHI, "Ran out of internal command buffers");
        PHX_ASSERT(false);

        return false;
    }

    VkCommandBuffer& vk_cmd_buffer = frame_ctx.vk_cmd_buffers[frame_ctx.cmd_in_use++];
    if (vk_cmd_buffer == VK_NULL_HANDLE)
    {
        VkCommandBufferAllocateInfo cmd_alloc_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = frame_ctx.vk_cmd_buffer_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        
        vulkan_check(
            vkAllocateCommandBuffers(g_context.vk_device, &cmd_alloc_info, &vk_cmd_buffer));
    }

    cmd_impl->cmd_buffer = vk_cmd_buffer;
 
    
    VkCommandBufferBeginInfo cmd_buffer_bi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };

    vkBeginCommandBuffer(cmd_impl->cmd_buffer, &cmd_buffer_bi);
    
    return true;
}