#pragma once

#include "phxGfxCommonResources.h"
#include "phxGfxResources.h"
#include "phxGfxDescriptorHeap.h"

#include "phxSpan.h"

#include <mutex>

namespace phx::gfx
{

    enum class DescriptorHeapTypes : uint8_t
    {
        CBV_SRV_UAV,
        Sampler,
        RTV,
        DSV,
        Count,
    };

    struct D3D12CommandQueue final
    {
        D3D12_COMMAND_LIST_TYPE m_type;

        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_platformQueue;
        Microsoft::WRL::ComPtr<ID3D12Fence> m_platformFence;

        std::mutex m_commandListMutx;

        uint64_t m_nextFenceValue = 0;
        uint64_t m_lastCompletedFenceValue = 0;

        std::mutex m_fenceMutex;
        std::mutex m_eventMutex;
        HANDLE m_fenceEvent;

        std::vector<ID3D12CommandList*> m_pendingCmdLists;
        std::vector<ID3D12CommandAllocator*> m_pendingAllocators;

        void Initialize(ID3D12Device* nativeDevice, D3D12_COMMAND_LIST_TYPE type);
        void Finailize();

        D3D12_COMMAND_LIST_TYPE GetType() const { return this->m_type; }

        void EnqueueCommandList(ID3D12CommandList* cmdList, ID3D12CommandAllocator* allocator)
        {
            this->m_pendingCmdLists.push_back(cmdList);
            this->m_pendingAllocators.push_back(allocator);
        }

        uint64_t Submit();

        ID3D12CommandAllocator* RequestAllocator();
        void DiscardAllocators(uint64_t fence, Span<ID3D12CommandAllocator*> allocators);
        void DiscardAllocator(uint64_t fence, ID3D12CommandAllocator* allocator);

        uint64_t IncrementFence();
        uint64_t GetNextFenceValue() { return this->m_nextFenceValue + 1; }
        bool IsFenceComplete(uint64_t fenceValue);
        void WaitForFence(uint64_t fenceValue);
        void WaitForIdle() { this->WaitForFence(this->IncrementFence()); }
    };

    using CommandQueue = D3D12CommandQueue;

    struct D3D12DeviceBasicInfo final
    {
        uint32_t NumDeviceNodes;
    };

    struct D3D12GpuAdapter final
    {
        std::wstring Name;
        size_t DedicatedSystemMemory = 0;
        size_t DedicatedVideoMemory = 0;
        size_t SharedSystemMemory = 0;
        D3D12DeviceBasicInfo BasicDeviceInfo;
        DXGI_ADAPTER_DESC PlatformDesc;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> PlatformAdapter;

        static HRESULT EnumAdapters(uint32_t adapterIndex, IDXGIFactory6* factory6, IDXGIAdapter1** outAdapter);
    };

    using GpuAdpater = D3D12GpuAdapter;

    class D3D12Device final
    {
    public:
        inline static D3D12Device* Ptr = nullptr;

    public:
        D3D12Device() = default;
        ~D3D12Device() { this->Finalize(); }

        void Initialize();
        void Finalize();

        // -- Factory methods ---
    public:
        void Create(SwapChainDesc const& desc, SwapChain& out);


        // -- Getters ---
    public:
        Microsoft::WRL::ComPtr<ID3D12Device> GetPlatformDevice() { return this->m_platformDevice; }
        Microsoft::WRL::ComPtr<ID3D12Device2> GetPlatformDevice2() { return this->m_platformDevice2; }
        Microsoft::WRL::ComPtr<ID3D12Device5> GetPlatformDevice5() { return this->m_platformDevice5; }

        Microsoft::WRL::ComPtr<IDXGIFactory6> GetPlatformFactory() { return this->m_factory; }
        Microsoft::WRL::ComPtr<IDXGIAdapter> GetPlatformGpuAdapter() { return this->m_gpuAdapter.PlatformAdapter; }

        D3D12CommandQueue& GetGfxQueue() { return this->GetQueue(CommandQueueType::Graphics); }
        D3D12CommandQueue& GetComputeQueue() { return this->GetQueue(CommandQueueType::Compute); }
        D3D12CommandQueue& GetCopyQueue() { return this->GetQueue(CommandQueueType::Copy); }

        D3D12CommandQueue& GetQueue(CommandQueueType type) { return this->m_commandQueues[(int)type]; }

    private:
        void InitializeD3D12();
        void FindAdapter();

    private:
        GpuAdpater m_gpuAdapter;
        Microsoft::WRL::ComPtr<IDXGIFactory6>   m_factory;
        Microsoft::WRL::ComPtr<ID3D12Device>    m_platformDevice;
        Microsoft::WRL::ComPtr<ID3D12Device2>   m_platformDevice2;
        Microsoft::WRL::ComPtr<ID3D12Device5>   m_platformDevice5;


        D3D12_FEATURE_DATA_ROOT_SIGNATURE FeatureDataRootSignature = {};
        D3D12_FEATURE_DATA_SHADER_MODEL   FeatureDataShaderModel = {};
        ShaderModel m_minShaderModel = ShaderModel::SM_6_0;
        gfx::DeviceCapability m_capabilities;


        // -- Command Queues ---
        EnumArray<CommandQueueType, D3D12CommandQueue> m_commandQueues;

        // -- Descriptor Heaps ---
        EnumArray<DescriptorHeapTypes, CpuDescriptorHeap> m_cpuDescriptorHeaps;
        std::array<GpuDescriptorHeap, 2> m_gpuDescriptorHeaps;

    };

    using Device = D3D12Device;
#if false
    class D3D12GfxDevice final
    {
    public:
        D3D12GfxDevice();
        ~D3D12GfxDevice();

        inline static D3D12GfxDevice* Ptr = nullptr;

        // -- Interface Functions ---
        // -- Frame Functions ---
    public:
        void Initialize();
        void Finalize();

        // -- Resizes swapchain ---
        void ResizeSwapchain(uint32_t width, uint32_t height);
        void ResizeSwapchain(SwapChainDesc const& desc);

        // -- Submits Command lists and presents ---
        void SubmitFrame();
        void WaitForIdle();

        bool IsDevicedRemoved();

        // -- Resouce Functions ---
    public:
        template<typename T>
        CommandSignatureHandle CreateCommandSignature(CommandSignatureDesc const& desc)
        {
            static_assert(sizeof(T) % sizeof(uint32_t) == 0);
            return this->CreateCommandSignature(desc, sizeof(T));
        }
        CommandSignatureHandle CreateCommandSignature(CommandSignatureDesc const& desc, size_t byteStride);
        ShaderHandle CreateShader(ShaderDesc const& desc, Span<uint8_t> shaderByteCode);
        InputLayoutHandle CreateInputLayout(VertexAttributeDesc* desc, uint32_t attributeCount);
        GfxPipelineHandle CreateGfxPipeline(GfxPipelineDesc const& desc);
        ComputePipelineHandle CreateComputePipeline(ComputePipelineDesc const& desc);
        MeshPipelineHandle CreateMeshPipeline(MeshPipelineDesc const& desc);
        TextureHandle CreateTexture(TextureDesc const& desc);
        RenderPassHandle CreateRenderPass(RenderPassDesc const& desc);
        BufferHandle CreateBuffer(BufferDesc const& desc);
        int CreateSubresource(TextureHandle texture, SubresouceType subresourceType, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip = 0, uint32_t mpCount = ~0);
        int CreateSubresource(BufferHandle buffer, SubresouceType subresourceType, size_t offset, size_t size = ~0u);
        RTAccelerationStructureHandle CreateRTAccelerationStructure(RTAccelerationStructureDesc const& desc);
        TimerQueryHandle CreateTimerQuery();


        void DeleteCommandSignature(CommandSignatureHandle handle);
        void DeleteGfxPipeline(GfxPipelineHandle handle);


        void DeleteMeshPipeline(MeshPipelineHandle handle);

        const TextureDesc& GetTextureDesc(TextureHandle handle);
        DescriptorIndex GetDescriptorIndex(TextureHandle handle, SubresouceType type, int subResource = -1);
        void DeleteTexture(TextureHandle handle);

        void GetRenderPassFormats(RenderPassHandle handle, std::vector<rhi::Format>& outRtvFormats, rhi::Format& depthFormat);
        RenderPassDesc GetRenderPassDesc(RenderPassHandle);
        void DeleteRenderPass(RenderPassHandle handle);

        // -- TODO: End Remove

        const BufferDesc& GetBufferDesc(BufferHandle handle);
        DescriptorIndex GetDescriptorIndex(BufferHandle handle, SubresouceType type, int subResource = -1);

        template<typename T>
        T* GetBufferMappedData(BufferHandle handle)
        {
            return static_cast<T*>(this->GetBufferMappedData(handle));
        };

        void* GetBufferMappedData(BufferHandle handle);
        uint32_t GetBufferMappedDataSizeInBytes(BufferHandle handle);
        void DeleteBuffer(BufferHandle handle);

        // -- Ray Tracing ---
        size_t GetRTTopLevelAccelerationStructureInstanceSize();
        void WriteRTTopLevelAccelerationStructureInstance(RTAccelerationStructureDesc::TopLevelDesc::Instance const& instance, void* dest);
        const RTAccelerationStructureDesc& GetRTAccelerationStructureDesc(RTAccelerationStructureHandle handle);
        void DeleteRtAccelerationStructure(RTAccelerationStructureHandle handle);
        DescriptorIndex GetDescriptorIndex(RTAccelerationStructureHandle handle);

        // -- Query Stuff ---
        void DeleteTimerQuery(TimerQueryHandle query);
        bool PollTimerQuery(TimerQueryHandle query);
        TimeStep GetTimerQueryTime(TimerQueryHandle query);
        void ResetTimerQuery(TimerQueryHandle query);

        // -- Utility ---
    public:
        TextureHandle GetBackBuffer();
        size_t GetBackBufferIndex() const;
        size_t GetNumBindlessDescriptors() const override { return NUM_BINDLESS_RESOURCES; }

        ShaderModel GetMinShaderModel() const override { return this->m_minShaderModel; };
        ShaderType GetShaderType() const override { return ShaderType::HLSL6; };
        GraphicsAPI GetApi() const override { return GraphicsAPI::DX12; }

        void BeginCapture(std::wstring const& filename);
        void EndCapture(bool discard = false);

        size_t GetFrameIndex() override { return this->GetBackBufferIndex(); };
        size_t GetMaxInflightFrames() override { return this->m_swapChain.Desc.BufferCount; }
        bool CheckCapability(DeviceCapability deviceCapability);

        float GetAvgFrameTime();

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
        CommandListHandle BeginCommandList(CommandQueueType queueType = CommandQueueType::Graphics);
        void WaitCommandList(CommandListHandle cmd, CommandListHandle WaitOn);

        // -- Ray Trace stuff       ---
        void RTBuildAccelerationStructure(rhi::RTAccelerationStructureHandle accelStructure, CommandListHandle cmd);

        // -- Ray Trace Stuff END   ---
        void BeginMarker(std::string_view name, CommandListHandle cmd);
        void EndMarker(CommandListHandle cmd);
        GPUAllocation AllocateGpu(size_t bufferSize, size_t stride, CommandListHandle cmd);

        void TransitionBarrier(TextureHandle texture, ResourceStates beforeState, ResourceStates afterState, CommandListHandle cmd);
        void TransitionBarrier(BufferHandle buffer, ResourceStates beforeState, ResourceStates afterState, CommandListHandle cmd);
        void TransitionBarriers(Span<GpuBarrier> gpuBarriers, CommandListHandle cmd);
        void ClearTextureFloat(TextureHandle texture, Color const& clearColour, CommandListHandle cmd);
        void ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil, CommandListHandle cmd);

        void Draw(DrawArgs const& args, CommandListHandle cmd);
        void DrawIndexed(DrawArgs const& args, CommandListHandle cmd);

        void ExecuteIndirect(rhi::CommandSignatureHandle commandSignature, rhi::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);
        void ExecuteIndirect(rhi::CommandSignatureHandle commandSignature, rhi::BufferHandle args, size_t argsOffsetInBytes, rhi::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);

        void DrawIndirect(rhi::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);
        void DrawIndirect(rhi::BufferHandle args, size_t argsOffsetInBytes, rhi::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);

        void DrawIndexedIndirect(rhi::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);
        void DrawIndexedIndirect(rhi::BufferHandle args, size_t argsOffsetInBytes, rhi::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);

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
        void WriteBuffer(BufferHandle buffer, Span<T> data, uint64_t destOffsetBytes, CommandListHandle cmd)
        {
            this->WriteBuffer(buffer, data.begin(), sizeof(T) * data.Size(), destOffsetBytes, cmd);
        }

        void WriteBuffer(BufferHandle buffer, const void* Data, size_t dataSize, uint64_t destOffsetBytes, CommandListHandle cmd);

        void CopyBuffer(BufferHandle dst, uint64_t dstOffset, BufferHandle src, uint64_t srcOffset, size_t sizeInBytes, CommandListHandle cmd);

        void WriteTexture(TextureHandle texture, uint32_t firstSubResource, size_t numSubResources, SubresourceData* pSubResourceData, CommandListHandle cmd);
        void WriteTexture(TextureHandle texture, uint32_t arraySlice, uint32_t mipLevel, const void* Data, size_t rowPitch, size_t depthPitch, CommandListHandle cmd);

        void SetGfxPipeline(GfxPipelineHandle gfxPipeline, CommandListHandle cmd);
        void SetViewports(Viewport* viewports, size_t numViewports, CommandListHandle cmd);
        void SetScissors(Rect* scissor, size_t numScissors, CommandListHandle cmd);
        void SetRenderTargets(Span<TextureHandle> renderTargets, TextureHandle depthStenc, CommandListHandle cmd);

        // -- Comptute Stuff ---
        void SetComputeState(ComputePipelineHandle state, CommandListHandle cmd);
        void Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ, CommandListHandle cmd);
        void DispatchIndirect(rhi::BufferHandle args, uint32_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);
        void DispatchIndirect(rhi::BufferHandle args, uint32_t argsOffsetInBytes, rhi::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);

        // -- Mesh Stuff ---
        void SetMeshPipeline(MeshPipelineHandle meshPipeline, CommandListHandle cmd);
        void DispatchMesh(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ, CommandListHandle cmd);
        void DispatchMeshIndirect(rhi::BufferHandle args, uint32_t argsOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);
        void DispatchMeshIndirect(rhi::BufferHandle args, uint32_t argsOffsetInBytes, rhi::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount, CommandListHandle cmd);

        void BindPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants, CommandListHandle cmd);
        template<typename T>
        void BindPushConstant(uint32_t rootParameterIndex, const T& constants, CommandListHandle cmd)
        {
            static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be a multiple of 4 bytes");
            this->BindPushConstant(rootParameterIndex, sizeof(T), &constants, cmd);
        }

        void BindConstantBuffer(size_t rootParameterIndex, BufferHandle constantBuffer, CommandListHandle cmd);
        void BindDynamicConstantBuffer(size_t rootParameterIndex, size_t sizeInBytes, const void* bufferData, CommandListHandle cmd);
        template<typename T>
        void BindDynamicConstantBuffer(size_t rootParameterIndex, T const& bufferData, CommandListHandle cmd)
        {
            this->BindDynamicConstantBuffer(rootParameterIndex, sizeof(T), &bufferData, cmd);
        }

        void BindVertexBuffer(uint32_t slot, BufferHandle vertexBuffer, CommandListHandle cmd);

        /**
         * Set dynamic vertex buffer data to the rendering pipeline.
         */
        void BindDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData, CommandListHandle cmd);

        template<typename T>
        void BindDynamicVertexBuffer(uint32_t slot, const std::vector<T>& vertexBufferData, CommandListHandle cmd)
        {
            this->BindDynamicVertexBuffer(slot, vertexBufferData.size(), sizeof(T), vertexBufferData.data(), cmd);
        }

        void BindIndexBuffer(BufferHandle bufferHandle, CommandListHandle cmd);

        /**
         * Bind dynamic index buffer data to the rendering pipeline.
         */
        void BindDynamicIndexBuffer(size_t numIndicies, rhi::Format indexFormat, const void* indexBufferData, CommandListHandle cmd);

        template<typename T>
        void BindDynamicIndexBuffer(const std::vector<T>& indexBufferData, CommandListHandle cmd)
        {
            staticassert(sizeof(T) == 2 || sizeof(T) == 4);

            rhi::Format indexFormat = (sizeof(T) == 2) ? rhi::Format::R16_UINT : rhi::Format::R32_UINT;
            this->BindDynamicIndexBuffer(indexBufferData.size(), indexFormat, indexBufferData.Data(), cmd);
        }

        /**
         * Set dynamic structured buffer contents.
         */
        void BindDynamicStructuredBuffer(uint32_t rootParameterIndex, size_t numElements, size_t elementSize, const void* bufferData, CommandListHandle cmd);

        template<typename T>
        void BindDynamicStructuredBuffer(uint32_t rootParameterIndex, std::vector<T> const& bufferData, CommandListHandle cmd)
        {
            this->BindDynamicStructuredBuffer(rootParameterIndex, bufferData.size(), sizeof(T), bufferData.Data(), cmd);
        }

        void BindStructuredBuffer(size_t rootParameterIndex, BufferHandle buffer, CommandListHandle cmd);

        void BindDynamicDescriptorTable(size_t rootParameterIndex, Span<TextureHandle> textures, CommandListHandle cmd);
        void BindDynamicUavDescriptorTable(
            size_t rootParameterIndex,
            Span<BufferHandle> buffers,
            CommandListHandle cmd)
        {
            this->BindDynamicUavDescriptorTable(rootParameterIndex, buffers, {}, cmd);
        }
        void BindDynamicUavDescriptorTable(
            size_t rootParameterIndex,
            Span<TextureHandle> textures,
            CommandListHandle cmd)
        {
            this->BindDynamicUavDescriptorTable(rootParameterIndex, {}, textures, cmd);
        }
        void BindDynamicUavDescriptorTable(
            size_t rootParameterIndex,
            Span<BufferHandle> buffers,
            Span<TextureHandle> textures,
            CommandListHandle cmd);

        void BindResourceTable(size_t rootParameterIndex, CommandListHandle cmd);
        void BindSamplerTable(size_t rootParameterIndex, CommandListHandle cmd);

        void BeginTimerQuery(TimerQueryHandle query, CommandListHandle cmd);
        void EndTimerQuery(TimerQueryHandle query, CommandListHandle cmd);

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
        ID3D12CommandSignature* GetDispatchIndirectCommandSignature() const { return this->m_dispatchIndirectCommandSignature.Get(); }
        ID3D12CommandSignature* GetDrawInstancedIndirectCommandSignature() const { return this->m_drawInstancedIndirectCommandSignature.Get(); }
        ID3D12CommandSignature* GetDrawIndexedInstancedIndirectCommandSignature() const { return this->m_drawIndexedInstancedIndirectCommandSignature.Get(); }
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
        HandlePool<D3D12Texture, Texture>& GetTexturePool() { return this->m_texturePool; };
        HandlePool<D3D12Buffer, Buffer>& GetBufferPool() { return this->m_bufferPool; };
        HandlePool<D3D12RenderPass, RenderPass>& GetRenderPassPool() { return this->m_renderPassPool; };
        HandlePool<D3D12GraphicsPipeline, GfxPipeline>& GetGraphicsPipelinePool() { return this->m_gfxPipelinePool; }
        HandlePool<D3D12ComputePipeline, ComputePipeline>& GetComputePipelinePool() { return this->m_computePipelinePool; }
        HandlePool<D3D12MeshPipeline, MeshPipeline>& GetMeshPipelinePool() { return this->m_meshPipelinePool; }
        HandlePool<D3D12RTAccelerationStructure, RTAccelerationStructure>& GetRTAccelerationStructurePool() { return this->m_rtAccelerationStructurePool; }
        HandlePool<D3D12CommandSignature, CommandSignature>& GetCommandSignaturePool() { return this->m_commandSignaturePool; }
        HandlePool<D3D12TimerQuery, TimerQuery>& GetTimerQueryPool() { return this->m_timerQueryPool; }

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
        rhi::DeviceCapability m_capabilities;

        D3D12SwapChain m_swapChain;

        // -- Resouce Pool ---
        HandlePool<D3D12Texture, Texture> m_texturePool;
        HandlePool<D3D12CommandSignature, CommandSignature> m_commandSignaturePool;
        HandlePool<D3D12Shader, rhi::Shader> m_shaderPool;
        HandlePool<D3D12InputLayout, InputLayout> m_inputLayoutPool;
        HandlePool<D3D12Buffer, Buffer> m_bufferPool;
        HandlePool<D3D12RenderPass, RenderPass> m_renderPassPool;
        HandlePool<D3D12RTAccelerationStructure, RTAccelerationStructure> m_rtAccelerationStructurePool;
        HandlePool<D3D12GraphicsPipeline, GfxPipeline> m_gfxPipelinePool;
        HandlePool<D3D12ComputePipeline, ComputePipeline> m_computePipelinePool;
        HandlePool<D3D12MeshPipeline, MeshPipeline> m_meshPipelinePool;
        HandlePool<D3D12TimerQuery, TimerQuery> m_timerQueryPool;

        // -- Command Queues ---
        std::array<D3D12CommandQueue, (int)CommandQueueType::Count> m_commandQueues;

        // -- Command lists ---
        uint32_t m_activeCmdCount;
        std::array<D3D12CommandList, 32> m_commandListPool;

        // -- Descriptor Heaps ---
        std::array<CpuDescriptorHeap, (int)DescriptorHeapTypes::Count> m_cpuDescriptorHeaps;
        std::array<GpuDescriptorHeap, 2> m_gpuDescriptorHeaps;

        // -- Query Heaps ---
        Microsoft::WRL::ComPtr<ID3D12QueryHeap> m_gpuTimestampQueryHeap;
        BufferHandle m_timestampQueryBuffer;
        BitSetAllocator m_timerQueryIndexPool;

        D3D12BindlessDescriptorTable m_bindlessResourceDescriptorTable;

        // -- Frame Frences --
        std::vector<phx::core::EnumArray<CommandQueueType, Microsoft::WRL::ComPtr<ID3D12Fence>>> m_frameFences;
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
#endif
}

