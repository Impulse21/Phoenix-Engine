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

    [[nodiscard]] DeviceCapabilities GetDeviceCapabilities();

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

#pragma region new_gpu_memory_model
    // -- GPU Memory ---
    // New API for texture and buffer resources
    // Based on https://github.com/sebbbi/NoGraphicsAPI

    // Not sure about this - might be isolated to within the RHI?
    [[nodiscard]] GpuHeap AllocateGpuHeap(u64 size, GpuMemoryType memory_type) noexcept;
    void DestroyGpuHeap(const GpuHeap& heap) noexcept;

    [[nodiscard]] TextureHeap AllocateTextureHeap(u64 size) noexcept;
    void DestroyTextureHeap(const TextureHeap& heap) noexcept;

    [[nodiscard]] TextureHandle CreateTexture(const TextureDescriptor& desc, const TextureHeap& heap, u64 offset) noexcept;
    void DestoryTexture(TextureHandle texture) noexcept;

    SizeAlign GetTextureSizeAlign(const TextureDescriptor& desc) noexcept;

    void WriteDescriptor(TextureHandle handle, void* dest) noexcept;
    void WriteSamplerDescriptor(const SamplerDescriptor& desc, void* dest) noexcept;

    using DeferCallbackFn = FixedCallable<16>;
    void DeferUntilGpuComplete(DeferCallbackFn deferCallback);
    void ExecuteAfter(UploadTicket ticket, DeferCallbackFn deferCallback);

#pragma endregion

    // -- Pipeline State API ---
    PipelineStateHandle CreatePipelineState(const PipelineStateDescriptor& desc);
    void DestroyPipelineState(PipelineStateHandle handle);
    
    // -- Shader Module API ---
    ShaderModuleHandle CreateShaderModule(const ShaderModuleDescriptor& desc);
    void DestroyShaderModule(ShaderModuleHandle handle);
    
    // -- Resource Introspection ---
    // TODO: Remove
    DescriptorIndex GetShaderResourceIndex(TextureHandle handle);

    // -- Command Buffer API ---
    // Starts recording and hands back a transient CommandBuffer for this use
    // only.
    // SubmitAndPresent when done; don't hold onto it past that point.

    // --Command Factory
    [[nodiscard]] CommandBuffer BeginCommandRecording(CommandQueueType type = CommandQueueType::Graphics);

    // -- Command Submission
    [[nodiscard]] UploadTicket SubmitUpload(CommandBuffer cmd);
    void WaitForUpload(UploadTicket ticket);

    void CmdBeginRenderPass(
        TextureHandle texture,
        const ClearValue& clear,
        TextureHandle depth_texture,
        const ClearValue& depth_clear_value,
        CommandBuffer cmd);

    void CmdBeginRenderPass(const ClearValue& clear, CommandBuffer cmd);
    void CmdEndRenderPass(CommandBuffer cmd);

    // -- Cmd Copy ---
    void CmdCopyMemory(CommandBuffer cmd, GpuRange src, GpuRange desc);
    void CmdCopyMemoryToTexture(CommandBuffer cmd, GpuRange src, TextureHandle dest, const TexturCopyDesc& copy_desc);

    struct GpuAllocation {};
    [[nodiscard]] GpuAllocation GpuUploadMalloc(u32 size);


    // -- Draw & Binding ---
    // BeginRenderPass already sets a full-target viewport/scissor, so a
    // simple full-screen pass needs nothing extra before these.
    void CmdBindPipelineState(PipelineStateHandle pipeline, CommandBuffer cmd);
    void CmdSetPushConstants(CommandBuffer cmd, const void* data, u32 size);
    void CmdDraw(CommandBuffer cmd, u32 vertex_count, u32 instance_count = 1, u32 first_vertex = 0, u32 first_instance = 0);
    void CmdDrawIndex(
        CommandBuffer   cmd,
        ByteSpan        root,
        GpuRange        indices,
        IndexFormat     format,
        u32             index_count,
        u32             instance_count = 1,
        u32             first_index = 0,
        i32             vertex_offset = 0,
        u32             first_instance = 0) noexcept;
        
    // -- Synchronization ---
    void CmdBarrier(CommandBuffer cmd, BarrierStage src = BarrierStage::All, BarrierStage dst = BarrierStage::All);
}