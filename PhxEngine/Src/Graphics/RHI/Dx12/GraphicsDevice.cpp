#include "phxpch.h"
#include "GraphicsDevice.h"

#include "PhxEngine/Core/Helpers.h"

#include "BiindlessDescriptorTable.h"
#include "CommandList.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <pix3.h>

#ifdef _DEBUG
#pragma comment(lib, "dxguid.lib")
#endif


using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::Dx12;

static const GUID RenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
static const GUID PixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

static bool sDebugEnabled = false;

// Come up with a better reporting system for resources
#define TRACK_RESOURCES 0
namespace
{
#if TRACK_RESOURCES
	static std::unordered_set<std::string> sTrackedResources;
#endif
}

// TODO: Find a home for this, it's also in scene exporer
constexpr uint64_t AlignTo(uint64_t value, uint64_t alignment)
{
	return ((value + alignment - 1) / alignment) * alignment;
}

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
	: m_frameCount(1)
	, m_timerQueryIndexPool(kTimestampQueryHeapSize)
	, m_texturePool(AlignTo(kResourcePoolSize / sizeof(Dx12Texture), sizeof(Dx12Texture)))
	, m_bufferPool(AlignTo(kResourcePoolSize / sizeof(Dx12Texture), sizeof(Dx12Buffer)))
	, m_renderPassPool(AlignTo(10 * sizeof(Dx12RenderPass), sizeof(Dx12RenderPass)))
{
#if ENABLE_PIX_CAPUTRE
	this->m_pixCaptureModule = PIXLoadLatestWinPixGpuCapturerLibrary();
#endif 

	this->m_factory = this->CreateFactory();
	this->m_gpuAdapter = this->SelectOptimalGpuApdater();
	this->CreateDevice(this->m_gpuAdapter->DxgiAdapter);

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

	// TODO: Data drive device create info
	this->CreateGpuTimestampQueryHeap(this->kTimestampQueryHeapSize);

	BufferDesc desc = {};
	desc.SizeInBytes = this->kTimestampQueryHeapSize * sizeof(uint64_t);
	desc.Usage = Usage::ReadBack;

	// Create GPU Buffer for timestamp readbacks
	this->m_timestampQueryBuffer = this->CreateBuffer(desc);

}

PhxEngine::RHI::Dx12::GraphicsDevice::~GraphicsDevice()
{
	this->DeleteBuffer(this->m_timestampQueryBuffer);

	for (auto handle : this->m_swapChain.BackBuffers)
	{
		this->m_texturePool.Release(handle);
	}

	this->m_swapChain.BackBuffers.clear();
	while (!this->m_deleteQueue.empty())
	{
		DeleteItem& deleteItem = this->m_deleteQueue.front();
		deleteItem.DeleteFn();
		this->m_deleteQueue.pop_front();
	}

#if ENABLE_PIX_CAPUTRE
	FreeLibrary(this->m_pixCaptureModule);
#endif

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

void PhxEngine::RHI::Dx12::GraphicsDevice::QueueWaitForCommandList(CommandQueueType waitQueue, ExecutionReceipt waitOnRecipt)
{
	auto pWaitQueue = this->GetQueue(waitQueue);
	auto executionQueue = this->GetQueue(waitOnRecipt.CommandQueue);

	// assert(waitOnRecipt.FenceValue <= executionQueue->GetLastCompletedFence());
	pWaitQueue->GetD3D12CommandQueue()->Wait(executionQueue->GetFence(), waitOnRecipt.FenceValue);
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

	ThrowIfFailed(
		this->GetD3D12Device2()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->m_frameFence)));
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

GraphicsPSOHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateGraphicsPSO(GraphicsPSODesc const& desc)
{
	std::unique_ptr<GraphicsPSO> psoImpl = std::make_unique<GraphicsPSO>();
	psoImpl->Desc = desc;

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

ComputePSOHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateComputePso(ComputePSODesc const& desc)
{

	std::unique_ptr<ComputePSO> psoImpl = std::make_unique<ComputePSO>();
	psoImpl->Desc = desc;


	assert(desc.ComputeShader);
	auto shaderImpl = std::static_pointer_cast<Shader>(desc.ComputeShader);
	if (psoImpl->RootSignature == nullptr)
	{
		psoImpl->RootSignature = shaderImpl->GetRootSignature();
	}

	D3D12_COMPUTE_PIPELINE_STATE_DESC d3d12Desc = {};
	d3d12Desc.pRootSignature = psoImpl->RootSignature.Get();
	d3d12Desc.CS = { shaderImpl->GetByteCode().data(), shaderImpl->GetByteCode().size() };

	ThrowIfFailed(
		this->GetD3D12Device2()->CreateComputePipelineState(&d3d12Desc, IID_PPV_ARGS(&psoImpl->D3D12PipelineState)));

	return psoImpl;
}

void PhxEngine::RHI::Dx12::GraphicsDevice::CreateRenderPass(RenderPassDesc const& desc)
{
	// TODO: I am here.
	qweqwe
}

void PhxEngine::RHI::Dx12::GraphicsDevice::DeleteRenderPass(RenderPassHandle handle)
{
	aqwdasd
}

TextureHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateDepthStencil(TextureDesc const& desc)
{

	D3D12_CLEAR_VALUE d3d12OptimizedClearValue = {};
	d3d12OptimizedClearValue.Color[0] = desc.OptmizedClearValue.Colour.R;
	d3d12OptimizedClearValue.Color[1] = desc.OptmizedClearValue.Colour.G;
	d3d12OptimizedClearValue.Color[2] = desc.OptmizedClearValue.Colour.B;
	d3d12OptimizedClearValue.Color[3] = desc.OptmizedClearValue.Colour.A;
	d3d12OptimizedClearValue.DepthStencil.Depth = desc.OptmizedClearValue.DepthStencil.Depth;
	d3d12OptimizedClearValue.DepthStencil.Depth = desc.OptmizedClearValue.DepthStencil.Stencil;

	auto dxgiFormatMapping = GetDxgiFormatMapping(desc.Format);
	d3d12OptimizedClearValue.Format = dxgiFormatMapping.rtvFormat;

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
			&d3d12OptimizedClearValue,
			IID_PPV_ARGS(&d3d12Resource)));

	// Create DSV
	Dx12Texture texture = {};
	texture.Desc = desc;
	texture.D3D12Resource = d3d12Resource;
	texture.DsvAllocation = this->GetDsvCpuHeap()->Allocate(1);
	texture.SrvAllocation = this->GetResourceCpuHeap()->Allocate(1);

	std::wstring debugName(desc.DebugName.begin(), desc.DebugName.end());
	texture.D3D12Resource->SetName(debugName.c_str());

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = dxgiFormatMapping.rtvFormat;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // TODO: use desc.
	dsvDesc.Texture2D.MipSlice = 0;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	this->GetD3D12Device2()->CreateDepthStencilView(
		texture.D3D12Resource.Get(),
		&dsvDesc,
		texture.DsvAllocation.GetCpuHandle());


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
		texture.D3D12Resource.Get(),
		&srvDesc,
		texture.SrvAllocation.GetCpuHandle());

	return this->m_texturePool.Insert(texture);
}

TextureHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateTexture(TextureDesc const& desc)
{
	D3D12_CLEAR_VALUE d3d12OptimizedClearValue = {};
	d3d12OptimizedClearValue.Color[0] = desc.OptmizedClearValue.Colour.R;
	d3d12OptimizedClearValue.Color[1] = desc.OptmizedClearValue.Colour.G;
	d3d12OptimizedClearValue.Color[2] = desc.OptmizedClearValue.Colour.B;
	d3d12OptimizedClearValue.Color[3] = desc.OptmizedClearValue.Colour.A;
	d3d12OptimizedClearValue.DepthStencil.Depth = desc.OptmizedClearValue.DepthStencil.Depth;
	d3d12OptimizedClearValue.DepthStencil.Stencil = desc.OptmizedClearValue.DepthStencil.Stencil;

	auto dxgiFormatMapping = GetDxgiFormatMapping(desc.Format);
	d3d12OptimizedClearValue.Format = dxgiFormatMapping.rtvFormat;

	auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
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

	switch (desc.Dimension)
	{
	case TextureDimension::Texture1D:
	case TextureDimension::Texture1DArray:
	{
		resourceDesc =
			CD3DX12_RESOURCE_DESC::Tex1D(
				desc.IsTypeless ? dxgiFormatMapping.resourcFormatType : dxgiFormatMapping.rtvFormat,
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
				desc.IsTypeless ? dxgiFormatMapping.resourcFormatType : dxgiFormatMapping.rtvFormat,
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
				desc.IsTypeless ? dxgiFormatMapping.resourcFormatType : dxgiFormatMapping.rtvFormat,
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

	Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource;
	ThrowIfFailed(
		this->GetD3D12Device2()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			ConvertResourceStates(desc.InitialState),
			useClearValue ? &d3d12OptimizedClearValue : nullptr,
			IID_PPV_ARGS(&d3d12Resource)));

	Dx12Texture texture = {};
	texture.Desc = desc;
	texture.D3D12Resource = d3d12Resource;

	std::wstring debugName;
	Helpers::StringConvert(desc.DebugName, debugName);
	texture.D3D12Resource->SetName(debugName.c_str());

	if ((desc.BindingFlags & BindingFlags::ShaderResource) == BindingFlags::ShaderResource)
	{
		this->CreateShaderResourceView(texture);
	}

	if ((desc.BindingFlags & BindingFlags::RenderTarget) == BindingFlags::RenderTarget)
	{
		this->CreateRenderTargetView(texture);
	}

	if ((desc.BindingFlags & BindingFlags::DepthStencil) == BindingFlags::DepthStencil)
	{
		this->CreateDepthStencilView(texture);
	}

	if ((desc.BindingFlags & BindingFlags::UnorderedAccess) == BindingFlags::UnorderedAccess)
	{
		this->CreateUnorderedAccessView(texture);
	}

	return this->m_texturePool.Insert(texture);
}

const TextureDesc& PhxEngine::RHI::Dx12::GraphicsDevice::GetTextureDesc(TextureHandle handle)
{
	const Dx12Texture* texture = this->m_texturePool.Get(handle);
	return texture->Desc;
}

DescriptorIndex PhxEngine::RHI::Dx12::GraphicsDevice::GetDescriptorIndex(TextureHandle handle)
{
	const Dx12Texture* texture = this->m_texturePool.Get(handle);
	return texture 
		? texture->BindlessResourceIndex
		: cInvalidDescriptorIndex;
}

void PhxEngine::RHI::Dx12::GraphicsDevice::DeleteTexture(TextureHandle handle)
{
	if (!handle.IsValid())
	{
		return;
	}

	DeleteItem d =
	{
		this->m_frameCount,
		[=]()
		{
			Dx12Texture* texture = this->m_texturePool.Get(handle);

			if (texture)
			{
				this->m_bindlessResourceDescriptorTable->Free(texture->BindlessResourceIndex);
				texture->DisposeViews();
				this->GetTexturePool().Release(handle);
			}
		}
	};

	this->m_deleteQueue.push_back(d);
}

BufferHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateIndexBuffer(BufferDesc const& desc)
{
	Dx12Buffer bufferImpl = {};
	this->CreateBufferInternal(desc, bufferImpl);

	bufferImpl.IndexView = {};
	auto& view = bufferImpl.IndexView;
	view.BufferLocation = bufferImpl.D3D12Resource->GetGPUVirtualAddress();
	view.Format = bufferImpl.GetDesc().StrideInBytes == sizeof(uint32_t)
		? DXGI_FORMAT_R32_UINT
		: DXGI_FORMAT_R16_UINT;
	view.SizeInBytes = bufferImpl.GetDesc().SizeInBytes;

	if ((desc.Binding & BindingFlags::ShaderResource) == BindingFlags::ShaderResource)
	{
		this->CreateSRVViews(bufferImpl);
	}

	return this->m_bufferPool.Insert(bufferImpl);
}

BufferHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateVertexBuffer(BufferDesc const& desc)
{
	Dx12Buffer bufferImpl = {};
	this->CreateBufferInternal(desc, bufferImpl);

	bufferImpl.VertexView = {};
	auto& view = bufferImpl.VertexView;
	view.BufferLocation = bufferImpl.D3D12Resource->GetGPUVirtualAddress();
	view.StrideInBytes = bufferImpl.GetDesc().StrideInBytes;
	view.SizeInBytes = bufferImpl.GetDesc().SizeInBytes;

	if ((desc.Binding & BindingFlags::ShaderResource) == BindingFlags::ShaderResource)
	{
		this->CreateSRVViews(bufferImpl);
	}

	return this->m_bufferPool.Insert(bufferImpl);
}

BufferHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateBuffer(BufferDesc const& desc)
{
	Dx12Buffer bufferImpl = {};
	this->CreateBufferInternal(desc, bufferImpl);
	if ((desc.Binding & BindingFlags::ShaderResource) == BindingFlags::ShaderResource)
	{
		this->CreateSRVViews(bufferImpl);
	}

#if TRACK_RESOURCES
	if (sTrackedResources.find(desc.DebugName) == sTrackedResources.end())
	{
		sTrackedResources.insert(desc.DebugName);
	}
#endif
	return this->m_bufferPool.Insert(bufferImpl);
}

const BufferDesc& GraphicsDevice::GetBufferDesc(BufferHandle handle)
{
	return this->m_bufferPool.Get(handle)->Desc;
}

DescriptorIndex GraphicsDevice::GetDescriptorIndex(BufferHandle handle)
{
	const Dx12Buffer* bufferImpl = this->m_bufferPool.Get(handle);
	return bufferImpl
		? bufferImpl->BindlessResourceIndex
		: cInvalidDescriptorIndex;
}

void* GraphicsDevice::GetBufferMappedData(BufferHandle handle)
{
	const Dx12Buffer* bufferImpl = this->m_bufferPool.Get(handle);
	assert(bufferImpl->GetDesc().Usage != Usage::Default);
	return bufferImpl->MappedData;
}

uint32_t GraphicsDevice::GetBufferMappedDataSizeInBytes(BufferHandle handle)
{
	const Dx12Buffer* bufferImpl = this->m_bufferPool.Get(handle);
	assert(bufferImpl->GetDesc().Usage != Usage::Default);
	return bufferImpl->MappedSizeInBytes;
}

void GraphicsDevice::DeleteBuffer(BufferHandle handle)
{
if (!handle.IsValid())
	{
		return;
	}

	DeleteItem d =
	{
		this->m_frameCount,
		[=]()
		{
			Dx12Buffer* texture = this->m_bufferPool.Get(handle);

			if (texture)
			{
#if TRACK_RESOURCES
				if (sTrackedResources.find(texture->GetDesc().DebugName) != sTrackedResources.end())
				{
					sTrackedResources.erase(texture->GetDesc().DebugName);
				}
#endif
				this->m_bindlessResourceDescriptorTable->Free(texture->BindlessResourceIndex);
				texture->DisposeViews();
				this->GetBufferPool().Release(handle);
			}
		}
	};

	this->m_deleteQueue.push_back(d);
}

TimerQueryHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateTimerQuery()
{

	int queryIndex = this->m_timerQueryIndexPool.Allocate();
	if (queryIndex < 0)
	{
		return nullptr;
	}

	auto timerQuery = std::make_shared<TimerQuery>(this->m_timerQueryIndexPool);
	timerQuery->BeginQueryIndex = queryIndex * 2;
	timerQuery->EndQueryIndex = timerQuery->BeginQueryIndex + 1;
	timerQuery->Resolved = false;
	timerQuery->Time = {};

	return timerQuery;
}

bool PhxEngine::RHI::Dx12::GraphicsDevice::PollTimerQuery(TimerQueryHandle query)
{
	auto queryImpl = std::static_pointer_cast<TimerQuery>(query);

	if (!queryImpl->Started)
	{
		return false;
	}

	if (!queryImpl->CommandQueue)
	{
		return true;
	}

	if (queryImpl->CommandQueue->GetLastCompletedFence() >= queryImpl->FenceCount)
	{
		queryImpl->CommandQueue = nullptr;
		return true;
	}

	return false;
}

TimeStep PhxEngine::RHI::Dx12::GraphicsDevice::GetTimerQueryTime(TimerQueryHandle query)
{
	auto queryImpl = std::static_pointer_cast<TimerQuery>(query);

	if (queryImpl->Resolved)
	{
		return queryImpl->Time;
	}

	if (queryImpl->CommandQueue)
	{
		queryImpl->CommandQueue->WaitForFence(queryImpl->FenceCount);
		queryImpl->CommandQueue = nullptr;
	}

	// Could use the queue that it was executued on
	// The GPU timestamp counter frequency (in ticks/second)
	uint64_t frequency;
	this->GetQueue(CommandQueueType::Graphics)->GetD3D12CommandQueue()->GetTimestampFrequency(&frequency);

	D3D12_RANGE bufferReadRange = {
	queryImpl->BeginQueryIndex * sizeof(uint64_t),
	(queryImpl->BeginQueryIndex + 2) * sizeof(uint64_t) };

	const Dx12Buffer* timestampQueryBuffer = this->m_bufferPool.Get(this->m_timestampQueryBuffer);
	uint64_t* data;
	const HRESULT res = timestampQueryBuffer->D3D12Resource->Map(0, &bufferReadRange, (void**)&data);

	if (FAILED(res))
	{
		return TimeStep(0.0f);
	}

	queryImpl->Resolved = true;
	queryImpl->Time = TimeStep(float(double(data[queryImpl->EndQueryIndex] - data[queryImpl->BeginQueryIndex]) / double(frequency)));

	timestampQueryBuffer->D3D12Resource->Unmap(0, nullptr);

	return queryImpl->Time;
}

void PhxEngine::RHI::Dx12::GraphicsDevice::ResetTimerQuery(TimerQueryHandle query)
{
	auto queryImpl = std::static_pointer_cast<TimerQuery>(query);
	queryImpl->Started = false;
	queryImpl->Resolved = false;
	queryImpl->Time = 0.f;
	queryImpl->CommandQueue = nullptr;
}

void PhxEngine::RHI::Dx12::GraphicsDevice::Present()
{
	HRESULT hr = this->m_swapChain.DxgiSwapchain->Present(0, 0);

	// If the device was reset we must completely reinitialize the renderer.
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
#ifdef _DEBUG
		char buff[64] = {};
		sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
			static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? this->GetD3D12Device2()->GetDeviceRemovedReason() : hr));
		OutputDebugStringA(buff);
#endif

		// TODO: Handle device lost
		// HandleDeviceLost();
	}

	// Mark complition of the graphics Queue 
	this->GetGfxQueue()->GetD3D12CommandQueue()->Signal(this->m_frameFence.Get(), this->m_frameCount);

	// Begin the next frame - this affects the GetCurrentBackBufferIndex()
	this->m_frameCount++;

	const UINT64 completedFrame = this->m_frameFence->GetCompletedValue();

	// If the fence is max uint64, that might indicate that the device has been removed as fences get reset in this case
	assert(completedFrame != UINT64_MAX);

	// Since our frame count is 1 based rather then 0, increment the number of buffers by 1 so we don't have to wait on the first 3 frames
	// that are kicked off.
	if (this->m_frameCount >= (this->m_swapChain.GetNumBackBuffers() + 1) && completedFrame < this->m_frameCount)
	{
		// Wait on the frames last value?
		// NULL event handle will simply wait immediately:
		//	https://docs.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12fence-seteventoncompletion#remarks
		ThrowIfFailed(
			this->m_frameFence->SetEventOnCompletion(this->m_frameCount - this->m_swapChain.GetNumBackBuffers(), NULL));
	}

	while (!this->m_deleteQueue.empty())
	{
		DeleteItem& deleteItem = this->m_deleteQueue.front();
		if (deleteItem.Frame < completedFrame)
		{
			deleteItem.DeleteFn();
			this->m_deleteQueue.pop_front();
		}
		else
		{
			break;
		}
	}

	this->RunGarbageCollection();
}

ExecutionReceipt PhxEngine::RHI::Dx12::GraphicsDevice::ExecuteCommandLists(
	ICommandList* const* pCommandLists,
	size_t numCommandLists,
	bool waitForCompletion,
	CommandQueueType executionQueue)
{
	std::vector<ID3D12CommandList*> d3d12CommandLists(numCommandLists);
	for (size_t i = 0; i < numCommandLists; i++)
	{
		auto cmdImpl = static_cast<CommandList*>(pCommandLists[i]);
		d3d12CommandLists[i] = cmdImpl->GetD3D12CommandList();		
	}

	auto* queue = this->GetQueue(executionQueue);

	auto fenceValue = queue->ExecuteCommandLists(d3d12CommandLists);

	if (waitForCompletion)
	{
		queue->WaitForFence(fenceValue);

		for (size_t i = 0; i < numCommandLists; i++)
		{
			auto trackedResources = static_cast<CommandList*>(pCommandLists[i])->Executed(fenceValue);

			for (auto timerQuery : trackedResources->TimerQueries)
			{
				timerQuery->Started = true;
				timerQuery->Resolved = false;

				// No need to set command queue as it has already been executed.
			}
		}
	}
	else
	{
		for (size_t i = 0; i < numCommandLists; i++)
		{
			auto trackedResources = static_cast<CommandList*>(pCommandLists[i])->Executed(fenceValue);
			
			for (auto timerQuery : trackedResources->TimerQueries)
			{
				timerQuery->Started = true;
				timerQuery->Resolved = false;
				timerQuery->CommandQueue = queue;
				timerQuery->FenceCount = fenceValue;
			}

			InflightDataEntry entry = { fenceValue, trackedResources };
			this->m_inflightData[(int)executionQueue].emplace_back(entry);
		}
	}

	return { fenceValue, executionQueue };
}

void PhxEngine::RHI::Dx12::GraphicsDevice::BeginCapture(std::wstring const& filename)
{
#if ENABLE_PIX_CAPUTRE
	if (this->m_pixCaptureModule)
	{
		PIXCaptureParameters param = {};
		param.GpuCaptureParameters.FileName = filename.c_str();
		PIXBeginCapture(PIX_CAPTURE_GPU, &param);
	}
#endif
}

void PhxEngine::RHI::Dx12::GraphicsDevice::EndCapture()
{
#if ENABLE_PIX_CAPUTRE
	if (this->m_pixCaptureModule)
	{
		PIXEndCapture(false);
	}
#endif
}

bool PhxEngine::RHI::Dx12::GraphicsDevice::CheckCapability(DeviceCapability deviceCapability)
{
	return (this->m_capabilities & deviceCapability) == deviceCapability;
}

TextureHandle PhxEngine::RHI::Dx12::GraphicsDevice::CreateRenderTarget(
	TextureDesc const& desc,
	Microsoft::WRL::ComPtr<ID3D12Resource> d3d12TextureResource)
{
	// Construct Texture
	Dx12Texture texture = {};
	texture.Desc = desc;
	texture.D3D12Resource = d3d12TextureResource;
	texture.RtvAllocation = this->GetRtvCpuHeap()->Allocate(1);

	this->GetD3D12Device2()->CreateRenderTargetView(
		texture.D3D12Resource.Get(),
		nullptr,
		texture.RtvAllocation.GetCpuHandle());

	// Set debug name
	std::wstring debugName(texture.Desc.DebugName.begin(), texture.Desc.DebugName.end());
	texture.D3D12Resource->SetName(debugName.c_str());

	return this->m_texturePool.Insert(texture);
}

void PhxEngine::RHI::Dx12::GraphicsDevice::CreateShaderResourceView(Dx12Texture& texture)
{
	auto dxgiFormatMapping = GetDxgiFormatMapping(texture.Desc.Format);
	texture.SrvAllocation = this->GetResourceCpuHeap()->Allocate(1);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = dxgiFormatMapping.srvFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	uint32_t planeSlice = (srvDesc.Format == DXGI_FORMAT_X24_TYPELESS_G8_UINT) ? 1 : 0;

	if (texture.Desc.Dimension == TextureDimension::TextureCube)
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;  // Only 2D textures are supported (this was checked in the calling function).
		srvDesc.TextureCube.MipLevels = texture.Desc.MipLevels;
		srvDesc.TextureCube.MostDetailedMip = 0;
	}
	else
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;  // Only 2D textures are supported (this was checked in the calling function).
		srvDesc.Texture2D.MipLevels = texture.Desc.MipLevels;
	}
	switch (texture.Desc.Dimension)
	{
	case TextureDimension::Texture1D:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
		srvDesc.Texture1D.MostDetailedMip = 0; // Subresource data
		srvDesc.Texture1D.MipLevels = texture.Desc.MipLevels;// Subresource data
		break;
	case TextureDimension::Texture1DArray:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
		srvDesc.Texture1DArray.FirstArraySlice = 0;
		srvDesc.Texture1DArray.ArraySize = texture.Desc.ArraySize;// Subresource data
		srvDesc.Texture1DArray.MostDetailedMip = 0;// Subresource data
		srvDesc.Texture1DArray.MipLevels = texture.Desc.MipLevels;// Subresource data
		break;
	case TextureDimension::Texture2D:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;// Subresource data
		srvDesc.Texture2D.MipLevels = texture.Desc.MipLevels;// Subresource data
		srvDesc.Texture2D.PlaneSlice = planeSlice;
		break;
	case TextureDimension::Texture2DArray:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.FirstArraySlice = 0;// Subresource data
		srvDesc.Texture2DArray.ArraySize = texture.Desc.ArraySize;// Subresource data
		srvDesc.Texture2DArray.MostDetailedMip = 0;// Subresource data
		srvDesc.Texture2DArray.MipLevels = texture.Desc.MipLevels;// Subresource data
		srvDesc.Texture2DArray.PlaneSlice = planeSlice;
		break;
	case TextureDimension::TextureCube:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;// Subresource data
		srvDesc.TextureCube.MipLevels = texture.Desc.MipLevels;// Subresource data
		break;
	case TextureDimension::TextureCubeArray:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
		srvDesc.TextureCubeArray.First2DArrayFace = 0;// Subresource data
		srvDesc.TextureCubeArray.NumCubes = texture.Desc.ArraySize / 6;// Subresource data
		srvDesc.TextureCubeArray.MostDetailedMip = 0;// Subresource data
		srvDesc.TextureCubeArray.MipLevels = texture.Desc.MipLevels;// Subresource data
		break;
	case TextureDimension::Texture2DMS:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
		break;
	case TextureDimension::Texture2DMSArray:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
		srvDesc.Texture2DMSArray.FirstArraySlice = 0;// Subresource data
		srvDesc.Texture2DMSArray.ArraySize = texture.Desc.ArraySize;// Subresource data
		break;
	case TextureDimension::Texture3D:
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		srvDesc.Texture3D.MostDetailedMip = 0;// Subresource data
		srvDesc.Texture3D.MipLevels = texture.Desc.MipLevels;// Subresource data
		break;
	case TextureDimension::Unknown:
	default:
		throw std::runtime_error("Unsupported Enum");
		return;
	}

	this->GetD3D12Device2()->CreateShaderResourceView(
		texture.D3D12Resource.Get(),
		&srvDesc,
		texture.SrvAllocation.GetCpuHandle());

	if (texture.Desc.IsBindless)
	{
		// Copy Descriptor to Bindless since we are creating a texture as a shader resource view
		texture.BindlessResourceIndex = this->m_bindlessResourceDescriptorTable->Allocate();
		if (texture.BindlessResourceIndex != cInvalidDescriptorIndex)
		{
			this->GetD3D12Device2()->CopyDescriptorsSimple(
				1,
				this->m_bindlessResourceDescriptorTable->GetCpuHandle(texture.BindlessResourceIndex),
				texture.SrvAllocation.GetCpuHandle(),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
	}
}

void PhxEngine::RHI::Dx12::GraphicsDevice::CreateRenderTargetView(Dx12Texture& texture)
{
	auto dxgiFormatMapping = GetDxgiFormatMapping(texture.Desc.Format);

	texture.RtvAllocation = this->GetRtvCpuHeap()->Allocate(1);

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = dxgiFormatMapping.rtvFormat;

	switch (texture.Desc.Dimension)
	{
	case TextureDimension::Texture1D:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
		rtvDesc.Texture1D.MipSlice = 0;
		break;
	case TextureDimension::Texture1DArray:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
		rtvDesc.Texture1DArray.FirstArraySlice = 0;
		rtvDesc.Texture1DArray.ArraySize = texture.Desc.ArraySize;
		rtvDesc.Texture1DArray.MipSlice = 0;
		break;
	case TextureDimension::Texture2D:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		break;
	case TextureDimension::Texture2DArray:
	case TextureDimension::TextureCube:
	case TextureDimension::TextureCubeArray:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.ArraySize = texture.Desc.ArraySize;
		rtvDesc.Texture2DArray.FirstArraySlice = 0;
		rtvDesc.Texture2DArray.MipSlice = 0;
		break;
	case TextureDimension::Texture2DMS:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
		break;
	case TextureDimension::Texture2DMSArray:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
		rtvDesc.Texture2DMSArray.FirstArraySlice = 0;
		rtvDesc.Texture2DMSArray.ArraySize = texture.Desc.ArraySize;
		break;
	case TextureDimension::Texture3D:
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
		rtvDesc.Texture3D.FirstWSlice = 0;
		rtvDesc.Texture3D.WSize = texture.Desc.ArraySize;
		rtvDesc.Texture3D.MipSlice = 0;
		break;
	case TextureDimension::Unknown:
	default:
		throw std::runtime_error("Unsupported Enum");
		return;
	}

	this->GetD3D12Device2()->CreateRenderTargetView(
		texture.D3D12Resource.Get(),
		&rtvDesc,
		texture.RtvAllocation.GetCpuHandle());
}

void PhxEngine::RHI::Dx12::GraphicsDevice::CreateDepthStencilView(Dx12Texture& texture)
{
	auto dxgiFormatMapping = GetDxgiFormatMapping(texture.Desc.Format);

	texture.DsvAllocation = this->GetDsvCpuHeap()->Allocate(1);


	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = dxgiFormatMapping.rtvFormat;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	switch (texture.Desc.Dimension)
	{
	case TextureDimension::Texture1D:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
		dsvDesc.Texture1D.MipSlice = 0;
		break;
	case TextureDimension::Texture1DArray:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
		dsvDesc.Texture1DArray.FirstArraySlice = 0;
		dsvDesc.Texture1DArray.ArraySize = texture.Desc.ArraySize;
		dsvDesc.Texture1DArray.MipSlice = 0;
		break;
	case TextureDimension::Texture2D:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
		break;
	case TextureDimension::Texture2DArray:
	case TextureDimension::TextureCube:
	case TextureDimension::TextureCubeArray:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.ArraySize = texture.Desc.ArraySize;
		dsvDesc.Texture2DArray.FirstArraySlice = 0;
		dsvDesc.Texture2DArray.MipSlice = 0;
		break;
	case TextureDimension::Texture2DMS:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
		break;
	case TextureDimension::Texture2DMSArray:
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
		dsvDesc.Texture2DMSArray.FirstArraySlice = 0;
		dsvDesc.Texture2DMSArray.ArraySize = texture.Desc.ArraySize;
		break;
	case TextureDimension::Texture3D: 
	{
		return;
	}
	case TextureDimension::Unknown:
	default:
		throw std::runtime_error("Unsupported Enum");
		return;
	}

	this->GetD3D12Device2()->CreateDepthStencilView(
		texture.D3D12Resource.Get(),
		&dsvDesc,
		texture.DsvAllocation.GetCpuHandle());
}

void PhxEngine::RHI::Dx12::GraphicsDevice::CreateUnorderedAccessView(Dx12Texture& texture)
{
	auto dxgiFormatMapping = GetDxgiFormatMapping(texture.Desc.Format);
	texture.UavAllocation = this->GetResourceCpuHeap()->Allocate(1);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = dxgiFormatMapping.srvFormat;

	switch (texture.Desc.Dimension)
	{
	case TextureDimension::Texture1D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
		uavDesc.Texture1D.MipSlice = 0;
		break;
	case TextureDimension::Texture1DArray:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
		uavDesc.Texture1DArray.FirstArraySlice = 0;
		uavDesc.Texture1DArray.ArraySize = texture.Desc.ArraySize;
		uavDesc.Texture1DArray.MipSlice = 0;
		break;
	case TextureDimension::Texture2D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		break;
	case TextureDimension::Texture2DArray:
	case TextureDimension::TextureCube:
	case TextureDimension::TextureCubeArray:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.FirstArraySlice = 0;
		uavDesc.Texture2DArray.ArraySize = texture.Desc.ArraySize;
		uavDesc.Texture2DArray.MipSlice = 0;
		break;
	case TextureDimension::Texture3D:
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		uavDesc.Texture3D.FirstWSlice = 0;
		uavDesc.Texture3D.WSize = texture.Desc.Depth;
		uavDesc.Texture3D.MipSlice = 0;
		break;
	case TextureDimension::Texture2DMS:
	case TextureDimension::Texture2DMSArray:
	{
		throw std::runtime_error("Unsupported Dimention");
		return;
	}
	case TextureDimension::Unknown:
	default:
		throw std::runtime_error("Unsupported Enum");;
		return;
	}

	this->GetD3D12Device2()->CreateUnorderedAccessView(
		texture.D3D12Resource.Get(),
		nullptr,
		&uavDesc,
		texture.UavAllocation.GetCpuHandle());

	// Bindless for UAV?
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


// TODO: remove
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

std::unique_ptr<DXGIGpuAdapter>  PhxEngine::RHI::Dx12::GraphicsDevice::SelectOptimalGpuApdater()
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

	auto selectedAdataper = std::make_unique<DXGIGpuAdapter>();
	selectedAdataper->Name = NarrowString(desc.Description);
	selectedAdataper->DedicatedVideoMemory = desc.DedicatedVideoMemory;
	selectedAdataper->DedicatedSystemMemory = desc.DedicatedSystemMemory;
	selectedAdataper->SharedSystemMemory = desc.SharedSystemMemory;

	selectedAdataper->DxgiAdapter = selectedGpu;

	return std::move(selectedAdataper);
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

void GraphicsDevice::CreateBufferInternal(BufferDesc const& desc, Dx12Buffer& outBuffer)
{
	outBuffer.Desc = desc;

	D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
	if (desc.AllowUnorderedAccess)
	{
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}
	
	CD3DX12_HEAP_PROPERTIES heapProperties;
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;

	switch (desc.Usage)
	{
	case Usage::ReadBack:
		heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
		initialState = D3D12_RESOURCE_STATE_COPY_DEST;
		break;

	case Usage::Upload:
		heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
		break;

	case Usage::Default:
	default:
		heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		initialState = D3D12_RESOURCE_STATE_COMMON;
	}

	UINT64 alignedSize = 0;
	if ((desc.Binding & BindingFlags::ConstantBuffer) == BindingFlags::ConstantBuffer)
	{
		alignedSize = AlignTo(alignedSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	}

	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(desc.SizeInBytes, resourceFlags, alignedSize);

	// Create a committed resource for the GPU resource in a default heap.
	ThrowIfFailed(
		this->GetD3D12Device2()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			initialState,
			nullptr,
			IID_PPV_ARGS(&outBuffer.D3D12Resource)));

	switch (desc.Usage)
	{
	case Usage::ReadBack:
	{
		ThrowIfFailed(
			outBuffer.D3D12Resource->Map(0, nullptr, &outBuffer.MappedData));
		outBuffer.MappedSizeInBytes = static_cast<uint32_t>(desc.SizeInBytes);

		break;
	}

	case Usage::Upload:
	{
		D3D12_RANGE readRange = {};
		ThrowIfFailed(
			outBuffer.D3D12Resource->Map(0, &readRange, &outBuffer.MappedData));

		outBuffer.MappedSizeInBytes = static_cast<uint32_t>(desc.SizeInBytes);
		break;
	}
	case Usage::Default:
	default:
		heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		initialState = D3D12_RESOURCE_STATE_COMMON;
	}

	std::wstring debugName(desc.DebugName.begin(), desc.DebugName.end());
	outBuffer.D3D12Resource->SetName(debugName.c_str());
}

void PhxEngine::RHI::Dx12::GraphicsDevice::CreateSRVViews(Dx12Buffer& gpuBuffer)
{
	gpuBuffer.SrvAllocation = this->GetResourceCpuHeap()->Allocate(1);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;

	if (gpuBuffer.Desc.Format == FormatType::UNKNOWN)
	{
		if ((gpuBuffer.Desc.MiscFlags & BufferMiscFlags::Raw) != 0)
		{
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
			srvDesc.Buffer.NumElements = gpuBuffer.Desc.SizeInBytes / sizeof(uint32_t);
		}
		else if((gpuBuffer.Desc.MiscFlags & BufferMiscFlags::Structured) != 0)
		{
			// This is a Structured Buffer
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.Buffer.NumElements = gpuBuffer.Desc.SizeInBytes / gpuBuffer.Desc.StrideInBytes;
			srvDesc.Buffer.StructureByteStride = gpuBuffer.Desc.StrideInBytes;
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
		gpuBuffer.D3D12Resource.Get(),
		&srvDesc,
		gpuBuffer.SrvAllocation.GetCpuHandle());

	if (gpuBuffer.GetDesc().CreateBindless || (gpuBuffer.GetDesc().MiscFlags & RHI::BufferMiscFlags::Bindless) != 0)
	{
		// Copy Descriptor to Bindless since we are creating a texture as a shader resource view
		gpuBuffer.BindlessResourceIndex = this->m_bindlessResourceDescriptorTable->Allocate();
		if (gpuBuffer.BindlessResourceIndex != cInvalidDescriptorIndex)
		{
			this->GetD3D12Device2()->CopyDescriptorsSimple(
				1,
				this->m_bindlessResourceDescriptorTable->GetCpuHandle(gpuBuffer.BindlessResourceIndex),
				gpuBuffer.SrvAllocation.GetCpuHandle(),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
	}
}

void GraphicsDevice::CreateGpuTimestampQueryHeap(uint32_t queryCount)
{
	D3D12_QUERY_HEAP_DESC dx12Desc = {};
	dx12Desc.Count = queryCount;
	dx12Desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;

	ThrowIfFailed(
		this->m_device->CreateQueryHeap(
			&dx12Desc,
			IID_PPV_ARGS(&this->m_gpuTimestampQueryHeap)));
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

	D3D12_FEATURE_DATA_D3D12_OPTIONS featureOpptions = {};
	bool hasOptions = SUCCEEDED(this->m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOpptions, sizeof(featureOpptions)));

	if (hasOptions)
	{
		if (featureOpptions.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation)
		{
			this->m_capabilities |= DeviceCapability::RT_VT_ArrayIndex_Without_GS;
		}
	}

	// TODO: Move to acability array
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport5 = {};
	bool hasOptions5 = SUCCEEDED(this->m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport5, sizeof(featureSupport5)));

	if (SUCCEEDED(this->m_device.As(&this->m_device5)) && hasOptions5)
	{
		if (featureSupport5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
		{
			this->m_capabilities |= DeviceCapability::DXR;
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
	bool hasOptions6 = SUCCEEDED(this->m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureSupport6, sizeof(featureSupport6)));

	if (hasOptions6)
	{
		if (featureSupport6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2)
		{
			this->m_capabilities |= DeviceCapability::VariableRateShading;
		}
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS7 featureSupport7 = {};
	bool hasOptions7 = SUCCEEDED(this->m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &featureSupport7, sizeof(featureSupport7)));

	if (SUCCEEDED(this->m_device.As(&this->m_device2)) && hasOptions7)
	{
		if (featureSupport7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1)
		{
			this->m_capabilities |= DeviceCapability::MeshShading;
		}
		this->m_capabilities |= DeviceCapability::CreateNoteZeroed;
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
#ifdef _DEBUG
	if (sDebugEnabled)
	{
		Microsoft::WRL::ComPtr<IDXGIDebug1> dxgiDebug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
		{
			dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_ALL));
		}
	}
#endif
}