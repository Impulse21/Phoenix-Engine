#pragma once

#include <PhxEngine/Core/MemoryBuffer.h>
#include <PhxEngine/Resources/MaterialResource.h>
#include <PhxEngine/Resources/Intermediate/IntermediateMaterials.h>

#include <string>

namespace phx::resources
{
    struct IntermediateModel;

    struct CompiledMaterial
    {
        renderer::MaterialData data;
        std::string archetype;
        MaterialDomain domain = MaterialDomain::Opaque;
        bool double_sided = false;

        std::string texture_paths[Slot_Count];
    };

    CompiledMaterial CompileMaterial(const IntermediateMaterial& material, const IntermediateModel& model, const std::string& source_path);
    MemoryBuffer SerializeMaterial(const CompiledMaterial& material, const std::string& source_path, const std::string& material_name);
    bool WriteMaterialFile(const char* virtual_path, const MemoryBuffer& file_bytes);
}
