#include <PhxResource/PhxResource_pch.h>
#include "ResourceSystem.h"

#include "IResource.h"

using namespace phx;

void phx::ResourceSystem::Initialize(data::IVirtualFileSystem* vfs)
{
	m_vfs = vfs;
}

void phx::ResourceSystem::Shutdown()
{
}

RefCountPtr<Resource> phx::ResourceSystem::Get(const char* path)
{
	// TODO: Clean up all these allocations and use simpler string functions
	// that just strip out data a single string.
	// way to many allocations probably make this code really really slow.
	StringHash filenameHash(path);

	{
		std::scoped_lock _(m_cacheMutex);
		auto itr = m_cache.find(filenameHash);
		if (itr != m_cache.end())
			return itr->second;
	}

	RefCountPtr<Resource> resource = nullptr;

	PHX_CORE_ASSERT(false, "TODO");
	//std::string ext = FileSystem::GetFileExt(path);
	std::string ext;
	auto handlerItr = m_resourceHandlers.find(StringHash(ext));

	if (handlerItr == m_resourceHandlers.end())
	{
		PHX_CORE_ERROR("Unknown resource extension '{0}'", ext.c_str());
		return nullptr;
	}
	// Check Pak logic
#if false 
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
			PHX_CORE_INFO(
				"Loading Resource '{0}' from Pak file '{1}'",
				filename.c_str(),
				pakFile->GetFilename().c_str());

			resource = handlerItr->second->LoadFromPak(ms_assetStreamer, pakFile->GetFileHandle(), *entry);
		}
	}
#endif

	if (!resource)
	{
		PHX_CORE_INFO(
			"Loading Resource '{0}' from disk",
			path);

#if false
		auto resolvedPath = IRootFileSystem::Ptr->ResolvePath(path);
		FileHandle fileHandle = m_fs->OpenFile(resolvedPath, FileAccessMode::Read);
#else
		PHX_CORE_ASSERT(false, "TODO");
#endif
		resource = handlerItr->second->LoadLoose(m_vfs);
	}

	if (resource)
	{
		std::scoped_lock _(m_cacheMutex);
		m_cache[filenameHash] = resource;
	}

	return resource;
}
