#pragma once

#include <functional>

#include "PhxRhi/RHITypes.h"
#include "PhxCore/Pool.h"

#include "D3D12Base.h"
#include "D3D12Types.h"

// Teir 1 limit is 1,000,000
// https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
#define TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE 1000000

#define NUM_BINDLESS_RESOURCES TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE / 2

namespace phx::rhi
{
	struct RhiCreateInfo;
}

namespace phx::RHI::d3d12
{
	// -- Forward Declares ---
	struct D3D12CommandQueue;
	class CpuDescriptorHeap;
	class GpuDescriptorHeap;
	class D3D12ResourceManager;

    // -- Globals ---
    extern Microsoft::WRL::ComPtr<IDXGIFactory6> g_dxgiFactory;
    extern Microsoft::WRL::ComPtr<ID3D12Device> g_d3d12Device;
	extern Microsoft::WRL::ComPtr<ID3D12Device2> g_d3d12Device2;
    extern Microsoft::WRL::ComPtr<ID3D12Device5> g_d3d12Device5;

	extern Microsoft::WRL::ComPtr<D3D12MA::Allocator> g_d3d12MemAllocator;

	extern D3D12Adapter g_adapter;

	extern bool g_debugLayersEnabled;
	extern bool g_isUnderGfxDebugger;
	extern RHI::DeviceCapability g_capabilities;
	extern D3D12_RESOURCE_HEAP_TIER g_resourceHeapTeir;

	// -- Command queues ---
	extern EnumArray<D3D12CommandQueue, CommandQueueType> g_commandQueue;

	// -- Descriptor Heaps ---
	extern CpuDescriptorHeap* g_cpuDescHeap_Resource;
	extern CpuDescriptorHeap* g_cpuDescHeap_Sampler;
	extern CpuDescriptorHeap* g_cpuDescHeap_Rtv;
	extern CpuDescriptorHeap* g_cpuDescHeap_Dsv;

	extern GpuDescriptorHeap* g_gpuDescHeap_Resource;
	extern GpuDescriptorHeap* g_gpuDescHeap_Sampler;

	extern D3D12ResourceManager* g_resourceManager;

	extern size_t g_frameCount;

	extern D3D12SwapChain g_swapChain;

	// -- Resource pools --- TODO: Move to a ResourceManager class that wraps these pool
	extern phx::PagedPool<RHI::PipelineState, d3d12::PipelineState> g_pipelineStatePool;
	extern phx::PagedPool<RHI::Texture, d3d12::Texture> g_texturePool;
	extern phx::PagedPool<RHI::GpuBuffer, d3d12::GpuBuffer> g_bufferPool;


	void InitializeResources(RHI::RhiCreateInfo const& createInfo);
	void FinalizeResources();

	struct DeferredItem
	{
		uint64_t Frame;
		std::function<void()> DeferredFunc;
	};

	void EnqueueDelete(DeferredItem&& item);
}
