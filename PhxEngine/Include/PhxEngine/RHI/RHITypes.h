#pragma once

#include <memory>
#include <string>

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
#pragma region Enums

    enum class Capability
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

    PHXRHI_ENUM_CLASS_FLAG_OPERATORS(Capability);

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

    enum CommandQueueType : uint8_t
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
        HasCounter = 1 << 4,
        IsAliasedResource = 1 << 5,
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

#pragma region Utility Structures
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

    struct NonCopyable
    {
        NonCopyable() = default;
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
    };

#pragma endregion


	struct RHIResource
	{
		std::shared_ptr<void> RHIResource;
		bool IsValid() const { return this->RHIResource != nullptr; }

		virtual ~RHIResource() = default;
	};

	struct GpuResource : public RHIResource
	{
		enum class Type
		{
			Unkown,
			Buffer,
			Texture,
			RTAccelerationStructure,
		} Type;

		constexpr bool IsTexture() const { return this->Type == Type::Texture; }
		constexpr bool IsBuffer() const { return this->Type == Type::Buffer; }
		constexpr bool IsAccelerationStructure() const { return this->Type == Type::RTAccelerationStructure; }
	};

	struct GpuTexture : public GpuResource
	{
		
	};

	struct GpuBuffer : public GpuResource
	{

	};

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

	struct SwapChain : public RHIResource
	{
        SwapChainDesc Desc;

        constexpr const SwapChainDesc& GetDesc() const { return this->Desc; }
	};

    class ScopedMarker;
    class CommandContext : NonCopyable
    {
    public:
        virtual ~CommandContext() = default;

        virtual ScopedMarker BeginScopedMarker(std::string_view name) = 0;
        virtual void BeginMarker(std::string_view name) = 0;
        virtual void EndMarker() = 0;

#if false
        // -- Ray Trace stuff       ---
        // virtual void RTBuildAccelerationStructure(RHI::RTAccelerationStructureHandle accelStructure) = 0;

        // -- Ray Trace Stuff END   ---

        virtual GPUAllocation AllocateGpu(size_t bufferSize, size_t stride) = 0;

        virtual void TransitionBarrier(TextureHandle texture, ResourceStates beforeState, ResourceStates afterState) = 0;
        virtual void TransitionBarrier(BufferHandle buffer, ResourceStates beforeState, ResourceStates afterState) = 0;
        virtual void TransitionBarriers(Core::Span<GpuBarrier> gpuBarriers) = 0;
        virtual void ClearTextureFloat(TextureHandle texture, Color const& clearColour) = 0;
        virtual void ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil) = 0;

        virtual void BeginRenderPassBackBuffer(bool clear = true) = 0;
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

        virtual void ExecuteIndirect(RHI::CommandSignatureHandle commandSignature, RHI::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount = 1) = 0;
        virtual void ExecuteIndirect(RHI::CommandSignatureHandle commandSignature, RHI::BufferHandle args, size_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount) = 0;

        virtual void DrawIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount = 1) = 0;
        virtual void DrawIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount) = 0;

        virtual void DrawIndexedIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount = 1) = 0;
        virtual void DrawIndexedIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount) = 0;

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

        template<typename T>
        void WriteBuffer(BufferHandle buffer, Core::Span<T> data, uint64_t destOffsetBytes = 0)
        {
            this->WriteBuffer(buffer, data.begin(), sizeof(T) * data.Size(), destOffsetBytes);
        }

        // TODO: Take ownership of the data
        virtual void WriteBuffer(BufferHandle buffer, const void* Data, size_t dataSize, uint64_t destOffsetBytes = 0) = 0;

        virtual void CopyBuffer(BufferHandle dst, uint64_t dstOffset, BufferHandle src, uint64_t srcOffset, size_t sizeInBytes) = 0;

        virtual void WriteTexture(TextureHandle texture, uint32_t firstSubResource, size_t numSubResources, SubresourceData* pSubResourceData) = 0;
        virtual void WriteTexture(TextureHandle texture, uint32_t arraySlice, uint32_t mipLevel, const void* Data, size_t rowPitch, size_t depthPitch) = 0;

        virtual void SetGraphicsPipeline(GraphicsPipelineHandle graphisPSO) = 0;
        virtual void SetViewports(Viewport* viewports, size_t numViewports) = 0;
        virtual void SetScissors(Rect* scissor, size_t numScissors) = 0;
        virtual void SetRenderTargets(std::vector<TextureHandle> const& renderTargets, TextureHandle depthStencil) = 0;

        // -- Comptute Stuff ---
        virtual void SetComputeState(ComputePipelineHandle state) = 0;
        virtual void Dispatch(uint32_t groupsX, uint32_t groupsY = 1, uint32_t groupsZ = 1) = 0;
        virtual void DispatchIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, uint32_t maxCount = 1) = 0;
        virtual void DispatchIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount) = 0;

        // -- Mesh Stuff ---
        virtual void SetMeshPipeline(MeshPipelineHandle meshPipeline) = 0;
        virtual void DispatchMesh(uint32_t groupsX, uint32_t groupsY = 1u, uint32_t groupsZ = 1u) = 0;
        virtual void DispatchMeshIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, uint32_t maxCount = 1) = 0;
        virtual void DispatchMeshIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount) = 0;

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
            this->BindDynamicIndexBuffer(indexBufferData.size(), indexFormat, indexBufferData.Data());
        }

        /**
         * Set dynamic structured buffer contents.
         */
        virtual void BindDynamicStructuredBuffer(uint32_t rootParameterIndex, size_t numElements, size_t elementSize, const void* bufferData) = 0;

        template<typename T>
        void BindDynamicStructuredBuffer(uint32_t rootParameterIndex, std::vector<T> const& bufferData)
        {
            this->BindDynamicStructuredBuffer(rootParameterIndex, bufferData.size(), sizeof(T), bufferData.Data());
        }

        virtual void BindStructuredBuffer(size_t rootParameterIndex, BufferHandle buffer) = 0;

        virtual void BindDynamicDescriptorTable(size_t rootParameterIndex, Core::Span<TextureHandle> textures) = 0;
        void BindDynamicUavDescriptorTable(
            size_t rootParameterIndex,
            Core::Span<BufferHandle> buffers)
        {
            this->BindDynamicUavDescriptorTable(rootParameterIndex, buffers, {});
        }
        void BindDynamicUavDescriptorTable(
            size_t rootParameterIndex,
            Core::Span<TextureHandle> textures)
        {
            this->BindDynamicUavDescriptorTable(rootParameterIndex, {}, textures);
        }
        virtual void BindDynamicUavDescriptorTable(
            size_t rootParameterIndex,
            Core::Span<BufferHandle> buffers,
            Core::Span<TextureHandle> textures) = 0;

        virtual void BindResourceTable(size_t rootParameterIndex) = 0;
        virtual void BindSamplerTable(size_t rootParameterIndex) = 0;

        virtual void BeginTimerQuery(TimerQueryHandle query) = 0;
        virtual void EndTimerQuery(TimerQueryHandle query) = 0;
#endif
    };

    class ScopedMarker
    {
    public:
        ScopedMarker(CommandContext* context)
            : m_context(context) {}

        ~ScopedMarker() { this->m_context->EndMarker(); }

    private:
        CommandContext* m_context;
    };
}