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
		RegisterPakFile(pakFile);
	}
}

RefCountPtr<PakFile> phx::ResourceManger::RegisterPakFile(std::filesystem::path const& pakFile)
{
	// Get filename of pak file
	PHX_CORE_INFO("Registering PakFile {0}.", pakFile.generic_string().c_str());
	std::filesystem::path const& pakDirectoryAlias = pakFile.parent_path() / pakFile.stem();
	StringHash pakFileId(pakDirectoryAlias.generic_string());

	auto itr = ms_pakLut.find(pakFileId);
	if (itr != ms_pakLut.end())
		return ms_registeredPaks[itr->second];

	RefCountPtr<PakFile> pakfile = ms_pakFileHandler.Load(pakFile, ms_fileSytem);
	if (pakfile)
	{
		ms_pakLut[pakFileId] = ms_registeredPaks.size();
		ms_registeredPaks.push_back(pakfile);
	}

	return pakfile;
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


RefCountPtr<IResource> ResourceManger::Get(std::filesystem::path const& path)
{
	// TODO: Clean up all these allocations and use simpler string functions
	// that just strip out data a single string.
	// way to many allocations probably make this code really really slow.

	StringHash filenameHash(path.generic_string());

	{
		std::scoped_lock _(ms_cacheMutex);
		auto itr = ms_cache.find(filenameHash);
		if (itr != ms_cache.end())
			return itr->second;
	}

	RefCountPtr<IResource> resource = nullptr;

	std::string ext = path.extension().generic_string();
	auto handlerItr = ms_resourceHandlers.find(StringHash(ext));

	if (handlerItr == ms_resourceHandlers.end())
	{
		PHX_CORE_ERROR("Unknown resource extension '{0}'", ext.c_str());
		return nullptr;
	}

	// Check if file is in a pak
	StringHash directoryId(path.parent_path().generic_string());
	auto pakItr = ms_pakLut.find(directoryId);
	if (pakItr != ms_pakLut.end())
	{
		RefCountPtr<PakFile> pakFile = ms_registeredPaks[pakItr->second];
		if (!pakFile->IsLoaded())
		{
			PHX_CORE_WARN("Pak File isn't loaded yet.");
			return resource;

		}

		const PakFileFormat::AssetEntry* entry = pakFile->FindEntryByHash(StringHash(path.filename().generic_string()));
		if (entry)
		{
			resource = handlerItr->second->Load(path, ms_fileSytem, pakFile->GetFileHandle(), entry->Offset);
		}
	}

	if (!resource)
	{
		PHX_CORE_ERROR("Loding from disk is not currently supported");
	}

	if (resource)
	{
		std::scoped_lock _(ms_cacheMutex);
		ms_cache[filenameHash] = resource;
	}

	return resource;
}