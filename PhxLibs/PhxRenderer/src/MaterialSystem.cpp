#include "PhxRenderer_pch.h"

#include <PhxRenderer/MaterialSystem.h>

#include <PhxCore/IVirtualFileSystem.h>
#include <PhxCore/Pool.h>
#include <PhxCore/IO/MemoryRegion.h>

#include <PhxAsset/AssetDatabase.h>
#include <PhxRenderer/Shaders/ShaderSystem.h>

#include <PhxRenderer/TextureResource.h>
#include <PhxResource/ResourceManager.h>

// -- TODO: Clean API Leak at some point ---
#include <slang.h>

using namespace phx;
using namespace phx::renderer;

namespace
{
    constexpr const char* kMaterialDataStructName = "MaterialData";

    struct MtlArchetypeSlang
    {
        std::string virtual_path;
        phx::MemoryBuffer default_instance_data;
        struct TextureBinding
        {
            ShaderFieldDesc field_desc;
            RefCountPtr<TextureResource> default_texture;
        };

        std::vector<TextureBinding> default_textures;
    };

    struct MtlInstanceImpl
    {
        
        MtlArchetypeHandle arch_handle;
    };

    phx::IVirtualFileSystem* g_vfs;
    phx::PagedPool<MaterialArchetype, MtlArchetypeSlang> g_archetype_pool;
    phx::PagedPool<MaterialInstance, MtlInstanceImpl> g_instance_pool;
    std::unordered_map<std::string, MtlArchetypeHandle> g_archetype_lut;
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
    auto itr = g_archetype_lut.find(virtual_path);
    if (itr != g_archetype_lut.end())
        return itr->second;

    const bool registered = ShaderSystem::RegisterModule(def.shader_desc.source);
    if (!registered)
    {
        PHX_CORE_ERROR("Failed to register shader module for material archetype '{0}'", virtual_path.c_str());
        return MtlArchetypeHandle::CreateInvalid();
    }

    ShaderStructDesc material_struct_desc;

    if (!ShaderSystem::FindStruct(def.shader_desc.source, kMaterialDataStructName, material_struct_desc))
    {
        PHX_CORE_ERROR(
            "Failed to find '{0}' struct in shader module for material archetype '{1}'",
            kMaterialDataStructName,
            virtual_path.c_str());
        return MtlArchetypeHandle::CreateInvalid();
    }
    
    PHX_ASSERT(material_struct_desc.size == 256, "MaterialData struct must be 256 bytes in size");

    MtlArchetypeHandle archetype_handle = g_archetype_pool.Allocate();
    PHX_CORE_ASSERT(archetype_handle.IsValid(), "Failed to acquire material archetype handle.");

    MtlArchetypeSlang& archetype = *g_archetype_pool.GetHot(archetype_handle);
        struct ShaderDescriptor
    {
        std::string virtual_path;

        struct GenericArg
        {
            std::string name;
            std::string value;

            bool is_type = false;
        };

        std::vector<GenericArg> generic_args;
        std::vector<ShaderEntryPoint> entry_points;

        Hash64 GetHash() const;
    };


    archetype.virtual_path = def.shader_desc.source;
    archetype.default_instance_data = phx::MemoryBuffer(material_struct_desc.size);
    TypedView<uint8_t> instance_data_view = archetype.default_instance_data.GetView<uint8_t>();

    for (auto& param : def.params)
    {
        for (auto& field : material_struct_desc.fields)
        {
            if (field.name != phx::StringHash(param.name))
                continue;

            std::visit([&](auto &&arg)
            {
                using T = std::decay_t<decltype(arg)>;
                
                if constexpr (std::is_same_v<T, rfl::Field<"texture", std::string>>) 
                {
                    RefCountPtr<TextureResource> texture = ResourceManager::Load<TextureResource>(arg.value().c_str());
                    archetype.default_textures.push_back({ field, texture });
                }
                else 
                {
                    PHX_CORE_ASSERT(field.size == sizeof(T::Type), "Field size mismatch for float4");
                    uint8_t* entry = instance_data_view.Get() + field.offset;
                    std::memcpy(entry, &arg.value(), field.size);
                }
            }, param.value);
        }
    }

    g_archetype_lut[virtual_path] = archetype_handle;
    return archetype_handle;
}

MtlInstanceHandle phx::renderer::MaterialSystem::CreateInstance(const asset::MaterialInstanceDef &def)
{
    // TODO: I am here.
    return MtlInstanceHandle();
}

MtlInstanceHandle phx::renderer::MaterialSystem::CreateInstance(MtlArchetypeHandle hande, const asset::MaterialInstanceDef &def)
{
    return MtlInstanceHandle();
}
