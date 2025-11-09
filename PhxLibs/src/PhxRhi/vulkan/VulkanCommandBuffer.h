#pragma once

#include <PhxRhi/ICommandBuffer.h>

#include <volk.h>

namespace phx::rhi
{
	struct VulkanCommandBuffer : public ICommandBuffer
	{
		VkCommandBuffer vk_handle;
		uint32_t thread_id;
		CommandQueueType queue_type;

		// -- Interface Implementation ---
		void BindPipelineState(PipelineStateHandle pso) override;
		void Draw(uint32_t vertex_count, uint32_t start_vertex_location) override;
		void DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location) override;
		void DrawInstanced(uint32_t vertex_count, uint32_t instance_count, uint32_t start_vertex_location, uint32_t start_instance_location) override;
		void DrawIndexedInstanced(uint32_t index_count, uint32_t instance_count, uint32_t start_index_location, int32_t base_vertex_location, uint32_t startInstanceLocation) override;

		VulkanCommandBuffer(VkCommandBuffer vk_handle, CommandQueueType type, uint32_t thread_id);
		~VulkanCommandBuffer() override;

	};
}