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
        const phx::EngineDesc& GetDesc() override;
    
        // -- Application interface impl ---
    public:
        void OnInit() override;
        void OnCache(phx::RenderWorld& world) override;
        void OnUpdate(float dt) override;
        void OnShutdown() override;
    };
}