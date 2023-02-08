#pragma once

#include "PhxEngine/Core/Platform.h"
#include "PhxEngine/Core/TimeStep.h"
#include "PhxEngine/Core/Span.h"
#include "PhxEngine/Core/Handle.h"

#include <stdint.h>
#include <optional>
#include <string>
#include <vector>
#include <memory>
#include <variant>

#define PHXRHI_ENUM_CLASS_FLAG_OPERATORS(T) \
    inline T operator | (T a, T b) { return T(uint32_t(a) | uint32_t(b)); } \
    inline T& operator |=(T& a, T b) { return a = a | b; }\
    inline T operator & (T a, T b) { return T(uint32_t(a) & uint32_t(b)); } /* NOLINT(bugprone-macro-parentheses) */ \
    inline T operator ~ (T a) { return T(~uint32_t(a)); } /* NOLINT(bugprone-macro-parentheses) */ \
    inline bool operator !(T a) { return uint32_t(a) == 0; } \
    inline bool operator ==(T a, uint32_t b) { return uint32_t(a) == b; } \
    inline bool operator !=(T a, uint32_t b) { return uint32_t(a) != b; }

namespace PhxEngine::RHI
{
    typedef uint32_t DescriptorIndex;

    static constexpr DescriptorIndex cInvalidDescriptorIndex = ~0u;

    static constexpr uint32_t cMaxRenderTargets = 8;
    static constexpr uint32_t cMaxViewports = 16;
    static constexpr uint32_t cMaxVertexAttributes = 16;
    static constexpr uint32_t cMaxBindingLayouts = 5;
    static constexpr uint32_t cMaxBindingsPerLayout = 128;
    static constexpr uint32_t cMaxVolatileConstantBuffersPerLayout = 6;
    static constexpr uint32_t cMaxVolatileConstantBuffers = 32;
    static constexpr uint32_t cMaxPushConstantSize = 128;      // D3D12: root signature is 256 bytes max., Vulkan: 128 bytes of push constants guaranteed

    enum class GraphicsAPI
    {
        Unknown = 0,
        DX12
    };

    enum class ShaderStage : uint16_t
    {
        None = 0x0000,

        Compute = 0x0020,

        Vertex = 0x0001,
        Hull = 0x0002,
        Domain = 0x0004,
        Geometry = 0x0008,
        Pixel = 0x0010,
        Amplification = 0x0040,
        Mesh = 0x0080,
        AllGraphics = 0x00FE,

        RayGeneration = 0x0100,
        AnyHit = 0x0200,
        ClosestHit = 0x0400,
        Miss = 0x0800,
        Intersection = 0x1000,
        Callable = 0x2000,
        AllRayTracing = 0x3F00,

        All = 0x3FFF,
    };
    PHXRHI_ENUM_CLASS_FLAG_OPERATORS(ShaderStage)

    enum class ColourSpace
    {
        SRGB,
        HDR_LINEAR,
        HDR10_ST2084
    };

    enum class RHIFormat : uint8_t
    {
        UNKNOWN = 0,
        R8_UINT,
        R8_SINT,
        R8_UNORM,
        R8_SNORM,
        RG8_UINT,
        RG8_SINT,
        RG8_UNORM,
        RG8_SNORM,
        R16_UINT,
        R16_SINT,
        R16_UNORM,
        R16_SNORM,
        R16_FLOAT,
        BGRA4_UNORM,
        B5G6R5_UNORM,
        B5G5R5A1_UNORM,
        RGBA8_UINT,
        RGBA8_SINT,
        RGBA8_UNORM,
        RGBA8_SNORM,
        BGRA8_UNORM,
        SRGBA8_UNORM,
        SBGRA8_UNORM,
        R10G10B10A2_UNORM,
        R11G11B10_FLOAT,
        RG16_UINT,
        RG16_SINT,
        RG16_UNORM,
        RG16_SNORM,
        RG16_FLOAT,
        R32_UINT,
        R32_SINT,
        R32_FLOAT,
        RGBA16_UINT,
        RGBA16_SINT,
        RGBA16_FLOAT,
        RGBA16_UNORM,
        RGBA16_SNORM,
        RG32_UINT,
        RG32_SINT,
        RG32_FLOAT,
        RGB32_UINT,
        RGB32_SINT,
        RGB32_FLOAT,
        RGBA32_UINT,
        RGBA32_SINT,
        RGBA32_FLOAT,

        D16,
        D24S8,
        X24G8_UINT,
        D32,
        D32S8,
        X32G8_UINT,

        BC1_UNORM,
        BC1_UNORM_SRGB,
        BC2_UNORM,
        BC2_UNORM_SRGB,
        BC3_UNORM,
        BC3_UNORM_SRGB,
        BC4_UNORM,
        BC4_SNORM,
        BC5_UNORM,
        BC5_SNORM,
        BC6H_UFLOAT,
        BC6H_SFLOAT,
        BC7_UNORM,
        BC7_UNORM_SRGB,

        COUNT,
    };

    struct Color
    {
        float R;
        float G;
        float B;
        float A;

        Color()
            : R(0.f), G(0.f), B(0.f), A(0.f)
        { }

        Color(float c)
            : R(c), G(c), B(c), A(c)
        { }

        Color(float r, float g, float b, float a)
            : R(r), G(g), B(b), A(a) { }

        bool operator ==(const Color& other) const { return R == other.R && G == other.G && B == other.B && A == other.A; }
        bool operator !=(const Color& other) const { return !(*this == other); }
    };

    union RHIClearValue
    {
        // TODO: Change to be a flat array
        // float Colour[4];
        Color Colour;
        struct ClearDepthStencil
        {
            float Depth;
            uint32_t Stencil;
        } DepthStencil;
    };

    struct Viewport
    {
        float MinX, MaxX;
        float MinY, MaxY;
        float MinZ, MaxZ;

        Viewport() : MinX(0.f), MaxX(0.f), MinY(0.f), MaxY(0.f), MinZ(0.f), MaxZ(1.f) { }

        Viewport(float width, float height) : MinX(0.f), MaxX(width), MinY(0.f), MaxY(height), MinZ(0.f), MaxZ(1.f) { }

        Viewport(float _minX, float _maxX, float _minY, float _maxY, float _minZ, float _maxZ)
            : MinX(_minX), MaxX(_maxX), MinY(_minY), MaxY(_maxY), MinZ(_minZ), MaxZ(_maxZ)
        { }

        bool operator ==(const Viewport& b) const
        {
            return MinX == b.MinX
                && MinY == b.MinY
                && MinZ == b.MinZ
                && MaxX == b.MaxX
                && MaxY == b.MaxY
                && MaxZ == b.MaxZ;
        }
        bool operator !=(const Viewport& b) const { return !(*this == b); }

        float GetWidth() const { return MaxX - MinX; }
        float GetHeight() const { return MaxY - MinY; }
    };

    struct Rect
    {
        int MinX, MaxX;
        int MinY, MaxY;

        Rect() : MinX(0), MaxX(0), MinY(0), MaxY(0) { }
        Rect(int width, int height) : MinX(0), MaxX(width), MinY(0), MaxY(height) { }
        Rect(int _minX, int _maxX, int _minY, int _maxY) : MinX(_minX), MaxX(_maxX), MinY(_minY), MaxY(_maxY) { }
        explicit Rect(const Viewport& viewport)
            : MinX(int(floorf(viewport.MinX)))
            , MaxX(int(ceilf(viewport.MaxX)))
            , MinY(int(floorf(viewport.MinY)))
            , MaxY(int(ceilf(viewport.MaxY)))
        {
        }

        bool operator ==(const Rect& b) const {
            return MinX == b.MinX && MinY == b.MinY && MaxX == b.MaxX && MaxY == b.MaxY;
        }
        bool operator !=(const Rect& b) const { return !(*this == b); }

        int GetWidth() const { return MaxX - MinX; }
        int GetHeight() const { return MaxY - MinY; }
    };

#pragma region Enums

    enum class Usage
    {
        Default = 0,
        ReadBack,
        Upload
    };

    enum class FormatKind : uint8_t
    {
        Integer,
        Normalized,
        Float,
        DepthStencil
    };

    enum class CommandQueueType : uint8_t
    {
        Graphics = 0,
        Compute,
        Copy,

        Count
    };

    constexpr size_t NumCommandQueues = static_cast<size_t>(CommandQueueType::Count);

    enum class TextureDimension : uint8_t
    {
        Unknown,
        Texture1D,
        Texture1DArray,
        Texture2D,
        Texture2DArray,
        TextureCube,
        TextureCubeArray,
        Texture2DMS,
        Texture2DMSArray,
        Texture3D
    };

    enum class ResourceStates : uint32_t
    {
        Unknown                 = 0,
        Common                  = 1 << 0,
        ConstantBuffer          = 1 << 1,
        VertexBuffer            = 1 << 2,
        IndexGpuBuffer          = 1 << 3,
        IndirectArgument        = 1 << 4,
        ShaderResource          = 1 << 5,
        UnorderedAccess         = 1 << 6,
        RenderTarget            = 1 << 7,
        DepthWrite              = 1 << 8,
        DepthRead               = 1 << 9,
        StreamOut               = 1 << 10,
        CopyDest                = 1 << 11,
        CopySource              = 1 << 12,
        ResolveDest             = 1 << 13,
        ResolveSource           = 1 << 14,
        Present                 = 1 << 15,
        AccelStructRead         = 1 << 16,
        AccelStructWrite        = 1 << 17,
        AccelStructBuildInput   = 1 << 18,
        AccelStructBuildBlas    = 1 << 19,
        ShadingRateSurface      = 1 << 20,
        GenericRead             = 1 << 21,
    };

    PHXRHI_ENUM_CLASS_FLAG_OPERATORS(ResourceStates)

    enum class ShaderType
    {
        None,
        HLSL6,
    };

    enum class FeatureLevel
    {
        SM5,
        SM6
    };

    enum class ShaderModel
    {
        SM_6_0,
        SM_6_1,
        SM_6_2,
        SM_6_3,
        SM_6_4,
        SM_6_5,
        SM_6_6,
        SM_6_7,
    };


    enum class PrimitiveType : uint8_t
    {
        PointList,
        LineList,
        TriangleList,
        TriangleStrip,
        TriangleFan,
        TriangleListWithAdjacency,
        TriangleStripWithAdjacency,
        PatchList
    };

    enum class BlendFactor : uint8_t
    {
        Zero = 1,
        One = 2,
        SrcColor = 3,
        InvSrcColor = 4,
        SrcAlpha = 5,
        InvSrcAlpha = 6,
        DstAlpha = 7,
        InvDstAlpha = 8,
        DstColor = 9,
        InvDstColor = 10,
        SrcAlphaSaturate = 11,
        ConstantColor = 14,
        InvConstantColor = 15,
        Src1Color = 16,
        InvSrc1Color = 17,
        Src1Alpha = 18,
        InvSrc1Alpha = 19,

        // Vulkan names
        OneMinusSrcColor = InvSrcColor,
        OneMinusSrcAlpha = InvSrcAlpha,
        OneMinusDstAlpha = InvDstAlpha,
        OneMinusDstColor = InvDstColor,
        OneMinusConstantColor = InvConstantColor,
        OneMinusSrc1Color = InvSrc1Color,
        OneMinusSrc1Alpha = InvSrc1Alpha,
    };

    enum class EBlendOp : uint8_t
    {
        Add = 1,
        Subrtact = 2,
        ReverseSubtract = 3,
        Min = 4,
        Max = 5
    };

    enum class ColorMask : uint8_t
    {
        // These values are equal to their counterparts in DX11, DX12, and Vulkan.
        Red = 1,
        Green = 2,
        Blue = 4,
        Alpha = 8,
        All = 0xF
    };

    enum class StencilOp : uint8_t
    {
        Keep = 1,
        Zero = 2,
        Replace = 3,
        IncrementAndClamp = 4,
        DecrementAndClamp = 5,
        Invert = 6,
        IncrementAndWrap = 7,
        DecrementAndWrap = 8
    };

    enum class ComparisonFunc : uint8_t
    {
        Never = 1,
        Less = 2,
        Equal = 3,
        LessOrEqual = 4,
        Greater = 5,
        NotEqual = 6,
        GreaterOrEqual = 7,
        Always = 8
    };

    enum class RasterFillMode : uint8_t
    {
        Solid,
        Wireframe,

        // Vulkan names
        Fill = Solid,
        Line = Wireframe
    };

    enum class RasterCullMode : uint8_t
    {
        Back,
        Front,
        None
    };

    // identifies the underlying resource type in a binding
    enum class ResourceType : uint8_t
    {
        None,
        PushConstants,
        ConstantBuffer,
        StaticSampler,
        SRVBuffer,
        SRVTexture,
        BindlessSRV,
        Count
    };

    enum class SamplerAddressMode : uint8_t
    {
        // D3D names
        Clamp,
        Wrap,
        Border,
        Mirror,
        MirrorOnce,

        // Vulkan names
        ClampToEdge = Clamp,
        Repeat = Wrap,
        ClampToBorder = Border,
        MirroredRepeat = Mirror,
        MirrorClampToEdge = MirrorOnce
    };

    enum class SamplerReductionType : uint8_t
    {
        Standard,
        Comparison,
        Minimum,
        Maximum
    };

    enum class BufferMiscFlags
    {
        None = 0,
        Bindless = 1 << 0,
        Raw = 1 << 1,
        Structured = 1 << 2,
        Typed = 1 << 3,
    };

    PHXRHI_ENUM_CLASS_FLAG_OPERATORS(BufferMiscFlags);

    enum class BindingFlags
    {
        None = 0,
        VertexBuffer = 1 << 0,
        IndexBuffer = 1 << 1,
        ConstantBuffer = 1 << 2,
        ShaderResource = 1 << 3,
        RenderTarget = 1 << 4,
        DepthStencil = 1 << 5,
        UnorderedAccess = 1 << 6,
        ShadingRate = 1 << 7,
    };

    PHXRHI_ENUM_CLASS_FLAG_OPERATORS(BindingFlags);

    enum class SubresouceType
    {
        SRV,
        UAV,
        RTV,
        DSV,
    };

#pragma endregion

    // -- Pipeline State objects ---
    struct BlendRenderState
    {
        struct RenderTarget
        {
            bool        BlendEnable = false;
            BlendFactor SrcBlend = BlendFactor::One;
            BlendFactor DestBlend = BlendFactor::Zero;
            EBlendOp    BlendOp = EBlendOp::Add;
            BlendFactor SrcBlendAlpha = BlendFactor::One;
            BlendFactor DestBlendAlpha = BlendFactor::Zero;
            EBlendOp    BlendOpAlpha = EBlendOp::Add;
            ColorMask   ColorWriteMask = ColorMask::All;
        };

        RenderTarget Targets[cMaxRenderTargets];
        bool alphaToCoverageEnable = false;
    };

    struct DepthStencilRenderState
    {
        struct StencilOpDesc
        {
            StencilOp FailOp = StencilOp::Keep;
            StencilOp DepthFailOp = StencilOp::Keep;
            StencilOp PassOp = StencilOp::Keep;
            ComparisonFunc StencilFunc = ComparisonFunc::Always;

            constexpr StencilOpDesc& setFailOp(StencilOp value) { FailOp = value; return *this; }
            constexpr StencilOpDesc& setDepthFailOp(StencilOp value) { DepthFailOp = value; return *this; }
            constexpr StencilOpDesc& setPassOp(StencilOp value) { PassOp = value; return *this; }
            constexpr StencilOpDesc& setStencilFunc(ComparisonFunc value) { StencilFunc = value; return *this; }
        };

        bool            DepthTestEnable = true;
        bool            DepthWriteEnable = true;
        ComparisonFunc  DepthFunc = ComparisonFunc::Less;
        bool            StencilEnable = false;
        uint8_t         StencilReadMask = 0xff;
        uint8_t         StencilWriteMask = 0xff;
        uint8_t         stencilRefValue = 0;
        StencilOpDesc   FrontFaceStencil;
        StencilOpDesc   BackFaceStencil;
    };

    struct RasterRenderState
    {
        RasterFillMode FillMode = RasterFillMode::Solid;
        RasterCullMode CullMode = RasterCullMode::Back;
        bool FrontCounterClockwise = false;
        bool DepthClipEnable = false;
        bool ScissorEnable = false;
        bool MultisampleEnable = false;
        bool AntialiasedLineEnable = false;
        int DepthBias = 0;
        float DepthBiasClamp = 0.f;
        float SlopeScaledDepthBias = 0.f;

        uint8_t ForcedSampleCount = 0;
        bool programmableSamplePositionsEnable = false;
        bool ConservativeRasterEnable = false;
        bool quadFillEnable = false;
        char samplePositionsX[16]{};
        char samplePositionsY[16]{};
    };
    // -- Pipeline State Objects End ---

    class IRHIResource
    {
    public:
        // Non-copyable and non-movable
        IRHIResource(const IRHIResource&) = delete;
        IRHIResource(const IRHIResource&&) = delete;
        IRHIResource& operator=(const IRHIResource&) = delete;
        IRHIResource& operator=(const IRHIResource&&) = delete;

    protected:
        IRHIResource() = default;
        virtual ~IRHIResource() = default;

    };

    struct ShaderDesc
    {
        ShaderStage Stage = ShaderStage::None;
        std::string DebugName = "";
    };

    struct RHIShader;
    using ShaderHandle = Core::Handle<RHIShader>;

    struct VertexAttributeDesc
    {
        static const uint32_t SAppendAlignedElement = 0xffffffff; // automatically figure out AlignedByteOffset depending on Format

        std::string SemanticName;
        uint32_t SemanticIndex = 0;
        RHIFormat Format = RHIFormat::UNKNOWN;
        uint32_t InputSlot = 0;
        uint32_t AlignedByteOffset = SAppendAlignedElement;
        bool IsInstanced = false;
    };

    struct InputLayout;
    using InputLayoutHandle = Core::Handle<InputLayout>;

    struct StaticSamplerParameter
    {
        uint32_t Slot;
        Color BorderColor = 1.f;
        float MaxAnisotropy = 1.f;
        float MipBias = 0.f;

        bool MinFilter = true;
        bool MagFilter = true;
        bool MipFilter = true;
        SamplerAddressMode AddressU = SamplerAddressMode::Clamp;
        SamplerAddressMode AddressV = SamplerAddressMode::Clamp;
        SamplerAddressMode AddressW = SamplerAddressMode::Clamp;
        ComparisonFunc ComparisonFunc = ComparisonFunc::LessOrEqual;
        SamplerReductionType ReductionType = SamplerReductionType::Standard;
    };

    struct ShaderParameter
    {
        uint32_t Slot;
        ResourceType Type : 8;
        bool IsVolatile: 8;
        uint16_t Size : 16;

    };

    struct BindlessShaderParameter
    {
        ResourceType Type : 8;
        uint16_t RegisterSpace : 16;
    };

    // This is just a workaround for now
    class IRootSignatureBuilder
    {
    public:
        virtual ~IRootSignatureBuilder() = default;
    };

    // Shader Binding Layout
    struct ShaderParameterLayout
    {
        ShaderStage Visibility = ShaderStage::None;
        uint32_t RegisterSpace = 0;
        std::vector<ShaderParameter> Parameters;
        std::vector<BindlessShaderParameter> BindlessParameters;
        std::vector<StaticSamplerParameter> StaticSamplers;

        ShaderParameterLayout& AddPushConstantParmaeter(uint32_t shaderRegister, size_t size)
        {
            ShaderParameter p = {};
            p.Size = size;
            p.Slot = shaderRegister;
            p.Type = ResourceType::PushConstants;

            this->AddParameter(std::move(p));

            return *this;
        }

        ShaderParameterLayout& AddCBVParameter(uint32_t shaderRegister, bool IsVolatile = false)
        {
            ShaderParameter p = {};
            p.Size = 0;
            p.Slot = shaderRegister;
            p.IsVolatile = IsVolatile;
            p.Type = ResourceType::ConstantBuffer;

            this->AddParameter(std::move(p));

            return *this;
        }

        ShaderParameterLayout& AddSRVParameter(uint32_t shaderRegister)
        {
            ShaderParameter p = {};
            p.Size = 0;
            p.Slot = shaderRegister;
            p.Type = ResourceType::SRVBuffer;

            this->AddParameter(std::move(p));

            return *this;
        }

        ShaderParameterLayout AddBindlessSRV(uint32_t shaderSpace)
        {
            BindlessShaderParameter p = {};
            p.RegisterSpace = shaderSpace;
            p.Type = ResourceType::BindlessSRV;

            this->AddBindlessParameter(std::move(p));

            return *this;
        }

        ShaderParameterLayout& AddStaticSampler(
            uint32_t shaderRegister,
            bool minFilter,
            bool magFilter,
            bool mipFilter,
            SamplerAddressMode         addressUVW,
            uint32_t				   maxAnisotropy = 16U,
            ComparisonFunc             comparisonFunc = ComparisonFunc::LessOrEqual,
            Color                      borderColor = {1.0f , 1.0f, 1.0f, 0.0f} )
        {
            StaticSamplerParameter& desc = this->StaticSamplers.emplace_back();
            desc.Slot = shaderRegister;
            desc.MinFilter = minFilter;
            desc.MagFilter = magFilter;
            desc.MagFilter = mipFilter;
            desc.AddressU = addressUVW;
            desc.AddressV = addressUVW;
            desc.AddressW = addressUVW;
            desc.MaxAnisotropy = static_cast<float>(maxAnisotropy);
            desc.ComparisonFunc = comparisonFunc;
            desc.BorderColor = borderColor;

            return *this;
        }

        void AddParameter(ShaderParameter&& parameter)
        {
            this->Parameters.emplace_back(parameter);
        }

        void AddBindlessParameter(BindlessShaderParameter&& bindlessParameter)
        {
            this->BindlessParameters.emplace_back(bindlessParameter);
        }
    };

    struct TextureDesc
    {
        BindingFlags BindingFlags = BindingFlags::ShaderResource;
        TextureDimension Dimension = TextureDimension::Texture2D;
        ResourceStates InitialState = ResourceStates::Common;

        RHIFormat Format = RHIFormat::UNKNOWN;
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

        RHIClearValue OptmizedClearValue = {};
        std::string DebugName;
    };

    // New;
    struct Texture;

    // using TextureHandle = std::shared_ptr<ITexture>;
    using TextureHandle = Core::Handle<Texture>;

    struct BufferRange
    {
        uint64_t ByteOffset = 0;
        uint64_t SizeInBytes = 0;

        BufferRange() = default;

        BufferRange(uint64_t byteOffset, uint64_t sizeInBytes)
            : ByteOffset(byteOffset)
            , SizeInBytes(sizeInBytes)
        { }
    };

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

        RHIFormat Format = RHIFormat::UNKNOWN;

        bool AllowUnorderedAccess = false;

        // TODO: Remove
        bool CreateBindless = false;

        std::string DebugName;

        BufferDesc& EnableBindless()
        {
            this->MiscFlags = this->MiscFlags | BufferMiscFlags::Bindless;
            return *this;
        }

        BufferDesc& IsRawBuffer()
        {
            this->MiscFlags = this->MiscFlags | BufferMiscFlags::Raw;
            return *this;
        }

        BufferDesc& IsStructuredBuffer()
        {
            this->MiscFlags = this->MiscFlags | BufferMiscFlags::Structured;
            return *this;
        }

        BufferDesc& IsTypedBuffer()
        {
            this->MiscFlags = this->MiscFlags | BufferMiscFlags::Typed;
            return *this;
        }
    };

    struct Buffer;
    using BufferHandle = Core::Handle<Buffer>;

    struct GraphicsPipelineDesc
    {
        PrimitiveType PrimType = PrimitiveType::TriangleList;
        InputLayoutHandle InputLayout;

        IRootSignatureBuilder* RootSignatureBuilder = nullptr;
        ShaderParameterLayout ShaderParameters;

        ShaderHandle VertexShader;
        ShaderHandle HullShader;
        ShaderHandle DomainShader;
        ShaderHandle GeometryShader;
        ShaderHandle PixelShader;
        
        BlendRenderState BlendRenderState = {};
        DepthStencilRenderState DepthStencilRenderState = {};
        RasterRenderState RasterRenderState = {};

        std::vector<RHIFormat> RtvFormats;
        std::optional<RHIFormat> DsvFormat;

        uint32_t SampleCount = 1;
        uint32_t SampleQuality = 0;
    };

    struct GraphicsPipeline;
    using GraphicsPipelineHandle = Core::Handle<GraphicsPipeline>;

    struct ComputePipelineDesc
    {
        ShaderHandle ComputeShader;
    };

    struct ComputePipeline;
    using ComputePipelineHandle = Core::Handle<ComputePipeline>;

    struct SubresourceData
    {
        const void* pData = nullptr;
        uint32_t rowPitch = 0;
        uint32_t slicePitch = 0;
    };

    struct RTAccelerationStructure;
    using RTAccelerationStructureHandle = Core::Handle<RTAccelerationStructure>;

    struct RTAccelerationStructureDesc
    {
        enum FLAGS
        {
            kEmpty = 0,
            kAllowUpdate = 1 << 0,
            kAllowCompaction = 1 << 1,
            kPreferFastTrace = 1 << 2,
            kPreferFastBuild = 1 << 3,
            kMinimizeMemory = 1 << 4,
        };
        uint32_t Flags = kEmpty;

        enum class Type
        {
            BottomLevel = 0,
            TopLevel
        } Type;

        struct BottomLevelDesc
        {
            struct Geometry
            {
                enum FLAGS
                {
                    kEmpty = 0,
                    kOpaque = 1 << 0,
                    kNoduplicateAnyHitInvocation = 1 << 1,
                    kUseTransform = 1 << 2,
                };
                uint32_t Flags = kEmpty;

                enum class Type
                {
                    Triangles,
                    ProceduralAABB,
                } Type = Type::Triangles;

                struct TrianglesDesc
                {
                    BufferHandle VertexBuffer;
                    BufferHandle IndexBuffer;
                    uint32_t IndexCount = 0;
                    uint64_t IndexOffset = 0;
                    uint32_t VertexCount = 0;
                    uint64_t VertexByteOffset = 0;
                    uint32_t VertexStride = 0;
                    RHI::RHIFormat IndexFormat = RHI::RHIFormat::R32_UINT;
                    RHI::RHIFormat VertexFormat = RHI::RHIFormat::RGB32_FLOAT;
                    BufferHandle Transform3x4Buffer;
                    uint32_t Transform3x4BufferOffset = 0;
                } Triangles;

                struct ProceduralAABBsDesc
                {
                    BufferHandle AABBBuffer;
                    uint32_t Offset = 0;
                    uint32_t Count = 0;
                    uint32_t Stride = 0;
                } AABBs;
            };

            std::vector<Geometry> Geometries;
        } ButtomLevel;

        struct TopLevelDesc
        {
            struct Instance
            {
                enum FLAGS
                {
                    kEmpty = 0,
                    kTriangleCullDisable = 1 << 0,
                    kTriangleFrontCounterClockwise = 1 << 1,
                    kForceOpaque = 1 << 2,
                    kForceNowOpaque = 1 << 3,
                };

                float Transform[3][4];
                uint32_t InstanceId : 24;
                uint32_t InstanceMask : 8;
                uint32_t InstanceContributionToHitGroupIndex : 24;
                uint32_t Flags : 8;
                RTAccelerationStructureHandle BottomLevel = {};
            };

            BufferHandle InstanceBuffer = {};
            uint32_t Offset = 0;
            uint32_t Count = 0;
        } TopLevel;
    };


    struct RenderPassAttachment
    {
        enum class Type
        {
            RenderTarget,
            DepthStencil,
        } Type = Type::RenderTarget;

        enum class LoadOpType
        {
            Load,
            Clear,
            DontCare,
        } LoadOp = LoadOpType::Load;

        TextureHandle Texture;
        int Subresource = -1;

        enum class StoreOpType
        {
            Store,
            DontCare,
        } StoreOp = StoreOpType::Store;

        ResourceStates InitialLayout = ResourceStates::Unknown;	// layout before the render pass
        ResourceStates SubpassLayout = ResourceStates::Unknown;	// layout within the render pass
        ResourceStates FinalLayout = ResourceStates::Unknown;   // layout after the render pass
    };

    struct RenderPassDesc
    {
        enum Flags
        {
            None = 0,
            AllowUavWrites = 1 << 0,
        };

        uint32_t Flags = Flags::None;
        std::vector<RenderPassAttachment> Attachments;
    };

    // Forward Declare, no real implementation
    struct RenderPass;
    using RenderPassHandle = Core::Handle<RenderPass>;

    class ITimerQuery
    {
    public:
        virtual ~ITimerQuery() = default;
    };

    using TimerQueryHandle = std::shared_ptr<ITimerQuery>;

    struct CommandListDesc
    {
        CommandQueueType QueueType = CommandQueueType::Graphics;
        std::string DebugName;
    };

    struct GpuBarrier
    {
        struct BufferBarrier
        {
            RHI::BufferHandle Buffer;
            RHI::ResourceStates BeforeState;
            RHI::ResourceStates AfterState;
        };

        struct TextureBarrier
        {
            RHI::TextureHandle Texture;
            RHI::ResourceStates BeforeState;
            RHI::ResourceStates AfterState;
            int Mip;
            int Slice;
        };

        struct MemoryBarrier
        {
            std::variant<TextureHandle, BufferHandle> Resource;
        };

        std::variant<BufferBarrier, TextureBarrier, GpuBarrier::MemoryBarrier> Data;

        static GpuBarrier CreateMemory()
        {
            GpuBarrier barrier = {};
            barrier.Data = GpuBarrier::MemoryBarrier{};

            return barrier;
        }

        static GpuBarrier CreateMemory(TextureHandle texture)
        {
            GpuBarrier barrier = {};
            barrier.Data = GpuBarrier::MemoryBarrier{ .Resource = texture };

            return barrier;
        }

        static GpuBarrier CreateMemory(BufferHandle buffer)
        {
            GpuBarrier barrier = {};
            barrier.Data = GpuBarrier::MemoryBarrier{ .Resource = buffer };

            return barrier;
        }

        static GpuBarrier CreateTexture(
            TextureHandle texture,
            ResourceStates beforeState,
            ResourceStates afterState,
            int mip = -1,
            int slice = -1)
        {
            GpuBarrier::TextureBarrier t = {};
            t.Texture = texture;
            t.BeforeState = beforeState;
            t.AfterState = afterState;
            t.Mip = mip;
            t.Slice = slice;

            GpuBarrier barrier = {};
            barrier.Data = t;

            return barrier;
        }

        static GpuBarrier CreateBuffer(BufferHandle buffer, ResourceStates beforeState, ResourceStates afterState)
        {
            GpuBarrier::BufferBarrier b = {};
            b.Buffer = buffer;
            b.BeforeState = beforeState;
            b.AfterState = afterState;

            GpuBarrier barrier = {};
            barrier.Data = b;

            return barrier;
        }
    };
    class ScopedMarker;

    struct GPUAllocation
    {
        void* CpuData;
        BufferHandle GpuBuffer;
        size_t Offset;
    };

    class ICommandList : public IRHIResource
    {
    public:
        virtual ~ICommandList() = default;

        virtual void Open() = 0;
        virtual void Close() = 0;

        // -- Ray Trace stuff       ---
        virtual void RTBuildAccelerationStructure(RHI::RTAccelerationStructureHandle accelStructure) = 0;

        // -- Ray Trace Stuff END   ---

        virtual ScopedMarker BeginScopedMarker(std::string name) = 0;
        virtual void BeginMarker(std::string name) = 0;
        virtual void EndMarker() = 0;
        virtual GPUAllocation AllocateGpu(size_t bufferSize, size_t stride) = 0;

        virtual void TransitionBarrier(TextureHandle texture, ResourceStates beforeState, ResourceStates afterState) = 0;
        virtual void TransitionBarrier(BufferHandle buffer, ResourceStates beforeState, ResourceStates afterState) = 0;
        virtual void TransitionBarriers(Core::Span<GpuBarrier> gpuBarriers) = 0;
        virtual void ClearTextureFloat(TextureHandle texture, Color const& clearColour) = 0 ;
        virtual void ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil) = 0;

        virtual void BeginRenderPassBackBuffer() = 0;
        virtual RenderPassHandle GetRenderPassBackBuffer() = 0;
        virtual void BeginRenderPass(RenderPassHandle renderPass) = 0;
        virtual void EndRenderPass() = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0) = 0;
        virtual void DrawIndexed(
            uint32_t indexCount,
            uint32_t instanceCount = 1,
            uint32_t startIndex = 0,
            int32_t baseVertex = 0,
            uint32_t startInstance = 0) = 0;

        template<typename T>
        void WriteBuffer(BufferHandle buffer, std::vector<T> const& data, uint64_t destOffsetBytes = 0)
        {
            this->WriteBuffer(buffer, data.data(), sizeof(T) * data.size(), destOffsetBytes);
        }

        template<typename T>
        void WriteBuffer(BufferHandle buffer, T const& data, uint64_t destOffsetBytes = 0)
        {
            this->WriteBuffer(buffer, &data, sizeof(T), destOffsetBytes);
        }

        // TODO: Take ownership of the data
        virtual void WriteBuffer(BufferHandle buffer, const void* data, size_t dataSize, uint64_t destOffsetBytes = 0) = 0;

        virtual void CopyBuffer(BufferHandle dst, uint64_t dstOffset, BufferHandle src, uint64_t srcOffset, size_t sizeInBytes) = 0;

        virtual void WriteTexture(TextureHandle texture, uint32_t firstSubResource, size_t numSubResources, SubresourceData* pSubResourceData) = 0;
        virtual void WriteTexture(TextureHandle texture, uint32_t arraySlice, uint32_t mipLevel, const void* data, size_t rowPitch, size_t depthPitch) = 0;

        virtual void SetGraphicsPipeline(GraphicsPipelineHandle graphisPSO) = 0;
        virtual void SetViewports(Viewport* viewports, size_t numViewports) = 0;
        virtual void SetScissors(Rect* scissor, size_t numScissors) = 0;
        virtual void SetRenderTargets(std::vector<TextureHandle> const& renderTargets, TextureHandle depthStencil) = 0;

        // -- Comptute Stuff ---
        virtual void SetComputeState(ComputePipelineHandle state) = 0;
        virtual void Dispatch(uint32_t groupsX, uint32_t groupsY = 1, uint32_t groupsZ = 1) = 0;
        virtual void DispatchIndirect(uint32_t offsetBytes) = 0;

        virtual void BindPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants) = 0;
        template<typename T>
        void BindPushConstant(uint32_t rootParameterIndex, const T& constants)
        {
            static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be a multiple of 4 bytes");
            this->BindPushConstant(rootParameterIndex, sizeof(T), &constants);
        }

        virtual void BindConstantBuffer(size_t rootParameterIndex, BufferHandle constantBuffer) = 0;
        virtual void BindDynamicConstantBuffer(size_t rootParameterIndex, size_t sizeInBytes, const void* bufferData) = 0;
        template<typename T>
        void BindDynamicConstantBuffer(size_t rootParameterIndex, T const& bufferData)
        {
            this->BindDynamicConstantBuffer(rootParameterIndex, sizeof(T), &bufferData);
        }

        virtual void BindVertexBuffer(uint32_t slot, BufferHandle vertexBuffer) = 0;

        /**
         * Set dynamic vertex buffer data to the rendering pipeline.
         */
        virtual void BindDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData) = 0;

        template<typename T>
        void BindDynamicVertexBuffer(uint32_t slot, const std::vector<T>& vertexBufferData)
        {
            this->BindDynamicVertexBuffer(slot, vertexBufferData.size(), sizeof(T), vertexBufferData.data());
        }

        virtual void BindIndexBuffer(BufferHandle bufferHandle) = 0;

        /**
         * Bind dynamic index buffer data to the rendering pipeline.
         */
        virtual void BindDynamicIndexBuffer(size_t numIndicies, RHIFormat indexFormat, const void* indexBufferData) = 0;

        template<typename T>
        void BindDynamicIndexBuffer(const std::vector<T>& indexBufferData)
        {
            staticassert(sizeof(T) == 2 || sizeof(T) == 4);

            RHIFormat indexFormat = (sizeof(T) == 2) ? RHIFormat::R16_UINT : RHIFormat::R32_UINT;
            this->BindDynamicIndexBuffer(indexBufferData.size(), indexFormat, indexBufferData.data());
        }

        /**
         * Set dynamic structured buffer contents.
         */
        virtual void BindDynamicStructuredBuffer(uint32_t rootParameterIndex, size_t numElements, size_t elementSize, const void* bufferData) = 0;

        template<typename T>
        void BindDynamicStructuredBuffer(uint32_t rootParameterIndex, std::vector<T> const& bufferData)
        {
            this->BindDynamicStructuredBuffer(rootParameterIndex, bufferData.size(), sizeof(T), bufferData.data());
        }

        virtual void BindStructuredBuffer(size_t rootParameterIndex, BufferHandle buffer) = 0;

        virtual void BindDynamicDescriptorTable(size_t rootParameterIndex, Core::Span<TextureHandle> textures) = 0;
        virtual void BindDynamicUavDescriptorTable(size_t rootParameterIndex, Core::Span<TextureHandle> textures) = 0;

        virtual void BindResourceTable (size_t rootParameterIndex) = 0;
        virtual void BindSamplerTable(size_t rootParameterIndex) = 0;

        virtual void BeginTimerQuery(TimerQueryHandle query) = 0;
        virtual void EndTimerQuery(TimerQueryHandle query) = 0;
    };

    using CommandListHandle = std::shared_ptr<ICommandList>;

    class ScopedMarker
    {
    public:
        ScopedMarker(ICommandList* context)
            : m_commandList(context) {}

        ~ScopedMarker() { this->m_commandList->EndMarker(); }

    private:
        ICommandList* m_commandList;
    };

    struct ExecutionReceipt
    {
        uint64_t FenceValue;
        CommandQueueType CommandQueue;
    };

    enum class DeviceCapability
    {
        None = 0,
        RT_VT_ArrayIndex_Without_GS     = 1 << 0,
        RayTracing                      = 1 << 1,
        RenderPass                      = 1 << 2,
        RayQuery                        = 1 << 3,
        VariableRateShading             = 1 << 4,
        MeshShading                     = 1 << 5,
        CreateNoteZeroed                = 1 << 6,
        Bindless                        = 1 << 7,
    };

    PHXRHI_ENUM_CLASS_FLAG_OPERATORS(DeviceCapability);

    struct ViewportDesc
    {
        Core::Platform::WindowHandle WindowHandle;
        uint32_t Width;
        uint32_t Height;
        uint32_t BufferCount = 3;
        RHIFormat Format = RHIFormat::UNKNOWN;
        bool VSync = false;
        bool EnableHDR = false;
        RHIClearValue OptmizedClearValue =
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

    struct Viewport;
    using ViewportHandle = Core::Handle<Viewport>;

    class IGraphicsDevice
    {
    public:
        virtual ~IGraphicsDevice() = default;

        // Global Singleton not ideal as we can only have one device per application, however, it simplifies a lot when passing the object around
        // TODO: Remove. Only around for compiling.
        inline static IGraphicsDevice* GPtr;

    public:
        virtual void Initialize() = 0;
        virtual void Finalize() = 0;

        // -- Create Functions ---
    public:
        virtual void CreateViewport(ViewportDesc const& desc) = 0;
        virtual void ResizeViewport(ViewportDesc const desc) = 0;
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        // TODO: Change to a new pattern so we don't require a command list stored on an object. Instread, request from a pool of objects
        virtual CommandListHandle CreateCommandList(CommandListDesc const& desc = {}) = 0;
        virtual ICommandList* BeginCommandRecording(CommandQueueType QueueType = CommandQueueType::Graphics) = 0;

        virtual ShaderHandle CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode) = 0;
        virtual InputLayoutHandle CreateInputLayout(VertexAttributeDesc* desc, uint32_t attributeCount) = 0;
        virtual GraphicsPipelineHandle CreateGraphicsPipeline(GraphicsPipelineDesc const& desc) = 0;
        virtual const GraphicsPipelineDesc& GetGfxPipelineDesc(GraphicsPipelineHandle handle) = 0;
        virtual void DeleteGraphicsPipeline(GraphicsPipelineHandle handle) = 0;
        virtual ComputePipelineHandle CreateComputePipeline(ComputePipelineDesc const& desc) = 0;

        virtual TextureHandle CreateTexture(TextureDesc const& desc) = 0;
        virtual const TextureDesc& GetTextureDesc(TextureHandle handle) = 0;
        virtual DescriptorIndex GetDescriptorIndex(TextureHandle handle, SubresouceType type, int subResource = -1) = 0;
        virtual void DeleteTexture(TextureHandle handle) = 0;

        virtual RenderPassHandle CreateRenderPass(RenderPassDesc const& desc) = 0;
        virtual void GetRenderPassFormats(RenderPassHandle handle, std::vector<RHIFormat>& outRtvFormats, RHIFormat& depthFormat) = 0;
        virtual RenderPassDesc GetRenderPassDesc(RenderPassHandle) = 0;
        virtual void DeleteRenderPass(RenderPassHandle handle) = 0;

        virtual int CreateSubresource(TextureHandle texture, SubresouceType subresourceType, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip = 0, uint32_t mpCount = ~0) = 0;
        virtual int CreateSubresource(BufferHandle buffer, SubresouceType subresourceType, size_t offset, size_t size = ~0u) = 0;

        // TODO: I don't think is this a clear interface as to what desc data is required
        // Consider cleaning this up eventually.
        // -- TODO: Remove
        virtual BufferHandle CreateIndexBuffer(BufferDesc const& desc) = 0;
        virtual BufferHandle CreateVertexBuffer(BufferDesc const& desc) = 0;
        // -- TODO: End Remove

        virtual BufferHandle CreateBuffer(BufferDesc const& desc) = 0;
        virtual const BufferDesc& GetBufferDesc(BufferHandle handle) = 0;
        virtual DescriptorIndex GetDescriptorIndex(BufferHandle handle, SubresouceType type, int subResource = -1) = 0;

        template<typename T>
        T* GetBufferMappedData(BufferHandle handle)
        {
            return static_cast<T*>(this->GetBufferMappedData(handle));
        };

        virtual void* GetBufferMappedData(BufferHandle handle) = 0;
        virtual uint32_t GetBufferMappedDataSizeInBytes(BufferHandle handle) = 0;
        virtual void DeleteBuffer(BufferHandle handle) = 0;

        // -- Ray Tracing ---
        virtual RTAccelerationStructureHandle CreateRTAccelerationStructure(RTAccelerationStructureDesc const& desc) = 0;
        virtual size_t GetRTTopLevelAccelerationStructureInstanceSize() = 0;
        virtual void WriteRTTopLevelAccelerationStructureInstance(RTAccelerationStructureDesc::TopLevelDesc::Instance const& instance, void* dest) = 0;
        virtual const RTAccelerationStructureDesc& GetRTAccelerationStructureDesc(RTAccelerationStructureHandle handle) = 0;
        virtual void DeleteRtAccelerationStructure(RTAccelerationStructureHandle handle) = 0;
        virtual DescriptorIndex GetDescriptorIndex(RTAccelerationStructureHandle handle) = 0;

        // -- Query Stuff ---
        virtual TimerQueryHandle CreateTimerQuery() = 0;
        virtual bool PollTimerQuery(TimerQueryHandle query) = 0;
        virtual Core::TimeStep GetTimerQueryTime(TimerQueryHandle query) = 0;
        virtual void ResetTimerQuery(TimerQueryHandle query) = 0;

        // -- Create Functions End ---
        virtual TextureHandle GetBackBuffer() = 0;

        virtual void WaitForIdle() = 0;
        virtual void QueueWaitForCommandList(CommandQueueType waitQueue, ExecutionReceipt waitOnRecipt) = 0;

        virtual ExecutionReceipt ExecuteCommandLists(
            Core::Span<ICommandList*> commandLists,
            CommandQueueType executionQueue = CommandQueueType::Graphics) = 0;

        virtual ExecutionReceipt ExecuteCommandLists(
            Core::Span<ICommandList*> commandLists,
            bool waitForCompletion,
            CommandQueueType executionQueue = CommandQueueType::Graphics) = 0;

        virtual size_t GetNumBindlessDescriptors() const = 0;

        virtual ShaderModel GetMinShaderModel() const = 0;
        virtual ShaderType GetShaderType() const = 0;

        virtual GraphicsAPI GetApi() const = 0;

        virtual void BeginCapture(std::wstring const& filename) = 0;
        virtual void EndCapture(bool discard = false) = 0;

        virtual size_t GetFrameIndex() = 0;
        virtual size_t GetMaxInflightFrames() = 0;

        virtual bool CheckCapability(DeviceCapability deviceCapability) = 0;

        virtual bool IsDevicedRemoved() = 0;
    };

    extern void ReportLiveObjects();
    
    namespace DeviceFactory
    {
        extern IGraphicsDevice* CreateDx12Device();
    }

    std::unique_ptr<IGraphicsDevice> CreatePlatformGfxDevice();


    template <class T>
    void HashCombine(size_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
}


namespace std
{
    // TODO: Custom Hashes
}