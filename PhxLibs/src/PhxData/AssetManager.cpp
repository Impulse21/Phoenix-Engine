#include <PhxData/PhxData_pch.h>

#include <PhxData/AssetManager.h>

#include "AssetManager.h"


#include <PhxData/IVirtualFileSystem.h>
#include <PhxData/IStreamingManager.h>

using namespace phx;
using namespace phx::data;

void AssetManager::Initialize(IVirtualFileSystem* vfs, IStreamingManager* loader)
{
	m_vfs = vfs;
	m_loader = loader;
}

void phx::data::AssetManager::Shutdown()
{
	std::scoped_lock _(m_cacheMutex);
	m_cache.clear();
}
