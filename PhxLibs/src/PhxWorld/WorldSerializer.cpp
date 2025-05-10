#include "PhxWorld/PhxWorld_pch.h"

#include <vector>
#include <functional>
#include <unordered_map>

#include "WorldSerializer.h"
#include "World.h"
#include "Entity.h"

#include <PhxCore/VFS.h>

#include <nlohmann/json.hpp>

#include <yaml-cpp/yaml.h>

#include <DirectXMath.h>

using namespace phx;
using namespace phx::WorldSerializer;


namespace YAML
{
    template<>
    struct convert<DirectX::XMFLOAT2>
    {
        static Node encode(const DirectX::XMFLOAT2& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, DirectX::XMFLOAT2& rhs)
        {
            if (!node.IsSequence() || node.size() != 2)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            return true;
        }
    };

    template<>
    struct convert<DirectX::XMFLOAT3>
    {
        static Node encode(const DirectX::XMFLOAT3& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, DirectX::XMFLOAT3& rhs)
        {
            if (!node.IsSequence() || node.size() != 3)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            return true;
        }
    };

    template<>
    struct convert<DirectX::XMFLOAT4>
    {
        static Node encode(const DirectX::XMFLOAT4& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.push_back(rhs.w);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, DirectX::XMFLOAT4& rhs)
        {
            if (!node.IsSequence() || node.size() != 4)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            rhs.w = node[3].as<float>();
            return true;
        }
    };

    template<>
    struct convert<phx::UUID>
    {
        static Node encode(const phx::UUID& uuid)
        {
            Node node;
            node.push_back((uint64_t)uuid);
            return node;
        }

        static bool decode(const Node& node, phx::UUID& uuid)
        {
            uuid = node.as<uint64_t>();
            return true;
        }
    };

    inline YAML::Emitter& operator<<(YAML::Emitter& out, phx::UUID const& uuid)
    {
        out << (uint64_t)uuid;
        return out;
    }

    inline YAML::Emitter& operator<<(YAML::Emitter& out, DirectX::XMFLOAT2 const& v)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
        return out;
    }

    inline YAML::Emitter& operator<<(YAML::Emitter& out, DirectX::XMFLOAT3 const& v)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
        return out;
    }

    inline YAML::Emitter& operator<<(YAML::Emitter& out, DirectX::XMFLOAT4 const& v)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
        return out;
    }

}


namespace
{
    void SerializeEntity(World& world, YAML::Emitter& out, Entity& entity)
    {
        PHX_CORE_ASSERT(entity.HasComponent<IDComponent>());

        out << YAML::BeginMap; // Entity
        out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

        if (entity.HasComponent<NameComponent>())
        {
            out << YAML::Key << "NameComponent";
            out << YAML::BeginMap; // NameComponent

            auto& comp = entity.GetComponent<NameComponent>();
            out << YAML::Key << "Name" << YAML::Value << comp.Name;

            out << YAML::EndMap; // NameComponent
        }

        if (entity.HasComponent<HierarchyComponent>())
        {
            auto& comp = entity.GetComponent<HierarchyComponent>();
            Entity parent = { comp.ParentID, &world };

            if (parent)
            {
                PHX_CORE_ASSERT(parent.HasComponent<IDComponent>());
                out << YAML::Key << "HierarchyComponent";
                out << YAML::BeginMap; // HierarchyComponent
                out << YAML::Key << "ParentID" << YAML::Value << parent.GetComponent<IDComponent>().ID;
                out << YAML::EndMap; // HierarchyComponent
            }
        }

        if (entity.HasComponent<TransformComponent>())
        {
            out << YAML::Key << "TransformComponent";
            out << YAML::BeginMap; // TransformComponent

            auto& comp = entity.GetComponent<TransformComponent>();
            out << YAML::Key << "Translation" << YAML::Value << comp.Translation;
            out << YAML::Key << "Rotation" << YAML::Value << comp.Rotation;
            out << YAML::Key << "Scale" << YAML::Value << comp.Scale;

            out << YAML::EndMap; // TransformComponent
        }

        if (entity.HasComponent<MeshComponent>())
        {
            out << YAML::Key << "MeshComponent";
            out << YAML::BeginMap; // MeshComponent

            auto& comp = entity.GetComponent<MeshComponent>();
            out << YAML::Key << "Name" << YAML::Value << comp.Mesh;

            out << YAML::EndMap; // MeshComponent
        }
    }
}

bool phx::WorldSerializer::Save(phx::IFileSystem* fs, const char* filename, phx::World& world)
{
    nlohmann::json outJson;

    // TODO: I AM HERE.
    outJson["World"] = "Untitled";
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "World" << YAML::Value << "Untitled";
    out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

    auto view = world.GetRegistry().view<entt::entity>();

    for (auto entityID : view)
    {
        Entity entity = { entityID, &world };
        if (!entity)
            continue;

        SerializeEntity(world, out, entity);
    }
    out << YAML::EndSeq;
    out << YAML::EndMap;

    const char* strData = out.c_str();
    fs->WriteFile(filename, Span(strData, strlen(strData)));

    return true;
}

bool phx::WorldSerializer::Load(phx::IFileSystem* fs, const char* filename, phx::World& world)
{
    YAML::Node data;
    try
    {
        data = YAML::LoadFile(fs->ResolvePath(filename).generic_string().c_str());
    }
    catch (YAML::ParserException e)
    {
        PHX_CORE_ERROR("Failed to load .phxwld file '{0}'\n     {1}", filename, e.what());
        return false;
    }

    if (!data["World"])
        return false;


    std::string worldName = data["World"].as<std::string>();
    PHX_CORE_INFO("Loading world '{0}'", worldName);


    auto entities = data["Entities"];
    if (!entities)
        return true;

    std::vector<std::function<void(World&)>> deferredQueue;
    std::unordered_map<entt::entity, UUID> entityIdLut;
    for (auto entity : entities)
    {
        phx::UUID uuid = entity["Entity"].as<phx::UUID>();

        std::string name;
        auto tagComponent = entity["NameComponent"];
        if (tagComponent)
            name = tagComponent["Name"].as<std::string>();

        PHX_CORE_INFO("Loading entity with ID = {0}, name = {1}", (uint64_t)uuid, name);

        Entity deserializedEntity = world.CreateEntity(uuid, name);

        auto hierarchyComponent = entity["HierarchyComponent"];
        if (hierarchyComponent)
        {
            // TODO: Defer set these up we can find the new entity value
            // Entities always have transforms
            // auto& comp = deserializedEntity.GetComponent<HierarchyComponent>();
            // UUID parentsID = hierarchyComponent["ParentID"].as<phx::UUID>();
            deferredQueue.push_back([](World&) mutable {

                // auto& comp = deserializedEntity.GetComponent<HierarchyComponent>();
                // TODO: I am here
            });
        }

        auto transformComponent = entity["TransformComponent"];
        if (transformComponent)
        {
            // Entities always have transforms
            auto& comp = deserializedEntity.GetComponent<TransformComponent>();
            comp.Translation = transformComponent["Translation"].as<DirectX::XMFLOAT3>();
            comp.Rotation = transformComponent["Rotation"].as<DirectX::XMFLOAT4>();
            comp.Scale = transformComponent["Scale"].as<DirectX::XMFLOAT3>();
        }

        auto meshComponent = entity["MeshComponent"];
        if (meshComponent)
        {
            auto& comp = deserializedEntity.GetComponent<MeshComponent>();
            comp.Mesh = tagComponent["Name"].as<std::string>();
        }
    }

    return true;
}