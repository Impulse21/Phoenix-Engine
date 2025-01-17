#pragma once

#include "RHITypes.h"

namespace phx::rhi
{
    void Initialize();
    void Finalize();

    ShaderFormat GetShaderFormat();

    void Present(phx::Span<SwapChainHandle> swapchains);

    // TODO Context management;


    namespace ResourceManager
    {
        CommandListHandle CreateCommandList(CommandQueueType type);
        void DeleteCommandList(CommandListHandle handle);

        SwapChainHandle CreateSwapChain(SwapChainDescriptor const &desc);
        void CreateSwapChain(SwapChainDescriptor const &desc, SwapChainHandle handle);
        void DeleteSwapChain(SwapChainHandle swapChain);

        PipelineStateHandle CreatePipeline(PipelineStateDescriptor const &desc);
        void DeletePipeline(PipelineStateHandle handle);

        TextureHandle CreateTexture(TextureDescriptor const &desc, MemInfo *initData = nullptr);
        void DeleteTexture(TextureHandle handle);

        GpuBufferHandle CreateBuffer(GpuBufferDescriptor const &desc, MemInfo *initData = nullptr);
        void DeleteBuffer(GpuBufferHandle handle);

        DescriptorIndex GetDescriptorIndex(TextureHandle texture, SubresouceType type = SubresouceType::SRV);
    }
}