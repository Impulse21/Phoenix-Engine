#pragma once

namespace phx
{
    struct RenderWorld;
    struct EngineDesc;
    class IApplication
    {
    public:
        virtual ~IApplication() = default;

        virtual const EngineDesc&  GetDesc()                = 0;
        virtual void        OnInit()                        = 0;
        virtual void        OnCache(RenderWorld& world)     = 0;  // ← new
        virtual void        OnUpdate(float dt)              = 0;
        virtual void        OnShutdown()                    = 0;
    };
}