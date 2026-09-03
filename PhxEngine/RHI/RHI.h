#pragma once

#include <PhxEngine/Core/Handle.h>
#include <PhxEngine/Memory/ScratchAllocator.h>

#include "RHITypes.h"

namespace phx::rhi
{
    struct InitParam
    {
        const char* app_name = nullptr;

        // The engine's one viewport is created implicitly as part of
        // Initialize() — there is no separate CreateViewport call.
        ViewportDesc viewport = {};

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
    bool BeginFrame();

    // Ends and submits exactly the command buffers passed in (each one must
    // have come from BeginCommandRecording this frame), then presents. The
    // RHI still records one small internal command buffer of its own to
    // transition the swapchain image for presentation — callers never need
    // to think about that part.
    bool SubmitAndPresent(Span<CommandBuffer> cmds);

    // -- Resource Factory Methods ---

    // -- Viewport API ---
    // Describes the one viewport Initialize() created.
    bool GetViewportDesc(ViewportDesc& out_desc);

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
    // Starts recording and hands back a transient CommandBuffer for this use
    // only — there's no separate create/destroy step. Pass it to
    // SubmitAndPresent when done; don't hold onto it past that point.
    [[nodiscard]] CommandBuffer BeginCommandRecording(CommandQueueType type = CommandQueueType::Graphics);

    void BeginRenderPass(
        TextureHandle texture,
        const ClearValue& clear,
        TextureHandle depth_texture,
        const ClearValue& depth_clear_value,
        CommandBuffer cmd);

    void BeginRenderPass(const ClearValue& clear, CommandBuffer cmd);
    void EndRenderPass(CommandBuffer cmd);

    // -- Draw & Binding ---
    // BeginRenderPass already sets a full-target viewport/scissor, so a
    // simple full-screen pass needs nothing extra before these.
    void BindPipelineState(PipelineStateHandle pipeline, CommandBuffer cmd);
    void SetPushConstants(CommandBuffer cmd, const void* data, u32 size);
    void Draw(CommandBuffer cmd, u32 vertex_count, u32 instance_count = 1, u32 first_vertex = 0, u32 first_instance = 0);

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
    void Barrier(CommandBuffer cmd, BarrierStage src = BarrierStage::All, BarrierStage dst = BarrierStage::All);
}