#pragma once

#include "RHITypes.h"

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
			UNREFERENCED_PARAMETER(viewports);
		}

		void SetScissors(phx::Span<Rect> scissors)
		{
			UNREFERENCED_PARAMETER(scissors);

		}

		void SetPipelineState(PipelineStateHandle handle)
		{
			UNREFERENCED_PARAMETER(handle);

		}

		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0)
		{
			UNREFERENCED_PARAMETER(indexCount);
			UNREFERENCED_PARAMETER(instanceCount);
			UNREFERENCED_PARAMETER(startIndex);
			UNREFERENCED_PARAMETER(baseVertex);
			UNREFERENCED_PARAMETER(startInstance);

		}

		void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0)
		{
			UNREFERENCED_PARAMETER(vertexCount);
			UNREFERENCED_PARAMETER(instanceCount);
			UNREFERENCED_PARAMETER(startVertex);
			UNREFERENCED_PARAMETER(startInstance);
		}

		void SetDynamicVertexBuffer(GpuBufferHandle tempBuffer, size_t offset, uint32_t slot, size_t numVertices, size_t vertexSize)
		{
			UNREFERENCED_PARAMETER(tempBuffer);
			UNREFERENCED_PARAMETER(offset);
			UNREFERENCED_PARAMETER(slot);
			UNREFERENCED_PARAMETER(numVertices);
			UNREFERENCED_PARAMETER(vertexSize);
		}

		void SetDynamicIndexBuffer(GpuBufferHandle tempBuffer, size_t offset, size_t numIndicies, Format indexFormat)
		{
			UNREFERENCED_PARAMETER(tempBuffer);
			UNREFERENCED_PARAMETER(offset);
			UNREFERENCED_PARAMETER(numIndicies);
			UNREFERENCED_PARAMETER(indexFormat);
		}

		void SetPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants)
		{
			UNREFERENCED_PARAMETER(rootParameterIndex);
			UNREFERENCED_PARAMETER(sizeInBytes);
			UNREFERENCED_PARAMETER(constants);
		}

		template<typename T>
		void SetPushConstant(uint32_t rootParameterIndex, T const& constants)
		{
			SetPushConstant(rootParameterIndex, sizeof(T), &constants);
		}
	};
}