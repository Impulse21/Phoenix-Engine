#pragma once

#include <PhxEngine/Core/EnumUtils.h>
#include <PhxEngine/Core/Span.h>

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
        phx::rhi::PipelineStateHandle CreatePso(phx::Span<phx::rhi::ShaderStageInfo> shader_tages, const phx::rhi::ViewportDesc& viewport_desc) const;

    private:
        // TODO: Add Render Targets
        phx::EnumArray<phx::rhi::PipelineStateHandle, Pso> m_pso;
    };
}