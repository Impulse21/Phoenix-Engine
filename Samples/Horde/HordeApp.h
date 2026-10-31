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

        void FramePreRender(phx::Jobs::Barrier& frame_barrier) override;
        void FrameUpdate(float dt, phx::Jobs::Barrier& frame_barrier) override;
        void FrameRender(phx::Jobs::Barrier& frame_barrier) override;

        void OnShutdown() override;

    private:
        void Update(float dt);

    private:
        void CullAndCopy();

        void PreloadTextures();

    private:
        phx::FramePtr<FrameData> m_frame_data;
        HordeRenderer m_renderer;

        phx::ecs::World m_world{horde::WorldComponentId::NumComponents};
        phx::ecs::EntityId m_main_camera_id;

    };
}
