#pragma once

#include <PhxEngine/RHI/RHITypes.h>

namespace phx::ToneMapBlit
{
    bool Initialize();
    void Shutdown();

    constexpr float k_default_exposure = 0.0f;

    // `source` is a bindless SRV index into whatever resource heap the caller
    // already bound via CmdSetDescriptorHeaps — this pass owns no descriptor
    // state of its own, it just draws. Tonemaps into `destination`.
    void Blit(rhi::DescriptorIndex source, rhi::TextureHandle destination, rhi::CommandBuffer cmd, float exposure = k_default_exposure);

    // Overload targeting the current viewport/swapchain image.
    void Blit(rhi::DescriptorIndex source, rhi::CommandBuffer cmd, float exposure = k_default_exposure);
}
