#include "PhxAsset_pch.h"

#include <PhxCore/Reflect/TypeInfo.h>

#include <PhxAsset/YamlAssetWriter.h>

#include <yaml-cpp/yaml.h>
#include "YamlAssetWriter.h"

using namespace phx;
using namespace phx::asset;

YamlAssetWriter::YamlAssetWriter(IVirtualFileSystem *vfs)
    : m_vfs(vfs)
{
}

bool phx::asset::YamlAssetWriter::Write(std::string_view path, const reflect::TypeInfo &type_info, const void *asset)
{
    phx::Result<std::string> physical_path_result = m_vfs->ResolveVirtualToPhysicalPath(path);

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

void phx::asset::YamlAssetWriter::WriteStruct(YAML::Emitter &emitter, const reflect::TypeInfo &type_info, const void *asset)
{
    out << YAML::BeginMap;
    for (const auto& field_info : type_info.fields)
    {
        const void* field_ptr = static_cast<const uint8_t*>(in) + field_info.offset;
        emitter << YAML::Key << std::string(field_info.name);
        emitter << YAML::Value;

        WriteField(emitter, field_info, field_ptr);
    }

    out << YAML::EndMap;
}

void phx::asset::YamlAssetWriter::WriteField(YAML::Emitter &emitter, const reflect::FieldInfo &field_info, const void *field_ptr)
{
    switch (field.kind)
    {
    case reflect::FieldKind::Bool:
        out << *static_cast<const bool *>(ptr);
        break;

    case reflect::FieldKind::Int32:
        out << *static_cast<const int32_t *>(ptr);
        break;

    case reflect::FieldKind::Int64:
        out << *static_cast<const int64_t *>(ptr);
        break;

    case reflect::FieldKind::Uint32:
        out << *static_cast<const uint32_t *>(ptr);
        break;

    case reflect::FieldKind::Uint64:
        out << *static_cast<const uint64_t *>(ptr);
        break;

    case reflect::FieldKind::Float:
        out << *static_cast<const float *>(ptr);
        break;

    case reflect::FieldKind::Double:
        out << *static_cast<const double *>(ptr);
        break;

    case reflect::FieldKind::String:
        out << *static_cast<const std::string *>(ptr);
        break;

    case reflect::FieldKind::Nested:
        WriteStruct(out, ptr, *field.nested_type);
        break;

    case reflect::FieldKind::Array:
        WriteArray(out, ptr, field);
        break;

    case reflect::FieldKind::AssetRef:
        // TODO:
        // out << static_cast<const AssetPtrBase *>(ptr)->GetPath();
        break;

    case reflect::FieldKind::Float2:
    {
        const auto* v = static_cast<const hlslpp::interop::float2*>(ptr);
        out << YAML::Flow << YAML::BeginSeq << v->x << v->y << YAML::EndSeq;
        break;
    }
    case reflect::FieldKind::Float3:
    {
        const auto* v = static_cast<const hlslpp::interop::float3*>(ptr);
        out << YAML::Flow << YAML::BeginSeq << v->x << v->y << v->z << YAML::EndSeq;
        break;
    }
    case reflect::FieldKind::Float4:
    {
        const auto* v = static_cast<const hlslpp::interop::float3*>(ptr);
        out << YAML::Flow << YAML::BeginSeq << v->x << v->y << v->z << v->z << YAML::EndSeq;
        break;
    }
    }
}

void phx::asset::YamlAssetWriter::WriteArray(std::ostream &out, const reflect::FieldInfo &field_info, const void *vec_ptr)
{
    const auto *vec = static_cast<const std::vector<uint8_t>*>(vec_ptr);
    const size_t count = vec->size() / field.element_size;
    const uint8_t *data = vec->data();

    // TODO: This seems like a bug if the array is not a nested type
    out << YAML::BeginSeq;
    for (size_t i = 0; i < count; ++i)
        WriteStruct(out, data + i * field.element_size, *field.nested_type);
    out << YAML::EndSeq;
}
