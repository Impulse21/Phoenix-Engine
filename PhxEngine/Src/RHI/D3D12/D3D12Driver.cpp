#include "D3D12Driver.h"

#include "D3D12Native.h"
#include <PhxEngine/Core/Log.h>

using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;

namespace
{
	static const GUID RenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
	static const GUID PixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

}

void PhxEngine::RHI::D3D12::D3D12Driver::Initialize()
{
	this->InitializeNativeD3D12Resources();

	this->GetGfxQueue().Initialize(this->GetD3D12Device().Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	this->GetComputeQueue().Initialize(this->GetD3D12Device().Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE);
	this->GetCopyQueue().Initialize(this->GetD3D12Device().Get(), D3D12_COMMAND_LIST_TYPE_COPY);

}

void PhxEngine::RHI::D3D12::D3D12Driver::Finialize()
{
	this->WaitForIdle();

	this->GetGfxQueue().Finailize();
	this->GetComputeQueue().Finailize();
	this->GetCopyQueue().Finailize();

}
void PhxEngine::RHI::D3D12::D3D12Driver::WaitForIdle()
{
	assert(false);
}

bool PhxEngine::RHI::D3D12::D3D12Driver::CreateSwapChain(SwapChainDesc const& desc, void* windowHandle, SwapChain& swapchain)
{
	auto rhiResource = std::static_pointer_cast<D3D12SwapChain>(swapchain.RHIResource);
	if (swapchain.RHIResource == nullptr)
	{
		rhiResource = std::make_shared<D3D12SwapChain>();
	}

	swapchain.RHIResource = std::make_shared<D3D12SwapChain>();
	swapchain.Desc = desc;

	UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	HRESULT hr;
	auto& mapping = GetDxgiFormatMapping(desc.Format);
	if (rhiResource->Native == nullptr)
	{

		DXGI_SWAP_CHAIN_DESC1 dx12Desc = {};
		dx12Desc.Width = desc.Width;
		dx12Desc.Height = desc.Height;
		dx12Desc.Format = mapping.RtvFormat;
		dx12Desc.Stereo = false;
		dx12Desc.SampleDesc.Count = 1;
		dx12Desc.SampleDesc.Quality = 0;
		dx12Desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		dx12Desc.BufferCount = desc.BufferCount;
		dx12Desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		dx12Desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		dx12Desc.Flags = swapChainFlags;

		dx12Desc.Scaling = DXGI_SCALING_STRETCH;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
		fullscreenDesc.Windowed = !desc.Fullscreen;

		Microsoft::WRL::ComPtr<IDXGISwapChain1> tempSwapChain;
		hr = this->m_factory->CreateSwapChainForHwnd(
			this->GetGfxQueue().GetD3D12CommandQueue(),
			static_cast<HWND>(windowHandle),
			&dx12Desc,
			&fullscreenDesc,
			nullptr,
			tempSwapChain.GetAddressOf());

		if (FAILED(hr))
		{
			// LOG_RHI_ERROR("Failed to create swapchian");
			return false;
		}

		hr = tempSwapChain.As(&rhiResource->Native);
		if (FAILED(hr))
		{
			// LOG_RHI_ERROR("Failed to create swapchian");
			return false;
		}
	}
	else
	{
		// Handle resize
		this->WaitForIdle();
		rhiResource->BackBuffers.clear();
		for (auto& x : rhiResource->BackbufferRTV)
		{
			assert(false); // TODO: Free RTVs
			// allocationhandler->descriptors_rtv.free(x);
		}
		rhiResource->BackbufferRTV.clear();

		hr = rhiResource->Native->ResizeBuffers(
			desc.BufferCount,
			desc.Width,
			desc.Height,
			mapping.RtvFormat,
			swapChainFlags
		);
		assert(SUCCEEDED(hr));
	}

	rhiResource->BackbufferRTV.resize(desc.BufferCount);
	rhiResource->BackbufferRTV.resize(desc.BufferCount);

	// We can create swapchain just with given supported format, thats why we specify format in RTV
	// For example: BGRA8UNorm for SwapChain BGRA8UNormSrgb for RTV.
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = mapping.RtvFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	for (uint32_t i = 0; i < desc.BufferCount; ++i)
	{
		hr = rhiResource->Native->GetBuffer(i, IID_PPV_ARGS(&rhiResource->BackBuffers[i]));
		assert(SUCCEEDED(hr));

		assert(false); // TODO: allocate Descriptor
		// rhiResource->BackbufferRTV[i] = allocationhandler->descriptors_rtv.allocate();
		this->m_rootDevice->CreateRenderTargetView(rhiResource->BackBuffers[i].Get(), &rtvDesc, rhiResource->BackbufferRTV[i]);
	}

	return true;
}

void PhxEngine::RHI::D3D12::D3D12Driver::InitializeNativeD3D12Resources()
{
	this->m_factory = CreateDXGIFactory6();
	FindAdapter(this->m_factory, this->m_gpuDevice);

	if (!this->m_gpuDevice.NativeAdapter)
	{
		// LOG_CORE_ERROR("Unable to create D3D12 RHI On current platform.");
	}

	ThrowIfFailed(
		D3D12CreateDevice(
			this->m_gpuDevice.NativeAdapter.Get(),
			D3D_FEATURE_LEVEL_11_1,
			IID_PPV_ARGS(&this->m_rootDevice)));

	this->m_rootDevice->SetName(L"D3D12GraphicsDevice::RootDevice");
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
	bool hasOptions = SUCCEEDED(this->m_rootDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOpptions, sizeof(featureOpptions)));

	if (hasOptions)
	{
		if (featureOpptions.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation)
		{
			this->m_capabilities |= Capability::RT_VT_ArrayIndex_Without_GS;
		}
	}

	// TODO: Move to acability array
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport5 = {};
	bool hasOptions5 = SUCCEEDED(this->m_rootDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport5, sizeof(featureSupport5)));

	if (SUCCEEDED(this->m_rootDevice.As(&this->m_rootDevice5)) && hasOptions5)
	{
		if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
		{
			this->m_capabilities |= Capability::RayTracing;
		}
		if (featureSupport5.RenderPassesTier >= D3D12_RENDER_PASS_TIER_0)
		{
			this->m_capabilities |= Capability::RenderPass;
		}
		if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1)
		{
			this->m_capabilities |= Capability::RayQuery;
		}
	}


	D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureSupport6 = {};
	bool hasOptions6 = SUCCEEDED(this->m_rootDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureSupport6, sizeof(featureSupport6)));

	if (hasOptions6)
	{
		if (featureSupport6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
		{
			this->m_capabilities |= Capability::VariableRateShading;
		}
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupport7 = {};
	bool hasOptions7 = SUCCEEDED(this->m_rootDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureSupport7, sizeof(featureSupport7)));

	if (SUCCEEDED(this->m_rootDevice.As(&this->m_rootDevice2)) && hasOptions7)
	{
		if (featureSupport7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1)
		{
			this->m_capabilities |= Capability::MeshShading;
		}
		this->m_capabilities |= Capability::CreateNoteZeroed;
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
}
