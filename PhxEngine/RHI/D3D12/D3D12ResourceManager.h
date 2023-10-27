#pragma once

#include <functional>
#include <deque>

#include <Core/Span.h>
#include <Core/Pool.h>
#include <Core/SpinLock.h>
#include <RHI/RHIResources.h>
#include <D3D12Resources.h>

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
        SwapChainHandle CreateSwapChain(SwapchainDesc const& desc, void* windowHandle);
        ShaderHandle CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode);
        InputLayoutHandle CreateInputLayout(Core::Span<VertexAttributeDesc> descriptions, uint32_t attributeCount);
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

