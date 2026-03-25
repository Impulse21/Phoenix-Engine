#include "PhxRenderer_pch.h"

#include <PhxCore/IVirtualFileSystem.h>
#include <PhxRhi/PhxRhi_Types.h>

#include <PhxRenderer/Shaders/ShaderSystem.h>
#include "SlangShaderCompiler.h"

#include <PhxCore/Pool.h>
#include <PhxCore/StringHash.h>

#include <optional>
#include "ShaderSystem.h"

using namespace phx;
using namespace phx::renderer;

namespace
{
    struct SlangShader
    {
        std::string source_path;
        Slang::ComPtr<slang::IModule> slang_module;
    };

    // -- Not thread safe
    std::unique_ptr<SlangCompilerSession> g_session;

    std::unordered_map<Hash64, ShaderHandle> g_program_cache;
    std::mutex g_program_cache_mutex;
}

void ShaderSystem::Initialize(Span<std::string> preload_list)
{
    SlangCompiler::Initialize();

    g_session = SlangCompiler::CreateCompileSession({
        .vfs = IVirtualFileSystem::Ptr,
        .target = rhi::ShaderFormat::Spirv,
        .include_paths = {},
        .defines = {},
    });
}

void ShaderSystem::Shutdown()
{
    g_session.reset();
    SlangCompiler::Shutdown();
}

void ShaderSystem::RegisterModule(const std::string& virutal_path)
{
    g_session->LoadModule(virutal_path);
}

void phx::renderer::ShaderSystem::RegisterModule(const std::string& name, const std::string& path, const void *data, size_t data_size)
{
    g_session->LoadModule(name, path, data, data_size);
}

ShaderDescriptor phx::renderer::ShaderSystem::MakePassDescriptor(const ShaderDescriptor &base_desc, Span<PassInfo> passes)
{
    ShaderDescriptor out = base_desc;

    for (const auto &pass : passes)
    {
        // Traditional Pipeline
        if (EnumHasAnyFlags(pass.active_stages, ShaderStageFlags::Vertex))
        {
            out.entry_points.push_back({std::format("vs_{}", pass.name), rhi::ShaderStage::VS});
        }

        // Mesh Pipeline
        if (EnumHasAnyFlags(pass.active_stages, ShaderStageFlags::Mesh))
        {
            out.entry_points.push_back({std::format("ms_{}", pass.name), rhi::ShaderStage::MS});
        }

        if (EnumHasAnyFlags(pass.active_stages, ShaderStageFlags::Amplification))
        {
            out.entry_points.push_back({std::format("as_{}", pass.name), rhi::ShaderStage::AS});
        }

        if (EnumHasAnyFlags(pass.active_stages, ShaderStageFlags::Pixel))
        {
            out.entry_points.push_back({std::format("ps_{}", pass.name), rhi::ShaderStage::PS});
        }

        // Compute Pipeline
        if (EnumHasAnyFlags(pass.active_stages, ShaderStageFlags::Compute))
        {
            out.entry_points.push_back({std::format("cs_{}", pass.name), rhi::ShaderStage::CS});
        }
    }

    return out;
}

ShaderHandle phx::renderer::ShaderSystem::GetOrRequestProgram(const ShaderDescriptor &shader_desc)
{
    // Have we compiled already?
    Hash64 hash = shader_desc.GetHash();
    
    // TODO: Determine thread safety here.
    {
        // std::scoped_lock _(g_program_cache_mutex);
        auto itr = g_program_cache.find(hash);
        if (itr != g_program_cache.end())
            return itr->second;
    }

    // We need to compile
    g_session->LinkProgram(shader_desc);
    
    return ShaderHandle();
}