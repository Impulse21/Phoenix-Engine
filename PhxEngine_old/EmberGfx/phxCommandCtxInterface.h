#pragma once

#include "EmberGfx/phxGfxDeviceResources.h"

namespace phx::gfx
{
	class ICommandCtx
	{
	public:
		virtual ~ICommandCtx() = default;

		virtual void RenderPassBegin() = 0;
		virtual void RenderPassEnd() = 0;

		virtual void SetPipelineState(PipelineStateHandle pipelineState) = 0;
		virtual void SetViewports(Span<Viewport> viewports) = 0;
		virtual void SetScissors(Span<Rect> rects) = 0;

		virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0) = 0;
		virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0 , uint32_t startInstance = 0) = 0;

		virtual void SetDynamicVertexBuffer(BufferHandle tempBuffer, size_t offset, uint32_t slot, size_t numVertices, size_t vertexSize) = 0;
		virtual void SetDynamicIndexBuffer(BufferHandle tempBuffer, size_t offset, size_t numIndicies, Format indexFormat) = 0;

		virtual void SetPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants) = 0;

		template<typename T>
		void SetPushConstant(uint32_t rootParameterIndex, T const& constants)
		{
			SetPushConstant(rootParameterIndex, sizeof(T), &constants);
		}
	};
}