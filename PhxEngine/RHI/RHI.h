#pragma once

#include <PhxEngine/Core/Handle.h>
#include <PhxEngine/Memory/ScratchAllocator.h>
#include <PhxEngine/Memory/IHeapAllocator.h>

#include "RHITypes.h"

namespace phx::rhi
{
    struct InitParam
    {
        IHeapAllocator* heap_allocator = nullptr;
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
    [[nodiscard]] constexpr ShaderFormat GetShaderFormat();

    // -- Frame Submission ---
    bool BeginFrame(ViewportHandle viewport);
    bool EndFrame(ViewportHandle viewport);

    // -- Resource Factory Methods ---
    ViewportHandle CreateViewport(const ViewportDesc& desc);
    void DestoryViewport(ViewportHandle handle);
    bool GetViewportDesc(ViewportHandle handle, ViewportDesc& out_desc);

    CommandBufferHandle CreateCommandBuffer(const CommandBufferDesc& desc);
    void DestoryCommandBuffer(CommandBufferHandle handle);

    TextureHandle CreateTexture(const TextureDescriptor& desc);
    void DestroyTexture(TextureHandle handle);

    GpuBufferHandle CreateBuffer(const GpuBufferDescriptor& desc);
    void DestroyBuffer(GpuBufferHandle handle);
    
    // -- Command Buffer API ---
    bool BeginCommandRecording(CommandBufferHandle cmd_handle);
    void BeginRendering(ViewportHandle viewport, const ClearValue& clear, CommandBufferHandle cmd_handle);
    void EndRendering(CommandBufferHandle cmd_handle);
}