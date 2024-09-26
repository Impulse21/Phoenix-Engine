#pragma once

#include "D3D12/phxGfxPlatform.h"

namespace phx::gfx
{
	class CommandCtx
	{
	public:
		CommandCtx(PlatformCommandCtx* platform)
			: m_platform(platform)
		{};


		void TransitionBarrier(GpuBarrier const& barrier)
		{
			this->m_platform->TransitionBarrier(barrier);
		}

		void TransitionBarriers(Span<GpuBarrier> gpuBarriers)
		{
			this->m_platform->TransitionBarriers(gpuBarriers);
		}

		void ClearBackBuffer(Color const& clearColour)
		{
			this->m_platform->ClearBackBuffer(clearColour);
		}

		void ClearTextureFloat(TextureHandle texture, Color const& clearColour)
		{
			this->m_platform->ClearTextureFloat(texture, clearColour);
		}

		void ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil)
		{
			this->m_platform->ClearDepthStencilTexture(depthStencil, clearDepth, depth, clearStencil, stencil);
		}

		void SetGfxPipeline(GfxPipelineHandle handle)
		{
			this->m_platform->SetGfxPipeline(handle);
		}
	private:
		PlatformCommandCtx* m_platform;
	};

}