#include "D3D12Context.h"

#include <RHI/RHIEnums.h>
#include <Core/String.h>

static const GUID RenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
static const GUID PixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

// DirectX Aligily SDK
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 608; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }


using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;


static D3D12Adapter							D3D12Context::GpuAdapter = {};
static RefCountPtr<IDXGIFactory6>			D3D12Context::DxgiFctory6;
static RefCountPtr<ID3D12Device>			D3D12Context::D3D12Device;
static RefCountPtr<ID3D12Device2>			D3D12Context::D3D12Device2;
static RefCountPtr<ID3D12Device5>			D3D12Context::D3D12Device5;

static RHICapability						D3D12Context::RhiCapabilities;
static D3D12_FEATURE_DATA_ROOT_SIGNATURE	D3D12Context::FeatureDataRootSignature = {};
static D3D12_FEATURE_DATA_SHADER_MODEL		D3D12Context::FeatureDataShaderModel = {};

static ShaderModel							D3D12Context::MinShaderModel = ShaderModel::SM_6_0;
static bool									D3D12Context::IsUnderGraphicsDebugger = false;

namespace
{
	static const bool sDebugEnabled = IsDebuggerPresent();

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

	void CreateDeviceResources()
	{

	}
}

void PhxEngine::RHI::D3D12::D3D12Context::Initialize(D3D12Adapter const& gpuAdapter)
{
	GpuAdapter = gpuAdapter;
	DxgiFctory6 = CreateDXGIFactory6();

	ThrowIfFailed(
		D3D12CreateDevice(
			GpuAdapter.NativeAdapter,
			D3D_FEATURE_LEVEL_11_1,
			IID_PPV_ARGS(&D3D12Device)));

	D3D12Device->SetName(L"RHI::D3D12::D3D12Device");
	{
		// Ref counter isn't working so use ComPtr for this - to lazy to fight with
		// this right now.
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
	}

	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS featureOpptions = {};

		bool hasOptions = SUCCEEDED(D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOpptions, sizeof(featureOpptions)));

		if (hasOptions)
		{
			if (featureOpptions.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation)
			{
				RhiCapabilities |= RHICapability::RT_VT_ArrayIndex_Without_GS;
			}
		}

		// TODO: Move to acability array
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport5 = {};
		bool hasOptions5 = SUCCEEDED(D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport5, sizeof(featureSupport5)));

		if (SUCCEEDED(D3D12Device->QueryInterface(IID_PPV_ARGS(&D3D12Device5))) && hasOptions5)
		{
			if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
			{
				RhiCapabilities |= RHICapability::RayTracing;
			}
			if (featureSupport5.RenderPassesTier >= D3D12_RENDER_PASS_TIER_0)
			{
				RhiCapabilities |= RHICapability::RenderPass;
			}
			if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1)
			{
				RhiCapabilities |= RHICapability::RayQuery;
			}
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureSupport6 = {};
		bool hasOptions6 = SUCCEEDED(D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureSupport6, sizeof(featureSupport6)));
		if (hasOptions6)
		{
			if (featureSupport6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
			{
				RhiCapabilities |= RHICapability::VariableRateShading;
			}
		}

		D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupport7 = {};
		bool hasOptions7 = SUCCEEDED(D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureSupport7, sizeof(featureSupport7)));

		if (SUCCEEDED(D3D12Device->QueryInterface(&D3D12Device2)) && hasOptions7)
		{
			if (featureSupport7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1)
			{
				RhiCapabilities |= RHICapability::MeshShading;
			}
			RhiCapabilities |= RHICapability::CreateNoteZeroed;
		}

		FeatureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(D3D12Device2->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &FeatureDataRootSignature, sizeof(FeatureDataRootSignature))))
		{
			FeatureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		// Check shader model support
		FeatureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
		MinShaderModel = ShaderModel::SM_6_6;
		if (FAILED(D3D12Device2->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &FeatureDataShaderModel, sizeof(FeatureDataShaderModel))))
		{
			FeatureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_5;
			MinShaderModel = ShaderModel::SM_6_5;
		}
	}

	if (sDebugEnabled)
	{
		RefCountPtr<ID3D12InfoQueue1> infoQueue;
		if (SUCCEEDED(D3D12Device->QueryInterface<ID3D12InfoQueue1>(&infoQueue)))
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
}

void PhxEngine::RHI::D3D12::D3D12Context::Finalize()
{
	assert(false, "PROCESS DELETE QUEUE PLEASE");
	RefCountPtr<ID3D12InfoQueue1> infoQueue;
	if (SUCCEEDED(D3D12Device->QueryInterface<ID3D12InfoQueue1>(&infoQueue)))
	{
		infoQueue->UnregisterMessageCallback(CallbackCookie);
	}

	DxgiFctory6.Reset();
	D3D12Device.Reset();
	D3D12Device2.Reset();
	D3D12Device5.Reset();
	GpuAdapter = {};
}
