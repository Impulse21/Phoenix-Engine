#pragma once

#include "Dx12GfxDevice.h"

namespace phx::rhi::platform
{
	using GfxDevice = phx::rhi::dx12::GfxDeviceDx12;
	using SwapChain = phx::rhi::dx12::SwapChain;
	using PipelineState = phx::rhi::dx12::PipelineState;
	using Texture = phx::rhi::dx12::Texture;
	using GpuBuffer = phx::rhi::dx12::GpuBuffer;
}