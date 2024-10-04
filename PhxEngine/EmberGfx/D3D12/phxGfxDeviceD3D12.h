#pragma once

#include "EmberGfx/phxHandlePool.h"

#include "phxGfxDescriptorHeapsD3D12.h"
#include "phxCommandCtxD3D12.h"
#include "d3d12ma/D3D12MemAlloc.h"
#include "phxMemory.h"
#include "phxGfxResourceRegistryD3D12.h"
#include "phxDynamicMemoryPageAllocatorD3D12.h"

#include <deque>
#include <mutex>
#include <functional>

#define SCOPED_LOCK(x) std::scoped_lock _(x)

namespace phx::gfx
{
	constexpr size_t kBufferCount = 3;
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
			const uint64_t currentIndex = SwapChain4->GetCurrentBackBufferIndex();
			return Rtv.GetCpuHandle(currentIndex);
		}
		ID3D12Resource* GetBackBuffer()
		{
			const uint64_t currentIndex = SwapChain4->GetCurrentBackBufferIndex();
			return BackBuffers[currentIndex].Get();
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

			NextFenceValue = Fence->GetCompletedValue() + 1;
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
			Queue->ExecuteCommandLists((UINT)commandLists.Size(), commandLists.begin());

			return IncrementFence();
		}

		void DiscardAllocators(uint64_t fenceValue, Span<ID3D12CommandAllocator*> allocators)
		{
			SCOPED_LOCK(MutexAllocation);
			for (ID3D12CommandAllocator* allocator : allocators)
			{
				AvailableAllocators.push_back(std::make_pair(fenceValue, allocator));
			}
		}

		uint64_t IncrementFence()
		{
			Queue->Signal(Fence.Get(), NextFenceValue);
			return NextFenceValue++;
		}

		uint64_t Submit()
		{
			const uint64_t fenceValue = SubmitCommandLists(m_pendingSubmitCommands);
			m_pendingSubmitCommands.clear();

			DiscardAllocators(fenceValue, m_pendingSubmitAllocators);
			m_pendingSubmitAllocators.clear();

			return fenceValue;
		}

		void EnqueueForSubmit(ID3D12CommandList* cmdList, ID3D12CommandAllocator* allocator)
		{
			m_pendingSubmitAllocators.push_back(allocator);
			m_pendingSubmitCommands.push_back(cmdList);
		}

		ID3D12CommandAllocator* RequestAllocator();
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

			void Release(UINT Index)
			{
				std::scoped_lock Guard(this->IndexMutex);
				IndexQueue.push_back(Index);
			}

			std::mutex IndexMutex;
			std::deque<DescriptorIndex> IndexQueue;
			size_t Index = 0;
		};

	private:
		DescriptorHeapAllocation m_allocation;
		DescriptorIndexPool m_descriptorIndexPool;
	};

	class GfxDeviceD3D12 final
	{
	public:
#if false
		inline static GfxDeviceD3D12* Instance() { return Singleton; }
#endif
	public:
		GfxDeviceD3D12();
		~GfxDeviceD3D12();

		static void Initialize(SwapChainDesc const& swapChainDesc, void* windowHandle = nullptr);
		static void Finalize();

		static void WaitForIdle();
		static void ResizeSwapChain(SwapChainDesc const& swapChainDesc);

		static platform::CommandCtxD3D12* BeginGfxContext();
		static platform::CommandCtxD3D12* BeginComputeContext();

		static void SubmitFrame();

	public:
		static GfxPipelineHandle CreateGfxPipeline(GfxPipelineDesc const& desc);
		static void DeleteResource(GfxPipelineHandle handle);

		static TextureHandle CreateTexture(TextureDesc const& desc);
		static void DeleteResource(TextureHandle handle);

		static InputLayoutHandle CreateInputLayout(Span<VertexAttributeDesc> desc);
		static void DeleteResource(InputLayoutHandle handle);

		static BufferHandle CreateBuffer(BufferDesc const& desc);
		static void DeleteResource(BufferHandle handle);

		static void DeleteResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource);

		static DescriptorIndex GetDescriptorIndex(TextureHandle handle, SubresouceType type = SubresouceType::SRV, int subResource = -1);

		static DescriptorIndex GetDescriptorIndex(BufferHandle handle, SubresouceType type = SubresouceType::SRV, int subResource = -1);

		// -- Platform specific ---
	public:
		static D3D12_CPU_DESCRIPTOR_HANDLE GetBackBufferView() { return m_swapChain.GetBackBufferView(); }
		static ID3D12Resource* GetBackBuffer() { return m_swapChain.GetBackBuffer(); }

		static ID3D12Device* GetD3D12Device() { return m_d3d12Device.Get(); }
		static ID3D12Device2* GetD3D12Device2() { return m_d3d12Device2.Get(); }
		static ID3D12Device5* GetD3D12Device5() { return m_d3d12Device5.Get(); }

		static IDXGIFactory6* GetDxgiFactory() { return m_factory.Get(); }
		static IDXGIAdapter* GetDxgiAdapter() { return m_gpuAdapter.NativeAdapter.Get(); }

		static D3D12CommandQueue& GetQueue(CommandQueueType type) { return m_commandQueues[type]; }
		static SpanMutable<D3D12CommandQueue> GetQueues() { return SpanMutable(m_commandQueues); }
		static D3D12CommandQueue& GetGfxQueue() { return m_commandQueues[CommandQueueType::Graphics]; }
		static D3D12CommandQueue& GetComputeQueue() { return m_commandQueues[CommandQueueType::Compute]; }
		static D3D12CommandQueue& GetCopyQueue() { return m_commandQueues[CommandQueueType::Copy]; }

		static CpuDescriptorHeap& GetResourceCpuHeap() { return m_cpuDescriptorHeaps[DescriptorHeapTypes::CBV_SRV_UAV]; }
		static CpuDescriptorHeap& GetRtvCpuHeap() { return m_cpuDescriptorHeaps[DescriptorHeapTypes::RTV]; }
		static CpuDescriptorHeap& GetDsvCpuHeap() { return m_cpuDescriptorHeaps[DescriptorHeapTypes::DSV]; }
		static Span<GpuDescriptorHeap> GetGpuDescriptorHeaps() { return Span<GpuDescriptorHeap>(m_gpuDescriptorHeaps.data(), m_gpuDescriptorHeaps.size()); }

		static ResourceRegistryD3D12& GetRegistry() { return m_resourceRegistry; }

		static void PollDebugMessages();

		static GpuRingAllocator* GetDynamicPageAllocator() { return &m_tempPageAllocator; }
	private:
		static void Initialize();
		static void InitializeD3D12Context(IDXGIAdapter* gpuAdapter);
		static void CreateSwapChain(SwapChainDesc const& desc, HWND hwnd);

		static platform::CommandCtxD3D12* BeginCommandRecording(CommandQueueType type);

		static void SubmitCommandLists();
		static void Present();
		static void RunGarbageCollection(uint64_t completedFrame = ~0ul);

		static int CreateSubresource(BufferHandle buffer, BufferDesc const& desc, SubresouceType subresourceType, size_t offset, size_t size = ~0u);
		static int CreateSubresource(TextureHandle texture, TextureDesc const& desc, SubresouceType subresourceType, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);

		static int CreateShaderResourceView(TextureHandle texture, TextureDesc const& desc, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);
		static int CreateRenderTargetView(TextureHandle texture, TextureDesc const& desc, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);
		static int CreateDepthStencilView(TextureHandle texture, TextureDesc const& desc, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);
		static int CreateUnorderedAccessView(TextureHandle texture, TextureDesc const& desc, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);

		static int CreateShaderResourceView(BufferHandle buffer, BufferDesc const& desc, size_t offset, size_t size);
		static int CreateUnorderedAccessView(BufferHandle buffer, BufferDesc const& desc, size_t offset, size_t size);

	private:
		// inline static GfxDeviceD3D12* Singleton = nullptr;

		inline static Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
		inline static Microsoft::WRL::ComPtr<ID3D12Device> m_d3d12Device;
		inline static Microsoft::WRL::ComPtr<ID3D12Device2> m_d3d12Device2;
		inline static Microsoft::WRL::ComPtr<ID3D12Device5> m_d3d12Device5;
		inline static Microsoft::WRL::ComPtr<D3D12MA::Allocator> m_d3d12MemAllocator;
		
		inline static D3D12Adapter m_gpuAdapter;
		inline static D3D12SwapChain m_swapChain;

		inline static D3D12_FEATURE_DATA_ROOT_SIGNATURE m_featureDataRootSignature = {};
		inline static D3D12_FEATURE_DATA_SHADER_MODEL   m_featureDataShaderModel = {};
		inline static ShaderModel m_minShaderModel = ShaderModel::SM_6_0;

		inline static bool m_isUnderGraphicsDebugger = false;
		inline static bool m_debugLayersEnabled = false;
		inline static gfx::DeviceCapability m_capabilities;

		// -- Command Queues ---
		inline static EnumArray<D3D12CommandQueue, CommandQueueType> m_commandQueues;

		// -- Descriptor Heaps ---
		inline static EnumArray<CpuDescriptorHeap, DescriptorHeapTypes> m_cpuDescriptorHeaps;
		inline static std::array<GpuDescriptorHeap, 2> m_gpuDescriptorHeaps;

		inline static std::array<EnumArray<Microsoft::WRL::ComPtr<ID3D12Fence>, CommandQueueType>, kBufferCount> m_frameFences;
		inline static uint64_t m_frameCount = 0;
		struct DeferredItem
		{
			uint64_t Frame;
			std::function<void()> DeferredFunc;
		};
		inline static std::deque<DeferredItem> m_deferredQueue;

		inline static std::atomic_uint32_t m_activeCmdCount = 0;
		inline static std::vector<std::unique_ptr<platform::CommandCtxD3D12>> m_commandPool;
		inline static ResourceRegistryD3D12 m_resourceRegistry;
		inline static BindlessDescriptorTable m_bindlessDescritorTable;
		inline static GpuRingAllocator m_tempPageAllocator;
	};

}

#if false

struct D3D12TimerQuery
{
	size_t BeginQueryIndex = 0;
	size_t EndQueryIndex = 0;

	// Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
	CommandQueue* CommandQueue = nullptr;
	uint64_t FenceCount = 0;

	bool Started = false;
	bool Resolved = false;
	Core::TimeStep Time;

};


struct D3D12Viewport
{
	ViewportDesc Desc;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> NativeSwapchain;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> NativeSwapchain4;

	std::vector<TextureHandle> BackBuffers;
	RenderPassHandle RenderPass;
};

struct D3D12Shader
{
	std::vector<uint8_t> ByteCode;
	const ShaderDesc Desc;
	Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> RootSignatureDeserializer;
	const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* RootSignatureDesc = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;

	D3D12Shader() = default;
	D3D12Shader(ShaderDesc const& desc, const void* binary, size_t binarySize)
		: Desc(desc)
	{
		ByteCode.resize(binarySize);
		std::memcpy(ByteCode.data(), binary, binarySize);
	}
};

struct D3D12InputLayout
{
	std::vector<VertexAttributeDesc> Attributes;
	std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements;

	D3D12InputLayout() = default;
};

struct D3D12GraphicsPipeline
{
	Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;
	GraphicsPipelineDesc Desc;

	D3D12GraphicsPipeline() = default;
};

struct D3D12ComputePipeline
{
	Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;
	ComputePipelineDesc Desc;

	D3D12ComputePipeline() = default;
};

struct D3D12MeshPipeline
{
	Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;
	MeshPipelineDesc Desc;

	D3D12MeshPipeline() = default;
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

struct D3D12CommandSignature final
{
	std::vector<D3D12_INDIRECT_ARGUMENT_DESC> D3D12Descs;
	Microsoft::WRL::ComPtr<ID3D12CommandSignature> NativeSignature;
};

struct D3D12Texture final
{
	TextureDesc Desc = {};
	Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;
	Microsoft::WRL::ComPtr<D3D12MA::Allocation> Allocation;

	// -- The views ---
	DescriptorView RtvAllocation;
	std::vector<DescriptorView> RtvSubresourcesAlloc = {};

	DescriptorView DsvAllocation;
	std::vector<DescriptorView> DsvSubresourcesAlloc = {};

	DescriptorView Srv;
	std::vector<DescriptorView> SrvSubresourcesAlloc = {};

	DescriptorView UavAllocation;
	std::vector<DescriptorView> UavSubresourcesAlloc = {};

	D3D12Texture() = default;

	void DisposeViews()
	{
		RtvAllocation.Allocation.Free();
		for (auto& view : RtvSubresourcesAlloc)
		{
			view.Allocation.Free();
		}
		RtvSubresourcesAlloc.clear();
		RtvAllocation = {};

		DsvAllocation.Allocation.Free();
		for (auto& view : DsvSubresourcesAlloc)
		{
			view.Allocation.Free();
		}
		DsvSubresourcesAlloc.clear();
		DsvAllocation = {};

		Srv.Allocation.Free();
		for (auto& view : SrvSubresourcesAlloc)
		{
			view.Allocation.Free();
		}
		SrvSubresourcesAlloc.clear();
		Srv = {};

		UavAllocation.Allocation.Free();
		for (auto& view : UavSubresourcesAlloc)
		{
			view.Allocation.Free();
		}
		UavSubresourcesAlloc.clear();
		UavAllocation = {};
	}
};

struct D3D12Buffer final
{
	BufferDesc Desc = {};
	Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;
	Microsoft::WRL::ComPtr<D3D12MA::Allocation> Allocation;

	void* MappedData = nullptr;
	uint32_t MappedSizeInBytes = 0;

	// -- Views ---
	DescriptorView Srv;
	std::vector<DescriptorView> SrvSubresourcesAlloc = {};

	DescriptorView UavAllocation;
	std::vector<DescriptorView> UavSubresourcesAlloc = {};


	D3D12_VERTEX_BUFFER_VIEW VertexView = {};
	D3D12_INDEX_BUFFER_VIEW IndexView = {};

	const BufferDesc& GetDesc() const { return Desc; }

	D3D12Buffer() = default;

	void DisposeViews()
	{
		Srv.Allocation.Free();
		for (auto& view : SrvSubresourcesAlloc)
		{
			view.Allocation.Free();
		}
		SrvSubresourcesAlloc.clear();
		Srv = {};

		UavAllocation.Allocation.Free();
		for (auto& view : UavSubresourcesAlloc)
		{
			view.Allocation.Free();
		}
		UavSubresourcesAlloc.clear();
		UavAllocation = {};
	}
};

struct D3D12RTAccelerationStructure final
{
	RTAccelerationStructureDesc Desc = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Dx12Desc = {};
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> Geometries;
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO Info = {};
	BufferHandle SratchBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;
	Microsoft::WRL::ComPtr<D3D12MA::Allocation> Allocation;
	DescriptorView Srv;

	D3D12RTAccelerationStructure() = default;
};

struct D3D12RenderPass final
{
	RenderPassDesc Desc = {};

	D3D12_RENDER_PASS_FLAGS D12RenderFlags = D3D12_RENDER_PASS_FLAG_NONE;

	size_t NumRenderTargets = 0;
	std::array<D3D12_RENDER_PASS_RENDER_TARGET_DESC, kNumConcurrentRenderTargets> RTVs = {};
	D3D12_RENDER_PASS_DEPTH_STENCIL_DESC DSV = {};

	std::vector<D3D12_RESOURCE_BARRIER> BarrierDescBegin;
	std::vector<D3D12_RESOURCE_BARRIER> BarrierDescEnd;

	D3D12RenderPass() = default;
};

#endif