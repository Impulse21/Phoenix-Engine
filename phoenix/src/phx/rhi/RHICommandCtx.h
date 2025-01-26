#pragma once

#include <phx/rhi/RHITypes.h>

namespace phx::rhi
{
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

		void SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData)
		{
		}

		void SetDynamicIndexBuffer(size_t numIndicies, Format indexFormat, const void* indexBufferData)
		{
		}

		void SetPushConstant(uint32_t rootParameterIndex, size_t sizeInBytes, const void* constants)
		{
		}

		template<typename T>
		void SetPushConstant(uint32_t rootParameterIndex, T const& constants)
		{
			SetPushConstant(rootParameterIndex, sizeof(T), &constants);
		}


	private:

	};
}