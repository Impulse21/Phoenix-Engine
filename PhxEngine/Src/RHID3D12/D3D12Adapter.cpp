#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "D3D12Adapter.h"

#include "D3D12CommandQueue.h"
#include "D3D12Device.h"

using namespace PhxEngine::RHI::D3D12;

namespace
{
}

PhxEngine::RHI::D3D12::D3D12Adapter::D3D12Adapter(D3D12AdapterDesc const& descIn, RefCountPtr<IDXGIAdapter1>& dxgiAdapter)
	: m_dxgiAdapter(dxgiAdapter)
	, m_desc(descIn)
{
}

void PhxEngine::RHI::D3D12::D3D12Adapter::InitializeD3D12Devices()
{
	ThrowIfFailed(
		D3D12CreateDevice(
			this->GetDxgiAdapter(),
			D3D_FEATURE_LEVEL_11_1,
			IID_PPV_ARGS(&this->m_rootDevice)));

	this->m_rootDevice->GetNodeCount();

	D3D12_FEATURE_DATA_D3D12_OPTIONS featureOpptions = {};
	bool hasOptions = SUCCEEDED(this->m_rootDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOpptions, sizeof(featureOpptions)));

	if (hasOptions)
	{
		if (featureOpptions.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation)
		{
			this->m_capabilities |= DeviceCapability::RT_VT_ArrayIndex_Without_GS;
		}
	}

	// TODO: Move to acability array
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport5 = {};
	bool hasOptions5 = SUCCEEDED(this->m_rootDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport5, sizeof(featureSupport5)));

	if (SUCCEEDED(this->m_rootDevice->QueryInterface(IID_PPV_ARGS(this->m_rootDevice5.ReleaseAndGetAddressOf()))))
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
	bool hasOptions6 = SUCCEEDED(this->m_rootDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureSupport6, sizeof(featureSupport6)));

	if (hasOptions6)
	{
		if (featureSupport6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
		{
			this->m_capabilities |= DeviceCapability::VariableRateShading;
		}
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupport7 = {};
	bool hasOptions7 = SUCCEEDED(this->m_rootDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureSupport7, sizeof(featureSupport7)));

	if (SUCCEEDED(this->m_rootDevice->QueryInterface(IID_PPV_ARGS(this->m_rootDevice2.ReleaseAndGetAddressOf())) && hasOptions7))
	{
		if (featureSupport7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1)
		{
			this->m_capabilities |= DeviceCapability::MeshShading;
		}
		this->m_capabilities |= DeviceCapability::CreateNoteZeroed;
	}

	this->m_featureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(this->m_rootDevice2->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &this->m_featureDataRootSignature, sizeof(this->m_featureDataRootSignature))))
	{
		this->m_featureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Check shader model support
	this->m_featureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
	this->m_minShaderModel = ShaderModel::SM_6_6;
	if (FAILED(this->m_rootDevice2->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &this->m_featureDataShaderModel, sizeof(this->m_featureDataShaderModel))))
	{
		this->m_featureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_5;
		this->m_minShaderModel = ShaderModel::SM_6_5;
	}
	else
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS d3d12DeviceCapability{};
		this->m_rootDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &d3d12DeviceCapability, sizeof(d3d12DeviceCapability));

		if (d3d12DeviceCapability.ResourceBindingTier >= D3D12_RESOURCE_BINDING_TIER_3)
		{
			this->m_capabilities |= DeviceCapability::Bindless;
		}
	}


	static const bool debugEnabled = IsDebuggerPresent();
	if (debugEnabled)
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
		if (SUCCEEDED(this->m_rootDevice->QueryInterface<ID3D12InfoQueue>(&infoQueue)))
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

	this->m_device = std::make_unique<D3D12Device>(0, this);

}
