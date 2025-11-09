#include "PhxRhi/PhxRhi_pch.h"
#include "VulkanCommandBuffer.h"


void BindPipelineState(CommandBufferHandle handle, PipelineStateHandle pso)
{
}

void Draw(CommandBufferHandle handle, uint32_t vertex_count, uint32_t start_vertex_location)
{
    VkCommandBuffer& vk_cmd_buffer = VkContext::GetCurrentFrame().vk_command_buffers[handle.GetIndex()];
    vkCmdDraw(
        vk_cmd_buffer,
        vertex_count,
        1,
        start_vertex_location,
        0);
}

void DrawIndexed(CommandBufferHandle handle, uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location)
{
    VkCommandBuffer& vk_cmd_buffer = VkContext::GetCurrentFrame().vk_command_buffers[handle.GetIndex()];
    vkCmdDrawIndexed(
        vk_cmd_buffer,
        index_count,
        1,
        start_index_location,
        base_vertex_location,
        0);
}

void DrawInstanced(CommandBufferHandle handle, uint32_t vertex_count, uint32_t instance_count, uint32_t start_vertex_location, uint32_t start_instance_location)
{
    VkCommandBuffer& vk_cmd_buffer = VkContext::GetCurrentFrame().vk_command_buffers[handle.GetIndex()];
    vkCmdDraw(
        vk_cmd_buffer,
        vertex_count,
        instance_count,
        start_vertex_location,
        start_instance_location);
}

void DrawIndexedInstanced(CommandBufferHandle handle, uint32_t index_count, uint32_t instance_count, uint32_t start_index_location, int32_t base_vertex_location, uint32_t start_instance_location)
{
    VkCommandBuffer& vk_cmd_buffer = VkContext::GetCurrentFrame().vk_command_buffers[handle.GetIndex()];
    vkCmdDrawIndexed(
        vk_cmd_buffer,
        index_count,
        instance_count,
        start_index_location,
        base_vertex_location,
        start_instance_location);
}

void phx::rhi::VulkanCommandBuffer::BindPipelineState(PipelineStateHandle pso)
{
}

void phx::rhi::VulkanCommandBuffer::Draw(uint32_t vertex_count, uint32_t start_vertex_location)
{
}

void phx::rhi::VulkanCommandBuffer::DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location)
{
}

void phx::rhi::VulkanCommandBuffer::DrawInstanced(uint32_t vertex_count, uint32_t instance_count, uint32_t start_vertex_location, uint32_t start_instance_location)
{
}

void phx::rhi::VulkanCommandBuffer::DrawIndexedInstanced(uint32_t index_count, uint32_t instance_count, uint32_t start_index_location, int32_t base_vertex_location, uint32_t startInstanceLocation)
{
}
