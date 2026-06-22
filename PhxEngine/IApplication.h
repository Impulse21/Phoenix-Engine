#pragma once

#include <PhxEngine/Renderer/RenderGraph.h>
namespace phx
{
    struct RenderWorld
    {

    };
    
    struct EngineDesc;
    class IApplication
    {
    public:
        virtual ~IApplication() = default;

        virtual const char*         GetName() const                 = 0;
        virtual void                OnInit()                        = 0;
        virtual void                OnBuildGraph(renderer::RenderGraphBuilder& rg_builder, RenderWorld& world) = 0;
        virtual void                OnUpdate(float dt)              = 0;
        virtual void                OnRender()                      = 0;
        virtual void                OnShutdown()                    = 0;
    };
}