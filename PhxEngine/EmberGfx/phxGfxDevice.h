#pragma once

#include "D3D12/phxGfxPlatform.h"
#include "phxGfxCommandCtx.h"

namespace phx::gfx
{
	class GfxDevice : NonCopyable
	{
	public:
		static void Initialize(SwapChainDesc const& swapChainDesc, void* windowHandle = nullptr)
		{
			m_plaform.Initialize(swapChainDesc, windowHandle);
		}

		static void Finalize()
		{
			m_plaform.Finalize();
		}

		static void WaitForIdle()
		{
			m_plaform.WaitForIdle();
		}

		static void ResizeSwapChain(SwapChainDesc const& swapChainDesc)
		{
			m_plaform.ResizeSwapChain(swapChainDesc);
		}

		static CommandCtx BeginGfxContext()
		{
			return m_plaform.BeginGfxContext();
		}

		static CommandCtx BeginComputeContext()
		{
			return m_plaform.BeginComputeContext();
		}

		static void SubmitFrame()
		{
			m_plaform.SubmitFrame();
		}

	public:
		static GfxPipelineHandle CreateGfxPipeline(GfxPipelineDesc const& desc)
		{
			return m_plaform.CreateGfxPipeline(desc);
		}

		static void DeleteGfxPipeline(GfxPipelineHandle handle)
		{
			m_plaform.DeleteGfxPipeline(handle);
		}

	private:
		inline static PlatformGfxDevice m_plaform;
	};

}