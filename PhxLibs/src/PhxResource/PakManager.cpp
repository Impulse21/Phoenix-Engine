#include "PhxRenderer/PhxRenderer_pch.h"
#include "PakManager.h"

#include <imgui.h>
#include <PhxCore/StringUtils.h>

void phx::PakManager::Mount(std::string const& filename)
{
	std::wstring wide;
	StringConvert(filename, wide);
	ms_mountedPaks.push_back(std::make_unique<PakFile>(wide.c_str()));
}

void phx::PakManager::DrawGui()
{
	ImGui::Begin("Pak File Manager");
	for (auto& pakFile : ms_mountedPaks)
	{
		const uint8_t status = pakFile->GetStatus();
		switch (status)
		{
		case PakStatus::Unloaded:
			ImGui::Text("Unloaded");
			break;
		case PakStatus::LoadingHeader:
			ImGui::Text("Loaded header...");
			break;
		case PakStatus::LoadingAssetHeaders:
			ImGui::Text("Loaded asset headers...");
			break;

		case PakStatus::Loaded:
		default:
			for (const PakFileFormat::AssetEntry& entry : pakFile->GetEntries())
			{
				char buffer[9]; // 8 characters + null terminator
				std::snprintf(buffer, sizeof(buffer), "%08X", entry.Hash);

				const char* fileName = pakFile->FindFilenameByHash(entry.Hash);
				if (ImGui::TreeNode(fileName ? fileName : buffer))
				{
					ImGui::Text("ID: %s", buffer);
					ImGui::Text("Uncompressed Size: %d", entry.Size);
					ImGui::Text("Offset: %lld", entry.Offset);
					ImGui::Text("Num Dependecies: %d", entry.NumDependiences);

					ImGui::TreePop();
				}
			}
		}
	}

	ImGui::End();
}
