#include "PhxAsset_pch.h"

#include <PhxCore/Reflect/TypeInfo.h>

#include <PhxAsset/YamlAssetWriter.h>

#include <yaml-cpp/yaml.h>

#include <fstream>

using namespace phx;
using namespace phx::asset;

YamlAssetWriter::YamlAssetWriter(IVirtualFileSystem *vfs)
    : m_vfs(vfs)
{
}


#if !USE_VENDOR_REFLECTION
bool phx::asset::YamlAssetWriter::Write(std::string_view path, const reflect::TypeInfo &type_info, const void *asset)
{
    phx::Result<std::string> physical_path_result = m_vfs->ResolveVirtualToPhysicalPath(std::string(path));

    if (physical_path_result.HasError())
        return false;

    YAML::Emitter emitter;
    WriteStruct(emitter, type_info, asset);

    std::ofstream out(physical_path_result->c_str());
    if (!out.is_open()) 
        return false;

    out << emitter.c_str();

    return true;
}

void phx::asset::YamlAssetWriter::WriteStruct(YAML::Emitter& emitter, const reflect::TypeInfo &type_info, const void* struct_ptr) const
{
    emitter << YAML::BeginMap;
    for (const auto& field_info : type_info.fields)
    {
        const void* field_ptr = static_cast<const uint8_t*>(struct_ptr) + field_info.offset;
        emitter << YAML::Key << std::string(field_info.name);
        emitter << YAML::Value;

        WriteField(emitter, field_info, field_ptr);
    }

    emitter << YAML::EndMap;
}

void phx::asset::YamlAssetWriter::WriteField(YAML::Emitter& emitter, const reflect::FieldInfo &field_info, const void *field_ptr) const
{
    switch (field_info.kind)
    {
    case reflect::FieldKind::Bool:
        emitter << *static_cast<const bool *>(field_ptr);
        break;

    case reflect::FieldKind::Int32:
        emitter << *static_cast<const int32_t *>(field_ptr);
        break;

    case reflect::FieldKind::Int64:
        emitter << *static_cast<const int64_t *>(field_ptr);
        break;

    case reflect::FieldKind::Uint32:
        emitter << *static_cast<const uint32_t *>(field_ptr);
        break;

    case reflect::FieldKind::Uint64:
        emitter << *static_cast<const uint64_t *>(field_ptr);
        break;

    case reflect::FieldKind::Float:
        emitter << *static_cast<const float *>(field_ptr);
        break;

    case reflect::FieldKind::Double:
        emitter << *static_cast<const double *>(field_ptr);
        break;

    case reflect::FieldKind::String:
        emitter << *static_cast<const std::string *>(field_ptr);
        break;

    case reflect::FieldKind::Nested:
        WriteStruct(emitter, *field_info.nested_type, field_ptr);
        break;

    case reflect::FieldKind::Array:
        WriteArray(emitter, field_info, field_ptr);
        break;

    case reflect::FieldKind::AssetPtr:
        // TODO:
        // out << static_cast<const AssetPtrBase *>(ptr)->GetPath();
        PHX_ASSERT(false, "TODO");
        break;

    case reflect::FieldKind::Float2:
    {
        const auto* v = static_cast<const hlslpp::interop::float2*>(field_ptr);
        emitter << YAML::Flow << YAML::BeginSeq << v->x << v->y << YAML::EndSeq;
        break;
    }
    case reflect::FieldKind::Float3:
    {
        const auto* v = static_cast<const hlslpp::interop::float3*>(field_ptr);
        emitter << YAML::Flow << YAML::BeginSeq << v->x << v->y << v->z << YAML::EndSeq;
        break;
    }
    case reflect::FieldKind::Float4:
    {
        const auto* v = static_cast<const hlslpp::interop::float3*>(field_ptr);
        emitter << YAML::Flow << YAML::BeginSeq << v->x << v->y << v->z << v->z << YAML::EndSeq;
        break;
    }
    }
}

void phx::asset::YamlAssetWriter::WriteArray(YAML::Emitter& emitter, const reflect::FieldInfo& field_info, const void* vec_ptr) const
{
    const auto *vec = static_cast<const std::vector<uint8_t>*>(vec_ptr);
    const size_t count = vec->size() / field_info.element_size;
    const uint8_t *data = vec->data();

    // TODO: This seems like a bug if the array is not a nested type
    emitter << YAML::BeginSeq;
    for (size_t i = 0; i < count; ++i)
        WriteStruct(emitter, *field_info.nested_type, data + i * field_info.element_size);
    emitter << YAML::EndSeq;
}
#endif