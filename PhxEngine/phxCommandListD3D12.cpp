#include "pch.h"
#include "phxCommandListD3D12.h"

using namespace phx::gfx;

void phx::gfx::CommandListD3D12::Reset(CommandQueueType queueType, GfxDeviceD3D12* device)
{
}

void CommandListD3D12::TransitionBarrier(GpuBarrier const& barrier)
{
}

void phx::gfx::CommandListD3D12::TransitionBarriers(Span<GpuBarrier> gpuBarriers)
{
}

void phx::gfx::CommandListD3D12::ClearBackBuffer(Color const& clearColour)
{
}

void phx::gfx::CommandListD3D12::ClearTextureFloat(TextureHandle texture, Color const& clearColour)
{
}

void phx::gfx::CommandListD3D12::ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil)
{
}
