#include "PhxRhi/PhxRhi_pch.h"

#include "PhxCore/Math.h"
#include "PhxCore/StringUtils.h"
#include "PhxCore/CommandLineArgs.h"

#include "PhxRhi/RHICommandCtx.h"
#include "PhxRhi/RHITypes.h"
#include "PhxRhi/RHICore.h"

#include "D3D12Types.h"
#include "D3D12Core.h"
#include "D3D12Utils.h"

#include "D3D12CommandQueue.h"
#include "D3D12DescriptorHeaps.h"

#include "D3D12MemAlloc.h"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-function"
#endif

#define SAFE_DELETE(x) if (x) { delete x; }

using namespace phx;
using namespace phx::rhi;
using namespace phx::RHI::d3d12;
using namespace Microsoft::WRL;

namespace
{
	const GUID kRenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
	const GUID kPixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };


	D3D12_FEATURE_DATA_ROOT_SIGNATURE m_featureDataRootSignature = {};
	D3D12_FEATURE_DATA_SHADER_MODEL   m_featureDataShaderModel = {};
	ShaderModel m_minShaderModel = ShaderModel::SM_6_0;

	std::array<EnumArray<Microsoft::WRL::ComPtr<ID3D12Fence>, CommandQueueType>, kBufferCount> m_frameFences;

	std::deque<DeferredItem> m_deferredQueue;

	std::vector<std::unique_ptr<RHI::CommandCtx>> m_commandCtxPool;
	size_t m_numActiveCmdLists = 0;
	std::mutex m_commandCtxMutex;

}

namespace phx::RHI::d3d12
{
	Microsoft::WRL::ComPtr<IDXGIFactory6> g_dxgiFactory = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Device> g_d3d12Device = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Device2> g_d3d12Device2 = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Device5> g_d3d12Device5 = nullptr;
	bool g_isUnderGfxDebugger;
	bool g_debugLayersEnabled = false;
	D3D12_RESOURCE_HEAP_TIER g_resourceHeapTeir;

	D3D12Adapter g_adapter;

	RHI::DeviceCapability g_capabilities;

	// -- Command queues ---
	EnumArray<D3D12CommandQueue, CommandQueueType> g_commandQueue;

	// -- Descriptor Heaps ---
	CpuDescriptorHeap* g_cpuDescHeap_Resource = nullptr;
	CpuDescriptorHeap* g_cpuDescHeap_Sampler = nullptr;
	CpuDescriptorHeap* g_cpuDescHeap_Rtv = nullptr;
	CpuDescriptorHeap* g_cpuDescHeap_Dsv = nullptr;

	GpuDescriptorHeap* g_gpuDescHeap_Resource = nullptr;
	GpuDescriptorHeap* g_gpuDescHeap_Sampler = nullptr;

	D3D12SwapChain g_swapChain;

	size_t g_frameCount = 0;
}

namespace
{
	Microsoft::WRL::ComPtr<IDXGIFactory6> CreateDXGIFactory6(bool enableDebugLayers)
	{
		uint32_t flags = 0;
		if (enableDebugLayers)
		{
			Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
			ThrowIfFailed(
				D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));

			debugController->EnableDebugLayer();
			flags = DXGI_CREATE_FACTORY_DEBUG;

			Microsoft::WRL::ComPtr<ID3D12Debug3> debugController3;
			if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController3))))
			{
				debugController3->SetEnableGPUBasedValidation(false);
			}

			Microsoft::WRL::ComPtr<ID3D12Debug5> debugController5;
			if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController5))))
			{
				debugController5->SetEnableAutoName(true);
			}
		}

		Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
		ThrowIfFailed(
			CreateDXGIFactory2(flags, IID_PPV_ARGS(factory.ReleaseAndGetAddressOf())));

		return factory;
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
				PHX_CORE_WARN("D3D12CreateDevice failed.");
			}
		}
		catch (...)
		{
		}
#pragma warning(default:6322)

		return false;
	}

	void FindAdapter(Microsoft::WRL::ComPtr<IDXGIFactory6> factory, D3D12Adapter& outAdapter)
	{
		PHX_CORE_INFO("Finding a suitable adapter");

		// Create factory
		Microsoft::WRL::ComPtr<IDXGIAdapter1> selectedAdapter;
		D3D12DeviceBasicInfo selectedBasicDeviceInfo = {};
		size_t selectedGPUVideoMemeory = 0;
		Microsoft::WRL::ComPtr<IDXGIAdapter1> tempAdapter;
		for (uint32_t adapterIndex = 0; D3D12Adapter::EnumAdapters(adapterIndex, factory.Get(), tempAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
		{
			if (!tempAdapter)
			{
				continue;
			}

			DXGI_ADAPTER_DESC1 desc = {};
			tempAdapter->GetDesc1(&desc);

			std::string name;
			phx::StringConvert(desc.Description, name);
			size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
			// size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
			// size_t sharedSystemMemory = desc.SharedSystemMemory;

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				PHX_CORE_INFO("GPU '{0}' is a software adapter. Skipping consideration as this is not supported.", name.c_str());
				continue;
			}

			D3D12DeviceBasicInfo basicDeviceInfo = {};
			if (!SafeTestD3D12CreateDevice(tempAdapter.Get(), D3D_FEATURE_LEVEL_11_1, basicDeviceInfo))
			{
				continue;
			}

			if (basicDeviceInfo.NumDeviceNodes > 1)
			{
				PHX_CORE_INFO("GPU {0}' has one or more device nodes. Currently only support one device ndoe.", name.c_str());
			}

			if (!selectedAdapter || selectedGPUVideoMemeory < dedicatedVideoMemory)
			{
				selectedAdapter = tempAdapter;
				selectedGPUVideoMemeory = dedicatedVideoMemory;
				selectedBasicDeviceInfo = basicDeviceInfo;
			}
		}

		if (!selectedAdapter)
		{
			PHX_CORE_WARN("No suitable adapters were found.");
			return;
		}

		DXGI_ADAPTER_DESC desc = {};
		selectedAdapter->GetDesc(&desc);

		std::string name;
		phx::StringConvert(desc.Description, name);
		size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
		size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
		size_t sharedSystemMemory = desc.SharedSystemMemory;

		// TODO: FIXLOG
		PHX_CORE_INFO(
			"Found Suitable D3D12 Adapter '{0}'",
			name.c_str());

		// TODO: FIXLOG
		PHX_CORE_INFO(
			"Adapter has {0}MB of dedicated video memory, {1}MB of dedicated system memory, and {2}MB of shared system memory.",
			dedicatedVideoMemory / (1024 * 1024),
			dedicatedSystemMemory / (1024 * 1024),
			sharedSystemMemory / (1024 * 1024));

		phx::StringConvert(desc.Description, outAdapter.Name);
		outAdapter.BasicDeviceInfo = selectedBasicDeviceInfo;
		outAdapter.NativeDesc = desc;
		outAdapter.NativeAdapter = selectedAdapter;
	}

	void InitializeD3D12Context()
	{
		{
			uint32_t useDebugLayers = 0;
#if _DEBUG
			// Default to true for debug builds
			useDebugLayers = 1;
#endif
			phx::CommandLineArgs::GetInteger(L"debug", useDebugLayers);
			g_debugLayersEnabled = (bool)useDebugLayers;
		}

		g_dxgiFactory = CreateDXGIFactory6(g_debugLayersEnabled);
		FindAdapter(g_dxgiFactory, g_adapter);

		if (!g_adapter.NativeAdapter)
		{
			PHX_CORE_ERROR("Unable to create D3D12 RHI On current platform.");
		}

		ThrowIfFailed(
			D3D12CreateDevice(
				g_adapter.NativeAdapter.Get(),
				D3D_FEATURE_LEVEL_11_1,
				IID_PPV_ARGS(&g_d3d12Device)));

		g_d3d12Device->SetName(L"D3D12GfxDevice::RootDevice");
		Microsoft::WRL::ComPtr<IUnknown> renderdoc;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, kRenderdocUUID, &renderdoc)))
		{
			g_isUnderGfxDebugger |= !!renderdoc;
		}

		Microsoft::WRL::ComPtr<IUnknown> pix;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, kPixUUID, &pix)))
		{
			g_isUnderGfxDebugger |= !!pix;
		}

		// TODO: Make use of this feature below.
		CD3DX12FeatureSupport features;
		PHX_ASSERT(
			SUCCEEDED(features.Init(g_d3d12Device.Get())),
			"Unexpected Failure");


		g_resourceHeapTeir = features.ResourceHeapTier();

		if (g_resourceHeapTeir >= D3D12_RESOURCE_HEAP_TIER_2)
		{
			g_capabilities |= DeviceCapability::AliasingGeneric;
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS featureOpptions = {};
		bool hasOptions = SUCCEEDED(g_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOpptions, sizeof(featureOpptions)));

		if (hasOptions)
		{
			if (featureOpptions.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation)
			{
				g_capabilities |= DeviceCapability::RT_VT_ArrayIndex_Without_GS;
			}
		}

		// TODO: Move to acability array
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport5 = {};
		bool hasOptions5 = SUCCEEDED(g_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport5, sizeof(featureSupport5)));

		if (SUCCEEDED(g_d3d12Device.As(&g_d3d12Device5)) && hasOptions5)
		{
			if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
			{
				g_capabilities |= DeviceCapability::RayTracing;
			}
			if (featureSupport5.RenderPassesTier >= D3D12_RENDER_PASS_TIER_0)
			{
				g_capabilities |= DeviceCapability::RenderPass;
			}
			if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1)
			{
				g_capabilities |= DeviceCapability::RayQuery;
			}
		}


		D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureSupport6 = {};
		bool hasOptions6 = SUCCEEDED(g_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureSupport6, sizeof(featureSupport6)));

		if (hasOptions6)
		{
			if (featureSupport6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
			{
				g_capabilities |= DeviceCapability::VariableRateShading;
			}
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupport7 = {};
		bool hasOptions7 = SUCCEEDED(g_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureSupport7, sizeof(featureSupport7)));

		if (SUCCEEDED(g_d3d12Device.As(&g_d3d12Device2)) && hasOptions7)
		{
			if (featureSupport7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1)
			{
				g_capabilities |= DeviceCapability::MeshShading;
			}
			g_capabilities |= DeviceCapability::CreateNoteZeroed;
		}

		m_featureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(g_d3d12Device2->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &m_featureDataRootSignature, sizeof(m_featureDataRootSignature))))
		{
			m_featureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		// Check shader model support
		m_featureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
		m_minShaderModel = ShaderModel::SM_6_6;
		if (FAILED(g_d3d12Device2->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &m_featureDataShaderModel, sizeof(m_featureDataShaderModel))))
		{
			m_featureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_5;
			m_minShaderModel = ShaderModel::SM_6_5;
		}

		if (g_debugLayersEnabled)
		{
			Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
			if (SUCCEEDED(g_d3d12Device->QueryInterface<ID3D12InfoQueue>(&infoQueue)))
			{
				infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
				infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
				infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);

				D3D12_MESSAGE_SEVERITY severities[] =
				{
					D3D12_MESSAGE_SEVERITY_ERROR,
					D3D12_MESSAGE_SEVERITY_WARNING,
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

			// Create Queues
			g_commandQueue[CommandQueueType::Graphics].Initialize(g_d3d12Device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
			g_commandQueue[CommandQueueType::Compute].Initialize(g_d3d12Device.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE);
			g_commandQueue[CommandQueueType::Copy].Initialize(g_d3d12Device.Get(), D3D12_COMMAND_LIST_TYPE_COPY);

			// Create Descriptor Heaps
			g_cpuDescHeap_Resource = new CpuDescriptorHeap();
			g_cpuDescHeap_Resource->Initialize(
				g_d3d12Device2.Get(),
				1024,
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			g_cpuDescHeap_Sampler = new CpuDescriptorHeap();
			g_cpuDescHeap_Sampler->Initialize(
				g_d3d12Device2.Get(),
				1024,
				D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

			g_cpuDescHeap_Rtv = new CpuDescriptorHeap();
			g_cpuDescHeap_Rtv->Initialize(
				g_d3d12Device2.Get(),
				1024,
				D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			g_cpuDescHeap_Dsv = new CpuDescriptorHeap();
			g_cpuDescHeap_Dsv->Initialize(
				g_d3d12Device2.Get(),
				1024,
				D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

			g_gpuDescHeap_Resource = new GpuDescriptorHeap();
			g_gpuDescHeap_Resource->Initialize(
				g_d3d12Device2.Get(),
				NUM_BINDLESS_RESOURCES,
				TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE - NUM_BINDLESS_RESOURCES,
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

			g_gpuDescHeap_Sampler = new GpuDescriptorHeap();
			g_gpuDescHeap_Sampler->Initialize(
				g_d3d12Device2.Get(),
				10,
				100,
				D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
				D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

			for (auto& frameFences : m_frameFences)
			{
				for (size_t q = 0; q < (size_t)CommandQueueType::Count; ++q)
				{
					ThrowIfFailed(
						g_d3d12Device2->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frameFences[q])));
				}
			}
		};
	}

	void CreateSwapChain(RHI::SwapChainDescriptor const& desc, HWND hwnd)
	{
		HRESULT hr;

		g_swapChain.ClearColour = desc.OptmizedClearValue;
		g_swapChain.VSync = desc.VSync;
		g_swapChain.Fullscreen = desc.Fullscreen;
		g_swapChain.EnableHDR = desc.EnableHDR;

		UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		const auto& formatMapping = GetDxgiFormatMapping(desc.Format);
		if (g_swapChain.SwapChain == nullptr)
		{
			// Create swapchain:
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
			swapChainDesc.Width = desc.Width;
			swapChainDesc.Height = desc.Height;
			swapChainDesc.Format = formatMapping.RtvFormat;
			swapChainDesc.Stereo = false;
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.BufferCount = kBufferCount;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
			swapChainDesc.Flags = swapChainFlags;

			swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

			DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
			fullscreenDesc.Windowed = !desc.Fullscreen;

			hr = g_dxgiFactory->CreateSwapChainForHwnd(
				g_commandQueue[CommandQueueType::Graphics].Queue.Get(),
				hwnd,
				&swapChainDesc,
				&fullscreenDesc,
				nullptr,
				g_swapChain.SwapChain.GetAddressOf()
			);

			if (FAILED(hr))
			{
				throw std::exception();
			}

			hr = g_swapChain.SwapChain.As(&g_swapChain.SwapChain4);
			if (FAILED(hr))
			{
				throw std::exception();
			}
		}
		else
		{
			// Resize swapchain:
			WaitForIdle();

			// Delete back buffers
			g_swapChain.Rtv.Free();
			for (auto& backBuffer : g_swapChain.BackBuffers)
			{
				backBuffer.Reset();
			}

			hr = g_swapChain.SwapChain->ResizeBuffers(
				kBufferCount,
				desc.Width,
				desc.Height,
				formatMapping.RtvFormat,
				swapChainFlags
			);

			assert(SUCCEEDED(hr));
		}

		// -- From Wicked Engine
#ifdef ENABLE_HDR
		const bool hdr = desc->allow_hdr && IsSwapChainSupportsHDR(swapchain);

		// Ensure correct color space:
		//	https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HDR/src/D3D12HDR.cpp
		{
			internal_state->colorSpace = ColorSpace::SRGB; // reset to SDR, in case anything below fails to set HDR state
			DXGI_COLOR_SPACE_TYPE colorSpace = {};

			switch (desc->format)
			{
			case Format::R10G10B10A2_UNORM:
				// This format is either HDR10 (ST.2084), or SDR (SRGB)
				colorSpace = hdr ? DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
				break;
			case Format::R16G16B16A16_FLOAT:
				// This format is HDR (Linear):
				colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
				break;
			default:
				// Anything else will be SDR (SRGB):
				colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
				break;
			}

			UINT colorSpaceSupport = 0;
			if (SUCCEEDED(internal_state->swapChain->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport)))
			{
				if (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)
				{
					hr = internal_state->swapChain->SetColorSpace1(colorSpace);
					assert(SUCCEEDED(hr));
					if (SUCCEEDED(hr))
					{
						switch (colorSpace)
						{
						default:
						case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
							internal_state->colorSpace = ColorSpace::SRGB;
							break;
						case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
							internal_state->colorSpace = ColorSpace::HDR_LINEAR;
							break;
						case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
							internal_state->colorSpace = ColorSpace::HDR10_ST2084;
							break;
						}
					}
				}
			}
		}
#endif
		g_swapChain.Rtv = g_cpuDescHeap_Rtv->Allocate(kBufferCount);
		for (UINT i = 0; i < kBufferCount; i++)
		{
			Microsoft::WRL::ComPtr<ID3D12Resource>& backBuffer = g_swapChain.BackBuffers[i];
			ThrowIfFailed(
				g_swapChain.SwapChain4->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

			char allocatorName[32];
			sprintf_s(allocatorName, "Back Buffer %iu", i);

			g_d3d12Device2->CreateRenderTargetView(backBuffer.Get(), nullptr, g_swapChain.Rtv.GetCpuHandle(i));
		}
	}

	void RunGarbageCollection(uint64_t completedFrame)
	{
		while (!m_deferredQueue.empty())
		{
			DeferredItem& DeferredItem = m_deferredQueue.front();
			if (DeferredItem.Frame + kBufferCount < completedFrame)
			{
				DeferredItem.DeferredFunc();
				m_deferredQueue.pop_front();
			}
			else
			{
				break;
			}
		}
	}
}

namespace phx::rhi
{
	void Initialize(RhiCreateInfo const& createInfo)
	{
		PHX_CORE_INFO("Initialize RHI(D3D12)");
		InitializeD3D12Context();

		InitializeResources(createInfo);
		CreateSwapChain(createInfo.SwapChianDesc, static_cast<HWND>(createInfo.WindowsHandle));

		// TODO: Make this more consistent with out the rest of the code
		// works inside this module.
		TempMemoryBlockAllocator::Ptr = new TempMemoryBlockAllocator;
		TempMemoryBlockAllocator::Ptr->Initialize(math::GetNextPowerOfTwo(256_MiB));
	}

	void Finalize()
	{
		WaitForIdle();

		g_swapChain.Rtv.Free();
		for (auto& backBuffer : g_swapChain.BackBuffers)
		{
			backBuffer.Reset();
		}

		TempMemoryBlockAllocator::Ptr->Finalize();
		SAFE_DELETE(TempMemoryBlockAllocator::Ptr);

		FinalizeResources();

		SAFE_DELETE(g_cpuDescHeap_Resource)
		SAFE_DELETE(g_cpuDescHeap_Sampler)
		SAFE_DELETE(g_cpuDescHeap_Rtv)
		SAFE_DELETE(g_cpuDescHeap_Dsv)
		SAFE_DELETE(g_gpuDescHeap_Resource)
		SAFE_DELETE(g_gpuDescHeap_Sampler)
	}


	Budget GetBudget()
	{
		D3D12MA::Budget localBudget = {};
		g_d3d12MemAllocator->GetBudget(&localBudget, nullptr);

		return {
			.BudgetBytes = localBudget.BudgetBytes,
			.UsageBytes = localBudget.UsageBytes
		};
	}

	void WaitForIdle()
	{

		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		HRESULT hr = g_d3d12Device2->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		assert(SUCCEEDED(hr));
		
		for (auto& q : g_commandQueue)
		{
			hr = q.Queue->Signal(fence.Get(), 1);
			assert(SUCCEEDED(hr));
			if (fence->GetCompletedValue() < 1)
			{
				hr = fence->SetEventOnCompletion(1, NULL);
				assert(SUCCEEDED(hr));
			}
			fence->Signal(0);
		}

		RunGarbageCollection(UINT64_MAX);
	}

	CommandCtx* BeginCommnadCtx(CommandQueueType queueType)
	{
		size_t currentCtx = ~0u;
		{
			std::scoped_lock _(m_commandCtxMutex);
			currentCtx = m_numActiveCmdLists++;

			if (currentCtx >= m_commandCtxPool.size())
			{
				m_commandCtxPool.push_back(std::make_unique<CommandCtx>());
			}
		}

		std::unique_ptr<CommandCtx>& ctx = m_commandCtxPool[currentCtx];
		ctx->GetPlatform().Reset(queueType);

		return ctx.get();
	}

	void Present()
	{
		size_t numCommandLists = m_numActiveCmdLists;
		m_numActiveCmdLists = 0;

		// -- Process command contexts ---
		for (size_t i = 0; i < numCommandLists; i++)
		{
			// TODO: Add queue waiting support
			D3D12CommandCtx& d3d12Ctx = m_commandCtxPool[i]->GetPlatform();
			d3d12Ctx.EnqueueSubmit();
			
		}

		// -- Mark Queues for completion ---
		{
			const size_t backBufferIndex = g_swapChain.SwapChain4->GetCurrentBackBufferIndex();

			// -- Mark queues for Completion ---

			for (size_t qType = 0; qType < static_cast<size_t>(CommandQueueType::Count); qType++)
			{

				D3D12CommandQueue& queue = g_commandQueue[qType];
				queue.Submit();
				queue.Queue->Signal(m_frameFences[backBufferIndex][qType].Get(), 1);
			}

			TempMemoryBlockAllocator::Ptr->EndFrame();
		}

		// -- Present the back buffer ---
		{

			UINT presentFlags = 0;
			if (!g_swapChain.VSync)
			{
				presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
			}

			HRESULT hr = g_swapChain.SwapChain4->Present((UINT)g_swapChain.VSync, presentFlags);

			if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
			{
				assert(false);
			}
		}

		// -- wait for fence to finish
		{
			const size_t backBufferIndex = g_swapChain.SwapChain4->GetCurrentBackBufferIndex();

			g_frameCount++;

			// Sync The queeus
			for (size_t q = 0; q < (size_t)CommandQueueType::Count; q++)
			{
				ID3D12Fence* fence = m_frameFences[backBufferIndex][q].Get();
				const size_t completedValue = fence->GetCompletedValue();

				if (g_frameCount >= kBufferCount && completedValue < 1)
				{
					ThrowIfFailed(
						fence->SetEventOnCompletion(1, nullptr));
				}
				// Reset fence;
				fence->Signal(0);
			}
		}

		RunGarbageCollection(g_frameCount);
	}

	ShaderFormat GetShaderFormat() 
	{ 
		return ShaderFormat::Hlsl6;
	}
}

namespace phx::RHI::d3d12
{
	void EnqueueDelete(DeferredItem&& item)
	{
		m_deferredQueue.emplace_back(std::forward<DeferredItem>(item));
	}
}