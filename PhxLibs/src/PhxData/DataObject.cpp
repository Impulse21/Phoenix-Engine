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
            ar << ArchiverOp::Value << *((float*)data);
        }
        else if (typeId == IntId)
        {
            ar << ArchiverOp::Value << *((int*)data);
        }
        else if (typeId == UIntId)
        {
            ar << ArchiverOp::Value << *((uint32_t*)data);
        }
        else if (typeId == StringId)
        {
            ar << ArchiverOp::Value << *((std::string*)data);
        }
        else if (typeId == BoolId)
        {
            ar << ArchiverOp::Value << *((bool*)data);
        }
        else if (typeId == XMFloat2Id)
        {
            ar << ArchiverOp::Value << *((DirectX::XMFLOAT2*)data);
        }
        else if (typeId == XMFloat3Id)
        {
            ar << ArchiverOp::Value << *((DirectX::XMFLOAT3*)data);
        }
        else if (typeId == XMFloat4Id)
        {
            ar << ArchiverOp::Value << *((DirectX::XMFLOAT4*)data);
        }
        else if (typeId == UUIDID || typeId == UUIDID_NS)
        {
            ar << ArchiverOp::Value << *((UUID*)data);
        }
        else
            ar << ArchiverOp::Value << "<unsupported>";
    }
}

void phx::data::IDataObj::Serialize(IArchiver& ar) const
{
    const TypeInfo& typeInfo = GetTypeInfo();
    for (auto& field : typeInfo.GetFields())
    {
        const void* ptr = (char*)this + field.Offset;
        const char* key = field.Name;

        ar << ArchiverOp::Key << key;
        if (field.IsArray)
        {
            ar << ArchiverOp::BeginSeq;

            // You might need size metadata or fix this at generation time
            for (size_t i = 0; i < field.ArraySize; ++i)
            {
                void* element = (uint8_t*)ptr + i * field.Stride;
                SerializeElement(ar, field.TypeHash, element);
            }
            ar << ArchiverOp::EndSeq;
        }
        else if (field.IsVector)
        {
            const auto& vec = *reinterpret_cast<const std::vector<phx::RefCountPtr<IDataObj>>*>(ptr);
#if false
            ar << ArchiverOp::BeginSeq;

            for (auto& item : vec)
            {
                if (ptr)
                {
                    ar << ArchiverOp::BeginMap;
                    item->Serialize(ar);
                    ar << ArchiverOp::EndMap;
                }
                else
                    ar << ArchiverOp::Null;
            }

            ar << ArchiverOp::EndSeq;
#else

            for (auto& item : vec)
            {

                ar << ArchiverOp::BeginMap;
                ar << ArchiverOp::Key << "type";
                ar << ArchiverOp::Value << item->GetType();

                if (ptr)
                {
                    ar << ArchiverOp::Key << "data";
                    ar << ArchiverOp::BeginMap;
                    item->Serialize(ar);
                    ar << ArchiverOp::EndMap;
                }
                else
                    ar << ArchiverOp::Null;


                ar << ArchiverOp::EndMap;
            }


#endif
        }
        else if (field.IsPointer)
        {
            const IDataObj* dataPtr = reinterpret_cast<const phx::RefCountPtr<IDataObj>*>(ptr)->Get();
            if (dataPtr)
            {
                ar << ArchiverOp::BeginMap;
                dataPtr->Serialize(ar);
                ar << ArchiverOp::EndMap;
            }
            else
                ar << ArchiverOp::Null;
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
