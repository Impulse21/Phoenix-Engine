#pragma once

#include <PhxEngine/Core/PhxDefines.h>
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
        void OnBuildGraph(phx::renderer::RenderGraphBuilder& rg_builder, phx::RenderWorld& world) override;
        void OnUpdate(float dt) override;
        void OnRender() override;
        void OnShutdown() override;
    };
}