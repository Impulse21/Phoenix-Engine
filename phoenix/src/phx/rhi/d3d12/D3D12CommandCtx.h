#pragma once

#include "D3D12Base.h"
#include <phx/core/Span.h>
#include "phx/core/EnumUtils.h"
#include "phx/rhi/RHITypes.h"

namespace phx::rhi::d3d12
{
	class D3D12CommandCtx
	{
	public:
		void Reset(rhi::CommandQueueType queueType);
		void EnqueueSubmit();

		void RenderPassBegin() {}
		void RenderPassEnd() {}
		void SetViewports(phx::Span<Viewport> viewports) {}
		void SetScissors(phx::Span<Rect> scissors) {}
		void SetPipelineState(PipelineStateHandle handle) {}
		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0) {}
		void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0) {}
		void SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData) {}
		void SetDynamicIndexBuffer(size_t numIndicies, Format indexFormat, const void* indexBufferData) {}
		void SetPushConstant(uint32_t rootParameterIndex, size_t sizeInBytes, const void* constants) {}


	private:
		CommandQueueType m_queueType;
		EnumArray<Microsoft::WRL::ComPtr<ID3D12CommandList>, CommandQueueType> m_commandLists;
		ID3D12CommandAllocator* m_allocator;
	};

}
