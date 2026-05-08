#pragma once

#include <PhxEngine/IApplication.h>

namespace samples
{
    class CubeApp final : public phx::IApplication
    {
    public:
        CubeApp() = default;
        ~CubeApp() override = default;

        public:

        phx::EngineDesc GetDesc() override;
        
        void OnInit() override;
        void OnCache(phx::RenderWorld& world) override;
        void OnUpdate(float dt) override;
        void OnShutdown() override;
    };
}