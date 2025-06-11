#include <PhxData/AssetManager.h>

#include "AssetManager.h"


#include <PhxCore/IO/FileUtils.h>
#include <PhxData/IVirtualFileSystem.h>
#include <PhxData/IAsyncIOSystem.h>

using namespace phx;
using namespace phx::data;

void AssetManager::Initialize(IVirtualFileSystem* vfs, IAsyncIOSystem* loader)
{
	m_vfs = vfs;
	m_loader = loader;
}

void phx::data::AssetManager::Shutdown()
{
	std::scoped_lock _(m_cacheMutex);
	m_cache.clear();
}

RefCountPtr<Asset> phx::data::AssetManager::Get(const char* virtual_file_path)
{
	StringHash filenameHash(virtual_file_path);

	{
		std::scoped_lock _(m_cacheMutex);
		auto itr = m_cache.find(filenameHash);
		if (itr != m_cache.end())
			return itr->second;
	}

	RefCountPtr<Asset> asset = nullptr;

	std::string ext = phx::GetFileExt(virtual_file_path);
	auto handlerItr = m_assetImporters.find(StringHash(ext));

	if (handlerItr == m_assetImporters.end())
	{
		PHX_CORE_ERROR("Unknown asset extension '{0}'", ext.c_str());
		return nullptr;
	}

	PHX_CORE_INFO(
		"Importing asset '{0}' from disk",
		virtual_file_path);

	asset = handlerItr->second->ImportAsync(m_vfs, m_loader, virtual_file_path);

	if (asset)
	{
		std::scoped_lock _(m_cacheMutex);
		m_cache[filenameHash] = asset;
	}

	return asset;
}
