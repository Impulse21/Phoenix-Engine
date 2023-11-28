#pragma once

#include <vector>
#include <optional>
#include <variant>

#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Core/Vector.h>
#include <PhxEngine/RHI/PhxRHIDefinitions.h>
#include <PhxEngine/Core/RefCountPtr.h>

namespace PhxEngine::RHI
{
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

    class RHIResource
    {
    protected:
        RHIResource() = default;
        virtual ~RHIResource() = default;

    public:
        virtual unsigned long AddRef() = 0;
        virtual unsigned long Release() = 0;

        // Non-copyable and non-movable
        RHIResource(const RHIResource&) = delete;
        RHIResource(const RHIResource&&) = delete;
        RHIResource& operator=(const RHIResource&) = delete;
        RHIResource& operator=(const RHIResource&&) = delete;
    };

    struct ShaderDesc
    {
        ShaderStage Stage = ShaderStage::None;
        std::string DebugName = "";
    };

    class IShader : public RHIResource
    {
    public:
        virtual const ShaderDesc& GetDesc() const = 0;

        virtual ~IShader() = default;
    };
    using ShaderRef = Core::RefCountPtr<IShader>;

    struct VertexAttributeDesc
    {
        static const uint32_t SAppendAlignedElement = 0xffffffff; // automatically figure out AlignedByteOffset depending on Format

        std::string SemanticName;
        uint32_t SemanticIndex = 0;
        RHI::Format Format = RHI::Format::UNKNOWN;
        uint32_t InputSlot = 0;
        uint32_t AlignedByteOffset = SAppendAlignedElement;
        bool IsInstanced = false;
    };
    
    class IInputLayout : public RHIResource
    {
    public:
        virtual const PhxEngine::Core::Span<VertexAttributeDesc> GetDesc() const = 0;

        virtual ~IInputLayout() = default;
    };

    using InputLayoutRef = Core::RefCountPtr<IInputLayout>;

    struct TextureDesc
    {
        TextureMiscFlags MiscFlags = TextureMiscFlags::None;
        BindingFlags BindingFlags = BindingFlags::ShaderResource;
        TextureDimension Dimension = TextureDimension::Texture2D;
        ResourceStates InitialState = ResourceStates::Common;

        RHI::Format Format = RHI::Format::UNKNOWN;
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

        RHI::ClearValue OptmizedClearValue = {};
        std::string DebugName;
    };

    class ITexture : public RHIResource
    {
    public:
        virtual const TextureDesc& GetDesc() const = 0;

        virtual ~ITexture() = default;
    };

    using TextureRef = Core::RefCountPtr<ITexture>;

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

        RHI::Format Format = RHI::Format::UNKNOWN;

        bool AllowUnorderedAccess = false;
        // TODO: Remove
        bool CreateBindless = false;

        size_t UavCounterOffsetInBytes = 0;
        // BufferRef UavCounterBuffer = {};

        // BufferRef AliasedBuffer = {};

        std::string DebugName;
    };

    class IBuffer : public RHIResource
    {
    public:
        virtual const BufferDesc& GetDesc() const = 0;

        virtual ~IBuffer() = default;
    };

    using BufferRef = Core::RefCountPtr<IBuffer>;

    // This is just a workaround for now
    class IRootSignatureBuilder
    {
    public:
        virtual ~IRootSignatureBuilder() = default;
    };

    struct GfxPipelineDesc
    {
        PrimitiveType PrimType = PrimitiveType::TriangleList;
        InputLayoutRef InputLayout;

        IRootSignatureBuilder* RootSignatureBuilder = nullptr;

        ShaderRef VertexShader;
        ShaderRef HullShader;
        ShaderRef DomainShader;
        ShaderRef GeometryShader;
        ShaderRef PixelShader;

        BlendRenderState BlendRenderState = {};
        DepthStencilRenderState DepthStencilRenderState = {};
        RasterRenderState RasterRenderState = {};

        Phx::FlexArray<RHI::Format> RtvFormats;
        std::optional<RHI::Format> DsvFormat;

        uint32_t SampleCount = 1;
        uint32_t SampleQuality = 0;
    };

    class IGfxPipeline : public RHIResource
    {
    public:
        virtual const GfxPipelineDesc& GetDesc() const = 0;

        virtual ~IGfxPipeline() = default;
    };

    using GfxPipelineRef = Core::RefCountPtr<IGfxPipeline>;

    struct ComputePipelineDesc
    {
        ShaderRef ComputeShader;
    };

    class IComputePipeline : public RHIResource
    {
    public:
        virtual const ComputePipelineDesc& GetDesc() const = 0;

        virtual ~IComputePipeline() = default;
    };
    using ComputePipelineRef = Core::RefCountPtr<IComputePipeline>;

    struct MeshPipelineDesc
    {
        PrimitiveType PrimType = PrimitiveType::TriangleList;

        ShaderRef AmpShader;
        ShaderRef MeshShader;
        ShaderRef PixelShader;

        BlendRenderState BlendRenderState = {};
        DepthStencilRenderState DepthStencilRenderState = {};
        RasterRenderState RasterRenderState = {};

        Phx::FlexArray<RHI::Format> RtvFormats;
        std::optional<RHI::Format> DsvFormat;

        uint32_t SampleCount = 1;
        uint32_t SampleQuality = 0;

    };

    class IMeshPipeline : public RHIResource
    {
    public:
        virtual const MeshPipelineDesc& GetDesc() const = 0;

        virtual ~IMeshPipeline() = default;
    };
    using MeshPipelineRef = Core::RefCountPtr<IMeshPipeline>;

    class ISwapChain : public RHIResource
    {
    public:
        virtual const SwapChainDesc& GetDesc() const = 0;

        virtual ~ISwapChain() = default;
    };
    using SwapChainRef = Core::RefCountPtr<ISwapChain>;
    struct GpuBarrier
    {
        struct BufferBarrier
        {
            RHI::IBuffer* Buffer;
            RHI::ResourceStates BeforeState;
            RHI::ResourceStates AfterState;
        };

        struct TextureBarrier
        {
            RHI::ITexture* Texture;
            RHI::ResourceStates BeforeState;
            RHI::ResourceStates AfterState;
            int Mip;
            int Slice;
        };

        struct MemoryBarrier
        {
            std::variant<ITexture*, IBuffer*> Resource;
        };

        std::variant<BufferBarrier, TextureBarrier, GpuBarrier::MemoryBarrier> Data;

        static GpuBarrier CreateMemory()
        {
            GpuBarrier barrier = {};
            barrier.Data = GpuBarrier::MemoryBarrier{};

            return barrier;
        }

        static GpuBarrier CreateMemory(ITexture* texture)
        {
            GpuBarrier barrier = {};
            barrier.Data = GpuBarrier::MemoryBarrier{ .Resource = texture };

            return barrier;
        }

        static GpuBarrier CreateMemory(IBuffer* buffer)
        {
            GpuBarrier barrier = {};
            barrier.Data = GpuBarrier::MemoryBarrier{ .Resource = buffer };

            return barrier;
        }

        static GpuBarrier CreateTexture(
            ITexture* texture,
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

        static GpuBarrier CreateBuffer(IBuffer* buffer, ResourceStates beforeState, ResourceStates afterState)
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

    class ICommandList : public RHIResource
    {
    public:
        virtual ~ICommandList() = default;

        virtual void Reset() = 0;

        // -- Ray Trace stuff       ---
        // virtual void RTBuildAccelerationStructure(RHI::RTAccelerationStructureHandle accelStructure) = 0;
        // -- Ray Trace Stuff END   ---

        virtual void BeginMarker(std::string_view name) = 0;
        virtual void EndMarker() = 0;
       //  virtual GPUAllocation AllocateGpu(size_t bufferSize, size_t stride) = 0;

        virtual void TransitionBarrier(ITexture* texture, ResourceStates beforeState, ResourceStates afterState) = 0;
        virtual void TransitionBarrier(IBuffer* buffer, ResourceStates beforeState, ResourceStates afterState) = 0;
        virtual void TransitionBarriers(Core::Span<GpuBarrier> gpuBarriers) = 0;
        virtual void ClearBackBuffer(ISwapChain* swapChain, Color const& clearColour) = 0;
        virtual void ClearTextureFloat(ITexture* texture, Color const& clearColour) = 0;
        virtual void ClearDepthStencilTexture(ITexture* depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil) = 0;

        virtual void Draw(DrawArgs const& args) = 0;
        virtual void DrawIndexed(DrawArgs const& args) = 0;
#if 0
        virtual void ExecuteIndirect(RHI::CommandSignatureHandle commandSignature, IBuffer* args, size_t argsOffsetInBytes, uint32_t maxCount) = 0;
        virtual void ExecuteIndirect(RHI::CommandSignatureHandle commandSignature, IBuffer* args, size_t argsOffsetInBytes, IBuffer* count, size_t countOffsetInBytes, uint32_t maxCount) = 0;

        virtual void DrawIndirect(IBuffer* args, size_t argsOffsetInBytes, uint32_t maxCount) = 0;
        virtual void DrawIndirect(IBuffer* args, size_t argsOffsetInBytes, IBuffer* count, size_t countOffsetInBytes, uint32_t maxCount) = 0;

        virtual void DrawIndexedIndirect(IBuffer* args, size_t argsOffsetInBytes, uint32_t maxCount) = 0;
        virtual void DrawIndexedIndirect(IBuffer* args, size_t argsOffsetInBytes, IBuffer* count, size_t countOffsetInBytes, uint32_t maxCount) = 0;
#endif
        template<typename T>
        void WriteBuffer(IBuffer* buffer, Phx::FlexArray<T> const& data, uint64_t destOffsetBytes)
        {
            this->WriteBuffer(buffer, data.data(), sizeof(T) * data.size(), destOffsetBytes);
        }

        template<typename T>
        void WriteBuffer(IBuffer* buffer, T const& data, uint64_t destOffsetBytes)
        {
            this->WriteBuffer(buffer, &data, sizeof(T), destOffsetBytes);
        }

        template<typename T>
        void WriteBuffer(IBuffer* buffer, Core::Span<T> data, uint64_t destOffsetBytes)
        {
            this->WriteBuffer(buffer, data.begin(), sizeof(T) * data.Size(), destOffsetBytes);
        }

        virtual void WriteBuffer(IBuffer* buffer, const void* Data, size_t dataSize, uint64_t destOffsetBytes) = 0;

        virtual void CopyBuffer(IBuffer* dst, uint64_t dstOffset, IBuffer* src, uint64_t srcOffset, size_t sizeInBytes) = 0;

        virtual void WriteTexture(ITexture* texture, uint32_t firstSubResource, size_t numSubResources, SubresourceData* pSubResourceData) = 0;
        virtual void WriteTexture(ITexture* texture, uint32_t arraySlice, uint32_t mipLevel, const void* Data, size_t rowPitch, size_t depthPitch) = 0;

        virtual void SetGfxPipeline(IGfxPipeline* gfxPipeline) = 0;
        virtual void SetViewports(Viewport* viewports, size_t numViewports) = 0;
        virtual void SetScissors(Rect* scissor, size_t numScissors) = 0;
        virtual void SetRenderTargets(Core::Span<ITexture*> renderTargets, ITexture* depthStenc) = 0;

        // -- Comptute Stuff ---
        virtual void SetComputeState(IComputePipeline* state) = 0;
        virtual void Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) = 0;
        virtual void DispatchIndirect(IBuffer* args, uint32_t argsOffsetInBytes, uint32_t maxCount) = 0;
        virtual void DispatchIndirect(IBuffer* args, uint32_t argsOffsetInBytes, IBuffer* count, size_t countOffsetInBytes, uint32_t maxCount) = 0;

        // -- Mesh Stuff ---
        virtual void SetMeshPipeline(IMeshPipeline* meshPipeline) = 0;
        virtual void DispatchMesh(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) = 0;
        virtual void DispatchMeshIndirect(IBuffer* args, uint32_t argsOffsetInBytes, uint32_t maxCount) = 0;
        virtual void DispatchMeshIndirect(IBuffer* args, uint32_t argsOffsetInBytes, IBuffer* count, size_t countOffsetInBytes, uint32_t maxCount) = 0;

        virtual void BindPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants) = 0;
        template<typename T>
        void BindPushConstant(uint32_t rootParameterIndex, const T& constants)
        {
            static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be a multiple of 4 bytes");
            this->BindPushConstant(rootParameterIndex, sizeof(T), &constants);
        }

        virtual void BindConstantBuffer(size_t rootParameterIndex, IBuffer* constantBuffer) = 0;
        virtual void BindDynamicConstantBuffer(size_t rootParameterIndex, size_t sizeInBytes, const void* bufferData) = 0;
        template<typename T>
        void BindDynamicConstantBuffer(size_t rootParameterIndex, T const& bufferData)
        {
            this->BindDynamicConstantBuffer(rootParameterIndex, sizeof(T), &bufferData);
        }

        virtual void BindVertexBuffer(uint32_t slot, IBuffer* vertexBuffer) = 0;

        /**
         * Set dynamic vertex buffer data to the rendering pipeline.
         */
        virtual void BindDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData) = 0;

        template<typename T>
        void BindDynamicVertexBuffer(uint32_t slot, const Phx::FlexArray<T>& vertexBufferData)
        {
            this->BindDynamicVertexBuffer(slot, vertexBufferData.size(), sizeof(T), vertexBufferData.data());
        }

        virtual void BindIndexBuffer(IBuffer* bufferHandle) = 0;

        /**
         * Bind dynamic index buffer data to the rendering pipeline.
         */
        virtual void BindDynamicIndexBuffer(size_t numIndicies, RHI::Format indexFormat, const void* indexBufferData) = 0;

        template<typename T>
        void BindDynamicIndexBuffer(const Phx::FlexArray<T>& indexBufferData)
        {
            staticassert(sizeof(T) == 2 || sizeof(T) == 4);

            RHI::Format indexFormat = (sizeof(T) == 2) ? RHI::Format::R16_UINT : RHI::Format::R32_UINT;
            this->BindDynamicIndexBuffer(indexBufferData.size(), indexFormat, indexBufferData.Data());
        }

        /**
         * Set dynamic structured buffer contents.
         */
        virtual void BindDynamicStructuredBuffer(uint32_t rootParameterIndex, size_t numElements, size_t elementSize, const void* bufferData) = 0;

        template<typename T>
        void BindDynamicStructuredBuffer(uint32_t rootParameterIndex, Core::Span<T> const& bufferData)
        {
            this->BindDynamicStructuredBuffer(rootParameterIndex, bufferData.size(), sizeof(T), bufferData.Data());
        }

        virtual void BindStructuredBuffer(size_t rootParameterIndex, IBuffer* buffer) = 0;

        virtual void BindDynamicDescriptorTable(size_t rootParameterIndex, Core::Span<ITexture*> textures) = 0;
        void BindDynamicUavDescriptorTable(
            size_t rootParameterIndex,
            Core::Span<IBuffer*> buffers)
        {
            this->BindDynamicUavDescriptorTable(rootParameterIndex, buffers, {});
        }
        void BindDynamicUavDescriptorTable(
            size_t rootParameterIndex,
            Core::Span<ITexture*> textures)
        {
            this->BindDynamicUavDescriptorTable(rootParameterIndex, {}, textures);
        }
        virtual void BindDynamicUavDescriptorTable(
            size_t rootParameterIndex,
            Core::Span<IBuffer*> buffers,
            Core::Span<ITexture*> textures) = 0;

        virtual void BindResourceTable(size_t rootParameterIndex) = 0;
        virtual void BindSamplerTable(size_t rootParameterIndex) = 0;

        // virtual void BeginTimerQuery(TimerQueryHandle query) = 0;
        // virtual void EndTimerQuery(TimerQueryHandle query) = 0;
    };
    using CommandListRef = Core::RefCountPtr<ICommandList>;
}