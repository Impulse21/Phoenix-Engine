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

    // Disable this for now as the way I am currently submitting the CMD Buffers
    // I am using the frame context pool directly to avoid building an array of commands
    // to submit.
#if false
    if (cmd_impl->cmd_buffer !=  VK_NULL_HANDLE)
    {
        PHX_LOG_ERROR(Log::Channels::RHI, "Command Buffer is already started");
        PHX_ASSERT(false);
        return false;
    }
#endif    
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

namespace
{
    void BeginRendering(
        VkImageView rt_view, const ClearValue rt_clear,
        VkImageView ds_view, const ClearValue& ds_clear,
        const VkRect2D& rect,
        VkCommandBuffer cmd)
    {

        VkClearValue vk_rt_clear = {
            .color = {
                .float32 = { 
                    rt_clear.colour[0],
                    rt_clear.colour[1],
                    rt_clear.colour[2],
                    rt_clear.colour[3] }
            }
        };

        VkRenderingAttachmentInfo color_attachment_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = rt_view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = vk_rt_clear
        };

        const bool has_depth = ds_view != VK_NULL_HANDLE;
        VkRenderingAttachmentInfo depth_attachment_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        };

        if (has_depth)
        {
            VkClearValue vk_depth_clear = {
                .depthStencil = {
                    .depth = ds_clear.depth_stencil.depth,
                    .stencil = ds_clear.depth_stencil.stencil,
                }
            };

            depth_attachment_info.imageView = ds_view;
            depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depth_attachment_info.clearValue = vk_depth_clear;
        }

        VkRenderingInfo rendering_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {
                .offset = {.x = 0u, .y = 0u},
                .extent = rect.extent
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment_info,
            .pDepthAttachment = has_depth ? &depth_attachment_info : nullptr,
            .pStencilAttachment = nullptr,
        };

        vkCmdBeginRendering(cmd, &rendering_info);
    }
}
void rhi::BeginRendering(
        TextureHandle texture,
        const ClearValue& clear,
        TextureHandle depth_texture,
        const ClearValue& depth_clear_value,
        CommandBufferHandle cmd_handle)
{
    PHX_ASSERT(cmd_handle.IsValid());
    CommandBufferImpl* cmd_impl = g_context.pool_cmd_buffer.Get(cmd_handle);

    vulkan::VulkanTexture* render_target = g_context.pool_textures.Get(texture);
    PHX_ASSERT(render_target);

    VkImageView ds_view = VK_NULL_HANDLE;
    if (depth_texture.IsValid())
    {
        vulkan::VulkanTexture* depth_target = g_context.pool_textures.Get(depth_texture);
        ds_view = depth_target->vk_view_dsv;
    }

    VkRect2D rect = { 
        .extent = { 
            .width = render_target->width,
            .height = render_target->height
        }
    };
    
    ::BeginRendering(
        render_target->vk_view_rtv, clear,
        ds_view, depth_clear_value,
        rect,
        cmd_impl->cmd_buffer);    

}

void rhi::BeginRendering(ViewportHandle viewport, const ClearValue& clear,
                         CommandBufferHandle cmd_handle)
{
    PHX_ASSERT(cmd_handle.IsValid());
    CommandBufferImpl* cmd_impl = g_context.pool_cmd_buffer.Get(cmd_handle);

    ViewportImpl* viewport_impl = g_context.pool_viewports.Get(viewport);
    PHX_ASSERT(viewport_impl);

    VkImageView ds_view = VK_NULL_HANDLE;

    VkRect2D rect = { 
        .extent = { 
            .width = viewport_impl->width,
            .height = viewport_impl->height
        }
    };

    ::BeginRendering(
        viewport_impl->GetCurrentImageView(), clear,
        ds_view, {},
        rect,
        cmd_impl->cmd_buffer);
}

void rhi::EndRendering(CommandBufferHandle cmd_handle)
{
    PHX_ASSERT(cmd_handle.IsValid());
    CommandBufferImpl* cmd_impl = g_context.pool_cmd_buffer.Get(cmd_handle);

    vkCmdEndRendering(cmd_impl->cmd_buffer);
}