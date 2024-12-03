#pragma once

#include "Dx12Common.h"
#include "phx/rhi/RHITypes.h"
#include "Dx12DescriptorHeaps.h"

#include <array>

#define ALIGNAS(x)             __declspec(align(x))
#define DEFINE_ALIGNED(def, a) __declspec(align(a)) def
#define THREAD_LOCAL           __declspec(thread)

namespace phx::rhi::dx12
{
	struct SwapChain
	{
		CompPtr<IDXGISwapChain1> SwapChain;
		CompPtr<IDXGISwapChain4> SwapChain4;

		DescriptorHeapAllocation ViewAllocation;
		std::array<CompPtr<ID3D12Resource>, rhi::kBufferCount> BackBuffers;

		rhi::ClearValue ClearColour = {};

		uint32_t        CurrentIndex : 8;
		bool			Fullscreen : 1 = false;
		bool			VSync : 1 = false;
		bool			EnableHDR : 1 = false;
		rhi::Format		Format	: 8;
	};

	struct PipelineState
	{
		CompPtr<ID3D12PipelineState> D3D12PipelineState;
		CompPtr<ID3D12RootSignature> RootSignature;

		enum class PipelineType : uint8_t
		{
			Gfx = 0,
			Compute,
		} Type;

		D3D_PRIMITIVE_TOPOLOGY Topology;
	}; 
	static_assert(sizeof(PipelineState) <= kCacheLineSize);

	struct Texture
	{
		CompPtr<ID3D12Resource> Resource;
		CompPtr<D3D12MA::Allocation> Allocation;

		/// Current state of the buffer
		uint32_t Width : 16;
		uint32_t Height : 16;
		uint32_t Depth : 16;
		uint32_t MipLevels : 5;
		uint32_t ArraySize : 11;
		uint32_t Format : 8;
		uint32_t NodeIndex : 4;
		uint32_t SampleCount : 5;
		uint32_t Uav : 1;
	};
	static_assert(sizeof(Texture) <= kCacheLineSize);

	struct GpuBuffer
	{
		CompPtr<ID3D12Resource> Resource;
		CompPtr<D3D12MA::Allocation> Allocation;
		void* CpuMappedAddress;

		D3D12_GPU_VIRTUAL_ADDRESS GpuAddress;

	};
	static_assert(sizeof(GpuBuffer) <= kCacheLineSize);
}