#pragma once

#include <PhxEngine/RHI/RHITypes.h>
#include <PhxEngine/Renderer/IRenderer.h>
#include <PhxEngine/Core/Jobs.h>

namespace phx
{
    struct EngineDesc;
    class IApplication
    {
    public:
        virtual ~IApplication() = default;

        virtual const char*             GetName() const                             = 0;
        virtual void                    OnInit()                                    = 0;

        virtual renderer::IRenderer&    GetRenderer()                               = 0;

        virtual void OnBuildPreRenderFrame(Jobs::Graph& graph)                                          = 0;
        virtual void OnBuildUpdateFrame(Jobs::Graph& graph, float dt)                                   = 0;
        virtual void OnBuildRenderFrame(Jobs::Graph& graph, const FrameRenderTargets& targets, rhi::CommandBuffer& out_cmd) = 0;

        virtual void                OnShutdown()                                = 0;
    };
}
