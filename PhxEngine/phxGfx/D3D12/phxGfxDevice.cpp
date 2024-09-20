#include "pch.h"
#include "phxGfxCommon.h"
#include "phxGfxDevice.h"
#include "phxCommandLineArgs.h"

// Teir 1 limit is 1,000,000
// https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
#define TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE 1000000

#define NUM_BINDLESS_RESOURCES TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE / 2

using namespace phx;
using namespace phx::gfx;
using namespace dx;

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
			StringConvert(desc.Description, name);
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

		std::string name;
		StringConvert(desc.Description, name);
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

		StringConvert(desc.Description, outAdapter.Name);
		outAdapter.BasicDeviceInfo = selectedBasicDeviceInfo;
		outAdapter.NativeDesc = desc;
		outAdapter.NativeAdapter = selectedAdapter;
	}

}

phx::gfx::D3D12Device::D3D12Device(SwapChainDesc const& desc, HWND hwnd)
{
	assert(Singleton == nullptr);
	Singleton = this;
	this->Initialize();
	this->CreateSwapChain(desc, hwnd);
}

void phx::gfx::D3D12Device::WaitForIdle()
{
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	HRESULT hr = this->m_d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	for (size_t q = 0; q < (size_t)CommandQueueType::Count; ++q)
	{
		D3D12CommandQueue& queue = this->m_commandQueues[q];
		hr = queue.Queue->Signal(fence.Get(), 1);
		assert(SUCCEEDED(hr));
		if (fence->GetCompletedValue() < 1)
		{
			hr = fence->SetEventOnCompletion(1, NULL);
			assert(SUCCEEDED(hr));
		}
		fence->Signal(0);
	}
}

void phx::gfx::D3D12Device::ResizeSwapChain(SwapChainDesc const& desc)
{
	this->CreateSwapChain(desc, nullptr);
}

void phx::gfx::D3D12Device::Present()
{
	// -- Mark Queues for completion ---
	{
		const size_t backBufferIndex = this->m_swapChain.SwapChain4->GetCurrentBackBufferIndex();
		// -- Mark queues for Compleition ---
		for (size_t q = 0; q < (size_t)CommandQueueType::Count; q++)
		{
			D3D12CommandQueue& queue = this->m_commandQueues[q];
			queue.Queue->Signal(this->m_frameFences[backBufferIndex][q].Get(), 1);
		}
	}

	// -- Present the back buffer ---
	{

		UINT presentFlags = 0;
		if (!this->m_swapChain.VSync)
		{
			presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
		}

		HRESULT hr = this->m_swapChain.SwapChain4->Present((UINT)this->m_swapChain.VSync, presentFlags);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			assert(false);
		}
	}
	// -- wait for fence to finish
	{
		const size_t backBufferIndex = this->m_swapChain.SwapChain4->GetCurrentBackBufferIndex();

		static uint64_t sFrameCount = 0;
		sFrameCount++;

		// Sync The queeus
		for (size_t q = 0; q < (size_t)CommandQueueType::Count; q++)
		{
			ID3D12Fence* fence = this->m_frameFences[backBufferIndex][q].Get();
			const size_t completedValue = fence->GetCompletedValue();

			if (sFrameCount >= kBufferCount && completedValue < 1)
			{
				dx::ThrowIfFailed(
					fence->SetEventOnCompletion(1, NULL));
			}
			// Reset fence;
			fence->Signal(0);
		}
	}
}


void phx::gfx::D3D12Device::Initialize()
{
	PHX_CORE_INFO("Initialize DirectX 12 Graphics Device");

	this->InitializeD3D12Context(this->m_gpuAdapter.NativeAdapter.Get());

#if ENABLE_PIX_CAPUTRE
	this->m_pixCaptureModule = PIXLoadLatestWinPixGpuCapturerLibrary();
#endif 

	// Create Queues
	this->m_commandQueues[CommandQueueType::Graphics].Initialize(this->GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	this->m_commandQueues[CommandQueueType::Compute].Initialize(this->GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_COMPUTE);
	this->m_commandQueues[CommandQueueType::Copy].Initialize(this->GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_COPY);

	// Create Descriptor Heaps
	this->m_cpuDescriptorHeaps[DescriptorHeapTypes::CBV_SRV_UAV].Initialize(
		this->m_d3d12Device2,
		1024,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	this->m_cpuDescriptorHeaps[DescriptorHeapTypes::Sampler].Initialize(
		this->m_d3d12Device2,
		1024,
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	this->m_cpuDescriptorHeaps[DescriptorHeapTypes::RTV].Initialize(
		this->m_d3d12Device2,
		1024,
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	this->m_cpuDescriptorHeaps[DescriptorHeapTypes::DSV].Initialize(
		this->m_d3d12Device2,
		1024,
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


	this->m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV].Initialize(
		this->m_d3d12Device2,
		NUM_BINDLESS_RESOURCES,
		TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE - NUM_BINDLESS_RESOURCES,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);


	this->m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler].Initialize(
		this->m_d3d12Device2,
		10,
		100,
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

	for (auto& frameFences : this->m_frameFences)
	{
		for (size_t q = 0; q < (size_t)CommandQueueType::Count; ++q)
		{
			ThrowIfFailed(
				this->GetD3D12Device2()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frameFences[q])));
		}
	}

}

void phx::gfx::D3D12Device::InitializeD3D12Context(IDXGIAdapter* gpuAdapter)
{
	{
		uint32_t useDebugLayers = 0;
#if _DEBUG
		// Default to true for debug builds
		useDebugLayers = 1;
#endif
		CommandLineArgs::GetInteger(L"debug", useDebugLayers);
		this->m_debugLayersEnabled = (bool)useDebugLayers;
	}

	this->m_factory = CreateDXGIFactory6(this->m_debugLayersEnabled);
	FindAdapter(this->m_factory, this->m_gpuAdapter);

	if (!this->m_gpuAdapter.NativeAdapter)
	{
		// LOG_CORE_ERROR("Unable to create D3D12 RHI On current platform.");
	}

	ThrowIfFailed(
		D3D12CreateDevice(
			this->m_gpuAdapter.NativeAdapter.Get(),
			D3D_FEATURE_LEVEL_11_1,
			IID_PPV_ARGS(&this->m_d3d12Device)));

	ThrowIfFailed(
		D3D12CreateDevice(
			gpuAdapter,
			D3D_FEATURE_LEVEL_11_1,
			IID_PPV_ARGS(&this->m_d3d12Device)));

	this->m_d3d12Device->SetName(L"D3D12GfxDevice::RootDevice");
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

	D3D12_FEATURE_DATA_D3D12_OPTIONS featureOpptions = {};
	bool hasOptions = SUCCEEDED(this->m_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOpptions, sizeof(featureOpptions)));

	if (hasOptions)
	{
		if (featureOpptions.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation)
		{
			this->m_capabilities |= DeviceCapability::RT_VT_ArrayIndex_Without_GS;
		}
	}

	// TODO: Move to acability array
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport5 = {};
	bool hasOptions5 = SUCCEEDED(this->m_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport5, sizeof(featureSupport5)));

	if (SUCCEEDED(this->m_d3d12Device.As(&this->m_d3d12Device5)) && hasOptions5)
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
	bool hasOptions6 = SUCCEEDED(this->m_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureSupport6, sizeof(featureSupport6)));

	if (hasOptions6)
	{
		if (featureSupport6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
		{
			this->m_capabilities |= DeviceCapability::VariableRateShading;
		}
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupport7 = {};
	bool hasOptions7 = SUCCEEDED(this->m_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureSupport7, sizeof(featureSupport7)));

	if (SUCCEEDED(this->m_d3d12Device.As(&this->m_d3d12Device2)) && hasOptions7)
	{
		if (featureSupport7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1)
		{
			this->m_capabilities |= DeviceCapability::MeshShading;
		}
		this->m_capabilities |= DeviceCapability::CreateNoteZeroed;
	}

	this->m_featureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(this->m_d3d12Device2->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &this->m_featureDataRootSignature, sizeof(this->m_featureDataRootSignature))))
	{
		this->m_featureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Check shader model support
	this->m_featureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
	this->m_minShaderModel = ShaderModel::SM_6_6;
	if (FAILED(this->m_d3d12Device2->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &this->m_featureDataShaderModel, sizeof(this->m_featureDataShaderModel))))
	{
		this->m_featureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_5;
		this->m_minShaderModel = ShaderModel::SM_6_5;
	}


	static const bool debugEnabled = IsDebuggerPresent();
	if (debugEnabled)
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
		if (SUCCEEDED(this->m_d3d12Device->QueryInterface<ID3D12InfoQueue>(&infoQueue)))
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

void phx::gfx::D3D12Device::CreateSwapChain(SwapChainDesc const& desc, HWND hwnd)
{
	HRESULT hr;

	this->m_swapChain.VSync = desc.VSync;
	this->m_swapChain.Fullscreen = desc.Fullscreen;
	this->m_swapChain.EnableHDR = desc.EnableHDR;
	
	UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	const auto& formatMapping = GetDxgiFormatMapping(desc.Format);
	if (this->m_swapChain.SwapChain == nullptr)
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

		hr = this->m_factory->CreateSwapChainForHwnd(
			this->GetGfxQueue().Queue.Get(),
			hwnd,
			&swapChainDesc,
			&fullscreenDesc,
			nullptr,
			this->m_swapChain.SwapChain.GetAddressOf()
		);

		if (FAILED(hr))
		{
			throw std::exception();
		}

		hr = this->m_swapChain.SwapChain.As(&this->m_swapChain.SwapChain4);
		if (FAILED(hr))
		{
			throw std::exception();
		}
	}
	else
	{
		// Resize swapchain:
		this->WaitForIdle();

		// Delete back buffers
		this->m_swapChain.Rtv.Free();
		for (auto& backBuffer : this->m_swapChain.BackBuffers)
		{
			backBuffer.Reset();
		}

		hr = this->m_swapChain.SwapChain->ResizeBuffers(
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

	this->m_swapChain.Rtv = this->m_cpuDescriptorHeaps[DescriptorHeapTypes::RTV].Allocate(kBufferCount);
	for (UINT i = 0; i < kBufferCount; i++)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource>& backBuffer = this->m_swapChain.BackBuffers[i];
		ThrowIfFailed(
			this->m_swapChain.SwapChain4->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		char allocatorName[32];
		sprintf_s(allocatorName, "Back Buffer %iu", i);

		this->GetD3D12Device()->CreateRenderTargetView(backBuffer.Get(), nullptr, this->m_swapChain.Rtv.GetCpuHandle(i));
	}
}
