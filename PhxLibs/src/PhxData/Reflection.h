#pragma once

#include <PhxCore/StringHash.h>
#include <PhxCore/Span.h>
#include <DirectXMath.h>
#include <initializer_list>

#include <cstddef> // For std::offsetof

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

    struct FieldInfo
    {
        const char* Name;
        const char* Type;
        StringHash TypeHash;
        const char* Tooltip = nullptr;
        size_t Offset;                     // Byte offset within the struct
        std::initializer_list<ExtraInfo> Extras;
    };

    template<typename T>
    struct TypeInfo
    {
        static constexpr phx::Span<const FieldInfo> GetFields();
        static constexpr const char* GetTypeName();
        static constexpr phx::StringHash GetTypeNameHash();
    };

    template<>
    struct TypeInfo<DirectX::XMFLOAT2>
    {
        static constexpr FieldInfo Fields[] = {
            { "X", "float", "float"_hash, "X Component", offsetof(DirectX::XMFLOAT2, x), std::initializer_list<ExtraInfo>{} },
            { "Y", "float", "float"_hash, "Y Component", offsetof(DirectX::XMFLOAT2, y), std::initializer_list<ExtraInfo>{} },
        };

        static constexpr phx::Span<const FieldInfo> GetFields() { return Fields; }
        static constexpr const char* GetTypeName() { return "XMFLOAT2"; }
        static constexpr phx::StringHash GetTypeNameHash() { return "XMFLOAT2"_hash; }
    };


    template<>
    struct TypeInfo<DirectX::XMFLOAT3>
    {
        static constexpr FieldInfo Fields[] = {
            { "X", "float", "float"_hash, "X Component", offsetof(DirectX::XMFLOAT3, x), std::initializer_list<ExtraInfo>{} },
            { "Y", "float", "float"_hash, "Y Component", offsetof(DirectX::XMFLOAT3, y), std::initializer_list<ExtraInfo>{} },
            { "Z", "float", "float"_hash, "Z Component", offsetof(DirectX::XMFLOAT3, z), std::initializer_list<ExtraInfo>{} },
        };

        static constexpr phx::Span<const FieldInfo> GetFields() { return Fields; }
        static constexpr const char* GetTypeName() { return "XMFLOAT3"; }
        static constexpr phx::StringHash GetTypeNameHash() { return "XMFLOAT3"_hash; }
    };


    template<>
    struct TypeInfo<DirectX::XMFLOAT4>
    {
        static constexpr FieldInfo Fields[] = {
            { "X", "float", "float"_hash, "X Component", offsetof(DirectX::XMFLOAT4, x), std::initializer_list<ExtraInfo>{} },
            { "Y", "float", "float"_hash, "Y Component", offsetof(DirectX::XMFLOAT4, y), std::initializer_list<ExtraInfo>{} },
            { "Z", "float", "float"_hash, "Z Component", offsetof(DirectX::XMFLOAT4, z), std::initializer_list<ExtraInfo>{} },
            { "W", "float", "float"_hash, "W Component", offsetof(DirectX::XMFLOAT4, w), std::initializer_list<ExtraInfo>{} },
        };

        static constexpr phx::Span<const FieldInfo> GetFields() { return Fields; }
        static constexpr const char* GetTypeName() { return "XMFLOAT4"; }
        static constexpr phx::StringHash GetTypeNameHash() { return "XMFLOAT4"_hash; }
    };

    template<typename T>
    bool Serialize(IFileSystem* fs, TypeInfo<T>& typeInfo);
    template<typename T>
    bool rft::Serialize(IFileSystem* fs, TypeInfo<T>& typeInfo)
    {

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "World" << YAML::Value << "Untitled";
        out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

        for (auto& entityId : world.GetRegistry().view<entt::entity>())
        {
            Entity entity(entityId, &world);
            if (!entity)
                continue;

            Yaml::SerializeEntity(out, entity, world);
        };

        out << YAML::EndSeq;
        out << YAML::EndMap;
        const char* strData = out.c_str();
        fs->WriteFile(filename, Span(strData, strlen(strData)));

        return true;
    }
}