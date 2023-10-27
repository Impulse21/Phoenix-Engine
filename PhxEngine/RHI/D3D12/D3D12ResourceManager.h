#pragma once

#include <functional>
#include <deque>

#include <Core/Span.h>
#include <Core/Pool.h>
#include <Core/SpinLock.h>
#include <RHI/RHIResources.h>
#include <D3D12Resources.h>
#include <D3D12DescriptorHeap.h>
#include <D3D12BiindlessDescriptorTable.h>

namespace PhxEngine::RHI::D3D12
{
    class D3D12Device;
    class D3D12GpuMemoryAllocator;
    class D3D12ResourceManager final
    {
    public:
        D3D12ResourceManager(std::shared_ptr<D3D12Device> device, std::shared_ptr<D3D12GpuMemoryAllocator> gpuAllocator);
        ~D3D12ResourceManager();
        void RunGrabageCollection(size_t completedFrame = ~0u);

        CommandSignatureHandle CreateCommandSignature(CommandSignatureDesc const& desc, size_t byteStride);
        SwapChainHandle CreateSwapChain(SwapchainDesc const& desc);
        ShaderHandle CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode);
        InputLayoutHandle CreateInputLayout(Core::Span<VertexAttributeDesc> descriptions);
        GfxPipelineHandle CreateGfxPipeline(GfxPipelineDesc const& desc);
        ComputePipelineHandle CreateComputePipeline(ComputePipelineDesc const& desc);
        MeshPipelineHandle CreateMeshPipeline(MeshPipelineDesc const& desc);
        BufferHandle CreateGpuBuffer(BufferDesc const& desc, void* initalData = nullptr);
        TextureHandle CreateTexture(TextureDesc const& desc, void* initalData = nullptr);
        TextureHandle CreateTexture(TextureDesc const& desc, Core::RefCountPtr<ID3D12Resource> resource);
        RTAccelerationStructureHandle CreateRTAccelerationStructure(RTAccelerationStructureDesc const& desc);
        TimerQueryHandle CreateTimerQuery();

        void DeleteCommandSignature(CommandSignatureHandle  handle);
        void DeleteSwapChain(SwapChainHandle handle);
        void DeleteShader(ShaderHandle handle);
        void DeleteInputLayout(InputLayoutHandle handle);
        void DeleteGfxPipeline(GfxPipelineHandle handle);
        void DeleteComputePipeline(ComputePipelineHandle handle);
        void DeleteMeshPipeline(MeshPipelineHandle handle);
        void DeleteGpuBuffer(BufferHandle handle);
        void DeleteTexture(TextureHandle handle);
        void DeleteRTAccelerationStructure(RTAccelerationStructureHandle  handle);
        void DeleteTimerQuery(TimerQueryHandle handle);

        void ResizeSwapChain(SwapChainHandle handle, SwapchainDesc const& desc);

        const TextureDesc& GetTextureDesc(TextureHandle handle);
        const BufferDesc& GetBufferDesc(BufferHandle handle);

        DescriptorIndex GetDescriptorIndex(TextureHandle handle, SubresouceType type, int subResource = -1);
        DescriptorIndex GetDescriptorIndex(BufferHandle handle, SubresouceType type, int subResource = -1);

        void CreateShaderResourceView(BufferHandle handle, size_t offset, size_t size = ~0u);
        void CreateUnorderedAccessView(BufferHandle handle, size_t offset, size_t size = ~0u);

        void CreateShaderResourceView(TextureHandle handle, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip = 0, uint32_t mipCount = ~0);
        void CreateUnorderedAccessView(TextureHandle handle, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip = 0, uint32_t mipCount = ~0);
        void CreateRenderTargetView(TextureHandle handle, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip = 0, uint32_t mipCount = ~0);
        void CreateDepthStencilView(TextureHandle handle, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip = 0, uint32_t mipCount = ~0);

    public:
        Core::Pool<D3D12Texture, Texture>& GetTexturePool() { return this->m_texturePool; }
        Core::Pool<D3D12CommandSignature, CommandSignature>& GetCommandSignaturePool() { return this->m_commandSignaturePool; }
        Core::Pool<D3D12Shader, RHI::Shader>& GetShaderPool() { return this->m_shaderPool; }
        Core::Pool<D3D12InputLayout, InputLayout>& GetInputLayoutPool() { return this->m_inputLayoutPool; }
        Core::Pool<D3D12Buffer, Buffer>& GetBufferPool() { return this->m_bufferPool; }
        Core::Pool<D3D12RTAccelerationStructure, RTAccelerationStructure>& GetRtAccelerationStructurePool() { return this->m_rtAccelerationStructurePool; }
        Core::Pool<D3D12GfxPipeline, GfxPipeline>& GetGfxPipelinePool() { return this->m_gfxPipelinePool; }
        Core::Pool<D3D12ComputePipeline, ComputePipeline>& GetComputePipelinePool() { return this->m_computePipelinePool; }
        Core::Pool<D3D12MeshPipeline, MeshPipeline>& GetMeshPipelinePool() { return this->m_meshPipelinePool; }
        Core::Pool<D3D12TimerQuery, TimerQuery>& GetTimerQueryPool() { return this->m_timerQueryPool; }
        Core::Pool<D3D12SwapChain, SwapChain>& GetSwapChainPool() { return this->m_swapChainPool; }

        D3D12CpuDescriptorHeap& GetResourceCpuHeap() { return this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV]; }
        D3D12CpuDescriptorHeap& GetSamplerCpuHeap() { return this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler]; }
        D3D12CpuDescriptorHeap& GetRtvCpuHeap() { return this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::RTV]; }
        D3D12CpuDescriptorHeap& GetDsvCpuHeap() { return this->m_cpuDescriptorHeaps[(int)DescriptorHeapTypes::DSV]; }

        D3D12GpuDescriptorHeap& GetResourceGpuHeap() { return this->m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::CBV_SRV_UAV]; }
        D3D12GpuDescriptorHeap& GetSamplerGpuHeap() { return this->m_gpuDescriptorHeaps[(int)DescriptorHeapTypes::Sampler]; }

    private:
        std::shared_ptr<D3D12Device> m_device;
        std::shared_ptr<D3D12GpuMemoryAllocator> m_gpuAllocator;

        Core::Pool<D3D12Texture, Texture> m_texturePool;
        Core::Pool<D3D12CommandSignature, CommandSignature> m_commandSignaturePool;
        Core::Pool<D3D12Shader, RHI::Shader> m_shaderPool;
        Core::Pool<D3D12InputLayout, InputLayout> m_inputLayoutPool;
        Core::Pool<D3D12Buffer, Buffer> m_bufferPool;
        Core::Pool<D3D12RTAccelerationStructure, RTAccelerationStructure> m_rtAccelerationStructurePool;
        Core::Pool<D3D12GfxPipeline, GfxPipeline> m_gfxPipelinePool;
        Core::Pool<D3D12ComputePipeline, ComputePipeline> m_computePipelinePool;
        Core::Pool<D3D12MeshPipeline, MeshPipeline> m_meshPipelinePool;
        Core::Pool<D3D12TimerQuery, TimerQuery> m_timerQueryPool;
        Core::Pool<D3D12SwapChain, SwapChain> m_swapChainPool;

        std::array<D3D12CpuDescriptorHeap, (int)DescriptorHeapTypes::Count> m_cpuDescriptorHeaps;
        std::array<D3D12GpuDescriptorHeap, 2> m_gpuDescriptorHeaps;
        D3D12BindlessDescriptorTable m_bindlessResourceDescriptorTable;

        struct DeleteItem
        {
            uint64_t Frame;
            std::function<void()> DeleteFn;
        };
        
        Core::SpinLock m_deleteQueueGuard;
        std::deque<DeleteItem> m_deleteQueue;

        size_t m_frame = 0;
    };
}

