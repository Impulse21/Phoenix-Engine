#include "PhxData_pch.h"
#include "WorldChunk.def.h"

#include <PhxCore/Span.h>
#include <PhxCore/VFS.h>

#include <yaml-cpp/yaml.h>

#include <DirectXMath.h>

using namespace phx::data;


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

void WorldChunk::Serialize(YAML::Emitter& emitter) const
{

    emitter << YAML::Key << "WorldChunk";
    emitter << YAML::BeginMap; // TransformComponent
    emitter << "PackFile" << PackFile;

    emitter << "Root";

    Root->Serialize(emitter);

    emitter << YAML::EndMap; // TransformComponent
}

void phx::data::WorldChunk::Deserialize(YAML::Node& node)
{
    ID = node["ID"].as<UUID>();
    PackFile = node["PackFile"].as<std::string>();

    if (!node["PackFile"])
        return;

    Root = RefPtr<Entity>::Create();
    auto packFileNode = node["PackFile"];
    Root->Deserialize(packFileNode);
}

void Entity::Serialize(YAML::Emitter& emitter) const
{
    emitter << YAML::BeginMap; // Entity Map

    emitter << "ID" << ID;
    emitter << "Name" << Name;

    // Components
    emitter << YAML::Key << "Components" << YAML::Value;
    emitter << YAML::BeginSeq;
    for (auto& component : Components)
    {
        emitter << YAML::BeginMap;
        component->Serialize(emitter);
        emitter << YAML::EndMap;
    }
    emitter << YAML::EndSeq;

    // Children
    emitter << YAML::Key << "Children" << YAML::Value;
    emitter << YAML::BeginSeq;
    for (auto& child : Children)
    {
        child->Serialize(emitter);
    }
    emitter << YAML::EndSeq;

    emitter << YAML::EndMap; // End Entity Map
}

void phx::data::Entity::Deserialize(YAML::Node& node)
{
    ID = node["ID"].as<UUID>();
    Name = node["Name"].as<std::string>();

    auto components = node["Components"];
    if (components)
    {
        Components.reserve(components.size());
        for (auto component : components)
        {
            if (component["MeshComponent"])
            {
                auto compNode = component["MeshComponent"];

                auto comp = RefPtr<MeshComponent>::Create();
                comp->Deserialize(compNode);
                Components.push_back(comp);
            }
            else if (component["TransformComponent"])
            {
                auto compNode = component["MeshComponent"];

                auto comp = RefPtr<TransformComponent>::Create();
                comp->Deserialize(compNode);
                Components.push_back(comp);
            }
        }
    }

    auto children = node["Children"];
    if (children)
    {
        Children.reserve(children.size());
        for (auto child : children)
        {
            Children.push_back(RefPtr<Entity>::Create());
            Children.back()->Deserialize(child);
        }
    }
}

void MeshComponent::Serialize(YAML::Emitter& emitter) const
{
    emitter << YAML::Key << "MeshComponent";
    emitter << YAML::BeginMap; // TransformComponent

    emitter << YAML::Key << "Mesh" << YAML::Value << Mesh.c_str();

    emitter << YAML::EndMap; // TransformComponent
}

void phx::data::MeshComponent::Deserialize(YAML::Node& node)
{
    Mesh = node["Mesh"].as<std::string>();
}

void TransformComponent::Serialize(YAML::Emitter& emitter) const
{
    emitter << YAML::Key << "TransformComponent";
    emitter << YAML::BeginMap; // TransformComponent

    emitter << YAML::Key << "Translation" << YAML::Value << Translation;
    emitter << YAML::Key << "Rotation" << YAML::Value << Rotation;
    emitter << YAML::Key << "Scale" << YAML::Value << Scale;

    emitter << YAML::EndMap; // TransformComponent
}

void phx::data::TransformComponent::Deserialize(YAML::Node& node)
{
    Translation = node["Translation"].as<DirectX::XMFLOAT3>();
    Rotation = node["Translation"].as<DirectX::XMFLOAT4>();
    Scale = node["Translation"].as<DirectX::XMFLOAT3>();
}

void phx::data::Save(phx::IFileSystem* fs, const char* filename, WorldChunk const& chunk)
{
    YAML::Emitter emitter;

    emitter.SetIndent(4);
    emitter.SetMapFormat(YAML::Block);
    emitter << YAML::BeginMap;

    chunk.Serialize(emitter);

    emitter << YAML::EndMap;
    const char* strData = emitter.c_str();
    fs->WriteFile(filename, Span(strData, strlen(strData)));
}

phx::data::RefPtr<WorldChunk> phx::data::Load(phx::IFileSystem* fs, const char* filename)
{
    YAML::Node data;
    try
    {
        data = YAML::LoadFile(fs->ResolvePath(filename).generic_string().c_str());
    }
    catch (YAML::ParserException e)
    {
        PHX_CORE_ERROR("Failed to load .hazel file '{0}'\n     {1}", filename, e.what());
        return nullptr;
    }


    if (!data["WorldChunk"])
        return nullptr;

    auto worldChunk = RefPtr<WorldChunk>::Create();
    worldChunk->Deserialize(data);

    return worldChunk;
}
