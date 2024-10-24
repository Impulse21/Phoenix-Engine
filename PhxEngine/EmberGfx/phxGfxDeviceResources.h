#pragma once

#include <string>
#include <vector>
#include <optional>
#include <variant>

#include "phxSpan.h"
#include "phxEnumUtils.h"
#include "phxHandle.h"

#define USE_HANDLES true
namespace phx::gfx
{
    constexpr size_t kBufferCount = 2;

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

    enum class GfxBackend
    {
        Null = 0,
        Dx12,
        Vulkan
    };

    enum class ShaderFormat : uint8_t
    {
        None,		// Not used
        Hlsl6,		// DXIL
        Spriv,		// SPIR-V
    };

    enum class ShaderStage : uint8_t
    {
        MS,		// Mesh Shader
        AS,		// Amplification Shader
        VS,		// Vertex Shader
        HS,		// Hull Shader
        DS,		// Domain Shader
        GS,		// Geometry Shader
        PS,		// Pixel Shader
        CS,		// Compute Shader
        LIB,	// Shader Library
        Count,
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

    enum class ColourSpace
    {
        SRGB,
        HDR_LINEAR,
        HDR10_ST2084
    };

    enum class Format : uint8_t
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

    enum class ComponentSwizzle : uint8_t
    {
        R,
        G,
        B,
        A,
        Zero,
        One,
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

    union ClearValue
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


    struct RenderPassInfo
    {
        Format RenderTargetFormats[8] = {};
        uint32_t RenderTargetCount = 0;
        Format DsFormat = Format::UNKNOWN;
        uint32_t SampleCount = 1;
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
        Unknown = 0,
        Common = 1 << 0,
        ConstantBuffer = 1 << 1,
        VertexBuffer = 1 << 2,
        IndexGpuBuffer = 1 << 3,
        IndirectArgument = 1 << 4,
        ShaderResource = 1 << 5,
        UnorderedAccess = 1 << 6,
        RenderTarget = 1 << 7,
        DepthWrite = 1 << 8,
        DepthRead = 1 << 9,
        StreamOut = 1 << 10,
        CopyDest = 1 << 11,
        CopySource = 1 << 12,
        ResolveDest = 1 << 13,
        ResolveSource = 1 << 14,
        Present = 1 << 15,
        AccelStructRead = 1 << 16,
        AccelStructWrite = 1 << 17,
        AccelStructBuildInput = 1 << 18,
        AccelStructBuildBlas = 1 << 19,
        ShadingRateSurface = 1 << 20,
        GenericRead = 1 << 21,
        ShaderResourceNonPixel = 1 << 22,
    };

    PHX_ENUM_CLASS_FLAGS(ResourceStates)

    enum class InputClassification : uint8_t
    {
        PerVertexData,
        PerInstanceData,
    };
    enum class FeatureLevel
    {
        SM5,
        SM6
    };

    enum class PrimitiveType : uint8_t
    {
        PointList,
        LineList,
        LineStrip,
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

	enum class DepthWriteMask : uint8_t
	{
		Zero,	// Disables depth write
		All,	// Enables depth write
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
        HasCounter = 1 << 4,
        IsAliasedResource = 1 << 5,
        IndirectArgs = 1 << 6,
        RayTracing = 1 << 7,
        Sparse = 1 << 8,
    };

    PHX_ENUM_CLASS_FLAGS(BufferMiscFlags);

    enum class TextureMiscFlags
    {
        None = 0,
        Sparse = 1 << 0,
        TransientAttachment = 1 << 1,
        TypedFormatCasting = 1 << 2,
        TypelessFormatCasting = 1 << 3,
    };

    PHX_ENUM_CLASS_FLAGS(TextureMiscFlags);

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

    PHX_ENUM_CLASS_FLAGS(BindingFlags);

    enum class SubresouceType
    {
        SRV,
        UAV,
        RTV,
        DSV,
    };

#pragma endregion

    constexpr bool IsFormatSRGB(Format format)
    {
        switch (format)
        {
        case Format::BC1_UNORM_SRGB:
        case Format::BC2_UNORM_SRGB:
        case Format::BC3_UNORM_SRGB:
        case Format::BC7_UNORM_SRGB:
            return true;
        default:
            return false;
        }
    }

    struct GpuDeviceCapabilities
    {
        union
        {
            struct
            {
                uint32_t CacheCoherentUma   : 1;
                uint32_t RayTracing         : 31;
            };
            uint32_t Flags = 0;
        };
    };

    // -- Indirect Objects ---
    // 
    struct IndirectDrawArgInstanced
    {
        uint32_t VertexCount;
        uint32_t InstanceCount;
        uint32_t StartVertex;
        uint32_t StartInstance;
    };

    struct IndirectDrawArgsIndexedInstanced
    {
        uint32_t IndexCount;
        uint32_t InstanceCount;
        uint32_t StartIndex;
        uint32_t VertexOffset;
        uint32_t StartInstance;
    };

    struct IndirectDispatchArgs
    {
        uint32_t GroupCountX;
        uint32_t GroupCountY;
        uint32_t mGroupCountZ;
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
		bool DepthEnable = false;
		DepthWriteMask DepthWriteMask = DepthWriteMask::Zero;
		ComparisonFunc DepthFunc = ComparisonFunc::Never;
		bool StencilEnable = false;
		uint8_t StencilReadMask = 0xff;
		uint8_t StencilWriteMask = 0xff;

		struct DepthStencilOp
		{
			StencilOp StencilFailOp = StencilOp::Keep;
			StencilOp StencilDepthFailOp = StencilOp::Keep;
			StencilOp StencilPassOp = StencilOp::Keep;
			ComparisonFunc StencilFunc = ComparisonFunc::Never;
		};
		DepthStencilOp FrontFace;
		DepthStencilOp BackFace;
		bool DepthBoundsTestEnable = false;
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


    class Resource
    {
    protected:
        Resource() = default;
        virtual ~Resource() = default;

    public:
        virtual unsigned long AddRef() = 0;
        virtual unsigned long Release() = 0;

#if flase
        // Returns a native object or interface, for example ID3D11Device*, or nullptr if the requested interface is unavailable.
        // Does *not* AddRef the returned interface.
        virtual Object getNativeObject(ObjectType objectType) { (void)objectType; return nullptr; }
#endif
        // Non-copyable and non-movable
        Resource(const Resource&) = delete;
        Resource(const Resource&&) = delete;
        Resource& operator=(const Resource&) = delete;
        Resource& operator=(const Resource&&) = delete;
    };

    struct VertexAttributeDesc
    {
        static const uint32_t SAppendAlignedElement = 0xffffffff; // automatically figure out AlignedByteOffset depending on Format

        std::string SemanticName;
        uint32_t SemanticIndex = 0;
        gfx::Format Format = gfx::Format::UNKNOWN;
        uint32_t InputSlot = 0;
        uint32_t AlignedByteOffset = SAppendAlignedElement;
        bool IsInstanced = false;
    };

#if USE_HANDLES
    struct InputLayout;
    using InputLayoutHandle = Handle<InputLayout>;
#else
    class InputLayout : public Resource
    {
    };
    using InputLayoutHandle = RefCountPtr<InputLayout>;
#endif

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
        bool IsVolatile : 8;
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
        ShaderStage Visibility = ShaderStage::VS;
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
            Color                      borderColor = { 1.0f , 1.0f, 1.0f, 0.0f })
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

    struct Swizzle
    {
        ComponentSwizzle R = ComponentSwizzle::R;
        ComponentSwizzle G = ComponentSwizzle::G;
        ComponentSwizzle B = ComponentSwizzle::B;
        ComponentSwizzle A = ComponentSwizzle::A;
    };

    struct AliasDesc;
    struct TextureDesc
    {
        TextureMiscFlags MiscFlags = TextureMiscFlags::None;
        BindingFlags BindingFlags = BindingFlags::ShaderResource;
        TextureDimension Dimension = TextureDimension::Texture2D;
        ResourceStates InitialState = ResourceStates::Common;
        gfx::Format Format = gfx::Format::UNKNOWN;

        uint32_t Width;
        uint32_t Height;

        union
        {
            uint16_t ArraySize = 1;
            uint16_t Depth;
        };

        uint16_t MipLevels = 1;
        uint16_t SampleCount = 1;

        gfx::ClearValue OptmizedClearValue = {};
        AliasDesc* Alias = nullptr;
        std::string DebugName;
        bool IsTypeless : 1 = false;
    };

#if USE_HANDLES
    struct Texture;
    using TextureHandle = Handle<Texture>;
#else
    class Texture : public Resource
    {
    public:
        virtual const TextureDesc& GetDesc() const = 0;
    };
    using TextureHandle = RefCountPtr<Texture>;
    using TextureRef = Texture*;
#endif


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

    enum class IndirectArgumentType
    {
        Draw = 0,
        DrawIndex,
        Dispatch,
        DispatchMesh,
        Constant,
    };

    enum class PipelineType
    {
        None = 0,
        Gfx,
        Compute,
        Mesh,
    };
    struct IndirectArgumnetDesc
    {
        IndirectArgumentType Type;
        union
        {
            struct
            {
                uint32_t Slot;
            } 	VertexBuffer;
            struct
            {
                uint32_t RootParameterIndex;
                uint32_t DestOffsetIn32BitValues;
                uint32_t Num32BitValuesToSet;
            } 	Constant;
            struct
            {
                uint32_t RootParameterIndex;
            } 	ConstantBufferView;
            struct
            {
                uint32_t RootParameterIndex;
            } 	ShaderResourceView;
            struct
            {
                uint32_t RootParameterIndex;
            } 	UnorderedAccessView;
        };
    };

#if USE_HANDLES
    struct Buffer;
    using BufferHandle = Handle<Buffer>;
#else
    struct BufferDesc;
    class Buffer : public Resource
    {
    public:
        virtual const BufferDesc& GetDesc() const = 0;
    };
    using BufferHandle = RefCountPtr<BufferDesc>;
    using BufferRef = Buffer*;
#endif

    struct BufferDesc
    {
        BufferMiscFlags MiscFlags = BufferMiscFlags::None;
        Usage Usage = Usage::Default;
        BindingFlags Binding = BindingFlags::None;
        ResourceStates InitialState = ResourceStates::Common;
        gfx::Format Format = gfx::Format::UNKNOWN;

        size_t SizeInBytes = 0;
        size_t Stride = 0;
        size_t UavCounterOffset = 0;
        BufferHandle UavCounterBuffer = {};

        AliasDesc* Alias = nullptr;
        BufferHandle AliasedBuffer = {};

        std::string DebugName;
    };

    struct AliasDesc
    {
        std::variant<BufferHandle, TextureHandle> Handle;
        size_t AliasOffset = 0;
    };
    using ByteCodeView = Span<uint8_t>;
    struct GfxPipelineDesc
    {
        PrimitiveType PrimType = PrimitiveType::TriangleList;
        InputLayoutHandle InputLayout;

        IRootSignatureBuilder* RootSignatureBuilder = nullptr;
        ShaderParameterLayout ShaderParameters;

        ByteCodeView VertexShaderByteCode;
        ByteCodeView HullShaderByteCode;
        ByteCodeView DomainShaderByteCode;
        ByteCodeView GeometryShaderByteCode;
        ByteCodeView PixelShaderByteCode;

        BlendRenderState BlendRenderState = {};
        DepthStencilRenderState DepthStencilRenderState = {};
        RasterRenderState RasterRenderState = {};

        std::vector<gfx::Format> RtvFormats;
        std::optional<gfx::Format> DsvFormat;

        uint32_t SampleCount = 1;
        uint32_t SampleQuality = 0;
    };

#if USE_HANDLES
    struct GfxPipeline;
    using GfxPipelineHandle = Handle<GfxPipeline>;
#else
    class GfxPipeline : public Resource
    {
    public:
        virtual const GfxPipelineDesc& GetDesc() const = 0;
    };
    using GfxPipelineHandle = RefCountPtr<GfxPipeline>;
#endif


    struct InputLayout
    {
        static const uint32_t APPEND_ALIGNED_ELEMENT = ~0u; // automatically figure out AlignedByteOffset depending on Format

        struct Element
        {
            std::string SemanticName;
            uint32_t SemanticIndex = 0;
            Format Format = Format::UNKNOWN;
            uint32_t InputSlot = 0;
            uint32_t AlignedByteOffset = APPEND_ALIGNED_ELEMENT;
            InputClassification InputSlotClass = InputClassification::PerVertexData;
        };
        std::vector<Element> elements;
    };

    using ByteCodeView = Span<uint8_t>;

    struct ShaderDesc
    {
        ShaderStage Stage;
        ByteCodeView ByteCode;
        const char* EntryPoint = "main";
    };

    struct Shader;
    using ShaderHandle = Handle<Shader>;

    struct PipelineStateDesc2
    {
        ShaderHandle VS = {};
        ShaderHandle PS = {};
        ShaderHandle HS = {};
        ShaderHandle DS = {};
        ShaderHandle GS = {};
        ShaderHandle MS = {};
        ShaderHandle AS = {};
        BlendRenderState*           BlendRenderState = nullptr;
        DepthStencilRenderState*    DepthStencilRenderState = nullptr;
        RasterRenderState*          RasterRenderState = nullptr;
        InputLayout*                InputLayout = nullptr;
        PrimitiveType		        PrimType = PrimitiveType::TriangleList;
        uint32_t                    PatchControlPoints = 3;
        uint32_t			        SampleMask = 0xFFFFFFFF;
    };

    struct PipelineStateDesc
    {
        PrimitiveType PrimType = PrimitiveType::TriangleList;
        InputLayoutHandle InputLayout;

        IRootSignatureBuilder* RootSignatureBuilder = nullptr;
        ShaderParameterLayout ShaderParameters;

        ByteCodeView VertexShaderByteCode;
        ByteCodeView HullShaderByteCode;
        ByteCodeView DomainShaderByteCode;
        ByteCodeView GeometryShaderByteCode;
        ByteCodeView PixelShaderByteCode;
        ByteCodeView ComputeShaderByteCode;

        BlendRenderState BlendRenderState = {};
        DepthStencilRenderState DepthStencilRenderState = {};
        RasterRenderState RasterRenderState = {};

        std::vector<gfx::Format> RtvFormats;
        std::optional<gfx::Format> DsvFormat;

        uint32_t SampleCount = 1;
        uint32_t SampleQuality = 0;
    };

    struct PipelineState;
    using PipelineStateHandle = Handle<PipelineState>;

    struct ComputePipelineDesc
    {
        ByteCodeView ComputeShader;
    };

    struct ComputePipeline;
    using ComputePipelineHandle = Handle<ComputePipeline>;

    struct MeshPipelineDesc
    {
        PrimitiveType PrimType = PrimitiveType::TriangleList;

        ByteCodeView AmpShader;
        ByteCodeView MeshShader;
        ByteCodeView PixelShader;

        BlendRenderState BlendRenderState = {};
        DepthStencilRenderState DepthStencilRenderState = {};
        RasterRenderState RasterRenderState = {};

        std::vector<gfx::Format> RtvFormats;
        std::optional<gfx::Format> DsvFormat;

        uint32_t SampleCount = 1;
        uint32_t SampleQuality = 0;

    };

    struct MeshPipeline;
    using MeshPipelineHandle = Handle<MeshPipeline>;

    struct SubresourceData
    {
        const void* pData = nullptr;
        uint32_t rowPitch = 0;
        uint32_t slicePitch = 0;
    };

    struct RTAccelerationStructure;
    using RTAccelerationStructureHandle = Handle<RTAccelerationStructure>;

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
                    gfx::Format IndexFormat = gfx::Format::R32_UINT;
                    gfx::Format VertexFormat = gfx::Format::RGB32_FLOAT;
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
    using RenderPassHandle = Handle<RenderPass>;

    struct TimerQuery;
    using TimerQueryHandle = uint32_t;

    struct GpuBarrier
    {
        struct BufferBarrier
        {
            gfx::BufferHandle Buffer;
            gfx::ResourceStates BeforeState;
            gfx::ResourceStates AfterState;
        };

        struct TextureBarrier
        {
            gfx::TextureHandle Texture;
            gfx::ResourceStates BeforeState;
            gfx::ResourceStates AfterState;
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
        size_t SizeInBytes;
    };

    struct CommandSignatureDesc
    {
        Span<IndirectArgumnetDesc> ArgDesc;

        PipelineType PipelineType;
        union
        {
            gfx::GfxPipelineHandle GfxHandle;
            gfx::ComputePipelineHandle ComputeHandle;
            gfx::MeshPipelineHandle MeshHandle;
        };
    };

    struct CommandSignature;
    using CommandSignatureHandle = Handle<CommandSignature>;

    struct ExecutionReceipt
    {
        uint64_t FenceValue;
        CommandQueueType CommandQueue;
    };

    enum class DeviceCapability
    {
        None = 0,
        RT_VT_ArrayIndex_Without_GS = 1 << 0,
        RayTracing = 1 << 1,
        RenderPass = 1 << 2,
        RayQuery = 1 << 3,
        VariableRateShading = 1 << 4,
        MeshShading = 1 << 5,
        CreateNoteZeroed = 1 << 6,
        Bindless = 1 << 7,
    };

    PHX_ENUM_CLASS_FLAGS(DeviceCapability);

    struct SwapChainDesc
    {
        uint32_t Width = 0;
        uint32_t Height = 0;
        gfx::Format Format = gfx::Format::R10G10B10A2_UNORM;
        gfx::ClearValue OptmizedClearValue =
        {
            .Colour =
            {
                0.0f,
                0.0f,
                0.0f,
                1.0f,
            }
        };

        bool Fullscreen : 1 = false;
        bool VSync : 1 = false;
        bool EnableHDR : 1 = false;
    };

    struct DynamicMemoryPage
    {
        BufferHandle BufferHandle;
        size_t Offset;
        uint8_t* Data;
    };

    constexpr uint32_t GetFormatStride(Format format)
    {
        switch (format)
        {
        case Format::BC1_UNORM:
        case Format::BC1_UNORM_SRGB:
        case Format::BC4_SNORM:
        case Format::BC4_UNORM:
            return 8u;

        case Format::RGBA32_FLOAT:
        case Format::RGBA32_UINT:
        case Format::RGBA32_SINT:
        case Format::BC2_UNORM:
        case Format::BC2_UNORM_SRGB:
        case Format::BC3_UNORM:
        case Format::BC3_UNORM_SRGB:
        case Format::BC5_SNORM:
        case Format::BC5_UNORM:
        case Format::BC6H_UFLOAT:
        case Format::BC6H_SFLOAT:
        case Format::BC7_UNORM:
        case Format::BC7_UNORM_SRGB:
            return 16u;

        case Format::RGB32_FLOAT:
        case Format::RGB32_UINT:
        case Format::RGB32_SINT:
            return 12u;

        case Format::RGBA16_FLOAT:
        case Format::RGBA16_UNORM:
        case Format::RGBA16_UINT:
        case Format::RGBA16_SNORM:
        case Format::RGBA16_SINT:
            return 8u;

        case Format::RG32_FLOAT:
        case Format::RG32_UINT:
        case Format::RG32_SINT:
        case Format::D32S8:
            return 8u;

        case Format::R10G10B10A2_UNORM:
        //case Format::R10G10B10A2_UINT:
        case Format::R11G11B10_FLOAT:
        case Format::RGBA8_UNORM:
        // case Format::RGBA8_UNORM_SRGB:
        case Format::RGBA8_UINT:
        case Format::RGBA8_SNORM:
        case Format::RGBA8_SINT:
        case Format::BGRA8_UNORM:
        // case Format::BGRA8_UNORM_SRGB:
        case Format::RG16_FLOAT:
        case Format::RG16_UNORM:
        case Format::RG16_UINT:
        case Format::RG16_SNORM:
        case Format::RG16_SINT:
        // case Format::D32_FLOAT:
        case Format::R32_FLOAT:
        case Format::R32_UINT:
        case Format::R32_SINT:
        // case Format::D24_UNORM_S8_UINT:
        // case Format::R9G9B9E5_SHAREDEXP:
            return 4u;

        case Format::RG8_UNORM:
        case Format::RG8_UINT:
        case Format::RG8_SNORM:
        case Format::RG8_SINT:
        case Format::R16_FLOAT:
        case Format::D16:
        case Format::R16_UNORM:
        case Format::R16_UINT:
        case Format::R16_SNORM:
        case Format::R16_SINT:
            return 2u;

        case Format::R8_UNORM:
        case Format::R8_UINT:
        case Format::R8_SNORM:
        case Format::R8_SINT:
            return 1u;


        default:
            assert(0); // didn't catch format!
            return 16u;
        }
    }

    constexpr bool IsFormatBlockCompressed(Format format)
    {
        switch (format)
        {
        case Format::BC1_UNORM:
        case Format::BC1_UNORM_SRGB:
        case Format::BC2_UNORM:
        case Format::BC2_UNORM_SRGB:
        case Format::BC3_UNORM:
        case Format::BC3_UNORM_SRGB:
        case Format::BC4_UNORM:
        case Format::BC4_SNORM:
        case Format::BC5_UNORM:
        case Format::BC5_SNORM:
        case Format::BC7_UNORM:
        case Format::BC7_UNORM_SRGB:
            return true;
        default:
            return false;
        }
    }

    constexpr uint32_t GetFormatBlockSize(Format format)
    {
        if (IsFormatBlockCompressed(format))
        {
            return 4u;
        }
        return 1u;
    }
}

namespace std
{
    // TODO: Custom Hashes
}