#pragma once

#include <vector>
#include <optional>
#include <PhxEngine/RHI/PhxRHIDefinitions.h>
#include <PhxEngine/Core/RefCountPtr.h>

namespace PhxEngine::RHI
{
    struct SwapChainDesc
    {
        uint32_t Width = 0;
        uint32_t Height = 0;
        uint32_t BufferCount = 3;
        RHI::Format Format = RHI::Format::R10G10B10A2_UNORM;
        bool Fullscreen = false;
        bool VSync = false;
        bool EnableHDR = false;
        RHI::ClearValue OptmizedClearValue =
        {
            .Colour =
            {
                0.0f,
                0.0f,
                0.0f,
                1.0f,
            }
        };
    };

    class RHIResource
    {
    protected:
        RHIResource() = default;
        virtual ~RHIResource() = default;

    public:
        virtual unsigned long AddRef() = 0;
        virtual unsigned long Release() = 0;

        // Non-copyable and non-movable
        RHIResource(const RHIResource&) = delete;
        RHIResource(const RHIResource&&) = delete;
        RHIResource& operator=(const RHIResource&) = delete;
        RHIResource& operator=(const RHIResource&&) = delete;
    };

    struct ShaderDesc
    {
        ShaderStage Stage = ShaderStage::None;
        std::string DebugName = "";
    };

    class Shader : public RHIResource
    {
    public:
        Shader() = default;
        const ShaderDesc& Desc() const { return this->m_desc; }

    protected:
        ShaderDesc m_desc;
    };
    using ShaderRef = Core::RefCountPtr<Shader>;

    struct VertexAttributeDesc
    {
        static const uint32_t SAppendAlignedElement = 0xffffffff; // automatically figure out AlignedByteOffset depending on Format

        std::string SemanticName;
        uint32_t SemanticIndex = 0;
        RHI::Format Format = RHI::Format::UNKNOWN;
        uint32_t InputSlot = 0;
        uint32_t AlignedByteOffset = SAppendAlignedElement;
        bool IsInstanced = false;
    };
    
    class InputLayout : public RHIResource
    {
    public:
        InputLayout() = default;
        const Core::Span<VertexAttributeDesc>& Desc() const { return this->m_desc; }

    protected:
        std::vector<VertexAttributeDesc> m_desc;
    };

    using InputLayoutRef = Core::RefCountPtr<InputLayout>;

    struct TextureDesc
    {
        BindingFlags BindingFlags = BindingFlags::ShaderResource;
        TextureDimension Dimension = TextureDimension::Texture2D;
        ResourceStates InitialState = ResourceStates::Common;

        RHI::Format Format = RHI::Format::UNKNOWN;
        bool IsTypeless = false;
        bool IsBindless = true;

        uint32_t Width;
        uint32_t Height;

        union
        {
            uint16_t ArraySize = 1;
            uint16_t Depth;
        };

        uint16_t MipLevels = 1;

        RHI::ClearValue OptmizedClearValue = {};
        std::string DebugName;
    };

    class Texture : public RHIResource
    {
    public:
        Texture() = default;
        const TextureDesc& Desc() const { return this->m_desc; }

    protected:
        TextureDesc m_desc;
    };
    using TextureRef = Core::RefCountPtr<Texture>;

    struct BufferDesc
    {
        BufferMiscFlags MiscFlags = BufferMiscFlags::None;
        // TODO: Change to usage
        Usage Usage = Usage::Default;

        BindingFlags Binding = BindingFlags::None;
        ResourceStates InitialState = ResourceStates::Common;

        // Stride is required for structured buffers
        uint64_t StrideInBytes = 0;
        uint64_t SizeInBytes = 0;

        RHI::Format Format = RHI::Format::UNKNOWN;

        bool AllowUnorderedAccess = false;
        // TODO: Remove
        bool CreateBindless = false;

        size_t UavCounterOffsetInBytes = 0;
        // BufferRef UavCounterBuffer = {};

        // BufferRef AliasedBuffer = {};

        std::string DebugName;
    };

    class Buffer : public RHIResource
    {
    public:
        Buffer() = default;
        const BufferDesc& Desc() const { return this->m_desc; }

    protected:
        BufferDesc m_desc;
    };
    using BufferRef = Core::RefCountPtr<Buffer>;

    // This is just a workaround for now
    class IRootSignatureBuilder
    {
    public:
        virtual ~IRootSignatureBuilder() = default;
    };

    struct GfxPipelineDesc
    {
        PrimitiveType PrimType = PrimitiveType::TriangleList;
        InputLayoutRef InputLayout;

        IRootSignatureBuilder* RootSignatureBuilder = nullptr;

        ShaderRef VertexShader;
        ShaderRef HullShader;
        ShaderRef DomainShader;
        ShaderRef GeometryShader;
        ShaderRef PixelShader;

        BlendRenderState BlendRenderState = {};
        DepthStencilRenderState DepthStencilRenderState = {};
        RasterRenderState RasterRenderState = {};

        std::vector<RHI::Format> RtvFormats;
        std::optional<RHI::Format> DsvFormat;

        uint32_t SampleCount = 1;
        uint32_t SampleQuality = 0;
    };

    class GfxPipeline : public RHIResource
    {
    public:
        GfxPipeline() = default;
        const GfxPipelineDesc& Desc() const { return this->m_desc; }

    protected:
        GfxPipelineDesc m_desc;
    };
    using GfxPipelineRef = Core::RefCountPtr<GfxPipeline>;

    struct ComputePipelineDesc
    {
        ShaderRef ComputeShader;
    };

    class ComputePipeline : public RHIResource
    {
    public:
        ComputePipeline() = default;
        const ComputePipelineDesc& Desc() const { return this->m_desc; }

    protected:
        ComputePipelineDesc m_desc;
    };
    using ComputePipelineRef = Core::RefCountPtr<ComputePipeline>;

    struct MeshPipelineDesc
    {
        PrimitiveType PrimType = PrimitiveType::TriangleList;

        ShaderRef AmpShader;
        ShaderRef MeshShader;
        ShaderRef PixelShader;

        BlendRenderState BlendRenderState = {};
        DepthStencilRenderState DepthStencilRenderState = {};
        RasterRenderState RasterRenderState = {};

        std::vector<RHI::Format> RtvFormats;
        std::optional<RHI::Format> DsvFormat;

        uint32_t SampleCount = 1;
        uint32_t SampleQuality = 0;

    };

    class MeshPipeline : public RHIResource
    {
    public:
        MeshPipeline() = default;
        const MeshPipelineDesc& Desc() const { return this->m_desc; }

    protected:
        MeshPipelineDesc m_desc;
    };
    using MeshPipelineRef = Core::RefCountPtr<MeshPipeline>;

    class SwapChain : public RHIResource
    {
    public:
        SwapChain() = default;
        const SwapChainDesc& Desc() const { return this->m_desc; }

    protected:
        SwapChainDesc m_desc;
    };
    using SwapChainRef = Core::RefCountPtr<SwapChain>;

    class CommandList : public RHIResource
    {
    public:
        CommandList() = default;

    };
    using CommandListRef = Core::RefCountPtr<CommandList>;
}