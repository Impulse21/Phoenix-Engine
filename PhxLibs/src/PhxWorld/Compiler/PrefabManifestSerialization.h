#pragma once

#include <PhxWorld/PrefabResource.h>

#include <nlohmann/json.hpp>

#include <PhxCore/Math.h>

namespace hlslpp
{
    namespace interop
    {
            static_assert(sizeof(hlslpp::interop::float4x4) == sizeof(float) * 16);

            inline void to_json(nlohmann::json& j, const hlslpp::interop::float3& v)
            {
                j = { v.x, v.y, v.z };
            }

            inline void from_json(nlohmann::json const& j, hlslpp::interop::float3& v)
            {
                j.at(0).get_to(v.x);
                j.at(1).get_to(v.y);
                j.at(2).get_to(v.z);
            }

            inline void to_json(nlohmann::json& j, const hlslpp::interop::float4& v)
            {
                j = { v.x, v.y, v.z, v.w };
            }

            inline void from_json(nlohmann::json const& j, hlslpp::interop::float4& v)
            {
                j.at(0).get_to(v.x);
                j.at(1).get_to(v.y);
                j.at(2).get_to(v.z);
                j.at(3).get_to(v.w);
            }

            // --- Serializer for math::Mat4 ---
            inline void to_json(nlohmann::json& j, const hlslpp::interop::float4x4& m)
            {
                const float* p = reinterpret_cast<const float*>(&m);
                j = std::vector<float>(p, p + 16);
            }

            inline void from_json(nlohmann::json const& j, hlslpp::interop::float4x4& m)
            {
                // Read the array of 16 floats back from the JSON
                auto v = j.get<std::vector<float>>();
                if (v.size() == 16)
                {
                    memcpy(&m, v.data(), 16 * sizeof(float));
                }
            }
	}
}

namespace phx
{
    inline void to_json(nlohmann::json& j, ManifestMeshInstance const& d)
    {
        j = nlohmann::json{
            {"mesh", d.mesh_path},
        };

        if (d.material_path != std::nullopt)
        {
            j["material_path"] = d.material_path.value();
        }
    }

    inline void from_json(nlohmann::json const& j, ManifestMeshInstance& d)
    {
        j.at("mesh").get_to(d.mesh_path);

        if (j.count("material_path") != 0)
        {
            d.material_path = j.at("material_path").get<std::string>();
        }
    }

    inline void to_json(nlohmann::json& j, ManifestCameraData const& d)
    {
        j = nlohmann::json{
            {"type", d.type},
            {"fov_y", d.fov_y},
            {"z_near", d.z_near},
            {"z_far", d.z_far}
        };
    }

    inline void from_json(nlohmann::json const& j, ManifestCameraData& d)
    {
        j.at("type").get_to(d.type);
        j.at("fov_y").get_to(d.fov_y);
        j.at("z_near").get_to(d.z_near);
        j.at("z_far").get_to(d.z_far);
    }

    inline void to_json(nlohmann::json& j, const ManifestLightData& d)
    {
        j = nlohmann::json{
            {"type", d.type},
            {"colour", d.colour}, // This will now correctly call the to_json for math::float3
            {"intensity", d.intensity}
        };
    }

    inline void from_json(const nlohmann::json& j, ManifestLightData& d)
    {
        j.at("type").get_to(d.type);
        j.at("colour").get_to(d.colour); // This will now correctly call the from_json for math::float3
        j.at("intensity").get_to(d.intensity);
    }

    // --- Serializer for PrefabManifest::Node ---
    inline void to_json(nlohmann::json& j, PrefabManifest::Node const& n)
    {
        // Required fields
        j = nlohmann::json{
            {"name", n.name},
            {"parent_index", n.parent_index},
            {"transform", 
                { "scale", n.scale },
                { "rotation", n.rotation },
                { "translation", n.translation},
            },
            {"node_type", n.node_type}
        };

        // nlohmann::json handles std::optional automatically.
        // If the optional is empty (std::nullopt), it will serialize as 'null'.
        // If it has a value, it will serialize just the value.

        if (n.mesh_instance_data != std::nullopt)
        {
            j["mesh_instance_data"] = n.mesh_instance_data.value();
        }

        if (n.camera_data != std::nullopt)
        {
            j["camera_data"] = n.camera_data.value();
        }

        if (n.light_data != std::nullopt)
        {
            j["light_data"] = n.light_data.value();
        }

        if (n.nested_prefab_path != std::nullopt)
        {
            j["nested_prefab_path"] = n.nested_prefab_path.value();
        }
    }

    inline void from_json(const nlohmann::json& j, PrefabManifest::Node& n)
    {

        // Required fields
        j.at("name").get_to(n.name);
        j.at("parent_index").get_to(n.parent_index);
        j.at("transform").at("scale").get_to(n.scale);
        j.at("transform").at("rotation").get_to(n.rotation);
        j.at("transform").at("translation").get_to(n.translation);
        j.at("node_type").get_to(n.node_type);

        // --- Optional fields ---
        // We must check if the key exists AND is not null before reading.

        if (j.count("diagnostic") != 0)
        {
        }

        if (j.contains("mesh_instance_data") && !j.at("mesh_instance_data").is_null()) 
        {
			n.mesh_instance_data = j.at("mesh_instance_data").get< ManifestMeshInstance >();
        }

        if (j.contains("camera_data") && !j.at("camera_data").is_null()) 
        {
            n.camera_data = j.at("camera_data").get<ManifestCameraData>();
        }

        if (j.contains("light_data") && !j.at("light_data").is_null()) 
        {
            n.light_data = j.at("light_data").get<ManifestLightData>();
        }

        if (j.contains("nested_prefab_path") && !j.at("nested_prefab_path").is_null()) 
        {
            n.nested_prefab_path = j.at("nested_prefab_path").get<std::string>();
        }
    }

    // --- Serializer for PrefabManifest (The Root Object) ---

    inline void to_json(nlohmann::json& j, const PrefabManifest& p)
    {
        j = { {"nodes", p.nodes} };
    }

    inline void from_json(const nlohmann::json& j, PrefabManifest& p)
    {
        j.at("nodes").get_to(p.nodes);
    }

} // namespace phx::compiler
