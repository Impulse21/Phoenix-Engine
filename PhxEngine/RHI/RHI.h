#pragma once

#include <PhxEngine/Core/Handle.h>
#include <PhxEngine/Memory/ScratchAllocator.h>

#include "RHITypes.h"

namespace phx::rhi
{
    struct InitParam
    {
        const char* app_name = nullptr;
        
        u32  max_cmd_buffers_per_thread = 0; // if 0, we will use max number of threads.
        bool enable_validation          = false;
        bool enable_best_practices      = true;
        bool enable_sync_validation     = false;
        bool enable_gpu_assisted        = false; // Very Expensive

        u32 max_textures                = 1024;
        u32 max_buffers                 = 2048;
        u32 max_pipelines               = 256;
        u32 max_samplers                = 128;
        u32 max_shader_modules          = 128;
    };

    // -- Setup ---
    bool Initialize(const InitParam& param);
    void Shutdown();

    // -- RHI Info ---
    constexpr u32 MaxFramesInFlight = 2;
    [[nodiscard]] constexpr ShaderFormat GetShaderFormat();

    // -- Frame Submission ---
    bool BeginFrame(ViewportHandle viewport);
    bool EndFrame(ViewportHandle viewport);

    // -- Resource Factory Methods ---

    // -- Viewport API ---
    ViewportHandle CreateViewport(const ViewportDesc& desc);
    void DestoryViewport(ViewportHandle handle);
    bool GetViewportDesc(ViewportHandle handle, ViewportDesc& out_desc);

    // -- Command Buffer API ---
    CommandBufferHandle CreateCommandBuffer(const CommandBufferDesc& desc);
    void DestoryCommandBuffer(CommandBufferHandle handle);

    // -- Texture API ---
    TextureHandle CreateTexture(const TextureDescriptor& desc);
    void DestroyTexture(TextureHandle handle);

    // -- Buffer API ---
    GpuBufferHandle CreateBuffer(const GpuBufferDescriptor& desc);
    void DestroyBuffer(GpuBufferHandle handle);

    // -- Sampler API ---
    SamplerHandle CreateSampler(const SamplerDescriptor& desc);
    void DestroySampler(SamplerHandle handle);
    
    // -- Pipeline State API ---
    PipelineStateHandle CreatePipelineState(const PipelineStateDescriptor& desc);
    void DestroyPipelineState(PipelineStateHandle handle);
    
    // -- Shader Module API ---
    ShaderModuleHandle CreateShaderModule(const ShaderModuleDescriptor& desc);
    void DestroyShaderModule(ShaderModuleHandle handle);
    
    // -- Command Buffer API ---
    bool BeginCommandRecording(CommandBufferHandle cmd_handle);
    void BeginRendering(
        TextureHandle texture,
        const ClearValue& clear,
        TextureHandle depth_texture,
        const ClearValue& depth_clear_value,
        CommandBufferHandle cmd_handle);

    void BeginRendering(ViewportHandle viewport, const ClearValue& clear, CommandBufferHandle cmd_handle);
    void EndRendering(CommandBufferHandle cmd_handle);

    // -- Draw & Binding ---
    // BeginRendering already sets a full-target viewport/scissor, so a
    // simple full-screen pass needs nothing extra before these.
    void BindPipelineState(PipelineStateHandle pipeline, CommandBufferHandle cmd_handle);
    void SetPushConstants(CommandBufferHandle cmd_handle, const void* data, u32 size);
    void Draw(CommandBufferHandle cmd_handle, u32 vertex_count, u32 instance_count = 1, u32 first_vertex = 0, u32 first_instance = 0);

    // -- Resource Introspection ---
    // Bindless index this texture's shader-resource-view was registered at
    // (requires the texture to have been created with BindingFlags::ShaderResource).
    // Returns kInvalidDescriptorIndex otherwise.
    DescriptorIndex GetShaderResourceIndex(TextureHandle handle);

    // -- Synchronization ---
    // A coarse GPU sync point: work in the `src` domain(s) finishes before
    // work in the `dst` domain(s) starts. There is no per-resource state or
    // layout to pass in — with images fixed at a single layout for their
    // whole lifetime, that bookkeeping is gone; `src`/`dst` just say which
    // kind of GPU work is involved, so e.g. a graphics-only write->read
    // doesn't stall compute work that was never touching that data. Default
    // to All/All when unsure. Call it between passes where a later one reads
    // what an earlier one wrote.
    void Barrier(CommandBufferHandle cmd_handle, BarrierStage src = BarrierStage::All, BarrierStage dst = BarrierStage::All);
}