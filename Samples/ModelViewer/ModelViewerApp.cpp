#include "ModelViewerApp.h"

#include <PhxEngine/Core/Profile.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/PathUtils.h>
#include <PhxEngine/Core/CVar.h>

#include <PhxEngine/VFS/VFS.h>

#include <PhxEngine/Memory/TlsfHeapAllocator.h>
#include <PhxEngine/Memory/MemoryHelpers.h>

#include <PhxEngine/Renderer/ShaderCompiler.h>
#include <PhxEngine/Renderer/ToneMapBlit.h>

#include <PhxEngine/RHI/RHI.h>

#include <PhxEngine/Platform/EntryPoint.h>

// -- Resource headers ---
#include <PhxEngine/Resources/AssetImporters/GltfImporter.h>
#include <PhxEngine/Resources/MeshOptimizer.h>
#include <PhxEngine/Resources/Compiler/MeshCompiler.h>
#include <PhxEngine/Resources/Compiler/TextureCompiler.h>
#include <PhxEngine/Resources/Compiler/MaterialCompiler.h>
#include <PhxEngine/Resources/CookedPathBuilder.h>
#include <PhxEngine/Resources/ResourceCache.h>
#include <PhxEngine/Resources/MeshFileFormat.h>
#include <PhxEngine/Resources/TextureFileFormat.h>
#include <PhxEngine/Resources/MaterialFileFormat.h>

#include <PhxEngine/Engine.h>

#include <cstring>
#include <utility>

using namespace samples;
using namespace phx;

PHX_DEFINE_APP(ModelViewerApp);

PHX_CVAR_STRING(gltf_file, "assets://Box.glb", "Sets the GLTF file to load");

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

const char* samples::ModelViewerApp::GetName() const { return "PhxModelViewerApp"; }

void samples::ModelViewerApp::OnInit()
{
    PHX_PROFILE_SCOPE();
    ShaderCompiler::Initialize();

    VFS::Mount("shaders://", PHX_SHADER_SOURCE_DIR);
    VFS::Mount("assets://", PHX_ASSET_SOURCE_DIR);
    VFS::Mount("resources://", JoinPaths(PHX_ASSET_SOURCE_DIR, ".compiled").c_str());

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


    // TODO: Expose usage of executor.async here so I can send off one offs to the thread pool
    // instread of always requiring a graph.

    // TODO: Consider making this a pipeline that can be executed:
    const char* gltf_file = CVar_gltf_file.Get();
    PHX_LOG_INFO(Log::Channels::App, "Loading Gltf File '{0}'", gltf_file);

    Result<resources::IntermediateModel> gltf_model = resources::ImportGltfModel(gltf_file);

    if (gltf_model.HasError())
    {
        PHX_LOG_ERROR(Log::Channels::App, "Failed to import GLTF asset '{0}'", gltf_file);
    }
    else
    {
        MemoryBuffer source_bytes = VFS::ReadFile(gltf_file);
        const u64 source_hash = source_bytes.IsEmpty() ? 0 : resources::HashBytes(source_bytes.Data(), source_bytes.Size());

        for (const resources::IntermediateTexture& tex : gltf_model->textures)
        {
            if (!tex.IsValid())
                continue;

            const std::string cooked_path = resources::CookedTexturePath(gltf_file, tex.name);
            if (resources::IsCookedFileUpToDate(cooked_path.c_str(), resources::kTextureFileMagic, resources::kTextureFileVersion, source_hash))
                continue;

            resources::CompiledTexture compiled_tex = resources::CompileTexture(tex);
            MemoryBuffer tex_blob = resources::SerializeTexture(compiled_tex, gltf_file, tex.name);
            resources::WriteTextureFile(cooked_path.c_str(), tex_blob);
        }

        for (const resources::IntermediateMaterial& mat : gltf_model->materials)
        {
            const std::string cooked_path = resources::CookedMaterialPath(gltf_file, mat.name);
            if (resources::IsCookedFileUpToDate(cooked_path.c_str(), resources::kMaterialFileMagic, resources::kMaterialFileVersion, source_hash))
                continue;

            resources::CompiledMaterial compiled_mat = resources::CompileMaterial(mat, *gltf_model, gltf_file);
            MemoryBuffer mat_blob = resources::SerializeMaterial(compiled_mat, gltf_file, mat.name);
            resources::WriteMaterialFile(cooked_path.c_str(), mat_blob);
        }

        for (auto& mesh : gltf_model->meshes)
        {
            const std::string mesh_path = resources::CookedMeshPath(gltf_file, mesh.name);
            if (!resources::IsCookedFileUpToDate(mesh_path.c_str(), resources::kMeshFileMagic, resources::kMeshFileVersion, source_hash))
            {
                resources::OptimizeMesh(mesh);
                resources::CompiledMesh compiled_mesh = resources::CompileMesh(mesh);
                MemoryBuffer mesh_blob = resources::SerializeMesh(compiled_mesh, gltf_file, mesh.name);
                resources::WriteMeshFile(mesh_path.c_str(), mesh_blob);
            }

            LoadedMesh loaded;
            loaded.mesh = resources::LoadMeshResource(mesh_path.c_str());
            if (!loaded.mesh)
            {
                PHX_LOG_ERROR(Log::Channels::App, "Failed to load cooked mesh '{0}'", mesh_path);
                continue;
            }

            loaded.materials.resize(loaded.mesh->cpu_data->draw_info_count);
            for (u32 p = 0; p < loaded.mesh->cpu_data->draw_info_count; ++p)
            {
                const auto& draw_info = loaded.mesh->cpu_data->draw_info.Get()[p];
                loaded.materials[p] = resources::LoadMaterialForDrawInfo(*loaded.mesh, draw_info);
            }

            m_loaded_meshes.push_back(std::move(loaded));
        }
    }

    ToneMapBlit::Initialize();
}

void samples::ModelViewerApp::OnBuildPreRenderFrame(phx::Jobs::Graph& graph)
{
    graph.Emplace([this] { 
        PreRender(); 
    });
}

void samples::ModelViewerApp::OnBuildUpdateFrame(phx::Jobs::Graph& graph, float dt)
{
    graph.Emplace([this, dt] { 
        Update(dt);
    });
}

void samples::ModelViewerApp::OnBuildRenderFrame(
    phx::Jobs::Graph& graph,
    const phx::FrameRenderTargets& targets,
    phx::rhi::CommandBuffer& out_cmd)
{
    graph.Emplace(
        [this, targets, &out_cmd] { 
            out_cmd = Render(targets); 
        });
}

void samples::ModelViewerApp::PreRender()
{
    PHX_PROFILE_SCOPE();
    FrameAllocator& frame_alloc = Memory::GetFrameAlloc();

    m_render_packet = frame_alloc.Alloc<RenderPacket>();

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

void samples::ModelViewerApp::Update(float dt)
{
    PHX_PROFILE_SCOPE()
    m_time += dt;
}

phx::rhi::CommandBuffer samples::ModelViewerApp::Render(const phx::FrameRenderTargets& targets)
{
    PHX_PROFILE_SCOPE();
    // Field order must match Cube.slang's PushConstants exactly: the two
    // BDA pointers first (8 bytes each), matrix after.
    struct DrawData
    {
        u64 vertices;
        u64 indices;
        hlslpp::float4x4 mvp;
    } data;

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

void samples::ModelViewerApp::OnShutdown()
{
    m_loaded_meshes.clear();

    rhi::DestroyPipelineState(m_cube_pipeline);
    rhi::DestroyShaderModule(m_vertex_shader);
    rhi::DestroyShaderModule(m_fragment_shader);

    ToneMapBlit::Shutdown();
    phx::ShaderCompiler::Shutdown();
}
