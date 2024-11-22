#pragma once

#include "Dx12GfxDevice.h"

namespace phx::rhi::platform
{
	using GfxDevice = phx::rhi::dx12::GfxDeviceDx12;

	using SwapChain_Hot = phx::rhi::dx12::SwapChain_Hot;
	using SwapChain_Cold = phx::rhi::dx12::SwapChain_Cold;

	using PipelineState_Hot = phx::rhi::dx12::PipelineState_Hot;
	using PipelineState_Cold = phx::rhi::dx12::PipelineState_Cold;

	using Texture_Hot = phx::rhi::dx12::Texture_Hot;
	using Texture_Cold = phx::rhi::dx12::Texture_Cold;

	using GpuBuffer_Hot = phx::rhi::dx12::GpuBuffer_Hot;
	using GpuBuffer_Cold = phx::rhi::dx12::GpuBuffer_Cold;
}