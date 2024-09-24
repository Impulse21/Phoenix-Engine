#pragma once

#include "EmberGfx/phxGfxDeviceInterface.h"

namespace phx::gfx
{
	class GfxDeviceD3D12;
	class CommandListD3D12 final : public ICommandList
	{
	public:
		CommandListD3D12() = default;
		~CommandListD3D12() = default;
		
		void Reset(GfxDeviceD3D12* device);

	public:
		void TransitionBarrier(GpuBarrier const& barrier) override;
		void TransitionBarriers(Span<GpuBarrier> gpuBarriers) override;
		void ClearTextureFloat(TextureHandle texture, Color const& clearColour) override;
		void ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil) override;
	};
}

