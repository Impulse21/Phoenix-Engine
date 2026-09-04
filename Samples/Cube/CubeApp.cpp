#include "CubeApp.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Memory/TlsfHeapAllocator.h>
#include <PhxEngine/Memory/MemoryHelpers.h>

#include <PhxEngine/Renderer/ShaderCompiler.h>
#include <PhxEngine/Renderer/ToneMapBlit.h>
#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/VFS/VFS.h>

#include <PhxEngine/Platform/EntryPoint.h>
#include <PhxEngine/Engine.h>

#include <cstring>
#include <utility>

using namespace samples;
using namespace phx;

PHX_DEFINE_APP(CubeApp);

namespace
{
    // Byte-exact match for Cube.slang's Vertex — deliberately not
    // hlslpp::float3, which is a SIMD type with no guaranteed tight
    // 12-byte layout.
    struct GpuVertex
    {
        float position[3];
        float normal[3];
    };
    static_assert(sizeof(GpuVertex) == 24);
}

const char* samples::CubeApp::GetName() const { return "PhxCubeApp"; }

void samples::CubeApp::OnInit()
{
    ShaderCompiler::Initialize();

    VFS::Mount("shaders://", PHX_SHADER_SOURCE_DIR);

    auto vs_result = ShaderCompiler::Compile("shaders://Cube.slang", "VS_Main", ShaderCompiler::Stage::Vertex);
    auto fs_result = ShaderCompiler::Compile("shaders://Cube.slang", "FS_Main", ShaderCompiler::Stage::Fragment);

    if (!vs_result || !fs_result)
    {
        PHX_LOG_ERROR(Log::Channels::App, "Failed to compile Cube.slang");
        return;
    }

    m_vertex_shader   = rhi::CreateShaderModule({
            .byte_code = Span<u32>(
                reinterpret_cast<const u32*>(vs_result->Data()),
                vs_result->Size() / sizeof(u32)),
        });

    m_fragment_shader = rhi::CreateShaderModule({
            .byte_code = Span<u32>(
                reinterpret_cast<const u32*>(fs_result->Data()),
                fs_result->Size() / sizeof(u32)),
        });

    
    rhi::ShaderStageInfo stages[] = {
        { .stage = rhi::ShaderStage::VS, .module_handle = m_vertex_shader,   .entry_point = "VS_Main" },
        { .stage = rhi::ShaderStage::PS, .module_handle = m_fragment_shader, .entry_point = "FS_Main" },
    };

    rhi::Format colour_format = phx::Engine::GetColourBufferFormat();
    m_cube_pipeline = rhi::CreatePipelineState({
        .type           = rhi::PipelineType::Graphics,
        .shader_stages  = stages,
        .depth_stencil_state = {
            .depth_enable     = true,
            .depth_write_mask = rhi::DepthWriteMask::All,
            .depth_func       = rhi::ComparisonFunc::Less, // matches the depth_clear = 1.0f (far) convention used in OnRender
        },
        .raster_state = {
            .cull_mode = rhi::RasterCullMode::None,
            .front_counter_clockwise = !rhi::IsClipSpaceYDown(),
        },
        .prim_type      = rhi::PrimitiveType::TriangleList,
        .render_pass_info = {
            .color_attachments = Span<rhi::Format>(&colour_format, 1),
            .depth_stencil_format = phx::Engine::GetDepthBufferFormat(),
        },
    });

    // -- Mesh data: real GPU buffers, read via BDA pointers (see Cube.slang) ---
    // 24 unique vertices (4 per face x 6 faces), not 8 — flat per-face
    // normals mean the 8 shared cube corners can't each hold 3 different
    // face normals, so a correct hard-edged mesh needs a vertex per
    // (corner, face) pair, same as any real asset pipeline would emit.
    {
        constexpr float kCubeCorners[8][3] = {
            {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
            {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
        };
        // Per-face quad of corner indices (CCW as seen from outside) + flat
        // normal — same topology/winding Cube.slang used to hardcode.
        constexpr u32   kFaceQuads[6][4]   = { {0,1,2,3}, {5,4,7,6}, {4,0,3,7}, {1,5,6,2}, {3,2,6,7}, {4,5,1,0} };
        constexpr float kFaceNormals[6][3] = { {0,0,-1}, {0,0,1}, {-1,0,0}, {1,0,0}, {0,1,0}, {0,-1,0} };

        GpuVertex mesh_vertices[24];
        u32       mesh_indices[36];
        for (u32 face = 0; face < 6; ++face)
        {
            for (u32 j = 0; j < 4; ++j)
            {
                GpuVertex& v = mesh_vertices[face * 4 + j];
                std::memcpy(v.position, kCubeCorners[kFaceQuads[face][j]], sizeof(v.position));
                std::memcpy(v.normal,   kFaceNormals[face],                sizeof(v.normal));
            }

            const u32 base = face * 4;
            const u32 tri[6] = { base, base + 1, base + 2, base, base + 2, base + 3 };
            std::memcpy(&mesh_indices[face * 6], tri, sizeof(tri));
        }

        // sizeof(...) here must be explicitly cast to u32 — a bare size_t
        // exactly matches GpuMalloc<T>(const T&, usage)'s own template
        // (deducing T=size_t) instead of the plain size-based overload; see
        // the note on those templates in RHI.h.
        m_mesh.vertices = rhi::GpuMalloc(static_cast<u32>(sizeof(mesh_vertices)), rhi::GpuMemoryUsage::Upload);
        std::memcpy(m_mesh.vertices.cpu_ptr, mesh_vertices, sizeof(mesh_vertices));

        m_mesh.indices = rhi::GpuMalloc(static_cast<u32>(sizeof(mesh_indices)), rhi::GpuMemoryUsage::Upload);
        std::memcpy(m_mesh.indices.cpu_ptr, mesh_indices, sizeof(mesh_indices));
    }

    ToneMapBlit::Initialize();
}

void samples::CubeApp::OnBuildPreRenderFrame(phx::Jobs::Graph& graph)
{
    graph.Emplace([this] { 
        PreRender(); 
    });
}

void samples::CubeApp::OnBuildUpdateFrame(phx::Jobs::Graph& graph, float dt)
{
    graph.Emplace([this, dt] { 
        Update(dt);
    });
}

void samples::CubeApp::OnBuildRenderFrame(
    phx::Jobs::Graph& graph,
    const phx::FrameRenderTargets& targets,
    phx::rhi::CommandBuffer& out_cmd)
{
    graph.Emplace(
        [this, targets, &out_cmd] { 
            out_cmd = Render(targets); 
        });
}

void samples::CubeApp::PreRender()
{
    FrameAllocator& frame_alloc = Memory::GetFrameAlloc();

    m_render_packet = frame_alloc.Alloc<RenderPacket>();
    m_render_packet->mesh = &m_mesh;

    rhi::ViewportDesc viewport_desc;
    rhi::GetViewportDesc(viewport_desc);
    const float aspect = static_cast<float>(viewport_desc.width) / static_cast<float>(viewport_desc.height);

    const hlslpp::float1 t = m_time;
    const hlslpp::float3 eye = hlslpp::float3(hlslpp::sin(t), hlslpp::float1(0.6f), hlslpp::cos(t)) * 2.5f;
    const hlslpp::float4x4 view = hlslpp::float4x4::look_at(eye, hlslpp::float3(0.0f, 0.0f, 0.0f), hlslpp::float3(0.0f, 1.0f, 0.0f));

    const hlslpp::frustum frustum = hlslpp::frustum::field_of_view_y(hlslpp::radians(hlslpp::float1(60.0f)), aspect, 0.1f, 100.0f);
    const hlslpp::projection proj_params(frustum, hlslpp::zclip::zero, hlslpp::zdirection::forward, hlslpp::zplane::finite);
    const hlslpp::float4x4 proj = hlslpp::float4x4::perspective(proj_params);

    m_render_packet->mvp = hlslpp::mul(view, proj); // model is identity

    if (rhi::IsClipSpaceYDown())
        m_render_packet->mvp = hlslpp::mul(m_render_packet->mvp, hlslpp::float4x4::scale(1.0f, -1.0f, 1.0f));
}

void samples::CubeApp::Update(float dt)
{
    m_time += dt;
}

phx::rhi::CommandBuffer samples::CubeApp::Render(const phx::FrameRenderTargets& targets)
{
    // Field order must match Cube.slang's PushConstants exactly: the two
    // BDA pointers first (8 bytes each), matrix after.
    struct DrawData
    {
        u64 vertices;
        u64 indices;
        hlslpp::float4x4 mvp;
    } data;

    data.vertices = m_render_packet->mesh->vertices.gpu_address;
    data.indices  = m_render_packet->mesh->indices.gpu_address;
    data.mvp = m_render_packet->mvp;

    phx::rhi::CommandBuffer cmd = phx::rhi::BeginCommandRecording(phx::rhi::CommandQueueType::Graphics);

    phx::rhi::BeginRenderPass(
        targets.scene_colour,
        { .colour = { 0.0f, 0.0f, 0.0f, 1.0f }},
        targets.depth,
        { .depth_stencil = { .depth = 1.0f }},
        cmd
    );

    phx::rhi::BindPipelineState(m_cube_pipeline, cmd);
    
    phx::rhi::SetPushConstants(cmd, &data, sizeof(data));
    phx::rhi::Draw(cmd, 36);

    phx::rhi::EndRenderPass(cmd);

    ToneMapBlit::Blit(targets.scene_colour, cmd);

    return cmd;
}

void samples::CubeApp::OnShutdown()
{
    rhi::GpuFree(m_mesh.vertices);
    rhi::GpuFree(m_mesh.indices);

    ToneMapBlit::Shutdown();
    phx::ShaderCompiler::Shutdown();
}
