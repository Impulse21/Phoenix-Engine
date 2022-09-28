
#include <memory>
#include <array>
#include <deque>
#include <tuple>
#include <unordered_map>

#include "CommandQueue.h"
#include "DescriptorHeap.h"
#include "Common.h"
#include "PhxEngine/Core/BitSetAllocator.h"
#include "PhxEngine/Core/Pool.h"

// Teir 1 limit is 1,000,000
// https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
#define TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE 1000000

#define NUM_BINDLESS_RESOURCES TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE / 2

#define ENABLE_PIX_CAPUTRE 1

namespace PhxEngine::RHI::Dx12
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

    class Shader : public IShader
    {
    public:
        Shader(ShaderDesc const& desc, const void* binary, size_t binarySize)
            : m_desc(desc)
        {
            this->m_byteCode.resize(binarySize);
            std::memcpy(this->m_byteCode.data(), binary, binarySize);
        }

        const ShaderDesc& GetDesc() const { return this->m_desc; }
        const std::vector<uint8_t>& GetByteCode() const { return this->m_byteCode; }
        Microsoft::WRL::ComPtr<ID3D12RootSignatureDeserializer> GetRootSigDeserializer() { return this->m_rootSignatureDeserializer; }

        void SetRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig) { this->m_rootSignature = rootSig; };
        Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature() const { return this->m_rootSignature; };

    private:
        std::vector<uint8_t> m_byteCode;
        const ShaderDesc m_desc;
        Microsoft::WRL::ComPtr<ID3D12RootSignatureDeserializer> m_rootSignatureDeserializer;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    };

    struct InputLayout : public IInputLayout
    {
        std::vector<VertexAttributeDesc> Attributes;
        std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements;

        // maps a binding slot to an element stride
        std::unordered_map<uint32_t, uint32_t> ElementStrides;

        uint32_t GetNumAttributes() const override { return this->Attributes.size(); }
        const VertexAttributeDesc* GetAttributeDesc(uint32_t index) const override
        {
            assert(index < this->GetNumAttributes());
            return &this->Attributes[index];
        }
    };

    struct GraphicsPSO : public IGraphicsPSO
    {
        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;
        GraphicsPSODesc Desc;


        const GraphicsPSODesc& GetDesc() const override { return this->Desc; }
    };

    struct ComputePSO : public IComputePSO
    {
        Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState;
        ComputePSODesc Desc;


        const ComputePSODesc& GetDesc() const override { return this->Desc; }
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

    struct Dx12Texture final
    {
        TextureDesc Desc = {};
        Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;

        // -- The views ---
        DescriptorView RtvAllocation;
        std::vector<DescriptorView> RtvSubresourcesAlloc = {};

        DescriptorView DsvAllocation;
        std::vector<DescriptorView> DsvSubresourcesAlloc = {};

        DescriptorView SrvAllocation;
        std::vector<DescriptorView> SrvSubresourcesAlloc = {};

        DescriptorView UavAllocation;
        std::vector<DescriptorView> UavSubresourcesAlloc = {};

        Dx12Texture() = default;

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

            SrvAllocation.Allocation.Free();
            for (auto& view : this->SrvSubresourcesAlloc)
            {
                view.Allocation.Free();
            }
            SrvSubresourcesAlloc.clear();
            SrvAllocation = {};

            UavAllocation.Allocation.Free();
            for (auto& view : this->UavSubresourcesAlloc)
            {
                view.Allocation.Free();
            }
            UavSubresourcesAlloc.clear();
            UavAllocation = {};
        }
    };

    struct Dx12Buffer final
    {
        BufferDesc Desc = {};
        Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;

        void* MappedData = nullptr;
        uint32_t MappedSizeInBytes = 0;

        // -- Views ---
        DescriptorView SrvAllocation;
        std::vector<DescriptorView> SrvSubresourcesAlloc = {};

        DescriptorView UavAllocation;
        std::vector<DescriptorView> UavSubresourcesAlloc = {};


        D3D12_VERTEX_BUFFER_VIEW VertexView = {};
        D3D12_INDEX_BUFFER_VIEW IndexView = {};

        const BufferDesc& GetDesc() const { return this->Desc; }

        void DisposeViews()
        {
            SrvAllocation.Allocation.Free();
            for (auto& view : this->SrvSubresourcesAlloc)
            {
                view.Allocation.Free();
            }
            SrvSubresourcesAlloc.clear();
            SrvAllocation = {};

            UavAllocation.Allocation.Free();
            for (auto& view : this->UavSubresourcesAlloc)
            {
                view.Allocation.Free();
            }
            UavSubresourcesAlloc.clear();
            UavAllocation = {};
        }
    };

    struct Dx12RenderPass final
    {
        RenderPassDesc Desc = {};

        D3D12_RENDER_PASS_FLAGS D12RenderFlags = D3D12_RENDER_PASS_FLAG_NONE;

        size_t NumRenderTargets = 0;
        std::array<D3D12_RENDER_PASS_RENDER_TARGET_DESC, kNumConcurrentRenderTargets> RTVs = {};
        D3D12_RENDER_PASS_DEPTH_STENCIL_DESC DSV = {};

        std::vector<D3D12_RESOURCE_BARRIER> BarrierDescBegin;
        std::vector<D3D12_RESOURCE_BARRIER> BarrierDescEnd;
    };

    struct SwapChain
    {
        Microsoft::WRL::ComPtr<IDXGISwapChain4> DxgiSwapchain;
        SwapChainDesc Desc;
        std::vector<TextureHandle> BackBuffers;
        RenderPassHandle RenderPass = {};
        RHI::ColourSpace ColourSpace = RHI::ColourSpace::SRGB;

        uint32_t GetNumBackBuffers() const { return this->Desc.BufferCount; }
    };

    enum class DescriptorHeapTypes : uint8_t
    {
        CBV_SRV_UAV,
        Sampler,
        RTV,
        DSV,
        Count,
    };

    class DXGIGpuAdapter final : public IGpuAdapter
    {
    public:
        std::string Name;
        size_t DedicatedSystemMemory = 0;
        size_t DedicatedVideoMemory = 0;
        size_t SharedSystemMemory = 0;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> DxgiAdapter;

        const char* GetName() const override { return this->Name.c_str(); };
        virtual size_t GetDedicatedSystemMemory() const override { return this->DedicatedSystemMemory; };
        virtual size_t GetDedicatedVideoMemory() const override { return this->DedicatedVideoMemory; };
        virtual size_t GetSharedSystemMemory() const override { return this->SharedSystemMemory; };
    };

	class GraphicsDevice final : public IGraphicsDevice
	{
	public:
		GraphicsDevice();
		~GraphicsDevice();

        // -- Interface Functions ---
    public:
        void WaitForIdle() override;
        void QueueWaitForCommandList(CommandQueueType waitQueue, ExecutionReceipt waitOnRecipt) override;

		void CreateSwapChain(SwapChainDesc const& swapChainDesc) override;

        CommandListHandle CreateCommandList(CommandListDesc const& desc = {}) override;
        // CommandListHandle BeginGfxCommandList() override;
        // CommandListHandle BeginComputeCommandList() override;

        ShaderHandle CreateShader(ShaderDesc const& desc, const void* binary, size_t binarySize) override;
        InputLayoutHandle CreateInputLayout(VertexAttributeDesc* desc, uint32_t attributeCount) override;
        GraphicsPSOHandle CreateGraphicsPSO(GraphicsPSODesc const& desc) override;
        ComputePSOHandle CreateComputePso(ComputePSODesc const& desc) override;

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

        int CreateSubresource(TextureHandle texture, SubresouceType subresourceType, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip = 0, uint32_t mipCount = ~0u) override;
        int CreateSubresource(BufferHandle buffer, SubresouceType subresourceType, size_t offset, size_t size = ~0u) override;

        // -- Query Stuff ---
        TimerQueryHandle CreateTimerQuery() override;
        bool PollTimerQuery(TimerQueryHandle query) override;
        Core::TimeStep GetTimerQueryTime(TimerQueryHandle query) override;
        void ResetTimerQuery(TimerQueryHandle query) override;

        TextureHandle GetBackBuffer() override { return this->m_swapChain.BackBuffers[this->GetCurrentBackBufferIndex()]; }

        void Present() override;

        ExecutionReceipt ExecuteCommandLists(
            ICommandList* const* pCommandLists,
            size_t numCommandLists,
            CommandQueueType executionQueue = CommandQueueType::Graphics) override
        {
            return this->ExecuteCommandLists(
                pCommandLists,
                numCommandLists,
                false,
                executionQueue);
        }

        ExecutionReceipt ExecuteCommandLists(
            ICommandList* const* pCommandLists,
            size_t numCommandLists,
            bool waitForCompletion,
            CommandQueueType executionQueue = CommandQueueType::Graphics) override;

        size_t GetNumBindlessDescriptors() const override { return NUM_BINDLESS_RESOURCES; }

        ShaderModel GetMinShaderModel() const override { return this->m_minShaderModel;  };
        ShaderType GetShaderType() const override { return ShaderType::HLSL6; };
        GraphicsAPI GetApi() const override { return GraphicsAPI::DX12; }

        const IGpuAdapter* GetGpuAdapter() const override { return this->m_gpuAdapter.get(); };

        void BeginCapture(std::wstring const& filename) override;
        void EndCapture() override;

        size_t GetFrameIndex() override { return this->GetCurrentBackBufferIndex(); };
        size_t GetMaxInflightFrames() override { return this->m_swapChain.Desc.BufferCount; }

        bool CheckCapability(DeviceCapability deviceCapability);

        // -- Dx12 Specific functions ---
    public:
        TextureHandle CreateRenderTarget(TextureDesc const& desc, Microsoft::WRL::ComPtr<ID3D12Resource> d3d12TextureResource);

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
        Microsoft::WRL::ComPtr<ID3D12Device> GetD3D12Device() { return this->m_device; }
        Microsoft::WRL::ComPtr<ID3D12Device2> GetD3D12Device2() { return this->m_device2; }
        Microsoft::WRL::ComPtr<ID3D12Device5> GetD3D12Device5() { return this->m_device5; }

        Microsoft::WRL::ComPtr<IDXGIFactory6> GetDxgiFactory() { return this->m_factory; }
        Microsoft::WRL::ComPtr<IDXGIAdapter> GetDxgiAdapter() { return this->m_gpuAdapter->DxgiAdapter; }

        SwapChain& GetSwapchain() { return this->m_swapChain; }

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
        Core::Pool<Dx12Texture, Texture>& GetTexturePool() { return this->m_texturePool; };
        Core::Pool<Dx12Buffer, Buffer>& GetBufferPool() { return this->m_bufferPool; };
        Core::Pool<Dx12RenderPass, RenderPass>& GetRenderPassPool() { return this->m_renderPassPool; };

    private:
        size_t GetCurrentBackBufferIndex() const;
        size_t GetBackBufferIndex() const { return this->m_frameCount % this->m_swapChain.GetNumBackBuffers(); }
        bool IsHdrSwapchainSupported();

    private:
        void CreateBufferInternal(BufferDesc const& desc, Dx12Buffer& outBuffer);

        void CreateGpuTimestampQueryHeap(uint32_t queryCount);

        // -- Dx12 API creation ---
    private:
        Microsoft::WRL::ComPtr<IDXGIFactory6> CreateFactory() const;
        void CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter> gpuAdapter);

        std::unique_ptr<DXGIGpuAdapter> SelectOptimalGpuApdater();
        std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>> EnumerateAdapters(Microsoft::WRL::ComPtr<IDXGIFactory6> factory, bool includeSoftwareAdapter = false);

         // -- Pipeline state conversion --- 
    private:
        void TranslateBlendState(BlendRenderState const& inState, D3D12_BLEND_DESC& outState);
        void TranslateDepthStencilState(DepthStencilRenderState const& inState, D3D12_DEPTH_STENCIL_DESC& outState);
        void TranslateRasterState(RasterRenderState const& inState, D3D12_RASTERIZER_DESC& outState);

	private:
        const uint32_t kTimestampQueryHeapSize = 1024;

		Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
		Microsoft::WRL::ComPtr<ID3D12Device> m_device;
		Microsoft::WRL::ComPtr<ID3D12Device2> m_device2;
		Microsoft::WRL::ComPtr<ID3D12Device5> m_device5;

		// std::shared_ptr<IDxcUtils> dxcUtils;
		std::unique_ptr<DXGIGpuAdapter> m_gpuAdapter;

		D3D12_FEATURE_DATA_ROOT_SIGNATURE FeatureDataRootSignature = {};
		D3D12_FEATURE_DATA_SHADER_MODEL   FeatureDataShaderModel = {};
        ShaderModel m_minShaderModel = ShaderModel::SM_6_0;

		bool IsUnderGraphicsDebugger = false;
        RHI::DeviceCapability m_capabilities;

        // -- Resouce Pool ---
        Core::Pool<Dx12Texture, Texture> m_texturePool;
        Core::Pool<Dx12Buffer, Buffer> m_bufferPool;
        Core::Pool<Dx12RenderPass, RenderPass> m_renderPassPool;

        // -- SwapChain ---
		SwapChain m_swapChain;

        // -- Command Queues ---
		std::array<std::unique_ptr<CommandQueue>, (int)CommandQueueType::Count> m_commandQueues;

        // -- Descriptor Heaps ---
		std::array<std::unique_ptr<CpuDescriptorHeap>, (int)DescriptorHeapTypes::Count> m_cpuDescriptorHeaps;
        std::array<std::unique_ptr<GpuDescriptorHeap>, 2> m_gpuDescriptorHeaps;

        // -- Query Heaps ---
        Microsoft::WRL::ComPtr<ID3D12QueryHeap> m_gpuTimestampQueryHeap;
        BufferHandle m_timestampQueryBuffer;
        Core::BitSetAllocator m_timerQueryIndexPool;

        std::unique_ptr<BindlessDescriptorTable> m_bindlessResourceDescriptorTable;

        // -- Frame Frences ---
        Microsoft::WRL::ComPtr<ID3D12Fence> m_frameFence;
        uint64_t m_frameCount;

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