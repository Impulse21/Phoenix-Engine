#include "PhxWorld/PhxWorld_pch.h"

#include "WorldSerializer.h"
#include "World.h"

#include <PhxCore/VFS.h>

#include <yaml-cpp/yaml.h>

#include <DirectXMath.h>

using namespace phx;
using namespace phx::WorldSerializer;


namespace YAML
{
    template<>
    struct convert<DirectX::XMFLOAT2>
    {
        static Node encode(const DirectX::XMFLOAT2& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, DirectX::XMFLOAT2& rhs)
        {
            if (!node.IsSequence() || node.size() != 2)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            return true;
        }
    };

    template<>
    struct convert<DirectX::XMFLOAT3>
    {
        static Node encode(const DirectX::XMFLOAT3& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, DirectX::XMFLOAT3& rhs)
        {
            if (!node.IsSequence() || node.size() != 3)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            return true;
        }
    };

    template<>
    struct convert<DirectX::XMFLOAT4>
    {
        static Node encode(const DirectX::XMFLOAT4& rhs)
        {
            Node node;
            node.push_back(rhs.x);
            node.push_back(rhs.y);
            node.push_back(rhs.z);
            node.push_back(rhs.w);
            node.SetStyle(EmitterStyle::Flow);
            return node;
        }

        static bool decode(const Node& node, DirectX::XMFLOAT4& rhs)
        {
            if (!node.IsSequence() || node.size() != 4)
                return false;

            rhs.x = node[0].as<float>();
            rhs.y = node[1].as<float>();
            rhs.z = node[2].as<float>();
            rhs.w = node[3].as<float>();
            return true;
        }
    };

    template<>
    struct convert<phx::UUID>
    {
        static Node encode(const phx::UUID& uuid)
        {
            Node node;
            node.push_back((uint64_t)uuid);
            return node;
        }

        static bool decode(const Node& node, phx::UUID& uuid)
        {
            uuid = node.as<uint64_t>();
            return true;
        }
    };

    inline YAML::Emitter& operator<<(YAML::Emitter& out, phx::UUID const& uuid)
    {
        out << (uint64_t)uuid;
        return out;
    }

    inline YAML::Emitter& operator<<(YAML::Emitter& out, DirectX::XMFLOAT2 const& v)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
        return out;
    }

    inline YAML::Emitter& operator<<(YAML::Emitter& out, DirectX::XMFLOAT3 const& v)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
        return out;
    }

    inline YAML::Emitter& operator<<(YAML::Emitter& out, DirectX::XMFLOAT4 const& v)
    {
        out << YAML::Flow;
        out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
        return out;
    }

}


namespace
{

}

bool Save(IFileSystem* fs, const char* filename, World& world)
{
    return false;
}

data::RefPtr<World> WorldSerializer::Load(IFileSystem* fs, const char* filename)
{
    return data::RefPtr<World>();
}
