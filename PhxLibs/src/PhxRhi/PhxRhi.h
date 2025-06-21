#pragma once

#include <PhxRhi/RHICommon.h>

namespace phx::RHI
{
    struct RhiDescriptor
    {
        SwapChainDescriptor SwapChainDesc = {};
        void* WindowsHandle = nullptr;
        uint32_t MaxNumTextures = 1000;
        uint32_t MaxNumGpuBuffers = 1000;
        uint32_t MaxNumPipelineStates = 1000;
    };

    bool Initialize(RhiDescriptor const& desc);
    void Shutdown();

    CommandCtxHandle BeginFrameGfxContext();

    CommandCtxHandle BeginAsyncCopyContext();
    void SubmitAsyncCopyContext(phx::Span<CommandCtxHandle> contexts);

    void SubmitAndPresentFrame();
    void WaitForIdle();

    GpuBufferHandle CreateBuffer(const GpuBufferDescriptor& desc, const void* initialData = nullptr);
    TextureHandle CreateTexture(const TextureDescriptor& desc, const void* initialData = nullptr);
    PipelineStateHandle CreatePipeline(const PipelineStateDescriptor& desc);

    void DeletePipeline(PipelineStateHandle handle);
    void DeleteTexture(TextureHandle handle);
    void DeleteBuffer(GpuBufferHandle handle);

    DescriptorIndex GetDescriptorIndex(TextureHandle handle, SubresouceType type = SubresouceType::SRV);
    Budget GetBudget();
    ShaderFormat GetShaderFormat();
}