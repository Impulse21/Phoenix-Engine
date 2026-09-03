
#include "RHIVulkan.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/StaticArray.h>
#include <PhxEngine/Memory/MemoryHelpers.h>

using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::vulkan;

bool phx::rhi::BeginFrame()
{
    const u64 frame_slot = g_context.GetCurrentFrame();
    const u64 wait_fence_value = g_context.frame_wait_values[frame_slot];
    auto* viewport = &g_context.viewport;

    VkSemaphoreWaitInfo wait_info = {
        .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores    = &g_context.vk_timeline_sem,
        .pValues        = &wait_fence_value,
    };
 
    vulkan_check(
        vkWaitSemaphores(g_context.vk_device, &wait_info, UINT64_MAX));
 
    g_context.deferred_callback_queue.Flush(wait_fence_value);
    
    const VkResult acquire_result = vkAcquireNextImageKHR(
        g_context.vk_device,
        viewport->vk_swapchain,
        UINT64_MAX,
        viewport->vk_image_available_sem[viewport->curr_sem_index],
        VK_NULL_HANDLE,
        &viewport->curr_image_index);
 
    if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        PHX_LOG_INFO(
            Log::Channels::RHI,
            "Swapchain out of date on acquire — skipping frame");
        return false;
    }
 
    if (acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR)
    {
        PHX_LOG_ERROR(
            Log::Channels::RHI,
            "SvkAcquireNextImageKHR failed: %d",
            (int)acquire_result);

        return false;
    }

    auto& current_frame = g_context.frame_ctx[frame_slot];
    vulkan_check(
        vkResetCommandPool(
            g_context.vk_device,
            current_frame.vk_cmd_buffer_pool,
            0
    ));

    current_frame.cmd_in_use = 0;

    // The swapchain image's UNDEFINED/PRESENT_SRC_KHR -> GENERAL transition
    // happens lazily in BeginRenderPass (see TransitionToGeneral in
    // VulkanCmdBuffer.cpp), which correctly tracks per-image whether this is
    // the first-ever use (UNDEFINED) or a reused slot (PRESENT_SRC_KHR from
    // the previous EndFrame) — there's nothing left to do here.

    return true;
}

bool phx::rhi::EndFrame()
{
    const u64 frame_slot = g_context.GetCurrentFrame();
    auto* viewport = &g_context.viewport;
    const u64 current_image = viewport->curr_image_index;
    auto& current_frame = g_context.frame_ctx[frame_slot];

    VkImageMemoryBarrier2 to_render = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask    = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask     = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        .dstAccessMask    = VK_ACCESS_2_NONE,
        .oldLayout        = VK_IMAGE_LAYOUT_GENERAL, // unifiedImageLayouts — see TransitionToGeneral
        .newLayout        = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // WSI presentation is the one usage the extension doesn't unify away
        .image            = viewport->vk_images[viewport->curr_image_index],
        .subresourceRange = { 
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
    };

    VkDependencyInfo dep_info = {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &to_render,
    };
    
    rhi::CommandBuffer end_frame_cmd = rhi::BeginCommandRecording(CommandQueueType::Graphics);
    vkCmdPipelineBarrier2(vulkan::ToVkCommandBuffer(end_frame_cmd), &dep_info);
    
    const u64 signal_value = ++g_context.frame_number;
    const u32 curr_sem = viewport->curr_sem_index;
    viewport->curr_sem_index = (curr_sem + 1) % viewport->image_count;

    VkSemaphoreSubmitInfo wait_sem
    {
        .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = viewport->vk_image_available_sem[curr_sem],
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    };

    VkSemaphoreSubmitInfo sig_sem[2] = {
        {
            .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = viewport->vk_render_finished_sem[current_image],
            .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
        },
        {
            .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = g_context.vk_timeline_sem,
            .value     = signal_value,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        },
    };

    VkCommandBufferSubmitInfo cmd_submit_info[k_max_raw_per_frame];
    const u32 num_cmd_si = current_frame.cmd_in_use;

    for (u32 i = 0; i < num_cmd_si; ++i)
    {
        VkCommandBuffer cmd_buffer = current_frame.vk_cmd_buffers[i];
        vkEndCommandBuffer(cmd_buffer);

        cmd_submit_info[i] = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext = nullptr,
            .commandBuffer = cmd_buffer,
            .deviceMask = 0
        };
    }

    VkSubmitInfo2 submit_info = {
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount   = 1,
        .pWaitSemaphoreInfos      = &wait_sem,
        .commandBufferInfoCount   = num_cmd_si,
        .pCommandBufferInfos      = cmd_submit_info,
        .signalSemaphoreInfoCount = 2,
        .pSignalSemaphoreInfos    = sig_sem,
    };

    vulkan_check(
        vkQueueSubmit2(g_context.vk_gfx_queue, 1, &submit_info, VK_NULL_HANDLE));

    VkPresentInfoKHR present_info = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &viewport->vk_render_finished_sem[current_image],
        .swapchainCount     = 1,
        .pSwapchains        = &viewport->vk_swapchain,
        .pImageIndices      = &viewport->curr_image_index,
    };
 
    const VkResult present_result = vkQueuePresentKHR(g_context.vk_present_queue, &present_info);

    g_context.frame_wait_values[frame_slot] = signal_value;
 
    if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR)
    {
        PHX_LOG_INFO(
            Log::Channels::RHI,
            "Swapchain out of date on present — recreate before next frame");

        return false;
    }
 
    if (present_result != VK_SUCCESS)
    {
        PHX_LOG_ERROR(
            Log::Channels::RHI,
            "vkQueuePresentKHR failed: %d",
            (int)present_result);

        return false;
    }
 
    return true;

}