#include "PhxData_pch.h"
#include "Reflection.h"

#include <PhxCore/VFS.h>

#include "yaml-cpp/yaml.h"

using namespace phx;
using namespace phx::rft;


namespace
{
    constexpr phx::StringHash FloatId = "float"_hash;
    constexpr phx::StringHash IntId = "int_32"_hash;
    constexpr phx::StringHash UIntId = "uint_32"_hash;
    constexpr phx::StringHash StringId = "std::string"_hash;
}


void phx::rft::SerializeToYAML(YAML::Emitter& out, const void* obj, const TypeInfo& type)
{
    out << YAML::BeginMap;
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
            out << *(const char*)ptr;
        }
        // Add more types...
    }
    out << YAML::EndMap;
}

bool phx::rft::SerializeToYAML(phx::IFileSystem* fs, const char* filename, const void* obj)
{
    TypeInfo typeInfo;
    YAML::Emitter out;

    SerializeToYAML(out, obj, typeInfo);

    const char* strData = out.c_str();
    fs->WriteFile(filename, Span(strData, strlen(strData)));

    return true;
}
