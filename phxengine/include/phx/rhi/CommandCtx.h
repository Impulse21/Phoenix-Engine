#pragma once

#include "RHITypes.h"

namespace phx::rhi
{
#if false
	class CommandCtx
	{
	public:
		void RenderPassBegin()
		{

		}

		void RenderPassEnd()
		{

		}

		void SetViewports(phx::Span<Viewport> viewports)
		{

		}

		void SetScissors(phx::Span<Rect> scissors)
		{

		}

		void SetPipelineState(PipelineStateHandle handle)
		{

		}

		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0)
		{

		}

		void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0)
		{

		}

		void SetDynamicVertexBuffer(GpuBufferHandle tempBuffer, size_t offset, uint32_t slot, size_t numVertices, size_t vertexSize)
		{

		}

		void SetDynamicIndexBuffer(GpuBufferHandle tempBuffer, size_t offset, size_t numIndicies, Format indexFormat)
		{

		}

		void SetPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants)
		{

		}

		template<typename T>
		void SetPushConstant(uint32_t rootParameterIndex, T const& constants)
		{
			SetPushConstant(rootParameterIndex, sizeof(T), &constants);
		}
	};
#endif
}