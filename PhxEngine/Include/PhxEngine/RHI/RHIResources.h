#pragma once

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

#pragma region Enums
    enum class RHICommandQueueType : uint8_t
    {
        Graphics = 0,
        Compute,
        Copy,

        Count
    };

    enum class RHIPrimitiveType : uint8_t
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

    enum class RHIBindingFlags
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
    PHXRHI_ENUM_CLASS_FLAG_OPERATORS(RHIBindingFlags);

    enum class RHITextureDimension : uint8_t
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

    enum class RHIResourceStates : uint32_t
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
    };
    PHXRHI_ENUM_CLASS_FLAG_OPERATORS(RHIResourceStates)

    enum class RHISubresouceType
    {
        SRV,
        UAV,
        RTV,
        DSV,
    };

    enum class RHIBlendFactor : uint8_t
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

    enum class RHIBlendOp : uint8_t
    {
        Add = 1,
        Subrtact = 2,
        ReverseSubtract = 3,
        Min = 4,
        Max = 5
    };

    enum class RHIColorMask : uint8_t
    {
        // These values are equal to their counterparts in DX11, DX12, and Vulkan.
        Red = 1,
        Green = 2,
        Blue = 4,
        Alpha = 8,
        All = 0xF
    };

    enum class RHIStencilOp : uint8_t
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

    enum class RHIComparisonFunc : uint8_t
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

    enum class RHIRasterFillMode : uint8_t
    {
        Solid,
        Wireframe,

        // Vulkan names
        Fill = Solid,
        Line = Wireframe
    };

    enum class RHIRasterCullMode : uint8_t
    {
        Back,
        Front,
        None
    };

#pragma endregion

    class IRHIResource
    {
    public:
        virtual unsigned long AddRef() = 0;
        virtual unsigned long Release() = 0;

        // Non-copyable and non-movable
        IRHIResource(const IRHIResource&) = delete;
        IRHIResource(const IRHIResource&&) = delete;
        IRHIResource& operator=(const IRHIResource&) = delete;
        IRHIResource& operator=(const IRHIResource&&) = delete;

    protected:
        IRHIResource() = default;
        virtual ~IRHIResource() = default;
    };

    struct RHIColor
    {
        float R;
        float G;
        float B;
        float A;

        RHIColor()
            : R(0.f), G(0.f), B(0.f), A(0.f)
        { }

        RHIColor(float c)
            : R(c), G(c), B(c), A(c)
        { }

        RHIColor(float r, float g, float b, float a)
            : R(r), G(g), B(b), A(a) { }

        bool operator ==(const RHIColor& other) const { return R == other.R && G == other.G && B == other.B && A == other.A; }
        bool operator !=(const RHIColor& other) const { return !(*this == other); }
    };

    union RHIClearValue
    {
        // TODO: Change to be a flat array
        // float Colour[4];
        RHIColor Colour;
        struct ClearDepthStencil
        {
            float Depth;
            uint32_t Stencil;
        } DepthStencil;
    };

    class IRHIRHIResource
    {
    protected:
        IRHIRHIResource() = default;
        virtual ~IRHIRHIResource() = default;

    public:
        virtual unsigned long AddRef() = 0;
        virtual unsigned long Release() = 0;

        // Non-copyable and non-movable
        IRHIRHIResource(const IRHIRHIResource&) = delete;
        IRHIRHIResource(const IRHIRHIResource&&) = delete;
        IRHIRHIResource& operator=(const IRHIRHIResource&) = delete;
        IRHIRHIResource& operator=(const IRHIRHIResource&&) = delete;
    };


    struct RHIViewportDesc
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

    class IRHIViewport : public IRHIRHIResource
    {
    public:
        virtual const RHIViewportDesc& GetDesc() const = 0;
        virtual ~IRHIViewport() = default;

    };

    using RHIViewportHandle = RefCountPtr<IRHIViewport>;

    struct RHIShaderDesc
    {
        ShaderStage Stage = ShaderStage::None;
        std::string EntryPoint = "main";
        std::string DebugName = "";
    };

    class IRHIShader : public IRHIRHIResource
    {
    public:
        virtual ~IRHIShader() = default;

        virtual const RHIShaderDesc& GetDesc() const = 0;
        virtual const std::vector<uint8_t>& GetByteCode() const = 0;
    };

    using RHIShaderHandle = RefCountPtr<IRHIShader>;

    struct RHIVertexAttributeDesc
    {
        static const uint32_t SAppendAlignedElement = 0xffffffff; // automatically figure out AlignedByteOffset depending on Format

        std::string SemanticName;
        uint32_t SemanticIndex = 0;
        RHIFormat Format = RHIFormat::UNKNOWN;
        uint32_t InputSlot = 0;
        uint32_t AlignedByteOffset = SAppendAlignedElement;
        bool IsInstanced = false;
    };

    class IRHIInputLayout : public IRHIResource
    {
    public:
        [[nodiscard]] virtual uint32_t GetNumAttributes() const = 0;
        [[nodiscard]] virtual const RHIVertexAttributeDesc* GetAttributeDesc(uint32_t index) const = 0;
    };

    using RHIInputLayoutHandle = RefCountPtr<IRHIInputLayout>;

    struct RHITextureDesc
    {
        RHIBindingFlags BindingFlags = RHIBindingFlags::ShaderResource;
        RHITextureDimension Dimension = RHITextureDimension::Unknown;
        RHIResourceStates InitialState = RHIResourceStates::Common;

        RHIFormat Format = RHIFormat::UNKNOWN;

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

    class IRHITexture : public IRHIResource
    {
    public:
        virtual RHITextureDesc& GetDesc() const = 0;

        virtual ~IRHITexture() = default;
    };
    using RHITextureHandle = RefCountPtr<IRHITexture>;

#pragma region PipelineStateDesc
    // -- Pipeline State objects ---
    struct RHIBlendRenderState
    {
        struct RenderTarget
        {
            bool        BlendEnable = false;
            RHIBlendFactor SrcBlend = RHIBlendFactor::One;
            RHIBlendFactor DestBlend = RHIBlendFactor::Zero;
            RHIBlendOp    BlendOp = RHIBlendOp::Add;
            RHIBlendFactor SrcBlendAlpha = RHIBlendFactor::One;
            RHIBlendFactor DestBlendAlpha = RHIBlendFactor::Zero;
            RHIBlendOp    BlendOpAlpha = RHIBlendOp::Add;
            RHIColorMask   ColorWriteMask = RHIColorMask::All;
        };

        RenderTarget Targets[cMaxRenderTargets];
        bool alphaToCoverageEnable = false;
    };

    struct RHIDepthStencilRenderState
    {
        struct StencilOpDesc
        {
            RHIStencilOp FailOp = RHIStencilOp::Keep;
            RHIStencilOp DepthFailOp = RHIStencilOp::Keep;
            RHIStencilOp PassOp = RHIStencilOp::Keep;
            RHIComparisonFunc StencilFunc = RHIComparisonFunc::Always;

            constexpr StencilOpDesc& setFailOp(RHIStencilOp value) { this->FailOp = value; return *this; }
            constexpr StencilOpDesc& setDepthFailOp(RHIStencilOp value) { this->DepthFailOp = value; return *this; }
            constexpr StencilOpDesc& setPassOp(RHIStencilOp value) { this->PassOp = value; return *this; }
            constexpr StencilOpDesc& setStencilFunc(RHIComparisonFunc value) { this->StencilFunc = value; return *this; }
        };

        bool            DepthTestEnable = true;
        bool            DepthWriteEnable = true;
        RHIComparisonFunc  DepthFunc = RHIComparisonFunc::Less;
        bool            StencilEnable = false;
        uint8_t         StencilReadMask = 0xff;
        uint8_t         StencilWriteMask = 0xff;
        uint8_t         stencilRefValue = 0;
        StencilOpDesc   FrontFaceStencil;
        StencilOpDesc   BackFaceStencil;
    };

    struct RHIRasterRenderState
    {
        RHIRasterFillMode FillMode = RHIRasterFillMode::Solid;
        RHIRasterCullMode CullMode = RHIRasterCullMode::Back;
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

    struct RHIGraphicsPipelineDesc
    {
        RHIPrimitiveType PrimType = RHIPrimitiveType::TriangleList;

        RHIShaderHandle VertexShader;
        RHIShaderHandle HullShader;
        RHIShaderHandle DomainShader;
        RHIShaderHandle GeometryShader;
        RHIShaderHandle PixelShader;

        RHIBlendRenderState BlendRenderState = {};
        RHIDepthStencilRenderState DepthStencilRenderState = {};
        RHIRasterRenderState RasterRenderState = {};

        std::vector<RHITextureHandle> RenderTargets;
        RHITextureHandle DepthStencilTexture;

        RHIInputLayoutHandle InputLayout;

        uint32_t SampleCount = 1;
        uint32_t SampleQuality = 0;
    };
    // -- Pipeline State Objects End ---
#pragma endregion

    class IRHIGraphicsPipeline : public IRHIResource
    {
    public:
        virtual ~IRHIGraphicsPipeline() = default;

        virtual const RHIGraphicsPipelineDesc& GetDesc() const = 0;
    };

    using RHIGraphicsPipelineHandle = std::shared_ptr<IRHIGraphicsPipeline>;
    
    struct RHIRenderPassDesc
    {
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

            RHITextureHandle Texture;
            int Subresource = -1;

            enum class StoreOpType
            {
                Store,
                DontCare,
            } StoreOp = StoreOpType::Store;

            RHIResourceStates InitialLayout = RHIResourceStates::Unknown;	// layout before the render pass
            RHIResourceStates SubpassLayout = RHIResourceStates::Unknown;	// layout within the render pass
            RHIResourceStates FinalLayout = RHIResourceStates::Unknown;   // layout after the render pass
        };

        enum Flags
        {
            None = 0,
            AllowUavWrites = 1 << 0,
        };

        uint32_t Flags = Flags::None;
        std::vector<RenderPassAttachment> Attachments;
    };

    class IRHIRenderPass
    {
    public:
        virtual const RHIRenderPassDesc& GetDesc() const = 0;

        virtual ~IRHIRenderPass() = default;
    };
    using RHIRenderPassHandle = RefCountPtr<IRHIRenderPass>;

    class IRHICommandList : public IRHIResource
    {
    public:
        virtual ~IRHICommandList() = default;

        virtual RHIShaderHandle BeginScopedMarker(std::string name) = 0;
        virtual void BeginMarker(std::string name) = 0;
        virtual void EndMarker() = 0;

        virtual void BeginRenderPassBackBuffer() = 0;
        virtual void BeginRenderPass(RHIResourceStates renderPass) = 0;
        virtual void EndRenderPass() = 0;

        virtual void SetGraphicsPipeline(RHIGraphicsPipelineHandle pipeline) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0) = 0;
        virtual void DrawIndexed(
            uint32_t indexCount,
            uint32_t instanceCount = 1,
            uint32_t startIndex = 0,
            int32_t baseVertex = 0,
            uint32_t startInstance = 0) = 0;
    };

    class RHIScopedMarker
    {
    public:
        RHIScopedMarker(IRHICommandList* context)
            : m_commandList(context) {}

        ~RHIScopedMarker() { this->m_commandList->EndMarker(); }

    private:
        IRHICommandList* m_commandList;
    };

    class IRHIFrameRenderCtx
    {
    public:
        virtual ~IRHIFrameRenderCtx() = default;

        virtual IRHICommandList* BeginCommandRecording(RHICommandQueueType QueueType = RHICommandQueueType::Graphics) = 0;
        virtual uint64_t SubmitCommands(Core::Span<IRHICommandList*> commands) = 0;
        virtual RHITextureHandle GetBackBuffer() = 0;
    };
}