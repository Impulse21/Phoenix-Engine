#pragma once

#include "phx/rhi/RHITypes.h"
#include "phx/core/Pool.h"

#include "D3D12Base.h"
#include "D3D12Types.h"
#include "D3D12MemAlloc.h"

namespace phx::rhi
{
	struct RhiCreateInfo;
}

namespace phx::rhi::d3d12
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

	extern bool g_isUnderGfxDebugger;

	// -- Command queues ---
	extern D3D12CommandQueue* g_commandQueue_Gfx;
	extern D3D12CommandQueue* g_commandQueue_Compute;
	extern D3D12CommandQueue* g_commandQueue_Copy;

	// -- Descriptor Heaps ---
	extern CpuDescriptorHeap* g_cpuDescHeap_Resource;
	extern CpuDescriptorHeap* g_cpuDescHeap_Sampler;
	extern CpuDescriptorHeap* g_cpuDescHeap_Rtv;
	extern CpuDescriptorHeap* g_cpuDescHeap_Dsv;

	extern GpuDescriptorHeap* g_gpuDescHeap_Resource;
	extern GpuDescriptorHeap* g_gpuDescHeap_Sampler;

	extern D3D12ResourceManager* g_resourceManager;

	extern size_t g_frameCount;

	// -- Resource pools ---
	extern phx::ResourcePool<rhi::PipelineState, PipelineStateResource> g_pipelineStatePool;
	extern phx::ResourcePool<rhi::Texture, TextureBindings, TextureResource> g_texturePool;


	void InitializeResources(rhi::RhiCreateInfo const& createInfo);
}
