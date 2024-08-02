#pragma once

#include "Core/phxEnumClassFlags.h"
#include "Core/phxHandle.h"
#include "Core/phxSpan.h"
#include "Core/phxPlatform.h"
#include "Core/phxMemory.h"

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <thread>

namespace phx::rhi
{
#pragma region Constants

    using DescriptorIndex = uint32_t;

    static constexpr DescriptorIndex kInvalidDescriptorIndex = ~0u;

    static constexpr uint32_t kMaxRenderTargets = 8;
    static constexpr uint32_t kMaxViewports = 16;
    static constexpr uint32_t kMaxVertexAttributes = 16;
    static constexpr uint32_t kMaxBindingLayouts = 5;
    static constexpr uint32_t kMaxBindingsPerLayout = 128;
    static constexpr uint32_t kMaxVolatileConstantBuffersPerLayout = 6;
    static constexpr uint32_t kMaxVolatileConstantBuffers = 32;
    static constexpr uint32_t kMaxPushConstantSize = 128;      // D3D12: root signature is 256 bytes max., Vulkan: 128 bytes of push constants guaranteed
#pragma endregion

#pragma region Generic Structures


    struct DeviceCapabilities
    {
        union
        {
            uint8_t Flags = 0;
            struct
            {
                uint8_t RTVTArrayIndexWithoutGS : 1;
                uint8_t RayTracing : 1;
                uint8_t RenderPass : 1;
                uint8_t RayQuery : 1;
                uint8_t VariableRateShading : 1;
                uint8_t MeshShading : 1;
                uint8_t CreateNoteZeroed : 1;
                uint8_t Bindless : 1;
            };
        };
    };

    struct SubresourceData
    {
        const void* pData = nullptr;
        uint32_t rowPitch = 0;
        uint32_t slicePitch = 0;
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
#pragma endregion
#pragma region Enums

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
    PHX_ENUM_CLASS_FLAGS(ShaderStage)

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
#pragma endregion

#pragma region Resources

    struct Texture;
    using TextureHandle = Handle<Texture>;

    using CommandListHandle = uint32_t;

    class GfxDevice
    {
    public:
        inline static GfxDevice* Ptr = nullptr;
        virtual ~GfxDevice() = default;

    public:
        virtual void ResizeSwapchain(Rect const& size) = 0;

        virtual void SubmitFrame() = 0;
        virtual void WaitForIdle() = 0;

        virtual TextureHandle GetBackBuffer() = 0;

        virtual CommandListHandle BeginContext(CommandQueueType queueType = CommandQueueType::Graphics) = 0;
    };

#pragma endregion

    struct Config
    {
        GraphicsAPI Api = GraphicsAPI::DX12;
        core::WindowHandle Window;
        Rect WindowSize;
        uint8_t BufferCount = 3;
        rhi::Format BufferFormat = rhi::Format::R10G10B10A2_UNORM;

        union
        {
            struct
            {
                bool Fullscreen     : 1;
                bool VSync          : 1;
                bool AllowTearing   : 1;
                bool EnableHDR      : 5;
            };
            uint8_t Flags = 0;
        };
    };

#ifdef _WIN32
    void InitializeWindows(rhi::Config const& config);
#endif

    void Finalize();

}