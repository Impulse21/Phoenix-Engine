#pragma once

#include <PhxRhi/ICommandBuffer.h>

namespace phx::rhi
{
	struct VulkanCommandBuffer : public ICommandBuffer
	{
		VulkanCommandBuffer();
		~VulkanCommandBuffer() override;

		// Inherited via ICommandBuffer
		void BindPipelineState(PipelineStateHandle pso) override;
		void Draw(uint32_t vertex_count, uint32_t start_vertex_location) override;
		void DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location) override;
		void DrawInstanced(uint32_t vertex_count, uint32_t instance_count, uint32_t start_vertex_location, uint32_t start_instance_location) override;
		void DrawIndexedInstanced(uint32_t index_count, uint32_t instance_count, uint32_t start_index_location, int32_t base_vertex_location, uint32_t startInstanceLocation) override;
	};
}