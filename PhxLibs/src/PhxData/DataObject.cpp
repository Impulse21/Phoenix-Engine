#include "PhxData_pch.h"
#include "DataObject.h"
#include "ArchiverYaml.h"

#include <PhxCore/VFS.h>
#include <PhxCore/StringHash.h>


using namespace phx;
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
            ar << ArchiveField(key, *(float*)ptr);
        }
        else if (field.TypeHash == IntId)
        {
            ar << ArchiveField(key, *(int*)ptr);
        }
        else if (field.TypeHash == UIntId)
        {
            ar << ArchiveField(key, *(uint32_t*)ptr);
        }
        else if (field.TypeHash == StringId)
        {
            ar << ArchiveField(key, *(std::string*)ptr);
        }
        else if (field.TypeHash == BoolId)
        {
            ar << ArchiveField(key, *(bool*)ptr);
        }
        else if (field.TypeHash == XMFloat2Id)
        {
            ar << ArchiveField(key, *(DirectX::XMFLOAT2*)ptr);
        }
        else if (field.TypeHash == XMFloat3Id)
        {
            ar << ArchiveField(key, *(DirectX::XMFLOAT3*)ptr);
        }
        else if (field.TypeHash == XMFloat4Id)
        {
            ar << ArchiveField(key, *(DirectX::XMFLOAT4*)ptr);
        }
        else if (field.TypeHash == UUIDID || field.TypeHash == UUIDID_NS)
        {
            ar << ArchiveField(key, *(UUID*)ptr);
        }
        else
            ar << ArchiveField(key, "<unsupported>");
    }

}

void phx::data::IDataObj::Deserialize(IArchiver&)
{
}

void phx::data::Save(phx::IFileSystem* fs, const char* filename, IDataObj const& dataObj)
{
    YamlArchiver archiver(fs, filename);
    dataObj.Serialize(archiver);
}

void phx::data::Load(phx::IFileSystem* /*fs*/, const char* /*filename*/, IDataObj& /*dataObj*/)
{
}
