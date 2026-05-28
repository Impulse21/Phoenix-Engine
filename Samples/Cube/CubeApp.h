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
        const char* GetName() const override;
    
        // -- Application interface impl ---
    public:
        void OnInit() override;
        void OnFillWorld(phx::RenderWorld& world) override;
        void OnUpdate(float dt) override;
        void OnRender() override;
        void OnShutdown() override;
    };
}