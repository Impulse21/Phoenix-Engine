#include "pch.h"
#include "phxRHI_d3d12.h"


using namespace phx;
using namespace phx::rhi;
using namespace Microsoft::WRL;

#ifdef USING_D3D12_AGILITY_SDK
extern "C"
{
	// Used to enable the "Agility SDK" components
	__declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
	__declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}
#endif

#define SCOPE_LOCK(mutex) std::scoped_lock _(mutex)


namespace
{
	constexpr size_t MAX_BACK_BUFFER_COUNT = 3;
	constexpr D3D_FEATURE_LEVEL MIN_FEATURE_LEVEL = D3D_FEATURE_LEVEL_11_1;

	const GUID RenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
	const GUID PixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

	// helper function for texture subresource calculations
	// https://msdn.microsoft.com/en-us/library/windows/desktop/dn705766(v=vs.85).aspx
	uint32_t CalcSubresource(uint32_t mipSlice, uint32_t arraySlice, uint32_t planeSlice, uint32_t mipLevels, uint32_t arraySize)
	{
		return mipSlice + (arraySlice * mipLevels) + (planeSlice * mipLevels * arraySize);
	}


	// This method acquires the first available hardware adapter that supports Direct3D 12.
	// If no such adapter can be found, try WARP. Otherwise throw an exception.
	void FindAdapter(ComPtr<IDXGIFactory6> factory, IDXGIAdapter1** ppAdapter)
	{
		*ppAdapter = nullptr;
		size_t selectedGPUVideoMemeory = 0;
		ComPtr<IDXGIAdapter1> adapter;
		ComPtr<IDXGIAdapter1> selectedAdapter;
		for (UINT adapterIndex = 0;
			SUCCEEDED(factory->EnumAdapterByGpuPreference(
				adapterIndex,
				DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
				IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf())));
			adapterIndex++)
		{
			DXGI_ADAPTER_DESC1 desc;
			ThrowIfFailed(adapter->GetDesc1(&desc));

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				continue;
			}

			// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), MIN_FEATURE_LEVEL, __uuidof(ID3D12Device), nullptr)))
			{
				const size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
				//PHX_CORE_INFO("Direct3D Adapter (%u): VID:%04X, PID:%04X, VRAM:%zuMB - %ls", adapterIndex, desc.VendorId, desc.DeviceId, desc.DedicatedVideoMemory / (1024 * 1024), desc.Description);
#if false
#ifdef _DEBUG
				wchar_t buff[256] = {};
				swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X, VRAM:%zuMB - %ls\n",
					adapterIndex, desc.VendorId, desc.DeviceId, desc.DedicatedVideoMemory / (1024 * 1024), desc.Description);
				OutputDebugStringW(buff);
#endif
#endif
				if (dedicatedVideoMemory > selectedGPUVideoMemeory)
				{
					selectedGPUVideoMemeory = dedicatedVideoMemory;
					selectedAdapter = adapter;
				}
			}
		}

#if !defined(NDEBUG)
		if (!selectedAdapter)
		{
			// Try WARP12 instead
			if (FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()))))
			{
				throw std::runtime_error("WARP12 not available. Enable the 'Graphics Tools' optional feature");
			}

			PHX_CORE_INFO("Direct3D Adapter - WARP12");
			OutputDebugStringA("Direct3D Adapter - WARP12\n");
		}
#endif

		if (!selectedAdapter)
		{
			throw std::runtime_error("No Direct3D 12 device found");
		}

		*ppAdapter = selectedAdapter.Detach();
	}

	
}

namespace phx::rhi
{
#pragma region DeviceChildren

	class D3D12GfxDeviceChild
	{
	public:
		D3D12GfxDeviceChild(D3D12GfxDevice* gfxDevice)
			: m_gfxDevice(gfxDevice)
		{}

	protected:
		D3D12GfxDevice* m_gfxDevice;
	};

	class D3D12ResourceManager final : D3D12GfxDeviceChild
	{
	public:
		D3D12ResourceManager(D3D12GfxDevice* gfxDevice)
			: D3D12GfxDeviceChild(gfxDevice)
		{}

	private:
	};

	class CommandAllocatorPool : D3D12GfxDeviceChild
	{
	public:
		CommandAllocatorPool(D3D12GfxDevice* gfxDevice, D3D12_COMMAND_LIST_TYPE type)
			: D3D12GfxDeviceChild(gfxDevice)
			, m_type(type)
		{}

		ID3D12CommandAllocator* RequestAllocator(uint64_t completedFenceValue)
		{
			std::scoped_lock _(this->m_allocatonMutex);

			ID3D12CommandAllocator* pAllocator = nullptr;
			if (!this->m_availableAllocators.empty())
			{
				auto& allocatorPair = this->m_availableAllocators.front();
				if (allocatorPair.first <= completedFenceValue)
				{
					pAllocator = allocatorPair.second;
					ThrowIfFailed(
						pAllocator->Reset());

					this->m_availableAllocators.pop_front();
				}
			}

			if (!pAllocator)
			{
				Microsoft::WRL::ComPtr<ID3D12CommandAllocator> newAllocator;
				ThrowIfFailed(
					this->m_gfxDevice->GetD3D12Device2()->CreateCommandAllocator(
						this->m_type,
						IID_PPV_ARGS(&newAllocator)));

				// TODO: std::shared_ptr is leaky
				wchar_t allocatorName[32];
				swprintf(allocatorName, 32, L"CommandAllocator %zu", this->m_allocatorPool.size());
				newAllocator->SetName(allocatorName);
				this->m_allocatorPool.emplace_back(newAllocator);
				pAllocator = this->m_allocatorPool.back().Get();
			}

			return pAllocator;
		}

		void DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator)
		{
			std::scoped_lock _(this->m_allocatonMutex);

			assert(allocator);
			this->m_availableAllocators.push_back(std::make_pair(fenceValue, allocator));
		}
	private:
		const D3D12_COMMAND_LIST_TYPE m_type;
		std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_allocatorPool;
		std::deque<std::pair<uint64_t, ID3D12CommandAllocator*>> m_availableAllocators;
		std::mutex m_allocatonMutex;
	};

	struct D3D12CommandQueue : D3D12GfxDeviceChild
	{
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> Queue;
		Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
		Microsoft::WRL::Wrappers::Event FenceEvent;
		std::unique_ptr<CommandAllocatorPool> CmdAllocatorPool;
		D3D12_COMMAND_LIST_TYPE Type;

		D3D12CommandQueue(D3D12GfxDevice* gfxDevice, D3D12_COMMAND_LIST_TYPE type)
			: D3D12GfxDeviceChild(gfxDevice)
		{
			// Create Command Queue
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Type = this->Type;
			queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.NodeMask = 0;

			ThrowIfFailed(
				this->m_gfxDevice->GetD3D12Device()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&this->Queue)));

			switch (type)
			{
			case D3D12_COMMAND_LIST_TYPE_DIRECT:
				this->Queue->SetName(L"Direct Command Queue");
				break;
			case D3D12_COMMAND_LIST_TYPE_COPY:
				this->Queue->SetName(L"Copy Command Queue");
				break;
			case D3D12_COMMAND_LIST_TYPE_COMPUTE:
				this->Queue->SetName(L"Compute Command Queue");
				break;
			}

			// Create Fence
			ThrowIfFailed(
				this->m_gfxDevice->GetD3D12Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->Fence)));
			this->Fence->SetName(L"D3D12CommandQueue::Fence");

			FenceEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
			if (!this->FenceEvent.IsValid())
			{
				throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "CreateEventEx");
			}

			this->CmdAllocatorPool = std::make_unique<CommandAllocatorPool>(this->m_gfxDevice, this->Type);
		}
	};

	class DescriptorAllocator : D3D12GfxDeviceChild
	{
	public:
		DescriptorAllocator(D3D12GfxDevice* gfxDevice)
			: D3D12GfxDeviceChild(gfxDevice)
		{}

		void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptorsPerBlock)
		{
			this->m_heapDesc = {};
			this->m_heapDesc.Type = type;
			this->m_heapDesc.NumDescriptors = numDescriptorsPerBlock;
			this->m_descriptorSize = this->m_gfxDevice->GetD3D12Device()->GetDescriptorHandleIncrementSize(this->m_heapDesc.Type);
		}

		D3D12_CPU_DESCRIPTOR_HANDLE Allocate()
		{
			SCOPE_LOCK(this->m_mutex);

			if (this->m_freeList.empty())
				this->AllocateBlock();

			D3D12_CPU_DESCRIPTOR_HANDLE handle = this->m_freeList.back();
			this->m_freeList.pop_back();

			return handle;
		}
		void Free(D3D12_CPU_DESCRIPTOR_HANDLE handle)
		{
			SCOPE_LOCK(this->m_mutex);
			this->m_freeList.push_back(handle);
		}


	private:
		void AllocateBlock()
		{
			ComPtr<ID3D12DescriptorHeap>& heap = this->m_heaps.emplace_back();

			ThrowIfFailed(
				this->m_gfxDevice->GetD3D12Device()->CreateDescriptorHeap(
					&this->m_heapDesc,
					IID_PPV_ARGS(&heap)));

			D3D12_CPU_DESCRIPTOR_HANDLE heapStart = heap->GetCPUDescriptorHandleForHeapStart();
			for (UINT i = 0; i < this->m_heapDesc.NumDescriptors; ++i)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE handle = heapStart;
				handle.ptr += i * this->m_descriptorSize;
				this->m_freeList.push_back(handle);
			}
		}

	private:
		std::mutex m_mutex;
		UINT m_descriptorSize = 0;
		D3D12_DESCRIPTOR_HEAP_DESC m_heapDesc = {};
		std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> m_heaps;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_freeList;
	};

	struct DescritporHeapGpu : D3D12GfxDeviceChild
	{
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Heap;
		D3D12_CPU_DESCRIPTOR_HANDLE StartCpu = {};
		D3D12_GPU_DESCRIPTOR_HANDLE StartGpu = {};

		DescritporHeapGpu(D3D12GfxDevice* gfxDevice)
			: D3D12GfxDeviceChild(gfxDevice)
		{}

		void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors)
		{
			this->HeapDesc = {};
			this->HeapDesc.Type = type;
			this->HeapDesc.NumDescriptors = numDescriptors;
			this->HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			ThrowIfFailed(
				this->m_gfxDevice->GetD3D12Device()->CreateDescriptorHeap(
					&this->HeapDesc,
					IID_PPV_ARGS(&this->Heap)));

			this->StartCpu = this->Heap->GetCPUDescriptorHandleForHeapStart();
			this->StartGpu = this->Heap->GetGPUDescriptorHandleForHeapStart();
		}
	};

	struct SingleDescriptor
	{
		D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle = { 0 };
		D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle = { 0 };
		DescriptorIndex Index = kInvalidDescriptorIndex;
	};

	struct DescriptorAllocationHandlerDesc
	{
		UINT BlockSizeRes					= 4096u;
		UINT BlockSizeSampler				= 256u;
		UINT BlockSizeRTV					= 512u;
		UINT BlockSizeDsv					= 128u;
		UINT NumDescriptorsRes				= 1000000u; // tier 1 limit
		UINT NumBindlessDescriptorsRes		= 500000u;
		UINT NumDescriptorsSampler			= 2048;		// tier 1 limit
		UINT NumBindlessDescritorsSampler	= 256;
	};

	struct DescriptorAllocationHanlder : D3D12GfxDeviceChild
	{
		DescriptorAllocator AllocatorRes;
		DescriptorAllocator AllocatorSampler;
		DescriptorAllocator AllocatorRTV;
		DescriptorAllocator AllocatorDSV;

		std::vector<DescriptorIndex> FreeBindlessRes;
		std::vector<DescriptorIndex> FreeBindlessSam;

		DescritporHeapGpu GpuHeapRes;
		DescritporHeapGpu GpuHeapSampler;

		DescriptorAllocationHandlerDesc Desc = {};

		DescriptorAllocationHanlder(D3D12GfxDevice* gfxDevice, DescriptorAllocationHandlerDesc const& desc)
			: D3D12GfxDeviceChild(gfxDevice)
			, AllocatorRes(gfxDevice)
			, AllocatorSampler(gfxDevice)
			, AllocatorRTV(gfxDevice)
			, AllocatorDSV(gfxDevice)
			, GpuHeapRes(gfxDevice)
			, GpuHeapSampler(gfxDevice)
			, Desc(desc)
		{
			this->GpuHeapRes.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, desc.NumDescriptorsRes);
			this->GpuHeapSampler.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, desc.NumDescriptorsSampler);

			this->AllocatorRes.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, desc.BlockSizeRes);
			this->AllocatorSampler.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, desc.BlockSizeSampler);
			this->AllocatorRTV.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, desc.BlockSizeRTV);
			this->AllocatorDSV.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, desc.BlockSizeDsv);

			this->FreeBindlessRes.reserve(desc.NumBindlessDescriptorsRes);
			for (size_t i = 0; i < desc.NumBindlessDescriptorsRes; i++)
			{
				this->FreeBindlessRes.push_back(static_cast<DescriptorIndex>(desc.NumBindlessDescriptorsRes - i - 1));
			}

			this->FreeBindlessSam.reserve(desc.NumBindlessDescritorsSampler);
			for (size_t i = 0; i < desc.NumBindlessDescritorsSampler; i++)
			{
				this->FreeBindlessSam.push_back(static_cast<DescriptorIndex>(desc.NumBindlessDescritorsSampler - i - 1));
			}
		}
	};

#pragma endregion
}

#pragma region "GfxDevice Impl"
phx::rhi::D3D12GfxDevice::D3D12GfxDevice(Config const& config)
{
	D3D12GfxDevice::Ptr = this;

	this->CreateDevice(config);
	this->CreateDeviceResources(config);
}

phx::rhi::D3D12GfxDevice::~D3D12GfxDevice()
{
	D3D12GfxDevice::Ptr = nullptr;
}

void phx::rhi::D3D12GfxDevice::CreateDevice(Config const& config)
{
#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	//
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
		{
			debugController->EnableDebugLayer();
		}
		else
		{
			OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
		}

		ComPtr<ID3D12Debug3> debugController3;
		if (SUCCEEDED(debugController.As(&debugController3)))
		{
			debugController3->SetEnableGPUBasedValidation(false);
		}

		ComPtr<ID3D12Debug5> debugController5;
		if (SUCCEEDED(debugController.As(&debugController5)))
		{
			debugController5->SetEnableAutoName(true);
		}

		ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
		{
			this->m_dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

			DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
			{
				80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
			};
			DXGI_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
			filter.DenyList.pIDList = hide;
			dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
		}

	}
#endif

	ThrowIfFailed(
		CreateDXGIFactory2(this->m_dxgiFactoryFlags, IID_PPV_ARGS(this->m_dxgiFactory.ReleaseAndGetAddressOf())));


	// Determines whether tearing support is available for fullscreen borderless windows.
	if (config.AllowTearing)
	{
		BOOL allowTearing = FALSE;
		HRESULT hr = this->m_dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
		if (FAILED(hr) || !allowTearing)
		{
			PHX_CORE_WARN("Variable refresh rate displays not supported");
		}
	}

	FindAdapter(this->m_dxgiFactory, this->m_gpuAdapter.GetAddressOf());

	auto device = this->m_d3dDevice;
	// Create the DX12 API device object.
	HRESULT hr = D3D12CreateDevice(
		this->m_gpuAdapter.Get(),
		MIN_FEATURE_LEVEL,
		IID_PPV_ARGS(device.ReleaseAndGetAddressOf())
	);
	ThrowIfFailed(hr);

	#ifndef NDEBUG
	// Configure debug device (if active).
	ComPtr<ID3D12InfoQueue> d3dInfoQueue;
	if (SUCCEEDED(device.As(&d3dInfoQueue)))
	{
	#ifdef _DEBUG
		d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
	#endif
		D3D12_MESSAGE_ID hide[] =
		{
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
			// Workarounds for debug layer issues on hybrid-graphics systems
			D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
		};
		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
		filter.DenyList.pIDList = hide;
		d3dInfoQueue->AddStorageFilterEntries(&filter);
	}
	#endif
	Microsoft::WRL::ComPtr<IUnknown> renderdoc;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, RenderdocUUID, &renderdoc)))
	{
		this->m_isUnderGraphicsDebugger |= !!renderdoc;
	}

	Microsoft::WRL::ComPtr<IUnknown> pix;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, PixUUID, &pix)))
	{
		this->m_isUnderGraphicsDebugger |= !!pix;
	}

	// -- Get device capabilities ---
	DeviceCapabilities& capabilites = this->m_capabilities;
	D3D12_FEATURE_DATA_D3D12_OPTIONS featureOpptions = {};
	bool hasOptions = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOpptions, sizeof(featureOpptions)));

	if (hasOptions)
	{
		capabilites.RTVTArrayIndexWithoutGS = featureOpptions.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation;
	}

	// TODO: Move to acability array
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport5 = {};
	bool hasOptions5 = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport5, sizeof(featureSupport5)));

	if (SUCCEEDED(device.As(&this->m_d3dDevice5)) && hasOptions5)
	{
		capabilites.RayTracing = featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0;
		capabilites.RenderPass = featureSupport5.RenderPassesTier >= D3D12_RENDER_PASS_TIER_0;
		capabilites.RayQuery = featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1;
	}


	D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureSupport6 = {};
	bool hasOptions6 = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureSupport6, sizeof(featureSupport6)));

	if (hasOptions6)
	{
		capabilites.VariableRateShading = featureSupport6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2;
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupport7 = {};
	bool hasOptions7 = SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureSupport7, sizeof(featureSupport7)));

	if (SUCCEEDED(device.As(&this->m_d3dDevice2)) && hasOptions7)
	{
		capabilites.CreateNoteZeroed = true;
		capabilites.MeshShading = featureSupport7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1;
	}

	// Determine maximum supported feature level for this device
	static const D3D_FEATURE_LEVEL s_featureLevels[] =
	{
	#if defined(NTDDI_WIN10_FE) || defined(USING_D3D12_AGILITY_SDK)
			D3D_FEATURE_LEVEL_12_2,
	#endif
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
	};

	D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels =
	{
		static_cast<UINT>(std::size(s_featureLevels)), s_featureLevels, D3D_FEATURE_LEVEL_11_0
	};

	hr = device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
	if (SUCCEEDED(hr))
	{
		this->m_d3dFeatureLevel = featLevels.MaxSupportedFeatureLevel;
	}
	else
	{
		this->m_d3dMinFeatureLevel;
	}

	this->m_featureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(
		this->m_d3dDevice2->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &this->m_featureDataRootSignature, sizeof(this->m_featureDataRootSignature))))
	{
		this->m_featureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Check shader model support
	this->m_featureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
	if (FAILED(
		this->m_d3dDevice2->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &this->m_featureDataShaderModel, sizeof(this->m_featureDataShaderModel.HighestShaderModel))))
	{
		this->m_featureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_5;
		this->m_minShaderModel = ShaderModel::SM_6_5;
	}
}

void phx::rhi::D3D12GfxDevice::CreateDeviceResources(Config const&)
{
	this->m_queues[CommandQueueType::Graphics] = std::make_unique<D3D12CommandQueue>(this, D3D12_COMMAND_LIST_TYPE_DIRECT);
	this->m_queues[CommandQueueType::Compute] = std::make_unique<D3D12CommandQueue>(this, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	this->m_queues[CommandQueueType::Copy] = std::make_unique<D3D12CommandQueue>(this, D3D12_COMMAND_LIST_TYPE_COPY);

	// Create Heaps
	DescriptorAllocationHandlerDesc desc = {};
	this->m_descriptorAllocator = std::make_shared<DescriptorAllocationHanlder>(this, desc);

	// Create Allocator
	D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
	allocatorDesc.pDevice = this->m_d3dDevice.Get();
	allocatorDesc.pAdapter = this->m_gpuAdapter.Get();
	//allocatorDesc.PreferredBlockSize = 256 * 1024 * 1024;
	//allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_ALWAYS_COMMITTED;
	allocatorDesc.Flags = (D3D12MA::ALLOCATOR_FLAGS)(D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED);

	ThrowIfFailed(
		D3D12MA::CreateAllocator(&allocatorDesc, &this->m_gpuMemAllocator));

	// Create Command Lists
}

#pragma endregion
