#pragma once

#include <PhxEngine/RHI/RHITypes.h>

namespace phx
{
    struct RenderWorld
    {

    };
    
    struct FrameRenderTargets
    {
        rhi::TextureHandle scene_colour;
        rhi::TextureHandle depth;

        // No present-target handle — there's exactly one viewport, owned by
        // the RHI context; reach it via rhi::BeginRenderPass(clear, cmd).
    };

    struct EngineDesc;
    class IApplication
    {
    public:
        virtual ~IApplication() = default;

        virtual const char*         GetName() const                             = 0;
        virtual void                OnInit()                                    = 0;
        virtual void                OnPreRender()                               = 0;
        virtual void                OnUpdate(float dt)                          = 0;
        virtual void                OnRender(const FrameRenderTargets& targets) = 0;
        virtual void                OnShutdown()                                = 0;
    };
}