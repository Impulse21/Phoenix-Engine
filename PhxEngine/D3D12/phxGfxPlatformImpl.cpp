#include "pch.h"

#include <deque>
#include <functional>

#include "phxCommandLineArgs.h"
#include "phxEnumUtils.h"
#include "phxGfxPlatformImpl.h"
#include "phxGfxPlatformCommon.h"

#include "phxGfxHandlePool.h"


using namespace phx;
using namespace phx::gfx;
using namespace phx::gfx::dx;

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

// Teir 1 limit is 1,000,000
// https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
#define TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE 1000000

#define NUM_BINDLESS_RESOURCES TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE / 2

#define ENABLE_PIX_CAP


namespace phx::EngineCore { extern HWND g_hWnd; }

namespace
{
	static const GUID RenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
	static const GUID PixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

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

			ThrowIfFailed(
				nativeDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&Queue)));

			// Create Fence
			ThrowIfFailed(
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

	D3D12_FEATURE_DATA_ROOT_SIGNATURE FeatureDataRootSignature = {};
	D3D12_FEATURE_DATA_SHADER_MODEL   FeatureDataShaderModel = {};
	ShaderModel m_minShaderModel = ShaderModel::SM_6_0;

	D3D12Adapter m_gpuAdapter;

	phx::gfx::HandlePool<platform::D3D12SwapChain, PlatformSwapChain> m_poolSwapChain;
	Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
	bool IsUnderGraphicsDebugger = false;

	EnumArray<CommandQueueType, D3D12CommandQueue> m_CommandQueues;
	// -- Descriptor Heaps ---
	EnumArray<platform::DescriptorHeapTypes, d3d12::CpuDescriptorHeap> m_CpuDescriptorHeaps;
	std::array<d3d12::GpuDescriptorHeap, 2> m_GpuDescriptorHeaps;

	std::array<EnumArray<CommandQueueType, ComPtr<ID3D12Fence>>, platform::MaxNumInflightFames> m_FrameFences;
	struct DeleteItem
	{
		uint64_t Frame;
		std::function<void()> DeleteFn;
	};
	std::deque<DeleteItem> m_DeleteQueue;
	uint32_t m_FrameCount = 0;
}


namespace phx::gfx
{
	namespace platform
	{
		Microsoft::WRL::ComPtr<ID3D12Device> g_Device;
		Microsoft::WRL::ComPtr<ID3D12Device2> g_Device2;
		Microsoft::WRL::ComPtr<ID3D12Device5> g_Device5;
		DeviceCapability g_Capabilities;
	}
}

namespace
{

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

	Microsoft::WRL::ComPtr<IDXGIFactory6> CreateDXGIFactory6(bool enableDebug)
	{
		uint32_t flags = 0;
		if (enableDebug)
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

	void InitializeD3D12Context()
	{
		using namespace platform;
		{
			uint32_t useDebugLayers = 0;
#if _DEBUG
			// Default to true for debug builds
			useDebugLayers = 1;
#endif
			CommandLineArgs::GetInteger(L"debug", useDebugLayers);
			IsUnderGraphicsDebugger = (bool)useDebugLayers;
		}

		m_factory = CreateDXGIFactory6(IsUnderGraphicsDebugger);
		FindAdapter(m_factory, m_gpuAdapter);

		if (!m_gpuAdapter.NativeAdapter)
		{
			PHX_CORE_ERROR("Unable to create D3D12 RHI On current platform.");
		}

		ThrowIfFailed(
			D3D12CreateDevice(
				m_gpuAdapter.NativeAdapter.Get(),
				D3D_FEATURE_LEVEL_11_1,
				IID_PPV_ARGS(&g_Device)));

		g_Device->SetName(L"D3D12GfxDevice::RootDevice");
		Microsoft::WRL::ComPtr<IUnknown> renderdoc;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, RenderdocUUID, &renderdoc)))
		{
			IsUnderGraphicsDebugger |= !!renderdoc;
		}

		Microsoft::WRL::ComPtr<IUnknown> pix;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, PixUUID, &pix)))
		{
			IsUnderGraphicsDebugger |= !!pix;
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS featureOpptions = {};
		bool hasOptions = SUCCEEDED(g_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOpptions, sizeof(featureOpptions)));

		if (hasOptions)
		{
			if (featureOpptions.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation)
			{
				g_Capabilities |= DeviceCapability::RT_VT_ArrayIndex_Without_GS;
			}
		}

		// TODO: Move to acability array
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport5 = {};
		bool hasOptions5 = SUCCEEDED(g_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport5, sizeof(featureSupport5)));

		if (SUCCEEDED(g_Device.As(&g_Device5)) && hasOptions5)
		{
			if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
			{
				g_Capabilities |= DeviceCapability::RayTracing;
			}
			if (featureSupport5.RenderPassesTier >= D3D12_RENDER_PASS_TIER_0)
			{
				g_Capabilities |= DeviceCapability::RenderPass;
			}
			if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1)
			{
				g_Capabilities |= DeviceCapability::RayQuery;
			}
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureSupport6 = {};
		bool hasOptions6 = SUCCEEDED(g_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureSupport6, sizeof(featureSupport6)));

		if (hasOptions6)
		{
			if (featureSupport6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
			{
				g_Capabilities |= DeviceCapability::VariableRateShading;
			}
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupport7 = {};
		bool hasOptions7 = SUCCEEDED(g_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureSupport7, sizeof(featureSupport7)));

		if (SUCCEEDED(g_Device.As(&g_Device2)) && hasOptions7)
		{
			if (featureSupport7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1)
			{
				g_Capabilities |= DeviceCapability::MeshShading;
			}
			g_Capabilities |= DeviceCapability::CreateNoteZeroed;
		}

		FeatureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(g_Device2->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &FeatureDataRootSignature, sizeof(FeatureDataRootSignature))))
		{
			FeatureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		// Check shader model support
		FeatureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
		m_minShaderModel = ShaderModel::SM_6_6;
		if (FAILED(g_Device2->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &FeatureDataShaderModel, sizeof(FeatureDataShaderModel))))
		{
			FeatureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_5;
			m_minShaderModel = ShaderModel::SM_6_5;
		}


		static const bool debugEnabled = IsDebuggerPresent();
		if (debugEnabled)
		{
			Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
			if (SUCCEEDED(g_Device->QueryInterface<ID3D12InfoQueue>(&infoQueue)))
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

		for (auto& frame : m_FrameFences)
		{
			for (size_t q = 0; q < (size_t)CommandQueueType::Count; q++)
			{
				g_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frame[q]));
			}
		}

	}

	void RunGarbageCollection(uint64_t completedFrame = ~0lu)
	{
		while (!m_DeleteQueue.empty())
		{
			DeleteItem& deleteItem = m_DeleteQueue.front();
			if (deleteItem.Frame + platform::MaxNumInflightFames < completedFrame)
			{
				deleteItem.DeleteFn();
				m_DeleteQueue.pop_front();
			}
			else
			{
				break;
			}
		}
	}
}

namespace phx::gfx
{
	namespace platform
	{
		void Initialize()
		{
			m_poolSwapChain.Initialize(1);

			InitializeD3D12Context();

			// Create Queues
			m_CommandQueues[CommandQueueType::Graphics].Initialize(g_Device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
			m_CommandQueues[CommandQueueType::Compute].Initialize(g_Device.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE);
			m_CommandQueues[CommandQueueType::Copy].Initialize(g_Device.Get(), D3D12_COMMAND_LIST_TYPE_COPY);

			// Create Descriptor Heaps
			m_CpuDescriptorHeaps[DescriptorHeapTypes::CBV_SRV_UAV].Initialize(
				g_Device2,
				1024,
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			m_CpuDescriptorHeaps[DescriptorHeapTypes::Sampler].Initialize(
				g_Device2,
				1024,
				D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

			m_CpuDescriptorHeaps[DescriptorHeapTypes::RTV].Initialize(
				g_Device2,
				1024,
				D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			m_CpuDescriptorHeaps[DescriptorHeapTypes::DSV].Initialize(
				g_Device2,
				1024,
				D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


			m_GpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV].Initialize(
				g_Device2,
				NUM_BINDLESS_RESOURCES,
				TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE - NUM_BINDLESS_RESOURCES,
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);


			m_GpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler].Initialize(
				g_Device2,
				10,
				100,
				D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
				D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		}

		void Finalize()
		{
			platform::IdleGpu();

			m_poolSwapChain.Finalize();
		}

		void IdleGpu()
		{
			Microsoft::WRL::ComPtr<ID3D12Fence> fence;
			HRESULT hr = g_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
			assert(SUCCEEDED(hr));

			for (size_t q = 0; q < (size_t)CommandQueueType::Count; ++q)
			{
				D3D12CommandQueue& queue = m_CommandQueues[q];
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

		void SubmitFrame(SwapChainHandle handle)
		{

			if (!m_poolSwapChain.Contains(handle))
				return;

			const D3D12SwapChain& swapChain = *m_poolSwapChain.Get(handle);

			{
				const size_t backBufferIndex = swapChain.NativeSwapchain4->GetCurrentBackBufferIndex();
				// -- Mark queues for Compleition ---
				for (size_t q = 0; q < (size_t)CommandQueueType::Count; q++)
				{
					D3D12CommandQueue& queue = m_CommandQueues[q];
					queue.Queue->Signal(m_FrameFences[backBufferIndex][q].Get(), 1);
				}
			}

			bool enableVSync = false;
			UINT presentFlags = 0;
			if (!enableVSync)
			{
				presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
			}

			HRESULT hr = swapChain.NativeSwapchain4->Present((UINT)enableVSync, presentFlags);

			if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
			{
				assert(false);
			}

			m_FrameCount++;

			// -- wait for fence to finish
			{
				const size_t backBufferIndex = swapChain.NativeSwapchain4->GetCurrentBackBufferIndex();

				// Sync The queeus
				for (size_t q = 0; q < (size_t)CommandQueueType::Count; q++)
				{
					ID3D12Fence* fence = m_FrameFences[backBufferIndex][q].Get();
					const size_t completedValue = fence->GetCompletedValue();

					if (m_FrameCount >= platform::MaxNumInflightFames && completedValue < 1)
					{
						dx::ThrowIfFailed(
							fence->SetEventOnCompletion(1, NULL));
					}
					// Reset fence;
					fence->Signal(0);
				}
			}

			// I don't believe this will work.
			RunGarbageCollection(m_FrameCount);
		}
	}
}

void phx::gfx::platform::ResourceManger::CreateSwapChain(SwapChainDesc const& desc, SwapChainHandle& handle)
{
	HRESULT hr;

	UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	D3D12SwapChain* swapChain = nullptr;
	const auto& formatMapping = GetDxgiFormatMapping(desc.Format);
	if (handle.IsValid())
	{
		// Resize swapchain:
		IdleGpu();
		RunGarbageCollection();

		swapChain = m_poolSwapChain.Get(handle);
		swapChain->DescriptorAlloc.Free();

		// Delete back buffers
		for (auto& backBuffer : swapChain->BackBuffers)
		{
			backBuffer.Reset();
		}

		//Release Views
		hr = swapChain->NativeSwapchain4->ResizeBuffers(
			desc.BufferCount,
			desc.Width,
			desc.Height,
			formatMapping.RtvFormat,
			swapChainFlags
		);

		assert(SUCCEEDED(hr));
	}
	else
	{
		handle = m_poolSwapChain.Emplace();
		swapChain = m_poolSwapChain.Get(handle);

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

		hr = m_factory->CreateSwapChainForHwnd(
			m_CommandQueues[CommandQueueType::Graphics].Queue.Get(),
			static_cast<HWND>(phx::EngineCore::g_hWnd),
			&swapChainDesc,
			&fullscreenDesc,
			nullptr,
			swapChain->NativeSwapchain.GetAddressOf()
		);

		if (FAILED(hr))
		{
			throw std::exception();
		}

		hr = swapChain->NativeSwapchain.As(&swapChain->NativeSwapchain4);
		if (FAILED(hr))
		{
			throw std::exception();
		}
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

	swapChain->DescriptorAlloc = m_CpuDescriptorHeaps[DescriptorHeapTypes::RTV].Allocate(MaxNumInflightFames);
	for (UINT i = 0; i < desc.BufferCount; i++)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource>& backBuffer = swapChain->BackBuffers[i];
		ThrowIfFailed(
			swapChain->NativeSwapchain4->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		char allocatorName[32];
		sprintf_s(allocatorName, "Back Buffer %iu", i);

		g_Device->CreateRenderTargetView(
			backBuffer.Get(),
			nullptr,
			swapChain->DescriptorAlloc.GetCpuHandle(i));
	}
}

void phx::gfx::platform::ResourceManger::Release(SwapChainHandle handle)
{
	if (!handle.IsValid())
	{
		return;
	}

	DeleteItem d =
	{
		m_FrameCount,
		[=]()
		{
			D3D12SwapChain* impl = m_poolSwapChain.Get(handle);

			if (!impl)
				return;

			m_poolSwapChain.Release(handle);
		}
	};

	m_DeleteQueue.push_back(d);
}
