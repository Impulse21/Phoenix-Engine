#include "PhxRhi/PhxRhi_pch.h"
#include "VulkanCommandBuffer.h"

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

phx::rhi::VulkanCommandBuffer::VulkanCommandBuffer(VkCommandBuffer vk_handle, CommandQueueType type, uint32_t thread_id)
    : vk_handle(vk_handle)
    , queue_type(type)
    , thread_id(thread_id)
{

}

phx::rhi::VulkanCommandBuffer::~VulkanCommandBuffer() = default;