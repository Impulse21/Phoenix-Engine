#include "D3D12Context.h"

#include <Core/Log.h>

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

	D3D12Adapter							m_GpuAdapter;
	Core::RefCountPtr<IDXGIFactory6>		m_DxgiFctory6;
	Core::RefCountPtr<ID3D12Device>			m_D3d12Device;
	Core::RefCountPtr<ID3D12Device2>		m_D3d12Device2;
	Core::RefCountPtr<ID3D12Device5>		m_D3d12Device5;

	D3D12CommandQueue						m_Queues[(size_t)RHI::CommandListType::Count];
	// -- Descriptor Heaps ---
	std::array<D3D12CpuDescriptorHeap, (int)DescriptorHeapTypes::Count> m_cpuDescriptorHeaps;
	std::array<D3D12GpuDescriptorHeap, 2> m_gpuDescriptorHeaps;

	Core::RefCountPtr<IDStorageFactory>		m_DStorageFactory;
	Core::RefCountPtr<IDStorageQueue>		m_DStorageQueues[2];
	Core::RefCountPtr<ID3D12Fence>			m_DStorageFence;

	Core::RefCountPtr<D3D12MA::Allocator>	m_D3d12MemAllocator;

	DeferedReleaseQueue						m_ReleaseQueue;
	RHICapability							m_RhiCapabilities;
	D3D12_FEATURE_DATA_ROOT_SIGNATURE		m_FeatureDataRootSignature;
	D3D12_FEATURE_DATA_SHADER_MODEL			m_FeatureDataShaderModel;
	ShaderModel								m_MinShaderModel;
	bool									m_IsUnderGraphicsDebugger;
	size_t									m_BufferCount;

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

void PhxEngine::RHI::D3D12::Context::Initialize(D3D12Adapter const& adapter, size_t bufferCount)
{
	m_BufferCount = bufferCount;
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

void PhxEngine::RHI::D3D12::Context::Finalize()
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
}

void PhxEngine::RHI::D3D12::Context::WaitForIdle()
{
	for (size_t i = 0; i < (size_t)CommandListType::Count; i++)
	{
		m_Queues[i].WaitForIdle();
	}
}

GpuMemoryUsage PhxEngine::RHI::D3D12::Context::GetMemoryUsage()
{
	D3D12MA::Budget budget;
	m_D3d12MemAllocator->GetBudget(&budget, nullptr);

	GpuMemoryUsage retval;
	retval.Budget = budget.BudgetBytes;
	retval.Usage = budget.UsageBytes;

	return retval;
}

D3D12Adapter PhxEngine::RHI::D3D12::Context::GpuAdapter()
{
	return m_GpuAdapter;
}

Core::RefCountPtr<IDXGIFactory6> PhxEngine::RHI::D3D12::Context::DxgiFctory6()
{
	return m_DxgiFctory6;
}

Core::RefCountPtr<ID3D12Device> PhxEngine::RHI::D3D12::Context::D3d12Device()
{
	return m_D3d12Device;
}

Core::RefCountPtr<ID3D12Device2> PhxEngine::RHI::D3D12::Context::D3d12Device2()
{
	return m_D3d12Device2;
}

Core::RefCountPtr<ID3D12Device5> PhxEngine::RHI::D3D12::Context::D3d12Device5()
{
	return m_D3d12Device5;
}

D3D12CommandQueue& PhxEngine::RHI::D3D12::Context::Queue(RHI::CommandListType type)
{
	return m_Queues[(size_t)type];
}

Core::RefCountPtr<IDStorageFactory> PhxEngine::RHI::D3D12::Context::DStorageFactory()
{
	return m_DStorageFactory;
}

Core::RefCountPtr<IDStorageQueue> PhxEngine::RHI::D3D12::Context::DStorageQueue(DSTORAGE_REQUEST_SOURCE_TYPE type)
{
	return m_DStorageQueues[(size_t)type];
}

Core::RefCountPtr<ID3D12Fence> PhxEngine::RHI::D3D12::Context::DStorageFence()
{
	return m_DStorageFence;
}

Core::RefCountPtr<D3D12MA::Allocator> PhxEngine::RHI::D3D12::Context::D3d12MemAllocator()
{
	return m_D3d12MemAllocator;
}

RHICapability PhxEngine::RHI::D3D12::Context::RhiCapabilities()
{
	return m_RhiCapabilities;
}

D3D12_FEATURE_DATA_ROOT_SIGNATURE PhxEngine::RHI::D3D12::Context::FeatureDataRootSignature()
{
	return m_FeatureDataRootSignature;
}

D3D12_FEATURE_DATA_SHADER_MODEL PhxEngine::RHI::D3D12::Context::FeatureDataShaderModel()
{
	return m_FeatureDataShaderModel;
}

ShaderModel PhxEngine::RHI::D3D12::Context::MinShaderModel()
{
	return m_MinShaderModel;
}

bool PhxEngine::RHI::D3D12::Context::IsUnderGraphicsDebugger()
{
	return m_IsUnderGraphicsDebugger;
}

bool PhxEngine::RHI::D3D12::Context::DebugEnabled()
{
	return sDebugEnabled;
}

ID3D12DescriptorAllocator& PhxEngine::RHI::D3D12::Context::CpuResourceHeap()
{
	return m_cpuDescriptorHeaps[(size_t)DescriptorHeapTypes::CBV_SRV_UAV];
}

ID3D12DescriptorAllocator& PhxEngine::RHI::D3D12::Context::CpuSamplerHeap()
{
	return m_cpuDescriptorHeaps[(size_t)DescriptorHeapTypes::Sampler];
}

ID3D12DescriptorAllocator& PhxEngine::RHI::D3D12::Context::CpuRenderTargetHeap()
{
	return m_cpuDescriptorHeaps[(size_t)DescriptorHeapTypes::RTV];
}

ID3D12DescriptorAllocator& PhxEngine::RHI::D3D12::Context::CpuDepthStencilHeap()
{
	return m_cpuDescriptorHeaps[(size_t)DescriptorHeapTypes::DSV];
}

ID3D12DescriptorAllocator& PhxEngine::RHI::D3D12::Context::GpuResourceHeap()
{
	return m_gpuDescriptorHeaps[0];
}

ID3D12DescriptorAllocator& PhxEngine::RHI::D3D12::Context::GpuSamplerHeap()
{
	return m_gpuDescriptorHeaps[1];
}

RHI::DeferedReleaseQueue& PhxEngine::RHI::D3D12::Context::ReleaseQueue()
{
	return m_ReleaseQueue;
}

size_t PhxEngine::RHI::D3D12::Context::MaxFramesInflight()
{
	return m_BufferCount;
}

