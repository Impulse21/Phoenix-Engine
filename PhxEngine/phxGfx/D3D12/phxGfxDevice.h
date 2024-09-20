#pragma once

#include "pch.h"

#include "phxGfxCore.h"
#include "phxGfxD3D12DescriptorHeaps.h"

namespace phx::gfx
{
	static const GUID RenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
	static const GUID PixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

	struct D3D12DeviceBasicInfo final
	{
		uint32_t NumDeviceNodes;
	};

	enum class DescriptorHeapTypes : uint8_t
	{
		CBV_SRV_UAV,
		Sampler,
		RTV,
		DSV,
		Count,
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

	struct D3D12CommandQueue
	{
		D3D12_COMMAND_LIST_TYPE Type;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> Queue;
		Microsoft::WRL::ComPtr<ID3D12Fence> Fence;

		void Initialize(ID3D12Device* nativeDevice, D3D12_COMMAND_LIST_TYPE type)
		{
			Type = type;

			// Create Command Queue
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Type = Type;
			queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.NodeMask = 0;

			dx::ThrowIfFailed(
				nativeDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&Queue)));

			// Create Fence
			dx::ThrowIfFailed(
				nativeDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
			Fence->SetName(L"D3D12CommandQueue::D3D12CommandQueue::Fence");

			switch (type)
			{
			case D3D12_COMMAND_LIST_TYPE_DIRECT:
				Queue->SetName(L"Direct Command Queue");
				break;
			case D3D12_COMMAND_LIST_TYPE_COPY:
				Queue->SetName(L"Copy Command Queue");
				break;
			case D3D12_COMMAND_LIST_TYPE_COMPUTE:
				Queue->SetName(L"Compute Command Queue");
				break;
			}
		}
	};

	struct D3D12SwapChain final
	{
		Microsoft::WRL::ComPtr<IDXGISwapChain1> SwapChain;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> SwapChain4;

		std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, gfx::kBufferCount> BackBuffers;
		DescriptorHeapAllocation Rtv;

		bool Fullscreen : 1 = false;
		bool VSync : 1 = false;
		bool EnableHDR : 1 = false;
	};

	class D3D12Device final : public Device
	{
	public:
		inline static D3D12Device* Impl() { return Singleton; }

	public:
		D3D12Device(SwapChainDesc const& desc, HWND hwnd);
		~D3D12Device() = default;

		void WaitForIdle() override;
		void ResizeSwapChain(SwapChainDesc const& desc) override;
		void Present() override;

	public:
		ID3D12Device* GetD3D12Device() { return this->m_d3d12Device.Get(); }
		ID3D12Device2* GetD3D12Device2() { return this->m_d3d12Device2.Get(); }
		ID3D12Device5* GetD3D12Device5() { return this->m_d3d12Device5.Get(); }

		IDXGIFactory6* GetDxgiFactory() { return this->m_factory.Get(); }
		IDXGIAdapter* GetDxgiAdapter() { return this->m_gpuAdapter.NativeAdapter.Get(); }

		D3D12CommandQueue& GetQueue(CommandQueueType type) { return this->m_commandQueues[type]; }
		D3D12CommandQueue& GetGfxQueue() { return this->m_commandQueues[CommandQueueType::Graphics]; }
		D3D12CommandQueue& GetComputeQueue() { return this->m_commandQueues[CommandQueueType::Compute]; }
		D3D12CommandQueue& GetCopyQueue() { return this->m_commandQueues[CommandQueueType::Copy]; }

	private:
		void Initialize();
		void InitializeD3D12Context(IDXGIAdapter* gpuAdapter);
		void CreateSwapChain(SwapChainDesc const& desc, HWND hwnd);

	private:
		inline static D3D12Device* Singleton = nullptr;
		Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
		Microsoft::WRL::ComPtr<ID3D12Device> m_d3d12Device;
		Microsoft::WRL::ComPtr<ID3D12Device2> m_d3d12Device2;
		Microsoft::WRL::ComPtr<ID3D12Device5> m_d3d12Device5;

		D3D12Adapter m_gpuAdapter;
		D3D12SwapChain m_swapChain;

		D3D12_FEATURE_DATA_ROOT_SIGNATURE m_featureDataRootSignature = {};
		D3D12_FEATURE_DATA_SHADER_MODEL   m_featureDataShaderModel = {};
		ShaderModel m_minShaderModel = ShaderModel::SM_6_0;

		bool m_isUnderGraphicsDebugger = false;
		bool m_debugLayersEnabled = false;
		gfx::DeviceCapability m_capabilities;


		// -- Command Queues ---
		EnumArray<D3D12CommandQueue, CommandQueueType> m_commandQueues;

		// -- Descriptor Heaps ---
		EnumArray<CpuDescriptorHeap, DescriptorHeapTypes> m_cpuDescriptorHeaps;
		std::array<GpuDescriptorHeap, 2> m_gpuDescriptorHeaps;

		std::array<EnumArray<Microsoft::WRL::ComPtr<ID3D12Fence>, CommandQueueType>, kBufferCount> m_frameFences;
		uint64_t m_frameCount = 1;
	};
}