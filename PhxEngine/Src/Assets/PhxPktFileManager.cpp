#include <PhxEngine/Assets/PhxPktFileManager.h>
#include <PhxEngine/Assets/PhxArchFile.h>
#include <PhxEngine/Assets/AssetPktFileFactory.h>

#include <imgui.h>

#include <filesystem>

using namespace PhxEngine;
using namespace PhxEngine::Assets;

void PhxEngine::Assets::PhxPktFileManager::Initialize(std::unique_ptr<IPktFileFactory>&& factory)
{
    m_pktFileFactory = std::move(factory);
}

void PhxEngine::Assets::PhxPktFileManager::Finalize()
{
    m_pktFileFactory.reset();
}

void PhxEngine::Assets::PhxPktFileManager::Update()
{
}

void PhxEngine::Assets::PhxPktFileManager::RenderDebugWindow()
{
	ImGui::Begin("Pkt Files");
	// Draw a table.
    const int numColumns = 3;
    if (ImGui::BeginTable("Files", 3))
    {
        for (int row = 0; row < m_files.size(); row++)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            std::filesystem::path f(m_files[row].Filename);
            ImGui::Text(f.stem().generic_string().c_str());

            ImGui::TableSetColumnIndex(1);

            // Data Is Loaded
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
            ImGui::Text("UNLOADED");
            ImGui::PopStyleColor();
        }
        ImGui::EndTable();
    }

	ImGui::End();
}

FileId PhxEngine::Assets::PhxPktFileManager::AddFile(std::string const& filename)
{
    File f;

    f.Filename = filename;
    f.PktFile = m_pktFileFactory->Create(filename);
    f.PktFile->StartMetadataLoad();

    auto id = m_files.size();
    m_files.push_back(std::move(f));
	return FileId();
}
