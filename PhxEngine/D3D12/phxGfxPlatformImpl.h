#pragma once

#include "phxGfxResources.h"

#include "pch.h"

namespace phx::gfx
{
	namespace platform
	{
		constexpr size_t MaxNumInflightFames = 3;

		extern ID3D12Device* g_Device;

		void Initialize();
		void Finalize();

		void IdleGpu();

		void SubmitFrame(SwapChainHandle handle);

		namespace ResourceManger
		{
			SwapChainHandle CreateSwapChain(SwapChainDesc const& desc);
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
		};
	}
}