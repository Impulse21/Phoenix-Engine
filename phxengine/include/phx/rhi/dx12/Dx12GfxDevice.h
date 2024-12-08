#pragma once

#include "phx/rhi/RHITypes.h"
#include "Dx12DeviceResources.h"
#include "d3d12ma/D3D12MemAlloc.h"

#include "Dx12Common.h"
#include "Dx12DescriptorHeaps.h"
#include "Dx12CommandQueue.h"

#include <deque>
#include <mutex>

namespace phx::rhi::dx12
{
	constexpr uint32_t kTimestampQueryHeapSize = 1024; // 4096;
	const GUID RenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
	const GUID PixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

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

	struct DescriptorView
	{
		DescriptorHeapAllocation Allocation;
		DescriptorIndex BindlessIndex = cInvalidDescriptorIndex;
		D3D12_DESCRIPTOR_HEAP_TYPE Type = {};
		union
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
			D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
			D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
			D3D12_SAMPLER_DESC SAMDesc;
			D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
			D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
		};

		uint32_t FirstMip = 0;
		uint32_t MipCount = 0;
		uint32_t FirstSlice = 0;
		uint32_t SliceCount = 0;
	};

	class BindlessDescriptorTable
	{
	public:
		BindlessDescriptorTable() = default;
		void Initialize(DescriptorHeapAllocation&& allocation) { this->m_allocation = allocation; }
		BindlessDescriptorTable(const BindlessDescriptorTable&) = delete;
		BindlessDescriptorTable(BindlessDescriptorTable&&) = delete;
		BindlessDescriptorTable& operator = (const BindlessDescriptorTable&) = delete;
		BindlessDescriptorTable& operator = (BindlessDescriptorTable&&) = delete;

	public:
		DescriptorIndex Allocate() { return this->m_descriptorIndexPool.Allocate(); }
		void Free(DescriptorIndex index) { this->m_descriptorIndexPool.Release(index); }

		D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(DescriptorIndex index) const { return this->m_allocation.GetCpuHandle(index); }
		D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(DescriptorIndex index) const { return this->m_allocation.GetGpuHandle(index); }

	private:
		struct DescriptorIndexPool
		{
			// Removes the first element from the free list and returns its index
			UINT Allocate()
			{
				std::scoped_lock Guard(this->IndexMutex);

				UINT NewIndex;
				if (!IndexQueue.empty())
				{
					NewIndex = IndexQueue.front();
					IndexQueue.pop_front();
				}
				else
				{
					NewIndex = Index++;
				}
				return NewIndex;
			}

			void Release(UINT index)
			{
				std::scoped_lock Guard(this->IndexMutex);
				IndexQueue.push_back(index);
			}

			std::mutex IndexMutex;
			std::deque<DescriptorIndex> IndexQueue;
			UINT Index = 0;
		};

	private:
		DescriptorHeapAllocation m_allocation;
		DescriptorIndexPool m_descriptorIndexPool;
	};

	struct GpuTimerManager
	{
		void Initialize() {};
		// TimerQueryHandle NewTimer() { return this->NumTimers++; }
		void BeginReadBack() {};
		void EndReadBack() {};

		// float GetTime(TimerQueryHandle handle);

		Microsoft::WRL::ComPtr<ID3D12QueryHeap> QueryHeap = nullptr;
		std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kBufferCount> ReadBackBuffers = {};
		uint64_t* TimeStampBuffer = nullptr;
		uint64_t Fence = 0;
		uint32_t MaxNumTimers = kTimestampQueryHeapSize;
		std::atomic<uint32_t> NumTimers = 1;
		uint64_t ValidTimeStart = 0;
		uint64_t ValidTimeEnd = 0;
		double GpuTickDelta = 0.0;
	};

	class GfxDeviceDx12 final
	{
	public:
		static GfxDeviceDx12* Instance() { return Singleton; }

	public:
		GfxDeviceDx12(GfxDeviceDescriptor const& descriptor);
		~GfxDeviceDx12();

		void WaitForIdle();
		// void ResizeSwapChain(SwapChainDesc const& swapChainDesc);

		// ICommandCtx* BeginCommandCtx(phx::gfx::CommandQueueType type = CommandQueueType::Graphics);
		// void SubmitFrame();

		// DynamicMemoryPage AllocateDynamicMemoryPage(size_t pageSize);

		void Present(SwapChainResource& swapChain, SwapChainBindings& bindings);

	public:	
		bool CreateSwapChain(SwapChainDescriptor const& desc, SwapChainResource& resource, SwapChainBindings& bindings);
		bool CreatePipeline(PipelineStateDescriptor const& desc, PipelineStateResource& resource);
		bool CreateBuffer(GpuBufferDescriptor const& desc, GpuBufferResource& resource, GpuBufferBindings& bindings, MemInfo* initData = nullptr);
		bool CreateTexture(TextureDescriptor const& desc, TextureResource& resource, TextureBindings& bindings, MemInfo* initData = nullptr);

		// -- Getter and setters ---
	public:
		ShaderFormat GetShaderFormat() const { return ShaderFormat::Hlsl6; }

		// -- Platform specific ---
	public:
		size_t GetFrameCount() { return m_frameCount; }

		ID3D12Device* GetD3D12Device() { return m_d3d12Device.Get(); }
		ID3D12Device2* GetD3D12Device2() { return m_d3d12Device2.Get(); }
		ID3D12Device5* GetD3D12Device5() { return m_d3d12Device5.Get(); }

		IDXGIFactory6* GetDxgiFactory() { return m_factory.Get(); }
		IDXGIAdapter* GetDxgiAdapter() { return m_gpuAdapter.NativeAdapter.Get(); }

		D3D12CommandQueue& GetQueue(CommandQueueType type) { return m_commandQueues[type]; }
		SpanMutable<D3D12CommandQueue> GetQueues() { return SpanMutable(m_commandQueues); }
		D3D12CommandQueue& GetGfxQueue() { return m_commandQueues[CommandQueueType::Graphics]; }
		D3D12CommandQueue& GetComputeQueue() { return m_commandQueues[CommandQueueType::Compute]; }
		D3D12CommandQueue& GetCopyQueue() { return m_commandQueues[CommandQueueType::Copy]; }

		CpuDescriptorHeap& GetResourceCpuHeap() { return m_cpuDescriptorHeaps[DescriptorHeapTypes::CBV_SRV_UAV]; }
		CpuDescriptorHeap& GetRtvCpuHeap() { return m_cpuDescriptorHeaps[DescriptorHeapTypes::RTV]; }
		CpuDescriptorHeap& GetDsvCpuHeap() { return m_cpuDescriptorHeaps[DescriptorHeapTypes::DSV]; }
		Span<GpuDescriptorHeap> GetGpuDescriptorHeaps() { return Span<GpuDescriptorHeap>(m_gpuDescriptorHeaps.data(), m_gpuDescriptorHeaps.size()); }
		GpuTimerManager& GetGpuTimerManager() { return m_gpuTimerManager; }

		void PollDebugMessages();

	private:
		void Initialize();
		void InitializeResourcePools();
		void FinalizeResourcePools();

		void InitializeD3D12Context();

		void SubmitCommandLists();
		void RunGarbageCollection(uint64_t completedFrame = ~0ul);

		void CreateSrv(TextureDescriptor const& desc, ID3D12Resource* d3d12Resource, D3D12_CPU_DESCRIPTOR_HANDLE handle);
		void CreateUav(TextureDescriptor const& desc, ID3D12Resource* d3d12Resource, D3D12_CPU_DESCRIPTOR_HANDLE handle);
		void CreateRtv(TextureDescriptor const& desc, ID3D12Resource* d3d12Resource, D3D12_CPU_DESCRIPTOR_HANDLE handle);
		void CreateDsv(TextureDescriptor const& desc, ID3D12Resource* d3d12Resource, D3D12_CPU_DESCRIPTOR_HANDLE handle);

		void CopyBindlessDescriptor(DescriptorIndex index, D3D12_CPU_DESCRIPTOR_HANDLE srcHandle);

		int CreateSubresource(GpuBufferHandle buffer, GpuBufferDescriptor const& desc, SubresouceType subresourceType, size_t offset, size_t size = ~0u);

		int CreateShaderResourceView(GpuBufferHandle buffer, GpuBufferDescriptor const& desc, size_t offset, size_t size);
		int CreateUnorderedAccessView(GpuBufferHandle buffer, GpuBufferDescriptor const& desc, size_t offset, size_t size);

	private:
		inline static GfxDeviceDx12* Singleton = nullptr;

		const GfxDeviceDescriptor m_descriptor = {};
		Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
		Microsoft::WRL::ComPtr<ID3D12Device> m_d3d12Device;
		Microsoft::WRL::ComPtr<ID3D12Device2> m_d3d12Device2;
		Microsoft::WRL::ComPtr<ID3D12Device5> m_d3d12Device5;
		Microsoft::WRL::ComPtr<D3D12MA::Allocator> m_d3d12MemAllocator;

		D3D12Adapter m_gpuAdapter;

		D3D12_FEATURE_DATA_ROOT_SIGNATURE m_featureDataRootSignature = {};
		D3D12_FEATURE_DATA_SHADER_MODEL   m_featureDataShaderModel = {};
		ShaderModel m_minShaderModel = ShaderModel::SM_6_0;

		bool m_isUnderGraphicsDebugger = false;
		bool m_debugLayersEnabled = false;
		rhi::DeviceCapability m_capabilities;

		// -- Command Queues ---
		EnumArray<D3D12CommandQueue, CommandQueueType> m_commandQueues;

		// -- Descriptor Heaps ---
		EnumArray<CpuDescriptorHeap, DescriptorHeapTypes> m_cpuDescriptorHeaps;
		std::array<GpuDescriptorHeap, 2> m_gpuDescriptorHeaps;

		std::array<EnumArray<Microsoft::WRL::ComPtr<ID3D12Fence>, CommandQueueType>, kBufferCount> m_frameFences;
		uint64_t m_frameCount = 0;

		std::atomic_uint32_t m_activeCmdCount = 0;
		BindlessDescriptorTable m_bindlessDescritorTable;
		// GpuRingAllocator m_tempPageAllocator;

		GpuTimerManager m_gpuTimerManager;

		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_emptyRootSignature;

		class CopyCtxManager
		{
		public:
			struct Ctx
			{
				Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Allocator = nullptr;
				Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList = nullptr;
				Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
				size_t FenceValue = 0;
				CompPtr<ID3D12Resource> UploadBuffer;
				size_t UploadBufferSize;
				void* MappedData = nullptr;
				inline bool IsValid() const { return CommandList != nullptr; }
				inline bool IsInValid() const { return CommandList == nullptr; }
				inline bool IsCompleted() const { return Fence->GetCompletedValue() >= FenceValue; }
			};

			void Initialize();
			void Finalize();

			Ctx Begin(size_t stagingSize);
			void Submit(Ctx uploadCtx);

		private:
			CompPtr<ID3D12CommandQueue> m_copyQueue;
			std::vector<Ctx> m_freeList;
			std::mutex m_mutex;
		} m_copyCtxManager;

	};
}