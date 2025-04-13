#pragma once

#include "Archiver.h"
#include <yaml-cpp/yaml.h>

namespace phx::data
{
    class YamlArchiver : public IArchiver
    {
    public:
        // Constructor for writing
        YamlArchiver(YAML::Emitter& outEmitter)
            : m_mode(ArchiveMode::Write)
            , m_emitter(&outEmitter)
            , m_node(nullptr)
        {
            m_emitter->SetIndent(4);
            m_emitter->SetMapFormat(YAML::Block);
            *m_emitter << YAML::BeginMap;
        }

        // Constructor for reading
        YamlArchiver(const YAML::Node& inNode)
            : m_mode(ArchiveMode::Read)
            , m_emitter(nullptr)
            , m_node(&inNode)
        {
        }

        ~YamlArchiver()
        {
            if (m_mode == ArchiveMode::Write && m_emitter)
            {
                *m_emitter << YAML::EndMap;
            }
        }

        ArchiveMode GetMode() const override { return m_mode; }

    protected:

        void Write(const char* key, const int32_t& value) override;
        void Write(const char* key, const uint32_t& value) override;
        void Write(const char* key, const float& value) override;
        void Write(const char* key, const std::string& value) override;
        void Write(const char* key, const bool& value) override;
        void Write(const char* key, const phx::UUID& value) override;
        void Write(const char* key, const DirectX::XMFLOAT2& value) override;
        void Write(const char* key, const DirectX::XMFLOAT3& value) override;
        void Write(const char* key, const DirectX::XMFLOAT4& value) override;

        void Read(const char* key, int32_t& value) override;
        void Read(const char* key, uint32_t& value) override;
        void Read(const char* key, float& value) override;
        void Read(const char* key, bool& value) override;
        void Read(const char* key, phx::UUID& value) override;
        void Read(const char* key, DirectX::XMFLOAT2& value) override;
        void Read(const char* key, DirectX::XMFLOAT3& value) override;
        void Read(const char* key, DirectX::XMFLOAT4& value) override;


    private:
        ArchiveMode m_mode;
        YAML::Emitter* m_emitter;
        const YAML::Node* m_node;
    };
}