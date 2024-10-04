#pragma once

#include "phxMemory.h"
#include "phxDynamicMemoryPageAllocatorD3D12.h"
#include <deque>

namespace phx::gfx
{
	class GfxDeviceD3D12;
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

	struct DynamicAllocator
	{
		DynamicBuffer Allocate(uint32_t byteSize, uint32_t alignment)
		{
			uint32_t offset = MemoryAlign(ByteOffset, alignment);
			this->ByteOffset = offset + byteSize;

			if (!Page.has_value() || this->ByteOffset > this->PageSize)
			{
				this->Page = this->RingAllocator->Allocate(this->PageSize);
				offset = 0;
				this->ByteOffset = byteSize;
			}

			return DynamicBuffer{
				.BufferHandle = this->Page->BufferHandle,
				.Offset = offset,
				.Data = this->Page->Data + offset
			};
		}

		GpuRingAllocator* RingAllocator;
		std::optional<DynamicMemoryPage> Page;
		size_t PageSize = 4_MiB;
		uint32_t ByteOffset = 0;
	};

	class CommandCtxD3D12 final
	{
		friend GfxDeviceD3D12;
	public:
		CommandCtxD3D12() = default;
		~CommandCtxD3D12() = default;
		
		void Reset(size_t id, CommandQueueType queueType);
		void Close();

	public:
		DynamicBuffer AllocateDynamic(size_t sizeInBytes, size_t alignment = 16);
		void TransitionBarrier(GpuBarrier const& barrier);
		void TransitionBarriers(Span<GpuBarrier> gpuBarriers);
		void ClearBackBuffer(Color const& clearColour);
		void ClearTextureFloat(TextureHandle texture, Color const& clearColour);
		void ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil);
		void SetGfxPipeline(GfxPipelineHandle pipeline); 
		void SetRenderTargetSwapChain();
		void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance);
		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance);
		void SetViewports(Span<Viewport> viewports);
		void SetScissors(Span<Rect> scissors);

		void WriteTexture(TextureHandle texture, uint32_t firstSubresource, size_t numSubresources, SubresourceData* pSubresourceData);

		void SetRenderTargets(Span<TextureHandle> renderTargets, TextureHandle depthStencil);
		void SetDynamicVertexBuffer(BufferHandle tempBuffer, size_t offset, uint32_t slot, size_t numVertices, size_t vertexSize);
		void SetIndexBuffer(BufferHandle indexBuffer);
		void SetDynamicIndexBuffer(BufferHandle tempBuffer, size_t offset, size_t numIndicies, Format indexFormat);
		void SetPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants);

	private:
		std::vector<D3D12_RESOURCE_BARRIER> m_barrierMemoryPool;
		GfxDeviceD3D12* m_device;
		size_t m_id = ~0ul;
		CommandQueueType m_queueType = CommandQueueType::Graphics;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> m_commandList6;
		ID3D12CommandAllocator* m_allocator;
		std::atomic_bool m_isWaitedOn = false;
		std::vector<D3D12Semaphore> m_waits;
		PipelineType m_activePipelineType = PipelineType::Gfx;
		DynamicAllocator m_dynamicAllocator;
		
	};
}

