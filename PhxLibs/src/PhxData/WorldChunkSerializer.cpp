#include "PhxData_pch.h"
#include "WorldChunk.def.h"

#include <PhxCore/Span.h>
#include <PhxCore/VFS.h>

#include <yaml-cpp/yaml.h>

#include <DirectXMath.h>

using namespace phx::data;


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

void WorldChunk::Serialize(YAML::Emitter& emitter) const
{

    emitter << YAML::Key << "WorldChunk";
    emitter << YAML::BeginMap; // TransformComponent
    emitter << "PackFile" << PackFile;

    emitter << "Root";

    Root->Serialize(emitter);

    emitter << YAML::EndMap; // TransformComponent
}

void Entity::Serialize(YAML::Emitter& emitter) const
{
    emitter << YAML::BeginMap; // Entity Map

    emitter << YAML::Key << "Entity" << YAML::Value << Name;

    // Components
    emitter << YAML::Key << "Components" << YAML::Value;
    emitter << YAML::BeginSeq;
    for (auto& component : Components)
    {
        component->Serialize(emitter);
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

void MeshComponent::Serialize(YAML::Emitter& emitter) const
{
    emitter << YAML::Key << "MeshComponent";
    emitter << YAML::BeginMap; // TransformComponent

    emitter << YAML::Key << "Mesh" << YAML::Value << Mesh.c_str();

    emitter << YAML::EndMap; // TransformComponent
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