#pragma once


#include "PhxCore/Base.h"
#include "PhxRhi/RHITypes.h"
#include "D3D12Base.h"
#include "D3D12DescriptorHeaps.h"

namespace D3D12MA
{
	class Allocation;
}

namespace phx::rhi::d3d12
{
	constexpr size_t kCacheLineSize = 8 * sizeof(uint64_t);

	enum class DescriptorHeapTypes : uint8_t
	{
		CBV_SRV_UAV,
		Sampler,
		RTV,
		DSV,
		Count,
	};

	struct D3D12DeviceBasicInfo final
	{
		uint32_t NumDeviceNodes;
	};

	struct D3D12Adapter final
	{
		std::string Name;
		size_t DedicatedSystemMemory = 0;
		size_t DedicatedVideoMemory = 0;
		size_t SharedSystemMemory = 0;
		D3D12DeviceBasicInfo BasicDeviceInfo;
		DXGI_ADAPTER_DESC NativeDesc;
		Microsoft::WRL::ComPtr<IDXGIAdapter1> NativeAdapter;

		static HRESULT EnumAdapters(uint32_t adapterIndex, IDXGIFactory6* factory6, IDXGIAdapter1** outAdapter)
		{
			return factory6->EnumAdapterByGpuPreference(
				adapterIndex,
				DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
				IID_PPV_ARGS(outAdapter));
		}
	};


	struct D3D12SwapChain final
	{
		Microsoft::WRL::ComPtr<IDXGISwapChain1> SwapChain;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> SwapChain4;

		std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, rhi::kBufferCount> BackBuffers;
		DescriptorHeapAllocation Rtv;
		rhi::ClearValue ClearColour = {};

		uint32_t        CurrentIndex : 8;
		bool			Fullscreen : 1 = false;
		bool			VSync : 1 = false;
		bool			EnableHDR : 1 = false;
		rhi::Format		Format : 8;

		D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferView()
		{
			const uint64_t currentIndex = SwapChain4->GetCurrentBackBufferIndex();
			return Rtv.GetCpuHandle(currentIndex);
		}
		ID3D12Resource* GetBackBuffer()
		{
			const uint64_t currentIndex = SwapChain4->GetCurrentBackBufferIndex();
			return BackBuffers[currentIndex].Get();
		}
	};

	struct SwapChain final
	{
		Microsoft::WRL::ComPtr<IDXGISwapChain1> SwapChain;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> SwapChain4;

		DescriptorHeapAllocation ViewAllocation;
		std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, rhi::kBufferCount> BackBuffers;

	};

	struct DEFINE_ALIGNED(PipelineState, 64)
	{
		Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;

		enum class PipelineType : uint8_t
		{
			Gfx = 0,
			Compute,
		} Type;

		D3D_PRIMITIVE_TOPOLOGY Topology;
	};
	static_assert(sizeof(PipelineState) <= kCacheLineSize);

	// -- Texture Data ---
	// TODO: Make these fit within a cache line.
	struct Texture
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
		Microsoft::WRL::ComPtr<D3D12MA::Allocation> Allocation;
		DescriptorHeapAllocation DescriptorAllocation_CbvSrvUav; // SRV = 0, UAV = 1

		DescriptorHeapAllocation DescriptorAllocation_Rtv;
		DescriptorHeapAllocation DescriptorAllocation_Dsv;

		rhi::DescriptorIndex BindlessIndex_Srv = rhi::cInvalidDescriptorIndex;
		rhi::DescriptorIndex BindlessIndex_Uav = rhi::cInvalidDescriptorIndex;

		union
		{
			uint16_t ArraySize = 1;
			uint16_t Depth;
		};
		uint16_t MipLevels = 1;
		uint16_t SampleCount = 1;
	};

	// -- End Texture data ---

	struct GpuBuffer
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
		Microsoft::WRL::ComPtr<D3D12MA::Allocation> Allocation;

		void* CpuMappedAddress;
		D3D12_GPU_VIRTUAL_ADDRESS GpuAddress;

		D3D12_GPU_DESCRIPTOR_HANDLE Srv;
		D3D12_GPU_DESCRIPTOR_HANDLE Uav;

		rhi::DescriptorIndex BindlessIndex_Cbv = rhi::cInvalidDescriptorIndex;
		rhi::DescriptorIndex BindlessIndex_Srv = rhi::cInvalidDescriptorIndex;
		rhi::DescriptorIndex BindlessIndex_Uav = rhi::cInvalidDescriptorIndex;

		DescriptorHeapAllocation DescriptorAllocation_CbvSrvUav;
		uint8_t SrvOffset = 0xFF;
		uint8_t UavOffset = 0xFF;

		union
		{
			uint16_t ArraySize = 1;
			uint16_t Depth;
		};
		uint16_t MipLevels = 1;
		uint16_t SampleCount = 1;
	};
}
