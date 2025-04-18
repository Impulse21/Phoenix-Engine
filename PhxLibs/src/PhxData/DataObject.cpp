#include "PhxData_pch.h"
#include "DataObject.h"
#include "ArchiverYaml.h"

#include <PhxCore/VFS.h>
#include <PhxCore/StringHash.h>
#include <PhxCore/RefCountPtr.h>

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

    void SerializeElement(IArchiver& ar, StringHash typeId, const void* data)
    {
        if (typeId == FloatId)
        {
            ar << ArhiverOp::Value << *((float*)data);
        }
        else if (typeId == IntId)
        {
            ar << ArhiverOp::Value << *((int*)data);
        }
        else if (typeId == UIntId)
        {
            ar << ArhiverOp::Value << *((uint32_t*)data);
        }
        else if (typeId == StringId)
        {
            ar << ArhiverOp::Value << *((std::string*)data);
        }
        else if (typeId == BoolId)
        {
            ar << ArhiverOp::Value << *((bool*)data);
        }
        else if (typeId == XMFloat2Id)
        {
            ar << ArhiverOp::Value << *((DirectX::XMFLOAT2*)data);
        }
        else if (typeId == XMFloat3Id)
        {
            ar << ArhiverOp::Value << *((DirectX::XMFLOAT3*)data);
        }
        else if (typeId == XMFloat4Id)
        {
            ar << ArhiverOp::Value << *((DirectX::XMFLOAT4*)data);
        }
        else if (typeId == UUIDID || typeId == UUIDID_NS)
        {
            ar << ArhiverOp::Value << *((UUID*)data);
        }
        else
            ar << ArhiverOp::Value << "<unsupported>";
    }
}

void phx::data::IDataObj::Serialize(IArchiver& ar) const
{
    const TypeInfo& typeInfo = GetTypeInfo();
    for (auto& field : typeInfo.GetFields())
    {
        const void* ptr = (char*)this + field.Offset;
        const char* key = field.Name;

        ar << ArhiverOp::Key << key;
        if (field.IsArray)
        {
            ar << ArhiverOp::BeginSeq;

            // You might need size metadata or fix this at generation time
            for (size_t i = 0; i < field.ArraySize; ++i)
            {
                void* element = (uint8_t*)ptr + i * field.Stride;
                SerializeElement(ar, field.TypeHash, element);
            }
            ar << ArhiverOp::EndSeq;
        }
        else if (field.IsVector)
        {
            const auto& vec = *reinterpret_cast<const std::vector<phx::RefCountPtr<IDataObj>>*>(ptr);
            ar << ArhiverOp::BeginSeq;

            for (auto& item : vec)
            {
                if (ptr)
                {
                    ar << ArhiverOp::BeginMap;
                    item->Serialize(ar);
                    ar << ArhiverOp::EndMap;
                }
                else
                    ar << ArhiverOp::Null;
            }

            ar << ArhiverOp::EndSeq;
        }
        else if (field.IsPointer)
        {
            const IDataObj* dataPtr = reinterpret_cast<const phx::RefCountPtr<IDataObj>*>(ptr)->Get();
            if (dataPtr)
            {
                ar << ArhiverOp::BeginMap;
                dataPtr->Serialize(ar);
                ar << ArhiverOp::EndMap;
            }
            else
                ar << ArhiverOp::Null;
        }
        else
        {
            SerializeElement(ar, field.TypeHash, ptr);
        }
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
