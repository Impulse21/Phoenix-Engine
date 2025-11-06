#pragma once

#include "RHICommon.h"

namespace phx::rhi
{
	class ICommandBuffer
	{
	public:
		virtual ~ICommandBuffer() = default;

        virtual void BindPipelineState(PipelineStateHandle pso) = 0;

        virtual void Draw(uint32_t vertex_count, uint32_t start_vertex_location) = 0;
		virtual void DrawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location) = 0;
		virtual void DrawInstanced(uint32_t vertex_count, uint32_t instance_count, uint32_t start_vertex_location, uint32_t start_instance_location) = 0;
		virtual void DrawIndexedInstanced(uint32_t index_count, uint32_t instance_count, uint32_t start_index_location, int32_t base_vertex_location, uint32_t startInstanceLocation) = 0;
	};
}