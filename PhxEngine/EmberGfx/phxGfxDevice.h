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
			PlatformGfxDevice::Initialize(swapChainDesc, windowHandle);
		}

		static void Finalize()
		{
			PlatformGfxDevice::Finalize();
		}

		static void WaitForIdle()
		{
			PlatformGfxDevice::WaitForIdle();
		}

		static void ResizeSwapChain(SwapChainDesc const& swapChainDesc)
		{
			PlatformGfxDevice::ResizeSwapChain(swapChainDesc);
		}

		static CommandCtx BeginGfxContext()
		{
			return PlatformGfxDevice::BeginGfxContext();
		}

		static CommandCtx BeginComputeContext()
		{
			return PlatformGfxDevice::BeginComputeContext();
		}

		static void SubmitFrame()
		{
			PlatformGfxDevice::SubmitFrame();
		}

	public:
		static GfxPipelineHandle CreateGfxPipeline(GfxPipelineDesc const& desc)
		{
			return PlatformGfxDevice::CreateGfxPipeline(desc);
		}

		static TextureHandle CreateTexture(TextureDesc const& texture)
		{
			return PlatformGfxDevice::CreateTexture(texture);
		}

		static InputLayoutHandle CreateInputLayout(Span<VertexAttributeDesc> desc)
		{
			return PlatformGfxDevice::CreateInputLayout(desc);
		}

	private:
	};

}