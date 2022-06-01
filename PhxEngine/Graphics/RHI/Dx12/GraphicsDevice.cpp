#include "phxpch.h"
#include "GraphicsDevice.h"

#include "BiindlessDescriptorTable.h"
#include "CommandList.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#ifdef _DEBUG
#pragma comment(lib, "dxguid.lib")
#endif
using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::Dx12;

static const GUID RenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
static const GUID PixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

static bool sDebugEnabled = false;


DXGI_FORMAT ConvertFormat(FormatType format)
{
	return Dx12::GetDxgiFormatMapping(format).srvFormat;
}

D3D12_SHADER_VISIBILITY ConvertShaderStage(ShaderStage s)
{
	switch (s)  // NOLINT(clang-diagnostic-switch-enum)
	{
	case ShaderStage::Vertex:
		return D3D12_SHADER_VISIBILITY_VERTEX;
	case ShaderStage::Hull:
		return D3D12_SHADER_VISIBILITY_HULL;
	case ShaderStage::Domain:
		return D3D12_SHADER_VISIBILITY_DOMAIN;
	case ShaderStage::Geometry:
		return D3D12_SHADER_VISIBILITY_GEOMETRY;
	case ShaderStage::Pixel:
		return D3D12_SHADER_VISIBILITY_PIXEL;
	case ShaderStage::Amplification:
		return D3D12_SHADER_VISIBILITY_AMPLIFICATION;
	case ShaderStage::Mesh:
		return D3D12_SHADER_VISIBILITY_MESH;

	default:
		// catch-all case - actually some of the bitfield combinations are unrepresentable in DX12
		return D3D12_SHADER_VISIBILITY_ALL;
	}
}

D3D12_BLEND ConvertBlendValue(BlendFactor value)
{
	switch (value)
	{
	case BlendFactor::Zero:
		return D3D12_BLEND_ZERO;
	case BlendFactor::One:
		return D3D12_BLEND_ONE;
	case BlendFactor::SrcColor:
		return D3D12_BLEND_SRC_COLOR;
	case BlendFactor::InvSrcColor:
		return D3D12_BLEND_INV_SRC_COLOR;
	case BlendFactor::SrcAlpha:
		return D3D12_BLEND_SRC_ALPHA;
	case BlendFactor::InvSrcAlpha:
		return D3D12_BLEND_INV_SRC_ALPHA;
	case BlendFactor::DstAlpha:
		return D3D12_BLEND_DEST_ALPHA;
	case BlendFactor::InvDstAlpha:
		return D3D12_BLEND_INV_DEST_ALPHA;
	case BlendFactor::DstColor:
		return D3D12_BLEND_DEST_COLOR;
	case BlendFactor::InvDstColor:
		return D3D12_BLEND_INV_DEST_COLOR;
	case BlendFactor::SrcAlphaSaturate:
		return D3D12_BLEND_SRC_ALPHA_SAT;
	case BlendFactor::ConstantColor:
		return D3D12_BLEND_BLEND_FACTOR;
	case BlendFactor::InvConstantColor:
		return D3D12_BLEND_INV_BLEND_FACTOR;
	case BlendFactor::Src1Color:
		return D3D12_BLEND_SRC1_COLOR;
	case BlendFactor::InvSrc1Color:
		return D3D12_BLEND_INV_SRC1_COLOR;
	case BlendFactor::Src1Alpha:
		return D3D12_BLEND_SRC1_ALPHA;
	case BlendFactor::InvSrc1Alpha:
		return D3D12_BLEND_INV_SRC1_ALPHA;
	default:
		return D3D12_BLEND_ZERO;
	}
}

D3D12_BLEND_OP ConvertBlendOp(EBlendOp value)
{
	switch (value)
	{
	case EBlendOp::Add:
		return D3D12_BLEND_OP_ADD;
	case EBlendOp::Subrtact:
		return D3D12_BLEND_OP_SUBTRACT;
	case EBlendOp::ReverseSubtract:
		return D3D12_BLEND_OP_REV_SUBTRACT;
	case EBlendOp::Min:
		return D3D12_BLEND_OP_MIN;
	case EBlendOp::Max:
		return D3D12_BLEND_OP_MAX;
	default:
		return D3D12_BLEND_OP_ADD;
	}
}

D3D12_STENCIL_OP ConvertStencilOp(StencilOp value)
{
	switch (value)
	{
	case StencilOp::Keep:
		return D3D12_STENCIL_OP_KEEP;
	case StencilOp::Zero:
		return D3D12_STENCIL_OP_ZERO;
	case StencilOp::Replace:
		return D3D12_STENCIL_OP_REPLACE;
	case StencilOp::IncrementAndClamp:
		return D3D12_STENCIL_OP_INCR_SAT;
	case StencilOp::DecrementAndClamp:
		return D3D12_STENCIL_OP_DECR_SAT;
	case StencilOp::Invert:
		return D3D12_STENCIL_OP_INVERT;
	case StencilOp::IncrementAndWrap:
		return D3D12_STENCIL_OP_INCR;
	case StencilOp::DecrementAndWrap:
		return D3D12_STENCIL_OP_DECR;
	default:
		return D3D12_STENCIL_OP_KEEP;
	}
}

D3D12_COMPARISON_FUNC ConvertComparisonFunc(ComparisonFunc value)
{
	switch (value)
	{
	case ComparisonFunc::Never:
		return D3D12_COMPARISON_FUNC_NEVER;
	case ComparisonFunc::Less:
		return D3D12_COMPARISON_FUNC_LESS;
	case ComparisonFunc::Equal:
		return D3D12_COMPARISON_FUNC_EQUAL;
	case ComparisonFunc::LessOrEqual:
		return D3D12_COMPARISON_FUNC_LESS_EQUAL;
	case ComparisonFunc::Greater:
		return D3D12_COMPARISON_FUNC_GREATER;
	case ComparisonFunc::NotEqual:
		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case ComparisonFunc::GreaterOrEqual:
		return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case ComparisonFunc::Always:
		return D3D12_COMPARISON_FUNC_ALWAYS;
	default:
		return D3D12_COMPARISON_FUNC_NEVER;
	}
}
D3D_PRIMITIVE_TOPOLOGY ConvertPrimitiveType(PrimitiveType pt, uint32_t controlPoints)
{
	switch (pt)
	{
	case PrimitiveType::PointList:
		return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	case PrimitiveType::LineList:
		return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	case PrimitiveType::TriangleList:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case PrimitiveType::TriangleStrip:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	case PrimitiveType::TriangleFan:
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	case PrimitiveType::TriangleListWithAdjacency:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
	case PrimitiveType::TriangleStripWithAdjacency:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
	case PrimitiveType::PatchList:
		if (controlPoints == 0 || controlPoints > 32)
		{
			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
		return D3D_PRIMITIVE_TOPOLOGY(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (controlPoints - 1));
	default:
		return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	}
}

D3D12_TEXTURE_ADDRESS_MODE ConvertSamplerAddressMode(SamplerAddressMode mode)
{
	switch (mode)
	{
	case SamplerAddressMode::Clamp:
		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case SamplerAddressMode::Wrap:
		return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	case SamplerAddressMode::Border:
		return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	case SamplerAddressMode::Mirror:
		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	case SamplerAddressMode::MirrorOnce:
		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	default:
		return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	}
}

UINT ConvertSamplerReductionType(SamplerReductionType reductionType)
{
	switch (reductionType)
	{
	case SamplerReductionType::Standard:
		return D3D12_FILTER_REDUCTION_TYPE_STANDARD;
	case SamplerReductionType::Comparison:
		return D3D12_FILTER_REDUCTION_TYPE_COMPARISON;
	case SamplerReductionType::Minimum:
		return D3D12_FILTER_REDUCTION_TYPE_MINIMUM;
	case SamplerReductionType::Maximum:
		return D3D12_FILTER_REDUCTION_TYPE_MAXIMUM;
	default:
		return D3D12_FILTER_REDUCTION_TYPE_STANDARD;
	}
}

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


	this->m_bindlessResourceDescriptorTable = std::make_unique<BindlessDescriptorTable>(
		this->GetResourceGpuHeap()->Allocate(NUM_BINDLESS_RESOURCES));
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

	this->RunGarbageCollection();
}

void PhxEngine::RHI::Dx12::GraphicsDevice::CreateSwapChain(SwapChainDesc const& swapChainDesc)
{
	assert(swapChainDesc.WindowHandle);
	if (!swapChainDesc.WindowHandle)
	{
		// LOG_CORE_ERROR("Invalid window handle");
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

	Microsoft::WRL::ComPtr<IDXGISwapChain1> tmpSwapChain;
	ThrowIfFailed(
		this->m_factory->CreateSwapChainForHwnd(
			this->GetGfxQueue()->GetD3D12CommandQueue(),
			static_cast<HWND>(swapChainDesc.WindowHandle),
			&dx12Desc,
			nullptr,
			nullptr,
			&tmpSwapChain));

	ThrowIfFailed(
		tmpSwapChain.As(&this->m_swapChain.DxgiSwapchain));

	this->m_swapChain.Desc = swapChainDesc;

	this->m_swapChain.BackBuffers.resize(this->m_swapChain.Desc.BufferCount);
	this->m_frameFence.resize(this->m_swapChain.Desc.BufferCount, 0);
	for (UINT i = 0; i < this->m_swapChain.Desc.BufferCount; i++)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(
			this->m_swapChain.DxgiSwapchain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		auto textureDesc = TextureDesc();
		textureDesc.Dimension = TextureDimension::Texture2D;
		textureDesc.Format = this->m_swapChain.Desc.Format;
		textureDesc.Width = this->m_swapChain.Desc.Width;
		textureDesc.Height = this->m_swapChain.Desc.Height;

		char allocatorName[32];
		sprintf_s(allocatorName, "Back Buffer %iu", i);
		textureDesc.DebugName = std::string(allocatorName);

		this->m_swapChain.BackBuffers[i] = this->CreateRenderTarget(textureDesc, backBuffer);
	}
}

CommandListHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateCommandList(CommandListDesc const& desc)
{
	auto commandListImpl = std::make_unique<CommandList>(*this, desc);
	return commandListImpl;
}

ShaderHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateShader(ShaderDesc const& desc, const void* binary, size_t binarySize)
{
	auto shaderImpl = std::make_unique<Shader>(desc, binary, binarySize);

	// Create Root signature data

	/* This is not working, we don't need it right now. Good for reflection data.
	ThrowIfFailed(
		D3D12CreateVersionedRootSignatureDeserializer(
			shaderImpl->GetByteCode().data(),
			shaderImpl->GetByteCode().size(),
			IID_PPV_ARGS(&shaderImpl->GetRootSigDeserializer())));

	assert(shaderImpl->GetRootSigDeserializer()->GetRootSignatureDesc());
	*/

	// TODO: Check version

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
	auto hr = 
		this->m_device->CreateRootSignature(
			0,
			shaderImpl->GetByteCode().data(),
			shaderImpl->GetByteCode().size(),
			IID_PPV_ARGS(&rootSig));

	shaderImpl->SetRootSignature(rootSig);
	return shaderImpl;
}

InputLayoutHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateInputLayout(VertexAttributeDesc* desc, uint32_t attributeCount)
{
	auto inputLayoutImpl = std::make_unique<InputLayout>();
	inputLayoutImpl->Attributes.resize(attributeCount);

	for (uint32_t index = 0; index < attributeCount; index++)
	{
		VertexAttributeDesc& attr = inputLayoutImpl->Attributes[index];

		// Copy the description to get a stable name pointer in desc
		attr = desc[index];

		D3D12_INPUT_ELEMENT_DESC& desc = inputLayoutImpl->InputElements.emplace_back();

		desc.SemanticName = attr.SemanticName.c_str();
		desc.SemanticIndex = attr.SemanticIndex;


		const DxgiFormatMapping& formatMapping = GetDxgiFormatMapping(attr.Format);
		desc.Format = formatMapping.srvFormat;
		desc.InputSlot = attr.InputSlot;
		desc.AlignedByteOffset = attr.AlignedByteOffset;
		if (desc.AlignedByteOffset == VertexAttributeDesc::SAppendAlignedElement)
		{
			desc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		}

		if (attr.IsInstanced)
		{
			desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
			desc.InstanceDataStepRate = 1;
		}
		else
		{
			desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			desc.InstanceDataStepRate = 0;
		}
	}

	return inputLayoutImpl;
}

GraphicsPSOHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateGraphicsPSOHandle(GraphicsPSODesc const& desc)
{
	std::unique_ptr<GraphicsPSO> psoImpl = std::make_unique<GraphicsPSO>();

	if (desc.VertexShader)
	{
		auto shaderImpl = std::static_pointer_cast<Shader>(desc.VertexShader);
		if (psoImpl->RootSignature == nullptr)
		{
			psoImpl->RootSignature = shaderImpl->GetRootSignature();
		}
	}

	if (desc.PixelShader)
	{
		auto shaderImpl = std::static_pointer_cast<Shader>(desc.PixelShader);
		if (psoImpl->RootSignature == nullptr)
		{
			psoImpl->RootSignature = shaderImpl->GetRootSignature();
		}
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12Desc = {};
	d3d12Desc.pRootSignature = psoImpl->RootSignature.Get();

	std::shared_ptr<Shader> shaderImpl = nullptr;
	shaderImpl = std::static_pointer_cast<Shader>(desc.VertexShader);
	if (shaderImpl)
	{
		d3d12Desc.VS = { shaderImpl->GetByteCode().data(), shaderImpl->GetByteCode().size() };
	}

	shaderImpl = std::static_pointer_cast<Shader>(desc.HullShader);
	if (shaderImpl)
	{
		d3d12Desc.HS = { shaderImpl->GetByteCode().data(), shaderImpl->GetByteCode().size() };
	}

	shaderImpl = std::static_pointer_cast<Shader>(desc.DomainShader);
	if (shaderImpl)
	{
		d3d12Desc.DS = { shaderImpl->GetByteCode().data(), shaderImpl->GetByteCode().size() };
	}

	shaderImpl = std::static_pointer_cast<Shader>(desc.GeometryShader);
	if (shaderImpl)
	{
		d3d12Desc.GS = { shaderImpl->GetByteCode().data(), shaderImpl->GetByteCode().size() };
	}

	shaderImpl = std::static_pointer_cast<Shader>(desc.PixelShader);
	if (shaderImpl)
	{
		d3d12Desc.PS = { shaderImpl->GetByteCode().data(), shaderImpl->GetByteCode().size() };
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
		d3d12Desc.DSVFormat = GetDxgiFormatMapping(desc.DsvFormat.value()).rtvFormat;
	}

	d3d12Desc.SampleDesc.Count = desc.SampleCount;
	d3d12Desc.SampleDesc.Quality = desc.SampleQuality;

	for (size_t i = 0; i < desc.RtvFormats.size(); i++)
	{
		d3d12Desc.RTVFormats[i] = GetDxgiFormatMapping(desc.RtvFormats[i]).rtvFormat;
	}

	auto inputLayout = std::static_pointer_cast<InputLayout>(desc.InputLayout);
	if (inputLayout && !inputLayout->InputElements.empty())
	{
		d3d12Desc.InputLayout.NumElements = uint32_t(inputLayout->InputElements.size());
		d3d12Desc.InputLayout.pInputElementDescs = &(inputLayout->InputElements[0]);
	}

	d3d12Desc.NumRenderTargets = (uint32_t)desc.RtvFormats.size();
	d3d12Desc.SampleMask = ~0u;

	ThrowIfFailed(
		this->GetD3D12Device2()->CreateGraphicsPipelineState(&d3d12Desc, IID_PPV_ARGS(&psoImpl->D3D12PipelineState)));

	return psoImpl;
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
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resourceDesc = 
		CD3DX12_RESOURCE_DESC::Tex2D(
			dxgiFormatMapping.rtvFormat,
			desc.Width,
			desc.Height,
			desc.ArraySize,
			desc.MipLevels,
			1,
			0,
			resourceFlags);
	Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource;
	ThrowIfFailed(
		this->GetD3D12Device2()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			desc.OptmizedClearValue.has_value() ? &d3d12OptimizedClearValue : nullptr,
			IID_PPV_ARGS(&d3d12Resource)));

	// Create DSV
	auto textureImpl = std::make_unique<Texture>();
	textureImpl->Desc = desc;
	textureImpl->D3D12Resource = d3d12Resource;
	textureImpl->DsvAllocation = this->GetDsvCpuHeap()->Allocate(1);
	textureImpl->SrvAllocation = this->GetResourceCpuHeap()->Allocate(1);

	std::wstring debugName(desc.DebugName.begin(), desc.DebugName.end());
	textureImpl->D3D12Resource->SetName(debugName.c_str());

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = dxgiFormatMapping.rtvFormat;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // TODO: use desc.
	dsvDesc.Texture2D.MipSlice = 0;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	this->GetD3D12Device2()->CreateDepthStencilView(
		textureImpl->D3D12Resource.Get(),
		&dsvDesc,
		textureImpl->DsvAllocation.GetCpuHandle());


	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = dxgiFormatMapping.srvFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (desc.Dimension == TextureDimension::TextureCube)
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;  // Only 2D textures are supported (this was checked in the calling function).
		srvDesc.TextureCube.MipLevels = desc.MipLevels;
		srvDesc.TextureCube.MostDetailedMip = 0;
	}
	else
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;  // Only 2D textures are supported (this was checked in the calling function).
		srvDesc.Texture2D.MipLevels = desc.MipLevels;
	}

	this->GetD3D12Device2()->CreateShaderResourceView(
		textureImpl->D3D12Resource.Get(),
		&srvDesc,
		textureImpl->SrvAllocation.GetCpuHandle());

	return textureImpl;
}

TextureHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateTexture(TextureDesc const& desc)
{
	auto& dxgiFormatMapping = GetDxgiFormatMapping(desc.Format);

	D3D12_CLEAR_VALUE d3d12OptimizedClearValue = {};
	if (desc.OptmizedClearValue.has_value())
	{
		d3d12OptimizedClearValue.Format = dxgiFormatMapping.rtvFormat;
		d3d12OptimizedClearValue.DepthStencil.Depth = desc.OptmizedClearValue.value().R;
		d3d12OptimizedClearValue.DepthStencil.Stencil = desc.OptmizedClearValue.value().G;
	}

	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resourceDesc =
		CD3DX12_RESOURCE_DESC::Tex2D(
			dxgiFormatMapping.srvFormat,
			desc.Width,
			desc.Height,
			desc.ArraySize,
			desc.MipLevels,
			1,
			0,
			D3D12_RESOURCE_FLAG_NONE);

	Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource;
	ThrowIfFailed(
		this->GetD3D12Device2()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			desc.OptmizedClearValue.has_value() ? &d3d12OptimizedClearValue : nullptr,
			IID_PPV_ARGS(&d3d12Resource)));

	// Create SRV
	auto textureImpl = std::make_unique<Texture>();
	textureImpl->Desc = desc;
	textureImpl->D3D12Resource = d3d12Resource;
	textureImpl->SrvAllocation = this->GetResourceCpuHeap()->Allocate(1);

	textureImpl->D3D12Resource->SetName(std::wstring(desc.DebugName.begin(), desc.DebugName.end()).c_str());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = dxgiFormatMapping.srvFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (desc.Dimension == TextureDimension::TextureCube)
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;  // Only 2D textures are supported (this was checked in the calling function).
		srvDesc.TextureCube.MipLevels = desc.MipLevels;
		srvDesc.TextureCube.MostDetailedMip = 0;
	}
	else
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;  // Only 2D textures are supported (this was checked in the calling function).
		srvDesc.Texture2D.MipLevels = desc.MipLevels;
	}

	this->GetD3D12Device2()->CreateShaderResourceView(
		textureImpl->D3D12Resource.Get(),
		&srvDesc,
		textureImpl->SrvAllocation.GetCpuHandle());

	// Copy Descriptor to Bindless since we are creating a texture as a shader resource view
	textureImpl->BindlessResourceIndex = this->m_bindlessResourceDescriptorTable->Allocate();
	if (textureImpl->BindlessResourceIndex != cInvalidDescriptorIndex)
	{
		this->GetD3D12Device2()->CopyDescriptorsSimple(
			1,
			this->m_bindlessResourceDescriptorTable->GetCpuHandle(textureImpl->BindlessResourceIndex),
			textureImpl->SrvAllocation.GetCpuHandle(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	
	return textureImpl;
}

BufferHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateIndexBuffer(BufferDesc const& desc)
{
	auto bufferImpl = this->CreateBufferInternal(desc);

	bufferImpl->IndexView = {};
	auto& view = bufferImpl->IndexView;
	view.BufferLocation = bufferImpl->D3D12Resource->GetGPUVirtualAddress();
	view.Format = bufferImpl->GetDesc().StrideInBytes == sizeof(uint32_t)
		? DXGI_FORMAT_R32_UINT
		: DXGI_FORMAT_R16_UINT;
	view.SizeInBytes = bufferImpl->GetDesc().SizeInBytes;

	if (desc.CreateSRVViews || (desc.MiscFlags & BufferMiscFlags::SrvView) == 1)
	{
		this->CreateSRVViews(bufferImpl.get());
	}

	return bufferImpl;
}

BufferHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateVertexBuffer(BufferDesc const& desc)
{
	auto bufferImpl = this->CreateBufferInternal(desc);

	bufferImpl->VertexView = {};
	auto& view = bufferImpl->VertexView;
	view.BufferLocation = bufferImpl->D3D12Resource->GetGPUVirtualAddress();
	view.StrideInBytes = bufferImpl->GetDesc().StrideInBytes;
	view.SizeInBytes = bufferImpl->GetDesc().SizeInBytes;

	if (desc.CreateSRVViews || (desc.MiscFlags & BufferMiscFlags::SrvView) == 1)
	{
		this->CreateSRVViews(bufferImpl.get());
	}

	return bufferImpl;
}

BufferHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateBuffer(BufferDesc const& desc)
{
	auto bufferImpl = this->CreateBufferInternal(desc);

	if (desc.CreateSRVViews || (desc.MiscFlags & BufferMiscFlags::SrvView) == 1)
	{
		this->CreateSRVViews(bufferImpl.get());
	}

	return bufferImpl;
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
		d3d12CommandLists[i] = static_cast<CommandList*>(pCommandLists[i])->GetD3D12CommandList();
	}

	auto* queue = this->GetQueue(executionQueue);

	auto fenceValue = queue->ExecuteCommandLists(d3d12CommandLists);

	if (waitForCompletion)
	{
		queue->WaitForFence(fenceValue);

		for (size_t i = 0; i < numCommandLists; i++)
		{
			static_cast<CommandList*>(pCommandLists[i])->Executed(fenceValue);
		}
	}
	else
	{
		for (size_t i = 0; i < numCommandLists; i++)
		{
			auto trackedResources = static_cast<CommandList*>(pCommandLists[i])->Executed(fenceValue);
			InflightDataEntry entry = { fenceValue, trackedResources };
			this->m_inflightData[(int)executionQueue].emplace_back(entry);
		}
	}

	return fenceValue;
}

TextureHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateRenderTarget(
	TextureDesc const& desc,
	Microsoft::WRL::ComPtr<ID3D12Resource> d3d12TextureResource)
{
	// Construct Texture
	auto textureImpl = std::make_unique<Texture>();
	textureImpl->Desc = desc;
	textureImpl->D3D12Resource = d3d12TextureResource;
	textureImpl->RtvAllocation = this->GetRtvCpuHeap()->Allocate(1);

	this->GetD3D12Device2()->CreateRenderTargetView(
		textureImpl->D3D12Resource.Get(),
		nullptr,
		textureImpl->RtvAllocation.GetCpuHandle());

	// Set debug name
	std::wstring debugName(textureImpl->Desc.DebugName.begin(), textureImpl->Desc.DebugName.end());
	textureImpl->D3D12Resource->SetName(debugName.c_str());

	return textureImpl;
}

/*
RootSignatureHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateRootSignature(GraphicsPSODesc const& desc)
{
	/*
	auto rootSignatureImpl = std::make_unique<RootSignature>();

	assert(desc.RootSignatureBuilder);
	RootSignatureBuilder* rootSignatureBuilder = SafeCast<RootSignatureBuilder*>(desc.RootSignatureBuilder);

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc = {};
	rsDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;

	rsDesc.Desc_1_1 = rootSignatureBuilder->Build();

	if (desc.InputLayout)
	{
		rsDesc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	}

	std::shared_ptr<ID3DBlob> rsBlob;
	std::shared_ptr<ID3DBlob> errorBlob;
	ThrowIfFailed(
		D3D12SerializeVersionedRootSignature(&rsDesc, &rsBlob, &errorBlob));

	ThrowIfFailed(
		this->GetD3D12Device2()->CreateRootSignature(
			0,
			rsBlob->GetBufferPointer(),
			rsBlob->GetBufferSize(),
			IID_PPV_ARGS(&rootSignatureImpl->D3D12RootSignature)));

	return RootSignatureHandle::Create(rootSignatureImpl.release());
	std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters;

	for (const auto& parameter : desc.ShaderParameters.Parameters)
	{
		switch (parameter.Type)
		{
		case ResourceType::PushConstants:
			rootSignatureImpl->PushConstantByteSize = parameter.Size / 4;
			
			rootParameters.emplace_back().InitAsConstants(
				rootSignatureImpl->PushConstantByteSize,
				parameter.Slot);
			break;

		case ResourceType::ConstantBuffer:
			rootParameters.emplace_back().InitAsConstantBufferView(
				parameter.Slot,
				0,
				parameter.IsVolatile ? D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC : D3D12_ROOT_DESCRIPTOR_FLAG_NONE);
			break;

		case ResourceType::SRVBuffer:
			rootParameters.emplace_back().InitAsShaderResourceView(
				parameter.Slot,
				0,
				parameter.IsVolatile ? D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC : D3D12_ROOT_DESCRIPTOR_FLAG_NONE);
			break;

		case ResourceType::SRVTexture:
			assert(false);
			break;
		}
	}

	// Create Enable Bindless Table
	if (!desc.ShaderParameters.BindlessParameters.empty())
	{
		std::vector<CD3DX12_DESCRIPTOR_RANGE1> bindlessRanges;
		rootSignatureImpl->BindBindlessTables = true;
		for (const auto& bindlessParam : desc.ShaderParameters.BindlessParameters)
		{
			auto& range = bindlessRanges.emplace_back();
			switch (bindlessParam.Type)
			{
			case ResourceType::BindlessSRV:
				range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				break;
			default:
				throw std::exception("Not implemented yet");
			}
			range.BaseShaderRegister = 0;
			range.RegisterSpace = bindlessParam.RegisterSpace;
			range.OffsetInDescriptorsFromTableStart = 0;
			range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
		}

		rootParameters.emplace_back().InitAsDescriptorTable(
			bindlessRanges.size(),
			bindlessRanges.data());
	}

	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers(desc.ShaderParameters.StaticSamplers.size());
	for (int i = 0; i < staticSamplers.size(); i++)
	{
		const auto& sampler = desc.ShaderParameters.StaticSamplers[i];

		staticSamplers[i].ShaderRegister = sampler.Slot;
		staticSamplers[i].RegisterSpace = 0;

		UINT reductionType = ConvertSamplerReductionType(sampler.ReductionType);
		if (sampler.MaxAnisotropy > 1.0f)
		{
			staticSamplers[i].Filter = D3D12_ENCODE_ANISOTROPIC_FILTER(reductionType);
		}
		else
		{
			staticSamplers[i].Filter = D3D12_ENCODE_BASIC_FILTER(
				sampler.MinFilter ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT,
				sampler.MagFilter ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT,
				sampler.MipFilter ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT,
				reductionType);
		}

		staticSamplers[i].AddressU = ConvertSamplerAddressMode(sampler.AddressU);
		staticSamplers[i].AddressV = ConvertSamplerAddressMode(sampler.AddressV);
		staticSamplers[i].AddressW = ConvertSamplerAddressMode(sampler.AddressW);

		staticSamplers[i].MipLODBias = sampler.MipBias;
		staticSamplers[i].MaxAnisotropy = std::max((UINT)sampler.MaxAnisotropy, 1U);
		staticSamplers[i].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
		staticSamplers[i].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		staticSamplers[i].MinLOD = 0;
		staticSamplers[i].MaxLOD = D3D12_FLOAT32_MAX;
	}

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc = {};
	rsDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (desc.InputLayout)
	{
		rsDesc.Desc_1_1.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	}

	if (!rootParameters.empty())
	{
		rsDesc.Desc_1_1.pParameters = rootParameters.data();
		rsDesc.Desc_1_1.NumParameters = UINT(rootParameters.size());
	}

	if (!staticSamplers.empty())
	{
		rsDesc.Desc_1_1.pStaticSamplers = staticSamplers.data();
		rsDesc.Desc_1_1.NumStaticSamplers = UINT(staticSamplers.size());
	}
	// Serialize the root signature

	std::shared_ptr<ID3DBlob> rsBlob;
	std::shared_ptr<ID3DBlob> errorBlob;
	ThrowIfFailed(
		D3D12SerializeVersionedRootSignature(&rsDesc, &rsBlob, &errorBlob));

	ThrowIfFailed(
		this->GetD3D12Device2()->CreateRootSignature(
			0,
			rsBlob->GetBufferPointer(),
			rsBlob->GetBufferSize(),
			IID_PPV_ARGS(&rootSignatureImpl->D3D12RootSignature)));

	return RootSignatureHandle::Create(rootSignatureImpl.release());
}
	*/


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

Microsoft::WRL::ComPtr<IDXGIAdapter1> PhxEngine::RHI::Dx12::GraphicsDevice::SelectOptimalGpu()
{
	// LOG_CORE_INFO("Selecting Optimal GPU");
	Microsoft::WRL::ComPtr<IDXGIAdapter1> selectedGpu;

	size_t selectedGPUVideoMemeory = 0;
	for (auto adapter : EnumerateAdapters(this->m_factory))
	{
		DXGI_ADAPTER_DESC desc = {};
		adapter->GetDesc(&desc);

		std::string name = NarrowString(desc.Description);
		size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
		size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
		size_t sharedSystemMemory = desc.SharedSystemMemory;

		/*
		LOG_CORE_INFO(
			"\t%s [VRAM = %zu MB, SRAM = %zu MB, SharedRAM = %zu MB]",
			name,
			BYTE_TO_MB(dedicatedVideoMemory),
			BYTE_TO_MB(dedicatedSystemMemory),
			BYTE_TO_MB(sharedSystemMemory));
			*/
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

	/*
	LOG_CORE_INFO(
		"Selected GPU %s [VRAM = %zu MB, SRAM = %zu MB, SharedRAM = %zu MB]",
		name,
		BYTE_TO_MB(dedicatedVideoMemory),
		BYTE_TO_MB(dedicatedSystemMemory),
		BYTE_TO_MB(sharedSystemMemory));
	*/
	return selectedGpu;
}

std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>> PhxEngine::RHI::Dx12::GraphicsDevice::EnumerateAdapters(Microsoft::WRL::ComPtr<IDXGIFactory6> factory, bool includeSoftwareAdapter)
{
	std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>> adapters;
	auto nextAdapter = [&](uint32_t adapterIndex, Microsoft::WRL::ComPtr<IDXGIAdapter1>& adapter)
	{
		return factory->EnumAdapterByGpuPreference(
			adapterIndex,
			DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
			IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()));
	};

	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
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

void PhxEngine::RHI::Dx12::GraphicsDevice::TranslateBlendState(BlendRenderState const& inState, D3D12_BLEND_DESC& outState)
{
	outState.AlphaToCoverageEnable = inState.alphaToCoverageEnable;
	outState.IndependentBlendEnable = true;

	for (uint32_t i = 0; i < cMaxRenderTargets; i++)
	{
		const auto& src = inState.Targets[i];
		auto& dst = outState.RenderTarget[i];


		dst.BlendEnable = src.BlendEnable ? TRUE : FALSE;
		dst.SrcBlend = ConvertBlendValue(src.SrcBlend);
		dst.DestBlend = ConvertBlendValue(src.DestBlend);
		dst.BlendOp = ConvertBlendOp(src.BlendOp);
		dst.SrcBlendAlpha = ConvertBlendValue(src.SrcBlendAlpha);
		dst.DestBlendAlpha = ConvertBlendValue(src.DestBlendAlpha);
		dst.BlendOpAlpha = ConvertBlendOp(src.BlendOpAlpha);
		dst.RenderTargetWriteMask = (D3D12_COLOR_WRITE_ENABLE)src.ColorWriteMask;
	}
}

void PhxEngine::RHI::Dx12::GraphicsDevice::TranslateDepthStencilState(DepthStencilRenderState const& inState, D3D12_DEPTH_STENCIL_DESC& outState)
{
	outState.DepthEnable = inState.DepthTestEnable ? TRUE : FALSE;
	outState.DepthWriteMask = inState.DepthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	outState.DepthFunc = ConvertComparisonFunc(inState.DepthFunc);
	outState.StencilEnable = inState.StencilEnable ? TRUE : FALSE;
	outState.StencilReadMask = (UINT8)inState.StencilReadMask;
	outState.StencilWriteMask = (UINT8)inState.StencilWriteMask;
	outState.FrontFace.StencilFailOp = ConvertStencilOp(inState.FrontFaceStencil.FailOp);
	outState.FrontFace.StencilDepthFailOp = ConvertStencilOp(inState.FrontFaceStencil.DepthFailOp);
	outState.FrontFace.StencilPassOp = ConvertStencilOp(inState.FrontFaceStencil.PassOp);
	outState.FrontFace.StencilFunc = ConvertComparisonFunc(inState.FrontFaceStencil.StencilFunc);
	outState.BackFace.StencilFailOp = ConvertStencilOp(inState.BackFaceStencil.FailOp);
	outState.BackFace.StencilDepthFailOp = ConvertStencilOp(inState.BackFaceStencil.DepthFailOp);
	outState.BackFace.StencilPassOp = ConvertStencilOp(inState.BackFaceStencil.PassOp);
	outState.BackFace.StencilFunc = ConvertComparisonFunc(inState.BackFaceStencil.StencilFunc);
}

void PhxEngine::RHI::Dx12::GraphicsDevice::TranslateRasterState(RasterRenderState const& inState, D3D12_RASTERIZER_DESC& outState)
{
	switch (inState.FillMode)
	{
	case RasterFillMode::Solid:
		outState.FillMode = D3D12_FILL_MODE_SOLID;
		break;
	case RasterFillMode::Wireframe:
		outState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		break;
	default:
		break;
	}

	switch (inState.CullMode)
	{
	case RasterCullMode::Back:
		outState.CullMode = D3D12_CULL_MODE_BACK;
		break;
	case RasterCullMode::Front:
		outState.CullMode = D3D12_CULL_MODE_FRONT;
		break;
	case RasterCullMode::None:
		outState.CullMode = D3D12_CULL_MODE_NONE;
		break;
	default:
		break;
	}

	outState.FrontCounterClockwise = inState.FrontCounterClockwise ? TRUE : FALSE;
	outState.DepthBias = inState.DepthBias;
	outState.DepthBiasClamp = inState.DepthBiasClamp;
	outState.SlopeScaledDepthBias = inState.SlopeScaledDepthBias;
	outState.DepthClipEnable = inState.DepthClipEnable ? TRUE : FALSE;
	outState.MultisampleEnable = inState.MultisampleEnable ? TRUE : FALSE;
	outState.AntialiasedLineEnable = inState.AntialiasedLineEnable ? TRUE : FALSE;
	outState.ConservativeRaster = inState.ConservativeRasterEnable ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	outState.ForcedSampleCount = inState.ForcedSampleCount;
}

size_t PhxEngine::RHI::Dx12::GraphicsDevice::GetCurrentBackBufferIndex() const
{
	assert(this->m_swapChain.DxgiSwapchain);
	
	return (size_t)this->m_swapChain.DxgiSwapchain->GetCurrentBackBufferIndex();
}

std::unique_ptr<GpuBuffer> GraphicsDevice::CreateBufferInternal(BufferDesc const& desc)
{
	D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
	if (desc.AllowUnorderedAccess)
	{
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	std::unique_ptr<GpuBuffer> bufferImpl = std::make_unique<GpuBuffer>();
	bufferImpl->Desc = desc;
	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(desc.SizeInBytes, resourceFlags);
	// Create a committed resource for the GPU resource in a default heap.
	ThrowIfFailed(
		this->GetD3D12Device2()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&bufferImpl->D3D12Resource)));


	std::wstring debugName(desc.DebugName.begin(), desc.DebugName.end());
	bufferImpl->D3D12Resource->SetName(debugName.c_str());

	return bufferImpl;
}

void PhxEngine::RHI::Dx12::GraphicsDevice::CreateSRVViews(GpuBuffer* gpuBuffer)
{
	gpuBuffer->SrvAllocation = this->GetResourceCpuHeap()->Allocate(1);

	const BufferDesc& desc = gpuBuffer->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;

	if (gpuBuffer->Desc.Format == FormatType::UNKNOWN)
	{
		if ((gpuBuffer->Desc.MiscFlags & BufferMiscFlags::Raw) == 1)
		{
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
			srvDesc.Buffer.NumElements = desc.SizeInBytes / sizeof(uint32_t);
		}
		else if((gpuBuffer->Desc.MiscFlags & BufferMiscFlags::Structured) == 1)
		{
			// This is a Structured Buffer
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.Buffer.NumElements = desc.SizeInBytes / desc.StrideInBytes;
			srvDesc.Buffer.StructureByteStride = desc.StrideInBytes;
		}
	}
	else
	{
		// This is a typed buffer
		// TODO: Not implemented
		assert(false);
		/*
		uint32_t stride = GetFormatStride(format);
		srv_desc.Format = _ConvertFormat(format);
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srv_desc.Buffer.FirstElement = offset / stride;
		srv_desc.Buffer.NumElements = (UINT)std::min(size, desc.size - offset) / stride;
		*/
	}

	this->GetD3D12Device2()->CreateShaderResourceView(
		gpuBuffer->D3D12Resource.Get(),
		&srvDesc,
		gpuBuffer->SrvAllocation.GetCpuHandle());

	if (gpuBuffer->GetDesc().CreateBindless || (gpuBuffer->GetDesc().MiscFlags & RHI::BufferMiscFlags::Bindless) == 1)
	{
		// Copy Descriptor to Bindless since we are creating a texture as a shader resource view
		gpuBuffer->BindlessResourceIndex = this->m_bindlessResourceDescriptorTable->Allocate();
		if (gpuBuffer->BindlessResourceIndex != cInvalidDescriptorIndex)
		{
			this->GetD3D12Device2()->CopyDescriptorsSimple(
				1,
				this->m_bindlessResourceDescriptorTable->GetCpuHandle(gpuBuffer->BindlessResourceIndex),
				gpuBuffer->SrvAllocation.GetCpuHandle(),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
	}
}

Microsoft::WRL::ComPtr<IDXGIFactory6> PhxEngine::RHI::Dx12::GraphicsDevice::CreateFactory() const
{
	uint32_t flags = 0;
	sDebugEnabled = IsDebuggerPresent();

	if (sDebugEnabled)
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(
			D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));

		debugController->EnableDebugLayer();
		flags = DXGI_CREATE_FACTORY_DEBUG;
	}
	
	Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
	ThrowIfFailed(
		CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory)));

	return factory;
}

void PhxEngine::RHI::Dx12::GraphicsDevice::CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter> gpuAdapter)
{
	ThrowIfFailed(
		D3D12CreateDevice(
			gpuAdapter.Get(),
			D3D_FEATURE_LEVEL_11_1,
			IID_PPV_ARGS(&this->m_device)));

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

	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport5 = {};
	bool hasOptions5 = SUCCEEDED(this->m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport5, sizeof(featureSupport5)));

	if (SUCCEEDED(this->m_device.As(&this->m_device5)) && hasOptions5)
	{
		this->IsDxrSupported = featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0;
		this->IsRenderPassSupported = featureSupport5.RenderPassesTier >= D3D12_RENDER_PASS_TIER_0;
		this->IsRayQuerySupported = featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1;
	}


	D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureSupport6 = {};
	bool hasOptions6 = SUCCEEDED(this->m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureSupport6, sizeof(featureSupport6)));

	if (hasOptions6)
	{
		this->IsVariableRateShadingSupported = featureSupport6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2;
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupport7 = {};
	bool hasOptions7 = SUCCEEDED(this->m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureSupport7, sizeof(featureSupport7)));

	if (SUCCEEDED(this->m_device.As(&this->m_device2)) && hasOptions7)
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
	this->m_minShaderModel = ShaderModel::SM_6_6;
	if (FAILED(this->m_device2->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &this->FeatureDataShaderModel, sizeof(this->FeatureDataShaderModel))))
	{
		this->FeatureDataShaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_5;
		this->m_minShaderModel = ShaderModel::SM_6_5;
	}


	static const bool debugEnabled = IsDebuggerPresent();
	if (debugEnabled)
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
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

void PhxEngine::RHI::ReportLiveObjects()
{
	if (sDebugEnabled)
	{
		Microsoft::WRL::ComPtr<IDXGIDebug1> dxgiDebug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
		{
			dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_ALL));
		}
	}
}