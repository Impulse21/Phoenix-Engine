#pragma once

#include <PhxCore/IO/MemoryRegion.h>
#include <PhxCore/EnumUtils.h>

#include <PhxRhi/PhxRhi.h>

#include <PhxResource/Resource.h>
#include <PhxResource/ResourceTypes.h>
#include <PhxResource/ResourceTypeTraits.h>

#include <PhxRenderer/TextureResource.h>
#include <PhxAsset/AssetPtr.h>

#include <PhxRenderer/MaterialArchetype.def.h>
#include <PhxRenderer/Shaders/ShaderModuleResource.h>

namespace phx::renderer
{
    constexpr const char* kMaterialDataStructName = "MaterialData";
    constexpr size_t kMaterialDataStructSize = 256;

    struct MaterialArchetypeResource final : public Resource
    {
        phx::asset::Ptr<asset::MaterialArchetypeDef> archetype_def;
        RefCountPtr<renderer::ShaderModuleResource> shader_module;
        phx::MemoryBuffer default_instance_data;

        std::vector<RefCountPtr<TextureResource>> textures;

        // TOOD: Remove
        std::unordered_map<phx::StringHash, size_t> texture_lut;
        
        void Dispose() override 
        {
            // no-op
        };

        bool CollectPendingGpuTransitions(SpanMutable<rhi::GpuBarrier>, size_t&) override 
        { 
            return true; 
        }

        PHX_DECLARE_RESOURCE(MaterialArchetypeResource)
    };
}

PHX_DEFINE_RESOURCE(
    phx::renderer::MaterialArchetypeResource,
    ".phxarc",                          // Extension
    "MaterialArcLoader"                 // Loader ID
);