#pragma once

#include <PhxEngine/Core/MemoryBuffer.h>
#include <PhxEngine/Core/PhxDefines.h>

#include <PhxEngine/RHI/RHITypes.h>
#include <PhxEngine/RHI/GpuMemory/BumpAllocator.h>

#include <PhxEngine/IApplication.h>

#include "Shaders/Cube_interop.h"
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

        phx::rhi::GpuHeap m_buffer_heap;
        phx::rhi::GpuBumpAllocator m_buffer_allocator;

        struct Mesh
        {
            phx::rhi::GpuCpuRange<Vertex> vertices;
            phx::rhi::GpuCpuRange<u32> indices;
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


        // 24 vertices, 4 per face -- shared corners need distinct normals
        // per face so they can't be welded to 8. Each face is wound
        // 0,1,2,2,3,0 (see cube_indices) with a standard 0..1 UV rect;
        // cull_mode is None so winding doesn't matter for visibility.
        // Not constexpr: interop::float3/float2's converting constructors
        // route through hlslpp's SIMD intrinsics, which aren't constexpr.
        static inline const Vertex cube_vertices[] =
        {
            // Back (-Z)
            { .position = hlslpp::float3(-0.5f,-0.5f,-0.5f), .normal = hlslpp::float3(0.0f, 0.0f,-1.0f), .uv = hlslpp::float2(0.0f, 0.0f) },
            { .position = hlslpp::float3( 0.5f,-0.5f,-0.5f), .normal = hlslpp::float3(0.0f, 0.0f,-1.0f), .uv = hlslpp::float2(1.0f, 0.0f) },
            { .position = hlslpp::float3( 0.5f, 0.5f,-0.5f), .normal = hlslpp::float3(0.0f, 0.0f,-1.0f), .uv = hlslpp::float2(1.0f, 1.0f) },
            { .position = hlslpp::float3(-0.5f, 0.5f,-0.5f), .normal = hlslpp::float3(0.0f, 0.0f,-1.0f), .uv = hlslpp::float2(0.0f, 1.0f) },

            // Front (+Z)
            { .position = hlslpp::float3(-0.5f,-0.5f, 0.5f), .normal = hlslpp::float3(0.0f, 0.0f, 1.0f), .uv = hlslpp::float2(0.0f, 0.0f) },
            { .position = hlslpp::float3( 0.5f,-0.5f, 0.5f), .normal = hlslpp::float3(0.0f, 0.0f, 1.0f), .uv = hlslpp::float2(1.0f, 0.0f) },
            { .position = hlslpp::float3( 0.5f, 0.5f, 0.5f), .normal = hlslpp::float3(0.0f, 0.0f, 1.0f), .uv = hlslpp::float2(1.0f, 1.0f) },
            { .position = hlslpp::float3(-0.5f, 0.5f, 0.5f), .normal = hlslpp::float3(0.0f, 0.0f, 1.0f), .uv = hlslpp::float2(0.0f, 1.0f) },

            // Left (-X)
            { .position = hlslpp::float3(-0.5f,-0.5f,-0.5f), .normal = hlslpp::float3(-1.0f, 0.0f, 0.0f), .uv = hlslpp::float2(0.0f, 0.0f) },
            { .position = hlslpp::float3(-0.5f,-0.5f, 0.5f), .normal = hlslpp::float3(-1.0f, 0.0f, 0.0f), .uv = hlslpp::float2(1.0f, 0.0f) },
            { .position = hlslpp::float3(-0.5f, 0.5f, 0.5f), .normal = hlslpp::float3(-1.0f, 0.0f, 0.0f), .uv = hlslpp::float2(1.0f, 1.0f) },
            { .position = hlslpp::float3(-0.5f, 0.5f,-0.5f), .normal = hlslpp::float3(-1.0f, 0.0f, 0.0f), .uv = hlslpp::float2(0.0f, 1.0f) },

            // Right (+X)
            { .position = hlslpp::float3( 0.5f,-0.5f,-0.5f), .normal = hlslpp::float3(1.0f, 0.0f, 0.0f), .uv = hlslpp::float2(0.0f, 0.0f) },
            { .position = hlslpp::float3( 0.5f,-0.5f, 0.5f), .normal = hlslpp::float3(1.0f, 0.0f, 0.0f), .uv = hlslpp::float2(1.0f, 0.0f) },
            { .position = hlslpp::float3( 0.5f, 0.5f, 0.5f), .normal = hlslpp::float3(1.0f, 0.0f, 0.0f), .uv = hlslpp::float2(1.0f, 1.0f) },
            { .position = hlslpp::float3( 0.5f, 0.5f,-0.5f), .normal = hlslpp::float3(1.0f, 0.0f, 0.0f), .uv = hlslpp::float2(0.0f, 1.0f) },

            // Top (+Y)
            { .position = hlslpp::float3(-0.5f, 0.5f,-0.5f), .normal = hlslpp::float3(0.0f, 1.0f, 0.0f), .uv = hlslpp::float2(0.0f, 0.0f) },
            { .position = hlslpp::float3( 0.5f, 0.5f,-0.5f), .normal = hlslpp::float3(0.0f, 1.0f, 0.0f), .uv = hlslpp::float2(1.0f, 0.0f) },
            { .position = hlslpp::float3( 0.5f, 0.5f, 0.5f), .normal = hlslpp::float3(0.0f, 1.0f, 0.0f), .uv = hlslpp::float2(1.0f, 1.0f) },
            { .position = hlslpp::float3(-0.5f, 0.5f, 0.5f), .normal = hlslpp::float3(0.0f, 1.0f, 0.0f), .uv = hlslpp::float2(0.0f, 1.0f) },

            // Bottom (-Y)
            { .position = hlslpp::float3(-0.5f,-0.5f,-0.5f), .normal = hlslpp::float3(0.0f,-1.0f, 0.0f), .uv = hlslpp::float2(0.0f, 0.0f) },
            { .position = hlslpp::float3( 0.5f,-0.5f,-0.5f), .normal = hlslpp::float3(0.0f,-1.0f, 0.0f), .uv = hlslpp::float2(1.0f, 0.0f) },
            { .position = hlslpp::float3( 0.5f,-0.5f, 0.5f), .normal = hlslpp::float3(0.0f,-1.0f, 0.0f), .uv = hlslpp::float2(1.0f, 1.0f) },
            { .position = hlslpp::float3(-0.5f,-0.5f, 0.5f), .normal = hlslpp::float3(0.0f,-1.0f, 0.0f), .uv = hlslpp::float2(0.0f, 1.0f) },
        };

	    static constexpr u32 cube_indices[] = {
		    0, 1, 2, 2, 3, 0,
		    4, 5, 6, 6, 7, 4,
		    8, 9, 10, 10, 11, 8,
		    12, 13, 14, 14, 15, 12,
		    16, 17, 18, 18, 19, 16,
		    20, 21, 22, 22, 23, 20,
	    };

	    static constexpr u32 cube_vertex_count = u32(sizeof(cube_vertices) / sizeof(cube_vertices[0]));
	    static constexpr u32 cube_index_count = u32(sizeof(cube_indices) / sizeof(cube_indices[0]));


    };
}
