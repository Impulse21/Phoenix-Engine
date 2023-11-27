#pragma once

#include <PhxEngine/Core/TimeStep.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/RHI/PhxRHIResources.h>

#define NOMINMAX
#include <dstorage.h>

namespace PhxEngine::RHI
{
    class ScopedMarker;

    struct ExecutionReceipt
    {
        uint64_t FenceValue;
        CommandQueueType CommandQueue;
    };

    struct MemoryUsage
    {
        uint64_t Budget = 0ull;
        uint64_t Usage = 0ull;
    };

    struct SubmitReceipt
    {
        uint64_t FenceValue;
        void* Internal;
    };

    enum class RequestSourceType
    {
        File,
        Memory
    };

    enum class RequestDesntination
    {
        Buffer,
        TextureRegion,
        TextureAllSubResources
    };

    struct StorageRequest
    {
        DSTORAGE_REQUEST InternalRequest;

        union
        {
            IBuffer* DestinationBuffer;
            ITexture* DestinationTexture;
        };
    };

    class IDirectStorage
    {
    public:
        virtual void EnqueueRequest(StorageRequest const& request) = 0;
        virtual SubmitReceipt Submit(bool waitForComplete = false) = 0;

        virtual ~IDirectStorage() = default;
    };

    class DynamicRHI
    {
    public:
        virtual ~DynamicRHI() = default;

        // -- Frame Functions ---
    public:
        virtual void Initialize() = 0;
        virtual void Finalize() = 0;

        // -- Submits Command lists and presents ---
        virtual void Present(ISwapChain* swapChain) = 0;
        virtual void WaitForIdle() = 0;

        virtual bool IsDevicedRemoved() = 0;

        virtual bool CheckCapability(DeviceCapability deviceCapability) = 0;

        virtual IDirectStorage* GetDirectStorage() = 0;

        virtual void Wait(SubmitReceipt const& reciet) = 0;

        // -- Resouce Functions ---
    public:
        virtual SwapChainRef CreateSwapChain(SwapChainDesc const& desc, void* windowsRef) = 0;

        virtual ShaderRef CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode) = 0;
        virtual InputLayoutRef CreateInputLayout(VertexAttributeDesc* desc, uint32_t attributeCount) = 0;
        virtual GfxPipelineRef CreateGfxPipeline(GfxPipelineDesc const& desc) = 0;
        virtual ComputePipelineRef CreateComputePipeline(ComputePipelineDesc const& desc) = 0;
        virtual MeshPipelineRef CreateMeshPipeline(MeshPipelineDesc const& desc) = 0;
        virtual TextureRef CreateTexture(TextureDesc const& desc) = 0;
        virtual BufferRef CreateBuffer(BufferDesc const& desc) = 0;
        virtual CommandListRef CreateCommandList(RHI::CommandQueueType type = CommandQueueType::Graphics) = 0;

        virtual void ResizeSwapChain(ISwapChain* swapChain, SwapChainDesc const& desc) = 0;

        // Create the other stuff
    };

    template <class T>
    void HashCombine(size_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

}