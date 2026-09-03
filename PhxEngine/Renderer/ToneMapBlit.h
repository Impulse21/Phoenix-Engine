#pragma once

#include <PhxEngine/RHI/RHITypes.h>

namespace phx::ToneMapBlit
{
    bool Initialize();
    void Shutdown();

    void Blit(rhi::TextureHandle source, rhi::CommandBuffer cmd, float exposure = 1.0f);
}
