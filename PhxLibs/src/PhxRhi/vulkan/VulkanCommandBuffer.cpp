#include "PhxRhi/PhxRhi_pch.h"
#include "VulkanCommandBuffer.h"

#include "VulkanBackend.h"
#include "VulkanResourceManager.h"

void phx::rhi::VulkanCommandBuffer::Begin()
{
    VkCommandBufferBeginInfo being_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(vk_handle, &being_info);
}

void phx::rhi::VulkanCommandBuffer::End()
{
    vkEndCommandBuffer(vk_handle);
}

void phx::rhi::VulkanCommandBuffer::BindPipelineState(PipelineStateHandle /*pso*/)
{
}

void phx::rhi::VulkanCommandBuffer::Draw(uint32_t vertex_count, uint32_t start_vertex_location)
{
    vkCmdDraw(
        vk_handle,
        vertex_count,
        1,
        start_vertex_location,
        0);
}

void phx::rhi::VulkanCommandBuffer::DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location)
{
    vkCmdDrawIndexed(
        vk_handle,
        index_count,
        1,
        start_index_location,
        base_vertex_location,
        0);
}

void phx::rhi::VulkanCommandBuffer::DrawInstanced(uint32_t vertex_count, uint32_t instance_count, uint32_t start_vertex_location, uint32_t start_instance_location)
{
    vkCmdDraw(
        vk_handle,
        vertex_count,
        instance_count,
        start_vertex_location,
        start_instance_location);
}

void phx::rhi::VulkanCommandBuffer::DrawIndexedInstanced(uint32_t index_count, uint32_t instance_count, uint32_t start_index_location, int32_t base_vertex_location, uint32_t start_instance_location)
{
    vkCmdDrawIndexed(
        vk_handle,
        index_count,
        instance_count,
        start_index_location,
        base_vertex_location,
        start_instance_location);
}

void phx::rhi::VulkanCommandBuffer::BeginRendering(SwapchainHandle handle, rhi::ClearValue const& clear_value)
{
    InsertSwapchainBarrier(handle, ResourceStates::RenderTarget);

    VulkanSwapchainFrame* swapchain = vulkan_rm->swapchain_pool.GetHot(handle);
    VkClearValue vk_clear_value = {
        .color = {
            .float32 = { clear_value.Colour.R, clear_value.Colour.G, clear_value.Colour.B, clear_value.Colour.A }
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

    VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {
            .offset = {.x = 0u, .y = 0u},
            .extent = swapchain->vk_swapchain_extent,
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_info,
        .pDepthAttachment = nullptr,
        .pStencilAttachment = nullptr,
    };

    vkCmdBeginRendering(vk_handle, &rendering_info);
}

void phx::rhi::VulkanCommandBuffer::EndRendering()
{
    vkCmdEndRendering(vk_handle);
}

void phx::rhi::VulkanCommandBuffer::InsertSwapchainBarrier(SwapchainHandle handle, rhi::ResourceStates resource_state)
{
    VulkanSwapchainFrame* swapchain_handle = vulkan_rm->swapchain_pool.GetHot(handle);

    VkImageLayout old_layout = ResourceStateToImageLayout(swapchain_handle->resource_state);
    VkImageLayout new_layout = ResourceStateToImageLayout(resource_state);

    VkPipelineStageFlags src_stage = ResourceStateToPipelineStage(swapchain_handle->resource_state);
    VkPipelineStageFlags dest_stage = ResourceStateToPipelineStage(resource_state);

    VkImageMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = src_stage,
        .srcAccessMask = ResourceStateToAccessFlags2(swapchain_handle->resource_state),
        .dstStageMask = dest_stage,
        .dstAccessMask = ResourceStateToAccessFlags2(resource_state),
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapchain_handle->vk_image,
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

    vkCmdPipelineBarrier2(vk_handle, &dependency_info);

    swapchain_handle->resource_state = resource_state;
}

void phx::rhi::VulkanCommandBuffer::InsertBarriers(Span<GpuBarrier> barriers)
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


                    const DeviceCapability& capabilities = vulkan_rm->vulkan_backend->capabilities;

                    VkPipelineStageFlags dest_stage_mask = 0u;
                    if (this->queue_type == CommandQueueType::Copy)
                    {
                        dest_stage_mask |=
                            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT      |
                            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT     |
                            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT    |
                            VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
                            VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
                            VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
                            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
                    }

                    if (!EnumHasAnyFlags(capabilities, DeviceCapability::RayTracing))
                    {
                        dest_stage_mask |= VK_ACCESS_2_SHADER_STORAGE_READ_BIT_KHR;
                    }

                    dest_stage &= ~dest_stage_mask;

                    all_src_stage_mask |= src_stage;
                    all_dst_stage_mask |= dest_stage;

                    VulkanBuffer* vulkan_buffer = vulkan_rm->buffer_pool.GetHot(arg.buffer);
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

                    VkAccessFlagBits2 acces_flags = 0u;
                    acces_flags |= VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
                    buffer_barrier.dstAccessMask &= ~(acces_flags);
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

    PHX_ASSERT(vk_handle != VK_NULL_HANDLE, "Unexpected state of buffer handle");
    vkCmdPipelineBarrier2(vk_handle, &dependency_info);
}

void phx::rhi::VulkanCommandBuffer::CopyBuffer(
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
    

    VulkanBuffer* src_buffer_impl = vulkan_rm->buffer_pool.GetHot(src_buffer);
    VulkanBuffer* dest_buffer_impl = vulkan_rm->buffer_pool.GetHot(dest_buffer);

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
    vkCmdCopyBuffer2(vk_handle, &copyBufferInfo);
}

phx::rhi::VulkanCommandBuffer::VulkanCommandBuffer(rhi::VulkanResourceManager* rsc_manager, VkCommandBuffer vk_handle, CommandQueueType type, uint32_t thread_id)
    : vk_handle(vk_handle)
    , queue_type(type)
    , thread_id(thread_id)
    , vulkan_rm(rsc_manager)
{
    PHX_CORE_ASSERT(vk_handle != VK_NULL_HANDLE);
}

phx::rhi::VulkanCommandBuffer::~VulkanCommandBuffer() = default;