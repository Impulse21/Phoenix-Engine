#pragma once

#include <PhxEngine/RHI/RHITypes.h>

namespace phx::ToneMapBlit
{
    // Requires ShaderCompiler to already be initialized, and the VFS
    // "engine_shaders://" mount (set up by Engine::Initialize) to exist.
    // Builds the blit pipeline for present_target's current format.
    bool Initialize(rhi::ViewportHandle present_target);
    void Shutdown();

    // Draws `source` into `dest` as a full-screen triangle, applying an
    // ACES filmic tonemap + gamma encode. `exposure` is a linear scale
    // applied before tonemapping (1.0 = no adjustment). Opens and closes
    // its own render pass — call between BeginCommandRecording and
    // submission, not inside an existing BeginRendering/EndRendering pair.
    //
    // `source` must have been created with BindingFlags::ShaderResource.
    void Blit(rhi::TextureHandle source, rhi::ViewportHandle dest, rhi::CommandBufferHandle cmd, float exposure = 1.0f);
}
