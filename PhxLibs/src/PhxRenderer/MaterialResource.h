#pragma once

#include <PhxRhi/PhxRhi.h>

#include <PhxResource/Resource.h>
#include <PhxResource/ResourceTypes.h>
#include <PhxResource/ResourceTypeTraits.h>

#include <PhxRenderer/TextureResource.h>
#include <PhxRenderer/ShaderLIbrary.h>
#include <hlsl++.h>

namespace phx::renderer
{
    enum class MaterialPropertyType : uint8_t
    {
        Float,
        Float2,
        Float3,
        Float4,
        Int,
        Bool,
        Texture
    };

    struct ManifestMaterialValue
    {
        MaterialPropertyType type;
        union
        {
            float                   float_val;
            hlslpp::interop::float2 float2_val;
            hlslpp::interop::float3 float3_val;
            hlslpp::interop::float4 float4_val;
            int32_t                 int_val;
            bool                    bool_val;
        };

        std::string texture_path;

        ManifestMaterialValue(float v) : type(MaterialPropertyType::Float), float_val(v) {}
        ManifestMaterialValue(hlslpp::interop::float2 v) : type(MaterialPropertyType::Float2), float2_val(v) {}
        ManifestMaterialValue(hlslpp::interop::float3 v) : type(MaterialPropertyType::Float3), float3_val(v) {}
        ManifestMaterialValue(hlslpp::interop::float4 v) : type(MaterialPropertyType::Float4), float4_val(v) {}
        ManifestMaterialValue(const std::string& s) : type(MaterialPropertyType::Texture), texture_path(s) {}
        ManifestMaterialValue() : type(MaterialPropertyType::Float), float_val(0) {}
    };

    struct MaterialManifest
    {
        std::string archetype_name;
        std::unordered_map<std::string, ManifestMaterialValue> properties;
    };

    struct MaterialValue
    {
        MaterialPropertyType type;
        union
        {
            float                   float_val;
            hlslpp::interop::float2 float2_val;
            hlslpp::interop::float3 float3_val;
            hlslpp::interop::float4 float4_val;
            int32_t                 int_val;
            bool                    bool_val;
        };

        RefCountPtr<phx::renderer::TextureResource> texture;

        // Constructors for easy assignment
        MaterialValue(float v) : type(MaterialPropertyType::Float), float_val(v) {}
        MaterialValue(hlslpp::interop::float2 v) : type(MaterialPropertyType::Float2), float2_val(v) {}
        MaterialValue(hlslpp::interop::float3 v) : type(MaterialPropertyType::Float3), float3_val(v) {}
        MaterialValue(hlslpp::interop::float4 v) : type(MaterialPropertyType::Float4), float4_val(v) {}
        MaterialValue(RefCountPtr<phx::renderer::TextureResource> t) : type(MaterialPropertyType::Texture), texture(t) {}
        MaterialValue() : type(MaterialPropertyType::Float), float_val(0) {}
    };

    struct MaterialVariable
    {
        std::string name;
        MaterialValue value;
    };

    struct ArchetypeRenderState
    {
        rhi::RasterCullMode cull_mode = rhi::RasterCullMode::Back;
        rhi::FrontFace front_face = rhi::FrontFace::CounterClockwise;
        bool depth_test = true;
        bool depth_write = true;
        rhi::ComparisonFunc depth_compare = rhi::ComparisonFunc::LessOrEqual;

        // "Static" states that still require unique PSOs (e.g., Blend Mode)
        // stored here for the Compiler to read.
        struct BlendState
        {
            rhi::BlendFactor src = rhi::BlendFactor::One;
            rhi::BlendFactor dst = rhi::BlendFactor::One;
            rhi::EBlendOp op = rhi::EBlendOp::Add;
        } blend;
    };

    struct MaterialArchetypeResource final : public Resource
    {
        phx::MemoryBuffer default_instance_buffer;
        RefCountPtr<renderer::ShaderAsset> shader_asset;
        std::unordered_map<std::string, ShaderEntryPoint> techniques;
        std::unordered_map<std::string, rhi::PipelineStateHandle> pso_cache;
		ArchetypeRenderState render_state;

        void Dispose() override 
        {
            for (auto& [key, pso] : pso_cache)
                rhi::DeletePipeline(pso);
        };

        bool CollectPendingGpuTransitions(SpanMutable<rhi::GpuBarrier>, size_t&) override 
        { 
            return true; 
        }

        PHX_DECLARE_RESOURCE(MaterialArchetypeResource)
    };

    struct MaterialResource final : public Resource
    {
        RefCountPtr<MaterialArchetypeResource> archetype;
        std::vector<MaterialVariable> variables;
        uint32_t shadow_data_index = ~0u;

        void Dispose() override {};
        bool CollectPendingGpuTransitions(SpanMutable<rhi::GpuBarrier>, size_t&) override 
        { 
            return true; 
        }

        PHX_DECLARE_RESOURCE(MaterialResource)
	};

}

PHX_DEFINE_RESOURCE(
    phx::renderer::MaterialArchetypeResource,
    ".phxarc",                          // Extension
    "MaterialArcLoader"                 // Loader ID
);

PHX_DEFINE_RESOURCE(
    phx::renderer::MaterialResource,
    ".phxmat",                      // Extension
    "MaterialLooader"               // Loader ID
);