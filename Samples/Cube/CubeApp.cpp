#include "CubeApp.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/PhxDefines.h>

#include <PhxEngine/Memory/TlsfHeapAllocator.h>
#include <PhxEngine/Memory/MemoryHelpers.h>

#include <PhxEngine/RHI/GpuMemory/StandardSamplers.h>

#include <PhxEngine/Renderer/ToneMapBlit.h>

#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/VFS/VFS.h>

#include <PhxEngine/Platform/EntryPoint.h>
#include <PhxEngine/Engine.h>

#include "Shaders/Cube_interop.h"
#include <cstring>
#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace samples;
using namespace phx;

PHX_DEFINE_APP(CubeApp);

namespace
{
}

const char* samples::CubeApp::GetName() const { return "PhxCubeApp"; }

void samples::CubeApp::OnInit()
{
    // -- Set up mount mounts ---
    VFS::Mount("shaders://", PHX_SHADER_SOURCE_DIR);
    VFS::Mount("assets://", PHX_ASSET_SOURCE_DIR);

    m_renderer.Initialize();
    

    // -- Create RHI Resources ---
    rhi::GpuBumpAllocator& buffer_allocator = m_renderer.GetBufferAllocator();

    m_mesh.vertices = buffer_allocator.Alloc<Vertex>(cube_vertex_count);
    std::memcpy(m_mesh.vertices.cpu, cube_vertices, sizeof(cube_vertices));

    m_mesh.indices = buffer_allocator.Alloc<u32>(cube_index_count);
    std::memcpy(m_mesh.indices.cpu, cube_indices, sizeof(cube_indices));

    // Load and write textures
    {
        MemoryBuffer image_memory = VFS::ReadFile("assets://phx_image.png");
        TypedView<stbi_uc> data_view = image_memory.GetView<stbi_uc>(); 

        int width, height, channels;
        unsigned char *data = stbi_load_from_memory(
            data_view.Get(),
            image_memory.Size(), 
            &width,
            &height,
            &channels,
            STBI_rgb);

        if (data == NULL) 
        {
            PHX_LOG_ERROR(Log::Channels::App, "Failed to load PNG file: %s", stbi_failure_reason());
        }
        else
        {
            // Write the data directly to the CPU visiable memory
            size_t byte_count = (size_t)width * (size_t)height * 3;
            rhi::GpuCpuRange<byte> upload_alloc = buffer_allocator.Alloc(byte_count);

            // Copy to CPU memory. Would be nice to read directly to the GPU memory
            std::memcpy(upload_alloc.cpu, data, byte_count);            
        }

        stbi_image_free(data);

        // TODO: 
        // Allocate texture
        // Allocate descirptor
        // Issue a copy command
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
    const phx::renderer::FrameRenderTargets& targets,
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

    const DrawData draw_data = {
        .vertices = m_render_packet->mesh->vertices.gpu,
        .mvp = m_render_packet->mvp,
    };

    phx::rhi::CommandBuffer cmd = phx::rhi::BeginCommandRecording(phx::rhi::CommandQueueType::Graphics);

    phx::rhi::BeginRenderPass(
        targets.scene_colour,
        { .colour = { 0.0f, 0.0f, 0.0f, 1.0f }},
        targets.depth,
        { .depth_stencil = { .depth = 1.0f }},
        cmd
    );

    phx::rhi::BindPipelineState(m_cube_pipeline, cmd);
    
    phx::rhi::DrawIndex(
        cmd,
        draw_data,
        m_render_packet->mesh->indices.ToGpuRange(),
        rhi::IndexFormat::Uint32,
        cube_index_count);

    phx::rhi::EndRenderPass(cmd);

    ToneMapBlit::Blit(targets.scene_colour, cmd);

    return cmd;
}

void samples::CubeApp::OnShutdown()
{
    m_renderer.Shutdown();
    ToneMapBlit::Shutdown();
}
