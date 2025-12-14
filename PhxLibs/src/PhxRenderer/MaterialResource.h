#pragma once

#include <PhxRhi/PhxRhi.h>

#include <PhxResource/ResourceFwds.h>
#include <PhxResource/ResourceTypes.h>
#include <PhxResource/ResourceTypeTraits.h>

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

        TextureResourcePtr texture;

        // Constructors for easy assignment
        MaterialValue(float v) : type(MaterialPropertyType::Float), float_val(v) {}
        MaterialValue(hlslpp::interop::float2 v) : type(MaterialPropertyType::Float2), float2_val(v) {}
        MaterialValue(hlslpp::interop::float3 v) : type(MaterialPropertyType::Float3), float3_val(v) {}
        MaterialValue(hlslpp::interop::float4 v) : type(MaterialPropertyType::Float4), float4_val(v) {}
        MaterialValue(TextureResourcePtr t) : type(MaterialPropertyType::Texture), texture(t) {}
        MaterialValue() : type(MaterialPropertyType::Float), float_val(0) {}
    };

    struct MaterialVariable
    {
        std::string name;
        MaterialValue value;
    };

    struct MaterialArchetype;

    struct MaterialResource final : public ResourceHotData
    {
		ResourcePtr<MaterialArchetype> archetype;
        std::vector<MaterialVariable> variables;
		uint32_t shadow_data_index = ~0u;
	};

    struct MaterialColdData final : public ResourceColdData
    {

    };

}

PHX_DEFINE_RESOURCE(
    renderer::MaterialResource,     // T
    renderer::MaterialResource,      // Hot
    renderer::MaterialColdData,     // Cold
    ".phxmat",                      // Extension
    "MaterialLooader"               // Loader ID
);