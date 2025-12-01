#pragma once

#include <PhxRenderer/MaterialResource.h>

#include <nlohmann/json.hpp>
#include "PrefabManifestSerialization.h"
#include <PhxCore/Math.h>

namespace phx::renderer
{
    inline void to_json(nlohmann::json& j, const MaterialValue& v)
    {
        switch (v.type)
        {
        case MaterialPropertyType::Float:
            j = v.float_val;
            break;
        case MaterialPropertyType::Int:
            j = v.int_val;
            break;
        case MaterialPropertyType::Bool:
            j = v.bool_val;
            break;
        case MaterialPropertyType::Float2:
            j = v.float2_val;
            break;
        case MaterialPropertyType::Float3:
            j = v.float3_val;
            break;
        case MaterialPropertyType::Float4:
            j = v.float4_val;
            break;
        case MaterialPropertyType::TexturePath:
            j = v.texture_path;
            break;
        default:
            j = nullptr;
            break;
        }
    }

    void from_json(const nlohmann::json& j, MaterialValue& v)
    {
        using namespace nlohmann;
        if (j.is_number_float())
        {
            v.type = MaterialPropertyType::Float;
            v.float_val = j.get<float>();
        }
        else if (j.is_number_integer())
        {
            v.type = MaterialPropertyType::Int;
            v.int_val = j.get<int32_t>();
        }
        else if (j.is_boolean())
        {
            v.type = MaterialPropertyType::Bool;
            v.bool_val = j.get<bool>();
        }
        else if (j.is_string())
        {
            v.type = MaterialPropertyType::TexturePath;
            v.texture_path = j.get<std::string>();
        }
        else if (j.is_array())
        {
            size_t size = j.size();
            if (size == 2)
            {
                v.type = MaterialPropertyType::Float2;
                j.get_to(v.float2_val);
            }
            else if (size == 3)
            {
                v.type = MaterialPropertyType::Float3;
                j.get_to(v.float3_val);
            }
            else if (size == 4)
            {
                v.type = MaterialPropertyType::Float4;
                j.get_to(v.float4_val);
            }
            else
            {
                v.type = MaterialPropertyType::Float;
            }
        }
    }

    inline void to_json(nlohmann::json& j, const MaterialResource& resource)
    {
        j = {
            { "asset_type", "MaterialInstance"},
            { "archetype_name", resource.archetype_name},
            { "properties", resource.properties }};
    }

    inline void from_json(const nlohmann::json& j, MaterialResource& resource)
    {
        if (j.contains("asset_type"))
        {
            std::string type = j["asset_type"];
            if (type != "MaterialInstance") 
            {
                PHX_CORE_ERROR("Invalid material asset type");
                return;
            }
        }

        if (j.contains("archetype"))
        {
            resource.archetype_name = j["archetype"].get<std::string>();
        }
        else
        {
            resource.archetype_name = "Standard";
        }

        if (j.contains("properties"))
        {
            resource.properties = j["properties"].get<std::unordered_map<std::string, MaterialValue>>();
        }
    }

} // namespace phx::compiler
