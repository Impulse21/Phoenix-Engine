#include "PhxRenderer_pch.h"

#include <PhxRenderer/MaterialSystem.h>

#include <PhxCore/IVirtualFileSystem.h>
#include <PhxCore/Pool.h>
#include <PhxCore/IO/MemoryRegion.h>

#include <PhxAsset/AssetDatabase.h>
#include <PhxRenderer/Shaders/ShaderSystem.h>

// -- TODO: Clean API Leak at some point ---
#include <slang.h>

using namespace phx;
using namespace phx::renderer;

namespace
{
    struct MtlArchetypeSlang
    {
        phx::renderer::ShaderDescriptor base_shader_desc;
        bool is_double_sided;
        phx::MemoryBuffer default_instance_data;
    };

    struct MtlInstanceImpl
    {
        
        MtlArchetypeHandle arch_handle;
    };

    phx::IVirtualFileSystem* g_vfs;
    phx::PagedPool<MaterialArchetype, MtlArchetypeSlang> g_archetype_pool;
    phx::PagedPool<MaterialInstance, MtlInstanceImpl> g_instance_pool;
    std::unordered_map<MtlArchetypeHandle, std::string> g_archetype_lut;
}

void phx::renderer::MaterialSystem::Initialize(IVirtualFileSystem *vfs, uint32_t max_archetypes, uint32_t max_instances)
{
    g_vfs = vfs;
    g_archetype_pool.Initialize(max_archetypes);
    g_instance_pool.Initialize(max_instances);
}

void phx::renderer::MaterialSystem::Shutdonw()
{
}

void phx::renderer::MaterialSystem::RegisterArchetypes(Span<std::string> virtual_paths)
{
    for (auto& virtual_path : virtual_paths)
    {
        
        phx::asset::Ptr<asset::MaterialArchetypeDef> def = phx::asset::AssetDB::Get<asset::MaterialArchetypeDef>(virtual_path);
        if (def)
            CreateArchetype(virtual_path, *def);
    }
}

MtlArchetypeHandle phx::renderer::MaterialSystem::CreateArchetype(std::string virtual_path, const asset::MaterialArchetypeDef& def)
{
    const bool registered = ShaderSystem::RegisterModule(def.shader_desc.source);
    if (!registered)
    {
        PHX_LOG_ERROR("Failed to register shader module for material archetype '{}'", virtual_path);
        return MtlArchetypeHandle::CreateInvalid();
    }

    return MtlArchetypeHandle();
}

MtlInstanceHandle phx::renderer::MaterialSystem::CreateInstance(const asset::MaterialInstanceDef &def)
{
    return MtlInstanceHandle();
}

MtlInstanceHandle phx::renderer::MaterialSystem::CreateInstance(MtlArchetypeHandle hande, const asset::MaterialInstanceDef &def)
{
    return MtlInstanceHandle();
}
