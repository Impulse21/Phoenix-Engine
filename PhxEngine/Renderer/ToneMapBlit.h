#pragma once

#include <PhxEngine/RHI/RHITypes.h>

namespace phx::ToneMapBlit
{
    bool Initialize();
    void Shutdown();

    // `source` is a bindless SRV index into whatever resource heap the caller
    // already bound via CmdSetDescriptorHeaps — this pass owns no descriptor
    // state of its own, it just draws. Tonemaps into `destination`.
    void Blit(rhi::DescriptorIndex source, rhi::TextureHandle destination, rhi::CommandBuffer cmd, float exposure = 1.0f);

    // Overload targeting the current viewport/swapchain image.
    void Blit(rhi::DescriptorIndex source, rhi::CommandBuffer cmd, float exposure = 1.0f);
}
