#pragma once

#include <PhxCore/StringHash.h>
#include <PhxCore/Span.h>
#include <DirectXMath.h>
#include <initializer_list>

#include <cstddef> // For std::offsetof

namespace YAML
{
    class Emitter;
}

namespace phx
{
    class IFileSystem;
}

namespace phx::rft
{
	struct ExtraInfo
	{
		const char* Key;
		const char* Value;
	};

    struct TypeInfo;
    struct FieldInfo
    {
        const char* Name;
        StringHash TypeHash;
        const char* Tooltip = nullptr;
        size_t Offset;                     // Byte offset within the struct
        TypeInfo* NestedType = nullptr;
        std::initializer_list<ExtraInfo> Extras;
    };

    struct TypeInfo
    {
        const char* TypeName;
        Span<FieldInfo> Fields;

        phx::Span<FieldInfo> GetFields() const { return Fields; }
    };

    extern const std::unordered_map<std::string, const TypeInfo*> g_TypeRegistry;

    bool SerializeToYAML(phx::IFileSystem* fs, const char* filename, const void* obj);
    void SerializeToYAML(YAML::Emitter& out, const void* obj, const TypeInfo& type);
}