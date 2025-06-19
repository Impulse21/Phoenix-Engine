#include <PhxResource/PhxResource_pch.h>
#include "ResourceSystem.h"

#include "Resource.h"
#include <PhxCore/IO/FileUtils.h>
#include <PhxData/IVirtualFileSystem.h>
#include <PhxData/IStreamingManager.h>

using namespace phx;

void phx::ResourceSystem::Initialize(data::IVirtualFileSystem* vfs, data::IStreamingManager* loader)
{
	m_vfs = vfs;
	m_loader = loader;
}

void phx::ResourceSystem::Shutdown()
{
	std::scoped_lock _(m_cacheMutex);
	m_cache.clear();
}
