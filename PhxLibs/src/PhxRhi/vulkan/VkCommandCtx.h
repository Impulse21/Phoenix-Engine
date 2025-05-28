#pragma once

#include "PhxCore/Span.h"
#include "PhxCore/EnumUtils.h"
#include "PhxRhi/RHITypes.h"


#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif


namespace phx::rhi::vk
{
	struct VkCommandCtx
	{
	public:
		void Reset(rhi::CommandQueueType queueType);

		void RenderPassBegin()
		{
		}

		void RenderPassEnd()
		{
		}

		inline void ClearTexture(phx::rhi::Color const& /*clearColour*/) {
		}

		inline void SetViewports(phx::Span<Viewport> /*viewports*/)
		{
		}

		inline void SetScissors(phx::Span<Rect> /*scissors*/) 
		{
		
		}
		inline void SetPipelineState(PipelineStateHandle /*handle*/) 
		{
		}

		inline void DrawIndexed(uint32_t /*indexCount*/, uint32_t /*instanceCount*/ = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0)
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
