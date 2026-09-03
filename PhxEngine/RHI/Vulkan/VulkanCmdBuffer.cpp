#include "RHIVulkan.h"

#include <PhxEngine/Core/Thread.h>

using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::vulkan;

CommandBuffer rhi::BeginCommandRecording(CommandQueueType type)
{
    // Only support one thtread at the moment.
    PHX_ASSERT(Thread::IsMainThread());

    if (type == CommandQueueType::Copy)
    {
        // Deliberately not part of frame_ctx[] — its lifecycle is driven by
        // upload ticket completion (see SubmitUpload), not BeginFrame. Every
        // call allocates a fresh command buffer; SubmitUpload frees it once
        // the GPU work it recorded has completed.
        VkCommandBufferAllocateInfo cmd_alloc_info = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext              = nullptr,
            .commandPool        = g_context.vk_upload_cmd_pool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        VkCommandBuffer vk_cmd_buffer = VK_NULL_HANDLE;
        vulkan_check(
            vkAllocateCommandBuffers(g_context.vk_device, &cmd_alloc_info, &vk_cmd_buffer));

        VkCommandBufferBeginInfo cmd_buffer_bi = {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext            = nullptr,
            .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr,
        };
        vkBeginCommandBuffer(vk_cmd_buffer, &cmd_buffer_bi);

        return vulkan::FromVkCommandBuffer(vk_cmd_buffer);
    }

    switch (type)
    {
    case CommandQueueType::Graphics:
        break;
    case CommandQueueType::Compute:
    default:
        PHX_LOG_ERROR(Log::Channels::RHI, "Unsupported command Queue");
        return {};
    }

    FrameContext& frame_ctx = g_context.GetCurrentFrameCtx();
    if (frame_ctx.cmd_in_use == k_max_raw_per_frame)
    {
        PHX_LOG_ERROR(Log::Channels::RHI, "Ran out of internal command buffers");
        PHX_ASSERT(false);

        return {};
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

    VkCommandBufferBeginInfo cmd_buffer_bi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };

    vkBeginCommandBuffer(vk_cmd_buffer, &cmd_buffer_bi);

    return vulkan::FromVkCommandBuffer(vk_cmd_buffer);
}

namespace
{
    // With VK_KHR_unified_image_layouts, every image lives in GENERAL for its
    // whole life — this is the only layout transition it ever needs, done
    // once on first use. `old_layout` is UNDEFINED the very first time an
    // image is touched, or whatever non-GENERAL layout an outside consumer
    // (the WSI present engine, for swapchain images) last left it in.
    void TransitionToGeneral(VkCommandBuffer cmd, VkImage image, VkImageAspectFlags aspect, VkImageLayout old_layout)
    {
        VkImageMemoryBarrier2 barrier = {
            .sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask  = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
            .oldLayout     = old_layout,
            .newLayout     = VK_IMAGE_LAYOUT_GENERAL,
            .image         = image,
            .subresourceRange = {
                .aspectMask     = aspect,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };

        VkDependencyInfo dep_info = {
            .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers    = &barrier,
        };

        vkCmdPipelineBarrier2(cmd, &dep_info);
    }

    void BeginRenderPass(
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
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL, // unifiedImageLayouts — see TransitionToGeneral
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
            depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL; // unifiedImageLayouts
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

        // Pipelines declare viewport/scissor as dynamic state (VK_DYNAMIC_STATE_*_WITH_COUNT),
        // so every render pass needs these set before any draw. Default to
        // covering the full render target — callers that need less can add
        // a SetViewport/SetScissor call later.
        VkViewport viewport = {
            .x = 0.0f, .y = 0.0f,
            .width = static_cast<float>(rect.extent.width),
            .height = static_cast<float>(rect.extent.height),
            .minDepth = 0.0f, .maxDepth = 1.0f,
        };
        vkCmdSetViewportWithCount(cmd, 1, &viewport);
        vkCmdSetScissorWithCount(cmd, 1, &rect);
    }
}
void rhi::BeginRenderPass(
        TextureHandle texture,
        const ClearValue& clear,
        TextureHandle depth_texture,
        const ClearValue& depth_clear_value,
        CommandBuffer cmd)
{
    PHX_ASSERT(cmd.IsValid());
    VkCommandBuffer vk_cmd = vulkan::ToVkCommandBuffer(cmd);

    vulkan::VulkanTexture* render_target = g_context.pool_textures.Get(texture);
    PHX_ASSERT(render_target);

    if (!render_target->layout_initialized)
    {
        TransitionToGeneral(vk_cmd, render_target->vk_image,
            GetAspectFlags(render_target->vk_format), VK_IMAGE_LAYOUT_UNDEFINED);
        render_target->layout_initialized = true;
    }

    VkImageView ds_view = VK_NULL_HANDLE;
    if (depth_texture.IsValid())
    {
        vulkan::VulkanTexture* depth_target = g_context.pool_textures.Get(depth_texture);
        ds_view = depth_target->vk_view_dsv;

        if (!depth_target->layout_initialized)
        {
            TransitionToGeneral(vk_cmd, depth_target->vk_image,
                GetAspectFlags(depth_target->vk_format), VK_IMAGE_LAYOUT_UNDEFINED);
            depth_target->layout_initialized = true;
        }
    }

    VkRect2D rect = {
        .extent = {
            .width = render_target->width,
            .height = render_target->height
        }
    };

    ::BeginRenderPass(
        render_target->vk_view_rtv, clear,
        ds_view, depth_clear_value,
        rect,
        vk_cmd);
}

void rhi::BeginRenderPass(const ClearValue& clear, CommandBuffer cmd)
{
    PHX_ASSERT(cmd.IsValid());
    VkCommandBuffer vk_cmd = vulkan::ToVkCommandBuffer(cmd);

    ViewportImpl* viewport_impl = &g_context.viewport;

    // Unlike offscreen textures, the swapchain image oscillates every frame:
    // GENERAL while we render into it, PRESENT_SRC_KHR while the WSI owns it
    // for presentation (SubmitAndPresent transitions it back before present). Only
    // its very first use ever starts from UNDEFINED.
    const u32 image_index = viewport_impl->curr_image_index;
    const VkImageLayout old_layout = viewport_impl->vk_image_layout_initialized[image_index]
        ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        : VK_IMAGE_LAYOUT_UNDEFINED;

    TransitionToGeneral(vk_cmd, viewport_impl->GetCurrentImage(), VK_IMAGE_ASPECT_COLOR_BIT, old_layout);
    viewport_impl->vk_image_layout_initialized[image_index] = true;

    VkImageView ds_view = VK_NULL_HANDLE;

    VkRect2D rect = {
        .extent = {
            .width = viewport_impl->width,
            .height = viewport_impl->height
        }
    };

    ::BeginRenderPass(
        viewport_impl->GetCurrentImageView(), clear,
        ds_view, {},
        rect,
        vk_cmd);
}

void rhi::EndRenderPass(CommandBuffer cmd)
{
    PHX_ASSERT(cmd.IsValid());
    vkCmdEndRendering(vulkan::ToVkCommandBuffer(cmd));
}

void rhi::BindPipelineState(PipelineStateHandle pipeline, CommandBuffer cmd)
{
    PHX_ASSERT(cmd.IsValid());
    VkCommandBuffer vk_cmd = vulkan::ToVkCommandBuffer(cmd);

    vulkan::VulkanPipelineState* pipeline_impl = g_context.pool_pipeline_states.Get(pipeline);
    PHX_ASSERT(pipeline_impl);

    vkCmdBindPipeline(vk_cmd, pipeline_impl->bind_point, pipeline_impl->vk_pipeline);

    // Binds the global bindless descriptor buffers (resource + sampler heaps)
    // to the pipeline layout every pipeline shares.
    g_context.descriptor_system.Bind(vk_cmd, pipeline_impl->bind_point);

    // These are all declared dynamic state on every pipeline (see
    // CreatePipelineState) — the static values baked into VkPipeline
    // creation are ignored, so they must be (re)set here from the bound
    // pipeline's own cached PipelineStateDescriptor settings.
    vkCmdSetPrimitiveTopology(vk_cmd, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    vkCmdSetCullMode(vk_cmd, vulkan::ToVkCullMode(pipeline_impl->cull_mode));
    vkCmdSetFrontFace(vk_cmd, pipeline_impl->front_counter_clockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE);
    vkCmdSetDepthTestEnable(vk_cmd, pipeline_impl->depth_test_enable);
    vkCmdSetDepthWriteEnable(vk_cmd, pipeline_impl->depth_write_enable);
    vkCmdSetDepthCompareOp(vk_cmd, vulkan::ConvertComparisonFunc(pipeline_impl->depth_compare_op));
    vkCmdSetDepthBias(vk_cmd, 0.0f, 0.0f, 0.0f);
    vkCmdSetStencilTestEnable(vk_cmd, VK_FALSE);
    vkCmdSetStencilOp(vk_cmd, VK_STENCIL_FACE_FRONT_AND_BACK,
        VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_COMPARE_OP_ALWAYS);
}

void rhi::SetPushConstants(CommandBuffer cmd, const void* data, u32 size)
{
    PHX_ASSERT(cmd.IsValid());
    vkCmdPushConstants(vulkan::ToVkCommandBuffer(cmd), g_context.descriptor_system.pipeline_layout,
        VK_SHADER_STAGE_ALL, 0, size, data);
}

void rhi::Draw(CommandBuffer cmd, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance)
{
    PHX_ASSERT(cmd.IsValid());
    vkCmdDraw(vulkan::ToVkCommandBuffer(cmd), vertex_count, instance_count, first_vertex, first_instance);
}

namespace
{
    VkPipelineStageFlags2 BarrierStageToVk(BarrierStage stage)
    {
        if (stage == BarrierStage::All || stage == BarrierStage::None)
            return VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        VkPipelineStageFlags2 flags = 0;
        if (EnumHasAnyFlags(stage, BarrierStage::Graphics)) flags |= VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
        if (EnumHasAnyFlags(stage, BarrierStage::Compute))  flags |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
        if (EnumHasAnyFlags(stage, BarrierStage::Transfer)) flags |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
        return flags;
    }
}

// Coarse GPU synchronization point — no resource, no layout. With images
// fixed at GENERAL for their whole life (unifiedImageLayouts), the only
// thing left to get right at a barrier is "did the work I depend on finish"
// — no per-resource before/after state. `src`/`dst` narrow which kind of
// GPU work is actually involved so this doesn't stall domains that were
// never touching the data (see the "no graphics API" school of thought).
void rhi::Barrier(CommandBuffer cmd, BarrierStage src, BarrierStage dst)
{
    PHX_ASSERT(cmd.IsValid());

    VkMemoryBarrier2 barrier = {
        .sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
        .srcStageMask  = BarrierStageToVk(src),
        .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
        .dstStageMask  = BarrierStageToVk(dst),
        .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
    };

    VkDependencyInfo dep_info = {
        .sType              = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .memoryBarrierCount = 1,
        .pMemoryBarriers    = &barrier,
    };

    vkCmdPipelineBarrier2(vulkan::ToVkCommandBuffer(cmd), &dep_info);
}
