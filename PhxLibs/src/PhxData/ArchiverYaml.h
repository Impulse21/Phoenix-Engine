#pragma once

#include <PhxCore/VFS.h>
#include "Archiver.h"
#include <yaml-cpp/yaml.h>

namespace phx::data
{
#if true
    class YamlArchiver : public IArchiver
#else
    class YamlArchiver : public ArchiverMixin<YamlArchiver>
#endif
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

#if false
        YamlArchiver(YAML::Emitter& outEmitter)
            : m_mode(ArchiveMode::Write)
            , m_emitter(&outEmitter)
            , m_node(nullptr)
        {
            m_emitter->SetIndent(4);
            m_emitter->SetMapFormat(YAML::Block);
            *m_emitter << YAML::BeginMap;
        }
#endif

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

        void Write(const char* key, const int32_t& value) override;
        void Write(const char* key, const uint32_t& value) override;
        void Write(const char* key, const float& value) override;
        void Write(const char* key, const std::string& value) override;
        void Write(const char* key, const bool& value) override;
        void Write(const char* key, const phx::UUID& value) override;
        void Write(const char* key, const DirectX::XMFLOAT2& value) override;
        void Write(const char* key, const DirectX::XMFLOAT3& value) override;
        void Write(const char* key, const DirectX::XMFLOAT4& value) override;


       void Write(const char* key, const phx::Span<int32_t> value) override;
       void Write(const char* key, const phx::Span<uint32_t> value) override;
       void Write(const char* key, const phx::Span<float> value) override;
       void Write(const char* key, const phx::Span<std::string> value) override;
       void Write(const char* key, const phx::Span<bool> value) override;
       void Write(const char* key, const phx::Span<DirectX::XMFLOAT2> value) override;
       void Write(const char* key, const phx::Span<DirectX::XMFLOAT3> value) override;
       void Write(const char* key, const phx::Span<DirectX::XMFLOAT4> value) override;
        // virtual void Write(const std::string& key, phx::Span<IDataObj> value) override;

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