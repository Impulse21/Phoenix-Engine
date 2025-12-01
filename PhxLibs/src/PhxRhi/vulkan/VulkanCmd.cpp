#include "PhxRhi/PhxRhi_pch.h"
#include <PhxRhi/PhxRhi.h>

#include <PhxRhi/PhxRhi_Thread.h>

#include "VulkanInternal.h"

using namespace phx;
using namespace phx::rhi;

CmdHandle phx::rhi::BeginCommandBuffer(CommandQueueType queue_type)
{
    // Resolve thread
    const uint32_t thread_id = g_rhi_thread_index;
    vulkan::PerThreadData& thread_data = g_vulkan.submission.per_thread_data[thread_id];

    vulkan::CommandPool& pool = thread_data.command_pools[queue_type];
    const uint32_t index = pool.GetFreeBufferIndex();
    VkCommandBuffer vk_cmd_buffer = pool.cmd_buffer_pool[index];

    VkCommandBufferBeginInfo being_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(vk_cmd_buffer, &being_info);

    return EncodeCmdHandle(thread_id, queue_type, index);
}

void phx::rhi::PushConstants(CmdHandle cmd, const void* data, uint32_t size, uint32_t offset)
{
    VkCommandBuffer vk_cmd_buffer = ResolveCmdBuffer(cmd);
    vkCmdPushConstants(
        vk_cmd_buffer,
        g_vulkan.descriptor_system.pipeline_layout,
        VK_SHADER_STAGE_ALL,
        offset,
        size,
        data
    );
}

void phx::rhi::BindPipelineState(rhi::CmdHandle handle, PipelineStateHandle pso)
{
    const VulkanPipelineState& vulkan_pso = *g_vulkan.pipeline_state_pool.GetHot(pso);
    VkCommandBuffer vk_cmd_buffer = ResolveCmdBuffer(handle);

    vkCmdBindPipeline(
        vk_cmd_buffer,
        vulkan_pso.bind_point,
        vulkan_pso.vk_pipeline);
}


void phx::rhi::BindIndexBuffer(CmdHandle handle, BufferHandle index_buffer, uint64_t offset, IndexFormat format)
{
    const VulkanBuffer& vulkan_buffer = *g_vulkan.buffer_pool.GetHot(index_buffer);
    VkCommandBuffer vk_cmd_buffer = ResolveCmdBuffer(handle);
    VkIndexType vk_type = (format == IndexFormat::Uint16)
        ? VK_INDEX_TYPE_UINT16
        : VK_INDEX_TYPE_UINT32;

    vkCmdBindIndexBuffer(
        vk_cmd_buffer,
        vulkan_buffer.vk_buffer,
        offset,
        vk_type);
}


void phx::rhi::SetViewport(CmdHandle cmd, rhi::Viewport const& viewport)
{
    VkCommandBuffer vk_cmd_buffer = ResolveCmdBuffer(cmd);
    VkViewport vk_vp;
    vk_vp.x = viewport.MinX;
    vk_vp.width = viewport.GetWidth();

    // Y FLIP TRICK:
    // Vulkan Y points down in Clip Space. DX12 points up.
    // To use DX12 projection matrices in Vulkan without changing shaders, 
    // we specify a NEGATIVE height and start Y at the bottom.

    // DX12: Top=0, Height=H
    // Vulkan (Flipped): Y = Height, Height = -H
    vk_vp.y = viewport.MaxY;
    vk_vp.height = -viewport.GetHeight();

    // Depth (0..1 is standard for both)
    vk_vp.minDepth = viewport.MinZ;
    vk_vp.maxDepth = viewport.MaxZ;

    vkCmdSetViewportWithCount(vk_cmd_buffer, 1, &vk_vp);
}

void phx::rhi::SetScissor(CmdHandle cmd, rhi::Rect const& rect)
{
    VkCommandBuffer vk_cmd_buffer = ResolveCmdBuffer(cmd);

    VkRect2D scissor;
    scissor.offset = { 
        .x = rect.MinX, 
        .y = rect.MinY
    };

    scissor.extent = { 
        .width = (uint32_t)rect.GetWidth(),
        .height = (uint32_t)rect.GetHeight() 
    };

    vkCmdSetScissorWithCount(vk_cmd_buffer, 1, &scissor);
}


void phx::rhi::SetPrimitiveTopology(CmdHandle cmd, rhi::PrimitiveType prim_type)
{
    VkCommandBuffer vk_cmd_buffer = ResolveCmdBuffer(cmd);
    vkCmdSetPrimitiveTopology(vk_cmd_buffer, ToVkPrimtivieTopology(prim_type));
}

void phx::rhi::SetCullMode(CmdHandle cmd, rhi::RasterCullMode cull_mode)
{
    VkCommandBuffer vk_cmd_buffer = ResolveCmdBuffer(cmd);
    vkCmdSetCullMode(vk_cmd_buffer, ToVkCullMode(cull_mode));
}

void phx::rhi::SetFrontFace(CmdHandle cmd, rhi::FrontFace front_face) 
{
    VkCommandBuffer vk_cmd_buffer = ResolveCmdBuffer(cmd);
    vkCmdSetFrontFace(vk_cmd_buffer, ToVkFrontFace(front_face));
}

void phx::rhi::SetDepthTest(CmdHandle cmd, bool test_enable, bool write_enable, rhi::ComparisonFunc op)
{
    VkCommandBuffer vk_cmd_buffer = ResolveCmdBuffer(cmd);

    vkCmdSetDepthTestEnable(vk_cmd_buffer, test_enable ? VK_TRUE : VK_FALSE);
    vkCmdSetDepthWriteEnable(vk_cmd_buffer, write_enable ? VK_TRUE : VK_FALSE);
    vkCmdSetDepthCompareOp(vk_cmd_buffer, ConvertComparisonFunc(op));
}

void phx::rhi::SetDepthBias(CmdHandle cmd, float constant_factor, float clamp, float slope_factor)
{
    VkCommandBuffer vk_cmd_buffer = ResolveCmdBuffer(cmd);
    vkCmdSetDepthBias(vk_cmd_buffer, constant_factor, clamp, slope_factor);
}

void phx::rhi::SetStencilTest(CmdHandle cmd, bool enable)
{
    VkCommandBuffer vk_cmd_buffer = ResolveCmdBuffer(cmd);
    vkCmdSetStencilTestEnable(vk_cmd_buffer, enable ? VK_TRUE : VK_FALSE);
}

void phx::rhi::SetStencilOp(CmdHandle cmd, StencilOp fail, StencilOp pass, StencilOp depth_fail, rhi::ComparisonFunc op)
{
    VkCommandBuffer vk_cmd_buffer = ResolveCmdBuffer(cmd);

    // Note: This sets both Front and Back faces to the same op.
    // If you need separate front/back, you need to expose 'VkStencilFaceFlags' in your API.
    vkCmdSetStencilOp(
        vk_cmd_buffer,
        VK_STENCIL_FACE_FRONT_AND_BACK,
        static_cast<VkStencilOp>(fail),
        static_cast<VkStencilOp>(pass),
        static_cast<VkStencilOp>(depth_fail),
        ConvertComparisonFunc(op));
}


void phx::rhi::Draw(rhi::CmdHandle handle, uint32_t vertex_count, uint32_t start_vertex_location)
{
    vkCmdDraw(
        ResolveCmdBuffer(handle),
        vertex_count,
        1,
        start_vertex_location,
        0);
}

void phx::rhi::DrawIndexed(rhi::CmdHandle handle, uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location)
{
    vkCmdDrawIndexed(
        ResolveCmdBuffer(handle),
        index_count,
        1,
        start_index_location,
        base_vertex_location,
        0);
}

void phx::rhi::DrawInstanced(rhi::CmdHandle handle, uint32_t vertex_count, uint32_t instance_count, uint32_t start_vertex_location, uint32_t start_instance_location)
{
    vkCmdDraw(
        ResolveCmdBuffer(handle),
        vertex_count,
        instance_count,
        start_vertex_location,
        start_instance_location);
}

void phx::rhi::DrawIndexedInstanced(rhi::CmdHandle handle, uint32_t index_count, uint32_t instance_count, uint32_t start_index_location, int32_t base_vertex_location, uint32_t start_instance_location)
{
    vkCmdDrawIndexed(
        ResolveCmdBuffer(handle),
        index_count,
        instance_count,
        start_index_location,
        base_vertex_location,
        start_instance_location);
}

void phx::rhi::BeginRendering(
    rhi::CmdHandle handle,
    SwapchainHandle swapchain_handle,
    rhi::ClearValue const& clear_colour,
    TextureHandle depth_texture,
    const ClearValue& depth_clear_value)
{
    InsertSwapchainBarrier(handle, swapchain_handle, ResourceStates::RenderTarget);

    VulkanSwapchainFrame* swapchain = g_vulkan.swapchain_pool.GetHot(swapchain_handle);
    VkClearValue vk_clear_value = {
        .color = {
            .float32 = { clear_colour.Colour.R, clear_colour.Colour.G, clear_colour.Colour.B, clear_colour.Colour.A }
        }
    };

    VkRenderingAttachmentInfo color_attachment_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = swapchain->vk_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = vk_clear_value // Use this clear value
    };

    const bool has_depth = depth_texture.IsValid();
    VkRenderingAttachmentInfo depth_attachment_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    };

    if (has_depth)
    {
        VkClearValue vk_clear_depth_value = {
            .depthStencil = {
                .depth = depth_clear_value.DepthStencil.Depth,
                .stencil = depth_clear_value.DepthStencil.Stencil,
            }
        };

        VulkanTexture* vulkan_texture = g_vulkan.texture_pool.GetHot(depth_texture);
        depth_attachment_info.imageView = vulkan_texture->vk_view_dsv;
        depth_attachment_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depth_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment_info.clearValue = vk_clear_depth_value;
    }

    VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {
            .offset = {.x = 0u, .y = 0u},
            .extent = swapchain->vk_swapchain_extent,
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_info,
        .pDepthAttachment = has_depth ? &depth_attachment_info : nullptr,
        .pStencilAttachment = nullptr,
    };
    vkCmdBeginRendering(ResolveCmdBuffer(handle), &rendering_info);
}

void phx::rhi::EndRendering(rhi::CmdHandle handle)
{
    vkCmdEndRendering(ResolveCmdBuffer(handle));
}

void phx::rhi::InsertSwapchainBarrier(rhi::CmdHandle handle, SwapchainHandle swapchain_handle, rhi::ResourceStates resource_state)
{
    VulkanSwapchainFrame* swapchain_impl = g_vulkan.swapchain_pool.GetHot(swapchain_handle);

    VkImageLayout old_layout = ResourceStateToImageLayout(swapchain_impl->resource_state);
    VkImageLayout new_layout = ResourceStateToImageLayout(resource_state);

    VkPipelineStageFlags src_stage = ResourceStateToPipelineStage(swapchain_impl->resource_state);
    VkPipelineStageFlags dest_stage = ResourceStateToPipelineStage(resource_state);

    VkImageMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = src_stage,
        .srcAccessMask = ResourceStateToAccessFlags2(swapchain_impl->resource_state),
        .dstStageMask = dest_stage,
        .dstAccessMask = ResourceStateToAccessFlags2(resource_state),
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapchain_impl->vk_image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VkDependencyInfo dependency_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };

    vkCmdPipelineBarrier2(ResolveCmdBuffer(handle), &dependency_info);

    swapchain_impl->resource_state = resource_state;
}

void phx::rhi::InsertBarriers(rhi::CmdHandle handle, Span<GpuBarrier> barriers)
{
    if (barriers.IsEmpty())
        return;

    constexpr size_t MAX_BARRIER_COUNT = 16;
    std::array<VkMemoryBarrier2, MAX_BARRIER_COUNT> vk_mem_barriers;
    std::array<VkBufferMemoryBarrier2, MAX_BARRIER_COUNT> vk_buffer_barriers;
    std::array<VkImageMemoryBarrier2, MAX_BARRIER_COUNT> vk_texture_barriers;

    uint32_t mem_barrier_count = 0;
    uint32_t buffer_barrier_count = 0;
    uint32_t texture_barrier_count = 0;

    VkPipelineStageFlags all_src_stage_mask = 0;
    VkPipelineStageFlags all_dst_stage_mask = 0;
    for (auto& barrier : barriers)
    {
        std::visit(
            [&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, rhi::GpuBarrier::GlobalBarrier>)
                {
                    if (mem_barrier_count == MAX_BARRIER_COUNT)
                        return;

                    // --- Global Barrier ---
                    VkPipelineStageFlags src_stage = ResourceStateToPipelineStage(arg.before_state);
                    VkPipelineStageFlags dest_stage = ResourceStateToPipelineStage(arg.after_state);
                    all_src_stage_mask |= src_stage;
                    all_dst_stage_mask |= dest_stage;

                    vk_mem_barriers[mem_barrier_count++] = {
                        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
                        .pNext = nullptr,
                        .srcStageMask = src_stage,
                        .srcAccessMask = ResourceStateToAccessFlags2(arg.before_state),
                        .dstStageMask = dest_stage,
                        .dstAccessMask = ResourceStateToAccessFlags2(arg.after_state)
                    };
                }
                else if constexpr (std::is_same_v<T, GpuBarrier::BufferBarrier>)
                {
                    if (buffer_barrier_count== MAX_BARRIER_COUNT)
                        return;

                    VkPipelineStageFlags src_stage = ResourceStateToPipelineStage(arg.before_state);
                    VkPipelineStageFlags dest_stage = ResourceStateToPipelineStage(arg.after_state);

                    uint64_t mask = 0;
                    const VkPhysicalDeviceFeatures& device_features = g_vulkan.vk_physical_device_features;
                    if (!device_features.tessellationShader)
                        mask |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;

                    if (!device_features.geometryShader)
                        mask |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;

                    if (!EnumHasAnyFlags(g_vulkan.capabilities, DeviceCapability::RayTracing))
                        mask |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;

                    dest_stage &= ~mask;

                    all_src_stage_mask |= src_stage;
                    all_dst_stage_mask |= dest_stage;

                    VulkanBuffer* vulkan_buffer = g_vulkan.buffer_pool.GetHot(arg.buffer);
                    VkBufferMemoryBarrier2& buffer_barrier = vk_buffer_barriers[buffer_barrier_count++];
                    buffer_barrier = {
                        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                        .pNext = nullptr,
                        .srcStageMask = src_stage,
                        .srcAccessMask = ResourceStateToAccessFlags2(arg.before_state),
                        .dstStageMask = dest_stage,
                        .dstAccessMask = ResourceStateToAccessFlags2(arg.after_state),
                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .buffer = vulkan_buffer->vk_buffer,
                        .offset = arg.offset,
                        .size = (arg.size == ~0u) ? VK_WHOLE_SIZE : arg.size
                    };
                }
                else if constexpr (std::is_same_v<T, GpuBarrier::TextureBarrier>)
                {
#if false
                    if (texture_barrier_count == MAX_BARRIER_COUNT)
                        return;

                    // --- Texture Barrier ---
                    VkPipelineStageFlags src_stage = ConvertPipelineStages(arg.before_state);
                    VkPipelineStageFlags dest_stage = ConvertPipelineStages(arg.after_state);
                    all_src_stage_mask |= src_stage;
                    all_dst_stage_mask |= dest_stage;

                    vk_texture_barriers[texture_barrier_count++] = {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                        .pNext = nullptr,
                        .srcStageMask = src_stage,
                        .srcAccessMask = ResourceStateToAccessFlags2(arg.before_state),
                        .dstStageMask = dest_stage,
                        .dstAccessMask = ResourceStateToAccessFlags2(arg.after_state),
                        .oldLayout = ConvertImageLayout(arg.before_state),
                        .newLayout = ConvertImageLayout(arg.after_state),
                        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                        .image = arg.texture,
                        .subresourceRange = {
                            .aspectMask = TranslateImageAspects(arg.Aspects),
                            .baseMipLevel = static_cast<uint32_t>(arg.FirstMip),
                            .levelCount = (arg.mip == -1) ? VK_REMAINING_MIP_LEVELS : static_cast<uint32_t>(arg.mip),
                            .baseArrayLayer = static_cast<uint32_t>(arg.FirstSlice),
                            .layerCount = (arg.slice == -1) ? VK_REMAINING_ARRAY_LAYERS : static_cast<uint32_t>(arg.slice)
                        }
                     });
#endif
                }
            },
            barrier.Data
        );
    }

    if (all_src_stage_mask == 0) 
        all_src_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    if (all_dst_stage_mask == 0) 
        all_dst_stage_mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    // --- Select the correct data pointer (stack or heap) ---
    const VkMemoryBarrier2* memory_barriers_ptr = vk_mem_barriers.data();
    const VkBufferMemoryBarrier2* buffer_barriers_ptr = vk_buffer_barriers.data();
    const VkImageMemoryBarrier2* image_barriers_ptr = vk_texture_barriers.data();

    VkDependencyInfo dependency_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = 0, // or VK_DEPENDENCY_BY_REGION_BIT
        .memoryBarrierCount = static_cast<uint32_t>(mem_barrier_count),
        .pMemoryBarriers = memory_barriers_ptr,
        .bufferMemoryBarrierCount = static_cast<uint32_t>(buffer_barrier_count),
        .pBufferMemoryBarriers = buffer_barriers_ptr,
        .imageMemoryBarrierCount = static_cast<uint32_t>(texture_barrier_count),
        .pImageMemoryBarriers = image_barriers_ptr
    };

    vkCmdPipelineBarrier2(ResolveCmdBuffer(handle), &dependency_info);
}

void phx::rhi::CopyBuffer(
    rhi::CmdHandle handle,
    BufferHandle src_buffer,
    uint64_t src_offset,
    BufferHandle dest_buffer,
    uint64_t dest_offset,
    size_t size)
{
    VkBufferCopy2 bufferCopyRegion = { 
        .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
        .pNext = nullptr,
        .srcOffset = src_offset,
        .dstOffset = dest_offset,
        .size = size,
    };
    

    VulkanBuffer* src_buffer_impl = g_vulkan.buffer_pool.GetHot(src_buffer);
    VulkanBuffer* dest_buffer_impl = g_vulkan.buffer_pool.GetHot(dest_buffer);

    // Define the overall copy operation
    VkCopyBufferInfo2 copyBufferInfo = { 
        .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
        .pNext = nullptr,
        .srcBuffer = src_buffer_impl->vk_buffer,
        .dstBuffer = dest_buffer_impl->vk_buffer,
        .regionCount = 1,
        .pRegions = &bufferCopyRegion,
    };
    

    // Record the command into the command buffer
    vkCmdCopyBuffer2(ResolveCmdBuffer(handle), &copyBufferInfo);
}

