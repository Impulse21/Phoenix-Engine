#pragma once
#include "PhxRhi_Types.h"

namespace phx::rhi
{
    // -- Core --
	bool Initialize(Descriptor const& descriptor, void* window_handle, size_t thread_count);
    void Shutdown();
    ShaderFormat GetShaderFormat();
    GfxBackend GetBackend();

    // -- Resources --
    SwapchainHandle CreateSwapchain(const SwapchainDesc& desc);
    void DeleteSwapchain(SwapchainHandle handle);
    TextureHandle GetSwapchainBackBuffer(SwapchainHandle handle);

    void ResizeSwapchain(SwapchainHandle handle, uint32_t width, uint32_t height);
    BufferHandle CreateBuffer(const BufferDescriptor& desc, const void* initial_data = nullptr);
    void DeleteBuffer(BufferHandle handle);
    uint64_t GetGpuAddress(BufferHandle handle);

    TextureHandle CreateTexture(const TextureDescriptor& desc, const void* initial_data = nullptr);
    void DeleteTexture(TextureHandle handle);

    ShaderModuleHandle CreateShaderModule(const ShaderModuleDescriptor& desc);
    void DeleteShaderModule(ShaderModuleHandle handle);

    PipelineStateHandle CreatePipeline(const PipelineStateDescriptor& desc);\
    void DeletePipeline(PipelineStateHandle handle);

    // -- Submission & Sync --
    void BeginFrame(SwapchainHandle swap_chain);
    
    void EndFrame(SwapchainHandle swap_chain, Span<CmdHandle> graphics_buffers, Span<FenceHandle> wait_fences = {});

    void WaitForIdle();
    
    bool IsFenceCompleted(FenceHandle handle);

    StagingBlock RequestStagingMemory(uint32_t size, uint32_t alignment = 16);

    FenceHandle Submit(CommandQueueType queue_type, Span<CmdHandle> cmd_buffers, Span<FenceHandle> wait_fences = {});

    // -- Command Recording --

    CmdHandle BeginCommandBuffer(CommandQueueType queue_type);

    void PushConstants(CmdHandle cmd, const void* data, uint32_t size, uint32_t offset = 0);
    void BindPipelineState(CmdHandle cmd, PipelineStateHandle pso);

    void Draw(CmdHandle cmd, uint32_t vertex_count, uint32_t start_vertex_location);
    
    void DrawIndexed(CmdHandle cmd, uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location);
    
    void DrawInstanced(CmdHandle cmd, uint32_t vertex_count, uint32_t instance_count, uint32_t start_vertex_location, uint32_t start_instance_location);
    
    void DrawIndexedInstanced(CmdHandle cmd, uint32_t index_count, uint32_t instance_count, uint32_t start_index_location, int32_t base_vertex_location, uint32_t start_instance_location);

    void BeginRendering(CmdHandle cmd, SwapchainHandle handle, const ClearValue& clear_value);
    
    void EndRendering(CmdHandle cmd);

    void InsertSwapchainBarrier(CmdHandle cmd, SwapchainHandle handle, ResourceStates resource_state);
    
    void InsertBarriers(CmdHandle cmd, Span<GpuBarrier> barriers);

    void CopyBuffer(CmdHandle cmd, BufferHandle src_buffer, uint64_t src_offset, BufferHandle dest_buffer, uint64_t dest_offset, size_t size);
}