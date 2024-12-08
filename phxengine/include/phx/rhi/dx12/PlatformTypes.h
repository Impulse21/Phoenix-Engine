#pragma once

#include "Dx12GfxDevice.h"

namespace phx::rhi::platform
{
	using GfxDevice = phx::rhi::dx12::GfxDeviceDx12;

	using SwapChainResource = phx::rhi::dx12::SwapChainResource;
	using SwapChainBindings = phx::rhi::dx12::SwapChainBindings;

	using PipelineStateResource = phx::rhi::dx12::PipelineStateResource;

	using TextureResource = phx::rhi::dx12::TextureResource;
	using TextureBindings = phx::rhi::dx12::TextureBindings;

	using GpuBufferResource = phx::rhi::dx12::GpuBufferResource;
	using GpuBufferBindings = phx::rhi::dx12::GpuBufferBindings;
}