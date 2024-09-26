#pragma once

#include "EmberGfx/phxGfxDeviceInterface.h"

#ifndef PHX_VIRTUAL_DEVICE
#include "D3D12/phxGfxDeviceD3D12.h"
#endif

namespace phx::gfx
{

	template<typename TImpl>
	class GfxDeviceWrapper
	{
		friend class GfxDeviceFactory;
	public:
		void Initialize(SwapChainDesc const& swapChainDesc, void* windowHandle = nullptr)
		{
			this->m_impl->Initialize(swapChainDesc, windowHandle);
		}

		void Finalize()
		{
			this->m_impl->Finalize();
		}

		void WaitForIdle()
		{
			this->m_impl->WaitForIdle();
		}

		void ResizeSwapChain(SwapChainDesc const& swapChainDesc)
		{
			this->m_impl->ResizeSwapChain(swapChainDesc);
		}

		CommandCtx BeginGfxContext()
		{
			return this->m_impl->BeginGfxContext();
		}

		CommandCtx BeginComputeContext()
		{
			return this->m_impl->BeginComputeContext();
		}

		void SubmitFrame()
		{
			this->m_impl->SubmitFrame();
		}

	private:
		std::unique_ptr<TImpl> m_impl;
	};

#ifdef PHX_VIRTUAL_DEVICE
	using GfxDevice = GfxDeviceWrapper<IGfxDevice>;
#else
	using GfxDevice = GfxDeviceWrapper<GfxDeviceD3D12>;
#endif

	class GfxDeviceFactory
	{
	public:
		static void Create(GfxBackend backend, GfxDevice& device);
	};
}