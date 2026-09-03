#pragma once

#include <PhxEngine/RHI/RHITypes.h>

namespace phx::ToneMapBlit
{
    // Requires ShaderCompiler to already be initialized, and the VFS
    // "engine_shaders://" mount (set up by Engine::Initialize) to exist.
    // Builds the blit pipeline for the engine's one viewport's current format.
    bool Initialize();
    void Shutdown();

    // Draws `source` into the viewport as a full-screen triangle, applying
    // an ACES filmic tonemap + gamma encode. `exposure` is a linear scale
    // applied before tonemapping (1.0 = no adjustment). Opens and closes
    // its own render pass — call between BeginCommandRecording and
    // submission, not inside an existing BeginRenderPass/EndRenderPass pair.
    //
    // `source` must have been created with BindingFlags::ShaderResource.
    void Blit(rhi::TextureHandle source, rhi::CommandBufferHandle cmd, float exposure = 1.0f);
}
