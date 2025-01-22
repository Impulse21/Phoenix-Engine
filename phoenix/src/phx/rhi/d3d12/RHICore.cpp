#include "phxpch.h"

#include "phx/core/StringUtils.h"
#include "phx/core/CommandLineArgs.h"

#include "phx/rhi/RHICore.h"
#include "D3D12Types.h"
#include "D3D12Core.h"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-function"
#endif

// Teir 1 limit is 1,000,000
// https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
#define TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE 1000000

#define NUM_BINDLESS_RESOURCES TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE / 2

using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::d3d12;
using namespace Microsoft::WRL;

namespace
{
	const GUID kRenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
	const GUID kPixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

	bool m_debugLayersEnabled = false;
	rhi::DeviceCapability m_capabilities;

	D3D12_FEATURE_DATA_ROOT_SIGNATURE m_featureDataRootSignature = {};
	D3D12_FEATURE_DATA_SHADER_MODEL   m_featureDataShaderModel = {};
	ShaderModel m_minShaderModel = ShaderModel::SM_6_0;
	D3D12Adapter m_adapter;
}

namespace phx::rhi::d3d12
{
	Microsoft::WRL::ComPtr<IDXGIFactory6> g_dxgiFactory;
	Microsoft::WRL::ComPtr<ID3D12Device> g_d3d12Device;
	Microsoft::WRL::ComPtr<ID3D12Device2> g_d3d12Device2;
	Microsoft::WRL::ComPtr<ID3D12Device5> g_d3d12Device5;
	Microsoft::WRL::ComPtr<D3D12MA::Allocator> g_d3d12MemAllocator;
	bool g_isUnderGfxDebugger;
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

	ComPtr<ID3D12RootSignature> CreateRootSignature(phx::Span<uint8_t> byteCode)
	{
		HRESULT hr = (byteCode.IsEmpty() ? E_FAIL : S_OK);
		assert(SUCCEEDED(hr));

		ComPtr<ID3D12RootSignature> rootSig;
		ComPtr<ID3D12VersionedRootSignatureDeserializer> rootsigDeserializer;
		hr = D3D12CreateVersionedRootSignatureDeserializer(
			byteCode.data(),
			byteCode.size(),
			IID_PPV_ARGS(rootsigDeserializer.ReleaseAndGetAddressOf()));

		if (SUCCEEDED(hr))
		{
			const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* rootsigDesc = nullptr;
			hr = rootsigDeserializer->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_1, &rootsigDesc);
			if (SUCCEEDED(hr))
			{
				assert(rootsigDesc->Version == D3D_ROOT_SIGNATURE_VERSION_1_1);

				hr = g_d3d12Device2->CreateRootSignature(
					0,
					byteCode.data(),
					byteCode.size(),
					IID_PPV_ARGS(rootSig.ReleaseAndGetAddressOf())
				);
				assert(SUCCEEDED(hr));
			}
		}

		return rootSig;
	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateEmptyRootSignature()
	{
		using namespace Microsoft::WRL;

		// Define an empty root signature
		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.NumParameters = 0;         // No root parameters
		rootSignatureDesc.pParameters = nullptr;
		rootSignatureDesc.NumStaticSamplers = 0;     // No static samplers
		rootSignatureDesc.pStaticSamplers = nullptr;
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// Serialize the root signature
		ComPtr<ID3DBlob> serializedRootSignature;
		ComPtr<ID3DBlob> errorBlob; // To capture any errors
		HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&serializedRootSignature,
			&errorBlob);

		if (FAILED(hr)) {
			if (errorBlob) {
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			}
			throw std::runtime_error("Failed to serialize root signature");
		}

		// Create the root signature
		ComPtr<ID3D12RootSignature> rootSignature;
		hr = g_d3d12Device->CreateRootSignature(
			0,
			serializedRootSignature->GetBufferPointer(),
			serializedRootSignature->GetBufferSize(),
			IID_PPV_ARGS(&rootSignature));

		if (FAILED(hr)) {
			throw std::runtime_error("Failed to create root signature");
		}

		return rootSignature;
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
			m_debugLayersEnabled = (bool)useDebugLayers;
		}

		g_dxgiFactory = CreateDXGIFactory6(m_debugLayersEnabled);
		FindAdapter(g_dxgiFactory, m_adapter);

		if (!m_adapter.NativeAdapter)
		{
			// LOG_CORE_ERROR("Unable to create D3D12 RHI On current platform.");
		}

		ThrowIfFailed(
			D3D12CreateDevice(
				m_adapter.NativeAdapter.Get(),
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

		D3D12_FEATURE_DATA_D3D12_OPTIONS featureOpptions = {};
		bool hasOptions = SUCCEEDED(g_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOpptions, sizeof(featureOpptions)));

		if (hasOptions)
		{
			if (featureOpptions.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation)
			{
				m_capabilities |= DeviceCapability::RT_VT_ArrayIndex_Without_GS;
			}
		}

		// TODO: Move to acability array
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport5 = {};
		bool hasOptions5 = SUCCEEDED(g_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport5, sizeof(featureSupport5)));

		if (SUCCEEDED(g_d3d12Device.As(&g_d3d12Device5)) && hasOptions5)
		{
			if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
			{
				m_capabilities |= DeviceCapability::RayTracing;
			}
			if (featureSupport5.RenderPassesTier >= D3D12_RENDER_PASS_TIER_0)
			{
				m_capabilities |= DeviceCapability::RenderPass;
			}
			if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1)
			{
				m_capabilities |= DeviceCapability::RayQuery;
			}
		}


		D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureSupport6 = {};
		bool hasOptions6 = SUCCEEDED(g_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureSupport6, sizeof(featureSupport6)));

		if (hasOptions6)
		{
			if (featureSupport6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
			{
				m_capabilities |= DeviceCapability::VariableRateShading;
			}
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupport7 = {};
		bool hasOptions7 = SUCCEEDED(g_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureSupport7, sizeof(featureSupport7)));

		if (SUCCEEDED(g_d3d12Device.As(&g_d3d12Device2)) && hasOptions7)
		{
			if (featureSupport7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1)
			{
				m_capabilities |= DeviceCapability::MeshShading;
			}
			m_capabilities |= DeviceCapability::CreateNoteZeroed;
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

		if (m_debugLayersEnabled)
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
		}
	}
}

namespace phx::rhi
{

	void Initialize()
	{
		PHX_CORE_INFO("Initialize RHI(D3D12)");
		InitializeD3D12Context();
	}

	void Finalize()
	{

	}
}
