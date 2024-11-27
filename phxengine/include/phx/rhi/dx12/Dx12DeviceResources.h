#pragma once

#include "Dx12Common.h"
#include "phx/rhi/RHITypes.h"
#include "Dx12DescriptorHeaps.h"

#include <array>

namespace phx::rhi::dx12
{
	struct TypedCPUDescriptorHandle : public D3D12_CPU_DESCRIPTOR_HANDLE
	{
		TypedCPUDescriptorHandle() = default;
		TypedCPUDescriptorHandle(const TypedCPUDescriptorHandle &other) { ptr = other.ptr; }
		TypedCPUDescriptorHandle(const D3D12_CPU_DESCRIPTOR_HANDLE &other) { ptr = other.ptr; }

		TypedCPUDescriptorHandle operator =(const TypedCPUDescriptorHandle &other)
		{
			ptr = other.ptr;
			return *this;
		}
	};

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
		D3D_PRIMITIVE_TOPOLOGY Topology;
		CompPtr<ID3D12PipelineState> D3D12PipelineState;
	};

	struct PipelineState_Cold
	{
		Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
	};

	struct Texture_Hot
	{
		TypedCPUDescriptorHandle RtView = {};
		TypedCPUDescriptorHandle DsView = {};
		TypedCPUDescriptorHandle SrvView = {};
		TypedCPUDescriptorHandle UavView = {};
	};

	struct Texture_Cold
	{
		CompPtr<D3D12MA::Allocation> Allocation;
		CompPtr<ID3D12Resource> Resource;

		// -- The views ---
		DescriptorView RtvAllocation;
		std::vector<DescriptorView> RtvSubresourcesAlloc = {};

		DescriptorView DsvAllocation;
		std::vector<DescriptorView> DsvSubresourcesAlloc = {};

		DescriptorView Srv;
		std::vector<DescriptorView> SrvSubresourcesAlloc = {};

		DescriptorView UavAllocation;
		std::vector<DescriptorView> UavSubresourcesAlloc = {};

		union
		{
			uint16_t ArraySize = 1;
			uint16_t Depth;
		};
		uint16_t MipLevels = 1;
		uint16_t SampleCount = 1;
	};

	struct GpuBuffer_Hot
	{

	};

	struct GpuBuffer_Cold
	{

	};

}