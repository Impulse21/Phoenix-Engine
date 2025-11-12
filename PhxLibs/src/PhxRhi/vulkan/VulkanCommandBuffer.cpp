#include "PhxRhi/PhxRhi_pch.h"
#include "VulkanCommandBuffer.h"

#include "VulkanResourceManager.h"

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
    

    VulkanBuffer* src_buffer_impl = rsc_manager->buffer_pool.GetHot(src_buffer);
    VulkanBuffer* dest_buffer_impl = rsc_manager->buffer_pool.GetHot(dest_buffer);

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
    , rsc_manager(rsc_manager)
{

}

phx::rhi::VulkanCommandBuffer::~VulkanCommandBuffer() = default;