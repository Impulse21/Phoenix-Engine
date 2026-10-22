#pragma once

#include <PhxEngine/Engine.h>
#include <PhxEngine/Core/PhxDefines.h>

#include <PhxEngine/IApplication.h>
#include <PhxEngine/ECS/World.h>
#include <PhxEngine/Renderer/RenderGraphRegistry.h>

#include "WorldComponents.h"
#include "HordeRenderer.h"

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

        void OnBuildPreRenderFrame(phx::Jobs::Graph& graph, phx::FrameData* frame_data) override;
        void OnBuildUpdateFrame(phx::Jobs::Graph& graph, const phx::FrameData* frame_data) override;
        void OnBuildRenderFrame(phx::Jobs::Graph& graph, const phx::FrameData* frame_data) override;

        void OnShutdown() override;

    private:
        void Update(float dt);

    private:
        // Maybe the hooks provide this info.
        phx::FramePtr<phx::FrameData> frame_data;

        horde::HordeRenderer m_renderer;
        phx::ecs::World m_world{horde::WorldComponentId::NumComponents};
    };
}
