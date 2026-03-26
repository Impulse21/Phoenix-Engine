#pragma once

#include <string>
#include <PhxCore/Handle.h>
#include <PhxCore/Span.h>

#include <PhxRenderer/MaterialArchetype.def.h>

namespace phx
{
    class IVirtualFileSystem;
}
namespace phx::renderer
{
    struct MaterialArchetype;
    using MtlArchetypeHandle = Handle<MaterialArchetype>;

    struct MaterialInstance;
    using MtlInstanceHandle = Handle<MaterialInstance>;

    namespace MaterialSystem
    {
        void Initialize(IVirtualFileSystem* vfs, uint32_t max_archetypes, uint32_t max_instances);
        void Shutdonw();

        void RegisterArchetypes(Span<std::string> virtual_paths);

        MtlArchetypeHandle CreateArchetype(std::string virtual_path, const phx::renderer::asset::MaterialArchetypeDef& def);

        MtlInstanceHandle CreateInstance(const phx::renderer::asset::MaterialInstanceDef& def);
        MtlInstanceHandle CreateInstance(MtlArchetypeHandle hande, const phx::renderer::asset::MaterialInstanceDef& def);
        
    }
}