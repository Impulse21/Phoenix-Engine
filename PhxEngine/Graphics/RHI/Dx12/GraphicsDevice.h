
#include <memory>
#include <array>
#include <deque>
#include <tuple>
#include <unordered_map>

#include "CommandQueue.h"
#include "DescriptorHeap.h"
#include "Common.h"

// Teir 1 limit is 1,000,000
// https://docs.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
#define TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE 1000000

#define NUM_BINDLESS_RESOURCES TIER_ONE_GPU_DESCRIPTOR_HEAP_SIZE / 2

namespace PhxEngine::RHI::Dx12
{
    struct TrackedResources;

    class BindlessDescriptorTable;

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

    struct Texture final : public ITexture
    {
        TextureDesc Desc = {};
        Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource;
        DescriptorHeapAllocation RtvAllocation;
        DescriptorHeapAllocation DsvAllocation;
        DescriptorHeapAllocation SrvAllocation;

        // TODO: Free Index
        DescriptorIndex BindlessResourceIndex = cInvalidDescriptorIndex;

        const TextureDesc& GetDesc() const { return this->Desc; }
        virtual const DescriptorIndex GetDescriptorIndex() const { return this->BindlessResourceIndex; }
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

	class GraphicsDevice final : public IGraphicsDevice
	{
	public:
		GraphicsDevice();
		~GraphicsDevice();

        // -- Interface Functions ---
    public:
        void WaitForIdle() override;

		void CreateSwapChain(SwapChainDesc const& swapChainDesc) override;

        CommandListHandle CreateCommandList(CommandListDesc const& desc = {}) override;

        ShaderHandle CreateShader(ShaderDesc const& desc, const void* binary, size_t binarySize) override;
        InputLayoutHandle CreateInputLayout(VertexAttributeDesc* desc, uint32_t attributeCount) override;
        GraphicsPSOHandle CreateGraphicsPSOHandle(GraphicsPSODesc const& desc) override;

        TextureHandle CreateDepthStencil(TextureDesc const& desc) override;
        TextureHandle CreateTexture(TextureDesc const& desc) override;

        BufferHandle CreateIndexBuffer(BufferDesc const& desc) override;
        BufferHandle CreateVertexBuffer(BufferDesc const& desc) override;
        BufferHandle CreateBuffer(BufferDesc const& desc) override;

        TextureHandle GetBackBuffer() override { return this->m_swapChain.BackBuffers[this->GetCurrentBackBufferIndex()]; }

        void Present() override;

        uint64_t ExecuteCommandLists(
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

        uint64_t ExecuteCommandLists(
            ICommandList* const* pCommandLists,
            size_t numCommandLists,
            bool waitForCompletion,
            CommandQueueType executionQueue = CommandQueueType::Graphics) override;

        size_t GetNumBindlessDescriptors() const override { return NUM_BINDLESS_RESOURCES; }

        virtual ShaderModel GetMinShaderModel() const { return this->m_minShaderModel;  };
        virtual ShaderType GetShaderType() const { return ShaderType::HLSL6; };

        // -- Dx12 Specific functions ---
    public:
        TextureHandle CreateRenderTarget(TextureDesc const& desc, Microsoft::WRL::ComPtr<ID3D12Resource> d3d12TextureResource);

    public:
        void RunGarbageCollection();

        // -- Getters ---
    public:
        Microsoft::WRL::ComPtr<ID3D12Device> GetD3D12Device() { return this->m_device; }
        Microsoft::WRL::ComPtr<ID3D12Device2> GetD3D12Device2() { return this->m_device2; }
        Microsoft::WRL::ComPtr<ID3D12Device5> GetD3D12Device5() { return this->m_device5; }

        Microsoft::WRL::ComPtr<IDXGIFactory6> GetDxgiFactory() { return this->m_factory; }
        Microsoft::WRL::ComPtr<IDXGIAdapter> GetDxgiAdapter() { return this->m_gpuAdapter; }

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

        const BindlessDescriptorTable* GetBindlessTable() const { return this->m_bindlessResourceDescriptorTable.get(); }

    private:
        size_t GetCurrentBackBufferIndex() const;

    private:
        std::unique_ptr<GpuBuffer> CreateBufferInternal(BufferDesc const& desc);
        void CreateSRVViews(GpuBuffer* gpuBuffer);

        // -- Dx12 API creation ---
    private:
        Microsoft::WRL::ComPtr<IDXGIFactory6> CreateFactory() const;
        void CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter> gpuAdapter);

        Microsoft::WRL::ComPtr<IDXGIAdapter1> SelectOptimalGpu();
        std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>> EnumerateAdapters(Microsoft::WRL::ComPtr<IDXGIFactory6> factory, bool includeSoftwareAdapter = false);

         // -- Pipeline state conversion --- 
    private:
        void TranslateBlendState(BlendRenderState const& inState, D3D12_BLEND_DESC& outState);
        void TranslateDepthStencilState(DepthStencilRenderState const& inState, D3D12_DEPTH_STENCIL_DESC& outState);
        void TranslateRasterState(RasterRenderState const& inState, D3D12_RASTERIZER_DESC& outState);

	private:
		Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
		Microsoft::WRL::ComPtr<ID3D12Device> m_device;
		Microsoft::WRL::ComPtr<ID3D12Device2> m_device2;
		Microsoft::WRL::ComPtr<ID3D12Device5> m_device5;
		// std::shared_ptr<IDxcUtils> dxcUtils;
		Microsoft::WRL::ComPtr<IDXGIAdapter> m_gpuAdapter;

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

        // -- SwapChain ---
		SwapChain m_swapChain;

        // -- Command Queues ---
		std::array<std::unique_ptr<CommandQueue>, (int)CommandQueueType::Count> m_commandQueues;

        // -- Descriptor Heaps ---
		std::array<std::unique_ptr<CpuDescriptorHeap>, (int)DescriptorHeapTypes::Count> m_cpuDescriptorHeaps;
        std::array<std::unique_ptr<GpuDescriptorHeap>, 2> m_gpuDescriptorHeaps;

        std::unique_ptr<BindlessDescriptorTable> m_bindlessResourceDescriptorTable;

        // -- Frame Frences ---
        std::vector<uint64_t> m_frameFence;
        uint64_t m_frameCount;

        struct InflightDataEntry
        {
            uint64_t LastSubmittedFenceValue = 0;
            std::shared_ptr<TrackedResources> TrackedResources;
        };

        std::array<std::deque<InflightDataEntry>, (int)CommandQueueType::Count> m_inflightData;
	};
}