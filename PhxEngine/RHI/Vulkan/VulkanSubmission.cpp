
#include "RHIVulkan.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/StaticArray.h>

using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::vulkan;

bool phx::rhi::BeginFrame(ViewportHandle viewportHandle)
{
    const u64 frame_slot = g_context.GetCurrentFrame();
    const u64 wait_fence_value = g_context.frame_wait_values[frame_slot];
    auto* viewport = g_context.pool_viewports.Get(viewportHandle);

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
        viewport->vk_image_available_sem[frame_slot],
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

    // TODO: reset Command pool

    return true;
}

bool phx::rhi::EndFrame(ViewportHandle viewportHandle)
{
    const u64 frame_slot = g_context.GetCurrentFrame();
    auto* viewport = g_context.pool_viewports.Get(viewportHandle);

    const uint64_t signal_value = ++g_context.frame_number;


    VkSemaphoreSubmitInfo wait_sem
    {
        .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = viewport->vk_image_available_sem[frame_slot],
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    };


    VkSemaphoreSubmitInfo sig_sem[2] = 
    {
        {
            .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = viewport->vk_render_finished_sem[frame_slot],
            .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
        },
        {
            .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = g_context.vk_timeline_sem,
            .value     = signal_value,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        },

    };

    VkSubmitInfo2 submit_info
    {
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount   = 1,
        .pWaitSemaphoreInfos      = &wait_sem,
        .commandBufferInfoCount   = 0u,
        .pCommandBufferInfos      = nullptr,
        .signalSemaphoreInfoCount = 2,
        .pSignalSemaphoreInfos    = sig_sem,
    };

    vulkan_check(
        vkQueueSubmit2(g_context.vk_gfx_queue, 1, &submit_info, VK_NULL_HANDLE));


    VkPresentInfoKHR present_info
    {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &viewport->vk_render_finished_sem[frame_slot],
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