#include "GraphicsDevice.h"

#include <PhxEngine/Core/SafeCast.h>
#include <PhxEngine/Core/Defines.h>
#include <PhxEngine/Core/Asserts.h>

#include "CommandList.h"
using namespace PhxEngine::RHI;

static const GUID RenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
static const GUID PixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };


PhxEngine::RHI::Dx12::GraphicsDevice::GraphicsDevice()
	: m_frameCount(0)
{
	this->m_factory = this->CreateFactory();
	this->m_gpuAdapter = this->SelectOptimalGpu();
	this->CreateDevice(this->m_gpuAdapter);

	// Create Queues
	this->m_commandQueues[(int)CommandQueueType::Graphics] = std::make_unique<CommandQueue>(*this, D3D12_COMMAND_LIST_TYPE_DIRECT);
	this->m_commandQueues[(int)CommandQueueType::Compute] = std::make_unique<CommandQueue>(*this, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	this->m_commandQueues[(int)CommandQueueType::Copy] = std::make_unique<CommandQueue>(*this, D3D12_COMMAND_LIST_TYPE_COPY);

	// Create Descriptor Heaps
	this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV] =
		std::make_unique<CpuDescriptorHeap>(
			*this,
			1024,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler] =
		std::make_unique<CpuDescriptorHeap>(
			*this,
			1024,
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::RTV] =
		std::make_unique<CpuDescriptorHeap>(
			*this,
			1024,
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::DSV] =
		std::make_unique<CpuDescriptorHeap>(
			*this,
			1024,
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


	this->m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV] =
		std::make_unique<GpuDescriptorHeap>(
			*this,
			NUM_BINDLESS_RESOURCES,
			TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE - NUM_BINDLESS_RESOURCES,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);


	this->m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler] =
		std::make_unique<GpuDescriptorHeap>(
			*this,
			10,
			100,
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
}

PhxEngine::RHI::Dx12::GraphicsDevice::~GraphicsDevice()
{
	// TODO: this is to prevent an issue with dangling pointers. See DescriptorHeapAllocator
	this->m_swapChain.BackBuffers.clear();
}

void PhxEngine::RHI::Dx12::GraphicsDevice::WaitForIdle()
{
	for (auto& queue : this->m_commandQueues)
	{
		if (queue)
		{
			queue->WaitForIdle();
		}
	}
}

void PhxEngine::RHI::Dx12::GraphicsDevice::CreateSwapChain(SwapChainDesc const& swapChainDesc)
{
	PHX_ASSERT(swapChainDesc.WindowHandle);
	if (!swapChainDesc.WindowHandle)
	{
		LOG_CORE_ERROR("Invalid window handle");
		throw std::runtime_error("Invalid Window Error");
	}

	auto& mapping = GetDxgiFormatMapping(swapChainDesc.Format);

	DXGI_SWAP_CHAIN_DESC1 dx12Desc = {};
	dx12Desc.Width = swapChainDesc.Width;
	dx12Desc.Height = swapChainDesc.Height;
	dx12Desc.Format = mapping.rtvFormat;
	dx12Desc.SampleDesc.Count = 1;
	dx12Desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dx12Desc.BufferCount = swapChainDesc.BufferCount;
	dx12Desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dx12Desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	RefCountPtr<IDXGISwapChain1> tmpSwapChain;
	ThrowIfFailed(
		this->m_factory->CreateSwapChainForHwnd(
			this->GetGfxQueue()->GetD3D12CommandQueue(),
			static_cast<HWND>(swapChainDesc.WindowHandle),
			&dx12Desc,
			nullptr,
			nullptr,
			&tmpSwapChain));

	ThrowIfFailed(
		tmpSwapChain->QueryInterface(&this->m_swapChain.DxgiSwapchain));

	this->m_swapChain.Desc = swapChainDesc;

	this->m_swapChain.BackBuffers.resize(this->m_swapChain.Desc.BufferCount);
	this->m_frameFence.resize(this->m_swapChain.Desc.BufferCount, 0);
	for (UINT i = 0; i < this->m_swapChain.Desc.BufferCount; i++)
	{
		RefCountPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(
			this->m_swapChain.DxgiSwapchain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		auto textureDesc = TextureDesc();
		textureDesc.Dimension = TextureDimension::Texture2D;
		textureDesc.Format = this->m_swapChain.Desc.Format;
		textureDesc.Width = this->m_swapChain.Desc.Width;
		textureDesc.Height = this->m_swapChain.Desc.Height;

		char allocatorName[32];
		sprintf(allocatorName, "Back Buffer %iu", i);
		textureDesc.DebugName = std::string(allocatorName);

		this->m_swapChain.BackBuffers[i] = this->CreateRenderTarget(textureDesc, backBuffer);
	}
}

CommandListHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateCommandList(CommandListDesc const& desc)
{
	auto commandListImpl = std::make_unique<CommandList>(*this, desc);
	return CommandListHandle::Create(commandListImpl.release());
}

TextureHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateDepthStencil(TextureDesc const& desc)
{
	D3D12_CLEAR_VALUE d3d12OptimizedClearValue = {};
	auto dxgiFormatMapping = GetDxgiFormatMapping(desc.Format);
	if (desc.OptmizedClearValue.has_value())
	{
		d3d12OptimizedClearValue.Format = dxgiFormatMapping.rtvFormat;
		d3d12OptimizedClearValue.DepthStencil.Depth = desc.OptmizedClearValue.value().R;
		d3d12OptimizedClearValue.DepthStencil.Stencil = desc.OptmizedClearValue.value().G;
	}

	D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	RefCountPtr<ID3D12Resource> d3d12Resource;
	ThrowIfFailed(
		this->GetD3D12Device2()->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(
				dxgiFormatMapping.rtvFormat,
				desc.Width, 
				desc.Height,
				desc.ArraySize,
				desc.MipLevels,
				1,
				0,
				resourceFlags),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			desc.OptmizedClearValue.has_value() ? &d3d12OptimizedClearValue : nullptr,
			IID_PPV_ARGS(&d3d12Resource)));

	// Create DSV
	auto textureImpl = std::make_unique<Texture>();
	textureImpl->Desc = desc;
	textureImpl->D3D12Resource = d3d12Resource;
	textureImpl->DsvAllocation = this->GetDsvCpuHeap()->Allocate(1);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = dxgiFormatMapping.rtvFormat;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // TODO: use desc.
	dsvDesc.Texture2D.MipSlice = 0;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	this->GetD3D12Device2()->CreateDepthStencilView(
		textureImpl->D3D12Resource,
		&dsvDesc,
		textureImpl->DsvAllocation.GetCpuHandle());

	return TextureHandle::Create(textureImpl.release());
}

void PhxEngine::RHI::Dx12::GraphicsDevice::Present()
{
	this->m_swapChain.DxgiSwapchain->Present(0, 0);

	this->m_frameFence[this->GetCurrentBackBufferIndex()] = this->GetGfxQueue()->IncrementFence();
	this->m_frameCount++;

	// TODO: Add timers to see how long we are waiting for a fence to finish.
	this->GetGfxQueue()->WaitForFence(this->m_frameFence[this->GetCurrentBackBufferIndex()]);

	this->RunGarbageCollection();
}

uint64_t PhxEngine::RHI::Dx12::GraphicsDevice::ExecuteCommandLists(
	ICommandList* const* pCommandLists,
	size_t numCommandLists,
	bool waitForCompletion,
	CommandQueueType executionQueue)
{
	std::vector<ID3D12CommandList*> d3d12CommandLists(numCommandLists);
	for (size_t i = 0; i < numCommandLists; i++)
	{
		d3d12CommandLists[i] = SafeCast<CommandList*>(pCommandLists[i])->GetD3D12CommandList();
	}

	auto* queue = this->GetQueue(executionQueue);

	auto fenceValue = queue->ExecuteCommandLists(d3d12CommandLists);

	if (waitForCompletion)
	{
		queue->WaitForFence(fenceValue);

		for (size_t i = 0; i < numCommandLists; i++)
		{
			SafeCast<CommandList*>(pCommandLists[i])->Executed(fenceValue);
		}
	}
	else
	{
		for (size_t i = 0; i < numCommandLists; i++)
		{
			auto trackedResources = SafeCast<CommandList*>(pCommandLists[i])->Executed(fenceValue);
			InflightDataEntry entry = { fenceValue, trackedResources };
			this->m_inflightData[(int)executionQueue].emplace_back(entry);
		}
	}

	return fenceValue;
}

TextureHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateRenderTarget(
	TextureDesc const& desc,
	RefCountPtr<ID3D12Resource> d3d12TextureResource)
{
	// Construct Texture
	auto textureImpl = std::make_unique<Texture>();
	textureImpl->Desc = desc;
	textureImpl->D3D12Resource = d3d12TextureResource;
	textureImpl->RtvAllocation = this->GetRtvCpuHeap()->Allocate(1);

	this->GetD3D12Device2()->CreateRenderTargetView(
		textureImpl->D3D12Resource,
		nullptr,
		textureImpl->RtvAllocation.GetCpuHandle());

	// Set debug name
	std::wstring debugName(textureImpl->Desc.DebugName.begin(), textureImpl->Desc.DebugName.end());
	textureImpl->D3D12Resource->SetName(debugName.c_str());

	return TextureHandle::Create(textureImpl.release());
}

void PhxEngine::RHI::Dx12::GraphicsDevice::RunGarbageCollection()
{
	for (size_t i = 0; i < (size_t)CommandQueueType::Count; i++)
	{
		auto* queue = this->GetQueue((CommandQueueType)i);

		while (!this->m_inflightData[i].empty() && this->m_inflightData[i].front().LastSubmittedFenceValue <= queue->GetLastCompletedFence())
		{
			this->m_inflightData[i].pop_front();
		}
	}
}

RefCountPtr<IDXGIAdapter1> PhxEngine::RHI::Dx12::GraphicsDevice::SelectOptimalGpu()
{
	LOG_CORE_INFO("Selecting Optimal GPU");
	RefCountPtr<IDXGIAdapter1> selectedGpu;

	size_t selectedGPUVideoMemeory = 0;
	for (auto adapter : EnumerateAdapters(this->m_factory))
	{
		DXGI_ADAPTER_DESC desc = {};
		adapter->GetDesc(&desc);

		std::string name = NarrowString(desc.Description);
		size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
		size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
		size_t sharedSystemMemory = desc.SharedSystemMemory;

		LOG_CORE_INFO(
			"\t{0} [VRAM={1}MB, SRAM={2}MB, SharedRAM={3}MB]",
			name,
			BYTE_TO_MB(dedicatedVideoMemory),
			BYTE_TO_MB(dedicatedSystemMemory),
			BYTE_TO_MB(sharedSystemMemory));

		if (!selectedGpu || selectedGPUVideoMemeory < dedicatedVideoMemory)
		{
			selectedGpu = adapter;
			selectedGPUVideoMemeory = dedicatedVideoMemory;
		}
	}

	DXGI_ADAPTER_DESC desc = {};
	selectedGpu->GetDesc(&desc);

	std::string name = NarrowString(desc.Description);
	size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
	size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
	size_t sharedSystemMemory = desc.SharedSystemMemory;

	LOG_CORE_INFO(
		"Selected GPU {0} [VRAM={1}MB, SRAM={2}MB, SharedRAM={3}MB]",
		name,
		BYTE_TO_MB(dedicatedVideoMemory),
		BYTE_TO_MB(dedicatedSystemMemory),
		BYTE_TO_MB(sharedSystemMemory));

	return selectedGpu;
}

std::vector<RefCountPtr<IDXGIAdapter1>> PhxEngine::RHI::Dx12::GraphicsDevice::EnumerateAdapters(RefCountPtr<IDXGIFactory6> factory, bool includeSoftwareAdapter)
{
	std::vector<RefCountPtr<IDXGIAdapter1>> adapters;
	auto nextAdapter = [&](uint32_t adapterIndex, RefCountPtr<IDXGIAdapter1>& adapter)
	{
		return factory->EnumAdapterByGpuPreference(
			adapterIndex,
			DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
			IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()));
	};

	RefCountPtr<IDXGIAdapter1> adapter;
	uint32_t gpuIndex = 0;
	for (uint32_t adapterIndex = 0; DXGI_ERROR_NOT_FOUND != nextAdapter(adapterIndex, adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc = {};
		adapter->GetDesc1(&desc);
		if (!includeSoftwareAdapter && (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
			continue;

		adapters.push_back(adapter);
	}

	return adapters;
}

size_t PhxEngine::RHI::Dx12::GraphicsDevice::GetCurrentBackBufferIndex() const
{
	PHX_ASSERT(this->m_swapChain.DxgiSwapchain);
	
	return (size_t)this->m_swapChain.DxgiSwapchain->GetCurrentBackBufferIndex();
}

RefCountPtr<IDXGIFactory6> PhxEngine::RHI::Dx12::GraphicsDevice::CreateFactory() const
{
	uint32_t flags = 0;
	static const bool debugEnabled = IsDebuggerPresent();

	if (debugEnabled)
	{
		RefCountPtr<ID3D12Debug> debugController;
		ThrowIfFailed(
			D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));

		debugController->EnableDebugLayer();
		flags = DXGI_CREATE_FACTORY_DEBUG;
	}
	
	RefCountPtr<IDXGIFactory6> factory;
	ThrowIfFailed(
		CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory)));

	return factory;
}

void PhxEngine::RHI::Dx12::GraphicsDevice::CreateDevice(RefCountPtr<IDXGIAdapter> gpuAdapter)
{
	ThrowIfFailed(
		D3D12CreateDevice(
			gpuAdapter.Get(),
			D3D_FEATURE_LEVEL_11_1,
			IID_PPV_ARGS(&this->m_device)));

	/*
	RefCountPtr<IUnknown> renderdoc;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, RenderdocUUID, &renderdoc)))
	{
		this->IsUnderGraphicsDebugger |= !!renderdoc;
	}

	RefCountPtr<IUnknown> pix;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, PixUUID, &pix)))
	{
		this->IsUnderGraphicsDebugger |= !!pix;
	}
	*/

	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport5 = {};
	bool hasOptions5 = SUCCEEDED(this->m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport5, sizeof(featureSupport5)));

	D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureSupport6 = {};
	bool hasOptions6 = SUCCEEDED(this->m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureSupport6, sizeof(featureSupport6)));

	D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupport7 = {};
	bool hasOptions7 = SUCCEEDED(this->m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureSupport7, sizeof(featureSupport7)));

	if (SUCCEEDED(this->m_device->QueryInterface(&this->m_device5)) && hasOptions5)
	{
		this->IsDxrSupported = featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0;
		this->IsRenderPassSupported = featureSupport5.RenderPassesTier >= D3D12_RENDER_PASS_TIER_0;
		this->IsRayQuerySupported = featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1;
	}

	if (hasOptions6)
	{
		this->IsVariableRateShadingSupported = featureSupport6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2;
	}


	if (SUCCEEDED(this->m_device->QueryInterface(&this->m_device2)) && hasOptions7)
	{
		this->IsCreateNotZeroedAvailable = true;
		this->IsMeshShadingSupported = featureSupport7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1;
	}

	this->FeatureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(this->m_device2->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &this->FeatureDataRootSignature, sizeof(this->FeatureDataRootSignature))))
	{
		this->FeatureDataRootSignature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Check shader model support
	this->FeatureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
	if (FAILED(this->m_device2->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &this->FeatureDataShaderModel, sizeof(this->FeatureDataShaderModel))))
	{
		this->FeatureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_0;
	}


	static const bool debugEnabled = IsDebuggerPresent();
	if (debugEnabled)
	{
		RefCountPtr<ID3D12InfoQueue> infoQueue;
		if (SUCCEEDED(this->m_device->QueryInterface<ID3D12InfoQueue>(&infoQueue)))
		{
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

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
