#pragma once

#include "Core/Platform.h"
#include "Core/TimeStep.h"
#include "Core/Span.h"

#include <stdint.h>
#include <optional>
#include <string>
#include <vector>
#include <memory>
#include <variant>

#define PHXRHI_ENUM_CLASS_FLAG_OPERATORS(T) \
    inline T operator | (T a, T b) { return T(uint32_t(a) | uint32_t(b)); } \
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

    enum class CpuAccessMode
    {
        Default = 0,
        Read,
        Write
    };

    enum class FormatType : uint8_t
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
    };

    PHXRHI_ENUM_CLASS_FLAG_OPERATORS(ResourceStates)

    enum class ShaderType
    {
        None,
        HLSL6,
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

    enum class ShaderStage : uint16_t
    {
        None            = 0x0000,

        Compute         = 0x0020,

        Vertex          = 0x0001,
        Hull            = 0x0002,
        Domain          = 0x0004,
        Geometry        = 0x0008,
        Pixel           = 0x0010,
        Amplification   = 0x0040,
        Mesh            = 0x0080,
        AllGraphics     = 0x00FE,

        RayGeneration   = 0x0100,
        AnyHit          = 0x0200,
        ClosestHit      = 0x0400,
        Miss            = 0x0800,
        Intersection    = 0x1000,
        Callable        = 0x2000,
        AllRayTracing   = 0x3F00,

        All             = 0x3FFF,
    };
    PHXRHI_ENUM_CLASS_FLAG_OPERATORS(ShaderStage)

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
        SrvView = 1 << 0,
        Bindless = 1 << 1,
        Raw = 1 << 2,
        Structured = 1 << 3,
        Typed = 1 << 4,
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
#pragma endregion

    struct SwapChainDesc
    {
        uint32_t Width;
        uint32_t Height;
        uint32_t BufferCount = 3;
        FormatType Format = FormatType::UNKNOWN;
        bool VSync = false;
        Core::Platform::WindowHandle WindowHandle;
    };

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

    class IResource
    {
    protected:
        IResource() = default;
        virtual ~IResource() = default;

    public:

        // Non-copyable and non-movable
        IResource(const IResource&) = delete;
        IResource(const IResource&&) = delete;
        IResource& operator=(const IResource&) = delete;
        IResource& operator=(const IResource&&) = delete;
    };

    struct ShaderDesc
    {
        ShaderStage Stage = ShaderStage::None;
        std::string DebugName = "";
    };

    class IShader : public IResource
    {
    public:
        virtual ~IShader() = default;

        virtual const ShaderDesc& GetDesc() const = 0;
        virtual const std::vector<uint8_t>& GetByteCode() const = 0;
    };

    using ShaderHandle = std::shared_ptr<IShader>;

    struct VertexAttributeDesc
    {
        static const uint32_t SAppendAlignedElement = 0xffffffff; // automatically figure out AlignedByteOffset depending on Format

        std::string SemanticName;
        uint32_t SemanticIndex = 0;
        FormatType Format = FormatType::UNKNOWN;
        uint32_t InputSlot = 0;
        uint32_t AlignedByteOffset = SAppendAlignedElement;
        bool IsInstanced = false;
    };

    class IInputLayout : public IResource
    {
    public:
        [[nodiscard]] virtual uint32_t GetNumAttributes() const = 0;
        [[nodiscard]] virtual const VertexAttributeDesc* GetAttributeDesc(uint32_t index) const = 0;
    };

    using InputLayoutHandle = std::shared_ptr<IInputLayout>;

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

    struct GraphicsPSODesc
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

        std::vector<FormatType> RtvFormats;
        std::optional<FormatType> DsvFormat;

        uint32_t SampleCount = 1;
        uint32_t SampleQuality = 0;
    };

    class IGraphicsPSO : public IResource
    {
    public:
        virtual ~IGraphicsPSO() = default;

        virtual const GraphicsPSODesc& GetDesc() const = 0;
    };

    using GraphicsPSOHandle = std::shared_ptr<IGraphicsPSO>;

    struct ComputePSODesc
    {
        ShaderHandle ComputeShader;

    };

    class IComputePSO : public IResource
    {
    public:
        virtual ~IComputePSO() = default;

        virtual const ComputePSODesc& GetDesc() const = 0;
    };

    using ComputePSOHandle = std::shared_ptr<IComputePSO>;

    struct SubresourceData
    {
        const void* pData = nullptr;
        uint32_t rowPitch = 0;
        uint32_t slicePitch = 0;
    };

    struct TextureDesc
    {
        BindingFlags BindingFlags = BindingFlags::ShaderResource;
        TextureDimension Dimension = TextureDimension::Unknown;
        ResourceStates InitialState = ResourceStates::Common;

        FormatType Format = FormatType::UNKNOWN;
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

        std::optional<Color> OptmizedClearValue;
        std::string DebugName;
    };

    class ITexture : public IResource
    {
    public:
        virtual ~ITexture() = default;
        virtual const TextureDesc& GetDesc() const = 0;
        virtual const DescriptorIndex GetDescriptorIndex() const = 0;
    };

    using TextureHandle = std::shared_ptr<ITexture>;

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
        CpuAccessMode CpuAccessMode = CpuAccessMode::Default;

        // Stride is required for structured buffers
        uint64_t StrideInBytes = 0;
        uint64_t SizeInBytes = 0;

        FormatType Format = FormatType::UNKNOWN;

        bool AllowUnorderedAccess = false;

        // TODO: Remove
        bool CreateSRVViews = false;
        bool CreateBindless = false;

        std::string DebugName;

        // TODO: I am here
        BufferDesc& CreateSrvViews()
        {
            this->MiscFlags = this->MiscFlags | BufferMiscFlags::SrvView;
            return *this;
        }

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

    class IBuffer : public IResource
    {
    public:
        virtual ~IBuffer() = default;

        virtual const BufferDesc& GetDesc() const = 0;
        virtual const DescriptorIndex GetDescriptorIndex() const = 0;
    };

    using BufferHandle = std::shared_ptr<IBuffer>;

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
        };

        std::variant<BufferBarrier, TextureBarrier> Data;

        static GpuBarrier CreateTexture(TextureHandle texture, ResourceStates beforeState, ResourceStates afterState)
        {
            GpuBarrier::TextureBarrier t = {};
            t.Texture = texture;
            t.BeforeState = beforeState;
            t.AfterState = afterState;

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

    class ICommandList : public IResource
    {
    public:
        virtual ~ICommandList() = default;

        virtual void Open() = 0;
        virtual void Close() = 0;

        virtual ScopedMarker BeginScopedMarker(std::string name) = 0;
        virtual void BeginMarker(std::string name) = 0;
        virtual void EndMarker() = 0;

        virtual void TransitionBarrier(TextureHandle texture, ResourceStates beforeState, ResourceStates afterState) = 0;
        virtual void TransitionBarrier(BufferHandle buffer, ResourceStates beforeState, ResourceStates afterState) = 0;
        virtual void TransitionBarriers(Core::Span<GpuBarrier> gpuBarriers) = 0;
        virtual void ClearTextureFloat(TextureHandle texture, Color const& clearColour) = 0 ;
        virtual void ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil) = 0;

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

        // TODO: Take ownership of the data
        virtual void WriteBuffer(BufferHandle buffer, const void* data, size_t dataSize, uint64_t destOffsetBytes = 0) = 0;

        virtual void WriteTexture(TextureHandle texture, uint32_t firstSubResource, size_t numSubResources, SubresourceData* pSubResourceData) = 0;
        virtual void WriteTexture(TextureHandle texture, uint32_t arraySlice, uint32_t mipLevel, const void* data, size_t rowPitch, size_t depthPitch) = 0;
        virtual void SetGraphicsPSO(GraphicsPSOHandle graphisPSO) = 0;
        virtual void SetViewports(Viewport* viewports, size_t numViewports) = 0;
        virtual void SetScissors(Rect* scissor, size_t numScissors) = 0;
        virtual void SetRenderTargets(std::vector<TextureHandle> const& renderTargets, TextureHandle depthStencil) = 0;

        // -- Comptute Stuff ---
        virtual void SetComputeState(ComputePSOHandle state) = 0;
        virtual void Dispatch(uint32_t groupsX, uint32_t groupsY = 1, uint32_t groupsZ = 1) = 0;
        virtual void DispatchIndirect(uint32_t offsetBytes) = 0;

        virtual void BindPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants) = 0;
        template<typename T>
        void BindPushConstant(uint32_t rootParameterIndex, const T& constants)
        {
            static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be a multiple of 4 bytes");
            this->BindPushConstant(rootParameterIndex, sizeof(T), &constants);
        }

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
        virtual void BindDynamicIndexBuffer(size_t numIndicies, FormatType indexFormat, const void* indexBufferData) = 0;

        template<typename T>
        void BindDynamicIndexBuffer(const std::vector<T>& indexBufferData)
        {
            staticassert(sizeof(T) == 2 || sizeof(T) == 4);

            FormatType indexFormat = (sizeof(T) == 2) ? FormatType::R16_UINT : FormatType::R32_UINT;
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

        virtual void BindDynamicDescriptorTable(size_t rootParameterIndex, std::vector<TextureHandle> const& textures) = 0;
        virtual void BindDynamicUavDescriptorTable(size_t rootParameterIndex, std::vector<TextureHandle> const& textures) = 0;

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

    class IGpuAdapter
    {
    public:
        virtual ~IGpuAdapter() = default;

        virtual const char* GetName() const = 0;
        virtual size_t GetDedicatedSystemMemory() const = 0;
        virtual size_t GetDedicatedVideoMemory() const = 0;
        virtual size_t GetSharedSystemMemory() const = 0;

    };

    struct ExecutionReceipt
    {
        uint64_t FenceValue;
        CommandQueueType CommandQueue;
    };

    class IGraphicsDevice
    {
    public:
        virtual ~IGraphicsDevice() = default;

        // -- Create Functions ---
    public:
        virtual void CreateSwapChain(SwapChainDesc const& swapChainDesc) = 0;
        virtual CommandListHandle CreateCommandList(CommandListDesc const& desc = {}) = 0;

        virtual ShaderHandle CreateShader(ShaderDesc const& desc, const void* binary, size_t binarySize) = 0;
        virtual InputLayoutHandle CreateInputLayout(VertexAttributeDesc* desc, uint32_t attributeCount) = 0;
        virtual GraphicsPSOHandle CreateGraphicsPSO(GraphicsPSODesc const& desc) = 0;
        virtual ComputePSOHandle CreateComputePso(ComputePSODesc const& desc) = 0;

        virtual TextureHandle CreateDepthStencil(TextureDesc const& desc) = 0;
        virtual TextureHandle CreateTexture(TextureDesc const& desc) = 0;

        // TODO: I don't think is this a clear interface as to what desc data is required
        // Consider cleaning this up eventually.
        virtual BufferHandle CreateIndexBuffer(BufferDesc const& desc) = 0;
        virtual BufferHandle CreateVertexBuffer(BufferDesc const& desc) = 0;
        virtual BufferHandle CreateBuffer(BufferDesc const& desc) = 0;

        // -- Query Stuff ---
        virtual TimerQueryHandle CreateTimerQuery() = 0;
        virtual bool PollTimerQuery(TimerQueryHandle query) = 0;
        virtual Core::TimeStep GetTimerQueryTime(TimerQueryHandle query) = 0;
        virtual void ResetTimerQuery(TimerQueryHandle query) = 0;

        // -- Create Functions End ---
        virtual TextureHandle GetBackBuffer() = 0;

        virtual void Present() = 0;
        virtual void WaitForIdle() = 0;
        virtual void QueueWaitForCommandList(CommandQueueType waitQueue, ExecutionReceipt waitOnRecipt) = 0;

        virtual ExecutionReceipt ExecuteCommandLists(
            ICommandList* const* pCommandLists,
            size_t numCommandLists,
            CommandQueueType executionQueue = CommandQueueType::Graphics) = 0;

        virtual ExecutionReceipt ExecuteCommandLists(
            ICommandList* const* pCommandLists,
            size_t numCommandLists,
            bool waitForCompletion,
            CommandQueueType executionQueue = CommandQueueType::Graphics) = 0;

        // Front-end for executeCommandLists(..., 1) for compatibility and convenience
        ExecutionReceipt ExecuteCommandLists(ICommandList* commandList, CommandQueueType executionQueue = CommandQueueType::Graphics)
        {
            return this->ExecuteCommandLists(&commandList, 1, executionQueue);
        }

        ExecutionReceipt ExecuteCommandLists(ICommandList* commandList, bool waitForCompletion, CommandQueueType executionQueue = CommandQueueType::Graphics)
        {
            return this->ExecuteCommandLists(&commandList, 1, waitForCompletion, executionQueue);
        }

        virtual size_t GetNumBindlessDescriptors() const = 0;

        virtual ShaderModel GetMinShaderModel() const = 0;
        virtual ShaderType GetShaderType() const = 0;

        virtual GraphicsAPI GetApi() const = 0;
        virtual const IGpuAdapter* GetGpuAdapter() const = 0;
    };

    extern void ReportLiveObjects();
}