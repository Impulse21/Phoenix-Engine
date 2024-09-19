#pragma once

#include "phxGfxResources.h"

#include "pch.h"

#include "phxGfxD3D12DescriptorHeaps.h"

namespace phx::gfx
{
	namespace platform
	{
		constexpr size_t MaxNumInflightFames = 3;

		extern Microsoft::WRL::ComPtr<ID3D12Device> g_Device;
		extern Microsoft::WRL::ComPtr<ID3D12Device2> g_Device2;
		extern Microsoft::WRL::ComPtr<ID3D12Device5> g_Device5;
		extern DeviceCapability g_Capabilities;

		void Initialize();
		void Finalize();

		void IdleGpu();

		void SubmitFrame(SwapChainHandle handle);
		
		namespace ResourceManger
		{
			void CreateSwapChain(SwapChainDesc const& desc, SwapChainHandle& handle);

			void Release(SwapChainHandle handle);
		}


		enum class DescriptorHeapTypes : uint8_t
		{
			CBV_SRV_UAV,
			Sampler,
			RTV,
			DSV,
			Count,
		};

		struct D3D12SwapChain
		{
			Microsoft::WRL::ComPtr<IDXGISwapChain1> NativeSwapchain;
			Microsoft::WRL::ComPtr<IDXGISwapChain4> NativeSwapchain4;
			std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, MaxNumInflightFames> BackBuffers;

			d3d12::DescriptorHeapAllocation DescriptorAlloc;
		};
	}
}