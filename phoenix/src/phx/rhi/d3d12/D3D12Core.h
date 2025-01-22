#pragma once

#ifdef USING_DIRECTX_HEADERS
    #include "directx/dxgiformat.h"
    #include "directx/d3d12.h"
    #include "directx/d3dx12.h"
    #include "dxguids/dxguids.h"
#else
    #include <d3d12.h>
    #include "d3dx12\d3dx12.h"
#endif

#include <dxgi1_6.h>
#include "D3D12MemAlloc.h"

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

	// Helper class for COM exceptions
	class com_exception : public std::exception
	{
	public:
		com_exception(HRESULT hr) noexcept : result(hr) {}

		const char* what() const noexcept override
		{
			static char s_str[64] = {};
			sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
			return s_str;
		}

	private:
		HRESULT result;
	};

	// Helper utility converts D3D API failures into exceptions.
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			// Set a breakpoint on this line to catch DirectX API errors
			throw com_exception(hr);
		}
	}
}
