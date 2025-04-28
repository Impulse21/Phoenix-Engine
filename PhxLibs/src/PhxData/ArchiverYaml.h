#pragma once

#if false
#include <PhxCore/VFS.h>
#include "Archiver.h"
#include <yaml-cpp/yaml.h>
#include <DirectXMath.h>

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

namespace phx::data
{
    class YamlArchiver final : public IArchiver
    {
    public:
        // Constructor for writing
        YamlArchiver(IFileSystem* fs, std::string const& filename)
            : m_mode(ArchiveMode::Write)
            , m_node(nullptr)
            , m_fs(fs)
            , m_filename(filename)
        {

            m_emitter.SetIndent(4);
            m_emitter.SetMapFormat(YAML::Block);
            m_emitter << YAML::BeginMap;
        }

        // Constructor for reading
        YamlArchiver(const YAML::Node& inNode)
            : m_mode(ArchiveMode::Read)
            , m_node(&inNode)
        {
        }

        ~YamlArchiver()
        {
            Save();
        }


        ArchiveMode GetMode() const override { return m_mode; }

    protected:

        void WriteNull() override { m_emitter << YAML::Null; }
        void BeginArrayWrite() override { m_emitter << YAML::BeginSeq; }
        void EndArrayWrite() override { m_emitter << YAML::EndSeq; }

        void BeginMap() override { m_emitter << YAML::BeginMap; };
        void EndMap() override { m_emitter << YAML::EndMap; };

        void WriteKey(const char* key) override { m_emitter << YAML::Key << key; }

        void Write(const int32_t& value) override { m_emitter << YAML::Value << value; }
        void Write(const uint32_t& value) override { m_emitter << YAML::Value << value; }
        void Write(const float& value) override { m_emitter << YAML::Value << value; }
        void Write(const std::string& value) override { m_emitter << YAML::Value << value; }
        void Write(const bool& value) override { m_emitter << YAML::Value << value; }
        void Write(const phx::UUID& value) override { m_emitter << YAML::Value << value; }
        void Write(const DirectX::XMFLOAT2& value) override { m_emitter << YAML::Value << value; }
        void Write(const DirectX::XMFLOAT3& value) override { m_emitter << YAML::Value << value; }
        void Write(const DirectX::XMFLOAT4& value) override { m_emitter << YAML::Value << value; }

        void Read(const char* key, int32_t& value) override;
        void Read(const char* key, uint32_t& value) override;
        void Read(const char* key, float& value) override;
        void Read(const char* key, bool& value) override;
        void Read(const char* key, phx::UUID& value) override;
        void Read(const char* key, DirectX::XMFLOAT2& value) override;
        void Read(const char* key, DirectX::XMFLOAT3& value) override;
        void Read(const char* key, DirectX::XMFLOAT4& value) override;

        void Save() override
        {
            if (m_mode == ArchiveMode::Write && m_fs)
            {
                m_emitter << YAML::EndMap;

                const char* strData = m_emitter.c_str();
                m_fs->WriteFile(m_filename, Span(strData, strlen(strData)));
            }
        }

    private:
        phx::IFileSystem* m_fs;
        std::string m_filename;
        YAML::Emitter m_emitter;
        ArchiveMode m_mode;
        const YAML::Node* m_node;
    };
}
#endif