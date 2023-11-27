#pragma once

#define NOMINMAX

#include <memory>
#include <array>
#include <deque>
#include <tuple>
#include <unordered_map>
#include <functional>

#include <PhxEngine/RHI/PhxRHIResources.h>
#include <PhxEngine/RHI/PhxDynamicRHI.h>
#include <PhxEngine/Core/BitSetAllocator.h>
#include <PhxEngine/Core/Math.h>

#include "D3D12Common.h"
#include "D3D12CommandQueue.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12BiindlessDescriptorTable.h"
#include "D3D12UploadBuffer.h"
#include "D3D12Resources.h"
#include <functional>

#include <dstorage.h>

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

    class DirectStorage final : public Core::RefCounter<IDirectStorage>
    {
    public:
        DirectStorage(ID3D12Device* device);

        void EnqueueRequest(StorageRequest const& request) override;
        SubmitReceipt Submit(bool waitForComplete = false) override;

    private:
        Core::RefCountPtr<IDStorageFactory> m_factory;
        Core::RefCountPtr<IDStorageQueue> m_queue[2];
        Core::RefCountPtr<ID3D12Fence> m_fence;
        uint32_t m_fenceValue = 0;
    };

	class D3D12DynamicRHI final : public RHI::DynamicRHI
	{
        inline static D3D12DynamicRHI* SingleD3D12RHI;

	public:
        D3D12DynamicRHI();
		~D3D12DynamicRHI();

        static D3D12DynamicRHI* GetD3D12RHI() { return SingleD3D12RHI;  }
        template<typename TRHIType, typename TReturnType = typename TD3D12ResourceTraits<TRHIType>::TConcreteType>
        static FORCEINLINE TReturnType* ResourceCast(TRHIType* Resource)
        {
            return static_cast<TReturnType*>(Resource);
        }

    public:
        void Initialize() override;
        void Finalize() override;

        // -- Submits Command lists and presents ---
        void Present(ISwapChain* swapChain) override;
        void WaitForIdle() override;

        bool IsDevicedRemoved() override;
        bool CheckCapability(DeviceCapability deviceCapability) override;

        IDirectStorage* GetDirectStorage() override { return this->m_directStorage.Get(); }
        void Wait(SubmitReceipt const& reciet) override;

        // -- Resouce Functions ---
    public:
        SwapChainRef CreateSwapChain(SwapChainDesc const& desc, void* windowsHandle) override;
        ShaderRef CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode) override;
        InputLayoutRef CreateInputLayout(VertexAttributeDesc* desc, uint32_t attributeCount) override;
        GfxPipelineRef CreateGfxPipeline(GfxPipelineDesc const& desc) override;
        ComputePipelineRef CreateComputePipeline(ComputePipelineDesc const& desc) override;
        MeshPipelineRef CreateMeshPipeline(MeshPipelineDesc const& desc) override;
        TextureRef CreateTexture(TextureDesc const& desc) override;
        BufferRef CreateBuffer(BufferDesc const& desc) override;
        CommandListRef CreateCommandList(RHI::CommandQueueType type = CommandQueueType::Graphics) override;

    public:
        void ResizeSwapChain(ISwapChain* swapChain, SwapChainDesc const& desc) override;

        // -- Dx12 Specific functions ---
    public:
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
        // BufferHandle GetTimestampQueryBuffer() { return this->m_timestampQueryBuffer; }
        const D3D12BindlessDescriptorTable& GetBindlessTable() const { return this->m_bindlessResourceDescriptorTable; }

        inline void EnqueueDelete(std::function<void()>&& function)
        {
            this->m_deleteQueue.push_back({
                    .Frame = this->m_frameCount,
                    .DeleteFn = std::move(function)
                });
        }

    private:
        void CreateGpuTimestampQueryHeap(uint32_t queryCount);
        void CreateSwapChainResources(D3D12SwapChain* swapChain);
        bool IsHdrSwapchainSupported(D3D12SwapChain* swapChain);
        int CreateSubresource(D3D12Texture* texture, SubresouceType subresourceType, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip = 0, uint32_t mpCount = ~0);
        int CreateSubresource(D3D12Buffer* buffer, SubresouceType subresourceType, size_t offset, size_t size = ~0u);

        int CreateShaderResourceView(D3D12Buffer* buffer, size_t offset, size_t size);
        int CreateUnorderedAccessView(D3D12Buffer* buffer, size_t offset, size_t size);

        int CreateShaderResourceView(D3D12Texture* texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);
        int CreateRenderTargetView(D3D12Texture* texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);
        int CreateDepthStencilView(D3D12Texture* texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);
        int CreateUnorderedAccessView(D3D12Texture* texture, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount);

        // -- Dx12 API creation ---
    private:
        void InitializeD3D12NativeResources(IDXGIAdapter* gpuAdapter);
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

        // -- Command Queues ---
		std::array<D3D12CommandQueue, (int)CommandQueueType::Count> m_commandQueues;

        // -- Descriptor Heaps ---
		std::array<CpuDescriptorHeap, (int)DescriptorHeapTypes::Count> m_cpuDescriptorHeaps;
        std::array<GpuDescriptorHeap, 2> m_gpuDescriptorHeaps;

        // -- Query Heaps ---
        Microsoft::WRL::ComPtr<ID3D12QueryHeap> m_gpuTimestampQueryHeap;
        
        Core::BitSetAllocator m_timerQueryIndexPool;

        D3D12BindlessDescriptorTable m_bindlessResourceDescriptorTable;

        // -- Frame Frences --
        std::array<Microsoft::WRL::ComPtr<ID3D12Fence>, (int)CommandQueueType::Count> m_frameFences;
        uint64_t m_frameCount = 1;

        Core::RefCountPtr<DirectStorage> m_directStorage;

        struct DeleteItem
        {
            uint64_t Frame;
            std::function<void()> DeleteFn;
        };
        std::deque<DeleteItem> m_deleteQueue;

#ifdef ENABLE_PIX_CAPUTRE
        HMODULE m_pixCaptureModule;
#endif

        DWORD m_CallbackCookie;
	};

}