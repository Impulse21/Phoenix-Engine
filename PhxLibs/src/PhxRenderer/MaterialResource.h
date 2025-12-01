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
        TexturePath // Special: Just a string in the resource
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

        // Strings usually need external storage if not using a custom small-string optimization
        // But for a Resource, a std::string member outside the union is fine/safe.
        std::string texture_path;

        // Constructors for easy assignment
        MaterialValue(float v) : type(MaterialPropertyType::Float), float_val(v) {}
        MaterialValue(hlslpp::interop::float2 v) : type(MaterialPropertyType::Float2), float2_val(v) {}
        MaterialValue(hlslpp::interop::float3 v) : type(MaterialPropertyType::Float3), float3_val(v) {}
        MaterialValue(hlslpp::interop::float4 v) : type(MaterialPropertyType::Float4), float4_val(v) {}
        MaterialValue(const std::string& s) : type(MaterialPropertyType::TexturePath), texture_path(s) {}
        MaterialValue() : type(MaterialPropertyType::Float), float_val(0) {}
    };

	struct MaterialResource : public Resource
	{
		std::string archetype_name;
		std::unordered_map<std::string, MaterialValue> properties;

		PHX_DECLARE_RESOURCE(MaterialResource);

		~MaterialResource() override;

		bool CollectPendingGpuTransitions(SpanMutable<GpuTransitionWork> transitions, size_t& fill_index) override;
	};
}