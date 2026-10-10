#pragma once

#include <PhxEngine/RHI/RHITypes.h>

#include <PhxEngine/RHI/GpuMemory/BumpAllocator.h>

namespace phx::ToneMapBlit
{
    bool Initialize();
    void Shutdown();

    constexpr float k_default_exposure = 0.0f;

    struct RecordContext
    {
        rhi::CommandBuffer cmd_buffer;
        rhi::GpuBumpAllocator* gpu_frame_allocator;
    };

    // `source` is a bindless SRV index into whatever resource heap the caller
    // already bound via CmdSetDescriptorHeaps — this pass owns no descriptor
    // state of its own, it just draws. Tonemaps into `destination`.
    void Blit(rhi::DescriptorIndex source, rhi::TextureHandle destination, const RecordContext& ctx, float exposure = k_default_exposure);

    // Overload targeting the current viewport/swapchain image.
    void Blit(rhi::DescriptorIndex source, const RecordContext& ctx, float exposure = k_default_exposure);
}
