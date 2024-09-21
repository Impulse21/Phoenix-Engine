#pragma once

#include "phxGfxResources.h"

namespace phx::gfx
{
	constexpr size_t kBufferCount = 3;

	void InitializeNull();

#if defined(PHX_PLATFORM_WINDOWS)
	void InitializeWindows(SwapChainDesc const& desc, HWND hWnd);
#endif

	void Finalize();

	class Device
	{
	public:
		inline static Device* Ptr = nullptr;

		virtual void WaitForIdle() = 0;
		virtual void ResizeSwapChain(SwapChainDesc const& desc) = 0;
		virtual void Present() = 0;

	public:
		virtual ~Device() = default;
	};

	class GpuMemoryManager
	{
	public:
		inline static GpuMemoryManager* Ptr = nullptr;

	public:
		virtual ~GpuMemoryManager() = default;
	};

	class ResourceManager
	{
	public:
		inline static ResourceManager* Ptr = nullptr;

		virtual void RunGrabageCollection(uint64_t completedFrame = ~0u) = 0;

	public:
		virtual ~ResourceManager() = default;
	};

	class CommandList;
	class Renderer
	{
	public:
		inline static Renderer* Ptr = nullptr;

		virtual CommandList* BeginCommandRecording(CommandQueueType type = CommandQueueType::Graphics) = 0;
		virtual void SubmitCommands() = 0;

	public:
		virtual ~Renderer() = default;
	};

	class RenderPassRenderer;
	class CommandList
	{
	public:
		virtual ~CommandList() = default;
		virtual RenderPassRenderer* BeginRenderPass() = 0;
		virtual void EndRenderPass(RenderPassRenderer* pass) = 0;
	};

	class RenderPassRenderer
	{
	public:
		virtual ~RenderPassRenderer() = default;
	};

#if false

    class IGfxDevice
    {
    public:
        virtual ~IGfxDevice() = default;

        // -- Frame Functions ---
    public:
        virtual void Initialize(SwapChainDesc const& desc, void* windowHandle) = 0;
        virtual void Finalize() = 0;

        // -- Resizes swapchain ---
        virtual void ResizeSwapchain(SwapChainDesc const& desc) = 0;

        // -- Submits Command lists and presents ---
        virtual void SubmitFrame() = 0;
        virtual void WaitForIdle() = 0;

        virtual bool IsDevicedRemoved() = 0;

        // -- Resouce Functions ---
    public:
        template<typename T>
        CommandSignatureHandle CreateCommandSignature(CommandSignatureDesc const& desc)
        {
            static_assert(sizeof(T) % sizeof(uint32_t) == 0);
            return this->CreateCommandSignature(desc, sizeof(T));
        }
        virtual CommandSignatureHandle CreateCommandSignature(CommandSignatureDesc const& desc, size_t byteStride) = 0;
        virtual ShaderHandle CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode) = 0;
        virtual InputLayoutHandle CreateInputLayout(VertexAttributeDesc* desc, uint32_t attributeCount) = 0;
        virtual GfxPipelineHandle CreateGfxPipeline(GfxPipelineDesc const& desc) = 0;
        virtual ComputePipelineHandle CreateComputePipeline(ComputePipelineDesc const& desc) = 0;
        virtual MeshPipelineHandle CreateMeshPipeline(MeshPipelineDesc const& desc) = 0;
        virtual TextureHandle CreateTexture(TextureDesc const& desc) = 0;
        virtual RenderPassHandle CreateRenderPass(RenderPassDesc const& desc) = 0;
        virtual BufferHandle CreateBuffer(BufferDesc const& desc) = 0;
        virtual int CreateSubresource(TextureHandle texture, SubresouceType subresourceType, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip = 0, uint32_t mpCount = ~0) = 0;
        virtual int CreateSubresource(BufferHandle buffer, SubresouceType subresourceType, size_t offset, size_t size = ~0u) = 0;
        virtual RTAccelerationStructureHandle CreateRTAccelerationStructure(RTAccelerationStructureDesc const& desc) = 0;
        virtual TimerQueryHandle CreateTimerQuery() = 0;


        virtual void DeleteCommandSignature(CommandSignatureHandle handle) = 0;
        virtual const GfxPipelineDesc& GetGfxPipelineDesc(GfxPipelineHandle handle) = 0;
        virtual void DeleteGfxPipeline(GfxPipelineHandle handle) = 0;


        virtual void DeleteMeshPipeline(MeshPipelineHandle handle) = 0;

        virtual const TextureDesc& GetTextureDesc(TextureHandle handle) = 0;
        virtual DescriptorIndex GetDescriptorIndex(TextureHandle handle, SubresouceType type, int subResource = -1) = 0;
        virtual void DeleteTexture(TextureHandle handle) = 0;

        virtual void GetRenderPassFormats(RenderPassHandle handle, std::vector<RHI::Format>& outRtvFormats, RHI::Format& depthFormat) = 0;
        virtual RenderPassDesc GetRenderPassDesc(RenderPassHandle) = 0;
        virtual void DeleteRenderPass(RenderPassHandle handle) = 0;

        // -- TODO: End Remove

        virtual const BufferDesc& GetBufferDesc(BufferHandle handle) = 0;
        virtual DescriptorIndex GetDescriptorIndex(BufferHandle handle, SubresouceType type, int subResource = -1) = 0;

        template<typename T>
        T* GetBufferMappedData(BufferHandle handle)
        {
            return static_cast<T*>(this->GetBufferMappedData(handle));
        };

        virtual void* GetBufferMappedData(BufferHandle handle) = 0;
        virtual uint32_t GetBufferMappedDataSizeInBytes(BufferHandle handle) = 0;
        virtual void DeleteBuffer(BufferHandle handle) = 0;

        // -- Ray Tracing ---
        virtual size_t GetRTTopLevelAccelerationStructureInstanceSize() = 0;
        virtual void WriteRTTopLevelAccelerationStructureInstance(RTAccelerationStructureDesc::TopLevelDesc::Instance const& instance, void* dest) = 0;
        virtual const RTAccelerationStructureDesc& GetRTAccelerationStructureDesc(RTAccelerationStructureHandle handle) = 0;
        virtual void DeleteRtAccelerationStructure(RTAccelerationStructureHandle handle) = 0;
        virtual DescriptorIndex GetDescriptorIndex(RTAccelerationStructureHandle handle) = 0;

        // -- Query Stuff ---
        virtual void DeleteTimerQuery(TimerQueryHandle query) = 0;
        virtual bool PollTimerQuery(TimerQueryHandle query) = 0;
        virtual Core::TimeStep GetTimerQueryTime(TimerQueryHandle query) = 0;
        virtual void ResetTimerQuery(TimerQueryHandle query) = 0;

        // -- Utility ---
    public:
        virtual TextureHandle GetBackBuffer() = 0;
        virtual size_t GetNumBindlessDescriptors() const = 0;

        virtual ShaderModel GetMinShaderModel() const = 0;
        virtual ShaderType GetShaderType() const = 0;

        virtual GraphicsAPI GetApi() const = 0;

        virtual void BeginCapture(std::wstring const& filename) = 0;
        virtual void EndCapture(bool discard = false) = 0;

        virtual size_t GetFrameIndex() = 0;
        virtual size_t GetMaxInflightFrames() = 0;
        virtual bool CheckCapability(DeviceCapability deviceCapability) = 0;

        virtual float GetAvgFrameTime() = 0;
        virtual uint64_t GetUavCounterPlacementAlignment() = 0;
        virtual MemoryUsage GetMemoryUsage() const = 0;

        // -- Command list Functions ---
        // These are not thread Safe
    public:
        // TODO: Change to a new pattern so we don't require a command list stored on an object. Instread, request from a pool of objects
        virtual CommandListHandle BeginCommandList(CommandQueueType queueType = CommandQueueType::Graphics) = 0;
        virtual void WaitCommandList(CommandListHandle cmd, CommandListHandle WaitOn) = 0;

        // -- Ray Trace stuff       ---
        virtual void RTBuildAccelerationStructure(RHI::RTAccelerationStructureHandle accelStructure, CommandListHandle cmd) = 0;

        // -- Ray Trace Stuff END   ---
        virtual void BeginMarker(std::string_view name, CommandListHandle cmd) = 0;
        virtual void EndMarker(CommandListHandle cmd) = 0;
        virtual GPUAllocation AllocateGpu(size_t bufferSize, size_t stride, CommandListHandle cmd) = 0;

        virtual void TransitionBarrier(TextureHandle texture, ResourceStates beforeState, ResourceStates afterState, CommandListHandle cmd) = 0;
        virtual void TransitionBarrier(BufferHandle buffer, ResourceStates beforeState, ResourceStates afterState, CommandListHandle cmd) = 0;
        virtual void TransitionBarriers(Core::Span<GpuBarrier> gpuBarriers, CommandListHandle cmd) = 0;
        virtual void ClearTextureFloat(TextureHandle texture, Color const& clearColour, CommandListHandle cmd) = 0;
        virtual void ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil, CommandListHandle cmd) = 0;

        virtual void Draw(DrawArgs const& args, CommandListHandle cmd) = 0;
        virtual void DrawIndexed(DrawArgs const& args, CommandListHandle cmd) = 0;

        virtual void ExecuteIndirect(RHI::CommandSignatureHandle commandSignature, RHI::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) = 0;
        virtual void ExecuteIndirect(RHI::CommandSignatureHandle commandSignature, RHI::BufferHandle args, size_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) = 0;

        virtual void DrawIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) = 0;
        virtual void DrawIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) = 0;

        virtual void DrawIndexedIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) = 0;
        virtual void DrawIndexedIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) = 0;

        template<typename T>
        void WriteBuffer(BufferHandle buffer, std::vector<T> const& data, uint64_t destOffsetBytes, CommandListHandle cmd)
        {
            this->WriteBuffer(buffer, data.data(), sizeof(T) * data.size(), destOffsetBytes, cmd);
        }

        template<typename T>
        void WriteBuffer(BufferHandle buffer, T const& data, uint64_t destOffsetBytes, CommandListHandle cmd)
        {
            this->WriteBuffer(buffer, &data, sizeof(T), destOffsetBytes, cmd);
        }

        template<typename T>
        void WriteBuffer(BufferHandle buffer, Core::Span<T> data, uint64_t destOffsetBytes, CommandListHandle cmd)
        {
            this->WriteBuffer(buffer, data.begin(), sizeof(T) * data.Size(), destOffsetBytes, cmd);
        }

        virtual void WriteBuffer(BufferHandle buffer, const void* Data, size_t dataSize, uint64_t destOffsetBytes, CommandListHandle cmd) = 0;

        virtual void CopyBuffer(BufferHandle dst, uint64_t dstOffset, BufferHandle src, uint64_t srcOffset, size_t sizeInBytes, CommandListHandle cmd) = 0;

        virtual void WriteTexture(TextureHandle texture, uint32_t firstSubResource, size_t numSubResources, SubresourceData* pSubResourceData, CommandListHandle cmd) = 0;
        virtual void WriteTexture(TextureHandle texture, uint32_t arraySlice, uint32_t mipLevel, const void* Data, size_t rowPitch, size_t depthPitch, CommandListHandle cmd) = 0;

        virtual void SetGfxPipeline(GfxPipelineHandle gfxPipeline, CommandListHandle cmd) = 0;
        virtual void SetViewports(Viewport* viewports, size_t numViewports, CommandListHandle cmd) = 0;
        virtual void SetScissors(Rect* scissor, size_t numScissors, CommandListHandle cmd) = 0;
        virtual void SetRenderTargets(Core::Span<TextureHandle> renderTargets, TextureHandle depthStenc, CommandListHandle cmd) = 0;

        // -- Comptute Stuff ---
        virtual void SetComputeState(ComputePipelineHandle state, CommandListHandle cmd) = 0;
        virtual void Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ, CommandListHandle cmd) = 0;
        virtual void DispatchIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) = 0;
        virtual void DispatchIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) = 0;

        // -- Mesh Stuff ---
        virtual void SetMeshPipeline(MeshPipelineHandle meshPipeline, CommandListHandle cmd) = 0;
        virtual void DispatchMesh(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ, CommandListHandle cmd) = 0;
        virtual void DispatchMeshIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) = 0;
        virtual void DispatchMeshIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) = 0;

        virtual void BindPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants, CommandListHandle cmd) = 0;
        template<typename T>
        void BindPushConstant(uint32_t rootParameterIndex, const T& constants, CommandListHandle cmd)
        {
            static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be a multiple of 4 bytes");
            this->BindPushConstant(rootParameterIndex, sizeof(T), &constants, cmd);
        }

        virtual void BindConstantBuffer(size_t rootParameterIndex, BufferHandle constantBuffer, CommandListHandle cmd) = 0;
        virtual void BindDynamicConstantBuffer(size_t rootParameterIndex, size_t sizeInBytes, const void* bufferData, CommandListHandle cmd) = 0;
        template<typename T>
        void BindDynamicConstantBuffer(size_t rootParameterIndex, T const& bufferData, CommandListHandle cmd)
        {
            this->BindDynamicConstantBuffer(rootParameterIndex, sizeof(T), &bufferData, cmd);
        }

        virtual void BindVertexBuffer(uint32_t slot, BufferHandle vertexBuffer, CommandListHandle cmd) = 0;

        /**
         * Set dynamic vertex buffer data to the rendering pipeline.
         */
        virtual void BindDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData, CommandListHandle cmd) = 0;

        template<typename T>
        void BindDynamicVertexBuffer(uint32_t slot, const std::vector<T>& vertexBufferData, CommandListHandle cmd)
        {
            this->BindDynamicVertexBuffer(slot, vertexBufferData.size(), sizeof(T), vertexBufferData.data(), cmd);
        }

        virtual void BindIndexBuffer(BufferHandle bufferHandle, CommandListHandle cmd) = 0;

        /**
         * Bind dynamic index buffer data to the rendering pipeline.
         */
        virtual void BindDynamicIndexBuffer(size_t numIndicies, RHI::Format indexFormat, const void* indexBufferData, CommandListHandle cmd) = 0;

        template<typename T>
        void BindDynamicIndexBuffer(const std::vector<T>& indexBufferData, CommandListHandle cmd)
        {
            staticassert(sizeof(T) == 2 || sizeof(T) == 4);

            RHI::Format indexFormat = (sizeof(T) == 2) ? RHI::Format::R16_UINT : RHI::Format::R32_UINT;
            this->BindDynamicIndexBuffer(indexBufferData.size(), indexFormat, indexBufferData.Data(), cmd);
        }

        /**
         * Set dynamic structured buffer contents.
         */
        virtual void BindDynamicStructuredBuffer(uint32_t rootParameterIndex, size_t numElements, size_t elementSize, const void* bufferData, CommandListHandle cmd) = 0;

        template<typename T>
        void BindDynamicStructuredBuffer(uint32_t rootParameterIndex, std::vector<T> const& bufferData, CommandListHandle cmd)
        {
            this->BindDynamicStructuredBuffer(rootParameterIndex, bufferData.size(), sizeof(T), bufferData.Data(), cmd);
        }

        virtual void BindStructuredBuffer(size_t rootParameterIndex, BufferHandle buffer, CommandListHandle cmd) = 0;

        virtual void BindDynamicDescriptorTable(size_t rootParameterIndex, Core::Span<TextureHandle> textures, CommandListHandle cmd) = 0;
        void BindDynamicUavDescriptorTable(
            size_t rootParameterIndex,
            Core::Span<BufferHandle> buffers,
            CommandListHandle cmd)
        {
            this->BindDynamicUavDescriptorTable(rootParameterIndex, buffers, {}, cmd);
        }
        void BindDynamicUavDescriptorTable(
            size_t rootParameterIndex,
            Core::Span<TextureHandle> textures,
            CommandListHandle cmd)
        {
            this->BindDynamicUavDescriptorTable(rootParameterIndex, {}, textures, cmd);
        }
        virtual void BindDynamicUavDescriptorTable(
            size_t rootParameterIndex,
            Core::Span<BufferHandle> buffers,
            Core::Span<TextureHandle> textures,
            CommandListHandle cmd) = 0;

        virtual void BindResourceTable(size_t rootParameterIndex, CommandListHandle cmd) = 0;
        virtual void BindSamplerTable(size_t rootParameterIndex, CommandListHandle cmd) = 0;

        virtual void BeginTimerQuery(TimerQueryHandle query, CommandListHandle cmd) = 0;
        virtual void EndTimerQuery(TimerQueryHandle query, CommandListHandle cmd) = 0;

    };

    using GfxDevice = IGfxDevice;

#endif
}