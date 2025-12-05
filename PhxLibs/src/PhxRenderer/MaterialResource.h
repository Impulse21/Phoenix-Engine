#pragma once

#include <PhxResource/Resource.h>
#include <PhxRhi/PhxRhi.h>

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

        RefCountPtr<Resource> texture;

        // Constructors for easy assignment
        MaterialValue(float v) : type(MaterialPropertyType::Float), float_val(v) {}
        MaterialValue(hlslpp::interop::float2 v) : type(MaterialPropertyType::Float2), float2_val(v) {}
        MaterialValue(hlslpp::interop::float3 v) : type(MaterialPropertyType::Float3), float3_val(v) {}
        MaterialValue(hlslpp::interop::float4 v) : type(MaterialPropertyType::Float4), float4_val(v) {}
        MaterialValue(RefCountPtr<Resource>& t) : type(MaterialPropertyType::Texture), texture(t) {}
        MaterialValue() : type(MaterialPropertyType::Float), float_val(0) {}
    };

    struct MaterialVariable
    {
        std::string name;
        MaterialValue value;
    };

    struct MaterialArchetype : public RefCounted
    {

    };

	struct MaterialResource : public Resource
	{
		RefCountPtr<MaterialArchetype> archetype;
        std::vector<MaterialVariable> variables;

		PHX_DECLARE_RESOURCE(MaterialResource);

		~MaterialResource() override;

		bool CollectPendingGpuTransitions(SpanMutable<GpuTransitionWork> transitions, size_t& fill_index) override;
	};
}