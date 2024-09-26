#pragma once

#include "phxGfxDeviceResources.h"

namespace phx::gfx::internal
{
    class ICommandCtx
    {
    public:
        virtual ~ICommandCtx() = default;

        virtual void TransitionBarrier(GpuBarrier const& barrier) = 0;
        virtual void TransitionBarriers(Span<GpuBarrier> gpuBarriers) = 0;
        virtual void ClearBackBuffer(Color const& clearColour) = 0;
        virtual void ClearTextureFloat(TextureHandle texture, Color const& clearColour) = 0;
        virtual void ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil) = 0;
#if false

        // -- Ray Trace stuff       ---
        virtual void RTBuildAccelerationStructure(RHI::RTAccelerationStructureHandle accelStructure) = 0;

        // -- Ray Trace Stuff END   ---

        virtual ScopedMarker BeginScopedMarker(std::string_view name) = 0;
        virtual void BeginMarker(std::string_view name) = 0;
        virtual void EndMarker() = 0;
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
}