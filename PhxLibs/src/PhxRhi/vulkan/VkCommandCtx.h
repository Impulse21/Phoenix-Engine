#pragma once

#include "D3D12Base.h"
#include "D3D12Core.h"
#include "D3D12Types.h"
#include "D3D12GpuTempMemory.h"

#include "PhxCore/Span.h"
#include "PhxCore/EnumUtils.h"
#include "PhxRhi/RHITypes.h"

#include <D3D12Utils.h>

namespace phx::rhi::vk
{
	class D3D12CommandCtx
	{
	public:
		void Reset(rhi::CommandQueueType queueType);

		inline void ClearTexture(D3D12_CPU_DESCRIPTOR_HANDLE view, phx::rhi::Color const& clearColour) {
		}

		inline void SetViewports(phx::Span<Viewport> viewports)
		{
		}

		inline void SetScissors(phx::Span<Rect> scissors) 
		{
		
		}
		inline void SetPipelineState(PipelineStateHandle handle) 
		{
		}

		inline void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0)
		{
		}

		inline void Draw(uint32_t , uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0) 
		{
		}

		inline void SetDynamicVertexBuffer(uint32_t , size_t , size_t , const void* ) 
		{
		}

		inline void SetDynamicIndexBuffer(size_t , Format , const void* ) 
		{
		}

		inline void SetPushConstant(uint32_t , size_t , const void* ) 
		{
		}

	public:
		void EnqueueSubmit();

	public:
	private:
	};

}
