#include "PhxRenderer_pch.h"

#include <PhxCore/IVirtualFileSystem.h>
#include <PhxRhi/PhxRhi_Types.h>

#include <PhxRenderer/Shaders/ShaderSystem.h>
#include "SlangShaderCompiler.h"

#include <PhxCore/Pool.h>
#include <PhxCore/StringHash.h>


using namespace phx;
using namespace phx::renderer;

namespace
{
    struct SlangShaderModule
    {
        std::string source_path;
        Slang::ComPtr<slang::IModule> slang_module;
    };

    struct SlangShader
    {
        std::string source_path;
        Slang::ComPtr<slang::IModule> slang_module;
    };

    // -- Not thread safe
    std::unique_ptr<SlangCompilerSession> g_session;
    phx::SmallObjectPool<ShaderModule, SlangShaderModule, 16> g_shader_module_pool;
    
    std::unordered_map<std::string, ShaderModuleHandle> g_module_lut; 
    std::unordered_map<Hash64, ShaderHandle> g_program_cache;
    std::mutex g_program_cache_mutex;

    template <typename Func>
    auto RegisterShaderModule(std::string virtual_path, Func &&fn)
    {
        ShaderModuleHandle handle = ShaderModuleHandle::CreateInvalid();

        auto itr = g_module_lut.find(virutal_path);
        if (itr != g_module_lut.end())
        {
            handle = itr->second;
            if (g_shader_module_pool.Contains(handle))
                return handle;
        }

        auto slang_module = fn();

        if (!slang_module)
            return ShaderModuleHandle::CreateInvalid();

        ShaderModuleHandle handle = g_shader_module_pool.Allocate();
        if (!handle.IsValid())
            return handle;

        SlangShaderModule *shader_module = g_shader_module_pool.Get(handle);
        shader_module->slang_module = slang_module;
        shader_module->source_path = virutal_path;

        g_module_lut[virutal_path] = handle;

        return handle;
    }
}

void ShaderSystem::Initialize(Span<std::string> preload_list, uint32_t max_shader_modules)
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
    g_shader_module_pool.Shutdown();
    g_session.reset();
    SlangCompiler::Shutdown();
}

ShaderModuleHandle ShaderSystem::RegisterModule(const std::string& virutal_path)
{
  return RegisterShaderModule(virutal_path, [virutal_path](){ return g_session->LoadModule(virutal_path); });
}

ShaderModuleHandle phx::renderer::ShaderSystem::RegisterModule(const std::string& name, const std::string& virtual_path, const void *data, size_t data_size)
{
  return RegisterShaderModule(virtual_path, [&](){ return g_session->LoadModule(name, virtual_path, data, data_size); });
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