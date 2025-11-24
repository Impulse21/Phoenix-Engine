#pragma once

#include <cstdint>
#include <variant>

#include "PhxCore/Base.h"
#include "PhxCore/Span.h"
#include "PhxCore/EnumUtils.h"

#include "PhxCore/Handle.h"

namespace phx::rhi
{

    using DescriptorIndex = uint32_t;

    constexpr size_t cMaxInflightFrames = 3;
    constexpr DescriptorIndex cInvalidDescriptorIndex = ~0u;

    constexpr uint32_t cMaxRenderTargets = 8;
    constexpr uint32_t cMaxViewports = 16;
    constexpr uint32_t cMaxVertexAttributes = 16;
    constexpr uint32_t cMaxBindingLayouts = 5;
    constexpr uint32_t cMaxBindingsPerLayout = 128;
    constexpr uint32_t cMaxVolatileConstantBuffersPerLayout = 6;
    constexpr uint32_t cMaxVolatileConstantBuffers = 32;
    constexpr uint32_t cMaxPushConstantSize = 128;      // D3D12: root signature is 256 bytes max., Vulkan: 128 bytes of push constants guaranteed

#pragma region Enums

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
        Spirv,		// SPIR-V
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

    enum class Usage
    {
        Default = 0,
        ReadBack,
        Upload,
        Dynamic          // CPU writes, GPU reads directly (for UI, constant buffers, etc.)
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

    enum class TextureType : uint8_t
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

    enum class ResourceMiscFlags
    {
        None = 0,
        TextureCube = BIT(0),
        IndirectArgs = BIT(1),
        BufferRaw = BIT(2),
        BufferStructured = BIT(3),
        RayTracing = BIT(4),
        AliasBuffer = BIT(5),
        AliasTexture_NonRtDs = BIT(6),
        AliasTexture_RtDs = BIT(7),
        Sparse = BIT(8),
        HasCounter = BIT(9),
        TypedFormatCasting = BIT(10),	// enable casting formats between same type and different modifiers: eg. UNORM -> SRGB
        TypelessFormatCasting = BIT(11),  // enable casting formats to other formats that have the s
        DescriptorTable = BIT(12),
        Alias = AliasBuffer | AliasTexture_NonRtDs | AliasTexture_RtDs,

    };

    PHX_ENUM_CLASS_FLAGS(ResourceMiscFlags);

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
        IndirectBuffer = 1 << 8,
        RayTracing = 1 << 9,
    };

    PHX_ENUM_CLASS_FLAGS(BindingFlags);

    enum class SubresouceType
    {
        SRV,
        UAV,
        RTV,
        DSV,
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
        AliasingGeneric = 1 << 8,
    };

    PHX_ENUM_CLASS_FLAGS(DeviceCapability);
#pragma endregion

#pragma region Types

    struct Descriptor
    {
        uint32_t MaxNumTextures = 1000;
        uint32_t MaxNumGpuBuffers = 1000;
        uint32_t MaxNumPipelineStates = 1000;

    };
    struct Budget
    {
        uint64_t BudgetBytes = 0ull;
        uint64_t UsageBytes = 0ull;
    };

    struct SparseTextureProperties
    {
        uint32_t TileWidth = 0;				    // width of 1 tile in texels
        uint32_t TileHeight = 0;				// height of 1 tile in texels
        uint32_t TileDepth = 0;				// depth of 1 tile in texels
        uint32_t TotalTileCount = 0;			// number of tiles for entire resource
        uint32_t PackedMipStart = 0;			// first mip of packed mipmap levels, these cannot be individually mapped and they cannot use a box mapping
        uint32_t PackedMipCount = 0;			// number of packed mipmap levels, these cannot be individually mapped and they cannot use a box mapping
        uint32_t PackedMipTileOffset = 0;	    // offset of the tiles for packed mip data relative to the entire resource
        uint32_t PackedMipTileCount = 0;		// how many tiles are required for the packed mipmaps
    };

    struct MemInfo
    {
        const void* Data = nullptr;
        uint32_t RowPitch = 0;
        uint32_t SlicePitch = 0;
    };

    struct Color
    {
        float R;
        float G;
        float B;
        float A;

        Color()
            : R(0.f), G(0.f), B(0.f), A(0.f)
        {
        }

        Color(float c)
            : R(c), G(c), B(c), A(c)
        {
        }

        Color(float r, float g, float b, float a)
            : R(r), G(g), B(b), A(a) {
        }

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

    struct Swizzle
    {
        ComponentSwizzle r = ComponentSwizzle::R;
        ComponentSwizzle g = ComponentSwizzle::G;
        ComponentSwizzle b = ComponentSwizzle::B;
        ComponentSwizzle a = ComponentSwizzle::A;
    };

    struct Viewport
    {
        float MinX, MaxX;
        float MinY, MaxY;
        float MinZ, MaxZ;

        Viewport() : MinX(0.f), MaxX(0.f), MinY(0.f), MaxY(0.f), MinZ(0.f), MaxZ(1.f) {}

        Viewport(float width, float height) : MinX(0.f), MaxX(width), MinY(0.f), MaxY(height), MinZ(0.f), MaxZ(1.f) {}

        Viewport(float _minX, float _maxX, float _minY, float _maxY, float _minZ, float _maxZ)
            : MinX(_minX), MaxX(_maxX), MinY(_minY), MaxY(_maxY), MinZ(_minZ), MaxZ(_maxZ)
        {
        }

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

    struct SubresourceData
    {
        const void* data_ptr = nullptr;	// pointer to the beginning of the subresource data (pointer to beginning of resource + subresource offset)
        uint32_t row_pitch = 0;			// bytes between two rows of a texture (2D and 3D textures)
        uint32_t slice_pitch = 0;		// bytes between two depth slices of a texture (3D textures only)
    };

    struct Rect
    {
        int MinX, MaxX;
        int MinY, MaxY;

        Rect() : MinX(0), MaxX(0), MinY(0), MaxY(0) {}
        Rect(int width, int height) : MinX(0), MaxX(width), MinY(0), MaxY(height) {}
        Rect(int _minX, int _maxX, int _minY, int _maxY) : MinX(_minX), MaxX(_maxX), MinY(_minY), MaxY(_maxY) {}
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
        DepthStencilOp FrontFace = {};
        DepthStencilOp BackFace = {};
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
#pragma endregion

#pragma region Helper_Functions

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
#pragma endregion

#pragma region Device_Types

    struct VertexBufferBinding
    {
        static const uint32_t sAppendAlignedElement = ~0u; // automatically figure out AlignedByteOffset depending on Format

        const char* SemanticName;
        Format Format = Format::UNKNOWN;
        uint32_t InputSlot = 0;
        uint32_t AlignedByteOffset = sAppendAlignedElement;
        InputClassification InputSlotClass = InputClassification::PerVertexData;
    };

    struct RenderPassInfo
    {
        phx::Span<Format> RTFormats = {};
        Format DsFormat = Format::UNKNOWN;
        uint32_t SampleCount = 1;
    };
    
    struct Swapchain;
    using SwapchainHandle = Handle<Swapchain>;
    struct SwapchainDesc
    {
        uint32_t Width = 0;
        uint32_t Height = 0;
        rhi::Format Format = rhi::Format::R10G10B10A2_UNORM;
        rhi::ClearValue OptmizedClearValue =
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

    struct ShaderModuleDescriptor
    {
        phx::Span<uint8_t> byte_code;
        bool IsValid() const { return !byte_code.IsEmpty(); }
    };

    struct ShaderModule;
    using ShaderModuleHandle = Handle<ShaderModule>;

    struct PipelineState;
    using PipelineStateHandle = Handle<PipelineState>;
    struct PipelineStateDescriptor
    {
        struct ShaderStageInfo 
        {
            ShaderModuleHandle module;
            const char* entry_point;
        };

        ShaderStageInfo VS = {};
        ShaderStageInfo PS = {};
        ShaderStageInfo HS = {};
        ShaderStageInfo DS = {};
        ShaderStageInfo GS = {};
        ShaderStageInfo MS = {};
        ShaderStageInfo AS = {};

        BlendRenderState        BlendState = {};
        DepthStencilRenderState DepthStencilState = {};
        RasterRenderState       RasterState = {};

        rhi::PrimitiveType              PrimType = rhi::PrimitiveType::TriangleList;
        phx::Span<VertexBufferBinding>  VertexBufferBindings;
        RenderPassInfo                  RenderPassInfo;
        uint32_t                        PatchControlPoints = 3;
        uint32_t			            SampleMask = ~0u;
    };

    struct Buffer;
    using BufferHandle = Handle<Buffer>;
    struct Texture;
    using TextureHandle = Handle<Texture>;

    struct BufferDescriptor
    {
        const char* DebugName = "";
        rhi::Format Format = rhi::Format::UNKNOWN;
        uint32_t Size = 0;
        uint32_t Stride = 0;
        Usage Usage = Usage::Default;

        BindingFlags BindingFlags = BindingFlags::None;
        ResourceMiscFlags MiscFlags = ResourceMiscFlags::None;
        ResourceStates InitialState = ResourceStates::Common;

        struct AliasDescriptor
        {
            std::variant<BufferHandle, TextureHandle> handle;
            uint64_t offset;
        };
        AliasDescriptor* Alias = nullptr;
    };

#if false
    enum class Type : uint8_t
    {
        TEXTURE_1D,
        TEXTURE_2D,
        TEXTURE_3D,
    } type = Type::TEXTURE_2D;
    Format format = Format::UNKNOWN;
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    uint32_t array_size = 1;
    uint32_t mip_levels = 1;
    uint32_t sample_count = 1;
    ClearValue clear = {};
    Swizzle swizzle;
    Usage usage = Usage::DEFAULT;
    BindFlag bind_flags = BindFlag::NONE;
    ResourceMiscFlag misc_flags = ResourceMiscFlag::NONE;
    ResourceState layout = ResourceState::SHADER_RESOURCE;
#endif

    struct TextureDescriptor
    {
        const char* DebugName = "";
        TextureType Type = TextureType::Texture2D;
        rhi::Format Format = rhi::Format::UNKNOWN;

        uint32_t Width = 1;
        uint32_t Height = 1;

        union
        {
            uint16_t ArraySize = 1;
            uint16_t Depth;
        };
        uint16_t MipLevels = 1;
        uint16_t SampleCount = 1;

        rhi::ClearValue ClearValue = {};
        Usage Usage = Usage::Default;

        BindingFlags BindingFlags = BindingFlags::ShaderResource;
        ResourceMiscFlags MiscFlags = ResourceMiscFlags::None;
        ResourceStates InitialState = ResourceStates::ShaderResource;

        // COMBINE with Buffer
        struct AliasDescriptor
        {
            BufferHandle Buffer;
            uint64_t Offset;
        } Alias = {};
    };

    struct SwapChainDescriptor
    {
        uint32_t Width = 0;
        uint32_t Height = 0;
        rhi::Format Format = rhi::Format::R10G10B10A2_UNORM;
        rhi::ClearValue OptmizedClearValue =
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

    struct FenceHandle
    {
        uint64_t value = 0;
        CommandQueueType queue_type = CommandQueueType::Graphics;
    };

    struct StagingBlock
    {
        void* data_ptr = nullptr;
        uint64_t size = 0;

        // Internal data
        rhi::BufferHandle buffer_handle;
        uint64_t gpu_offset;
    };

    struct GpuBarrier
    {
        struct BufferBarrier
        {
            BufferHandle buffer;
            ResourceStates before_state;
            ResourceStates after_state;

            uint64_t offset;
            uint64_t size;
        };

        struct TextureBarrier
        {
            TextureHandle texture;
            ResourceStates before_state;
            ResourceStates after_state;
            int mip;
            int slice;
        };

        struct GlobalBarrier
        {
            ResourceStates before_state;
            ResourceStates after_state;
        };

        std::variant<BufferBarrier, TextureBarrier, GlobalBarrier> Data;

        static GpuBarrier CreateGlobal(ResourceStates before_state, ResourceStates after_state)
        {
            GpuBarrier::GlobalBarrier g = {
                .before_state = before_state,
                .after_state = after_state
            };

            GpuBarrier barrier = {
                .Data = g
            };

            return barrier;
        }

        static GpuBarrier CreateTexture(
            TextureHandle texture,
            ResourceStates before_state,
            ResourceStates after_state,
            int mip = -1,
            int slice = -1)
        {
            GpuBarrier::TextureBarrier t = {
                .texture = texture,
                .before_state = before_state,
                .after_state = after_state,
                .mip = mip,
                .slice = slice
            };

            GpuBarrier barrier = {
                .Data = t
            };
            return barrier;
        }

        static GpuBarrier CreateBuffer(
            BufferHandle buffer,
            ResourceStates before_state,
            ResourceStates after_state,
            uint32_t size = ~0,
            uint32_t offset = 0)
        {
            GpuBarrier::BufferBarrier b = {
                .buffer = buffer,
                .before_state = before_state,
                .after_state = after_state,
                .offset = offset,
                .size = size
            };

            GpuBarrier barrier = {
                .Data = b
            };

            return barrier;
        }
    };
#pragma endregion
}
