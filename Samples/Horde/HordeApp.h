#pragma once

#include <PhxEngine/Core/PhxDefines.h>

#include <PhxEngine/IApplication.h>
#include <PhxEngine/ECS/World.h>

#include "WorldComponents.h"
#include "HordeRenderer.h"

#include "FrameData.h"

namespace horde
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

    private:
        void CullAndCopy();

    private:
        phx::FramePtr<FrameData> m_frame_data;
        HordeRenderer m_renderer;

        phx::ecs::World m_world{horde::WorldComponentId::NumComponents};
        phx::ecs::EntityId m_main_camera_id;
    };
}
