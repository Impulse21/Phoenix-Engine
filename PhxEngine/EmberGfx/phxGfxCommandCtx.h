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

		void SetRenderTargetSwapChain()
		{
			this->m_platform->SetRenderTargetSwapChain();
		}

		void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
		{
			this->m_platform->Draw(vertexCount, instanceCount, startVertex, startInstance);
		}

		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance)
		{
			this->m_platform->DrawIndexed(indexCount, instanceCount, startIndex, baseVertex, startInstance);
		}

		void SetViewports(Span<Viewport> viewports)
		{
			this->m_platform->SetViewports(viewports);
		}

	private:
		PlatformCommandCtx* m_platform;
	};

}