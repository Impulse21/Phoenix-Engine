#include "PhxWorld/PhxWorld_pch.h"

#include <vector>
#include <functional>
#include <unordered_map>
#include <fstream>

#include "WorldSerializer.h"
#include "World.h"
#include "Entity.h"


#include <nlohmann/json.hpp>

#include <yaml-cpp/yaml.h>

#include <DirectXMath.h>

using namespace phx;
using namespace phx::WorldSerializer;

using json = nlohmann::json;

namespace DirectX
{
    void to_json(json& j, const DirectX::XMFLOAT3& v) 
    {
        j = json{ {"x", v.x}, {"y", v.y}, {"z", v.z}};
    }

    void from_json(const json& j, DirectX::XMFLOAT3& v)
    {
        j.at("x").get_to(v.x);
        j.at("y").get_to(v.y);
        j.at("z").get_to(v.x);
    }

    void to_json(json& j, const DirectX::XMFLOAT4& v)
    {
        j = json{ {"x", v.x}, {"y", v.y}, {"z", v.z},{"w", v.w} };
    }

    void from_json(const json& j, DirectX::XMFLOAT4& v)
    {
        j.at("x").get_to(v.x);
        j.at("y").get_to(v.y);
        j.at("z").get_to(v.x);
        j.at("w").get_to(v.w);
    }
} // namespace ns
namespace
{
    void SerializeEntity(World& world, nlohmann::json& out, Entity& entity)
    {
        using namespace entt;
        out["_id"] = static_cast<uint64_t>(entity.GetUUID());
        out["_name"] = entity.GetName();

        if (entity.HasComponent<HierarchyComponent>())
        {
            auto& comp = entity.GetComponent<HierarchyComponent>();
            Entity parent = { comp.ParentID, &world };

            if (parent)
            {
                out["_parent"] = static_cast<uint64_t>(parent.GetUUID());
            }
        }

        nlohmann::json componentsJson;
        // Disabled reflection version of code for now as it wasn't working
#if false
        for (auto&& [typeId, storageBase] : world.GetRegistry().storage())
        {
            if (!storageBase.contains(entity))
                continue;

            entt::meta_type type = entt::resolve(typeId);
            if (!type)
                continue;


            entt::meta_any instance = storageBase.value(entity);
            if (!instance)
                continue;

            nlohmann::json compJson;
            for (auto&& [typeId, member] : type.data())
            {
                if (!member.type())
                    continue;

                entt::meta_any value = member.get(instance);
                if (!value)
                    continue;

                std::string_view key = member.type().info().name();

                if (float* v = value.try_cast<float>(); v)
                    componentsJson[key] = *v;
                else if (int* v = value.try_cast<int>(); v)
                    componentsJson[key] = *v;
                else if (uint32_t* v = value.try_cast<uint32_t>(); v)
                    componentsJson[key] = *v;
                else if (std::string* v = value.try_cast<std::string>(); v)
                    componentsJson[key] = *v;
                else if (DirectX::XMFLOAT3* v = value.try_cast<DirectX::XMFLOAT3>(); v)
                {
                    nlohmann::json float3Json;
                    float3Json["x"] = v->x;
                    float3Json["y"] = v->y;
                    float3Json["z"] = v->z;
                    componentsJson[key] = float3Json;
                }
                else if (DirectX::XMFLOAT4* v = value.try_cast<DirectX::XMFLOAT4>(); v)
                {
                    nlohmann::json float3Json;
                    float3Json["x"] = v->x;
                    float3Json["y"] = v->y;
                    float3Json["z"] = v->z;
                    float3Json["w"] = v->w;
                    componentsJson[key] = float3Json;
                }
                else
                    componentsJson[key] = "[unsupported]";
            }
            componentsJson[type.info().name()] = compJson;
        }
#else
		if (entity.HasComponent<TransformComponent>())
		{
			auto& comp = entity.GetComponent<TransformComponent>();

			componentsJson["TransformComponent"] = {
			    {"transation", comp.Translation},
			    {"rotation", comp.Rotation},
			    {"scale", comp.Scale},
			};
		}
        if (entity.HasComponent<MeshComponent>())
        {
        }
#endif

        out["components"] = componentsJson;
    }
}

bool phx::WorldSerializer::Save(phx::IFileSystem* /*fs*/, const char* /*filename*/, phx::World& world)
{
    nlohmann::json outJson;

    // TODO: I AM HERE.
    outJson["World"] = "Untitled";

    nlohmann::json entitiesArray = nlohmann::json::array();

    auto view = world.GetRegistry().view<entt::entity>();
    for (auto entityID : view)
    {
        Entity entity = { entityID, &world };
        if (!entity)
            continue;
        
        nlohmann::json entityJson;

        SerializeEntity(world, entityJson, entity);
        entitiesArray.push_back(entityJson);
    }

    outJson["Entities"] = entitiesArray;
    const int indent = 4;
    std::string jsonStr = outJson.dump(indent);
    PHX_CORE_ASSERT(false, "IMPLEMENT WRITING");
    //fs->WriteFile(filename, Span(jsonStr.data(), jsonStr.size()));

    return true;
}

bool phx::WorldSerializer::Load(phx::IFileSystem* /*fs*/, const char* /*filename*/, phx::World& world)
{
    //std::string str = fs->ResolvePath(filename).generic_string();

    PHX_CORE_ASSERT(false, "IMPLEMENT READING");
    std::string str;
    std::ifstream ifs(str.c_str());
    json inputJson = json::parse(ifs);

    for (auto& entityJson : inputJson["Entities"])
    {
        UUID id(entityJson["_id"].get<uint64_t>());
        std::string name = entityJson["_name"].get<std::string>();

        Entity entity = world.CreateEntity(id, name);

        if (!entityJson.contains("components"))
            continue;

		json componentsJson = entityJson["components"];
		if (componentsJson.contains("MeshComponent"))
		{
#if false
			nlohmann::json compJson = componentsJson["MeshComponent"];
			MeshComponent comp = {
				.Mesh = compJson["Mesh"].get<std::string>()
			};
			entity.AddComponent<MeshComponent>(comp);
#endif
		}

		if (componentsJson.contains("TransformComponent"))
		{
			nlohmann::json compJson = componentsJson["TransformComponent"];
			TransformComponent comp = {
				.Scale = compJson["scale"],
				.Rotation = compJson["rotation"],
				.Translation = compJson["transation"],
			};

			entity.AddOrReplaceComponent<TransformComponent>(comp);
		}
	}

    return true;
}