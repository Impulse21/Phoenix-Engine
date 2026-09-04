#pragma once

#include <PhxEngine/Core/Handle.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Core/EnumUtils.h>
#include <PhxEngine/Platform/OSWindow.h>

#include <array>

namespace phx::rhi
{
    constexpr uint32_t kMaxRenderTargets = 8;

    enum class ShaderFormat : u8
    {
        None,		// Not used
        Hlsl6,		// DXIL
        Spirv,		// SPIR-V
    };
    
    enum class PipelineType 
    {
        Graphics,
        Compute,
        RayTracing,
        Count,
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

    // Opaque handle to a transfer-queue submission made via SubmitUpload.
    // 0 means "no submission yet" / "already known complete".
    using UploadTicket = u64;

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

    // Coarse GPU work domains for Barrier() — not a per-resource state, just
    // which kind of work has to finish (src) before which kind of work is
    // allowed to start (dst). Narrow it when you know the domains involved
    // to avoid stalling work that was never going to touch the same data;
    // default to All when unsure.
    enum class BarrierStage : u32
    {
        None     = 0,
        Graphics = 1 << 0,
        Compute  = 1 << 1,
        Transfer = 1 << 2,
        All      = Graphics | Compute | Transfer,
    };

    PHX_ENUM_CLASS_FLAGS(BarrierStage);

    struct Swizzle
    {
        ComponentSwizzle r = ComponentSwizzle::R;
        ComponentSwizzle g = ComponentSwizzle::G;
        ComponentSwizzle b = ComponentSwizzle::B;
        ComponentSwizzle a = ComponentSwizzle::A;
    };
    
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
        bool unified_image_layouts;

        // Gates VK_DYNAMIC_STATE_POLYGON_MODE_EXT (wireframe/solid toggle
        // without a second pipeline) — VK_EXT_extended_dynamic_state3 is a
        // real (non-promoted) extension, not guaranteed on every device.
        bool extended_dynamic_state3;
    };

    // -- Pipeline State objects ---
    struct BlendRenderState
    {
        struct RenderTarget
        {
            bool        blend_enable = false;
            BlendFactor src_blend = BlendFactor::One;
            BlendFactor dest_blend = BlendFactor::Zero;
            EBlendOp    blend_op = EBlendOp::Add;
            BlendFactor src_blend_alpha = BlendFactor::One;
            BlendFactor dest_blend_alpha = BlendFactor::Zero;
            EBlendOp    blend_op_alpha = EBlendOp::Add;
            ColorMask   color_write_mask = ColorMask::All;
        };

        RenderTarget targets[kMaxRenderTargets];
        bool         alpha_to_coverage_enable = false;
    };

    struct DepthStencilRenderState
    {
        bool           depth_enable = false;
        DepthWriteMask depth_write_mask = DepthWriteMask::Zero;
        ComparisonFunc depth_func = ComparisonFunc::Never;
        bool           stencil_enable = false;
        uint8_t        stencil_read_mask = 0xff;
        uint8_t        stencil_write_mask = 0xff;

        struct DepthStencilOp
        {
            StencilOp      stencil_fail_op = StencilOp::Keep;
            StencilOp      stencil_depth_fail_op = StencilOp::Keep;
            StencilOp      stencil_pass_op = StencilOp::Keep;
            ComparisonFunc stencil_func = ComparisonFunc::Never;
        };

        DepthStencilOp front_face = {};
        DepthStencilOp back_face = {};
        bool           depth_bounds_test_enable = false;
    };

    struct RasterRenderState
    {
        RasterFillMode fill_mode = RasterFillMode::Solid;
        RasterCullMode cull_mode = RasterCullMode::Back;
        bool           front_counter_clockwise = false;
        bool           depth_clip_enable = false;
        bool           scissor_enable = false;
        bool           multisample_enable = false;
        bool           antialiased_line_enable = false;
        int            depth_bias = 0;
        float          depth_bias_clamp = 0.f;
        float          slope_scaled_depth_bias = 0.f;

        uint8_t        forced_sample_count = 0;
        bool           programmable_sample_positions_enable = false;
        bool           conservative_raster_enable = false;
        bool           quad_fill_enable = false;
        char           sample_positions_x[16]{};
        char           sample_positions_y[16]{};
    };

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
        phx::Span<Format> color_attachments = {};
        Format            depth_stencil_format = Format::UNKNOWN;
        uint32_t          sample_count = 1;
    };
    // -- Pipeline State Objects End ---

    using DescriptorIndex = uint32_t;
    constexpr DescriptorIndex kInvalidDescriptorIndex = ~0u;

    // The engine has exactly one viewport, owned directly by the RHI context
    // rather than pooled/handled like other resources — see rhi::Initialize.
    struct ViewportDesc
    {
        platform::OSWindowHandle window_handle;
        rhi::ClearValue clear_value = {
            .colour = {0.0f, 0.0f, 0.0f, 1.0f}
        };

        uint32_t width = 0;
        uint32_t height = 0;
        rhi::Format format = rhi::Format::R10G10B10A2_UNORM;
        rhi::Format depth_format = rhi::Format::D32;

        bool fullscreen : 1;
        bool v_sync     : 1;
        bool enable_hdr : 1;
        bool _reserved  : 5;
    };

    // A transient recording session for one queue, handed out by
    // BeginCommandRecording for the duration of a single use — not a
    // persistent resource with a Create/Destroy lifecycle. Backends stash
    // whatever they need to find the real command buffer in internal_state.
    struct CommandBuffer
    {
        void* internal_state = nullptr;

        bool IsValid() const { return internal_state != nullptr; }
    };

    // Raw GPU memory — no handle, no descriptor binding. gpu_address is a
    // VK_KHR_buffer_device_address pointer: embed it directly in push
    // constants or inside another buffer's contents. cpu_ptr is non-null
    // only for host-visible allocations (Upload/ReadBack).
    struct GpuAllocation
    {
        void* internal_state = nullptr; // opaque backend data — only GpuFree needs this
        void* cpu_ptr        = nullptr;
        u64   gpu_address    = 0;
        u32   size           = 0;

        bool IsValid() const { return gpu_address != 0; }
    };

    enum class GpuMemoryUsage : u8
    {
        DeviceLocal, // GPU-only; fastest GPU access
        Upload,      // host-visible + coherent, mapped for CPU writes
        ReadBack,    // host-visible, mapped for CPU reads of GPU-written data
    };

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
    
    struct Texture;
    using TextureHandle = Handle<Texture>;
    struct TextureDescriptor
    {
        const char* debug_name      = "";
        TextureType texture_type    = TextureType::Texture2D;
        rhi::Format format          = rhi::Format::UNKNOWN;

        u32         width   = 1;
        u32         height  = 1;

        union
        {
            u16 array_size = 1;
            u16 depth;
        };

        u16 mip_levels      = 1;
        u16 sample_count    = 1;

        rhi::ClearValue clear_value = {};
        Usage           usage       = Usage::Default;

        BindingFlags        binding_flags   = BindingFlags::ShaderResource;
        ResourceMiscFlags   misc_flags      = ResourceMiscFlags::None;
        ResourceStates      initial_state   = ResourceStates::ShaderResource;

        // alias is not supported yet
#if false
        // COMBINE with Buffer
        struct AliasDescriptor
        {
            BufferHandle Buffer;
            uint64_t Offset;
        } Alias = {};
#endif
    };

    struct Sampler;
    using SamplerHandle = Handle<Sampler>;
    struct SamplerDescriptor
    {
    };

    struct ShaderModule;
    using ShaderModuleHandle = Handle<ShaderModule>;
    struct ShaderModuleDescriptor
    {
        phx::Span<uint32_t> byte_code;
        bool IsValid() const { return !byte_code.IsEmpty(); }
    };

    struct ShaderStageInfo
    {
        ShaderStage         stage;
        ShaderModuleHandle  module_handle;
        const char*         entry_point;
    };

    struct PipelineState;
    using PipelineStateHandle = Handle<PipelineState>;
    struct PipelineStateDescriptor
    {        
        PipelineType                    type = PipelineType::Graphics;
        Span<ShaderStageInfo>           shader_stages;

        BlendRenderState                blend_state = {};
        DepthStencilRenderState         depth_stencil_state = {};
        RasterRenderState               raster_state = {};

        rhi::PrimitiveType              prim_type = rhi::PrimitiveType::TriangleList;
        phx::Span<VertexBufferBinding>  vertex_buffer_bindings;
        RenderPassInfo                  render_pass_info;
        uint32_t                        patch_control_points = 3;
        uint32_t                        sample_mask = ~0u;
    };

}