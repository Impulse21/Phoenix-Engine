#pragma once

#include "phx/rhi/RHITypes.h"

#include "Dx12Common.h"
#include "Dx12DescriptorHeaps.h"
#include "d3d12ma/D3D12MemAlloc.h"

#include <array>

#define ALIGNAS(x)             __declspec(align(x))
#define DEFINE_ALIGNED(def, a) __declspec(align(a)) def
#define THREAD_LOCAL           __declspec(thread)

#pragma warning(push)
#pragma warning(disable: 4324)  // Warning about structure padding
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

	struct SwapChainBindings
	{
		ID3D12Resource* FrameBackBuffer;
		D3D12_CPU_DESCRIPTOR_HANDLE FrameBackBufferRTV;
		rhi::ClearValue* ClearColour;
	};
	static_assert(sizeof(SwapChainBindings) <= kCacheLineSize);

	struct SwapChainResource
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

	struct DEFINE_ALIGNED(PipelineStateResource, 64)
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
	static_assert(sizeof(PipelineStateResource) <= kCacheLineSize);

	// -- Texture Data ---
	struct DEFINE_ALIGNED(TextureResource, 64)
	{
		CompPtr<ID3D12Resource> Resource;
		CompPtr<D3D12MA::Allocation> Allocation;
		DescriptorHeapAllocation DescriptorAllocation_CbvSrvUav; // SRV = 0, UAV = 1

		DescriptorHeapAllocation DescriptorAllocation_Rtv;
		DescriptorHeapAllocation DescriptorAllocation_Dsv;

		union
		{
			uint16_t ArraySize = 1;
			uint16_t Depth;
		};
		uint16_t MipLevels = 1;
		uint16_t SampleCount = 1;
	};
	// static_assert(sizeof(TextureResource) <= kCacheLineSize);

	struct DEFINE_ALIGNED(TextureBindings, 64)
	{
		ID3D12Resource* Resource;
		D3D12_GPU_DESCRIPTOR_HANDLE Srv;
		D3D12_GPU_DESCRIPTOR_HANDLE Uav;

		D3D12_GPU_DESCRIPTOR_HANDLE Rtv;
		D3D12_GPU_DESCRIPTOR_HANDLE Dsv;

		DescriptorIndex BindlessIndex_Srv = cInvalidDescriptorIndex;
		DescriptorIndex BindlessIndex_Uav = cInvalidDescriptorIndex;
	};
	static_assert(sizeof(TextureBindings) <= kCacheLineSize);

	// -- End Texture data ---

	struct DEFINE_ALIGNED(GpuBufferResource, 64)
	{
		CompPtr<ID3D12Resource> Resource;
		CompPtr<D3D12MA::Allocation> Allocation;
		union
		{
			uint16_t ArraySize = 1;
			uint16_t Depth;
		};
		uint16_t MipLevels = 1;
		uint16_t SampleCount = 1;
	};

	struct DEFINE_ALIGNED(GpuBufferBindings, 64)
	{
		ID3D12Resource* Resource;
		D3D12_GPU_VIRTUAL_ADDRESS GpuAddress;

		DescriptorHeapAllocation DescriptorAllocation_CbvSrvUav;
		uint8_t SrvOffset = 0xFF;
		uint8_t UavOffset = 0xFF;

		DescriptorIndex BindlessIndex_Cbv = cInvalidDescriptorIndex;
		DescriptorIndex BindlessIndex_Srv = cInvalidDescriptorIndex;
		DescriptorIndex BindlessIndex_Uav = cInvalidDescriptorIndex;
	};
	static_assert(sizeof(GpuBufferBindings) <= kCacheLineSize);

	struct DEFINE_ALIGNED(CommandListResource, 64)
	{
		CompPtr<ID3D12GraphicsCommandList> CmdList;
		CompPtr<ID3D12GraphicsCommandList6> CmdList6;
		CommandQueueType Type;
		ID3D12CommandAllocator* Allocator; // TODO: Look into setting this into the command list to save on space
	};
}
#pragma warning(pop)