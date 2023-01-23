#pragma once

#include <PhxEngine/RHI/PhxRHI.h>

#include "D3D12Common.h"
#include <vector>

namespace PhxEngine::RHI::D3D12
{
	class D3D12Viewport : public RefCounter<IRHIViewport>, public D3D12RHIChild
	{
	public:
		D3D12Viewport(RHIViewportDesc const& desc, D3D12RHI* parent) 
			: D3D12RHIChild(parent) {}
		~D3D12Viewport() = default;

		const RHIViewportDesc& GetDesc() const override { return this->m_desc; }

		size_t GetCurrentBackBufferIndex() const;
		RHITextureHandle GetCurrentBackBuffer() const;
		RHITextureHandle GetBackBuffer(size_t index) const;
		RHIRenderPassHandle GetRenderPass() const { return this->m_renderPass; }

	public:
		void Initialize();
		void WaitForIdleFrame();

		void Present();

	private:
		void CreateRenderPass();

	private:
		RHIViewportDesc m_desc;
		RefCountPtr<IDXGISwapChain1> m_swapchain;
		RefCountPtr<IDXGISwapChain4> m_swapchain4;

		RefCountPtr<ID3D12Fence> m_frameFence;
		uint64_t m_frameCount;

		std::vector<RHITextureHandle> m_backBuffers;
		RHIRenderPassHandle m_renderPass;

	};
}

