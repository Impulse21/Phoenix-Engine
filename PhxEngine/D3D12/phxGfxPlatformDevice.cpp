#include "pch.h"
#include "phxGfxPlatformDevice.h"
#include "phxCommandLineArgs.h"
#include "phxGfxPlatformCommon.h"

using namespace phx;
using namespace dx;


// Teir 1 limit is 1,000,000
// https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
#define TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE 1000000

#define NUM_BINDLESS_RESOURCES TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE / 2


namespace
{

	const GUID RenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
	const GUID PixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

	bool SafeTestD3D12CreateDevice(IDXGIAdapter* adapter, D3D_FEATURE_LEVEL minFeatureLevel, phx::gfx::D3D12DeviceBasicInfo& outInfo)
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
}

namespace phx::gfx
{

	HRESULT D3D12GpuAdapter::EnumAdapters(uint32_t adapterIndex, IDXGIFactory6* factory6, IDXGIAdapter1** outAdapter)
	{
		return factory6->EnumAdapterByGpuPreference(
			adapterIndex,
			DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
			IID_PPV_ARGS(outAdapter));
	}

	void D3D12Device::Initialize()
	{
		PHX_CORE_INFO("Initialize DirectX 12 Graphics Device");
#if true
		this->InitializeD3D12();

		// Create Queues
		this->m_commandQueues[(int)CommandQueueType::Graphics].Initialize(this->GetPlatformDevice().Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
		this->m_commandQueues[(int)CommandQueueType::Compute].Initialize(this->GetPlatformDevice().Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE);
		this->m_commandQueues[(int)CommandQueueType::Copy].Initialize(this->GetPlatformDevice().Get(), D3D12_COMMAND_LIST_TYPE_COPY);


		// Create Descriptor Heaps
		this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV].Initialize(
			this,
			1024,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler].Initialize(
			this,
			1024,
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

		this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::RTV].Initialize(
			this,
			1024,
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::DSV].Initialize(
			this,
			1024,
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


		this->m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV].Initialize(
			this,
			NUM_BINDLESS_RESOURCES,
			TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE - NUM_BINDLESS_RESOURCES,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);


		this->m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler].Initialize(
			this,
			10,
			100,
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

		D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
		allocatorDesc.pDevice = this->m_platformDevice.Get();
		allocatorDesc.pAdapter = this->m_gpuAdapter.PlatformAdapter.Get();
		//allocatorDesc.PreferredBlockSize = 256 * 1024 * 1024;
		//allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_ALWAYS_COMMITTED;
		allocatorDesc.Flags = (D3D12MA::ALLOCATOR_FLAGS)(D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED);

		ThrowIfFailed(
			D3D12MA::CreateAllocator(&allocatorDesc, &this->m_gpuMemAllocator));


#else
		this->InitializeResourcePools();
		this->InitializeD3D12NativeResources(this->m_gpuAdapter.NativeAdapter.Get());

#if ENABLE_PIX_CAPUTRE
		this->m_pixCaptureModule = PIXLoadLatestWinPixGpuCapturerLibrary();
#endif 

		// Create Queues
		this->m_commandQueues[(int)CommandQueueType::Graphics].Initialize(this->GetD3D12Device().Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
		this->m_commandQueues[(int)CommandQueueType::Compute].Initialize(this->GetD3D12Device().Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE);
		this->m_commandQueues[(int)CommandQueueType::Copy].Initialize(this->GetD3D12Device().Get(), D3D12_COMMAND_LIST_TYPE_COPY);

		// -- Create Common indirect signatures --
		// Create common indirect command signatures:
		D3D12_INDIRECT_ARGUMENT_DESC dispatchArgs[1];
		dispatchArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

		D3D12_COMMAND_SIGNATURE_DESC cmdDesc = {};
		cmdDesc.ByteStride = sizeof(IndirectDispatchArgs);
		cmdDesc.NumArgumentDescs = 1;
		cmdDesc.pArgumentDescs = dispatchArgs;
		ThrowIfFailed(
			this->GetD3D12Device()->CreateCommandSignature(&cmdDesc, nullptr, IID_PPV_ARGS(&this->m_dispatchIndirectCommandSignature)));


		D3D12_INDIRECT_ARGUMENT_DESC drawInstancedArgs[1];
		drawInstancedArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

		cmdDesc.ByteStride = sizeof(IndirectDrawArgInstanced);
		cmdDesc.NumArgumentDescs = 1;
		cmdDesc.pArgumentDescs = drawInstancedArgs;
		ThrowIfFailed(
			this->GetD3D12Device()->CreateCommandSignature(&cmdDesc, nullptr, IID_PPV_ARGS(&this->m_drawInstancedIndirectCommandSignature)));

		D3D12_INDIRECT_ARGUMENT_DESC drawIndexedInstancedArgs[1];
		drawIndexedInstancedArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

		cmdDesc.ByteStride = sizeof(IndirectDrawArgsIndexedInstanced);
		cmdDesc.NumArgumentDescs = 1;
		cmdDesc.pArgumentDescs = drawIndexedInstancedArgs;
		ThrowIfFailed(
			this->GetD3D12Device()->CreateCommandSignature(&cmdDesc, nullptr, IID_PPV_ARGS(&this->m_drawIndexedInstancedIndirectCommandSignature)));

		if (this->CheckCapability(DeviceCapability::MeshShading))
		{
			D3D12_INDIRECT_ARGUMENT_DESC dispatchMeshArgs[1];
			dispatchMeshArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;

			cmdDesc.ByteStride = sizeof(IndirectDispatchArgs);
			cmdDesc.NumArgumentDescs = 1;
			cmdDesc.pArgumentDescs = dispatchMeshArgs;
			ThrowIfFailed(
				this->GetD3D12Device()->CreateCommandSignature(&cmdDesc, nullptr, IID_PPV_ARGS(&this->m_dispatchMeshIndirectCommandSignature)));
		}

		// Create Descriptor Heaps
		this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV].Initialize(
			this,
			1024,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler].Initialize(
			this,
			1024,
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

		this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::RTV].Initialize(
			this,
			1024,
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::DSV].Initialize(
			this,
			1024,
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


		this->m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV].Initialize(
			this,
			NUM_BINDLESS_RESOURCES,
			TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE - NUM_BINDLESS_RESOURCES,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);


		this->m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler].Initialize(
			this,
			10,
			100,
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);


		this->m_bindlessResourceDescriptorTable.Initialize(this->GetResourceGpuHeap().Allocate(NUM_BINDLESS_RESOURCES));

		D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
		allocatorDesc.pDevice = this->m_platformDevice.Get();
		allocatorDesc.pAdapter = this->m_gpuAdapter.NativeAdapter.Get();
		//allocatorDesc.PreferredBlockSize = 256 * 1024 * 1024;
		//allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_ALWAYS_COMMITTED;
		allocatorDesc.Flags = (D3D12MA::ALLOCATOR_FLAGS)(D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED);

		ThrowIfFailed(
			D3D12MA::CreateAllocator(&allocatorDesc, &this->m_d3d12MemAllocator));

		// TODO: Data drive device create info
		this->CreateGpuTimestampQueryHeap(this->kTimestampQueryHeapSize);

		// Create GPU Buffer for timestamp readbacks
		this->m_timestampQueryBuffer = this->CreateBuffer({
			.Usage = Usage::ReadBack,
			.Stride = sizeof(uint64_t),
			.NumElements = this->kTimestampQueryHeapSize,
			.DebugName = "Timestamp Query Buffer" });

		this->CreateSwapChain(swapchainDesc, windowHandle);

		// -- Mark Queues for completion ---

		for (uint32_t buffer = 0; buffer < this->m_swapChain.Desc.BufferCount; ++buffer)
		{
			for (size_t q = 0; q < (size_t)CommandQueueType::Count; ++q)
			{
				ThrowIfFailed(
					this->GetD3D12Device2()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->m_frameFences[buffer][q])));
			}
		}
#endif
	}

	void D3D12Device::Finalize()
	{
	}

	void D3D12Device::WaitForIdle()
	{
		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		HRESULT hr = this->m_platformDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		assert(SUCCEEDED(hr));

		for (size_t q = 0; q < (size_t)CommandQueueType::Count; ++q)
		{
			D3D12CommandQueue& queue = this->m_commandQueues[q];
			hr = queue.m_platformQueue->Signal(fence.Get(), 1);
			assert(SUCCEEDED(hr));
			if (fence->GetCompletedValue() < 1)
			{
				hr = fence->SetEventOnCompletion(1, NULL);
				assert(SUCCEEDED(hr));
			}
			fence->Signal(0);
		}
	}

	bool D3D12Device::Create(SwapChainDesc const& desc, D3D12SwapChain& out)
	{
		HRESULT hr;

		UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

		const auto& formatMapping = dx::GetDxgiFormatMapping(desc.Format);
		if (out.m_platform == nullptr)
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
			swapChainDesc.BufferCount = desc.BufferCount;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
			swapChainDesc.Flags = swapChainFlags;

			swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

			DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
			fullscreenDesc.Windowed = !desc.Fullscreen;

			hr = this->m_factory->CreateSwapChainForHwnd(
				this->GetGfxQueue().m_platformQueue.Get(),
				static_cast<HWND>(desc.WindowHandle),
				&swapChainDesc,
				&fullscreenDesc,
				nullptr,
				out.m_platform.GetAddressOf());

			if (FAILED(hr))
			{
				throw std::exception();
			}


			hr = out.m_platform.As(&out.m_platform4);
			if (FAILED(hr))
			{
				throw std::exception();
			}

		}
		else
		{
			// Resize swapchain:
			this->WaitForIdle();

			out.m_backBuffers.clear();

			hr = out.m_platform4->ResizeBuffers(
				desc.BufferCount,
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

		out.m_backBuffers.resize(desc.BufferCount);

		for (UINT i = 0; i < desc.BufferCount; i++)
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
			ThrowIfFailed(
				out.m_platform4->GetBuffer(i, IID_PPV_ARGS(&out.m_backBuffers[i])));

			wchar_t allocatorName[32];
			swprintf_s(allocatorName, L"Back Buffer %iu", i);
			out.m_backBuffers[i]->SetName(allocatorName);
		}
	}

	void D3D12Device::InitializeD3D12()
	{
		uint32_t useDebugLayers = 0;
#if _DEBUG
		// Default to true for debug builds
		useDebugLayers = 1;
#endif
		CommandLineArgs::GetInteger(L"debug", useDebugLayers);

		// -- Create DXGI Factory ---
		{
			uint32_t flags = 0;
			if (useDebugLayers)
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

			ThrowIfFailed(
				CreateDXGIFactory2(flags, IID_PPV_ARGS(this->m_factory.ReleaseAndGetAddressOf())));
		}

		this->FindAdapter();

		if (!this->m_gpuAdapter.PlatformAdapter)
		{
			PHX_CORE_ERROR("Unable to create D3D12 RHI On current platform.");
			return;
		}

		ThrowIfFailed(
			D3D12CreateDevice(
				this->m_gpuAdapter.PlatformAdapter.Get(),
				D3D_FEATURE_LEVEL_11_1,
				IID_PPV_ARGS(&this->m_platformDevice)));

		this->m_platformDevice->SetName(L"Main_Device");
#if false
		Microsoft::WRL::ComPtr<IUnknown> renderdoc;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, RenderdocUUID, &renderdoc)))
		{
			this->IsUnderGraphicsDebugger |= !!renderdoc;
		}

		Microsoft::WRL::ComPtr<IUnknown> pix;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, PixUUID, &pix)))
		{
			this->IsUnderGraphicsDebugger |= !!pix;
		}
#endif
		D3D12_FEATURE_DATA_D3D12_OPTIONS featureOpptions = {};
		bool hasOptions = SUCCEEDED(this->m_platformDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOpptions, sizeof(featureOpptions)));

		if (hasOptions)
		{
			if (featureOpptions.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation)
			{
				this->m_capabilities |= DeviceCapability::RT_VT_ArrayIndex_Without_GS;
			}
		}

		// TODO: Move to acability array
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport5 = {};
		bool hasOptions5 = SUCCEEDED(this->m_platformDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport5, sizeof(featureSupport5)));

		if (SUCCEEDED(this->m_platformDevice.As(&this->m_platformDevice5)) && hasOptions5)
		{
			if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
			{
				this->m_capabilities |= DeviceCapability::RayTracing;
			}
			if (featureSupport5.RenderPassesTier >= D3D12_RENDER_PASS_TIER_0)
			{
				this->m_capabilities |= DeviceCapability::RenderPass;
			}
			if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1)
			{
				this->m_capabilities |= DeviceCapability::RayQuery;
			}
		}


		D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureSupport6 = {};
		bool hasOptions6 = SUCCEEDED(this->m_platformDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureSupport6, sizeof(featureSupport6)));

		if (hasOptions6)
		{
			if (featureSupport6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
			{
				this->m_capabilities |= DeviceCapability::VariableRateShading;
			}
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupport7 = {};
		bool hasOptions7 = SUCCEEDED(this->m_platformDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureSupport7, sizeof(featureSupport7)));

		if (SUCCEEDED(this->m_platformDevice.As(&this->m_platformDevice2)) && hasOptions7)
		{
			if (featureSupport7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1)
			{
				this->m_capabilities |= DeviceCapability::MeshShading;
			}
			this->m_capabilities |= DeviceCapability::CreateNoteZeroed;
		}

		this->FeatureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(this->m_platformDevice2->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &this->FeatureDataRootSignature, sizeof(this->FeatureDataRootSignature))))
		{
			this->FeatureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		// Check shader model support
		this->FeatureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
		this->m_minShaderModel = ShaderModel::SM_6_6;
		if (FAILED(this->m_platformDevice2->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &this->FeatureDataShaderModel, sizeof(this->FeatureDataShaderModel))))
		{
			this->FeatureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_5;
			this->m_minShaderModel = ShaderModel::SM_6_5;
		}

		if (useDebugLayers)
		{
			Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
			if (SUCCEEDED(this->m_platformDevice->QueryInterface<ID3D12InfoQueue>(&infoQueue)))
			{
				infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
				infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
				infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);

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

		// Create Compiler
		// ThrowIfFailed(
			// DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&context.dxcUtils)));
	}

	void D3D12Device::FindAdapter()
	{
		PHX_CORE_INFO("Finding a suitable adapter");

		// Create factory
		Microsoft::WRL::ComPtr<IDXGIAdapter1> selectedAdapter;
		D3D12DeviceBasicInfo selectedBasicDeviceInfo = {};
		size_t selectedGPUVideoMemeory = 0;
		Microsoft::WRL::ComPtr<IDXGIAdapter1> tempAdapter;
		for (uint32_t adapterIndex = 0; D3D12GpuAdapter::EnumAdapters(adapterIndex, this->m_factory.Get(), tempAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
		{
			if (!tempAdapter)
			{
				continue;
			}

			DXGI_ADAPTER_DESC1 desc = {};
			tempAdapter->GetDesc1(&desc);

			std::wstring name  = desc.Description;
			size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
			// size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
			// size_t sharedSystemMemory = desc.SharedSystemMemory;

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				PHX_CORE_INFO("GPU '%s' is a software adapter. Skipping consideration as this is not supported.", name.c_str());
				continue;
			}

			D3D12DeviceBasicInfo basicDeviceInfo = {};
			if (!SafeTestD3D12CreateDevice(tempAdapter.Get(), D3D_FEATURE_LEVEL_11_1, basicDeviceInfo))
			{
				continue;
			}

			if (basicDeviceInfo.NumDeviceNodes > 1)
			{
				PHX_CORE_INFO("GPU '%s' has one or more device nodes. Currently only support one device ndoe.", name.c_str());
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

		size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
		size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
		size_t sharedSystemMemory = desc.SharedSystemMemory;

		// TODO: FIXLOG
		PHX_CORE_INFO(
			"Found Suitable D3D12 Adapter '%s'",
			name.c_str());

		// TODO: FIXLOG
		PHX_CORE_INFO(
			"Adapter has %dMB of dedicated video memory, %dMB of dedicated system memory, and %dMB of shared system memory.",
			dedicatedVideoMemory / (1024 * 1024),
			dedicatedSystemMemory / (1024 * 1024),
			sharedSystemMemory / (1024 * 1024));

		this->m_gpuAdapter.Name = desc.Description;
		this->m_gpuAdapter.BasicDeviceInfo = selectedBasicDeviceInfo;
		this->m_gpuAdapter.PlatformDesc = desc;
		this->m_gpuAdapter.PlatformAdapter = selectedAdapter;
	}
}