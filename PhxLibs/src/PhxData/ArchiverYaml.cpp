#include "PhxData_pch.h"
#include "ArchiverYaml.h"

#if false

using namespace phx::data;

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
#endif