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

        // Returns the command buffer this frame's rendering was recorded
        // into — Engine::Run passes it straight to rhi::SubmitAndPresent.
        [[nodiscard]] virtual rhi::CommandBuffer OnRender(const FrameRenderTargets& targets) = 0;

        virtual void                OnShutdown()                                = 0;
    };
}