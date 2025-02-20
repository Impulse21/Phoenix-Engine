#include "PhxResource/PhxResource_pch.h"

#include "PhxCore/Base.h"
#include "ResourceManger.h"
#include "imgui.h"

using namespace phx;

static std::unique_ptr<PakFileFormat::Header> g_testHeader = {};

void ResourceManger::Initialize(std::filesystem::path const& resourcePath)
{
	ms_fileSytem = phx::FileSystemFactory::CreateResourceFileSystem();

	ms_fileSytem->Mount("res:/", resourcePath);
}

void ResourceManger::RegisterPakFiles(Span<std::filesystem::path> pakFiles)
{
	for (auto& pakFile : pakFiles)
	{
		PHX_CORE_INFO("Registering PakFile {0}.", pakFile.generic_string().c_str());
		RegisterPakFile(pakFile);
	}
}
void phx::ResourceManger::RegisterPakFile(std::filesystem::path const& pakFile)
{
	// Get filename of pak file

	std::filesystem::path const& pakDirectoryAlias = pakFile.parent_path() / pakFile.stem();
	StringHash pakFileId(pakDirectoryAlias.generic_string());

	auto itr = ms_pakLut.find(pakFileId);
	if (itr != ms_pakLut.end())
		return;

	RefCountPtr<PakFile> pakfile = ms_pakFileHandler.Load(pakFile, ms_fileSytem);
	if (pakfile)
	{
		ms_pakLut[pakFileId] = ms_registeredPaks.size();
		ms_registeredPaks.push_back(pakfile);
	}
}

void phx::ResourceManger::DrawGui()
{
	ImGui::Begin("Pak File Manager");
	for (const auto& pakFile : ms_registeredPaks)
	{
		if (ImGui::TreeNode(pakFile->GetFilename().c_str()))
		{
			const bool isLoaded = pakFile->IsLoaded();
			ImGui::Text("Status: %s", isLoaded ? "Loaded" : "Unloaded");
			if (ImGui::TreeNode("Entries"))
			{
				if (isLoaded)
				{
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
				ImGui::TreePop();
			}

			ImGui::TreePop();
		}
	}

	ImGui::End();
}


RefCountPtr<IResource> ResourceManger::Get(const char* name, const char* ext)
{
	StringHash filenameHash(std::format("{}.{}", name, ext));

	std::scoped_lock _(ms_mutex);
	auto itr = ms_cache.find(filenameHash);
	if (itr != ms_cache.end())
		return itr->second;

	return nullptr;;
}