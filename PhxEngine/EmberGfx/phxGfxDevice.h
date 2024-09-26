#pragma once

#include "D3D12/phxGfxPlatform.h"
#include "phxGfxCommandCtx.h"

namespace phx::gfx
{
	class GfxDevice : NonCopyable
	{
	public:
		void Initialize(SwapChainDesc const& swapChainDesc, void* windowHandle = nullptr)
		{
			this->m_plaform.Initialize(swapChainDesc, windowHandle);
		}

		void Finalize()
		{
			this->m_plaform.Finalize();
		}

		void WaitForIdle()
		{
			this->m_plaform.WaitForIdle();
		}

		void ResizeSwapChain(SwapChainDesc const& swapChainDesc)
		{
			this->m_plaform.ResizeSwapChain(swapChainDesc);
		}

		CommandCtx BeginGfxContext()
		{
			return this->m_plaform.BeginGfxContext();
		}

		CommandCtx BeginComputeContext()
		{
			return this->m_plaform.BeginComputeContext();
		}

		void SubmitFrame()
		{
			this->m_plaform.SubmitFrame();
		}

	public:
		GfxPipelineHandle CreateGfxPipeline(GfxPipelineDesc const& desc)
		{
			return this->m_plaform.CreateGfxPipeline(desc);
		}

		void DeleteGfxPipeline(GfxPipelineHandle handle)
		{
			this->m_plaform.DeleteGfxPipeline(handle);
		}

	private:
		PlatformGfxDevice m_plaform;
	};

}