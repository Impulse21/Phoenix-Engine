#pragma once

#include <PhxEngine/Core/MemoryBuffer.h>
#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/RHI/RHITypes.h>

#include <PhxEngine/IApplication.h>

#include <hlsl++.h>

namespace samples
{
    class ModelViewerApp final : public phx::IApplication
    {
    public:
        ModelViewerApp() = default;
        ~ModelViewerApp() override = default;

    public:
        const char* GetName() const override;

        // -- Application interface impl ---
    public:
        void OnInit() override;

        void OnBuildPreRenderFrame(phx::Jobs::Graph& graph) override;
        void OnBuildUpdateFrame(phx::Jobs::Graph& graph, float dt) override;
        void OnBuildRenderFrame(phx::Jobs::Graph& graph, const phx::FrameRenderTargets& targets, phx::rhi::CommandBuffer& out_cmd) override;

        void OnShutdown() override;

    private:
        void PreRender();
        void Update(float dt);
        phx::rhi::CommandBuffer Render(const phx::FrameRenderTargets& targets);

    private:
        phx::rhi::ShaderModuleHandle m_vertex_shader;
        phx::rhi::ShaderModuleHandle m_fragment_shader;
        phx::rhi::PipelineStateHandle m_cube_pipeline;

        struct Mesh
        {
            phx::rhi::GpuAllocation vertices;
            phx::rhi::GpuAllocation indices;
        } m_mesh;

        float m_time = 0.0f;

        // Cached once per frame by PreRender (frame-allocated -- valid
        // only for the frame that made it) so Render doesn't recompute the
        // camera/MVP itself.
        struct RenderPacket
        {
            hlslpp::float4x4 mvp;
            Mesh* mesh;
        };

        RenderPacket* m_render_packet = nullptr;
    };
}
