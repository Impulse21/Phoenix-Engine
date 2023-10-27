#include "D3D12ResourceManager.h"

#include "D3D12Device.h"
#include "DxgiFormatMapping.h"

#include "D3D12GpuMemoryAllocator.h"
#include "Core/String.h"

using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;

PhxEngine::RHI::D3D12::D3D12ResourceManager::D3D12ResourceManager(std::shared_ptr<D3D12Device> device, std::shared_ptr<D3D12GpuMemoryAllocator> gpuAllocator)
	: m_device(device)
	, m_gpuAllocator(gpuAllocator)
{
	this->m_texturePool.Initialize(1);
	this->m_commandSignaturePool.Initialize(1);
	this->m_shaderPool.Initialize(1);
	this->m_inputLayoutPool.Initialize(1);
	this->m_bufferPool.Initialize(1);
	this->m_rtAccelerationStructurePool.Initialize(1);
	this->m_gfxPipelinePool.Initialize(1);
	this->m_computePipelinePool.Initialize(1);
	this->m_meshPipelinePool.Initialize(1);
	this->m_timerQueryPool.Initialize(1);
}

PhxEngine::RHI::D3D12::D3D12ResourceManager::~D3D12ResourceManager()
{
    this->m_texturePool.Finalize();
    this->m_commandSignaturePool.Finalize();
    this->m_shaderPool.Finalize();
    this->m_inputLayoutPool.Finalize();
    this->m_bufferPool.Finalize();
    this->m_rtAccelerationStructurePool.Finalize();
    this->m_gfxPipelinePool.Finalize();
    this->m_computePipelinePool.Finalize();
    this->m_meshPipelinePool.Finalize();
    this->m_timerQueryPool.Finalize();
}

void PhxEngine::RHI::D3D12::D3D12ResourceManager::RunGrabageCollection(size_t frame)
{
	assert(false, "TODO: Ensure this works as expected");
	this->m_frame = frame;
	while (!this->m_deleteQueue.empty())
	{
		DeleteItem& deleteItem = this->m_deleteQueue.front();
		if (deleteItem.Frame < this->m_frame)
		{
			deleteItem.DeleteFn();
			this->m_deleteQueue.pop_front();
		}
		else
		{
			break;
		}
	}
}

CommandSignatureHandle PhxEngine::RHI::D3D12::D3D12ResourceManager::CreateCommandSignature(CommandSignatureDesc const& desc, size_t byteStride)
{
	CommandSignatureHandle handle = this->m_commandSignaturePool.Emplace();
	D3D12CommandSignature* signature = this->m_commandSignaturePool.Get(handle);

	signature->D3D12Descs.resize(desc.ArgDesc.Size());
	for (int i = 0; i < desc.ArgDesc.Size(); i++)
	{
		const IndirectArgumnetDesc& argDesc = desc.ArgDesc[i];
		D3D12_INDIRECT_ARGUMENT_DESC& d3d12DispatchArgs = signature->D3D12Descs[i];
		switch (argDesc.Type)
		{
		case IndirectArgumentType::Draw:
			d3d12DispatchArgs.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
			break;

		case IndirectArgumentType::DrawIndex:
			d3d12DispatchArgs.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
			break;
		case IndirectArgumentType::Dispatch:
			d3d12DispatchArgs.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
			break;
		case IndirectArgumentType::DispatchMesh:
			d3d12DispatchArgs.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
			break;
		case IndirectArgumentType::Constant:
			d3d12DispatchArgs.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
			d3d12DispatchArgs.Constant.DestOffsetIn32BitValues = argDesc.Constant.DestOffsetIn32BitValues;
			d3d12DispatchArgs.Constant.RootParameterIndex = argDesc.Constant.RootParameterIndex;
			d3d12DispatchArgs.Constant.Num32BitValuesToSet = argDesc.Constant.Num32BitValuesToSet;
			break;
		default:
			break;
		}
	}

	D3D12_COMMAND_SIGNATURE_DESC cmdDesc = {};
	cmdDesc.ByteStride = byteStride;
	cmdDesc.NumArgumentDescs = signature->D3D12Descs.size();
	cmdDesc.pArgumentDescs = signature->D3D12Descs.data();

	ID3D12RootSignature* rootSig = nullptr;
	switch (desc.PipelineType)
	{
	case PipelineType::Gfx:
		rootSig = this->m_gfxPipelinePool.Get(desc.GfxHandle)->RootSignature.Get();
		break;
	case PipelineType::Compute:
		rootSig = this->m_computePipelinePool.Get(desc.ComputeHandle)->RootSignature.Get();
		break;
	case PipelineType::Mesh:
		rootSig = this->m_meshPipelinePool.Get(desc.MeshHandle)->RootSignature.Get();
		break;
	default:
		rootSig = nullptr;
		break;
	}

	ThrowIfFailed(
		this->m_device->GetNativeDevice()->CreateCommandSignature(
			&cmdDesc,
			rootSig,
			IID_PPV_ARGS(&signature->NativeSignature)));

	return handle;
}

SwapChainHandle PhxEngine::RHI::D3D12::D3D12ResourceManager::CreateSwapChain(SwapchainDesc const& desc, void* windowHandle)
{
	SwapChainHandle handle = this->m_swapChainPool.Emplace();
	D3D12SwapChain* impl = this->m_swapChainPool.Get(handle);

	HRESULT hr;
	UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	const auto& formatMapping = GetDxgiFormatMapping(desc.Format);

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

	hr = this->m_device->GetNativeFactory()->CreateSwapChainForHwnd(
		this->GetGfxQueue().GetD3D12CommandQueue(),
		static_cast<HWND>(windowHandle),
		&swapChainDesc,
		&fullscreenDesc,
		nullptr,
		impl->NativeSwapchain.GetAddressOf()
	);

	if (FAILED(hr))
	{
		throw std::exception();
	}

	if (FAILED(impl->NativeSwapchain->QueryInterface(IID_PPV_ARGS(&impl->NativeSwapchain4))))
	{
		throw std::exception();
	}

	return handle;
}

ShaderHandle PhxEngine::RHI::D3D12::D3D12ResourceManager::CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode)
{
	ShaderHandle handle = this->m_shaderPool.Emplace(desc, shaderByteCode.begin(), shaderByteCode.Size());
	D3D12Shader* shaderImpl = this->m_shaderPool.Get(handle);
	auto hr = D3D12CreateVersionedRootSignatureDeserializer(
		shaderImpl->ByteCode.data(),
		shaderImpl->ByteCode.size(),
		IID_PPV_ARGS(&shaderImpl->RootSignatureDeserializer));

	if (SUCCEEDED(hr))
	{
		hr = shaderImpl->RootSignatureDeserializer->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_1, &shaderImpl->RootSignatureDesc);
		if (SUCCEEDED(hr))
		{
			assert(shaderImpl->RootSignatureDesc->Version == D3D_ROOT_SIGNATURE_VERSION_1_1);

			Core::RefCountPtr<ID3D12RootSignature> rootSig;
			hr = this->m_device->GetNativeDevice2()->CreateRootSignature(
				0,
				shaderImpl->ByteCode.data(),
				shaderImpl->ByteCode.size(),
				IID_PPV_ARGS(&rootSig)
			);
			assert(SUCCEEDED(hr));
			if (SUCCEEDED(hr))
			{
				shaderImpl->RootSignature = rootSig;
			}
		}
	}

	return handle;
}

InputLayoutHandle PhxEngine::RHI::D3D12::D3D12ResourceManager::CreateInputLayout(Core::Span<VertexAttributeDesc> descriptions, uint32_t attributeCount)
{
	InputLayoutHandle handle = this->m_inputLayoutPool.Emplace();
	D3D12InputLayout* inputLayoutImpl = this->m_inputLayoutPool.Get(handle);
	inputLayoutImpl->Attributes.resize(attributeCount);

	for (uint32_t index = 0; index < attributeCount; index++)
	{
		VertexAttributeDesc& attr = inputLayoutImpl->Attributes[index];

		// Copy the description to get a stable name pointer in desc
		attr = descriptions[index];

		D3D12_INPUT_ELEMENT_DESC& dx12Desc = inputLayoutImpl->InputElements.emplace_back();

		dx12Desc.SemanticName = attr.SemanticName.c_str();
		dx12Desc.SemanticIndex = attr.SemanticIndex;


		const DxgiFormatMapping& formatMapping = GetDxgiFormatMapping(attr.Format);
		dx12Desc.Format = formatMapping.SrvFormat;
		dx12Desc.InputSlot = attr.InputSlot;
		dx12Desc.AlignedByteOffset = attr.AlignedByteOffset;
		if (dx12Desc.AlignedByteOffset == VertexAttributeDesc::SAppendAlignedElement)
		{
			dx12Desc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		}

		if (attr.IsInstanced)
		{
			dx12Desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
			dx12Desc.InstanceDataStepRate = 1;
		}
		else
		{
			dx12Desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			dx12Desc.InstanceDataStepRate = 0;
		}
	}

	return handle;
}

GfxPipelineHandle PhxEngine::RHI::D3D12::D3D12ResourceManager::CreateGfxPipeline(GfxPipelineDesc const& desc)
{
	D3D12GfxPipeline pipeline = {};
	pipeline.Desc = desc;

	if (desc.VertexShader.IsValid())
	{
		D3D12Shader* shaderImpl = this->m_shaderPool.Get(desc.VertexShader);
		if (pipeline.RootSignature == nullptr)
		{
			pipeline.RootSignature = shaderImpl->RootSignature;
		}
	}

	if (desc.PixelShader.IsValid())
	{
		D3D12Shader* shaderImpl = this->m_shaderPool.Get(desc.PixelShader);
		if (pipeline.RootSignature == nullptr)
		{
			pipeline.RootSignature = shaderImpl->RootSignature;
		}
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12Desc = {};
	d3d12Desc.pRootSignature = pipeline.RootSignature.Get();

	D3D12Shader* shaderImpl = nullptr;
	shaderImpl = this->m_shaderPool.Get(desc.VertexShader);
	if (shaderImpl)
	{
		d3d12Desc.VS = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}

	shaderImpl = this->m_shaderPool.Get(desc.HullShader);
	if (shaderImpl)
	{
		d3d12Desc.HS = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}

	shaderImpl = this->m_shaderPool.Get(desc.DomainShader);
	if (shaderImpl)
	{
		d3d12Desc.DS = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}


	shaderImpl = this->m_shaderPool.Get(desc.GeometryShader);
	if (shaderImpl)
	{
		d3d12Desc.GS = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}
	;
	shaderImpl = this->m_shaderPool.Get(desc.PixelShader);
	if (shaderImpl)
	{
		d3d12Desc.PS = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}

	this->TranslateBlendState(desc.BlendRenderState, d3d12Desc.BlendState);
	this->TranslateDepthStencilState(desc.DepthStencilRenderState, d3d12Desc.DepthStencilState);
	this->TranslateRasterState(desc.RasterRenderState, d3d12Desc.RasterizerState);

	switch (desc.PrimType)
	{
	case PrimitiveType::PointList:
		d3d12Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		break;
	case PrimitiveType::LineList:
		d3d12Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;
	case PrimitiveType::TriangleList:
	case PrimitiveType::TriangleStrip:
		d3d12Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	case PrimitiveType::PatchList:
		d3d12Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
		break;
	}

	if (desc.DsvFormat.has_value())
	{
		d3d12Desc.DSVFormat = GetDxgiFormatMapping(desc.DsvFormat.value()).RtvFormat;
	}

	d3d12Desc.SampleDesc.Count = desc.SampleCount;
	d3d12Desc.SampleDesc.Quality = desc.SampleQuality;

	for (size_t i = 0; i < desc.RtvFormats.size(); i++)
	{
		d3d12Desc.RTVFormats[i] = GetDxgiFormatMapping(desc.RtvFormats[i]).RtvFormat;
	}

	D3D12InputLayout* inputLayout = this->m_inputLayoutPool.Get(desc.InputLayout);
	if (inputLayout && !inputLayout->InputElements.empty())
	{
		d3d12Desc.InputLayout.NumElements = uint32_t(inputLayout->InputElements.size());
		d3d12Desc.InputLayout.pInputElementDescs = &(inputLayout->InputElements[0]);
	}

	d3d12Desc.NumRenderTargets = (uint32_t)desc.RtvFormats.size();
	d3d12Desc.SampleMask = ~0u;

	ThrowIfFailed(
		this->m_device->GetNativeDevice2()->CreateGraphicsPipelineState(&d3d12Desc, IID_PPV_ARGS(&pipeline.D3D12PipelineState)));

	return this->m_gfxPipelinePool.Insert(pipeline);
}

ComputePipelineHandle PhxEngine::RHI::D3D12::D3D12ResourceManager::CreateComputePipeline(ComputePipelineDesc const& desc)
{
	D3D12ComputePipeline pipeline = {};
	pipeline.Desc = desc;

	assert(desc.ComputeShader.IsValid());
	D3D12Shader* shaderImpl = this->m_shaderPool.Get(desc.ComputeShader);
	if (pipeline.RootSignature == nullptr)
	{
		pipeline.RootSignature = shaderImpl->RootSignature;
	}

	D3D12_COMPUTE_PIPELINE_STATE_DESC d3d12Desc = {};
	d3d12Desc.pRootSignature = pipeline.RootSignature.Get();
	d3d12Desc.CS = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };

	ThrowIfFailed(
		this->m_device->GetNativeDevice()->CreateComputePipelineState(&d3d12Desc, IID_PPV_ARGS(&pipeline.D3D12PipelineState)));

	return this->m_computePipelinePool.Insert(pipeline);
}

MeshPipelineHandle PhxEngine::RHI::D3D12::D3D12ResourceManager::CreateMeshPipeline(MeshPipelineDesc const& desc)
{

	D3D12MeshPipeline pipeline = {};
	pipeline.Desc = desc;

	if (desc.AmpShader.IsValid())
	{
		D3D12Shader* shaderImpl = this->m_shaderPool.Get(desc.AmpShader);
		if (pipeline.RootSignature == nullptr)
		{
			pipeline.RootSignature = shaderImpl->RootSignature;
		}
	}

	if (desc.MeshShader.IsValid())
	{
		D3D12Shader* shaderImpl = this->m_shaderPool.Get(desc.MeshShader);
		if (pipeline.RootSignature == nullptr)
		{
			pipeline.RootSignature = shaderImpl->RootSignature;
		}
	}

	if (desc.PixelShader.IsValid())
	{
		D3D12Shader* shaderImpl = this->m_shaderPool.Get(desc.PixelShader);
		if (pipeline.RootSignature == nullptr)
		{
			pipeline.RootSignature = shaderImpl->RootSignature;
		}
	}

#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignment specifier
	struct PSO_STREAM
	{
		typedef __declspec(align(sizeof(void*))) D3D12_PIPELINE_STATE_SUBOBJECT_TYPE ALIGNED_TYPE;

		ALIGNED_TYPE RootSignature_Type;        ID3D12RootSignature* RootSignature;
		ALIGNED_TYPE PrimitiveTopology_Type;    D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
		ALIGNED_TYPE AmplificationShader_Type;  D3D12_SHADER_BYTECODE AmplificationShader;
		ALIGNED_TYPE MeshShader_Type;           D3D12_SHADER_BYTECODE MeshShader;
		ALIGNED_TYPE PixelShader_Type;          D3D12_SHADER_BYTECODE PixelShader;
		ALIGNED_TYPE RasterizerState_Type;      D3D12_RASTERIZER_DESC RasterizerState;
		ALIGNED_TYPE DepthStencilState_Type;    D3D12_DEPTH_STENCIL_DESC DepthStencilState;
		ALIGNED_TYPE BlendState_Type;           D3D12_BLEND_DESC BlendState;
		ALIGNED_TYPE SampleDesc_Type;           DXGI_SAMPLE_DESC SampleDesc;
		ALIGNED_TYPE SampleMask_Type;           UINT SampleMask;
		ALIGNED_TYPE RenderTargets_Type;        D3D12_RT_FORMAT_ARRAY RenderTargets;
		ALIGNED_TYPE DSVFormat_Type;            DXGI_FORMAT DSVFormat;
	} psoDesc = { };
#pragma warning(pop)

	psoDesc.RootSignature_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE;
	psoDesc.PrimitiveTopology_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY;
	psoDesc.AmplificationShader_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_AS;
	psoDesc.MeshShader_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MS;
	psoDesc.PixelShader_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS;
	psoDesc.RasterizerState_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER;
	psoDesc.DepthStencilState_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL;
	psoDesc.BlendState_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND;
	psoDesc.SampleDesc_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC;
	psoDesc.SampleMask_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK;
	psoDesc.RenderTargets_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS;
	psoDesc.DSVFormat_Type = D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT;

	psoDesc.RootSignature = pipeline.RootSignature.Get();

	D3D12Shader* shaderImpl = nullptr;
	shaderImpl = this->m_shaderPool.Get(desc.AmpShader);
	if (shaderImpl)
	{
		psoDesc.AmplificationShader = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}

	shaderImpl = this->m_shaderPool.Get(desc.MeshShader);
	if (shaderImpl)
	{
		psoDesc.MeshShader = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}

	shaderImpl = this->m_shaderPool.Get(desc.PixelShader);
	if (shaderImpl)
	{
		psoDesc.PixelShader = { shaderImpl->ByteCode.data(), shaderImpl->ByteCode.size() };
	}

	this->TranslateBlendState(desc.BlendRenderState, psoDesc.BlendState);
	this->TranslateDepthStencilState(desc.DepthStencilRenderState, psoDesc.DepthStencilState);
	this->TranslateRasterState(desc.RasterRenderState, psoDesc.RasterizerState);

	switch (desc.PrimType)
	{
	case PrimitiveType::PointList:
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		break;
	case PrimitiveType::LineList:
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		break;
	case PrimitiveType::TriangleList:
	case PrimitiveType::TriangleStrip:
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		break;
	case PrimitiveType::PatchList:
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
		break;
	}
	psoDesc.SampleDesc.Count = desc.SampleCount;
	psoDesc.SampleDesc.Quality = desc.SampleQuality;
	psoDesc.SampleMask = ~0u;

	psoDesc.RenderTargets.NumRenderTargets = desc.RtvFormats.size();

	for (size_t i = 0; i < desc.RtvFormats.size(); i++)
	{
		psoDesc.RenderTargets.RTFormats[i] = GetDxgiFormatMapping(desc.RtvFormats[i]).RtvFormat;
	}

	if (desc.DsvFormat.has_value())
	{
		psoDesc.DSVFormat = GetDxgiFormatMapping(desc.DsvFormat.value()).RtvFormat;
	}

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
	streamDesc.pPipelineStateSubobjectStream = &psoDesc;
	streamDesc.SizeInBytes = sizeof(psoDesc);

	ThrowIfFailed(
		this->m_device->GetNativeDevice2()->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&pipeline.D3D12PipelineState)));

	return this->m_meshPipelinePool.Insert(pipeline);
}

BufferHandle PhxEngine::RHI::D3D12::D3D12ResourceManager::CreateGpuBuffer(BufferDesc const& desc, void* initalData)
{
	BufferHandle buffer = this->m_bufferPool.Emplace();
	D3D12Buffer& bufferImpl = *this->m_bufferPool.Get(buffer);

	D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
	if ((desc.Binding & RHI::BindingFlags::UnorderedAccess) == RHI::BindingFlags::UnorderedAccess)
	{
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;

	switch (desc.Usage)
	{
	case Usage::ReadBack:
		allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;

		initialState = D3D12_RESOURCE_STATE_COPY_DEST;
		break;

	case Usage::Upload:
		allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
		initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
		break;

	case Usage::Default:
	default:
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		initialState = D3D12_RESOURCE_STATE_COMMON;
	}

	UINT64 alignedSize = 0;
	if ((desc.Binding & BindingFlags::ConstantBuffer) == BindingFlags::ConstantBuffer)
	{
		alignedSize = Core::AlignTo(alignedSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	}

	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(desc.Stride * desc.NumElements, resourceFlags, alignedSize);

	/* TODO: Add Sparse stuff
	if (resource_heap_tier >= D3D12_RESOURCE_HEAP_TIER_2)
	{
		// tile pool memory can be used for sparse buffers and textures alike (requires resource heap tier 2):
		allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
	}
	else if (has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE_TILE_POOL_BUFFER))
	{
		allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	}
	else if (has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE_TILE_POOL_TEXTURE_NON_RT_DS))
	{
		allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
	}
	else if (has_flag(desc->misc_flags, ResourceMiscFlag::SPARSE_TILE_POOL_TEXTURE_RT_DS))
	{
		allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
	}
	*/

	if ((bufferImpl.Desc.MiscFlags & BufferMiscFlags::IsAliasedResource) == BufferMiscFlags::IsAliasedResource)
	{
		D3D12_RESOURCE_ALLOCATION_INFO finalAllocInfo = {};
		finalAllocInfo.Alignment = 0;
		finalAllocInfo.SizeInBytes = Core::AlignTo(
			bufferImpl.Desc.Stride * bufferImpl.Desc.NumElements,
			D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT * 1024);

		this->m_gpuAllocator->AllocateAliasedResource(
			allocationDesc,
			finalAllocInfo,
			&bufferImpl.Allocation);

		return;
	}
	else if (bufferImpl.Desc.AliasedBuffer.IsValid())
	{
		D3D12Buffer* aliasedBuffer = this->m_bufferPool.Get(bufferImpl.Desc.AliasedBuffer);

		this->m_gpuAllocator->AllocateAliasingResource(
				aliasedBuffer->Allocation.Get(),
				resourceDesc,
				initialState,
				bufferImpl.D3D12Resource);
	}
	else
	{
		this->m_gpuAllocator->AllocateResource(
				allocationDesc,
				resourceDesc,
				initialState,
				&bufferImpl.Allocation,
				&bufferImpl.D3D12Resource);
	}


	// Issue data copy on request:
	if (initalData != nullptr)
	{
		auto cmd = this->m_copyCtxAllocator.Allocate(desc.Size());

		auto& internalUploadBuffer = *this->m_bufferPool.Get(cmd.UploadBuffer);
		std::memcpy(internalUploadBuffer.MappedData, initalData, desc.Size());

		cmd.NativeCmdList->CopyBufferRegion(
			bufferImpl.D3D12Resource.Get(),
			0,
			internalUploadBuffer.D3D12Resource.Get(),
			0,
			desc.Size());

		this->m_copyCtxAllocator.Finalize();
	}

	if ((desc.MiscFlags & BufferMiscFlags::IsAliasedResource) == BufferMiscFlags::IsAliasedResource)
	{
		return buffer;
	}

	if ((desc.Binding & BindingFlags::ShaderResource) == BindingFlags::ShaderResource)
	{
		this->CreateSubresource(buffer, RHI::SubresouceType::SRV, 0u);
	}

	if ((desc.Binding & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess)
	{
		this->CreateSubresource(buffer, RHI::SubresouceType::UAV, 0u);
	}

	return buffer;
}

TextureHandle PhxEngine::RHI::D3D12::D3D12ResourceManager::CreateTexture(TextureDesc const& desc, void* initalData)
{
	D3D12_CLEAR_VALUE d3d12OptimizedClearValue = {};
	d3d12OptimizedClearValue.Color[0] = desc.OptmizedClearValue.Colour.R;
	d3d12OptimizedClearValue.Color[1] = desc.OptmizedClearValue.Colour.G;
	d3d12OptimizedClearValue.Color[2] = desc.OptmizedClearValue.Colour.B;
	d3d12OptimizedClearValue.Color[3] = desc.OptmizedClearValue.Colour.A;
	d3d12OptimizedClearValue.DepthStencil.Depth = desc.OptmizedClearValue.DepthStencil.Depth;
	d3d12OptimizedClearValue.DepthStencil.Stencil = desc.OptmizedClearValue.DepthStencil.Stencil;

	auto dxgiFormatMapping = GetDxgiFormatMapping(desc.Format);
	d3d12OptimizedClearValue.Format = dxgiFormatMapping.RtvFormat;

	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;

	if ((desc.BindingFlags & BindingFlags::DepthStencil) == BindingFlags::DepthStencil)
	{
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		if ((desc.BindingFlags & BindingFlags::ShaderResource) != BindingFlags::ShaderResource)
		{
			resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}
	}
	if ((desc.BindingFlags & BindingFlags::RenderTarget) == BindingFlags::RenderTarget)
	{
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}
	if ((desc.BindingFlags & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess)
	{
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	CD3DX12_RESOURCE_DESC resourceDesc = {};

	const bool isTypeless = (desc.MiscFlags | RHI::TextureMiscFlags::Typeless) == RHI::TextureMiscFlags::Typeless;
	switch (desc.Dimension)
	{
	case TextureDimension::Texture1D:
	case TextureDimension::Texture1DArray:
	{
		resourceDesc =
			CD3DX12_RESOURCE_DESC::Tex1D(
				isTypeless ? dxgiFormatMapping.ResourceFormat : dxgiFormatMapping.RtvFormat,
				desc.Width,
				desc.ArraySize,
				desc.MipLevels,
				resourceFlags);
		break;
	}
	case TextureDimension::Texture2D:
	case TextureDimension::Texture2DArray:
	case TextureDimension::TextureCube:
	case TextureDimension::TextureCubeArray:
	case TextureDimension::Texture2DMS:
	case TextureDimension::Texture2DMSArray:
	{
		resourceDesc =
			CD3DX12_RESOURCE_DESC::Tex2D(
				isTypeless ? dxgiFormatMapping.ResourceFormat : dxgiFormatMapping.RtvFormat,
				desc.Width,
				desc.Height,
				desc.ArraySize,
				desc.MipLevels,
				1,
				0,
				resourceFlags);
		break;
	}
	case TextureDimension::Texture3D:
	{
		resourceDesc =
			CD3DX12_RESOURCE_DESC::Tex3D(
				isTypeless ? dxgiFormatMapping.ResourceFormat : dxgiFormatMapping.RtvFormat,
				desc.Width,
				desc.Height,
				desc.ArraySize,
				desc.MipLevels,
				resourceFlags);
		break;
	}
	default:
		throw std::runtime_error("Unsupported texture dimension");
	}

	const bool useClearValue =
		((desc.BindingFlags & BindingFlags::RenderTarget) == BindingFlags::RenderTarget) ||
		((desc.BindingFlags & BindingFlags::DepthStencil) == BindingFlags::DepthStencil);

	D3D12Texture textureImpl = {};
	textureImpl.Desc = desc;

	textureImpl.TotalSize = 0;
	textureImpl.Footprints.resize(desc.ArraySize * std::max(uint16_t(1u), desc.MipLevels));
	textureImpl.RowSizesInBytes.resize(textureImpl.Footprints.size());
	textureImpl.NumRows.resize(textureImpl.Footprints.size());
	this->m_device->GetNativeDevice2()->GetCopyableFootprints(
		&resourceDesc,
		0,
		(UINT)textureImpl.Footprints.size(),
		0,
		textureImpl.Footprints.data(),
		textureImpl.NumRows.data(),
		textureImpl.RowSizesInBytes.data(),
		&textureImpl.TotalSize
	);

	ThrowIfFailed(
		this->m_gpuAllocator->GetNativeAllocator()->CreateResource(
			&allocationDesc,
			&resourceDesc,
			ConvertResourceStates(desc.InitialState),
			useClearValue ? &d3d12OptimizedClearValue : nullptr,
			&textureImpl.Allocation,
			IID_PPV_ARGS(&textureImpl.D3D12Resource)));


	std::wstring debugName;
	Core::StringConvert(desc.DebugName, debugName);
	textureImpl.D3D12Resource->SetName(debugName.c_str());

	TextureHandle texture = this->m_texturePool.Insert(textureImpl);

	if ((desc.BindingFlags & BindingFlags::ShaderResource) == BindingFlags::ShaderResource)
	{
		this->CreateSubresource(texture, RHI::SubresouceType::SRV, 0, ~0u, 0, ~0u);
	}

	if ((desc.BindingFlags & BindingFlags::RenderTarget) == BindingFlags::RenderTarget)
	{
		this->CreateSubresource(texture, RHI::SubresouceType::RTV, 0, ~0u, 0, ~0u);
	}

	if ((desc.BindingFlags & BindingFlags::DepthStencil) == BindingFlags::DepthStencil)
	{
		this->CreateSubresource(texture, RHI::SubresouceType::DSV, 0, ~0u, 0, ~0u);
	}

	if ((desc.BindingFlags & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess)
	{
		this->CreateSubresource(texture, RHI::SubresouceType::UAV, 0, ~0u, 0, ~0u);
	}

	if (initalData != nullptr)
	{
		std::vector<D3D12_SUBRESOURCE_DATA> data(textureImpl.Footprints.size());
		for (size_t i = 0; i < textureImpl.Footprints.size(); ++i)
		{
			data[i] = _ConvertSubresourceData(initalData[i]);
		}

		auto cmd = this->m_copyCtxAllocator.Allocate(textureImpl.TotalSize);

		for (size_t i = 0; i < textureImpl.Footprints.size(); ++i)
		{
			void* mappedUploadData = this->GetBufferMappedData(cmd.UploadBuffer);
			if (textureImpl.RowSizesInBytes[i] > (SIZE_T)-1)
				continue;
			D3D12_MEMCPY_DEST DestData = {};
			DestData.pData = (void*)((UINT64)mappedUploadData + textureImpl.Footprints[i].Offset);
			DestData.RowPitch = (SIZE_T)textureImpl.Footprints[i].Footprint.RowPitch;
			DestData.SlicePitch = (SIZE_T)textureImpl.Footprints[i].Footprint.RowPitch * (SIZE_T)textureImpl.NumRows[i];
			MemcpySubresource(&DestData, &data[i], (SIZE_T)textureImpl.RowSizesInBytes[i], textureImpl.NumRows[i], textureImpl.Footprints[i].Footprint.Depth);
		}

		for (UINT i = 0; i < textureImpl.Footprints.size(); ++i)
		{
			CD3DX12_TEXTURE_COPY_LOCATION Dst(textureImpl.D3D12Resource.Get(), i);
			auto& uploadBuffer = *this->m_bufferPool.Get(cmd.UploadBuffer);
			CD3DX12_TEXTURE_COPY_LOCATION Src(uploadBuffer.D3D12Resource.Get(), textureImpl.Footprints[i]);
			cmd.NativeCmdList->CopyTextureRegion(
				&Dst,
				0,
				0,
				0,
				&Src,
				nullptr
			);
		}

		// Blocking call
		this->m_copyCtxAllocator.Submit(cmd);
	}

	return texture;
}

TextureHandle PhxEngine::RHI::D3D12::D3D12ResourceManager::CreateTexture(TextureDesc const& desc, Core::RefCountPtr<ID3D12Resource> resource)
{
	D3D12Texture textureImpl = {};
	textureImpl.Desc = desc;
	textureImpl.D3D12Resource = resource;

	std::wstring debugName;
	Core::StringConvert(desc.DebugName, debugName);
	textureImpl.D3D12Resource->SetName(debugName.c_str());

	TextureHandle texture = this->m_texturePool.Insert(textureImpl);

	if ((desc.BindingFlags & BindingFlags::ShaderResource) == BindingFlags::ShaderResource)
	{
		this->CreateSubresource(texture, RHI::SubresouceType::SRV, 0, ~0u, 0, ~0u);
	}

	if ((desc.BindingFlags & BindingFlags::RenderTarget) == BindingFlags::RenderTarget)
	{
		this->CreateSubresource(texture, RHI::SubresouceType::RTV, 0, ~0u, 0, ~0u);
	}

	if ((desc.BindingFlags & BindingFlags::DepthStencil) == BindingFlags::DepthStencil)
	{
		this->CreateSubresource(texture, RHI::SubresouceType::DSV, 0, ~0u, 0, ~0u);
	}

	if ((desc.BindingFlags & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess)
	{
		this->CreateSubresource(texture, RHI::SubresouceType::UAV, 0, ~0u, 0, ~0u);
	}

	return texture;
}

RTAccelerationStructureHandle PhxEngine::RHI::D3D12::D3D12ResourceManager::CreateRTAccelerationStructure(RTAccelerationStructureDesc const& desc)
{
	RTAccelerationStructureHandle handle = this->m_rtAccelerationStructurePool.Emplace();
	D3D12RTAccelerationStructure& rtAccelerationStructureImpl = *this->m_rtAccelerationStructurePool.Get(handle);
	rtAccelerationStructureImpl.Desc = desc;

	if (desc.Flags & RTAccelerationStructureDesc::FLAGS::kAllowUpdate)
	{
		rtAccelerationStructureImpl.Dx12Desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	}

	if (desc.Flags & RTAccelerationStructureDesc::FLAGS::kAllowCompaction)
	{
		rtAccelerationStructureImpl.Dx12Desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
	}

	if (desc.Flags & RTAccelerationStructureDesc::FLAGS::kPreferFastTrace)
	{
		rtAccelerationStructureImpl.Dx12Desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	}

	if (desc.Flags & RTAccelerationStructureDesc::FLAGS::kPreferFastBuild)
	{
		rtAccelerationStructureImpl.Dx12Desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
	}

	if (desc.Flags & RTAccelerationStructureDesc::FLAGS::kMinimizeMemory)
	{
		rtAccelerationStructureImpl.Dx12Desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
	}


	switch (desc.Type)
	{
	case RTAccelerationStructureDesc::Type::BottomLevel:
	{
		rtAccelerationStructureImpl.Dx12Desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		rtAccelerationStructureImpl.Dx12Desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

		for (auto& g : desc.ButtomLevel.Geometries)
		{
			D3D12_RAYTRACING_GEOMETRY_DESC& dx12GeometryDesc = rtAccelerationStructureImpl.Geometries.emplace_back();
			dx12GeometryDesc = {};

			if (g.Type == RTAccelerationStructureDesc::BottomLevelDesc::Geometry::Type::Triangles)
			{
				dx12GeometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;

				D3D12Buffer* dx12VertexBuffer = this->m_bufferPool.Get(g.Triangles.VertexBuffer);
				assert(dx12VertexBuffer);
				D3D12Buffer* dx12IndexBuffer = this->m_bufferPool.Get(g.Triangles.IndexBuffer);
				assert(dx12IndexBuffer);

				D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& trianglesDesc = dx12GeometryDesc.Triangles;
				trianglesDesc.VertexFormat = D3D12::GetDxgiFormatMapping(g.Triangles.VertexFormat).SrvFormat;
				trianglesDesc.VertexCount = (UINT)g.Triangles.VertexCount;
				trianglesDesc.VertexBuffer.StartAddress = dx12VertexBuffer->VertexView.BufferLocation + (D3D12_GPU_VIRTUAL_ADDRESS)g.Triangles.VertexByteOffset;
				trianglesDesc.VertexBuffer.StrideInBytes = g.Triangles.VertexStride;
				trianglesDesc.IndexBuffer =
					dx12VertexBuffer->IndexView.BufferLocation +
					(D3D12_GPU_VIRTUAL_ADDRESS)g.Triangles.IndexOffset * (g.Triangles.IndexFormat == RHI::Format::R16_UINT ? sizeof(uint16_t) : sizeof(uint32_t));
				trianglesDesc.IndexCount = g.Triangles.IndexCount;
				trianglesDesc.IndexFormat = g.Triangles.IndexFormat == RHI::Format::R16_UINT ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

				if (g.Flags & RTAccelerationStructureDesc::BottomLevelDesc::Geometry::kUseTransform)
				{
					D3D12Buffer* transform3x4Buffer = this->m_bufferPool.Get(g.Triangles.Transform3x4Buffer);
					assert(transform3x4Buffer);
					trianglesDesc.Transform3x4 = transform3x4Buffer->D3D12Resource->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)g.Triangles.Transform3x4BufferOffset;
				}
			}
			else if (g.Type == RTAccelerationStructureDesc::BottomLevelDesc::Geometry::Type::ProceduralAABB)
			{
				dx12GeometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;

				D3D12Buffer* aabbBuffer = this->m_bufferPool.Get(g.AABBs.AABBBuffer);
				assert(aabbBuffer);
				dx12GeometryDesc.AABBs.AABBs.StartAddress = aabbBuffer->D3D12Resource->GetGPUVirtualAddress() + (D3D12_GPU_VIRTUAL_ADDRESS)g.AABBs.Offset;
				dx12GeometryDesc.AABBs.AABBs.StrideInBytes = (UINT64)g.AABBs.Stride;
				dx12GeometryDesc.AABBs.AABBCount = g.AABBs.Stride;
			}
		}
		rtAccelerationStructureImpl.Dx12Desc.pGeometryDescs = rtAccelerationStructureImpl.Geometries.data();
		rtAccelerationStructureImpl.Dx12Desc.NumDescs = (UINT)rtAccelerationStructureImpl.Geometries.size();
		break;
	}
	case RTAccelerationStructureDesc::Type::TopLevel:
	{
		rtAccelerationStructureImpl.Dx12Desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		rtAccelerationStructureImpl.Dx12Desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

		D3D12Buffer* instanceBuffer = this->m_bufferPool.Get(desc.TopLevel.InstanceBuffer);
		assert(instanceBuffer);
		rtAccelerationStructureImpl.Dx12Desc.InstanceDescs = instanceBuffer->D3D12Resource->GetGPUVirtualAddress() +
			(D3D12_GPU_VIRTUAL_ADDRESS)desc.TopLevel.Offset;
		rtAccelerationStructureImpl.Dx12Desc.NumDescs = (UINT)desc.TopLevel.Count;

		break;
	}
	default:
		return {};
	}

	this->m_device->GetNativeDevice5()->GetRaytracingAccelerationStructurePrebuildInfo(
		&rtAccelerationStructureImpl.Dx12Desc,
		&rtAccelerationStructureImpl.Info);


	UINT64 alignment = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
	UINT64 alignedSize = Core::AlignTo(rtAccelerationStructureImpl.Info.ResultDataMaxSizeInBytes, alignment);

	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.Width = (UINT64)alignedSize;
	resourceDesc.Height = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Alignment = 0;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;

	D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;


	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
	ThrowIfFailed(
		this->m_gpuAllocator->GetNativeAllocator()->CreateResource(
			&allocationDesc,
			&resourceDesc,
			resourceState,
			nullptr,
			&rtAccelerationStructureImpl.Allocation,
			IID_PPV_ARGS(&rtAccelerationStructureImpl.D3D12Resource)));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.RaytracingAccelerationStructure.Location = rtAccelerationStructureImpl.D3D12Resource->GetGPUVirtualAddress();


	rtAccelerationStructureImpl.Srv = {
			.Allocation = this->GetResourceCpuHeap().Allocate(1),
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.SRVDesc = srvDesc,
	};

	this->m_device->GetNativeDevice5()->CreateShaderResourceView(
		nullptr,
		&srvDesc,
		rtAccelerationStructureImpl.Srv.Allocation.GetCpuHandle());

	// Copy Descriptor to Bindless since we are creating a texture as a shader resource view
	rtAccelerationStructureImpl.Srv.BindlessIndex = this->m_bindlessResourceDescriptorTable.Allocate();
	if (rtAccelerationStructureImpl.Srv.BindlessIndex != cInvalidDescriptorIndex)
	{
		this->m_device->GetNativeDevice5()->CopyDescriptorsSimple(
			1,
			this->m_bindlessResourceDescriptorTable.GetCpuHandle(rtAccelerationStructureImpl.Srv.BindlessIndex),
			rtAccelerationStructureImpl.Srv.Allocation.GetCpuHandle(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	BufferDesc scratchBufferDesc = {};
	rtAccelerationStructureImpl.SratchBuffer;
	scratchBufferDesc.Stride = (uint32_t)std::max(rtAccelerationStructureImpl.Info.ScratchDataSizeInBytes, rtAccelerationStructureImpl.Info.UpdateScratchDataSizeInBytes);
	scratchBufferDesc.NumElements = 1;
	scratchBufferDesc.DebugName = "RT Scratch Buffer";
	rtAccelerationStructureImpl.SratchBuffer = this->CreateGpuBuffer(scratchBufferDesc);

	return handle;
}

TimerQueryHandle PhxEngine::RHI::D3D12::D3D12ResourceManager::CreateTimerQuery()
{
	int queryIndex = this->m_timerQueryIndexPool.Allocate();
	if (queryIndex < 0)
	{
		assert(false);
		return {};
	}

	TimerQueryHandle handle = this->m_timerQueryPool.Emplace();

	D3D12TimerQuery& timerQuery = *this->m_timerQueryPool.Get(handle);

	timerQuery.BeginQueryIndex = queryIndex * 2;
	timerQuery.EndQueryIndex = timerQuery.BeginQueryIndex + 1;
	timerQuery.Resolved = false;
	timerQuery.Time = {};

	return handle;
}

void PhxEngine::RHI::D3D12::D3D12ResourceManager::DeleteCommandSignature(CommandSignatureHandle handle)
{
	if (!handle.IsValid())
		return;

	Core::ScopedSpinLock _(this->m_deleteQueueGuard);
	m_deleteQueue.push_back({
		.Frame = this->m_frame,
		.DeleteFn = [&]() {
			D3D12CommandSignature* signature = this->GetCommandSignaturePool().Get(handle);
			if (signature)
			{
				this->GetCommandSignaturePool().Release(handle);
			}
		}});
}

void PhxEngine::RHI::D3D12::D3D12ResourceManager::DeleteSwapChain(SwapChainHandle handle)
{
	if (!handle.IsValid())
		return;

	Core::ScopedSpinLock _(this->m_deleteQueueGuard);
	m_deleteQueue.push_back({
		.Frame = this->m_frame,
		.DeleteFn = [&]() {
			this->GetSwapChainPool().Release(handle);
		} });
}

void PhxEngine::RHI::D3D12::D3D12ResourceManager::DeleteShader(ShaderHandle handle)
{
	if (!handle.IsValid())
		return;

	Core::ScopedSpinLock _(this->m_deleteQueueGuard);
	m_deleteQueue.push_back({
		.Frame = this->m_frame,
		.DeleteFn = [&]() {
			this->GetShaderPool().Release(handle);
		} });
}

void PhxEngine::RHI::D3D12::D3D12ResourceManager::DeleteInputLayout(InputLayoutHandle handle)
{
	if (!handle.IsValid())
		return;

	Core::ScopedSpinLock _(this->m_deleteQueueGuard);
	m_deleteQueue.push_back({
		.Frame = this->m_frame,
		.DeleteFn = [&]() {
			this->GetInputLayoutPool().Release(handle);
		} });
}

void PhxEngine::RHI::D3D12::D3D12ResourceManager::DeleteGfxPipeline(GfxPipelineHandle handle)
{
	if (!handle.IsValid())
		return;

	Core::ScopedSpinLock _(this->m_deleteQueueGuard);
	m_deleteQueue.push_back({
		.Frame = this->m_frame,
		.DeleteFn = [&]() {
			this->GetGfxPipelinePool().Release(handle);
		} });
}

void PhxEngine::RHI::D3D12::D3D12ResourceManager::DeleteComputePipeline(ComputePipelineHandle handle)
{
	if (!handle.IsValid())
		return;

	Core::ScopedSpinLock _(this->m_deleteQueueGuard);
	m_deleteQueue.push_back({
		.Frame = this->m_frame,
		.DeleteFn = [&]() {
			this->GetComputePipelinePool().Release(handle);
		} });
}

void PhxEngine::RHI::D3D12::D3D12ResourceManager::DeleteMeshPipeline(MeshPipelineHandle handle)
{
	if (!handle.IsValid())
		return;

	Core::ScopedSpinLock _(this->m_deleteQueueGuard);
	m_deleteQueue.push_back({
		.Frame = this->m_frame,
		.DeleteFn = [&]() {
			this->GetMeshPipelinePool().Release(handle);
		} });
}

void PhxEngine::RHI::D3D12::D3D12ResourceManager::DeleteGpuBuffer(BufferHandle handle)
{
	if (!handle.IsValid())
		return;

	Core::ScopedSpinLock _(this->m_deleteQueueGuard);
	m_deleteQueue.push_back({
		.Frame = this->m_frame,
		.DeleteFn = [&]() {
			D3D12Buffer* bufferImpl = this->m_bufferPool.Get(handle);
			for (auto& view : bufferImpl->SrvSubresourcesAlloc)
			{
				if (view.BindlessIndex != cInvalidDescriptorIndex)
				{
					this->m_bindlessResourceDescriptorTable.Free(view.BindlessIndex);
				}
			}

			for (auto& view : bufferImpl->UavSubresourcesAlloc)
			{
				if (view.BindlessIndex != cInvalidDescriptorIndex)
				{
					this->m_bindlessResourceDescriptorTable.Free(view.BindlessIndex);
				}
			}

			bufferImpl->DisposeViews();
			this->GetBufferPool().Release(handle);
		} });
}

void PhxEngine::RHI::D3D12::D3D12ResourceManager::DeleteTexture(TextureHandle handle)
{
	if (!handle.IsValid())
		return;

	Core::ScopedSpinLock _(this->m_deleteQueueGuard);
	m_deleteQueue.push_back({
		.Frame = this->m_frame,
		.DeleteFn = [&]() {
			D3D12Texture* texture = this->m_texturePool.Get(handle);
			for (auto& view : texture->SrvSubresourcesAlloc)
			{
				if (view.BindlessIndex != cInvalidDescriptorIndex)
				{
					this->m_bindlessResourceDescriptorTable.Free(view.BindlessIndex);
				}
			}

			for (auto& view : texture->UavSubresourcesAlloc)
			{
				if (view.BindlessIndex != cInvalidDescriptorIndex)
				{
					this->m_bindlessResourceDescriptorTable.Free(view.BindlessIndex);
				}
			}

			texture->DisposeViews();
			this->GetTexturePool().Release(handle);
		} });
}

void PhxEngine::RHI::D3D12::D3D12ResourceManager::DeleteRTAccelerationStructure(RTAccelerationStructureHandle handle)
{
	if (!handle.IsValid())
		return;

	D3D12RTAccelerationStructure* impl = this->m_rtAccelerationStructurePool.Get(handle);
	this->DeleteGpuBuffer(impl->SratchBuffer);

	if (impl->Desc.Type == RTAccelerationStructureDesc::Type::TopLevel)
	{
		this->DeleteGpuBuffer(impl->Desc.TopLevel.InstanceBuffer);
	}

	Core::ScopedSpinLock _(this->m_deleteQueueGuard);
	m_deleteQueue.push_back({
		.Frame = this->m_frame,
		.DeleteFn = [&]() {
			D3D12RTAccelerationStructure* impl = this->m_rtAccelerationStructurePool.Get(handle);
			if (impl->Srv.BindlessIndex != cInvalidDescriptorIndex)
			{
				this->m_bindlessResourceDescriptorTable.Free(impl->Srv.BindlessIndex);
			}

			this->m_rtAccelerationStructurePool.Release(handle);
		} });
}

void PhxEngine::RHI::D3D12::D3D12ResourceManager::DeleteTimerQuery(TimerQueryHandle handle)
{
	if (!handle.IsValid())
		return;

	Core::ScopedSpinLock _(this->m_deleteQueueGuard);
	m_deleteQueue.push_back({
		.Frame = this->m_frame,
		.DeleteFn = [&]() {
			D3D12TimerQuery* impl = this->GetTimerQueryPool().Get(handle);
			this->m_timerQueryIndexPool.Release(static_cast<int>(impl->BeginQueryIndex) / 2);
			this->GetTimerQueryPool().Release(handle);
		} });
}

void PhxEngine::RHI::D3D12::D3D12ResourceManager::ResizeSwapChain(SwapChainHandle handle, SwapchainDesc const& desc)
{
	// Resize swapchain:
	this->m_device->WaitForIdle();

	D3D12SwapChain* impl = this->m_swapChainPool.Get(handle);
	impl->Desc = desc;

	// Delete back buffers
	for (int i = 0; i < impl->BackBuffers.size(); i++)
	{
		this->DeleteTexture(impl->BackBuffers[i]);
	}

	impl->BackBuffers.clear();

	const auto& formatMapping = GetDxgiFormatMapping(desc.Format);
	UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	ThrowIfFailed(
		impl->NativeSwapchain4->ResizeBuffers(
			desc.BufferCount,
			desc.Width,
			desc.Height,
			formatMapping.RtvFormat,
			swapChainFlags));
}

const TextureDesc& PhxEngine::RHI::D3D12::D3D12ResourceManager::GetTextureDesc(TextureHandle handle)
{
	const D3D12Texture* texture = this->m_texturePool.Get(handle);
	return texture->Desc;
}

const BufferDesc& PhxEngine::RHI::D3D12::D3D12ResourceManager::GetBufferDesc(BufferHandle handle)
{
	const D3D12Buffer* buffer = this->m_bufferPool.Get(handle);
	return buffer->Desc;
}

DescriptorIndex PhxEngine::RHI::D3D12::D3D12ResourceManager::GetDescriptorIndex(TextureHandle handle, SubresouceType type, int subResource)
{
	const D3D12Texture* texture = this->m_texturePool.Get(handle);

	if (!texture)
	{
		return cInvalidDescriptorIndex;
	}

	switch (type)
	{
	case SubresouceType::SRV:
		return subResource == -1
			? texture->Srv.BindlessIndex
			: texture->SrvSubresourcesAlloc[subResource].BindlessIndex;
		break;

	case SubresouceType::UAV:
		return subResource == -1
			? texture->UavAllocation.BindlessIndex
			: texture->UavSubresourcesAlloc[subResource].BindlessIndex;
		break;

	case SubresouceType::RTV:
		return subResource == -1
			? texture->RtvAllocation.BindlessIndex
			: texture->RtvSubresourcesAlloc[subResource].BindlessIndex;
		break;

	case SubresouceType::DSV:
		return subResource == -1
			? texture->DsvAllocation.BindlessIndex
			: texture->DsvSubresourcesAlloc[subResource].BindlessIndex;
		break;
	default:
		throw std::runtime_error("Unsupported enum type");
	}
}

DescriptorIndex PhxEngine::RHI::D3D12::D3D12ResourceManager::GetDescriptorIndex(BufferHandle handle, SubresouceType type, int subResource)
{
	const D3D12Buffer* bufferImpl = this->m_bufferPool.Get(handle);

	if (!bufferImpl)
	{
		return cInvalidDescriptorIndex;
	}

	switch (type)
	{
	case SubresouceType::SRV:
		return subResource == -1
			? bufferImpl->Srv.BindlessIndex
			: bufferImpl->SrvSubresourcesAlloc[subResource].BindlessIndex;

	case SubresouceType::UAV:
		return subResource == -1
			? bufferImpl->UavAllocation.BindlessIndex
			: bufferImpl->UavSubresourcesAlloc[subResource].BindlessIndex;
	default:
		throw std::runtime_error("Unsupported enum type");
	}
}
