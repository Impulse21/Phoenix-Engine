#pragma once

#include "D3D12Common.h"
#include <D3D12MemAlloc.h>
#include <RHI/D3D12/D3D12DescriptorHeap.h>
namespace PhxEngine::RHI::D3D12
{
    class D3D12GfxDevice;
    class D3D12CommandQueue;

    enum class DescriptorHeapTypes : uint8_t
    {
        CBV_SRV_UAV,
        Sampler,
        RTV,
        DSV,
        Count,
    };

    struct D3D12Shader final : Core::RefCounter<IShader>
    {
        ShaderDesc Desc = {};
        Phx::FlexArray<uint8_t> ByteCode;
        Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> RootSignatureDeserializer;
        const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* RootSignatureDesc = nullptr;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;

        D3D12Shader() = default;
        D3D12Shader(ShaderDesc const& desc, const void* binary, size_t binarySize)
        {
            this->Desc = desc;
            this->ByteCode.resize(binarySize);
            std::memcpy(ByteCode.data(), binary, binarySize);
        }

        ShaderDesc const& GetDesc() const override{ return this->Desc; }
    };

    template<>
    struct TD3D12ResourceTraits<IShader>
    {
        typedef D3D12Shader TConcreteType;
    };

    struct D3D12InputLayout final : Core::RefCounter<IInputLayout>
    {
        Phx::FlexArray<VertexAttributeDesc> Attributes;
        Phx::FlexArray<D3D12_INPUT_ELEMENT_DESC> InputElements;

        D3D12InputLayout() = default;

        const Core::Span<VertexAttributeDesc> GetDesc() const override{ return this->Attributes; }
    };

    template<>
    struct TD3D12ResourceTraits<IInputLayout>
    {
        typedef D3D12InputLayout TConcreteType;
    };

    struct D3D12GfxPipeline final : Core::RefCounter<IGfxPipeline>
    {
        GfxPipelineDesc Desc = {};
        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;

        D3D12GfxPipeline() = default;

        GfxPipelineDesc const& GetDesc() const override{ return this->Desc; }
    };

    template<>
    struct TD3D12ResourceTraits<IGfxPipeline>
    {
        typedef D3D12GfxPipeline TConcreteType;
    };

    struct D3D12ComputePipeline final : Core::RefCounter<IComputePipeline>
    {
        ComputePipelineDesc Desc = {};
        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;

        D3D12ComputePipeline() = default;
        ComputePipelineDesc const& GetDesc() const override{ return this->Desc; }
    };

    template<>
    struct TD3D12ResourceTraits<IComputePipeline>
    {
        typedef D3D12ComputePipeline TConcreteType;
    };

    struct D3D12MeshPipeline final : Core::RefCounter<IMeshPipeline>
    {
        MeshPipelineDesc Desc = {};
        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;

        D3D12MeshPipeline() = default;
        MeshPipelineDesc const& GetDesc() const override{ return this->Desc; }
    };

    template<>
    struct TD3D12ResourceTraits<IMeshPipeline>
    {
        typedef D3D12MeshPipeline TConcreteType;
    };

    struct DescriptorView
    {
        DescriptorHeapAllocation Allocation;
        DescriptorIndex BindlessIndex = cInvalidDescriptorIndex;
        D3D12_DESCRIPTOR_HEAP_TYPE Type = {};
        union
        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
            D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
            D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
            D3D12_SAMPLER_DESC SAMDesc;
            D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
            D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;
        };

        uint32_t FirstMip = 0;
        uint32_t MipCount = 0;
        uint32_t FirstSlice = 0;
        uint32_t SliceCount = 0;
    };

    class ICommandSignature
    {};

    struct D3D12CommandSignature final : Core::RefCounter<ICommandSignature>
    {
        Phx::FlexArray<D3D12_INDIRECT_ARGUMENT_DESC> D3D12Descs;
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> NativeSignature;
    };

    template<>
    struct TD3D12ResourceTraits<ICommandSignature>
    {
        typedef D3D12CommandSignature TConcreteType;
    };

    struct D3D12Texture final : Core::RefCounter<ITexture>
    {
        TextureDesc Desc = {};

        Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> Allocation;

        UINT64 TotalSize = 0;
        Phx::FlexArray<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> Footprints;
        Phx::FlexArray<UINT64> RowSizesInBytes;
        Phx::FlexArray<UINT> NumRows;

        // -- The views ---
        DescriptorView RtvAllocation;
        Phx::FlexArray<DescriptorView> RtvSubresourcesAlloc = {};

        DescriptorView DsvAllocation;
        Phx::FlexArray<DescriptorView> DsvSubresourcesAlloc = {};

        DescriptorView Srv;
        Phx::FlexArray<DescriptorView> SrvSubresourcesAlloc = {};

        DescriptorView UavAllocation;
        Phx::FlexArray<DescriptorView> UavSubresourcesAlloc = {};

        void DisposeViews()
        {
            RtvAllocation.Allocation.Free();
            for (auto& view : this->RtvSubresourcesAlloc)
            {
                view.Allocation.Free();
            }
            RtvSubresourcesAlloc.clear();
            RtvAllocation = {};

            DsvAllocation.Allocation.Free();
            for (auto& view : this->DsvSubresourcesAlloc)
            {
                view.Allocation.Free();
            }
            DsvSubresourcesAlloc.clear();
            DsvAllocation = {};

            Srv.Allocation.Free();
            for (auto& view : this->SrvSubresourcesAlloc)
            {
                view.Allocation.Free();
            }
            SrvSubresourcesAlloc.clear();
            Srv = {};

            UavAllocation.Allocation.Free();
            for (auto& view : this->UavSubresourcesAlloc)
            {
                view.Allocation.Free();
            }
            UavSubresourcesAlloc.clear();
            UavAllocation = {};
        }

        TextureDesc const& GetDesc() const override{ return this->Desc; }
    };

    template<>
    struct TD3D12ResourceTraits<ITexture>
    {
        typedef D3D12Texture TConcreteType;
    };

    struct D3D12Buffer final : Core::RefCounter<IBuffer>
    {
        BufferDesc Desc = {};
        Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> Allocation;

        void* MappedData = nullptr;
        uint32_t MappedSizeInBytes = 0;

        // -- Views ---
        DescriptorView Srv;
        Phx::FlexArray<DescriptorView> SrvSubresourcesAlloc = {};

        DescriptorView UavAllocation;
        Phx::FlexArray<DescriptorView> UavSubresourcesAlloc = {};


        D3D12_VERTEX_BUFFER_VIEW VertexView = {};
        D3D12_INDEX_BUFFER_VIEW IndexView = {};

        D3D12Buffer() = default;

        void DisposeViews()
        {
            Srv.Allocation.Free();
            for (auto& view : this->SrvSubresourcesAlloc)
            {
                view.Allocation.Free();
            }
            SrvSubresourcesAlloc.clear();
            Srv = {};

            UavAllocation.Allocation.Free();
            for (auto& view : this->UavSubresourcesAlloc)
            {
                view.Allocation.Free();
            }
            UavSubresourcesAlloc.clear();
            UavAllocation = {};
        }

        BufferDesc const& GetDesc() const override{ return this->Desc; }
    };

    template<>
    struct TD3D12ResourceTraits<IBuffer>
    {
        typedef D3D12Buffer TConcreteType;
    };

#if 0
    struct D3D12RTAccelerationStructure final : public RefCounter<IRTAccelerationStructure>
    {
        // RTAccelerationStructureDesc Desc = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Dx12Desc = {};
        Phx::FlexArray<D3D12_RAYTRACING_GEOMETRY_DESC> Geometries;
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO Info = {};
        // BufferHandle SratchBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> Allocation;
        DescriptorView Srv;


        void DisposeViews()
        {
            Srv.Allocation.Free();
        }

        D3D12RTAccelerationStructure() = default;
    };

    template<>
    struct TD3D12ResourceTraits<IRTAccelerationStructure>
    {
        typedef D3D12RTAccelerationStructure TConcreteType;
    };
#endif 
#if 0
    struct D3D12TimerQuery final : Core::RefCounter<Tim>
    {
        size_t BeginQueryIndex = 0;
        size_t EndQueryIndex = 0;

        // Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
        D3D12CommandQueue* CommandQueue = nullptr;
        uint64_t FenceCount = 0;

        bool Started = false;
        bool Resolved = false;
        Core::TimeStep Time;

    };
#endif

    struct D3D12SwapChain final : Core::RefCounter<ISwapChain>
    {
        SwapChainDesc Desc = {};
        Core::RefCountPtr<IDXGISwapChain1> NativeSwapchain;
        Core::RefCountPtr<IDXGISwapChain4> NativeSwapchain4;

        Phx::FlexArray<Core::RefCountPtr<D3D12Texture>> BackBuffers;

        D3D12Texture CurrentBackBuffer = {};

        SwapChainDesc const& GetDesc() const override{ return this->Desc; }
        ITexture* GetBackBuffer() const override{ return this->BackBuffers[NativeSwapchain4->GetCurrentBackBufferIndex()].Get(); }
    };

    template<>
    struct TD3D12ResourceTraits<ISwapChain>
    {
        typedef D3D12SwapChain TConcreteType;
    };

    struct D3D12DeviceBasicInfo final
    {
        uint32_t NumDeviceNodes;
    };

    class D3D12Adapter final
    {
    public:
        std::string Name;
        size_t DedicatedSystemMemory = 0;
        size_t DedicatedVideoMemory = 0;
        size_t SharedSystemMemory = 0;
        D3D12DeviceBasicInfo BasicDeviceInfo;
        DXGI_ADAPTER_DESC NativeDesc;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> NativeAdapter;

        static HRESULT EnumAdapters(uint32_t adapterIndex, IDXGIFactory6* factory6, IDXGIAdapter1** outAdapter);
    };

    class UploadBuffer;
    struct D3D12CommandList final : public Core::RefCounter<ICommandList>
    {
        CommandQueueType QueueType = CommandQueueType::Graphics;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> NativeCommandList;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> NativeCommandList6;
        ID3D12CommandAllocator* NativeCommandAllocator;
        std::unique_ptr<D3D12:: UploadBuffer> UploadBuffer;

        // -- Supplimentry data
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> BuildGeometries;
        std::vector<D3D12_RESOURCE_BARRIER> BarrierMemoryPool;

        DynamicSuballocator* ActiveDynamicSubAllocator;

        enum class PipelineType
        {
            Gfx,
            Compute,
            Mesh,
        } ActivePipelineType;

        union
        {
            D3D12ComputePipeline* ActiveComputePipeline;
            D3D12MeshPipeline* ActiveMeshPipeline;
        };
#if 0 
        std::vector<RHI::TimerQueryHandle> TimerQueries;
#endif

        // -- Helper Functions ---
        void TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
        {
            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                d3d12Resource.Get(),
                beforeState, afterState);

            // TODO: Batch Barrier
            this->NativeCommandList->ResourceBarrier(1, &barrier);
        }

        void Reset() override;

        // -- Ray Trace stuff       ---
        // void RTBuildAccelerationStructure(RHI::RTAccelerationStructureHandle accelStructure) override;
        // -- Ray Trace Stuff END   ---

        void BeginMarker(std::string_view name) override;
        void EndMarker() override;
        //  GPUAllocation AllocateGpu(size_t bufferSize, size_t stride) override;

        void TransitionBarrier(ITexture* texture, ResourceStates beforeState, ResourceStates afterState) override;
        void TransitionBarrier(IBuffer* buffer, ResourceStates beforeState, ResourceStates afterState) override;
        void TransitionBarriers(Core::Span<GpuBarrier> gpuBarriers) override;
        void ClearTextureFloat(ITexture* texture, Color const& clearColour) override;
        void ClearDepthStencilTexture(ITexture* depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil) override;

        void Draw(DrawArgs const& args) override;
        void DrawIndexed(DrawArgs const& args) override;
#if 0
        // void ExecuteIndirect(RHI::CommandSignatureHandle commandSignature, IBuffer* args, size_t argsOffsetInBytes, uint32_t maxCount) override;
        // void ExecuteIndirect(RHI::CommandSignatureHandle commandSignature, IBuffer* args, size_t argsOffsetInBytes, IBuffer* count, size_t countOffsetInBytes, uint32_t maxCount) override;

        void DrawIndirect(IBuffer* args, size_t argsOffsetInBytes, uint32_t maxCount) override;
        void DrawIndirect(IBuffer* args, size_t argsOffsetInBytes, IBuffer* count, size_t countOffsetInBytes, uint32_t maxCount) override;

        void DrawIndexedIndirect(IBuffer* args, size_t argsOffsetInBytes, uint32_t maxCount) override;
        void DrawIndexedIndirect(IBuffer* args, size_t argsOffsetInBytes, IBuffer* count, size_t countOffsetInBytes, uint32_t maxCount) override;
#endif
        void WriteBuffer(IBuffer* buffer, const void* Data, size_t dataSize, uint64_t destOffsetBytes) override;

        void CopyBuffer(IBuffer* dst, uint64_t dstOffset, IBuffer* src, uint64_t srcOffset, size_t sizeInBytes) override;

        void WriteTexture(ITexture* texture, uint32_t firstSubResource, size_t numSubResources, SubresourceData* pSubResourceData) override;
        void WriteTexture(ITexture* texture, uint32_t arraySlice, uint32_t mipLevel, const void* Data, size_t rowPitch, size_t depthPitch) override;

        void SetGfxPipeline(IGfxPipeline* gfxPipeline) override;
        void SetViewports(Viewport* viewports, size_t numViewports) override;
        void SetScissors(Rect* scissor, size_t numScissors) override;
        void SetRenderTargets(Core::Span<ITexture*> renderTargets, ITexture* depthStenc) override;

        // -- Comptute Stuff ---
        void SetComputeState(IComputePipeline* state) override;
        void Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) override;
        void DispatchIndirect(IBuffer* args, uint32_t argsOffsetInBytes, uint32_t maxCount) override;
        void DispatchIndirect(IBuffer* args, uint32_t argsOffsetInBytes, IBuffer* count, size_t countOffsetInBytes, uint32_t maxCount) override;

        // -- Mesh Stuff ---
        void SetMeshPipeline(IMeshPipeline* meshPipeline) override;
        void DispatchMesh(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) override;
        void DispatchMeshIndirect(IBuffer* args, uint32_t argsOffsetInBytes, uint32_t maxCount) override;
        void DispatchMeshIndirect(IBuffer* args, uint32_t argsOffsetInBytes, IBuffer* count, size_t countOffsetInBytes, uint32_t maxCount) override;

        void BindPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants) override;
        void BindConstantBuffer(size_t rootParameterIndex, IBuffer* constantBuffer) override;
        void BindDynamicConstantBuffer(size_t rootParameterIndex, size_t sizeInBytes, const void* bufferData) override;
        void BindVertexBuffer(uint32_t slot, IBuffer* vertexBuffer) override;

        /**
         * Set dynamic vertex buffer data to the rendering pipeline.
         */
        void BindDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData) override;
        void BindIndexBuffer(IBuffer* bufferHandle) override;

        /**
         * Bind dynamic index buffer data to the rendering pipeline.
         */
        void BindDynamicIndexBuffer(size_t numIndicies, RHI::Format indexFormat, const void* indexBufferData) override;
        /**
         * Set dynamic structured buffer contents.
         */
        void BindDynamicStructuredBuffer(uint32_t rootParameterIndex, size_t numElements, size_t elementSize, const void* bufferData) override;
        void BindStructuredBuffer(size_t rootParameterIndex, IBuffer* buffer) override;

        void BindDynamicDescriptorTable(size_t rootParameterIndex, Core::Span<ITexture*> textures) override;
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
        void BindDynamicUavDescriptorTable(
            size_t rootParameterIndex,
            Core::Span<IBuffer*> buffers,
            Core::Span<ITexture*> textures) override;

        void BindResourceTable(size_t rootParameterIndex) override;
        void BindSamplerTable(size_t rootParameterIndex) override;

        // void BeginTimerQuery(TimerQueryHandle query) override;
        // void EndTimerQuery(TimerQueryHandle query) override;
    };

    template<>
    struct TD3D12ResourceTraits<ICommandList>
    {
        typedef D3D12CommandList TConcreteType;
    };

}