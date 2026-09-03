
#include "RHIVulkan.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/StaticArray.h>
#include <PhxEngine/Core/Thread.h>
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

    // Rewind this frame-in-flight slot of the GpuTempMalloc ring — safe
    // because frame_slot only comes back around after MaxFramesInFlight
    // other frames have finished (same guarantee the wait above provides
    // for command buffer reuse).
    g_context.gpu_temp_ring.slot_offset[frame_slot] = 0;

    // The swapchain image's UNDEFINED/PRESENT_SRC_KHR -> GENERAL transition
    // happens lazily in BeginRenderPass (see TransitionToGeneral in
    // VulkanCmdBuffer.cpp), which correctly tracks per-image whether this is
    // the first-ever use (UNDEFINED) or a reused slot (PRESENT_SRC_KHR from
    // the previous SubmitAndPresent) — there's nothing left to do here.

    return true;
}

bool phx::rhi::SubmitAndPresent(Span<CommandBuffer> cmds)
{
    PHX_ASSERT(cmds.Size() + 1 <= k_max_raw_per_frame); // +1 for the internal present-barrier cmd below

    const u64 frame_slot = g_context.GetCurrentFrame();
    auto* viewport = &g_context.viewport;
    const u64 current_image = viewport->curr_image_index;

    // Present needs the swapchain image in PRESENT_SRC_KHR — an internal RHI
    // concern the caller shouldn't need to think about, so this records one
    // more small command buffer beyond whatever was passed in.
    rhi::CommandBuffer present_cmd = rhi::BeginCommandRecording(CommandQueueType::Graphics);
    VkCommandBuffer vk_present_cmd = vulkan::ToVkCommandBuffer(present_cmd);

    VkImageMemoryBarrier2 to_present = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask    = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask     = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        .dstAccessMask    = VK_ACCESS_2_NONE,
        .oldLayout        = VK_IMAGE_LAYOUT_GENERAL, // unifiedImageLayouts — see TransitionToGeneral
        .newLayout        = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, // WSI presentation is the one usage the extension doesn't unify away
        .image            = viewport->vk_images[current_image],
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
        .pImageMemoryBarriers    = &to_present,
    };

    vkCmdPipelineBarrier2(vk_present_cmd, &dep_info);

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

    // Submit exactly what the caller asked for, plus our own internal
    // present-barrier command buffer — not "whatever this frame opened."
    VkCommandBufferSubmitInfo cmd_submit_info[k_max_raw_per_frame];
    u32 num_cmd_si = 0;

    for (const CommandBuffer& cmd : cmds)
    {
        VkCommandBuffer vk_cmd = vulkan::ToVkCommandBuffer(cmd);
        vkEndCommandBuffer(vk_cmd);

        cmd_submit_info[num_cmd_si++] = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .pNext = nullptr,
            .commandBuffer = vk_cmd,
            .deviceMask = 0
        };
    }

    vkEndCommandBuffer(vk_present_cmd);
    cmd_submit_info[num_cmd_si++] = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext = nullptr,
        .commandBuffer = vk_present_cmd,
        .deviceMask = 0
    };

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

// -- Independent upload/transfer submission -----------------------------------

UploadTicket phx::rhi::SubmitUpload(CommandBuffer cmd)
{
    PHX_ASSERT(Thread::IsMainThread());
    PHX_ASSERT(cmd.IsValid());

    VkCommandBuffer vk_cmd = vulkan::ToVkCommandBuffer(cmd);
    vkEndCommandBuffer(vk_cmd);

    const u64 ticket = ++g_context.upload_submit_count;

    VkCommandBufferSubmitInfo cmd_submit_info = {
        .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = vk_cmd,
    };

    VkSemaphoreSubmitInfo signal_info = {
        .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = g_context.vk_upload_timeline_sem,
        .value     = ticket,
        .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
    };

    VkSubmitInfo2 submit_info = {
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .commandBufferInfoCount   = 1,
        .pCommandBufferInfos      = &cmd_submit_info,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos    = &signal_info,
    };

    vulkan_check(
        vkQueueSubmit2(g_context.vk_transfer_queue, 1, &submit_info, VK_NULL_HANDLE));

    // Free this command buffer once its ticket is confirmed done — deferred,
    // never blocks SubmitUpload itself.
    g_context.upload_deferred_queue.EnqueueDelete({
        .frame = ticket,
        .deferred_func = [vk_cmd]() {
            vkFreeCommandBuffers(g_context.vk_device, g_context.vk_upload_cmd_pool, 1, &vk_cmd);
        }
    });

    // Opportunistic non-blocking reclaim of anything already complete.
    u64 completed = 0;
    vkGetSemaphoreCounterValue(g_context.vk_device, g_context.vk_upload_timeline_sem, &completed);
    g_context.upload_deferred_queue.Flush(completed + 1); // see WaitForUpload for the "+1" boundary

    // Hand the ring off to the next slot; that slot's previous occupant (if
    // any) is only waited on lazily, the next time GpuUploadMalloc actually
    // needs to write into it — SubmitUpload itself never blocks.
    GpuUploadRing& ring = g_context.gpu_upload_ring;
    ring.slot_ticket[ring.current_slot]     = ticket;
    ring.slot_needs_wait[ring.current_slot] = true;
    ring.current_slot = (ring.current_slot + 1) % GpuUploadRing::kSlotCount;

    return ticket;
}

void phx::rhi::WaitForUpload(UploadTicket ticket)
{
    if (ticket == 0)
        return;

    VkSemaphoreWaitInfo wait_info = {
        .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores    = &g_context.vk_upload_timeline_sem,
        .pValues        = &ticket,
    };

    vulkan_check(
        vkWaitSemaphores(g_context.vk_device, &wait_info, UINT64_MAX));

    // DeferredCallbackQueue<0>::Flush frees items with `frame < completed_frame`
    // — pass ticket+1 so the item enqueued with `.frame = ticket` itself is
    // included (it's now provably done, since we just waited on it).
    g_context.upload_deferred_queue.Flush(ticket + 1);
}