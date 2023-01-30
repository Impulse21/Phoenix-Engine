
#include <memory>
#include <array>
#include <deque>
#include <tuple>
#include <unordered_map>

#include "CommandQueue.h"
#include "DescriptorHeap.h"
#include "D3D12Common.h"
#include "PhxEngine/Core/BitSetAllocator.h"
#include <PhxEngine/Core/Pool.h>
#include "CommandList.h"


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
    struct TrackedResources;

    class BindlessDescriptorTable;

    class TimerQuery : public ITimerQuery
    {
    public:
        explicit TimerQuery(Core::BitSetAllocator& timerQueryIndexPoolRef)
            : m_timerQueryIndexPoolRef(timerQueryIndexPoolRef)
        {}

        ~TimerQuery() override
        {
            // This will cause problems if we ever remove the device
            this->m_timerQueryIndexPoolRef.Release(static_cast<int>(this->BeginQueryIndex) / 2);
        }

        size_t BeginQueryIndex = 0;
        size_t EndQueryIndex = 0;

        // Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
        CommandQueue* CommandQueue = nullptr;
        uint64_t FenceCount = 0;

        bool Started = false;
        bool Resolved = false;
        Core::TimeStep Time;


    private:
        // This is for freeing the query index
        Core::BitSetAllocator& m_timerQueryIndexPoolRef;
    };

    struct D3D12Viewport
    {
        ViewportDesc Desc;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> NativeSwapchain;
        Microsoft::WRL::ComPtr<IDXGISwapChain4> NativeSwapchain4;

        std::vector<TextureHandle> BackBuffers;
        RenderPassHandle RenderPass;
    };

    struct D3D12Shader
    {
        std::vector<uint8_t> ByteCode;
        const ShaderDesc Desc;
        Microsoft::WRL::ComPtr<ID3D12RootSignatureDeserializer> RootSignatureDeserializer;
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
        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;
        GraphicsPipelineDesc Desc;

        D3D12GraphicsPipeline() = default;
    };

    struct D3D12ComputePipeline
    {
        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;
        ComputePipelineDesc Desc;

        D3D12ComputePipeline() = default;
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

    struct D3D12Texture final
    {
        TextureDesc Desc = {};
        Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;

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
        DescriptorView Srv;

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

    enum class DescriptorHeapTypes : uint8_t
    {
        CBV_SRV_UAV,
        Sampler,
        RTV,
        DSV,
        Count,
    };

    struct D3D12DeviceBasicInfo
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

	class D3D12GraphicsDevice final : public IGraphicsDevice
	{
    private:
        inline static D3D12GraphicsDevice* sSingleton = nullptr;

	public:
		D3D12GraphicsDevice(D3D12Adapter const& adapter, Microsoft::WRL::ComPtr<IDXGIFactory6> factory);
		~D3D12GraphicsDevice();

        // -- Interface Functions ---
    public:
        void Initialize() override;
        void Finalize() override;

        void WaitForIdle() override;
        void QueueWaitForCommandList(CommandQueueType waitQueue, ExecutionReceipt waitOnRecipt) override;

        // TODO: Return a handle to viewport so we can have more then one. Need to sort out how delete queue would work.
        void CreateViewport(ViewportDesc const& desc) override;
        TextureHandle GetBackBuffer() override { return this->m_activeViewport->BackBuffers[this->GetCurrentBackBufferIndex()]; }
        virtual void BeginFrame() override;
        virtual void EndFrame() override;

        // TODO: remove
        CommandListHandle CreateCommandList(CommandListDesc const& desc = {}) override;
        ICommandList* BeginCommandRecording(CommandQueueType QueueType = CommandQueueType::Graphics) override;

        ShaderHandle CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode) override;
        InputLayoutHandle CreateInputLayout(VertexAttributeDesc* desc, uint32_t attributeCount) override;
        GraphicsPipelineHandle CreateGraphicsPipeline(GraphicsPipelineDesc const& desc) override;
        ComputePipelineHandle CreateComputePipeline(ComputePipelineDesc const& desc) override;

        RenderPassHandle CreateRenderPass(RenderPassDesc const& desc) override;
        void DeleteRenderPass(RenderPassHandle handle) override;

        TextureHandle CreateTexture(TextureDesc const& desc) override;
        const TextureDesc& GetTextureDesc(TextureHandle handle) override;
        DescriptorIndex GetDescriptorIndex(TextureHandle handle, SubresouceType type, int subResource = -1) override;
        void DeleteTexture(TextureHandle) override;

        BufferHandle CreateIndexBuffer(BufferDesc const& desc) override;
        BufferHandle CreateVertexBuffer(BufferDesc const& desc) override;
        BufferHandle CreateBuffer(BufferDesc const& desc) override;
        const BufferDesc& GetBufferDesc(BufferHandle handle) override;
        DescriptorIndex GetDescriptorIndex(BufferHandle handle, SubresouceType type, int subResource = -1) override;
        void* GetBufferMappedData(BufferHandle handle) override;
        uint32_t GetBufferMappedDataSizeInBytes(BufferHandle handle) override;
        void DeleteBuffer(BufferHandle handle) override;

        RTAccelerationStructureHandle CreateRTAccelerationStructure(RTAccelerationStructureDesc const& desc) override;
        const RTAccelerationStructureDesc& GetRTAccelerationStructureDesc(RTAccelerationStructureHandle handle) override;
        void WriteRTTopLevelAccelerationStructureInstance(RTAccelerationStructureDesc::TopLevelDesc::Instance const& instance, void* dest) override;
        size_t GetRTTopLevelAccelerationStructureInstanceSize() override;
        void DeleteRtAccelerationStructure(RTAccelerationStructureHandle handle) override;
        DescriptorIndex GetDescriptorIndex(RTAccelerationStructureHandle handle) override;

        int CreateSubresource(TextureHandle texture, SubresouceType subresourceType, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip = 0, uint32_t mipCount = ~0u) override;
        int CreateSubresource(BufferHandle buffer, SubresouceType subresourceType, size_t offset, size_t size = ~0u) override;

        // -- Query Stuff ---
        TimerQueryHandle CreateTimerQuery() override;
        bool PollTimerQuery(TimerQueryHandle query) override;
        Core::TimeStep GetTimerQueryTime(TimerQueryHandle query) override;
        void ResetTimerQuery(TimerQueryHandle query) override;

        ExecutionReceipt ExecuteCommandLists(
            Core::Span<ICommandList*> commandLists,
            CommandQueueType executionQueue = CommandQueueType::Graphics) override
        {
            return this->ExecuteCommandLists(
                commandLists,
                false,
                executionQueue);
        }

        ExecutionReceipt ExecuteCommandLists(
            Core::Span<ICommandList*> commandLists,
            bool waitForCompletion,
            CommandQueueType executionQueue = CommandQueueType::Graphics) override;

        size_t GetNumBindlessDescriptors() const override { return NUM_BINDLESS_RESOURCES; }

        ShaderModel GetMinShaderModel() const override { return this->m_minShaderModel;  };
        ShaderType GetShaderType() const override { return ShaderType::HLSL6; };
        GraphicsAPI GetApi() const override { return GraphicsAPI::DX12; }

        void BeginCapture(std::wstring const& filename) override;
        void EndCapture(bool discard = false) override;

        size_t GetFrameIndex() override { return this->GetCurrentBackBufferIndex(); };
        size_t GetMaxInflightFrames() override { return this->m_activeViewport->Desc.BufferCount; }

        bool CheckCapability(DeviceCapability deviceCapability);
        bool IsDevicedRemoved() override;

        // -- Dx12 Specific functions ---
    public:
        D3D12Viewport* GetActiveViewport() const { return this->m_activeViewport.get(); }
        TextureHandle CreateRenderTarget(TextureDesc const& desc, Microsoft::WRL::ComPtr<ID3D12Resource> d3d12TextureResource);
        TextureHandle CreateTexture(TextureDesc const& desc, Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource);

        int CreateShaderResourceView(BufferHandle buffer, size_t offset, size_t size);
        int CreateUnorderedAccessView(BufferHandle buffer, size_t offset, size_t size);

        int CreateShaderResourceView(TextureHandle texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);
        int CreateRenderTargetView(TextureHandle texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);
        int CreateDepthStencilView(TextureHandle texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);
        int CreateUnorderedAccessView(TextureHandle texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);

    public:
        // TODO: Remove
        void RunGarbageCollection();

        // -- Getters ---
    public:
        Microsoft::WRL::ComPtr<ID3D12Device> GetD3D12Device() { return this->m_rootDevice; }
        Microsoft::WRL::ComPtr<ID3D12Device2> GetD3D12Device2() { return this->m_rootDevice2; }
        Microsoft::WRL::ComPtr<ID3D12Device5> GetD3D12Device5() { return this->m_rootDevice5; }

        Microsoft::WRL::ComPtr<IDXGIFactory6> GetDxgiFactory() { return this->m_factory; }
        Microsoft::WRL::ComPtr<IDXGIAdapter> GetDxgiAdapter() { return this->m_gpuAdapter.NativeAdapter; }

    public:
        CommandQueue* GetGfxQueue() { return this->GetQueue(CommandQueueType::Graphics); }
        CommandQueue* GetComputeQueue() { return this->GetQueue(CommandQueueType::Compute); }
        CommandQueue* GetCopyQueue() { return this->GetQueue(CommandQueueType::Copy); }

        CommandQueue* GetQueue(CommandQueueType type) { return this->m_commandQueues[(int)type].get(); }

        CpuDescriptorHeap* GetResourceCpuHeap() { return this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV].get(); }
        CpuDescriptorHeap* GetSamplerCpuHeap() { return this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler].get(); }
        CpuDescriptorHeap* GetRtvCpuHeap() { return this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::RTV].get(); }
        CpuDescriptorHeap* GetDsvCpuHeap() { return this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::DSV].get(); }

        GpuDescriptorHeap* GetResourceGpuHeap() { return this->m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV].get(); }
        GpuDescriptorHeap* GetSamplerGpuHeap() { return this->m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler].get(); }

        ID3D12QueryHeap* GetQueryHeap() { return this->m_gpuTimestampQueryHeap.Get(); }
        BufferHandle GetTimestampQueryBuffer() { return this->m_timestampQueryBuffer; }
        const BindlessDescriptorTable* GetBindlessTable() const { return this->m_bindlessResourceDescriptorTable.get(); }

        // Maybe better encapulate this.
        Core::Pool<D3D12Texture, Texture>& GetTexturePool() { return this->m_texturePool; };
        Core::Pool<D3D12Buffer, Buffer>& GetBufferPool() { return this->m_bufferPool; };
        Core::Pool<D3D12RenderPass, RenderPass>& GetRenderPassPool() { return this->m_renderPassPool; };
        Core::Pool<D3D12GraphicsPipeline, GraphicsPipeline>& GetGraphicsPipelinePool() { return this->m_graphicsPipelinePool; }
        Core::Pool<D3D12ComputePipeline, ComputePipeline>& GetComputePipelinePool() { return this->m_computePipelinePool; }
        Core::Pool<D3D12RTAccelerationStructure, RTAccelerationStructure>& GetRTAccelerationStructurePool() { return this->m_rtAccelerationStructurePool; }

    private:
        size_t GetCurrentBackBufferIndex() const;
        size_t GetBackBufferIndex() const { return this->m_frameCount % this->m_activeViewport->Desc.BufferCount; }
        bool IsHdrSwapchainSupported();

    private:
        void CreateBufferInternal(BufferDesc const& desc, D3D12Buffer& outBuffer);

        void CreateGpuTimestampQueryHeap(uint32_t queryCount);

        // -- Dx12 API creation ---
    private:
        void InitializeD3D12Device(IDXGIAdapter* gpuAdapter);

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

		// std::shared_ptr<IDxcUtils> dxcUtils;
		D3D12Adapter m_gpuAdapter;

		D3D12_FEATURE_DATA_ROOT_SIGNATURE FeatureDataRootSignature = {};
		D3D12_FEATURE_DATA_SHADER_MODEL   FeatureDataShaderModel = {};
        ShaderModel m_minShaderModel = ShaderModel::SM_6_0;

		bool IsUnderGraphicsDebugger = false;
        RHI::DeviceCapability m_capabilities;

        std::unique_ptr<D3D12Viewport> m_activeViewport;

        // -- Resouce Pool ---
        Core::Pool<D3D12Texture, Texture> m_texturePool;
        Core::Pool<D3D12Shader, RHIShader> m_shaderPool;
        Core::Pool<D3D12InputLayout, InputLayout> m_inputLayoutPool;
        Core::Pool<D3D12Buffer, Buffer> m_bufferPool;
        Core::Pool<D3D12RenderPass, RenderPass> m_renderPassPool;
        Core::Pool<D3D12RTAccelerationStructure, RTAccelerationStructure> m_rtAccelerationStructurePool;
        Core::Pool<D3D12GraphicsPipeline, GraphicsPipeline> m_graphicsPipelinePool;
        Core::Pool<D3D12ComputePipeline, ComputePipeline> m_computePipelinePool;

        // -- Command Queues ---
		std::array<std::unique_ptr<CommandQueue>, (int)CommandQueueType::Count> m_commandQueues;

        // -- Command lists ---
        // 
        // -- Descriptor Heaps ---
		std::array<std::unique_ptr<CpuDescriptorHeap>, (int)DescriptorHeapTypes::Count> m_cpuDescriptorHeaps;
        std::array<std::unique_ptr<GpuDescriptorHeap>, 2> m_gpuDescriptorHeaps;

        // -- Query Heaps ---
        Microsoft::WRL::ComPtr<ID3D12QueryHeap> m_gpuTimestampQueryHeap;
        BufferHandle m_timestampQueryBuffer;
        Core::BitSetAllocator m_timerQueryIndexPool;

        std::unique_ptr<BindlessDescriptorTable> m_bindlessResourceDescriptorTable;

        // -- Frame Frences --
        Microsoft::WRL::ComPtr<ID3D12Fence> m_frameFence;
        uint64_t m_frameCount = 1;

        struct InflightDataEntry
        {
            uint64_t LastSubmittedFenceValue = 0;
            std::shared_ptr<TrackedResources> TrackedResources;
        };

        std::array<std::deque<InflightDataEntry>, (int)CommandQueueType::Count> m_inflightData;

        struct DeleteItem
        {
            uint32_t Frame;
            std::function<void()> DeleteFn;
        };
        std::deque<DeleteItem> m_deleteQueue;

#ifdef ENABLE_PIX_CAPUTRE
        HMODULE m_pixCaptureModule;
#endif
	};
}