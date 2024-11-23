#pragma once

#include "Dx12Common.h"
#include "phx/rhi/RHITypes.h"
#include "Dx12DescriptorHeaps.h"

#include <array>

namespace phx::rhi::dx12
{
	struct SwapChain_Hot
	{
		ID3D12Resource* CurrentBackBuffer = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE CurrentRtv;
	};

	struct SwapChain_Cold
	{
		Microsoft::WRL::ComPtr<IDXGISwapChain1> SwapChain;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> SwapChain4;

		std::array<CompPtr<ID3D12Resource>, rhi::kBufferCount> BackBuffers;
		DescriptorHeapAllocation ViewAllocation;

		rhi::ClearValue ClearColour = {};
		bool Fullscreen : 1 = false;
		bool VSync : 1 = false;
		bool EnableHDR : 1 = false;
	};

	struct PipelineState_Hot
	{

	};

	struct PipelineState_Cold
	{

	};

	struct Texture_Hot
	{

	};

	struct Texture_Cold
	{

	};

	struct GpuBuffer_Hot
	{

	};

	struct GpuBuffer_Cold
	{

	};

}