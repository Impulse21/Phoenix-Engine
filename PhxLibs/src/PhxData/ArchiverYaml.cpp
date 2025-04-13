#include "PhxData_pch.h"
#include "ArchiverYaml.h"

#include <DirectXMath.h>

using namespace phx::data;


YAML::Emitter& operator<<(YAML::Emitter& out, phx::UUID const& uuid)
{
    out << (uint64_t)uuid;
    return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, DirectX::XMFLOAT2 const& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
    return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, DirectX::XMFLOAT3 const& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, DirectX::XMFLOAT4 const& v)
{
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
    return out;
}

namespace
{
    template<typename T>
    void ReadNode(const YAML::Node* node, const char* key, T& value)
    {
        const YAML::Node& val = (*node)[key];
        if (val)
        {
            value = val.as<int>();
        }
    }

    template<typename T>
    void WriteSequence(YAML::Emitter& emitter, const char* key, phx::Span<T> span)
    {
        emitter << YAML::Key << key << YAML::Value << YAML::BeginSeq;
        for (const auto& item : span)
        {
            emitter << YAML::BeginMap;
            emitter << const_cast<T&>(item);
            emitter << YAML::EndMap;
        }
        emitter << YAML::EndSeq;
    }
}

void YamlArchiver::Write(const char* key, const int32_t& value)
{
    m_emitter << YAML::Key << key << YAML::Value << value;
}

void YamlArchiver::Write(const char* key, const uint32_t& value)
{
    m_emitter << YAML::Key << key << YAML::Value << value;
}

void YamlArchiver::Write(const char* key, const float& value)
{
    m_emitter << YAML::Key << key << YAML::Value << value;
}

void YamlArchiver::Write(const char* key, const std::string& value)
{
    m_emitter << YAML::Key << key << YAML::Value << value;
}

void YamlArchiver::Write(const char* key, const bool& value)
{
    m_emitter << YAML::Key << key << YAML::Value << value;
}

void YamlArchiver::Write(const char* key, const phx::UUID& value)
{
    m_emitter << YAML::Key << key << YAML::Value << value;
}

void YamlArchiver::Write(const char* key, const DirectX::XMFLOAT2& value)
{
    m_emitter << YAML::Key << key << YAML::Value << value;
}

void YamlArchiver::Write(const char* key, const DirectX::XMFLOAT3& value)
{
    m_emitter << YAML::Key << key << YAML::Value << value;
}

void YamlArchiver::Write(const char* key, const DirectX::XMFLOAT4& value)
{
    m_emitter << YAML::Key << key << YAML::Value << value;
}


void YamlArchiver::Write(const char* key, const phx::Span<int32_t> value)
{
    WriteSequence(m_emitter, key, value);
};

void YamlArchiver::Write(const char* key, const phx::Span<uint32_t> value)
{
    WriteSequence(m_emitter, key, value);
};

void YamlArchiver::Write(const char* key, const phx::Span<float> value)
{
    WriteSequence(m_emitter, key, value);
};

void YamlArchiver::Write(const char* key, const phx::Span<std::string> value)
{
    WriteSequence(m_emitter, key, value);
};

void YamlArchiver::Write(const char* key, const phx::Span<bool> value)
{
    WriteSequence(m_emitter, key, value);
};

void YamlArchiver::Write(const char* key, const phx::Span<DirectX::XMFLOAT2> value)
{
    WriteSequence(m_emitter, key, value);
};

void YamlArchiver::Write(const char* key, const phx::Span<DirectX::XMFLOAT3> value)
{
    WriteSequence(m_emitter, key, value);
};

void YamlArchiver::Write(const char* key, const phx::Span<DirectX::XMFLOAT4> value)
{
    WriteSequence(m_emitter, key, value);
};


void YamlArchiver::Read(const char* key, int32_t& value)
{
    ReadNode(m_node, key, value);
}

void YamlArchiver::Read(const char* key, uint32_t& value)
{
    ReadNode(m_node, key, value);
}

void YamlArchiver::Read(const char* key, float& value)
{
    ReadNode(m_node, key, value);
}

void YamlArchiver::Read(const char* key, bool& value)
{
    ReadNode(m_node, key, value);
}

void YamlArchiver::Read(const char* key, phx::UUID& value)
{
    ReadNode(m_node, key, value);
}

void YamlArchiver::Read(const char* /*key*/, DirectX::XMFLOAT2& /*value*/ )
{
}

void YamlArchiver::Read(const char* /*key*/, DirectX::XMFLOAT3& /*value*/)
{
}

void YamlArchiver::Read(const char* /*key*/, DirectX::XMFLOAT4& /*value*/)
{
}
