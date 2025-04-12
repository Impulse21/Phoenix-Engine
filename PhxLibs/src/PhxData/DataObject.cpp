#include "PhxData_pch.h"
#include "DataObject.h"
#include "Archiver.h"

#include <PhxCore/StringHash.h>

using namespace phx::data;
namespace
{
    constexpr phx::StringHash FloatId = "float"_hash;
    constexpr phx::StringHash IntId = "int_32"_hash;
    constexpr phx::StringHash UIntId = "uint_32"_hash;
    constexpr phx::StringHash StringId = "std::string"_hash;
    constexpr phx::StringHash BoolId = "bool"_hash;
    constexpr phx::StringHash UUIDID_NS = "phx::UUID"_hash;
    constexpr phx::StringHash UUIDID = "UUID"_hash;
    constexpr phx::StringHash XMFloat2Id = "DirectX::XMFLOAT2"_hash;
    constexpr phx::StringHash XMFloat3Id = "DirectX::XMFLOAT3"_hash;
    constexpr phx::StringHash XMFloat4Id = "DirectX::XMFLOAT4"_hash;
}

void phx::data::IDataObj::Serialize(IArchiver& ar) const
{
    const TypeInfo& typeInfo = GetTypeInfo();
    for (auto& field : typeInfo.GetFields())
    {
        const void* ptr = (char*)this + field.Offset;
        const char* key = field.Name;

        if (field.IsPointer)
        {
            // SerializeToYAML(ar, ptr, *field.NestedType);
        }
        else if (field.TypeHash == FloatId)
        {
            ar << std::make_pair(key, *(float*)ptr);
        }
        else if (field.TypeHash == IntId)
        {
            ar << *(int*)ptr;
        }
        else if (field.TypeHash == UIntId)
        {
            ar << *(uint32_t*)ptr;
        }
        else if (field.TypeHash == StringId)
        {
            ar << *(std::string*)ptr;
        }
        else if (field.TypeHash == BoolId)
        {
            ar << (*(bool*)ptr ? "true" : "false");
        }
        else if (field.TypeHash == XMFloat2Id)
        {
            ar << *(DirectX::XMFLOAT2*)ptr;
        }
        else if (field.TypeHash == XMFloat3Id)
        {
            ar << *(DirectX::XMFLOAT3*)ptr;
        }
        else if (field.TypeHash == XMFloat4Id)
        {
            ar << *(DirectX::XMFLOAT4*)ptr;
        }
        else if (field.TypeHash == UUIDID || field.TypeHash == UUIDID_NS)
        {
            ar << *(UUID*)ptr;
        }
        else
            ar << "<unsupported>";
    }

}

void phx::data::IDataObj::Deserialize(IArchiver& ar)
{
}
