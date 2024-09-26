#pragma once

#ifdef PHX_VIRTUAL_DEVICE
#include "phxGfxCommandCtxInterface.h"
#else
#include "D3D12/phxCommandCtxD3D12.h"
#endif

namespace phx::gfx
{
	template<typename TImpl>
	class CommandCtxWrapper
	{
	public:
		CommandCtxWrapper(TImpl* impl)
			: m_impl(impl)
		{};


		void TransitionBarrier(GpuBarrier const& barrier)
		{
			this->m_impl->TransitionBarrier(barrier);
		}

		void TransitionBarriers(Span<GpuBarrier> gpuBarriers)
		{
			this->m_impl->TransitionBarrier(gpuBarriers);
		}

		void ClearBackBuffer(Color const& clearColour)
		{
			this->m_impl->ClearBackBuffer(clearColour);
		}

		void ClearTextureFloat(TextureHandle texture, Color const& clearColour)
		{
			this->m_impl->ClearTextureFloat(texture, clearColour);
		}

		void ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil)
		{
			this->m_impl->ClearDepthStencilTexture(depthStencil, clearDepth, depth, clearStencil, stencil);
		}

		void SetGfxPipeline(GfxPipelineHandle handle)
		{
			this->m_impl->SetGfxPipeline(handle);
		}
	private:
		TImpl* m_impl;
	};

#ifdef PHX_VIRTUAL_DEVICE
	using CommandCtx = CommandCtxWrapper<ICommandCtx>;
#else
	using CommandCtx = CommandCtxWrapper<platform::CommandCtxD3D12>;
#endif

}