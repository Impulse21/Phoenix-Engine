#pragma once

#include "EmberGfx/phxGfxCommandCtxInterface.h"

namespace phx::gfx
{
	class GfxDeviceD3D12;
}

namespace phx::gfx::platform
{
	struct D3D12Semaphore
	{
		Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
		uint64_t fenceValue = 0;
	};

	class CommandCtxD3D12 final : public internal::ICommandCtx
	{
		friend GfxDeviceD3D12;
	public:
		CommandCtxD3D12() = default;
		~CommandCtxD3D12() = default;
		
		void Reset(size_t id, CommandQueueType queueType, GfxDeviceD3D12* device);

	public:
		void TransitionBarrier(GpuBarrier const& barrier) override;
		void TransitionBarriers(Span<GpuBarrier> gpuBarriers) override;
		void ClearBackBuffer(Color const& clearColour) override;
		void ClearTextureFloat(TextureHandle texture, Color const& clearColour) override;
		void ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil) override;
		void SetGfxPipeline(GfxPipelineHandle handle) override;

	private:
		GfxDeviceD3D12* m_device;
		size_t m_id = ~0ul;
		CommandQueueType m_queueType = CommandQueueType::Graphics;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> m_commandList6;
		ID3D12CommandAllocator* m_allocator;
		std::atomic_bool m_isWaitedOn = false;
		std::vector<D3D12Semaphore> m_waits;
	};
}

