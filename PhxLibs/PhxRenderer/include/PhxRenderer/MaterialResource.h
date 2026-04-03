#pragma once

#include <PhxCore/EnumUtils.h>

#include <PhxRhi/PhxRhi.h>

#include <PhxResource/Resource.h>
#include <PhxResource/ResourceTypes.h>
#include <PhxResource/ResourceTypeTraits.h>

#include <PhxRenderer/TextureResource.h>
#include <PhxRenderer/Shaders/ShaderModuleResource.h>
#include <PhxRenderer/MaterialArchetype.def.h>
#include <PhxRenderer/MaterialArchetypeResource.h>

#include <hlsl++.h>

namespace phx::renderer
{
    struct MaterialResource final : public Resource
    {
        phx::asset::Ptr<asset::MaterialInstanceDef> instance_def;

        RefCountPtr<MaterialArchetypeResource> archetype;

        std::vector<RefCountPtr<TextureResource>> textures;
        
        phx::MemoryBuffer cpu_data_buffer;

        uint32_t global_material_index = ~0; // index in the global material buffer array in the renderer

        void Dispose() override {};
        bool CollectPendingGpuTransitions(SpanMutable<rhi::GpuBarrier>, size_t&) override 
        { 
            return true; 
        }

        PHX_DECLARE_RESOURCE(MaterialResource)
	};
}

PHX_DEFINE_RESOURCE(
    phx::renderer::MaterialResource,
    ".phxmat",                      // Extension
    "MaterialLooader"               // Loader ID
);