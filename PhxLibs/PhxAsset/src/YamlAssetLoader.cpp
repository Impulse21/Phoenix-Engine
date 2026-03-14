#include "PhxAsset_pch.h"

#include <PhxCore/Reflect/TypeInfo.h>

#include <PhxAsset/AssetLoaders.h>
#include <PhxAsset/AssetDatabase.h>

#include <yaml-cpp/yaml.h>

using namespace phx;
using namespace phx::asset;

namespace
{
    struct VectorProxy
    {
        uint8_t *begin = nullptr;
        uint8_t *end = nullptr;
        uint8_t *end_cap = nullptr;

        void Reserve(size_t elem_size, size_t count)
        {
            const size_t current = size(elem_size);
            if (current + count <= capacity(elem_size))
                return;

            const size_t new_cap = current + count;
            uint8_t *new_data = static_cast<uint8_t *>(::operator new(new_cap * elem_size));

            if (begin)
            {
                std::memcpy(new_data, begin, current * elem_size);
                ::operator delete(begin);
            }

            begin = new_data;
            end = begin + current * elem_size;
            end_cap = begin + new_cap * elem_size;
        }

        void *PushBackUninitialized(size_t elem_size)
        {
            void *slot = end;
            end += elem_size;
            return slot;
        }

        size_t size(size_t elem_size) const { return (end - begin) / elem_size; }
        size_t capacity(size_t elem_size) const { return (end_cap - begin) / elem_size; }
    };
}

YamlAssetLoader::YamlAssetLoader(IVirtualFileSystem* vfs)
    : m_vfs(vfs)
{

}

bool YamlAssetLoader::Load(std::string_view path, const reflect::TypeInfo &type_info, void *out) const
{
    phx::Result<std::string> physical_path_result = m_vfs->ResolveVirtualToPhysicalPath(std::string(path));
    if (physical_path_result.HasError())
        return false;

    try
    {
        const YAML::Node root = YAML::LoadFile(physical_path_result.GetValue());
        ReadStruct(root, type_info, out);
    }
    catch (const YAML::Exception &e)
    {
        PHX_CORE_ERROR("YamlLoader: failed to parse {0}: {1}", path, e.what());
        return false;
    }

    return true;
}

bool YamlAssetLoader::Exists(std::string_view path) const
{
    return m_vfs->Exists(std::string(path));
}

void phx::asset::YamlAssetLoader::ReadStruct(const YAML::Node& yaml_node, const reflect::TypeInfo& type_info, void* out_ptr) const
{
    if(!yaml_node.IsMap())
    {
        PHX_CORE_ERROR("Attempting to parse a YAML node that is not a strucutre as one.");
        return;
    }

    for (auto& field_info : type_info.fields)
    {
        const YAML::Node& yaml_value_node = yaml_node[std::string(field_info.name)];
        if (!yaml_value_node.IsDefined())
            continue;;

        uint8_t* field_ptr = static_cast<uint8_t*>(out_ptr) + field_info.offset;

        ReadField(yaml_value_node, field_info, field_ptr);
    }
}

void phx::asset::YamlAssetLoader::ReadField(const YAML::Node& yaml_node, const reflect::FieldInfo& field_info, void* out_ptr) const
{
    switch (field_info.kind)
    {
    case reflect::FieldKind::Bool:
        *static_cast<bool*>(out_ptr) = yaml_node.as<bool>();
        break;

    case reflect::FieldKind::Int32:
        *static_cast<int32_t*>(out_ptr) = yaml_node.as<int32_t>();
        break;

    case reflect::FieldKind::Int64:
        *static_cast<int64_t*>(out_ptr) = yaml_node.as<int64_t>();
        break;

    case reflect::FieldKind::Uint32:
        *static_cast<uint32_t*>(out_ptr) = yaml_node.as<uint32_t>();
        break;

    case reflect::FieldKind::Uint64:
        *static_cast<uint64_t*>(out_ptr) = yaml_node.as<uint64_t>();
        break;

    case reflect::FieldKind::Float:
        *static_cast<float*>(out_ptr) = yaml_node.as<float>();
        break;

    case reflect::FieldKind::Double:
        *static_cast<double*>(out_ptr) = yaml_node.as<double>();
        break;

    case reflect::FieldKind::String:
        *static_cast<std::string*>(out_ptr) = yaml_node.as<std::string>();
        break;

    case reflect::FieldKind::Nested:
        if (field_info.nested_type)
            ReadStruct(yaml_node, *field_info.nested_type, out_ptr);
        break;

    case reflect::FieldKind::Array:
        if (field_info.nested_type)
            ReadArray(yaml_node, field_info, out_ptr);
        break;

    case reflect::FieldKind::AssetPtr:
        ReadAssetPtr(yaml_node, field_info, out_ptr);
        break;

    case reflect::FieldKind::Float2:
        ReadFloatN(yaml_node, 2, out_ptr);
        break;

    case reflect::FieldKind::Float3:
        ReadFloatN(yaml_node, 3, out_ptr);
        break;

    case reflect::FieldKind::Float4:
        ReadFloatN(yaml_node, 4, out_ptr);
        break;
    }
}

void YamlAssetLoader::ReadAssetPtr(const YAML::Node& yaml_node, const reflect::FieldInfo& /*field_info*/, void* /*out_ptr*/) const
{
        if (!yaml_node.IsScalar())
            return;
 
        // TODO:
        PHX_ASSERT(false, "NOT IMPLEMENTED YET - needs to be thought out.")
}

void YamlAssetLoader::ReadArray(const YAML::Node &yaml_node, const reflect::FieldInfo &field_info, void *out_ptr) const
{
    if (!yaml_node.IsSequence())
        return;

    const reflect::TypeInfo &elem_type_info = *field_info.nested_type;
    const size_t elem_size = field_info.element_size;

    auto *vec = static_cast<VectorProxy*>(out_ptr);

    const size_t count = yaml_node.size();
    vec->Reserve(elem_size, count);

    for (const YAML::Node &elem_node : yaml_node)
    {
        // Grow the vector by one element
        void *elem_ptr = vec->PushBackUninitialized(elem_size);

        // Default-construct the element in place
        if (elem_type_info.construct_place)
            elem_type_info.construct_place(elem_ptr);

        ReadStruct(elem_node, elem_type_info, elem_ptr);
    }
}

void YamlAssetLoader::ReadFloatN(const YAML::Node &yaml_node, int n, void *out_ptr) const
{
    float* out_ptr_float = static_cast<float*>(out_ptr);

    if (yaml_node.IsSequence())
    {
        int i = 0;
        for (const auto& elem : yaml_node)
        {
            if (i >= n)
                break;

            out_ptr_float[i++] = elem.as<float>();
        }
    }
    else if (yaml_node.IsScalar())
    {
        std::istringstream ss(yaml_node.as<std::string>());
        for (int i = 0; i < n; ++i)
        {
            ss >> out_ptr_float[i];
        }
    }
}