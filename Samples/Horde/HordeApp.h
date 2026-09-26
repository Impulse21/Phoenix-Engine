#pragma once

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/IApplication.h>
#include <PhxEngine/ECS/World.h>

#include "WorldComponents.h"

namespace samples
{
    class HordeApp final : public phx::IApplication
    {
    public:
        HordeApp() = default;
        ~HordeApp() override = default;

    public:
        const char* GetName() const override;

        // -- Application interface impl ---
    public:
        void OnInit() override;

        void OnBuildPreRenderFrame(phx::Jobs::Graph& graph) override;
        void OnBuildUpdateFrame(phx::Jobs::Graph& graph, float dt) override;
        void OnBuildRenderFrame(phx::Jobs::Graph& graph) override;

        void OnShutdown() override;

    private:
        void Update(float dt);
        void Render();

    private:
        phx::ecs::World m_world{horde::WorldComponentId::NumComponents};
    };
}
