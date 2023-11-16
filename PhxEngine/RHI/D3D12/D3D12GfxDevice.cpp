#include "D3D12GfxDevice.h"
#include <Core/Log.h>
#include <Core/String.h>
#include "DxgiFormatMapping.h"

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;

static const GUID RenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
static const GUID PixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

// DirectX Aligily SDK
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 608; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }
// https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-support


// Teir 1 limit is 1,000,000
// https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
#define TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE 1000000

#define NUM_BINDLESS_RESOURCES TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE / 2


namespace
{
	const bool sDebugEnabled = IsDebuggerPresent();
	DWORD CallbackCookie;

	// Define a global function to handle the messages
	void CALLBACK DebugCallback(D3D12_MESSAGE_CATEGORY Category, D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID ID, LPCSTR pDescription, void* pContext)
	{
		switch (Severity)
		{
		case D3D12_MESSAGE_SEVERITY_MESSAGE:
		case D3D12_MESSAGE_SEVERITY_INFO:
			LOG_RHI_INFO(pDescription);
			break;
		case D3D12_MESSAGE_SEVERITY_WARNING:
			LOG_RHI_WARN(pDescription);
			break;
		case D3D12_MESSAGE_SEVERITY_ERROR:
		case D3D12_MESSAGE_SEVERITY_CORRUPTION:
			LOG_RHI_ERROR(pDescription);
			break;
		}
	}

	bool SafeTestD3D12CreateDevice(IDXGIAdapter* adapter, D3D_FEATURE_LEVEL minFeatureLevel, D3D12DeviceBasicInfo& outInfo)
	{
#pragma warning(disable:6322)
		try
		{
			ID3D12Device* Device = nullptr;
			const HRESULT d3D12CreateDeviceResult = D3D12CreateDevice(adapter, minFeatureLevel, IID_PPV_ARGS(&Device));
			if (SUCCEEDED(d3D12CreateDeviceResult))
			{
				outInfo.NumDeviceNodes = Device->GetNodeCount();

				Device->Release();
				return true;
			}
			else
			{
				LOG_RHI_WARN("D3D12CreateDevice failed.");
			}
		}
		catch (...)
		{
		}
#pragma warning(default:6322)

		return {};
	}

	inline D3D12_RESOURCE_STATES ConvertResourceStates(RHI::ResourceStates stateBits)
	{
		if (stateBits == ResourceStates::Common)
			return D3D12_RESOURCE_STATE_COMMON;

		D3D12_RESOURCE_STATES result = D3D12_RESOURCE_STATE_COMMON; // also 0

		if ((stateBits & ResourceStates::ConstantBuffer) != 0) result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if ((stateBits & ResourceStates::VertexBuffer) != 0) result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if ((stateBits & ResourceStates::IndexGpuBuffer) != 0) result |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
		if ((stateBits & ResourceStates::IndirectArgument) != 0) result |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		if ((stateBits & ResourceStates::ShaderResource) != 0) result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		if ((stateBits & ResourceStates::UnorderedAccess) != 0) result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		if ((stateBits & ResourceStates::RenderTarget) != 0) result |= D3D12_RESOURCE_STATE_RENDER_TARGET;
		if ((stateBits & ResourceStates::DepthWrite) != 0) result |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
		if ((stateBits & ResourceStates::DepthRead) != 0) result |= D3D12_RESOURCE_STATE_DEPTH_READ;
		if ((stateBits & ResourceStates::StreamOut) != 0) result |= D3D12_RESOURCE_STATE_STREAM_OUT;
		if ((stateBits & ResourceStates::CopyDest) != 0) result |= D3D12_RESOURCE_STATE_COPY_DEST;
		if ((stateBits & ResourceStates::CopySource) != 0) result |= D3D12_RESOURCE_STATE_COPY_SOURCE;
		if ((stateBits & ResourceStates::ResolveDest) != 0) result |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
		if ((stateBits & ResourceStates::ResolveSource) != 0) result |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
		if ((stateBits & ResourceStates::Present) != 0) result |= D3D12_RESOURCE_STATE_PRESENT;
		if ((stateBits & ResourceStates::AccelStructRead) != 0) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		if ((stateBits & ResourceStates::AccelStructWrite) != 0) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		if ((stateBits & ResourceStates::AccelStructBuildInput) != 0) result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		if ((stateBits & ResourceStates::AccelStructBuildBlas) != 0) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		if ((stateBits & ResourceStates::ShadingRateSurface) != 0) result |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
		if ((stateBits & ResourceStates::GenericRead) != 0) result |= D3D12_RESOURCE_STATE_GENERIC_READ;
		if ((stateBits & ResourceStates::ShaderResourceNonPixel) != 0) result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

		return result;
	}

	constexpr D3D12_SUBRESOURCE_DATA _ConvertSubresourceData(const SubresourceData& pInitialData)
	{
		D3D12_SUBRESOURCE_DATA data = {};
		data.pData = pInitialData.pData;
		data.RowPitch = pInitialData.rowPitch;
		data.SlicePitch = pInitialData.slicePitch;

		return data;
	}

	Core::RefCountPtr<IDXGIFactory6> CreateDXGIFactory6()
	{
		uint32_t flags = 0;

		if (sDebugEnabled)
		{
			Core::RefCountPtr<ID3D12Debug> debugController;
			ThrowIfFailed(
				D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));

			debugController->EnableDebugLayer();
			flags = DXGI_CREATE_FACTORY_DEBUG;
			// Set up the message callback function
			Core::RefCountPtr<ID3D12Debug3> debugController3;
			if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController3))))
			{
				debugController3->SetEnableGPUBasedValidation(false);
				debugController3->SetEnableSynchronizedCommandQueueValidation(true);
			}

			Core::RefCountPtr<ID3D12Debug5> debugController5;
			if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController5))))
			{
				debugController5->SetEnableAutoName(true);
			}
		}

		Core::RefCountPtr<IDXGIFactory6> factory;
		ThrowIfFailed(
			CreateDXGIFactory2(flags, IID_PPV_ARGS(factory.ReleaseAndGetAddressOf())));

		return factory;
	}
}

bool PhxEngine::RHI::D3D12::D3D12GfxDevice::Initialize()
{
	m_FrameCount = 0;
	m_BufferCount = 3;
	D3D12Adapter selectedAdapter = {};
	{
		Core::RefCountPtr<IDXGIFactory6> factory;
		ThrowIfFailed(
			CreateDXGIFactory2(0, IID_PPV_ARGS(factory.ReleaseAndGetAddressOf())));

		Core::RefCountPtr<IDXGIAdapter1> tempAdapter;
		for (uint32_t adapterIndex = 0; EnumAdapters(adapterIndex, factory.Get(), tempAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
		{
			if (!tempAdapter)
			{
				continue;
			}

			DXGI_ADAPTER_DESC1 desc = {};
			tempAdapter->GetDesc1(&desc);

			std::string name;
			Core::StringConvert(desc.Description, name);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				LOG_RHI_INFO("GPU '%s' is a software adapter. Skipping consideration as this is not supported.", name.c_str());
				continue;
			}

			D3D12DeviceBasicInfo basicDeviceInfo = {};
			if (!SafeTestD3D12CreateDevice(tempAdapter.Get(), D3D_FEATURE_LEVEL_11_1, basicDeviceInfo))
			{
				continue;
			}

			if (basicDeviceInfo.NumDeviceNodes > 1)
			{
				LOG_RHI_INFO("GPU '%s' has one or more device nodes. Currently only support one device ndoe.", name.c_str());
			}

			if (!selectedAdapter.NativeAdapter || selectedAdapter.DedicatedVideoMemory < desc.DedicatedVideoMemory)
			{
				selectedAdapter.Name = name;
				selectedAdapter.NativeAdapter = tempAdapter;
				selectedAdapter.DedicatedVideoMemory = desc.DedicatedVideoMemory;
				selectedAdapter.DedicatedVideoMemory = desc.DedicatedVideoMemory;
				selectedAdapter.DedicatedSystemMemory = desc.DedicatedSystemMemory;
				selectedAdapter.SharedSystemMemory = desc.SharedSystemMemory;
				selectedAdapter.BasicDeviceInfo = basicDeviceInfo;
			}
		}

		if (!selectedAdapter.NativeAdapter)
		{
			LOG_RHI_ERROR("No suitable adapters were found - Unable to initialize RHI.");
			return {};
		}
	}

	LOG_RHI_INFO(
		"Found Suitable D3D12 Adapter '%s'",
		selectedAdapter.Name.c_str());

	LOG_RHI_INFO(
		"Adapter has %dMB of dedicated video memory, %dMB of dedicated system memory, and %dMB of shared system memory.",
		selectedAdapter.DedicatedVideoMemory / (1024 * 1024),
		selectedAdapter.DedicatedSystemMemory / (1024 * 1024),
		selectedAdapter.SharedSystemMemory / (1024 * 1024));


	m_GpuAdapter = selectedAdapter;

	InitializeDeviceResources();

	// Create Frame Fences
	for (size_t q = 0; q < FrameFences.size(); q++)
	{
		// Create a frame fence per queue;
		ThrowIfFailed(
			D3d12Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&FrameFences[q])));
	}

	return true;
}

void D3D12GfxDevice::InitializeDeviceResources()
{
	m_DxgiFctory6 = CreateDXGIFactory6();

	ThrowIfFailed(
		D3D12CreateDevice(
			m_GpuAdapter.NativeAdapter,
			D3D_FEATURE_LEVEL_11_1,
			IID_PPV_ARGS(&m_D3d12Device)));

	m_D3d12Device->SetName(L"RHI::D3D12::D3D12Device");
	{
		// Ref counter isn't working so use ComPtr for this - to lazy to fight with
		// this right now.
		Microsoft::WRL::ComPtr<IUnknown> renderdoc;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, RenderdocUUID, &renderdoc)))
		{
			m_IsUnderGraphicsDebugger |= !!renderdoc;
		}

		Microsoft::WRL::ComPtr<IUnknown> pix;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, PixUUID, &pix)))
		{
			m_IsUnderGraphicsDebugger |= !!pix;
		}
	}

	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS featureOpptions = {};

		bool hasOptions = SUCCEEDED(m_D3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOpptions, sizeof(featureOpptions)));

		if (hasOptions)
		{
			if (featureOpptions.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation)
			{
				m_RhiCapabilities |= RHICapability::RT_VT_ArrayIndex_Without_GS;
			}
		}

		// TODO: Move to acability array
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport5 = {};
		bool hasOptions5 = SUCCEEDED(m_D3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport5, sizeof(featureSupport5)));

		if (SUCCEEDED(m_D3d12Device->QueryInterface(IID_PPV_ARGS(&m_D3d12Device5))) && hasOptions5)
		{
			if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
			{
				m_RhiCapabilities |= RHICapability::RayTracing;
			}
			if (featureSupport5.RenderPassesTier >= D3D12_RENDER_PASS_TIER_0)
			{
				m_RhiCapabilities |= RHICapability::RenderPass;
			}
			if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1)
			{
				m_RhiCapabilities |= RHICapability::RayQuery;
			}
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureSupport6 = {};
		bool hasOptions6 = SUCCEEDED(m_D3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureSupport6, sizeof(featureSupport6)));
		if (hasOptions6)
		{
			if (featureSupport6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
			{
				m_RhiCapabilities |= RHICapability::VariableRateShading;
			}
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupport7 = {};
		bool hasOptions7 = SUCCEEDED(m_D3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureSupport7, sizeof(featureSupport7)));

		if (SUCCEEDED(m_D3d12Device->QueryInterface(&m_D3d12Device2)) && hasOptions7)
		{
			if (featureSupport7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1)
			{
				m_RhiCapabilities |= RHICapability::MeshShading;
			}
			m_RhiCapabilities |= RHICapability::CreateNoteZeroed;
		}

		m_FeatureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(m_D3d12Device2->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &m_FeatureDataRootSignature, sizeof(m_FeatureDataRootSignature))))
		{
			m_FeatureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		// Check shader model support
		m_FeatureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
		m_MinShaderModel = ShaderModel::SM_6_6;
		if (FAILED(m_D3d12Device2->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &m_FeatureDataShaderModel, sizeof(m_FeatureDataShaderModel))))
		{
			m_FeatureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_5;
			m_MinShaderModel = ShaderModel::SM_6_5;
		}
	}

	if (sDebugEnabled)
	{
		RefCountPtr<ID3D12InfoQueue1> infoQueue;
		if (SUCCEEDED(m_D3d12Device->QueryInterface<ID3D12InfoQueue1>(&infoQueue)))
		{
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
			infoQueue->RegisterMessageCallback(DebugCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &CallbackCookie);

			D3D12_MESSAGE_SEVERITY severities[] =
			{
				D3D12_MESSAGE_SEVERITY_INFO,
			};

			D3D12_MESSAGE_ID denyIds[] =
			{
				// This occurs when there are uninitialized descriptors in a descriptor table, even when a
				// shader does not access the missing descriptors.  I find this is common when switching
				// shader permutations and not wanting to change much code to reorder resources.
				D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,

				// Triggered when a shader does not export all color components of a render target, such as
				// when only writing RGB to an R10G10B10A2 buffer, ignoring alpha.
				D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_PS_OUTPUT_RT_OUTPUT_MISMATCH,

				// This occurs when a descriptor table is unbound even when a shader does not access the missing
				// descriptors.  This is common with a root signature shared between disparate shaders that
				// don't all need the same types of resources.
				D3D12_MESSAGE_ID_COMMAND_LIST_DESCRIPTOR_TABLE_NOT_SET,

				// RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS
				(D3D12_MESSAGE_ID)1008,
			};

			D3D12_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumSeverities = static_cast<UINT>(std::size(severities));
			filter.DenyList.pSeverityList = severities;
			filter.DenyList.NumIDs = static_cast<UINT>(std::size(denyIds));
			filter.DenyList.pIDList = denyIds;
			infoQueue->PushStorageFilter(&filter);
		}
	}

	{
		D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
		allocatorDesc.pDevice = m_D3d12Device;
		allocatorDesc.pAdapter = m_GpuAdapter.NativeAdapter;
		//allocatorDesc.PreferredBlockSize = 256 * 1024 * 1024;
		//allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_ALWAYS_COMMITTED;
		allocatorDesc.Flags = (D3D12MA::ALLOCATOR_FLAGS)(D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED);

		ThrowIfFailed(
			D3D12MA::CreateAllocator(&allocatorDesc, &m_D3d12MemAllocator));
	}

#ifdef ENABLE_DIRECT_STORAGE
	{

		ThrowIfFailed(
			DStorageGetFactory(IID_PPV_ARGS(&m_DStorageFactory)));

		DSTORAGE_QUEUE_DESC QueueDesc = {
			.SourceType = DSTORAGE_REQUEST_SOURCE_FILE,
			.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY,
			.Priority = DSTORAGE_PRIORITY_NORMAL,
			.Name = "DStorageQueue: File",
			.Device = m_D3d12Device.Get()
		};
		ThrowIfFailed(
			m_DStorageFactory->CreateQueue(&QueueDesc, IID_PPV_ARGS(&m_DStorageQueues[DSTORAGE_REQUEST_SOURCE_FILE])));

		QueueDesc = {
			.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY,
			.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY,
			.Priority = DSTORAGE_PRIORITY_NORMAL,
			.Name = "DStorageQueue: Memory",
			.Device = m_D3d12Device.Get()
		};

		ThrowIfFailed(
			m_DStorageFactory->CreateQueue(&QueueDesc, IID_PPV_ARGS(&m_DStorageQueues[DSTORAGE_REQUEST_SOURCE_MEMORY])));

		ThrowIfFailed(
			m_D3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_DStorageFence)));
	}
#endif
	m_Queues[(size_t)RHI::CommandListType::Graphics].Initialize(m_D3d12Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_Queues[(size_t)RHI::CommandListType::Copy].Initialize(m_D3d12Device, D3D12_COMMAND_LIST_TYPE_COPY);
	m_Queues[(size_t)RHI::CommandListType::Compute].Initialize(m_D3d12Device, D3D12_COMMAND_LIST_TYPE_COMPUTE);

	// Create Descriptor Heaps
	m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV].Initialize(
		1024,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler].Initialize(
		1024,
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::RTV].Initialize(
		1024,
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::DSV].Initialize(
		1024,
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV].Initialize(
		NUM_BINDLESS_RESOURCES,
		TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE - NUM_BINDLESS_RESOURCES,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

	m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler].Initialize(
		10,
		100,
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
}

bool D3D12GfxDevice::Finalize()
{
	assert(false, "PROCESS DELETE QUEUE PLEASE");
	RefCountPtr<ID3D12InfoQueue1> infoQueue;
	if (SUCCEEDED(m_D3d12Device->QueryInterface<ID3D12InfoQueue1>(&infoQueue)))
	{
		infoQueue->UnregisterMessageCallback(CallbackCookie);
	}
	m_Queues[(size_t)RHI::CommandListType::Graphics].Finalize();
	m_Queues[(size_t)RHI::CommandListType::Copy].Finalize();
	m_Queues[(size_t)RHI::CommandListType::Compute].Finalize();

	return true;
}

void D3D12GfxDevice::WaitForIdle()
{
	for (size_t i = 0; i < (size_t)CommandListType::Count; i++)
	{
		m_Queues[i].WaitForIdle();
	}
}

GpuMemoryUsage D3D12GfxDevice::GetMemoryUsage()
{
	D3D12MA::Budget budget;
	m_D3d12MemAllocator->GetBudget(&budget, nullptr);

	GpuMemoryUsage retval;
	retval.Budget = budget.BudgetBytes;
	retval.Usage = budget.UsageBytes;

	return retval;
}

void PhxEngine::RHI::D3D12::D3D12GfxDevice::Present(SwapChain const& swapchainToPresent)
{
	// -- Mark Queues for completion ---
	for (size_t q = 0; q < (size_t)CommandListType::Count; ++q)
	{
		D3D12CommandQueue& queue = Queue(static_cast<CommandListType>(q));

		// Single Frame Fence to the frame it's currently doing work for.
		queue.GetD3D12CommandQueue()->Signal(FrameFences[q].Get(), m_FrameCount);
	}

	// -- Present SwapChain ---
	{
		UINT presentFlags = 0;
		const bool enableVSync = swapchainToPresent.Desc().VSync;

		if (!enableVSync && !swapchainToPresent.Desc().Fullscreen)
		{
			presentFlags = DXGI_PRESENT_ALLOW_TEARING;
		}
		const D3D12SwapChain& platformSwapChain = swapchainToPresent.Platform();
		HRESULT hr = platformSwapChain.NativeSwapchain4->Present((UINT)enableVSync, presentFlags);

		// If the device was reset we must completely reinitialize the renderer.
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{

#ifdef _DEBUG
			char buff[64] = {};
			sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
				static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED)
					? D3d12Device()->GetDeviceRemovedReason()
					: hr));

			LOG_RHI_ERROR(buff);
#endif

			// TODO: Handle device lost
			// HandleDeviceLost();
		}
	}
	// Begin the next frame - this affects the GetCurrentBackBufferIndex()
	m_FrameCount++;

	// -- Wait for next frame ---
	{
		for (size_t q = 0; q < (size_t)CommandListType::Count; ++q)
		{
			const UINT64 completedFrame = FrameFences[q]->GetCompletedValue();

			// If the fence is max uint64, that might indicate that the device has been removed as fences get reset in this case
			assert(completedFrame != UINT64_MAX);
			const uint64_t bufferCount = m_BufferCount;

			// Since our frame count is 1 based rather then 0, increment the number of buffers by 1 so we don't have to wait on the first 3 frames
			// that are kicked off.
			if (m_FrameCount >= (bufferCount + 1) && completedFrame < m_FrameCount)
			{
				// Wait on the frames last value?
				// NULL event handle will sswapChainy wait immediately:
				//	https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-seteventoncompletion#remarks
				ThrowIfFailed(
					FrameFences[q]->SetEventOnCompletion(m_FrameCount - bufferCount, nullptr));
			}
		}
	}
}

bool PhxEngine::RHI::D3D12::D3D12GfxDevice::CreateSwapChain(SwapChainDesc const& desc, void* windowHandle, D3D12SwapChain& swapChain)
{
	const auto& formatMapping = GetDxgiFormatMapping(desc.Format);
	if (swapChain.NativeSwapchain)
	{
		WaitForIdle();

		UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		ThrowIfFailed(
			swapChain.NativeSwapchain->ResizeBuffers(
				m_BufferCount,
				desc.Width,
				desc.Height,
				formatMapping.RtvFormat,
				swapChainFlags));
	}
	else
	{
		HRESULT hr;
		UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		// Create swapchain:
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = desc.Width;
		swapChainDesc.Height = desc.Height;
		swapChainDesc.Format = formatMapping.RtvFormat;
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = m_BufferCount;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Flags = swapChainFlags;

		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
		fullscreenDesc.Windowed = !desc.Fullscreen;

		hr = m_DxgiFctory6->CreateSwapChainForHwnd(
			Queue(CommandListType::Graphics).GetD3D12CommandQueue(),
			static_cast<HWND>(windowHandle),
			&swapChainDesc,
			&fullscreenDesc,
			nullptr,
			swapChain.NativeSwapchain.GetAddressOf()
		);

		if (FAILED(hr))
		{
			return false;
		}

		if (FAILED(swapChain.NativeSwapchain->QueryInterface(IID_PPV_ARGS(&swapChain.NativeSwapchain4))))
		{
			return false;
		}
	}

	// swapChain.Release();
	swapChain.BackBuffers.resize(m_BufferCount);
	swapChain.BackBuferViews.resize(m_BufferCount);

	assert(false); // Deferred Release
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = D3D12::GetDxgiFormatMapping(desc.Format).RtvFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	for (int i = 0; i < swapChain.BackBuffers.size(); i++)
	{
		ThrowIfFailed(
			swapChain.NativeSwapchain4->GetBuffer(i, IID_PPV_ARGS(&swapChain.BackBuffers[i])));

		D3D12Descriptor& descriptor = swapChain.BackBuferViews[i];
		descriptor.Allocation = CpuRenderTargetHeap().Allocate(1);

		m_D3d12Device->CreateRenderTargetView(swapChain.BackBuffers[i], &rtvDesc, descriptor.GetView());
	}

	return true;
}

bool PhxEngine::RHI::D3D12::D3D12GfxDevice::CreateTexture(TextureDesc const& desc, D3D12Texture& texture)
{
	D3D12_CLEAR_VALUE d3d12OptimizedClearValue = {};
	d3d12OptimizedClearValue.Color[0] = desc.OptmizedClearValue.Colour.R;
	d3d12OptimizedClearValue.Color[1] = desc.OptmizedClearValue.Colour.G;
	d3d12OptimizedClearValue.Color[2] = desc.OptmizedClearValue.Colour.B;
	d3d12OptimizedClearValue.Color[3] = desc.OptmizedClearValue.Colour.A;
	d3d12OptimizedClearValue.DepthStencil.Depth = desc.OptmizedClearValue.DepthStencil.Depth;
	d3d12OptimizedClearValue.DepthStencil.Stencil = desc.OptmizedClearValue.DepthStencil.Stencil;

	auto dxgiFormatMapping = GetDxgiFormatMapping(desc.Format);
	d3d12OptimizedClearValue.Format = dxgiFormatMapping.RtvFormat;

	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;

	if ((desc.BindingFlags & BindingFlags::DepthStencil) == BindingFlags::DepthStencil)
	{
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		if ((desc.BindingFlags & BindingFlags::ShaderResource) != BindingFlags::ShaderResource)
		{
			resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}
	}
	if ((desc.BindingFlags & BindingFlags::RenderTarget) == BindingFlags::RenderTarget)
	{
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}
	if ((desc.BindingFlags & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess)
	{
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	CD3DX12_RESOURCE_DESC resourceDesc = {};

	const bool isTypeless = (desc.MiscFlags | RHI::TextureMiscFlags::Typeless) == RHI::TextureMiscFlags::Typeless;
	switch (desc.Dimension)
	{
	case TextureDimension::Texture1D:
	case TextureDimension::Texture1DArray:
	{
		resourceDesc =
			CD3DX12_RESOURCE_DESC::Tex1D(
				isTypeless ? dxgiFormatMapping.ResourceFormat : dxgiFormatMapping.RtvFormat,
				desc.Width,
				desc.ArraySize,
				desc.MipLevels,
				resourceFlags);
		break;
	}
	case TextureDimension::Texture2D:
	case TextureDimension::Texture2DArray:
	case TextureDimension::TextureCube:
	case TextureDimension::TextureCubeArray:
	case TextureDimension::Texture2DMS:
	case TextureDimension::Texture2DMSArray:
	{
		resourceDesc =
			CD3DX12_RESOURCE_DESC::Tex2D(
				isTypeless ? dxgiFormatMapping.ResourceFormat : dxgiFormatMapping.RtvFormat,
				desc.Width,
				desc.Height,
				desc.ArraySize,
				desc.MipLevels,
				1,
				0,
				resourceFlags);
		break;
	}
	case TextureDimension::Texture3D:
	{
		resourceDesc =
			CD3DX12_RESOURCE_DESC::Tex3D(
				isTypeless ? dxgiFormatMapping.ResourceFormat : dxgiFormatMapping.RtvFormat,
				desc.Width,
				desc.Height,
				desc.ArraySize,
				desc.MipLevels,
				resourceFlags);
		break;
	}
	default:
		throw std::runtime_error("Unsupported texture dimension");
	}

	const bool useClearValue =
		((desc.BindingFlags & BindingFlags::RenderTarget) == BindingFlags::RenderTarget) ||
		((desc.BindingFlags & BindingFlags::DepthStencil) == BindingFlags::DepthStencil);

	texture.TotalSize = 0;
	texture.Footprints.resize(desc.ArraySize * std::max(uint16_t(1u), desc.MipLevels));
	texture.RowSizesInBytes.resize(texture.Footprints.size());
	texture.NumRows.resize(texture.Footprints.size());
	m_D3d12Device->GetCopyableFootprints(
		&resourceDesc,
		0,
		(UINT)texture.Footprints.size(),
		0,
		texture.Footprints.data(),
		texture.NumRows.data(),
		texture.RowSizesInBytes.data(),
		&texture.TotalSize
	);

	ThrowIfFailed(
		m_D3d12MemAllocator->CreateResource(
			&allocationDesc,
			&resourceDesc,
			ConvertResourceStates(desc.InitialState),
			useClearValue ? &d3d12OptimizedClearValue : nullptr,
			&texture.Allocation,
			IID_PPV_ARGS(&texture.D3D12Resource)));


	std::wstring debugName;
	Core::StringConvert(desc.DebugName, debugName);
	texture.D3D12Resource->SetName(debugName.c_str());

	// Create Data
	return true;
}

D3D12Adapter D3D12GfxDevice::GpuAdapter()
{
	return m_GpuAdapter;
}

Core::RefCountPtr<IDXGIFactory6> D3D12GfxDevice::DxgiFctory6()
{
	return m_DxgiFctory6;
}

Core::RefCountPtr<ID3D12Device> D3D12GfxDevice::D3d12Device()
{
	return m_D3d12Device;
}

Core::RefCountPtr<ID3D12Device2> D3D12GfxDevice::D3d12Device2()
{
	return m_D3d12Device2;
}

Core::RefCountPtr<ID3D12Device5> D3D12GfxDevice::D3d12Device5()
{
	return m_D3d12Device5;
}

D3D12CommandQueue& D3D12GfxDevice::Queue(RHI::CommandListType type)
{
	return m_Queues[(size_t)type];
}

Core::RefCountPtr<IDStorageFactory> D3D12GfxDevice::DStorageFactory()
{
	return m_DStorageFactory;
}

Core::RefCountPtr<IDStorageQueue> D3D12GfxDevice::DStorageQueue(DSTORAGE_REQUEST_SOURCE_TYPE type)
{
	return m_DStorageQueues[(size_t)type];
}

Core::RefCountPtr<ID3D12Fence> D3D12GfxDevice::DStorageFence()
{
	return m_DStorageFence;
}

Core::RefCountPtr<D3D12MA::Allocator> D3D12GfxDevice::D3d12MemAllocator()
{
	return m_D3d12MemAllocator;
}

RHICapability D3D12GfxDevice::RhiCapabilities()
{
	return m_RhiCapabilities;
}

D3D12_FEATURE_DATA_ROOT_SIGNATURE D3D12GfxDevice::FeatureDataRootSignature()
{
	return m_FeatureDataRootSignature;
}

D3D12_FEATURE_DATA_SHADER_MODEL D3D12GfxDevice::FeatureDataShaderModel()
{
	return m_FeatureDataShaderModel;
}

ShaderModel D3D12GfxDevice::MinShaderModel()
{
	return m_MinShaderModel;
}

bool D3D12GfxDevice::IsUnderGraphicsDebugger()
{
	return m_IsUnderGraphicsDebugger;
}

bool D3D12GfxDevice::DebugEnabled()
{
	return sDebugEnabled;
}

ID3D12DescriptorAllocator& D3D12GfxDevice::CpuResourceHeap()
{
	return m_cpuDescriptorHeaps[(size_t)DescriptorHeapTypes::CBV_SRV_UAV];
}

ID3D12DescriptorAllocator& D3D12GfxDevice::CpuSamplerHeap()
{
	return m_cpuDescriptorHeaps[(size_t)DescriptorHeapTypes::Sampler];
}

ID3D12DescriptorAllocator& D3D12GfxDevice::CpuRenderTargetHeap()
{
	return m_cpuDescriptorHeaps[(size_t)DescriptorHeapTypes::RTV];
}

ID3D12DescriptorAllocator& D3D12GfxDevice::CpuDepthStencilHeap()
{
	return m_cpuDescriptorHeaps[(size_t)DescriptorHeapTypes::DSV];
}

ID3D12DescriptorAllocator& D3D12GfxDevice::GpuResourceHeap()
{
	return m_gpuDescriptorHeaps[0];
}

ID3D12DescriptorAllocator& D3D12GfxDevice::GpuSamplerHeap()
{
	return m_gpuDescriptorHeaps[1];
}

RHI::DeferedReleaseQueue& D3D12GfxDevice::ReleaseQueue()
{
	return m_ReleaseQueue;
}

size_t D3D12GfxDevice::MaxFramesInflight()
{
	return m_BufferCount;
}

