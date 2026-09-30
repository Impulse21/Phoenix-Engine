#pragma once

#include <PhxEngine/Core/EnumUtils.h>
#include <PhxEngine/RHI/RHITypes.h>

namespace horde
{
    enum class Pso : u32
    {
        Capsule,
        Plane,
        Box,
        Count,
    };

    constexpr u32 kPsoCount = static_cast<u32>(Pso::Count);

    class HordeRenderer
    {
    public:
        [[nodiscard]] bool Initialize() noexcept;
        void Shutdown();
        
    private:
        // TODO: Add Render Targets
        phx::EnumArray<rhi::PipelineState, Pso> m_pso;
    };
}