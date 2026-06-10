#pragma once

#include <PhxEngine/Core/Handle.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Core/EnumUtils.h>
#include <array>

namespace phx::rhi
{
    enum class ShaderFormat : u8
    {
        None,		// Not used
        Hlsl6,		// DXIL
        Spirv,		// SPIR-V
    };
    
    enum class ShaderStage : u8
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

    enum class ColourSpace
    {
        SRGB,
        HDR_LINEAR,
        HDR10_ST2084
    };

    
    enum class Format : u8
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

    enum class ComponentSwizzle : u8
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

    enum class FormatKind : u8
    {
        Integer,
        Normalized,
        Float,
        DepthStencil
    };

    enum class CommandQueueType : u8
    {
        Graphics = 0,
        Compute,
        Copy,

        Count
    };

    constexpr size_t kNumCommandQueues = static_cast<size_t>(CommandQueueType::Count);

    enum class IndexFormat : u8
    { 
        Uint16, Uint32 
    };

    enum class ResourceStates : u32
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

    enum class InputClassification : u8
    {
        PerVertexData,
        PerInstanceData,
    };

    
    enum class PrimitiveType : u8
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

    enum class BlendFactor : u8
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

    enum class EBlendOp : u8
    {
        Add = 1,
        Subrtact = 2,
        ReverseSubtract = 3,
        Min = 4,
        Max = 5
    };

    enum class ColorMask : u8
    {
        // These values are equal to their counterparts in DX11, DX12, and Vulkan.
        Red = 1,
        Green = 2,
        Blue = 4,
        Alpha = 8,
        All = 0xF
    };

    enum class StencilOp : u8
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

    enum class DepthWriteMask : u8
    {
        Zero,	// Disables depth write
        All,	// Enables depth write
    };

    enum class ComparisonFunc : u8
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

    enum class RasterFillMode : u8
    {
        Solid,
        Wireframe,

        // Vulkan names
        Fill = Solid,
        Line = Wireframe
    };

    enum class RasterCullMode : u8
    {
        Back,
        Front,
        None
    };

    enum class FrontFace : u8 
    {
        CounterClockwise, Clockwise
    };

    // identifies the underlying resource type in a binding
    enum class ResourceType : u8
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

    enum class TextureType : u8
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

    enum class SamplerAddressMode : u8
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

    enum class SamplerReductionType : u8
    {
        Standard,
        Comparison,
        Minimum,
        Maximum
    };

    enum class ResourceMiscFlags
    {
        None = 0,
        TextureCube = PHX_BIT(0),
        IndirectArgs = PHX_BIT(1),
        BufferRaw = PHX_BIT(2),
        BufferStructured = PHX_BIT(3),
        RayTracing = PHX_BIT(4),
        AliasBuffer = PHX_BIT(5),
        AliasTexture_NonRtDs = PHX_BIT(6),
        AliasTexture_RtDs = PHX_BIT(7),
        Sparse = PHX_BIT(8),
        HasCounter = PHX_BIT(9),
        TypedFormatCasting = PHX_BIT(10),	// enable casting formats between same type and different modifiers: eg. UNORM -> SRGB
        TypelessFormatCasting = PHX_BIT(11),  // enable casting formats to other formats that have the s
        DescriptorTable = PHX_BIT(12),
        TransientAttachment = PHX_BIT(13),
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

    union ClearValue
    {
        std::array<float, 4> colour;
        struct ClearDepthStencil
        {
            float depth;
            uint32_t stencil;
        } depth_stencil;
    };

    struct RhiCapabilities
    {
        bool mesh_shaders;
        bool ray_query;
        bool acceleration_structures;
        bool deferred_host_operations;
        bool shader_object;
        bool calibrated_timestamps;
        bool multi_draw ;
    };

    struct Swapchain;
    using SwapchainHandle = Handle<Swapchain>;
    struct SwapchainDesc
    {
        rhi::ClearValue clear_value = {
            .colour = {0.0f, 0.0f, 0.0f, 1.0f}
        };

        uint32_t width = 0;
        uint32_t height = 0;
        rhi::Format format = rhi::Format::R10G10B10A2_UNORM;
        rhi::Format depth_format = rhi::Format::D32;

        union
        {
            struct
            {
                bool fullscreen : 1;
                bool v_sync     : 1;
                bool enable_hdr : 1;
                bool _reserved  : 5;
            };
            uint8_t flags = 0;
        };
    };
}