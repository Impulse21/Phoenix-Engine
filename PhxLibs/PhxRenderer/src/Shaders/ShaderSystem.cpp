#include "PhxRenderer_pch.h"

#include <PhxCore/IVirtualFileSystem.h>
#include <PhxRhi/PhxRhi_Types.h>

#include <PhxRenderer/Shaders/ShaderSystem.h>
#include "SlangShaderCompiler.h"

#include <PhxCore/Pool.h>
#include <PhxCore/StringHash.h>
#include "ShaderSystem.h"


using namespace phx;
using namespace phx::renderer;

namespace
{
    struct SlangShaderModule
    {
        std::string source_path;
        Slang::ComPtr<slang::IModule> slang_module;
        std::unordered_map<std::string, ShaderStructDesc> type_desc_cache;
    };

    struct SlangShader
    {
        std::string source_path;
        Slang::ComPtr<slang::IModule> slang_module;
    };

    // -- Not thread safe
    std::unique_ptr<SlangCompilerSession> g_session;
    
    std::unordered_map<std::string, SlangShaderModule> g_module_cache; 
    std::unordered_map<Hash64, ShaderHandle> g_program_cache;
    std::mutex g_program_cache_mutex;

    template <typename Func>
    auto RegisterShaderModule(std::string virtual_path, Func &&fn)
    {
        auto itr = g_module_cache.find(virtual_path);
        if (itr != g_module_cache.end())
            return true;

        auto slang_module = fn();

        if (!slang_module)
            return false;

        SlangShaderModule shader_module = {
            .source_path = virtual_path,
            .slang_module = slang_module,
        };

        g_module_cache[virtual_path] = shader_module;

        return true;
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
    g_module_cache.clear();
    g_session.reset();
    SlangCompiler::Shutdown();
}

bool ShaderSystem::RegisterModule(const std::string& virutal_path)
{
  return RegisterShaderModule(virutal_path, [virutal_path](){ return g_session->LoadModule(virutal_path); });
}

bool phx::renderer::ShaderSystem::RegisterModule(const std::string& name, const std::string& virtual_path, const void *data, size_t data_size)
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

bool phx::renderer::ShaderSystem::FindStruct(const std::string &virtual_path, std::string_view struct_name, ShaderStructDesc &out_desc)
{
    auto itr = g_module_cache.find(virtual_path);
    if (itr == g_module_cache.end())
        return false;

    auto emplace_result =itr->second.type_desc_cache.try_emplace(struct_name, [&]()
    {
        slang::ProgramLayout* program_layout = itr->second.slang_module->getLayout();
        PHX_ASSERT(program_layout);

        slang::TypeReflection* type_reflection = program_layout->findTypeByName(struct_name);
        if (!type_reflection)
            return false;

        // TODO: Cache this

        slang::TypeLayoutReflection* type_layout = program_layout->getTypeLayout(type_reflection, slang::LayoutRules::Default);

        ShaderStructDesc desc = {
            .name = StringHash(struct_name),
            .size = (uint32_t)type_layout->getSize(),
        };
        
        desc.fields.resize(type_reflection->getFieldCount());
        for (uint32_t i = 0; i < type_reflection->getFieldCount(); ++i)
        {
            auto field_reflection = type_reflection->getFieldByIndex(i);
            auto field_layout = type_layout->getFieldByIndex(i);

            desc.fields[i] = {
                .name = StringHash(field_reflection->getName()),
                .offset = static_cast<uint32_t>(field_layout->getOffset()),
                .size = static_cast<uint32_t>(field_layout->getSize()),
            };
        }
    });
    
    out_desc = emplace_result.first->second;
    return emplace_result.second;
}
