#pragma once

#include <PhxEngine/Core/MemoryBuffer.h>
#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/RHI/RHITypes.h>

#include <PhxEngine/IApplication.h>

#include <hlsl++.h>

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
        void OnPreRender() override;
        void OnUpdate(float dt) override;
        phx::rhi::CommandBuffer OnRender(const phx::FrameRenderTargets& targets) override;
        void OnShutdown() override;


    private:
        phx::rhi::ShaderModuleHandle m_vertex_shader;
        phx::rhi::ShaderModuleHandle m_fragment_shader;
        phx::rhi::PipelineStateHandle m_cube_pipeline;

        float m_time = 0.0f;
    };
}