#pragma once

#include "EmberGfx/phxGfxDeviceInterface.h"

#include "phxGfxDescriptorHeapsD3D12.h"
#include "phxCommandListD3D12.h"

#include <deque>
#include <mutex>
#include <functional>

#define SCOPED_LOCK(x) std::scoped_lock _(x)

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

	struct D3D12SwapChain final
	{
		Microsoft::WRL::ComPtr<IDXGISwapChain1> SwapChain;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> SwapChain4;

		std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, gfx::kBufferCount> BackBuffers;
		DescriptorHeapAllocation Rtv;

		bool Fullscreen : 1 = false;
		bool VSync : 1 = false;
		bool EnableHDR : 1 = false;

		D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferView()
		{
			const uint64_t currentIndex = this->SwapChain4->GetCurrentBackBufferIndex();
			return this->Rtv.GetCpuHandle(currentIndex);
		}
		ID3D12Resource* GetBackBuffer()
		{
			const uint64_t currentIndex = this->SwapChain4->GetCurrentBackBufferIndex();
			return this->BackBuffers[currentIndex].Get();
		}
	};


	struct D3D12CommandQueue
	{
		D3D12_COMMAND_LIST_TYPE Type;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> Queue;
		Microsoft::WRL::ComPtr<ID3D12Fence> Fence;

		std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> AllocatorPool;
		std::deque<std::pair<uint64_t, ID3D12CommandAllocator*>> AvailableAllocators;

		std::vector<ID3D12CommandList*> m_pendingSubmitCommands;
		std::vector<ID3D12CommandAllocator*> m_pendingSubmitAllocators;

		std::mutex MutexAllocation;
		uint64_t NextFenceValue = 0;

		void Initialize(ID3D12Device* nativeDevice, D3D12_COMMAND_LIST_TYPE type)
		{
			Type = type;

			// Create Command Queue
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Type = Type;
			queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.NodeMask = 0;


			ThrowIfFailed(
				nativeDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&Queue)));

			// Create Fence
			ThrowIfFailed(
				nativeDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
			Fence->SetName(L"D3D12CommandQueue::D3D12CommandQueue::Fence");

			this->NextFenceValue = this->Fence->GetCompletedValue() + 1;
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

		uint64_t SubmitCommandLists(Span<ID3D12CommandList*> commandLists)
		{
			this->Queue->ExecuteCommandLists((UINT)commandLists.Size(), commandLists.begin());

			return this->IncrementFence();
		}

		void DiscardAllocators(uint64_t fenceValue, Span<ID3D12CommandAllocator*> allocators)
		{
			SCOPED_LOCK(this->MutexAllocation);
			for (ID3D12CommandAllocator* allocator : allocators)
			{
				this->AvailableAllocators.push_back(std::make_pair(fenceValue, allocator));
			}
		}

		uint64_t IncrementFence()
		{
			this->Queue->Signal(this->Fence.Get(), this->NextFenceValue);
			return this->NextFenceValue++;
		}

		uint64_t Submit()
		{
			const uint64_t fenceValue = this->SubmitCommandLists(this->m_pendingSubmitCommands);
			this->m_pendingSubmitCommands.clear();

			this->DiscardAllocators(fenceValue, this->m_pendingSubmitAllocators);
			this->m_pendingSubmitAllocators.clear();

			return fenceValue;
		}

		void EnqueueForSubmit(ID3D12CommandList* cmdList, ID3D12CommandAllocator* allocator)
		{
			this->m_pendingSubmitAllocators.push_back(allocator);
			this->m_pendingSubmitCommands.push_back(cmdList);
		}

		ID3D12CommandAllocator* RequestAllocator();
	};

	class GfxDeviceD3D12 final : public IGfxDevice
	{
	public:
		inline static GfxDeviceD3D12* Instance() { return Singleton; }
	public:
		GfxDeviceD3D12();
		~GfxDeviceD3D12();

		void Initialize(SwapChainDesc const& swapChainDesc, void* windowHandle = nullptr) override;
		void Finalize() override;

		void WaitForIdle() override;
		void ResizeSwapChain(SwapChainDesc const& swapChainDesc) override;

		CommandList& BeginGfxContext() ;
		CommandList& BeginComputeContext();

		void SubmitFrame();

	public:
		  D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferView() { return this->m_swapChain.GetBackBufferView(); }
		  ID3D12Resource* GetBackBuffer() { return this->m_swapChain.GetBackBuffer(); }

		  ID3D12Device* GetD3D12Device() { return this->m_d3d12Device.Get(); }
		  ID3D12Device2* GetD3D12Device2() { return this->m_d3d12Device2.Get(); }
		  ID3D12Device5* GetD3D12Device5() { return this->m_d3d12Device5.Get(); }

		  IDXGIFactory6* GetDxgiFactory() { return this->m_factory.Get(); }
		  IDXGIAdapter* GetDxgiAdapter() { return this->m_gpuAdapter.NativeAdapter.Get(); }

		  D3D12CommandQueue& GetQueue(CommandQueueType type) { return this->m_commandQueues[type]; }
		  SpanMutable<D3D12CommandQueue> GetQueues() { return SpanMutable(this->m_commandQueues); }
		  D3D12CommandQueue& GetGfxQueue() { return this->m_commandQueues[CommandQueueType::Graphics]; }
		  D3D12CommandQueue& GetComputeQueue() { return this->m_commandQueues[CommandQueueType::Compute]; }
		  D3D12CommandQueue& GetCopyQueue() { return this->m_commandQueues[CommandQueueType::Copy]; }

		  Span<GpuDescriptorHeap> GetGpuDescriptorHeaps() { return Span<GpuDescriptorHeap>(this->m_gpuDescriptorHeaps.data(), this->m_gpuDescriptorHeaps.size()); }

	private:
		void Initialize();
		void InitializeD3D12Context(IDXGIAdapter* gpuAdapter);
		void CreateSwapChain(SwapChainDesc const& desc, HWND hwnd);

		platform::CommandListD3D12& BeginCommandRecording(CommandQueueType type);

		void SubmitCommandLists();
		void Present();
		void RunGarbageCollection(uint64_t completedFrame = ~0ul);

	private:
		inline static GfxDeviceD3D12* Singleton = nullptr;

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
		uint64_t m_frameCount = 0;
		struct DeleteItem
		{
			uint64_t Frame;
			std::function<void()> DeleteFn;
		};
		std::deque<DeleteItem> m_deleteQueue;

		std::atomic_uint32_t m_activeCmdCount = 0;
		std::vector<std::unique_ptr<platform::CommandListD3D12>> m_commandPool;
	};
}

