#pragma once

#include <PhxEngine/RHI/PhxRHI.h>

#include "D3D12Common.h"

namespace PhxEngine::RHI::D3D12
{
	class D3D12Viewport : public RefCounter<IRHIViewport>, public D3D12AdapterChild
	{
	public:
		D3D12Viewport(RHIViewportDesc const& desc, D3D12Adapter* parent) 
			: D3D12AdapterChild(parent) {}
		~D3D12Viewport() = default;

		const RHIViewportDesc& GetDesc() const override { return this->m_desc; }

	public:
		void Initialize();

	private:
		RHIViewportDesc m_desc;
		RefCountPtr<IDXGISwapChain1> m_swapchain;
		RefCountPtr<IDXGISwapChain4> m_swapchain4;
	};
}

