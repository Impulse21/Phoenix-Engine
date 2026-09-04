#pragma once

#include <PhxEngine/RHI/RHITypes.h>

#include <PhxEngine/Core/Jobs.h>

namespace phx
{
    struct FrameRenderTargets
    {
        rhi::TextureHandle scene_colour;
        rhi::TextureHandle depth;
    };

    struct EngineDesc;
    class IApplication
    {
    public:
        virtual ~IApplication() = default;

        virtual const char*         GetName() const                             = 0;
        virtual void                OnInit()                                    = 0;

        // Each builds its own section of the frame graph -- Engine::Run
        // composes them into one graph (PreRender must finish before
        // Update or Render start; Update and Render then run concurrently)
        // and drives the result. `out_cmd` must be set by whichever task
        // in the Render graph records the frame's command buffer --
        // Engine::Run reads it only after the whole graph has finished.
        virtual void OnBuildPreRenderFrame(Jobs::Graph& graph)                                          = 0;
        virtual void OnBuildUpdateFrame(Jobs::Graph& graph, float dt)                                   = 0;
        virtual void OnBuildRenderFrame(Jobs::Graph& graph, const FrameRenderTargets& targets, rhi::CommandBuffer& out_cmd) = 0;

        virtual void                OnShutdown()                                = 0;
    };
}
