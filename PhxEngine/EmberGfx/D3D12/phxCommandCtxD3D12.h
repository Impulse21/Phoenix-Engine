#pragma once

#include "EmberGfx/phxCommandCtxInterface.h"

#include "phxMemory.h"
#include "phxDynamicMemoryPageAllocatorD3D12.h"
#include <deque>

namespace phx::gfx
{
	class D3D12GpuDevice;
}

namespace phx::gfx
{
	struct DynamicBuffer
	{
		BufferHandle BufferHandle;
		size_t Offset;
		uint8_t* Data;
	};
}

namespace phx::gfx::platform
{
	struct D3D12Semaphore
	{
		Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
		uint64_t fenceValue = 0;
	};

	class CommandCtxD3D12 final : public phx::gfx::ICommandCtx
	{
		friend D3D12GpuDevice;
	public:
		CommandCtxD3D12() = default;
		~CommandCtxD3D12() = default;

		void RenderPassBegin() override;
		void RenderPassEnd() override;

		void Reset(size_t id, CommandQueueType queueType);
		void Close();

		void SetPipelineState(PipelineStateHandle pipelineState) override;
		void SetViewports(Span<Viewport> viewports) override;
		void SetScissors(Span<Rect> rects) override;

		void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance) override;
		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance) override;

		void SetDynamicVertexBuffer(BufferHandle tempBuffer, size_t offset, uint32_t slot, size_t numVertices, size_t vertexSize) override;
		void SetDynamicIndexBuffer(BufferHandle tempBuffer, size_t offset, size_t numIndicies, Format indexFormat) override;

	public:
		void TransitionBarrier(GpuBarrier const& barrier);
		void TransitionBarriers(Span<GpuBarrier> gpuBarriers);
		void ClearBackBuffer(Color const& clearColour);
		void ClearTextureFloat(TextureHandle texture, Color const& clearColour);
		void ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil);
		void SetGfxPipeline(GfxPipelineHandle pipeline); 
		void SetRenderTargetSwapChain();

		void WriteTexture(TextureHandle texture, uint32_t firstSubresource, size_t numSubresources, SubresourceData* pSubresourceData);

		void SetRenderTargets(Span<TextureHandle> renderTargets, TextureHandle depthStencil);
		void SetIndexBuffer(BufferHandle indexBuffer);
		void SetPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants);
		void StartTimer(TimerQueryHandle QueryIdx);
		void EndTimer(TimerQueryHandle QueryIdx);

	public:
		inline void InsertTimeStamp(ID3D12QueryHeap* pQueryHeap, uint32_t QueryIdx)
		{
			this->m_commandList->EndQuery(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, QueryIdx);
		}

		inline void ResolveTimeStamps(ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, uint32_t NumQueries)
		{
			this->m_commandList->ResolveQueryData(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, NumQueries, pReadbackHeap, 0);
		}


	private:
		CommandQueueType m_queueType = CommandQueueType::Graphics;
		std::vector<D3D12_RESOURCE_BARRIER> m_barrierMemoryPool;
		D3D12GpuDevice* m_device;
		size_t m_id = ~0ul;

		std::vector<D3D12_RESOURCE_BARRIER> m_barriersRenderPassEnd;

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> m_commandList6;
		ID3D12CommandAllocator* m_allocator;
		std::atomic_bool m_isWaitedOn = false;
		std::vector<D3D12Semaphore> m_waits;
		PipelineType m_activePipelineType = PipelineType::Gfx;
		
	};
}

