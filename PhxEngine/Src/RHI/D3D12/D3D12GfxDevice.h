#pragma once

#include <memory>
#include <array>
#include <deque>
#include <tuple>
#include <unordered_map>
#include <functional>

#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Core/BitSetAllocator.h>
#include <PhxEngine/Core/Pool.h>
#include <PhxEngine/Core/Math.h>

#include "D3D12Common.h"
#include "D3D12CommandQueue.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12BiindlessDescriptorTable.h"
#include "D3D12UploadBuffer.h"

#include <D3D12MemAlloc.h>


// Teir 1 limit is 1,000,000
// https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
#define TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE 1000000

#define NUM_BINDLESS_RESOURCES TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE / 2

#define ENABLE_PIX_CAPUTRE 1

namespace PhxEngine::RHI::D3D12
{
    constexpr size_t kNumCommandListPerFrame = 32;
    constexpr size_t kResourcePoolSize = 100000; // 1 KB of handles
    constexpr size_t kNumConcurrentRenderTargets = 8;

    class D3D12GfxDevice;

    enum class DescriptorHeapTypes : uint8_t
    {
        CBV_SRV_UAV,
        Sampler,
        RTV,
        DSV,
        Count,
    };

    struct D3D12TimerQuery
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

    struct D3D12SwapChain
    {
        SwapChainDesc Desc;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> NativeSwapchain;
        Microsoft::WRL::ComPtr<IDXGISwapChain4> NativeSwapchain4;

        std::vector<TextureHandle> BackBuffers;
    };

    struct D3D12Shader
    {
        std::vector<uint8_t> ByteCode;
        const ShaderDesc Desc;
        Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> RootSignatureDeserializer;
        const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* RootSignatureDesc = nullptr;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;

        D3D12Shader() = default;
        D3D12Shader(ShaderDesc const& desc, const void* binary, size_t binarySize)
            : Desc(desc)
        {
            this->ByteCode.resize(binarySize);
            std::memcpy(ByteCode.data(), binary, binarySize);
        }
    };

    struct D3D12InputLayout
    {
        std::vector<VertexAttributeDesc> Attributes;
        std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements;

        D3D12InputLayout() = default;
    };

    struct D3D12GraphicsPipeline
    {
        GfxPipelineDesc Desc;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;

        D3D12GraphicsPipeline() = default;
    };

    struct D3D12ComputePipeline
    {
        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;
        ComputePipelineDesc Desc;

        D3D12ComputePipeline() = default;
    };

    struct D3D12MeshPipeline
    {
        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;
        MeshPipelineDesc Desc;

        D3D12MeshPipeline() = default;
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

    struct D3D12CommandSignature final
    {
        std::vector<D3D12_INDIRECT_ARGUMENT_DESC> D3D12Descs;
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> NativeSignature;
    };

    struct D3D12Texture final
    {
        TextureDesc Desc = {};
        Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> Allocation;

        UINT64 TotalSize = 0;
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> Footprints;
        std::vector<UINT64> RowSizesInBytes;
        std::vector<UINT> NumRows;

        // -- The views ---
        DescriptorView RtvAllocation;
        std::vector<DescriptorView> RtvSubresourcesAlloc = {};

        DescriptorView DsvAllocation;
        std::vector<DescriptorView> DsvSubresourcesAlloc = {};

        DescriptorView Srv;
        std::vector<DescriptorView> SrvSubresourcesAlloc = {};

        DescriptorView UavAllocation;
        std::vector<DescriptorView> UavSubresourcesAlloc = {};

        D3D12Texture() = default;

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
    };

    struct D3D12Buffer final
    {
        BufferDesc Desc = {};
        Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> Allocation;

        void* MappedData = nullptr;
        uint32_t MappedSizeInBytes = 0;

        // -- Views ---
        DescriptorView Srv;
        std::vector<DescriptorView> SrvSubresourcesAlloc = {};

        DescriptorView UavAllocation;
        std::vector<DescriptorView> UavSubresourcesAlloc = {};


        D3D12_VERTEX_BUFFER_VIEW VertexView = {};
        D3D12_INDEX_BUFFER_VIEW IndexView = {};


        const BufferDesc& GetDesc() const { return this->Desc; }

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
    };

    struct D3D12RTAccelerationStructure final
    {
        RTAccelerationStructureDesc Desc = {};
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Dx12Desc = {};
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> Geometries;
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO Info = {};
        BufferHandle SratchBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> Allocation;
        DescriptorView Srv;


        void DisposeViews()
        {
            Srv.Allocation.Free();
        }

        D3D12RTAccelerationStructure() = default;
    };

    struct D3D12RenderPass final
    {
        RenderPassDesc Desc = {};

        D3D12_RENDER_PASS_FLAGS D12RenderFlags = D3D12_RENDER_PASS_FLAG_NONE;

        size_t NumRenderTargets = 0;
        std::array<D3D12_RENDER_PASS_RENDER_TARGET_DESC, kNumConcurrentRenderTargets> RTVs = {};
        D3D12_RENDER_PASS_DEPTH_STENCIL_DESC DSV = {};

        std::vector<D3D12_RESOURCE_BARRIER> BarrierDescBegin;
        std::vector<D3D12_RESOURCE_BARRIER> BarrierDescEnd;

        D3D12RenderPass() = default;
    };

    struct D3D12DeviceBasicInfo final
    {
        uint32_t NumDeviceNodes;
    };

    class UploadBuffer;
    struct D3D12CommandList
    {
        uint32_t Id;
        CommandQueueType QueueType = CommandQueueType::Graphics;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> NativeCommandList;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> NativeCommandList6;
        ID3D12CommandAllocator* NativeCommandAllocator;
        UploadBuffer UploadBuffer;

        // An Array of Memory, perferanle from Allocator Internal
        std::atomic_bool IsWaitedOn;
        std::vector<CommandListHandle> Waits;

        // -- Supplimentry data
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> BuildGeometries;
        std::vector<D3D12_RESOURCE_BARRIER> BarrierMemoryPool;
        std::deque<Microsoft::WRL::ComPtr<ID3D12Resource>> TrackedResources;

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

        std::vector<RHI::TimerQueryHandle> TimerQueries;

        // -- Helper Functions ---
        void TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
        {
            CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                d3d12Resource.Get(),
                beforeState, afterState);

            // TODO: Batch Barrier
            this->NativeCommandList->ResourceBarrier(1, &barrier);
        }
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

	class D3D12GfxDevice final : public RHI::GfxDevice
	{
	public:
        D3D12GfxDevice();
		~D3D12GfxDevice();

        inline static D3D12GfxDevice* GPtr = nullptr;

        // -- Interface Functions ---
        // -- Frame Functions ---
    public:
        void Initialize(SwapChainDesc const& desc, void* windowHandle) override;
        void Finalize() override;

        // -- Resizes swapchain ---
        void ResizeSwapchain(SwapChainDesc const& desc) override;

        // -- Submits Command lists and presents ---
        void SubmitFrame() override;
        void WaitForIdle() override;

        bool IsDevicedRemoved() override;

        // -- Resouce Functions ---
    public:
        template<typename T>
        CommandSignatureHandle CreateCommandSignature(CommandSignatureDesc const& desc)
        {
            static_assert(sizeof(T) % sizeof(uint32_t) == 0);
            return this->CreateCommandSignature(desc, sizeof(T));
        }
        CommandSignatureHandle CreateCommandSignature(CommandSignatureDesc const& desc, size_t byteStride) override;
        ShaderHandle CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode) override;
        InputLayoutHandle CreateInputLayout(VertexAttributeDesc* desc, uint32_t attributeCount) override;
        GfxPipelineHandle CreateGfxPipeline(GfxPipelineDesc const& desc) override;
        ComputePipelineHandle CreateComputePipeline(ComputePipelineDesc const& desc) override;
        MeshPipelineHandle CreateMeshPipeline(MeshPipelineDesc const& desc) override;
        TextureHandle CreateTexture(TextureDesc const& desc) override;
        RenderPassHandle CreateRenderPass(RenderPassDesc const& desc) override;
        BufferHandle CreateBuffer(BufferDesc const& desc) override;
        int CreateSubresource(TextureHandle texture, SubresouceType subresourceType, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip = 0, uint32_t mpCount = ~0) override;
        int CreateSubresource(BufferHandle buffer, SubresouceType subresourceType, size_t offset, size_t size = ~0u) override;
        RTAccelerationStructureHandle CreateRTAccelerationStructure(RTAccelerationStructureDesc const& desc) override;
        TimerQueryHandle CreateTimerQuery() override;


        void DeleteCommandSignature(CommandSignatureHandle handle) override;
        const GfxPipelineDesc& GetGfxPipelineDesc(GfxPipelineHandle handle) override;
        void DeleteGfxPipeline(GfxPipelineHandle handle) override;


        void DeleteMeshPipeline(MeshPipelineHandle handle) override;

        const TextureDesc& GetTextureDesc(TextureHandle handle) override;
        DescriptorIndex GetDescriptorIndex(TextureHandle handle, SubresouceType type, int subResource = -1) override;
        void DeleteTexture(TextureHandle handle) override;

        void GetRenderPassFormats(RenderPassHandle handle, std::vector<RHI::Format>& outRtvFormats, RHI::Format& depthFormat) override;
        RenderPassDesc GetRenderPassDesc(RenderPassHandle) override;
        void DeleteRenderPass(RenderPassHandle handle) override;

        // -- TODO: End Remove

        const BufferDesc& GetBufferDesc(BufferHandle handle) override;
        DescriptorIndex GetDescriptorIndex(BufferHandle handle, SubresouceType type, int subResource = -1) override;

        template<typename T>
        T* GetBufferMappedData(BufferHandle handle)
        {
            return static_cast<T*>(this->GetBufferMappedData(handle));
        };

        void* GetBufferMappedData(BufferHandle handle) override;
        uint32_t GetBufferMappedDataSizeInBytes(BufferHandle handle) override;
        void DeleteBuffer(BufferHandle handle) override;

        // -- Ray Tracing ---
        size_t GetRTTopLevelAccelerationStructureInstanceSize() override;
        void WriteRTTopLevelAccelerationStructureInstance(RTAccelerationStructureDesc::TopLevelDesc::Instance const& instance, void* dest) override;
        const RTAccelerationStructureDesc& GetRTAccelerationStructureDesc(RTAccelerationStructureHandle handle) override;
        void DeleteRtAccelerationStructure(RTAccelerationStructureHandle handle) override;
        DescriptorIndex GetDescriptorIndex(RTAccelerationStructureHandle handle) override;

        // -- Query Stuff ---
        void DeleteTimerQuery(TimerQueryHandle query) override;
        bool PollTimerQuery(TimerQueryHandle query) override;
        Core::TimeStep GetTimerQueryTime(TimerQueryHandle query) override;
        void ResetTimerQuery(TimerQueryHandle query) override;

        // -- Utility ---
    public:
        TextureHandle GetBackBuffer() override;
        size_t GetBackBufferIndex() const;
        size_t GetNumBindlessDescriptors() const override { return NUM_BINDLESS_RESOURCES; }

        ShaderModel GetMinShaderModel() const override { return this->m_minShaderModel; };
        ShaderType GetShaderType() const override { return ShaderType::HLSL6; };
        GraphicsAPI GetApi() const override { return GraphicsAPI::DX12; }

        void BeginCapture(std::wstring const& filename) override;
        void EndCapture(bool discard = false) override;

        size_t GetFrameIndex() override { return this->GetBackBufferIndex(); };
        size_t GetMaxInflightFrames() override { return this->m_swapChain.Desc.BufferCount; }
        bool CheckCapability(DeviceCapability deviceCapability) override;

        float GetAvgFrameTime() override;

        // float GetAvgFrameTime() override { return this->m_frameStats.GetAvg(); };
        uint64_t GetUavCounterPlacementAlignment() { return D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT; }
        // -- Command list Functions ---
        MemoryUsage GetMemoryUsage() const override
        {
            MemoryUsage retval;
            D3D12MA::Budget budget;
            this->m_d3d12MemAllocator->GetBudget(&budget, nullptr);
            retval.Budget = budget.BudgetBytes;
            retval.Usage = budget.UsageBytes;
            return retval;
        }

        // These are not thread Safe
    public:
        // TODO: Change to a new pattern so we don't require a command list stored on an object. Instread, request from a pool of objects
        CommandListHandle BeginCommandList(CommandQueueType queueType = CommandQueueType::Graphics) override;
        void WaitCommandList(CommandListHandle cmd, CommandListHandle WaitOn) override;

        // -- Ray Trace stuff       ---
        void RTBuildAccelerationStructure(RHI::RTAccelerationStructureHandle accelStructure, CommandListHandle cmd) override;

        // -- Ray Trace Stuff END   ---
        void BeginMarker(std::string_view name, CommandListHandle cmd) override;
        void EndMarker(CommandListHandle cmd) override;
        GPUAllocation AllocateGpu(size_t bufferSize, size_t stride, CommandListHandle cmd) override;

        void TransitionBarrier(TextureHandle texture, ResourceStates beforeState, ResourceStates afterState, CommandListHandle cmd) override;
        void TransitionBarrier(BufferHandle buffer, ResourceStates beforeState, ResourceStates afterState, CommandListHandle cmd) override;
        void TransitionBarriers(Core::Span<GpuBarrier> gpuBarriers, CommandListHandle cmd) override;
        void ClearTextureFloat(TextureHandle texture, Color const& clearColour, CommandListHandle cmd) override;
        void ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil, CommandListHandle cmd) override;

        void Draw(DrawArgs const& args, CommandListHandle cmd) override;
        void DrawIndexed(DrawArgs const& args, CommandListHandle cmd) override;

        void ExecuteIndirect(RHI::CommandSignatureHandle commandSignature, RHI::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) override;
        void ExecuteIndirect(RHI::CommandSignatureHandle commandSignature, RHI::BufferHandle args, size_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) override;

        void DrawIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) override;
        void DrawIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) override;

        void DrawIndexedIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) override;
        void DrawIndexedIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) override;

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

        void WriteBuffer(BufferHandle buffer, const void* Data, size_t dataSize, uint64_t destOffsetBytes, CommandListHandle cmd) override;

        void CopyBuffer(BufferHandle dst, uint64_t dstOffset, BufferHandle src, uint64_t srcOffset, size_t sizeInBytes, CommandListHandle cmd) override;

        void WriteTexture(TextureHandle texture, uint32_t firstSubResource, size_t numSubResources, SubresourceData* pSubResourceData, CommandListHandle cmd) override;
        void WriteTexture(TextureHandle texture, uint32_t arraySlice, uint32_t mipLevel, const void* Data, size_t rowPitch, size_t depthPitch, CommandListHandle cmd) override;

        void SetGfxPipeline(GfxPipelineHandle gfxPipeline, CommandListHandle cmd) override;
        void SetViewports(Viewport* viewports, size_t numViewports, CommandListHandle cmd) override;
        void SetScissors(Rect* scissor, size_t numScissors, CommandListHandle cmd) override;
        void SetRenderTargets(Core::Span<TextureHandle> renderTargets, TextureHandle depthStenc, CommandListHandle cmd) override;

        // -- Comptute Stuff ---
        void SetComputeState(ComputePipelineHandle state, CommandListHandle cmd) override;
        void Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ, CommandListHandle cmd) override;
        void DispatchIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) override;
        void DispatchIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) override;

        // -- Mesh Stuff ---
        void SetMeshPipeline(MeshPipelineHandle meshPipeline, CommandListHandle cmd) override;
        void DispatchMesh(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ, CommandListHandle cmd) override;
        void DispatchMeshIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) override;
        void DispatchMeshIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd) override;

        void BindPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants, CommandListHandle cmd) override;
        template<typename T>
        void BindPushConstant(uint32_t rootParameterIndex, const T& constants, CommandListHandle cmd)
        {
            static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be a multiple of 4 bytes");
            this->BindPushConstant(rootParameterIndex, sizeof(T), &constants, cmd);
        }

        void BindConstantBuffer(size_t rootParameterIndex, BufferHandle constantBuffer, CommandListHandle cmd) override;
        void BindDynamicConstantBuffer(size_t rootParameterIndex, size_t sizeInBytes, const void* bufferData, CommandListHandle cmd) override;
        template<typename T>
        void BindDynamicConstantBuffer(size_t rootParameterIndex, T const& bufferData, CommandListHandle cmd)
        {
            this->BindDynamicConstantBuffer(rootParameterIndex, sizeof(T), &bufferData, cmd);
        }

        void BindVertexBuffer(uint32_t slot, BufferHandle vertexBuffer, CommandListHandle cmd) override;

        /**
         * Set dynamic vertex buffer data to the rendering pipeline.
         */
        void BindDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData, CommandListHandle cmd) override;

        template<typename T>
        void BindDynamicVertexBuffer(uint32_t slot, const std::vector<T>& vertexBufferData, CommandListHandle cmd)
        {
            this->BindDynamicVertexBuffer(slot, vertexBufferData.size(), sizeof(T), vertexBufferData.data(), cmd);
        }

        void BindIndexBuffer(BufferHandle bufferHandle, CommandListHandle cmd) override;

        /**
         * Bind dynamic index buffer data to the rendering pipeline.
         */
        void BindDynamicIndexBuffer(size_t numIndicies, RHI::Format indexFormat, const void* indexBufferData, CommandListHandle cmd) override;

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
        void BindDynamicStructuredBuffer(uint32_t rootParameterIndex, size_t numElements, size_t elementSize, const void* bufferData, CommandListHandle cmd) override;

        template<typename T>
        void BindDynamicStructuredBuffer(uint32_t rootParameterIndex, std::vector<T> const& bufferData, CommandListHandle cmd)
        {
            this->BindDynamicStructuredBuffer(rootParameterIndex, bufferData.size(), sizeof(T), bufferData.Data(), cmd);
        }

        void BindStructuredBuffer(size_t rootParameterIndex, BufferHandle buffer, CommandListHandle cmd) override;

        void BindDynamicDescriptorTable(size_t rootParameterIndex, Core::Span<TextureHandle> textures, CommandListHandle cmd) override;
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
        void BindDynamicUavDescriptorTable(
            size_t rootParameterIndex,
            Core::Span<BufferHandle> buffers,
            Core::Span<TextureHandle> textures,
            CommandListHandle cmd) override;

        void BindResourceTable(size_t rootParameterIndex, CommandListHandle cmd) override;
        void BindSamplerTable(size_t rootParameterIndex, CommandListHandle cmd) override;

        void BeginTimerQuery(TimerQueryHandle query, CommandListHandle cmd) override;
        void EndTimerQuery(TimerQueryHandle query, CommandListHandle cmd) override;

        // -- Dx12 Specific functions ---
    public:
        void DeleteD3DResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource);

        const D3D12SwapChain& GetSwapChain() const { return this->m_swapChain; }
        TextureHandle CreateRenderTarget(TextureDesc const& desc, Microsoft::WRL::ComPtr<ID3D12Resource> d3d12TextureResource);
        TextureHandle CreateTexture(TextureDesc const& desc, Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource);

        int CreateShaderResourceView(BufferHandle buffer, size_t offset, size_t size);
        int CreateUnorderedAccessView(BufferHandle buffer, size_t offset, size_t size);

        int CreateShaderResourceView(TextureHandle texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);
        int CreateRenderTargetView(TextureHandle texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);
        int CreateDepthStencilView(TextureHandle texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);
        int CreateUnorderedAccessView(TextureHandle texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);

    public:
        void CreateBackBuffers(D3D12SwapChain* viewport);
        void RunGarbageCollection(uint64_t completedFrame);

        // -- Getters ---
    public:
        Microsoft::WRL::ComPtr<ID3D12Device> GetD3D12Device() { return this->m_rootDevice; }
        Microsoft::WRL::ComPtr<ID3D12Device2> GetD3D12Device2() { return this->m_rootDevice2; }
        Microsoft::WRL::ComPtr<ID3D12Device5> GetD3D12Device5() { return this->m_rootDevice5; }

        Microsoft::WRL::ComPtr<IDXGIFactory6> GetDxgiFactory() { return this->m_factory; }
        Microsoft::WRL::ComPtr<IDXGIAdapter> GetDxgiAdapter() { return this->m_gpuAdapter.NativeAdapter; }

    public:
        D3D12CommandQueue& GetGfxQueue() { return this->GetQueue(CommandQueueType::Graphics); }
        D3D12CommandQueue& GetComputeQueue() { return this->GetQueue(CommandQueueType::Compute); }
        D3D12CommandQueue& GetCopyQueue() { return this->GetQueue(CommandQueueType::Copy); }

        D3D12CommandQueue& GetQueue(CommandQueueType type) { return this->m_commandQueues[(int)type]; }

        // -- Command Signatures ---
        ID3D12CommandSignature* GetDispatchIndirectCommandSignature() const{ return this->m_dispatchIndirectCommandSignature.Get(); }
        ID3D12CommandSignature* GetDrawInstancedIndirectCommandSignature() const { return this->m_drawInstancedIndirectCommandSignature.Get(); }
        ID3D12CommandSignature* GetDrawIndexedInstancedIndirectCommandSignature() const{ return this->m_drawIndexedInstancedIndirectCommandSignature.Get(); }
        ID3D12CommandSignature* GetDispatchMeshIndirectCommandSignature() const { return this->m_dispatchMeshIndirectCommandSignature.Get(); }

        CpuDescriptorHeap& GetResourceCpuHeap() { return this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV]; }
        CpuDescriptorHeap& GetSamplerCpuHeap() { return this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler]; }
        CpuDescriptorHeap& GetRtvCpuHeap() { return this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::RTV]; }
        CpuDescriptorHeap& GetDsvCpuHeap() { return this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::DSV]; }

        GpuDescriptorHeap& GetResourceGpuHeap() { return this->m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV]; }
        GpuDescriptorHeap& GetSamplerGpuHeap() { return this->m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler]; }

        ID3D12QueryHeap* GetQueryHeap() { return this->m_gpuTimestampQueryHeap.Get(); }
        BufferHandle GetTimestampQueryBuffer() { return this->m_timestampQueryBuffer; }
        const D3D12BindlessDescriptorTable& GetBindlessTable() const { return this->m_bindlessResourceDescriptorTable; }

        // Maybe better encapulate this.
        Core::Pool<D3D12Texture, Texture>& GetTexturePool() { return this->m_texturePool; };
        Core::Pool<D3D12Buffer, Buffer>& GetBufferPool() { return this->m_bufferPool; };
        Core::Pool<D3D12RenderPass, RenderPass>& GetRenderPassPool() { return this->m_renderPassPool; };
        Core::Pool<D3D12GraphicsPipeline, GfxPipeline>& GetGraphicsPipelinePool() { return this->m_gfxPipelinePool; }
        Core::Pool<D3D12ComputePipeline, ComputePipeline>& GetComputePipelinePool() { return this->m_computePipelinePool; }
        Core::Pool<D3D12MeshPipeline, MeshPipeline>& GetMeshPipelinePool() { return this->m_meshPipelinePool; }
        Core::Pool<D3D12RTAccelerationStructure, RTAccelerationStructure>& GetRTAccelerationStructurePool() { return this->m_rtAccelerationStructurePool; }
        Core::Pool<D3D12CommandSignature, CommandSignature>& GetCommandSignaturePool() { return this->m_commandSignaturePool; }
        Core::Pool<D3D12TimerQuery, TimerQuery>& GetTimerQueryPool() { return this->m_timerQueryPool; }

    private:
        bool IsHdrSwapchainSupported();

    private:
        void CreateBufferInternal(BufferDesc const& desc, D3D12Buffer& outBuffer);

        void CreateGpuTimestampQueryHeap(uint32_t queryCount);

        // -- Dx12 API creation ---
    private:
        void InitializeResourcePools();

        void InitializeD3D12NativeResources(IDXGIAdapter* gpuAdapter);
        void CreateSwapChain(SwapChainDesc const& desc, void* windowHandle);

        void FindAdapter(Microsoft::WRL::ComPtr<IDXGIFactory6> factory, D3D12Adapter& outAdapter);

         // -- Pipeline state conversion --- 
    private:
        void TranslateBlendState(BlendRenderState const& inState, D3D12_BLEND_DESC& outState);
        void TranslateDepthStencilState(DepthStencilRenderState const& inState, D3D12_DEPTH_STENCIL_DESC& outState);
        void TranslateRasterState(RasterRenderState const& inState, D3D12_RASTERIZER_DESC& outState);

	private:
        const uint32_t kTimestampQueryHeapSize = 1024;
        
		Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
		Microsoft::WRL::ComPtr<ID3D12Device> m_rootDevice;
		Microsoft::WRL::ComPtr<ID3D12Device2> m_rootDevice2;
		Microsoft::WRL::ComPtr<ID3D12Device5> m_rootDevice5;

        // -- Command Signatures ---
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_dispatchIndirectCommandSignature;
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_drawInstancedIndirectCommandSignature;
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_drawIndexedInstancedIndirectCommandSignature;
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_dispatchMeshIndirectCommandSignature;

        Microsoft::WRL::ComPtr<D3D12MA::Allocator> m_d3d12MemAllocator;

		// std::shared_ptr<IDxcUtils> dxcUtils;
		D3D12Adapter m_gpuAdapter;

		D3D12_FEATURE_DATA_ROOT_SIGNATURE FeatureDataRootSignature = {};
		D3D12_FEATURE_DATA_SHADER_MODEL   FeatureDataShaderModel = {};
        ShaderModel m_minShaderModel = ShaderModel::SM_6_0;

		bool IsUnderGraphicsDebugger = false;
        RHI::DeviceCapability m_capabilities;

        D3D12SwapChain m_swapChain;

        // -- Resouce Pool ---
        Core::Pool<D3D12Texture, Texture> m_texturePool;
        Core::Pool<D3D12CommandSignature, CommandSignature> m_commandSignaturePool;
        Core::Pool<D3D12Shader, RHI::Shader> m_shaderPool;
        Core::Pool<D3D12InputLayout, InputLayout> m_inputLayoutPool;
        Core::Pool<D3D12Buffer, Buffer> m_bufferPool;
        Core::Pool<D3D12RenderPass, RenderPass> m_renderPassPool;
        Core::Pool<D3D12RTAccelerationStructure, RTAccelerationStructure> m_rtAccelerationStructurePool;
        Core::Pool<D3D12GraphicsPipeline, GfxPipeline> m_gfxPipelinePool;
        Core::Pool<D3D12ComputePipeline, ComputePipeline> m_computePipelinePool;
        Core::Pool<D3D12MeshPipeline, MeshPipeline> m_meshPipelinePool;
        Core::Pool<D3D12TimerQuery, TimerQuery> m_timerQueryPool;
        Core::Pool<D3D12CommandList, CommandList> m_commandListPool;

        // -- Command Queues ---
		std::array<D3D12CommandQueue, (int)CommandQueueType::Count> m_commandQueues;

        // -- Command lists ---
        std::atomic_uint32_t m_activeCmdCount;
        std::array<CommandListHandle, kNumCommandListPerFrame> m_frameCommandListHandles;

        // -- Descriptor Heaps ---
		std::array<CpuDescriptorHeap, (int)DescriptorHeapTypes::Count> m_cpuDescriptorHeaps;
        std::array<GpuDescriptorHeap, 2> m_gpuDescriptorHeaps;

        // -- Query Heaps ---
        Microsoft::WRL::ComPtr<ID3D12QueryHeap> m_gpuTimestampQueryHeap;
        BufferHandle m_timestampQueryBuffer;
        Core::BitSetAllocator m_timerQueryIndexPool;

        D3D12BindlessDescriptorTable m_bindlessResourceDescriptorTable;

        // -- Frame Frences --
        std::array<Microsoft::WRL::ComPtr<ID3D12Fence>, (int)CommandQueueType::Count> m_frameFences;
        uint64_t m_frameCount = 1;

        struct DeleteItem
        {
            uint64_t Frame;
            std::function<void()> DeleteFn;
        };
        std::deque<DeleteItem> m_deleteQueue;

#ifdef ENABLE_PIX_CAPUTRE
        HMODULE m_pixCaptureModule;
#endif
	};

}