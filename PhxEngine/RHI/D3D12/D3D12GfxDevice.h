#pragma once

#include "D3D12Common.h"
#include "D3D12Adapter.h"
#include "D3D12Resources.h"
#include "D3D12CommandQueue.h"

#include <dstorage.h>
#include <D3D12MemAlloc.h>

namespace PhxEngine::RHI::D3D12
{
	class D3D12GfxDevice
	{
	public:
		static bool Initialize();
		static bool Finalize();
		static void WaitForIdle();
		static GpuMemoryUsage GetMemoryUsage();

		static void Present(SwapChain const& swapchainToPresent);

		// Create Methods
	public:
		static bool CreateSwapChain(SwapChainDesc const& desc, void* windowHandle, D3D12SwapChain& swapChain);
		static bool CreateTexture(TextureDesc const& desc, D3D12Texture& texture);

	public:
		[[nodiscard]] static D3D12Adapter GpuAdapter();
		[[nodiscard]] static Core::RefCountPtr<IDXGIFactory6> DxgiFctory6();
		[[nodiscard]] static Core::RefCountPtr<ID3D12Device> D3d12Device();
		[[nodiscard]] static Core::RefCountPtr<ID3D12Device2> D3d12Device2();
		[[nodiscard]] static Core::RefCountPtr<ID3D12Device5> D3d12Device5();

		[[nodiscard]] static D3D12CommandQueue& Queue(RHI::CommandListType type);

		// -- Direct Storage ---
		[[nodiscard]] static Core::RefCountPtr<IDStorageFactory> DStorageFactory();
		[[nodiscard]] static Core::RefCountPtr<IDStorageQueue> DStorageQueue(DSTORAGE_REQUEST_SOURCE_TYPE type);
		[[nodiscard]] static Core::RefCountPtr<ID3D12Fence> DStorageFence();

		// -- Direct MA ---
		[[nodiscard]] static Core::RefCountPtr<D3D12MA::Allocator> D3d12MemAllocator();

		[[nodiscard]] static RHICapability RhiCapabilities();
		[[nodiscard]] static D3D12_FEATURE_DATA_ROOT_SIGNATURE FeatureDataRootSignature();
		[[nodiscard]] static D3D12_FEATURE_DATA_SHADER_MODEL FeatureDataShaderModel();
		[[nodiscard]] static ShaderModel MinShaderModel();
		[[nodiscard]] static bool IsUnderGraphicsDebugger();
		[[nodiscard]] static bool DebugEnabled();

		[[nodiscard]] static ID3D12DescriptorAllocator& CpuResourceHeap();
		[[nodiscard]] static ID3D12DescriptorAllocator& CpuSamplerHeap();
		[[nodiscard]] static ID3D12DescriptorAllocator& CpuRenderTargetHeap();
		[[nodiscard]] static ID3D12DescriptorAllocator& CpuDepthStencilHeap();

		[[nodiscard]] static ID3D12DescriptorAllocator& GpuResourceHeap();
		[[nodiscard]] static ID3D12DescriptorAllocator& GpuSamplerHeap();
		[[nodiscard]] static RHI::DeferedReleaseQueue& ReleaseQueue();

		static size_t MaxFramesInflight();

	private:
		static void InitializeDeviceResources();

	private:
		static D3D12Adapter								m_GpuAdapter;
		static Core::RefCountPtr<IDXGIFactory6>			m_DxgiFctory6;
		static Core::RefCountPtr<ID3D12Device>			m_D3d12Device;
		static Core::RefCountPtr<ID3D12Device2>			m_D3d12Device2;
		static Core::RefCountPtr<ID3D12Device5>			m_D3d12Device5;

		static D3D12CommandQueue						m_Queues[(size_t)RHI::CommandListType::Count];

		static Core::RefCountPtr<IDStorageFactory>		m_DStorageFactory;
		static Core::RefCountPtr<IDStorageQueue>		m_DStorageQueues[2];
		static Core::RefCountPtr<ID3D12Fence>			m_DStorageFence;

		static Core::RefCountPtr<D3D12MA::Allocator>	m_D3d12MemAllocator;

		static DeferedReleaseQueue						m_ReleaseQueue;
		static RHICapability							m_RhiCapabilities;
		static D3D12_FEATURE_DATA_ROOT_SIGNATURE		m_FeatureDataRootSignature;
		static D3D12_FEATURE_DATA_SHADER_MODEL			m_FeatureDataShaderModel;
		static ShaderModel								m_MinShaderModel;
		static bool										m_IsUnderGraphicsDebugger;
		static size_t									m_BufferCount;
		static size_t									m_FrameCount;

		// -- Descriptor Heaps ---
		static std::array<D3D12CpuDescriptorHeap, (int)DescriptorHeapTypes::Count> m_cpuDescriptorHeaps;
		static std::array<D3D12GpuDescriptorHeap, 2>	m_gpuDescriptorHeaps;

		static std::array<Core::RefCountPtr<ID3D12Fence>, (int)RHI::CommandListType::Count> FrameFences;

	};
}

