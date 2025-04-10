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

namespace phx::data
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
        std::initializer_list<ExtraInfo> Extras;
        bool IsPointer : 1 = false; // New flag to indicate pointer types
    };

    struct TypeInfo
    {
        const char* TypeName;
        Span<FieldInfo> Fields;

        phx::Span<FieldInfo> GetFields() const { return Fields; }
    };

    template<typename T>
    struct Reflection
    {
        static const TypeInfo& GetTypeInfo();
    };

    extern const std::unordered_map<std::string, const TypeInfo*> g_TypeRegistry;

    template<typename T>
    bool SerializeToYAML(phx::IFileSystem* fs, const char* filename, T const& obj)
    {
        const TypeInfo& typeInfo = Refelction<T>::GetTypeInfo();

        return SerializeToYAML(fs, filename, typeInfo, &obj);
    }

    template<typename T>
    void SerializeToYAML(YAML::Emitter& out, const void* obj)
    {
        const TypeInfo& typeInfo = Refelction<T>::GetTypeInfo();
        SerializeToYAML(out, obj, typeInfo);
    }


    bool SerializeToYAML(phx::IFileSystem* fs, const char* filename, TypeInfo const& typeInfo, const void* obj);
    void SerializeToYAML(YAML::Emitter& out, const void* obj, const TypeInfo& type);
}