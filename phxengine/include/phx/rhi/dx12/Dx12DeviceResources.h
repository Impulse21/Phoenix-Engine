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
	constexpr size_t kCacheLineSize = 8 * sizeof(uint64_t);

	struct TypedCPUDescriptorHandle : public D3D12_CPU_DESCRIPTOR_HANDLE
	{
		TypedCPUDescriptorHandle() = default;
		TypedCPUDescriptorHandle(const TypedCPUDescriptorHandle& other) { ptr = other.ptr; }
		TypedCPUDescriptorHandle(const D3D12_CPU_DESCRIPTOR_HANDLE& other) { ptr = other.ptr; }

		TypedCPUDescriptorHandle operator =(const TypedCPUDescriptorHandle& other)
		{
			ptr = other.ptr;
			return *this;
		}
	};

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

	// -- Texture Data ---
	struct DEFINE_ALIGNED(TextureResource, 64)
	{
		CompPtr<ID3D12Resource> Resource;
		CompPtr<D3D12MA::Allocation> Allocation;
	};

	static_assert(sizeof(TextureResource) <= kCacheLineSize);

	struct DEFINE_ALIGNED(TextureView, 64)
	{
		DescriptorHeapAllocation Srv_Uav_Descriptors;
	};
	static_assert(sizeof(TextureView) <= kCacheLineSize);

	// -- End Texture data ---
	struct GpuBuffer
	{
		CompPtr<ID3D12Resource> Resource;
		CompPtr<D3D12MA::Allocation> Allocation;
		D3D12_GPU_VIRTUAL_ADDRESS GpuAddress;
		union
		{
			uint16_t ArraySize = 1;
			uint16_t Depth;
		};
		uint16_t MipLevels = 1;
		uint16_t SampleCount = 1;
	};

	static_assert(sizeof(GpuBuffer) <= kCacheLineSize);
}