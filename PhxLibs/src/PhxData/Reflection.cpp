#include "PhxData_pch.h"
#include "Reflection.h"

#include <PhxCore/UUID.h>
#include <PhxCore/VFS.h>

#include "yaml-cpp/yaml.h"

using namespace phx;
using namespace phx::rft;


namespace
{
    constexpr phx::StringHash FloatId       = "float"_hash;
    constexpr phx::StringHash IntId         = "int_32"_hash;
    constexpr phx::StringHash UIntId        = "uint_32"_hash;
    constexpr phx::StringHash StringId      = "std::string"_hash;
    constexpr phx::StringHash BoolId        = "bool"_hash;
    constexpr phx::StringHash UUIDID_NS     = "phx::UUID"_hash;
    constexpr phx::StringHash UUIDID        = "UUID"_hash;
    constexpr phx::StringHash XMFloat2Id    = "DirectX::XMFLOAT2"_hash;
    constexpr phx::StringHash XMFloat3Id    = "DirectX::XMFLOAT3"_hash;
    constexpr phx::StringHash XMFloat4Id    = "DirectX::XMFLOAT4"_hash;
}

YAML::Emitter& operator<<(YAML::Emitter& out, phx::UUID const& uuid)
{
    out << (uint64_t)uuid;
    return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, DirectX::XMFLOAT2 const& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
    return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, DirectX::XMFLOAT3 const& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, DirectX::XMFLOAT4 const& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
    return out;
}

void phx::rft::SerializeToYAML(YAML::Emitter& out, const void* obj, const TypeInfo& type)
{

    out << YAML::Key << type.TypeName;
    out << YAML::BeginMap; // TransformComponent

    for (auto& field : type.GetFields())
    {
        const void* ptr = (char*)obj + field.Offset;
        out << YAML::Key << field.Name << YAML::Value;

        if (field.NestedType)
        {
            SerializeToYAML(out, ptr, *field.NestedType);
        }
        else if (field.TypeHash.Value() == FloatId.Value())
        {
            out << *(float*)ptr;
        }
        else if (field.TypeHash.Value() == IntId.Value())
        {
            out << *(int*)ptr;
        }
        else if (field.TypeHash.Value() == UIntId.Value())
        {
            out << *(uint32_t*)ptr;
        }
        else if (field.TypeHash.Value() == StringId.Value())
        {
            out << "\"" << *(std::string*)ptr << "\"";
        }
        else if (field.TypeHash.Value() == BoolId.Value())
        {
            out << (*(bool*)ptr ? "true" : "false");
        }
        else if (field.TypeHash.Value() == XMFloat2Id.Value())
        {
            out << *(DirectX::XMFLOAT2*)ptr;
        }
        else if (field.TypeHash.Value() == XMFloat3Id.Value())
        {
            out << *(DirectX::XMFLOAT3*)ptr;
        }
        else if (field.TypeHash.Value() == XMFloat4Id.Value())
        {
            out << *(DirectX::XMFLOAT4*)ptr;
        }
        else if (field.TypeHash.Value() == UUIDID.Value() || field.TypeHash.Value() == UUIDID_NS.Value())
        {
            out << *(UUID*)ptr;
        }
        else
            out << "<unsupported>";
    }

    out << YAML::EndMap;
}

bool phx::rft::SerializeToYAML(phx::IFileSystem* fs, const char* filename, TypeInfo const& typeInfo, const void* obj)
{
    YAML::Emitter out;

    SerializeToYAML(out, obj, typeInfo);

    const char* strData = out.c_str();
    fs->WriteFile(filename, Span(strData, strlen(strData)));

    return true;
}
