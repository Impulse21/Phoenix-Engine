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



    CommandBufferHandle BeginFrameCommandBuffer(CommandQueueType type = CommandQueueType::Graphics);
    CommandBufferHandle BeginAsyncCommandBuffer(CommandQueueType type);

    FenceHandle SubmitAsyncCommandBuffer(phx::Span<CommandBufferHandle> contexts);

    namespace CommandRecorder
    {
        void BindPipelineState(CommandBufferHandle handle, PipelineStateHandle pso);
        void Draw(CommandBufferHandle handle, uint32_t vertex_count, uint32_t start_vertex_location);
        void DrawIndexed(CommandBufferHandle handle, uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location);
        void DrawInstanced(CommandBufferHandle handle, uint32_t vertex_count, uint32_t instance_count, uint32_t start_vertex_location, uint32_t start_instance_location);
        void DrawIndexedInstanced(CommandBufferHandle handle, uint32_t index_count, uint32_t instance_count, uint32_t start_index_location, int32_t base_vertex_location, uint32_t startInstanceLocation);
    }
}