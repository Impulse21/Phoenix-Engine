#pragma once

#include <PhxEngine/Core/Handle.h>
#include <PhxEngine/Memory/ScratchAllocator.h>
#include <PhxEngine/Core/FixedCallable.h>

#include "RHITypes.h"

#include <cstring>

namespace phx::rhi
{
    struct InitParam
    {
        const char* app_name = nullptr;

        ViewportDesc viewport = {};

        u32  max_cmd_buffers_per_thread = 0; // if 0, we will use max number of threads.
        bool enable_validation          = false;
        bool enable_best_practices      = true;
        bool enable_sync_validation     = false;
        bool enable_gpu_assisted        = false; // Very Expensive

        u32 max_textures                = 1024;
        u32 max_pipelines               = 256;
        u32 max_samplers                = 128;
        u32 max_shader_modules          = 128;

        u32 gpu_temp_ring_size          = 8_MB;

        u32 gpu_arena_size_device_local = 256_MB;
        u32 gpu_arena_size_upload       = 32_MB;
        u32 gpu_arena_size_readback     = 16_MB;

        u32 gpu_upload_ring_slot_size   = 8_MB;
    };

    // -- Setup ---
    bool Initialize(const InitParam& param);
    void Shutdown();

    // -- RHI Info ---
    constexpr u32 MaxFramesInFlight = 2;

    // Not constexpr: neither of these is used in a constant-expression
    // context anywhere, and a constexpr function declared here with its
    // body defined out-of-line in a single per-backend .cpp (see
    // VulkanRHIInfo.cpp) only links correctly from that one file — any
    // other translation unit sees just the declaration and needs an
    // external symbol, which an implicitly-inline constexpr function
    // doesn't reliably emit. Plain declared-here/defined-once-per-backend
    // functions, like everything else in this header, avoid the problem.
    [[nodiscard]] ShaderFormat GetShaderFormat();

    // True if this backend's clip space has Y pointing down (Vulkan) rather
    // than up (D3D). Callers building a projection matrix with a Y-up-assuming
    // library (e.g. hlslpp) should negate the projection's Y row when this is
    // true, instead of special-casing the shader.
    [[nodiscard]] bool IsClipSpaceYDown();

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

    void UploadTextureData(CommandBuffer cmd, TextureHandle texture, Span<const TextureUploadRegion> regions);
    [[nodiscard]] TextureHandle CreateTextureWithData(const TextureDescriptor& desc, Span<const TextureUploadRegion> regions);

    // -- GPU Memory ---
    // New API for texture and buffer resources
    // Based on https://github.com/sebbbi/NoGraphicsAPI

    // Not sure about this - might be isolated to within the RHI?
    [[nodiscard]] GpuHeap AllocateGpuHeap(u64 byte_count, GpuMemoryType memory_type) noexcept;
    void DestroyGpuHeap(const GpuHeap& heap) noexcept;

    [[nodiscard]] TextureHeap CreateTextureHeap(u64 byte_count) noexcept;
    void DestroyTextureHeap(const TextureHeap& heap) noexcept;

    SizeAlign GetTextureSizeAlign(const TextureDescriptor& desc) noexcept;
    [[nodiscard]] TextureHandle CreateTexture(const TextureDescriptor& desc, TextureHeap* heap = nullptr, u64 offset = 0) noexcept;
    
    // void WriteTextureDescriptor
    using DeferCallbackFn = FixedCallable<8>;
    void DeferUntilGpuComplete(DeferCallbackFn deferCallback);
    void ExecuteAfter(UploadTicket ticket, DeferCallbackFn deferCallback);
    
    // -- Sampler API ---
    // TODO: Determine what needs to be done with this.
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
    // only.
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

    // -- Upload / Transfer Queue ---
    // Submits cmd (recorded via BeginCommandRecording(CommandQueueType::Copy))
    // to the transfer queue immediately — not gated by BeginFrame/SubmitAndPresent,
    // so streaming uploads don't have to wait on the render loop's cadence.
    // Main-thread-only for now, same as BeginCommandRecording.
    [[nodiscard]] UploadTicket SubmitUpload(CommandBuffer cmd);

    // Blocks the calling thread until the GPU work represented by `ticket`
    // (and everything submitted before it on the transfer queue) has completed.
    void WaitForUpload(UploadTicket ticket);

    // Bump-allocates staging memory from a small ring dedicated to uploads.
    // Reclaimed against upload completion (via WaitForUpload internally),
    // not the render frame's cadence — never freed individually.
    [[nodiscard]] GpuAllocation GpuUploadMalloc(u32 size);

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