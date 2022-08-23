
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

namespace PhxEngine::RHI::Dx12
{
    constexpr size_t kNumCommandListPerFrame = 32;
    constexpr size_t kResourcePoolSize = 500000; // 5 KB of handles

    struct TrackedResources;

    class BindlessDescriptorTable;

    struct FrameContext
    {
        uint64_t FenceValue;
        std::vector<Core::Handle<Texture>> PendingDeletionTextures;
    };

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

    struct Dx12Texture final
    {
        TextureDesc Desc = {};
        Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;

        // -- The views ---
        DescriptorHeapAllocation RtvAllocation;
        DescriptorHeapAllocation DsvAllocation;
        DescriptorHeapAllocation SrvAllocation;
        DescriptorHeapAllocation UavAllocation;

        DescriptorIndex BindlessResourceIndex = cInvalidDescriptorIndex;

        Dx12Texture() = default;
        Dx12Texture(Dx12Texture & other)
        {
            this->Desc = other.Desc;
            this->D3D12Resource = std::move(other.D3D12Resource);

            this->RtvAllocation = std::move(other.RtvAllocation);
            this->DsvAllocation = std::move(other.DsvAllocation);
            this->SrvAllocation = std::move(other.SrvAllocation);
            this->UavAllocation = std::move(other.UavAllocation);
            BindlessResourceIndex = other.BindlessResourceIndex;
        }

        Dx12Texture(Dx12Texture const& other)
        {
            this->Desc = other.Desc;
            this->D3D12Resource = other.D3D12Resource;

            this->RtvAllocation = other.RtvAllocation;
            this->DsvAllocation = other.DsvAllocation;
            this->SrvAllocation = other.SrvAllocation;
            this->UavAllocation = other.UavAllocation;
            BindlessResourceIndex = other.BindlessResourceIndex;
        }
    };

    struct GpuBuffer final : public IBuffer
    {
        BufferDesc Desc = {};
        Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;
        DescriptorIndex BindlessResourceIndex = cInvalidDescriptorIndex;

        // -- Views ---
        DescriptorHeapAllocation SrvAllocation;

        D3D12_VERTEX_BUFFER_VIEW VertexView = {};
        D3D12_INDEX_BUFFER_VIEW IndexView = {};

        const BufferDesc& GetDesc() const { return this->Desc; }
        const DescriptorIndex GetDescriptorIndex() const { return this->BindlessResourceIndex; }

        ~GpuBuffer()
        {
            SrvAllocation.Free();
        }
    };

    struct SwapChain
    {
        Microsoft::WRL::ComPtr<IDXGISwapChain4> DxgiSwapchain;
        SwapChainDesc Desc;
        std::vector<TextureHandle> BackBuffers;
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

        TextureHandle CreateDepthStencil(TextureDesc const& desc) override;

        TextureHandle CreateTexture(TextureDesc const& desc) override;
        const TextureDesc& GetTextureDesc(TextureHandle handle) override;
        DescriptorIndex GetDescriptorIndex(TextureHandle handle) override;
        void FreeTexture(TextureHandle) override;

        BufferHandle CreateIndexBuffer(BufferDesc const& desc) override;
        BufferHandle CreateVertexBuffer(BufferDesc const& desc) override;
        BufferHandle CreateBuffer(BufferDesc const& desc) override;

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

        // -- Dx12 Specific functions ---
    public:
        TextureHandle CreateRenderTarget(TextureDesc const& desc, Microsoft::WRL::ComPtr<ID3D12Resource> d3d12TextureResource);


        void CreateShaderResourceView(Dx12Texture& texture);
        void CreateRenderTargetView(Dx12Texture& texture);
        void CreateDepthStencilView(Dx12Texture& texture);
        void CreateUnorderedAccessView(Dx12Texture& texture);

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
        std::shared_ptr<GpuBuffer> GetTimestampQueryBuffer() { return this->m_timestampQueryBuffer; }
        const BindlessDescriptorTable* GetBindlessTable() const { return this->m_bindlessResourceDescriptorTable.get(); }

        // Maybe better encapulate this.
        Core::Pool<Dx12Texture, Texture>& GetTexturePool() { return this->m_texturePool; };

    private:
        size_t GetCurrentBackBufferIndex() const;
        FrameContext& GetCurrentFrameContext() 
        {
            return this->m_frameContext[this->m_frameCount % this->m_frameContext.size()];
        }

    private:
        std::unique_ptr<GpuBuffer> CreateBufferInternal(BufferDesc const& desc);
        void CreateSRVViews(GpuBuffer* gpuBuffer);

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

		bool IsDxrSupported = false;
		bool IsRayQuerySupported = false;
		bool IsRenderPassSupported = false;
		bool IsVariableRateShadingSupported = false;
		bool IsMeshShadingSupported = false;
		bool IsCreateNotZeroedAvailable = false;
		bool IsUnderGraphicsDebugger = false;

        // -- Data Pool ---
        Core::Pool<Dx12Texture, Texture> m_texturePool;

        // -- SwapChain ---
		SwapChain m_swapChain;

        // -- Command Queues ---
		std::array<std::unique_ptr<CommandQueue>, (int)CommandQueueType::Count> m_commandQueues;

        // -- Descriptor Heaps ---
		std::array<std::unique_ptr<CpuDescriptorHeap>, (int)DescriptorHeapTypes::Count> m_cpuDescriptorHeaps;
        std::array<std::unique_ptr<GpuDescriptorHeap>, 2> m_gpuDescriptorHeaps;

        // -- Query Heaps ---
        Microsoft::WRL::ComPtr<ID3D12QueryHeap> m_gpuTimestampQueryHeap;
        std::shared_ptr<GpuBuffer> m_timestampQueryBuffer;
        Core::BitSetAllocator m_timerQueryIndexPool;

        std::unique_ptr<BindlessDescriptorTable> m_bindlessResourceDescriptorTable;

        // -- Frame Frences ---
        std::vector<FrameContext> m_frameContext;
        uint64_t m_frameCount;

        struct InflightDataEntry
        {
            uint64_t LastSubmittedFenceValue = 0;
            std::shared_ptr<TrackedResources> TrackedResources;
        };

        std::array<std::deque<InflightDataEntry>, (int)CommandQueueType::Count> m_inflightData;
	};
}