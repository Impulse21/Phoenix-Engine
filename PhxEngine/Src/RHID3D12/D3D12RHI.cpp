#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "D3D12RHI.h"
#include <PhxEngine/RHI/PhxRHI.h>
#include "D3D12Common.h"
#include "D3D12Adapter.h"
#include "D3D12Viewport.h"
#include "D3D12CommandList.h"
#include "D3D12Device.h"


using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::D3D12;

namespace
{
    static const GUID RenderdocUUID = { 0xa7aa6116, 0x9c8d, 0x4bba, { 0x90, 0x83, 0xb4, 0xd8, 0x16, 0xb7, 0x1b, 0x78 } };
    static const GUID PixUUID = { 0x9f251514, 0x9d4d, 0x4902, { 0x9d, 0x60, 0x18, 0x98, 0x8a, 0xb7, 0xd4, 0xb5 } };

    static bool sDebugEnabled = false;
    bool IsAdapterSupported(std::shared_ptr<D3D12Adapter>& adapter, PhxEngine::RHI::FeatureLevel level)
    {
        if (!adapter)
        {
            return false;
        }

        DXGI_ADAPTER_DESC adapterDesc;
        if (FAILED(adapter->GetDxgiAdapter()->GetDesc(&adapterDesc)))
        {
            return false;
        }

        // TODO: Check feature level from adapter. Requries device to be created.
        return true;
    }

    static bool SafeTestD3D12CreateDevice(IDXGIAdapter* adapter, D3D_FEATURE_LEVEL minFeatureLevel, D3D12DeviceBasicInfo& outInfo)
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
                LOG_CORE_WARN("D3D12CreateDevice failed.");
            }
        }
        catch (...)
        {
        }
#pragma warning(default:6322)

        return false;
    }

    DXGI_FORMAT ConvertFormat(RHIFormat format)
    {
        return D3D12::GetDxgiFormatMapping(format).SrvFormat;
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

    D3D12_BLEND ConvertBlendValue(RHIBlendFactor value)
    {
        switch (value)
        {
        case RHIBlendFactor::Zero:
            return D3D12_BLEND_ZERO;
        case RHIBlendFactor::One:
            return D3D12_BLEND_ONE;
        case RHIBlendFactor::SrcColor:
            return D3D12_BLEND_SRC_COLOR;
        case RHIBlendFactor::InvSrcColor:
            return D3D12_BLEND_INV_SRC_COLOR;
        case RHIBlendFactor::SrcAlpha:
            return D3D12_BLEND_SRC_ALPHA;
        case RHIBlendFactor::InvSrcAlpha:
            return D3D12_BLEND_INV_SRC_ALPHA;
        case RHIBlendFactor::DstAlpha:
            return D3D12_BLEND_DEST_ALPHA;
        case RHIBlendFactor::InvDstAlpha:
            return D3D12_BLEND_INV_DEST_ALPHA;
        case RHIBlendFactor::DstColor:
            return D3D12_BLEND_DEST_COLOR;
        case RHIBlendFactor::InvDstColor:
            return D3D12_BLEND_INV_DEST_COLOR;
        case RHIBlendFactor::SrcAlphaSaturate:
            return D3D12_BLEND_SRC_ALPHA_SAT;
        case RHIBlendFactor::ConstantColor:
            return D3D12_BLEND_BLEND_FACTOR;
        case RHIBlendFactor::InvConstantColor:
            return D3D12_BLEND_INV_BLEND_FACTOR;
        case RHIBlendFactor::Src1Color:
            return D3D12_BLEND_SRC1_COLOR;
        case RHIBlendFactor::InvSrc1Color:
            return D3D12_BLEND_INV_SRC1_COLOR;
        case RHIBlendFactor::Src1Alpha:
            return D3D12_BLEND_SRC1_ALPHA;
        case RHIBlendFactor::InvSrc1Alpha:
            return D3D12_BLEND_INV_SRC1_ALPHA;
        default:
            return D3D12_BLEND_ZERO;
        }
    }

    D3D12_BLEND_OP ConvertBlendOp(RHIBlendOp value)
    {
        switch (value)
        {
        case RHIBlendOp::Add:
            return D3D12_BLEND_OP_ADD;
        case RHIBlendOp::Subrtact:
            return D3D12_BLEND_OP_SUBTRACT;
        case RHIBlendOp::ReverseSubtract:
            return D3D12_BLEND_OP_REV_SUBTRACT;
        case RHIBlendOp::Min:
            return D3D12_BLEND_OP_MIN;
        case RHIBlendOp::Max:
            return D3D12_BLEND_OP_MAX;
        default:
            return D3D12_BLEND_OP_ADD;
        }
    }

    D3D12_STENCIL_OP ConvertStencilOp(RHIStencilOp value)
    {
        switch (value)
        {
        case RHIStencilOp::Keep:
            return D3D12_STENCIL_OP_KEEP;
        case RHIStencilOp::Zero:
            return D3D12_STENCIL_OP_ZERO;
        case RHIStencilOp::Replace:
            return D3D12_STENCIL_OP_REPLACE;
        case RHIStencilOp::IncrementAndClamp:
            return D3D12_STENCIL_OP_INCR_SAT;
        case RHIStencilOp::DecrementAndClamp:
            return D3D12_STENCIL_OP_DECR_SAT;
        case RHIStencilOp::Invert:
            return D3D12_STENCIL_OP_INVERT;
        case RHIStencilOp::IncrementAndWrap:
            return D3D12_STENCIL_OP_INCR;
        case RHIStencilOp::DecrementAndWrap:
            return D3D12_STENCIL_OP_DECR;
        default:
            return D3D12_STENCIL_OP_KEEP;
        }
    }

    D3D12_COMPARISON_FUNC ConvertComparisonFunc(RHIComparisonFunc value)
    {
        switch (value)
        {
        case RHIComparisonFunc::Never:
            return D3D12_COMPARISON_FUNC_NEVER;
        case RHIComparisonFunc::Less:
            return D3D12_COMPARISON_FUNC_LESS;
        case RHIComparisonFunc::Equal:
            return D3D12_COMPARISON_FUNC_EQUAL;
        case RHIComparisonFunc::LessOrEqual:
            return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case RHIComparisonFunc::Greater:
            return D3D12_COMPARISON_FUNC_GREATER;
        case RHIComparisonFunc::NotEqual:
            return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case RHIComparisonFunc::GreaterOrEqual:
            return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case RHIComparisonFunc::Always:
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
}

bool D3D12RHIFactory::IsSupported(FeatureLevel requestedFeatureLevel)
{
    /*
    if (!Platform::VerifyWindowsVersion(10, 0, 15063))
    {
        LOG_CORE_WARN("Current windows version doesn't support D3D12. Update to 1703 or newer to for D3D12 support.");
        return false;
    }
    */
    if (!this->m_choosenAdapter)
    {
        this->FindAdapter();
    }

    return this->m_choosenAdapter
        && IsAdapterSupported(this->m_choosenAdapter, requestedFeatureLevel);
}

std::unique_ptr<PhxEngine::RHI::IPhxRHI> PhxEngine::RHI::D3D12::D3D12RHIFactory::CreateRHI()
{
    if (!this->m_choosenAdapter)
    {
        if (!this->IsSupported())
        {
            LOG_CORE_ERROR("Unable to create D3D12 RHI On current platform.");
        }
    }
    
    auto rhi = std::make_unique<D3D12RHI>(this->m_choosenAdapter);
    // Decorate with Validation layer if required

    return rhi;
}

void PhxEngine::RHI::D3D12::D3D12RHIFactory::FindAdapter()
{
    LOG_CORE_INFO("Finding a suitable adapter");

    // Create factory
    RefCountPtr<IDXGIFactory6> factory = this->CreateDXGIFactory6();

    RefCountPtr<IDXGIAdapter1> selectedAdapter;
    D3D12DeviceBasicInfo selectedBasicDeviceInfo = {};
    size_t selectedGPUVideoMemeory = 0;
    RefCountPtr<IDXGIAdapter1> tempAdapter;
    for (uint32_t adapterIndex = 0; DXGIGpuAdapter::EnumAdapters(adapterIndex, factory.Get(), tempAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
    {
        if (!tempAdapter)
        {
            continue;
        }

        DXGI_ADAPTER_DESC1 desc = {};
        tempAdapter->GetDesc1(&desc);

        std::string name = NarrowString(desc.Description);
        size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
        size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
        size_t sharedSystemMemory = desc.SharedSystemMemory;

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            LOG_CORE_INFO("GPU '{0}' is a software adapter. Skipping consideration as this is not supported.", name.c_str());
            continue;
        }

        D3D12DeviceBasicInfo basicDeviceInfo = {};
        if (!SafeTestD3D12CreateDevice(tempAdapter, D3D_FEATURE_LEVEL_11_1, basicDeviceInfo))
        {
            continue;
        }

        if (basicDeviceInfo.NumDeviceNodes > 1)
        {
            LOG_CORE_INFO("GPU '{0}' has one or more device nodes. Currently only support one device ndoe.", name.c_str());
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
        LOG_CORE_WARN("No suitable adapters were found.");
        return;
    }

    DXGI_ADAPTER_DESC desc = {};
    selectedAdapter->GetDesc(&desc);

    std::string name = NarrowString(desc.Description);
    size_t dedicatedVideoMemory = desc.DedicatedVideoMemory;
    size_t dedicatedSystemMemory = desc.DedicatedSystemMemory;
    size_t sharedSystemMemory = desc.SharedSystemMemory;

    LOG_CORE_INFO(
        "Found Suitable D3D12 Adapter {0}",
        name.c_str());

    LOG_CORE_INFO(
        "Adapter has {0}MB of dedicated video memory, {1}MB of dedicated system memory, and {2}MB of shared system memory.",
        dedicatedVideoMemory / (1024 * 1024),
        dedicatedSystemMemory / (1024 * 1024),
        sharedSystemMemory / (1024 * 1024));

    D3D12AdapterDesc adapterDesc =
    {
        .Name = NarrowString(desc.Description),
        .DeviceBasicInfo = selectedBasicDeviceInfo,
        .DXGIDesc = desc,
    };

    this->m_choosenAdapter = std::make_shared<D3D12Adapter>(adapterDesc, selectedAdapter);
}

RefCountPtr<IDXGIFactory6> PhxEngine::RHI::D3D12::D3D12RHIFactory::CreateDXGIFactory6() const
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

    RefCountPtr<IDXGIFactory6> factory;
    ThrowIfFailed(
        CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory)));

    return factory;
}

D3D12RHI* D3D12RHI::Singleton = nullptr;

D3D12RHI::D3D12RHI(std::shared_ptr<D3D12Adapter>& adapter)
    : m_adapter(adapter)
{
    assert(!Singleton);

    Singleton = this;

    // TODO: lite set up that is required.
}

PhxEngine::RHI::D3D12::D3D12RHI::~D3D12RHI()
{
    Singleton = nullptr;
}

void PhxEngine::RHI::D3D12::D3D12RHI::Initialize()
{
    this->m_adapter->InitializeD3D12Devices();
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
}

void PhxEngine::RHI::D3D12::D3D12RHI::Finalize()
{
}
RHIShaderHandle D3D12RHI::CreateShader(RHIShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode)
{
    auto shaderImpl = std::make_unique<D3D12Shader>(desc, shaderByteCode);

    RefCountPtr<ID3D12RootSignature> rootSig;
    auto hr =
        this->m_adapter->GetRootDevice2()->CreateRootSignature(
            0,
            shaderImpl->GetByteCode().data(),
            shaderImpl->GetByteCode().size(),
            IID_PPV_ARGS(&rootSig));

    shaderImpl->SetRootSignature(rootSig);

    return RHI::RHIShaderHandle::Create(shaderImpl.release());
}

RHIInputLayoutHandle PhxEngine::RHI::D3D12::D3D12RHI::CreateInputLayout(Span<RHIVertexAttributeDesc> desc)
{
	auto inputLayoutImpl = std::make_unique<D3D12InputLayout>(desc);

	for (uint32_t index = 0; index < inputLayoutImpl->GetNumAttributes(); index++)
	{
		RHIVertexAttributeDesc& attr = inputLayoutImpl->Attributes[index];

		// Copy the description to get a stable name pointer in desc
		attr = desc[index];

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

	return RHIInputLayoutHandle::Create(inputLayoutImpl.release());
}

RHIGraphicsPipelineHandle PhxEngine::RHI::D3D12::D3D12RHI::CreateGraphicsPipeline(RHIGraphicsPipelineDesc const& desc)
{
    std::unique_ptr<D3D12GraphicsPipeline> psoImpl = std::make_unique<D3D12GraphicsPipeline>(desc);

    if (desc.VertexShader)
    {
        auto shaderImpl = SafeCast<D3D12Shader*>(desc.VertexShader.Get());
        if (psoImpl->D3D12RootSignature == nullptr)
        {
            psoImpl->D3D12RootSignature = shaderImpl->GetRootSignature();
        }
    }

    if (desc.PixelShader)
    {
        auto shaderImpl = SafeCast<D3D12Shader*>(desc.PixelShader.Get());
        if (psoImpl->D3D12RootSignature == nullptr)
        {
            psoImpl->D3D12RootSignature = shaderImpl->GetRootSignature();
        }
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12Desc = {};
    d3d12Desc.pRootSignature = psoImpl->D3D12RootSignature.Get();

    D3D12Shader* shaderImpl = nullptr;
    shaderImpl = SafeCast<D3D12Shader*>(desc.VertexShader.Get());
    if (shaderImpl)
    {
        d3d12Desc.VS = { shaderImpl->GetByteCode().data(), shaderImpl->GetByteCode().size() };
    }

    shaderImpl = SafeCast<D3D12Shader*>(desc.HullShader.Get());
    if (shaderImpl)
    {
        d3d12Desc.HS = { shaderImpl->GetByteCode().data(), shaderImpl->GetByteCode().size() };
    }

    shaderImpl = SafeCast<D3D12Shader*>(desc.DomainShader.Get());
    if (shaderImpl)
    {
        d3d12Desc.DS = { shaderImpl->GetByteCode().data(), shaderImpl->GetByteCode().size() };
    }

    shaderImpl = SafeCast<D3D12Shader*>(desc.GeometryShader.Get());
    if (shaderImpl)
    {
        d3d12Desc.GS = { shaderImpl->GetByteCode().data(), shaderImpl->GetByteCode().size() };
    }

    shaderImpl = SafeCast<D3D12Shader*>(desc.PixelShader.Get());
    if (shaderImpl)
    {
        d3d12Desc.PS = { shaderImpl->GetByteCode().data(), shaderImpl->GetByteCode().size() };
    }

    this->TranslateBlendState(desc.BlendRenderState, d3d12Desc.BlendState);
    this->TranslateDepthStencilState(desc.DepthStencilRenderState, d3d12Desc.DepthStencilState);
    this->TranslateRasterState(desc.RasterRenderState, d3d12Desc.RasterizerState);

    switch (desc.PrimType)
    {
    case RHIPrimitiveType::PointList:
        d3d12Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        break;
    case RHIPrimitiveType::LineList:
        d3d12Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        break;
    case RHIPrimitiveType::TriangleList:
    case RHIPrimitiveType::TriangleStrip:
        d3d12Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        break;
    case RHIPrimitiveType::PatchList:
        d3d12Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        break;
    }

    if (desc.DepthStencilTexture != nullptr)
    {
        d3d12Desc.DSVFormat = GetDxgiFormatMapping(desc.DepthStencilTexture->GetDesc().Format).RtvFormat;
    }

    d3d12Desc.SampleDesc.Count = desc.SampleCount;
    d3d12Desc.SampleDesc.Quality = desc.SampleQuality;

    d3d12Desc.NumRenderTargets = (uint32_t)desc.RenderTargets.size();
    for (size_t i = 0; i < desc.RenderTargets.size(); i++)
    {
        d3d12Desc.RTVFormats[i] = GetDxgiFormatMapping(desc.RenderTargets[i]->GetDesc().Format).RtvFormat;
    }

    auto inputLayout = SafeCast<D3D12InputLayout*>(desc.InputLayout.Get());
    if (inputLayout && !inputLayout->InputElements.empty())
    {
        d3d12Desc.InputLayout.NumElements = uint32_t(inputLayout->InputElements.size());
        d3d12Desc.InputLayout.pInputElementDescs = &(inputLayout->InputElements[0]);
    }

    d3d12Desc.SampleMask = ~0u;

    ThrowIfFailed(
        this->m_adapter->GetRootDevice2()->CreateGraphicsPipelineState(
            &d3d12Desc, IID_PPV_ARGS(&psoImpl->D3D12PipelineState)));

    return psoImpl;
}

RHITextureHandle PhxEngine::RHI::D3D12::D3D12RHI::CreateTexture(RHITextureDesc const& desc)
{
    return RHITextureHandle();
}

RHITextureHandle PhxEngine::RHI::D3D12::D3D12RHI::CreateTexture(RHITextureDesc const& desc, RefCountPtr<D3D12Resource> resource)
{
    return RHITextureHandle();
}

RHIRenderPassHandle PhxEngine::RHI::D3D12::D3D12RHI::CreateRenderPass(RHIRenderPassDesc const& desc)
{
    if (!this->m_adapter->CheckCapability(DeviceCapability::RenderPass))
    {
        return {};
    }

    auto renderPassImpl = std::make_unique<D3D12RenderPass>();

    if (desc.Flags & RenderPassDesc::Flags::AllowUavWrites)
    {
        renderPassImpl->D12RenderFlags |= D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES;
    }

    for (auto& attachment : desc.Attachments)
    {
        auto textureImpl = SafeCast<D3D12Texture*>(attachment.Texture.Get());
        assert(textureImpl);
        if (!textureImpl)
        {
            continue;
        }

        auto dxgiFormatMapping = GetDxgiFormatMapping(textureImpl->Desc.Format);
        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Color[0] = textureImpl->Desc.OptmizedClearValue.Colour.R;
        clearValue.Color[1] = textureImpl->Desc.OptmizedClearValue.Colour.G;
        clearValue.Color[2] = textureImpl->Desc.OptmizedClearValue.Colour.B;
        clearValue.Color[3] = textureImpl->Desc.OptmizedClearValue.Colour.A;
        clearValue.DepthStencil.Depth = textureImpl->Desc.OptmizedClearValue.DepthStencil.Depth;
        clearValue.DepthStencil.Stencil = textureImpl->Desc.OptmizedClearValue.DepthStencil.Stencil;
        clearValue.Format = dxgiFormatMapping.RtvFormat;


        switch (attachment.Type)
        {
        case RHIRenderPassDesc::RenderPassAttachment::Type::RenderTarget:
        {
            D3D12_RENDER_PASS_RENDER_TARGET_DESC& renderPassRTDesc = renderPassImpl->RTVs[renderPassImpl->NumRenderTargets];

            // Use main view
            if (attachment.Subresource < 0 || textureImpl->RtvSubresourcesAlloc.empty())
            {
                renderPassRTDesc.cpuDescriptor = textureImpl->RtvAllocation.Allocation.GetCpuHandle();
            }
            else // Use Subresource
            {
                assert((size_t)attachment.Subresource < textureImpl->RtvSubresourcesAlloc.size());
                renderPassRTDesc.cpuDescriptor = textureImpl->RtvSubresourcesAlloc[(size_t)attachment.Subresource].Allocation.GetCpuHandle();
            }

            switch (attachment.LoadOp)
            {
            case RHIRenderPassDesc::RenderPassAttachment::LoadOpType::Clear:
            {
                renderPassRTDesc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
                renderPassRTDesc.BeginningAccess.Clear.ClearValue = clearValue;
                break;
            }
            case RHIRenderPassDesc::RenderPassAttachment::LoadOpType::DontCare:
            {
                renderPassRTDesc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
                break;
            }
            case RHIRenderPassDesc::RenderPassAttachment::LoadOpType::Load:
            default:
                renderPassRTDesc.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
            }

            switch (attachment.StoreOp)
            {
            case RHIRenderPassDesc::RenderPassAttachment::StoreOpType::DontCare:
            {
                renderPassRTDesc.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
                break;
            }
            case RHIRenderPassDesc::RenderPassAttachment::StoreOpType::Store:
            default:
                renderPassRTDesc.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
                break;
            }

            renderPassImpl->NumRenderTargets++;
            break;
        }
        case RHIRenderPassDesc::RenderPassAttachment::Type::DepthStencil:
        {
            D3D12_RENDER_PASS_DEPTH_STENCIL_DESC& renderPassDSDesc = renderPassImpl->DSV;

            // Use main view
            if (attachment.Subresource < 0 || textureImpl->DsvSubresourcesAlloc.empty())
            {
                renderPassDSDesc.cpuDescriptor = textureImpl->DsvAllocation.Allocation.GetCpuHandle();
            }
            else // Use Subresource
            {
                assert((size_t)attachment.Subresource < textureImpl->DsvSubresourcesAlloc.size());
                renderPassDSDesc.cpuDescriptor = textureImpl->DsvSubresourcesAlloc[(size_t)attachment.Subresource].Allocation.GetCpuHandle();
            }

            switch (attachment.LoadOp)
            {
            case RHIRenderPassDesc::RenderPassAttachment::LoadOpType::Clear:
            {
                renderPassDSDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
                renderPassDSDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
                renderPassDSDesc.DepthBeginningAccess.Clear.ClearValue = clearValue;
                renderPassDSDesc.StencilBeginningAccess.Clear.ClearValue = clearValue;
                break;
            }
            case RHIRenderPassDesc::RenderPassAttachment::LoadOpType::DontCare:
            {
                renderPassDSDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
                renderPassDSDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
                break;
            }
            case RHIRenderPassDesc::RenderPassAttachment::LoadOpType::Load:
            default:
                renderPassDSDesc.DepthBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
                renderPassDSDesc.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
            }

            switch (attachment.StoreOp)
            {
            case RHIRenderPassDesc::RenderPassAttachment::StoreOpType::DontCare:
            {
                renderPassDSDesc.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
                renderPassDSDesc.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
                break;
            }
            case RHIRenderPassDesc::RenderPassAttachment::StoreOpType::Store:
            default:
                renderPassDSDesc.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
                renderPassDSDesc.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
                break;
            }

            break;
        }
        }
    }

    // Construct Begin Resource Barriers
    for (auto& attachment : desc.Attachments)
    {
        D3D12_RESOURCE_STATES beforeState = ConvertResourceStates(attachment.InitialLayout);
        D3D12_RESOURCE_STATES afterState = ConvertResourceStates(attachment.SubpassLayout);

        if (beforeState == afterState)
        {
            // Nothing to do here;
            continue;
        }

        auto textureImpl = SafeCast<D3D12Texture*>(attachment.Texture);

        D3D12_RESOURCE_BARRIER barrierdesc = {};
        barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrierdesc.Transition.pResource = textureImpl->D3D12Resource.Get();
        barrierdesc.Transition.StateBefore = beforeState;
        barrierdesc.Transition.StateAfter = afterState;

        // Unroll subresouce Barriers
        if (attachment.Subresource >= 0)
        {
            const DescriptorView* descriptor = nullptr;
            switch (attachment.Type)
            {
            case RHIRenderPassDesc::RenderPassAttachment::Type::RenderTarget:
            {
                descriptor = &textureImpl->RtvSubresourcesAlloc[attachment.Subresource];
                break;
            }
            case RHIRenderPassDesc::RenderPassAttachment::Type::DepthStencil:
            {
                descriptor = &textureImpl->DsvSubresourcesAlloc[attachment.Subresource];
                break;
            }
            default:
                assert(false);
                continue;
            }

            for (uint32_t mip = descriptor->FirstMip; mip < std::min((uint32_t)textureImpl->Desc.MipLevels, descriptor->FirstMip + descriptor->MipCount); ++mip)
            {
                for (uint32_t slice = descriptor->FirstSlice; slice < std::min((uint32_t)textureImpl->Desc.ArraySize, descriptor->FirstSlice + descriptor->SliceCount); ++slice)
                {
                    barrierdesc.Transition.Subresource = D3D12CalcSubresource(mip, slice, 0, textureImpl->Desc.MipLevels, textureImpl->Desc.ArraySize);
                    renderPassImpl->BarrierDescBegin.push_back(barrierdesc);
                }
            }
        }
        else
        {
            barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            renderPassImpl->BarrierDescBegin.push_back(barrierdesc);
        }
    }

    // Construct End Resource Barriers
    for (auto& attachment : desc.Attachments)
    {
        D3D12_RESOURCE_STATES beforeState = ConvertResourceStates(attachment.SubpassLayout);
        D3D12_RESOURCE_STATES afterState = ConvertResourceStates(attachment.FinalLayout);

        if (beforeState == afterState)
        {
            // Nothing to do here;
            continue;
        }

        auto textureImpl = SafeCast<D3D12Texture*>(attachment.Texture);

        D3D12_RESOURCE_BARRIER barrierdesc = {};
        barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrierdesc.Transition.pResource = textureImpl->D3D12Resource.Get();
        barrierdesc.Transition.StateBefore = beforeState;
        barrierdesc.Transition.StateAfter = afterState;

        // Unroll subresouce Barriers
        if (attachment.Subresource >= 0)
        {
            const DescriptorView* descriptor = nullptr;
            switch (attachment.Type)
            {
            case RHIRenderPassDesc::RenderPassAttachment::Type::RenderTarget:
            {
                descriptor = &textureImpl->RtvSubresourcesAlloc[attachment.Subresource];
                break;
            }
            case RHIRenderPassDesc::RenderPassAttachment::Type::DepthStencil:
            {
                descriptor = &textureImpl->DsvSubresourcesAlloc[attachment.Subresource];
                break;
            }
            default:
                assert(false);
                continue;
            }

            for (uint32_t mip = descriptor->FirstMip; mip < std::min((uint32_t)textureImpl->Desc.MipLevels, descriptor->FirstMip + descriptor->MipCount); ++mip)
            {
                for (uint32_t slice = descriptor->FirstSlice; slice < std::min((uint32_t)textureImpl->Desc.ArraySize, descriptor->FirstSlice + descriptor->SliceCount); ++slice)
                {
                    barrierdesc.Transition.Subresource = D3D12CalcSubresource(mip, slice, 0, textureImpl->Desc.MipLevels, textureImpl->Desc.ArraySize);
                    renderPassImpl->BarrierDescBegin.push_back(barrierdesc);
                }
            }
        }
        else
        {
            barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            renderPassImpl->BarrierDescEnd.push_back(barrierdesc);
        }
    }

    return RHIRenderPassHandle::Create(renderPassImpl.release());
}

IRHIFrameRenderCtx& PhxEngine::RHI::D3D12::D3D12RHI::BeginFrameRenderContext(RHIViewportHandle viewport)
{
    auto* viewportImpl = SafeCast<D3D12Viewport*>(viewport.Get());

    // Wait on current frame's fence before providing
    viewportImpl->WaitForIdleFrame();

    // Construct a frame render contest
    // Avoid Allocations here, but how.
    // TODO: I am here.

    // Start Frame;
    return this->m_frameContext;
}

void PhxEngine::RHI::D3D12::D3D12RHI::FinishAndPresetFrameRenderContext(IRHIFrameRenderCtx* context)
{
    this->m_frameContext.GetViewport()->Present();
}

void PhxEngine::RHI::D3D12::D3D12RHI::TranslateBlendState(RHIBlendRenderState const& inState, D3D12_BLEND_DESC& outState)
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

void D3D12RHI::TranslateDepthStencilState(RHIDepthStencilRenderState const& inState, D3D12_DEPTH_STENCIL_DESC& outState)
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

void PhxEngine::RHI::D3D12::D3D12RHI::TranslateRasterState(RHIRasterRenderState const& inState, D3D12_RASTERIZER_DESC& outState)
{
    switch (inState.FillMode)
    {
    case RHIRasterFillMode::Solid:
        outState.FillMode = D3D12_FILL_MODE_SOLID;
        break;
    case RHIRasterFillMode::Wireframe:
        outState.FillMode = D3D12_FILL_MODE_WIREFRAME;
        break;
    default:
        break;
    }

    switch (inState.CullMode)
    {
    case RHIRasterCullMode::Back:
        outState.CullMode = D3D12_CULL_MODE_BACK;
        break;
    case RHIRasterCullMode::Front:
        outState.CullMode = D3D12_CULL_MODE_FRONT;
        break;
    case RHIRasterCullMode::None:
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

IRHICommandList* D3D12FrameRenderCtx::BeginCommandRecording(RHICommandQueueType queueType)
{
    ID3D12CommandAllocator* allocator = this->m_device->RequestAllocator(queueType);

    auto _ = std::scoped_lock(this->m_commandMutex);
    D3D12CommandList* commandList = nullptr;
    if (!this->m_availableCommandLists.empty())
    {
        commandList = this->m_availableCommandLists.front();
        this->m_availableCommandLists.pop();
        commandList->Reset(allocator);
    }
    else
    {
        // Create a queue
        // this->m_commandLists.push_back();
    }


    /*
    ID3D12CommandAllocator* pAllocator = nullptr;
    if (!this->m_availableAllocators.empty())
    {
        auto& allocatorPair = this->m_availableAllocators.front();
        if (allocatorPair.first <= completedFenceValue)
        {
            pAllocator = allocatorPair.second;
            ThrowIfFailed(
                pAllocator->Reset());

            this->m_availableAllocators.pop();
        }
    }

    if (!pAllocator)
    {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> newAllocator;
        ThrowIfFailed(
            this->m_rootDevice->CreateCommandAllocator(
                this->m_type,
                IID_PPV_ARGS(&newAllocator)));

        // TODO: std::shared_ptr is leaky
        wchar_t allocatorName[32];
        swprintf(allocatorName, 32, L"CommandAllocator %zu", this->Size());
        newAllocator->SetName(allocatorName);
        this->m_allocatorPool.emplace_back(newAllocator);
        pAllocator = this->m_allocatorPool.back().Get();
    }
    return pAllocator;

    */
    return nullptr;
}

uint64_t PhxEngine::RHI::D3D12::D3D12FrameRenderCtx::SubmitCommands(Core::Span<IRHICommandList*> commands)
{
    return 0;
}
