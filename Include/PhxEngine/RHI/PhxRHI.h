#pragma once

#include <stdint.h>
#include <optional>
#include <string>
#include <vector>

#include <PhxEngine/RHI/RefCountPtr.h>

namespace PhxEngine::RHI
{
    static constexpr uint32_t c_MaxRenderTargets = 8;
    static constexpr uint32_t c_MaxViewports = 16;
    static constexpr uint32_t c_MaxVertexAttributes = 16;
    static constexpr uint32_t c_MaxBindingLayouts = 5;
    static constexpr uint32_t c_MaxBindingsPerLayout = 128;
    static constexpr uint32_t c_MaxVolatileConstantBuffersPerLayout = 6;
    static constexpr uint32_t c_MaxVolatileConstantBuffers = 32;
    static constexpr uint32_t c_MaxPushConstantSize = 128;      // D3D12: root signature is 256 bytes max., Vulkan: 128 bytes of push constants guaranteed

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

    enum class EFormat : uint8_t
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

    enum ResourceStates : uint32_t
    {
        Unknown = 0,
        Common = 0x00000001,
        ConstantBuffer = 0x00000002,
        VertexBuffer = 0x00000004,
        IndexBuffer = 0x00000008,
        IndirectArgument = 0x00000010,
        ShaderResource = 0x00000020,
        UnorderedAccess = 0x00000040,
        RenderTarget = 0x00000080,
        DepthWrite = 0x00000100,
        DepthRead = 0x00000200,
        StreamOut = 0x00000400,
        CopyDest = 0x00000800,
        CopySource = 0x00001000,
        ResolveDest = 0x00002000,
        ResolveSource = 0x00004000,
        Present = 0x00008000,
        AccelStructRead = 0x00010000,
        AccelStructWrite = 0x00020000,
        AccelStructBuildInput = 0x00040000,
        AccelStructBuildBlas = 0x00080000,
        ShadingRateSurface = 0x00100000,
    };

    enum EShaderType : uint16_t
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

    struct SwapChainDesc
    {
        uint32_t Width;
        uint32_t Height;
        uint32_t BufferCount = 3;
        EFormat Format = EFormat::UNKNOWN;
        bool VSync = false;
        void* WindowHandle;
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

        RenderTarget Targets[c_MaxRenderTargets];
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
        ComparisonFunc  depthFunc = ComparisonFunc::Less;
        bool            StencilEnable = false;
        uint8_t         StencilReadMask = 0xff;
        uint8_t         StencilWriteMask = 0xff;
        uint8_t         stencilRefValue = 0;
        StencilOpDesc   FrontFaceStencil;
        StencilOpDesc   BackFaceStencil;
    };

    struct RasterRenderState
    {
        RasterFillMode fillMode = RasterFillMode::Solid;
        RasterCullMode cullMode = RasterCullMode::Back;
        bool FrontCounterClockwise = false;
        bool DepthClipEnable = false;
        bool scissorEnable = false;
        bool MultisampleEnable = false;
        bool AntialiasedLineEnable = false;
        int DepthBias = 0;
        float DepthBiasClamp = 0.f;
        float SlopeScaledDepthBias = 0.f;

        // Extended rasterizer state supported by Maxwell
        // In D3D11, use NvAPI_D3D11_CreateRasterizerState to create such rasterizer state.
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
        virtual unsigned long AddRef() = 0;
        virtual unsigned long Release() = 0;

        // Non-copyable and non-movable
        IResource(const IResource&) = delete;
        IResource(const IResource&&) = delete;
        IResource& operator=(const IResource&) = delete;
        IResource& operator=(const IResource&&) = delete;
    };

    struct ShaderDesc
    {
        EShaderType ShaderType = EShaderType::None;
        std::string DebugName = "";
    };

    class IShader : public IResource
    {
    public:
        virtual ~IShader() = default;

        virtual const ShaderDesc& GetDesc() const = 0;
        virtual const std::vector<uint8_t>& GetByteCode() const = 0;
    };

    typedef RefCountPtr<IShader> ShaderHandle;

    struct VertexAttributeDesc
    {
        std::string Name;
        EFormat Format;
        uint32_t ArraySize;
        uint32_t BufferIndex;
        uint32_t Offset;
        // note: for most APIs, all strides for a given bufferIndex must be identical
        uint32_t ElementStride;
        bool IsInstanced;
    };

    class IInputLayout : public IResource
    {
    public:
        [[nodiscard]] virtual uint32_t GetNumAttributes() const = 0;
        [[nodiscard]] virtual const VertexAttributeDesc* GetAttributeDesc(uint32_t index) const = 0;
    };

    typedef RefCountPtr<IInputLayout> InputLayoutHandle;

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

    // Shader Binding Layout
    struct ShaderParameterLayout
    {
        EShaderType Visibility = EShaderType::None;
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
            desc.MaxAnisotropy = maxAnisotropy;
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

        ShaderParameterLayout ShaderParameters;

        ShaderHandle VertexShader;
        ShaderHandle HullShader;
        ShaderHandle DomainShader;
        ShaderHandle GeometryShader;
        ShaderHandle PixelShader;
        
        BlendRenderState BlendRenderState = {};
        DepthStencilRenderState DepthStencilRenderState = {};
        RasterRenderState RasterRenderState = {};

        std::vector<EFormat> RtvFormats;
        std::optional<EFormat> DsvFormat;

        uint32_t SampleCount = 1;
        uint32_t SampleQuality = 0;
    };

    class IGraphicsPSO : public IResource
    {
    public:
        virtual ~IGraphicsPSO() = default;

        virtual const GraphicsPSODesc& GetDesc() const = 0;
    };

    typedef RefCountPtr<IGraphicsPSO> GraphicsPSOHandle;

    struct SubresourceData
    {
        const void* pData = nullptr;
        uint32_t rowPitch = 0;
        uint32_t slicePitch = 0;
    };

    struct TextureDesc
    {
        TextureDimension Dimension = TextureDimension::Unknown;
        EFormat Format = EFormat::UNKNOWN;

        uint32_t Width;
        uint32_t Height;

        union
        {
            uint16_t ArraySize = 1;
            uint16_t Depth;
        };

        uint16_t MipLevels = 0;

        std::optional<Color> OptmizedClearValue;
        std::string DebugName;
    };

    class ITexture : public IResource
    {
    public:
        virtual ~ITexture() = default;
        virtual const TextureDesc& GetDesc() const = 0;
    };

    using TextureHandle = RefCountPtr<ITexture>;

    class IBuffer : public IResource
    {
    public:
        virtual ~IBuffer() = default;
    };

    using BufferHandle = RefCountPtr<IBuffer>;

    struct CommandListDesc
    {
        CommandQueueType QueueType = CommandQueueType::Graphics;
    };

    class ScopedMarker;

    class ICommandList : public IResource
    {
    public:
        virtual ~ICommandList() = default;

        virtual void Open() = 0;
        virtual void Close() = 0;

        virtual ScopedMarker BeginScropedMarker(std::string name) = 0;
        virtual void BeginMarker(std::string name) = 0;
        virtual void EndMarker() = 0;

        virtual void TransitionBarrier(ITexture* texture, ResourceStates beforeState, ResourceStates afterState) = 0;
        virtual void TransitionBarrier(IBuffer* buffer, ResourceStates beforeState, ResourceStates afterState) = 0;
        virtual void ClearTextureFloat(ITexture* texture, Color const& clearColour) = 0 ;
        virtual void ClearDepthStencilTexture(ITexture* depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil) = 0;

        virtual void WriteTexture(TextureHandle texture, uint32_t firstSubResource, size_t numSubResources, SubresourceData* pSubResourceData) = 0;
    };

    using CommandListHandle = RefCountPtr<ICommandList>;

    class ScopedMarker
    {
    public:
        ScopedMarker(ICommandList* context)
            : m_commandList(context) {}

        ~ScopedMarker() { this->m_commandList->EndMarker(); }

    private:
        ICommandList* m_commandList;
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
        virtual GraphicsPSOHandle CreateGraphicsPSOHandle(GraphicsPSODesc const& desc) = 0;

        virtual TextureHandle CreateDepthStencil(TextureDesc const& desc) = 0;
        virtual TextureHandle CreateTexture(TextureDesc const& desc) = 0;

        // -- Create Functions End ---
        virtual ITexture* GetBackBuffer() = 0;

        virtual void Present() = 0;
        virtual void WaitForIdle() = 0;

        virtual uint64_t ExecuteCommandLists(
            ICommandList* const* pCommandLists,
            size_t numCommandLists,
            CommandQueueType executionQueue = CommandQueueType::Graphics) = 0;

        virtual uint64_t ExecuteCommandLists(
            ICommandList* const* pCommandLists,
            size_t numCommandLists,
            bool waitForCompletion,
            CommandQueueType executionQueue = CommandQueueType::Graphics) = 0;

        // Front-end for executeCommandLists(..., 1) for compatibility and convenience
        uint64_t ExecuteCommandLists(ICommandList* commandList, CommandQueueType executionQueue = CommandQueueType::Graphics)
        {
            return this->ExecuteCommandLists(&commandList, 1, executionQueue);
        }

        uint64_t ExecuteCommandLists(ICommandList* commandList, bool waitForCompletion, CommandQueueType executionQueue = CommandQueueType::Graphics)
        {
            return this->ExecuteCommandLists(&commandList, 1, waitForCompletion, executionQueue);
        }

    };
}